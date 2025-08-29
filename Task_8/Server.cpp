#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <netinet/in.h>
#include <unistd.h>
#include <atomic>
#include <cstring>
#include <cstdint>
#include <algorithm>
#include <fcntl.h>
#include <errno.h>
#include <chrono>
#include <sys/socket.h>
#include "Graph.h"
#include "AlgorithmFactory.h"

constexpr int PORT = 12345;
constexpr int THREAD_COUNT = 4;

// Leader-Follower coordination
std::mutex leader_mutex;
std::condition_variable leader_cv;
bool leader_active = false;

// Shutdown helper flags
std::atomic<bool> g_stop{false}; // Global atomic flag set to true when the server is shutting down.
int g_listen_fd = -1; // The listening socket file descriptor. We close this during shutdown to immediately unblock any thread stuck in accept().
std::mutex clients_mtx; // Protects the 'clients' container below. Always lock this mutex before reading/modifying 'clients'.
std::vector<int> clients; // List of currently active client socket FDs


// Helper functions for I/O
static bool readAll(int fd, void* buf, size_t n) {
    auto* p = static_cast<uint8_t*>(buf);
    while (n) {
        ssize_t r = ::read(fd, p, n);
        if (r <= 0) return false;
        p += r; n -= static_cast<size_t>(r);
    }
    return true;
}
static bool writeAll(int fd, const void* buf, size_t n) {
    auto* p = static_cast<const uint8_t*>(buf);
    while (n) {
        ssize_t w = ::write(fd, p, n);
        if (w <= 0) return false;
        p += w; n -= static_cast<size_t>(w);
    }
    return true;
}


// Helper function for client handling
static void addClient(int fd) {
    std::lock_guard<std::mutex> g(clients_mtx);
    clients.push_back(fd);
}
static void delClient(int fd) {
    std::lock_guard<std::mutex> g(clients_mtx);
    clients.erase(std::remove(clients.begin(), clients.end(), fd), clients.end());
}


// Handles a single client request
void handleClient(int sock) {
    addClient(sock);

    while (true) {
        // Read number of vertices and edges from client
        int32_t v_net, e_net;
        if (!readAll(sock, &v_net, 4) || !readAll(sock, &e_net, 4)) break;
        int V = ntohl(v_net), E = ntohl(e_net);

        // Construct graph from edge list
        Graph g(V);
        for (int i = 0; i < E; ++i) {
            int32_t u_net, v2_net;
            if (!readAll(sock, &u_net, 4) || !readAll(sock, &v2_net, 4)) {
                shutdown(sock, SHUT_RDWR); 
                close(sock); // Client disconnected mid-read
                delClient(sock); return;
            }
            g.addEdge(ntohl(u_net), ntohl(v2_net));
        }

        // Read algorithm name (length-prefixed string)
        int32_t len_net;
        if (!readAll(sock, &len_net, 4)) break;
        std::string algo(ntohl(len_net), '\0');
        if (!readAll(sock, algo.data(), algo.size())) break;


        if (algo == "quit") break; // Exit loop if client sends "quit"

        // Run all 4 algorithms if "all" is requested
        if (algo == "all") 
        {
            for (std::string name : {"mst","scc","maxflow","hamilton"}) 
            {
                auto alg = AlgorithmFactory::create(name);
                std::string res = alg->run(g);
                int32_t n = htonl(static_cast<int32_t>(res.size()));
                if (!writeAll(sock,&n,4)||!writeAll(sock,res.data(),res.size())) {
                    shutdown(sock, SHUT_RDWR); // Close socket from both ends
                    close(sock); // Client disconnected mid-write
                    delClient(sock); // Remove active client from the List of active clients
                    return;
                }
            }
        } 
        else // Run specific requested algorithm
        {
            auto alg = AlgorithmFactory::create(algo);
            std::string res = alg->run(g);
            int32_t n = htonl(static_cast<int32_t>(res.size()));
            if (!writeAll(sock,&n,4)||!writeAll(sock,res.data(),res.size())){
                shutdown(sock, SHUT_RDWR); // Close socket from both ends
                close(sock); // Client disconnected mid-write
                delClient(sock); // Remove active client from the List of active clients
                return;
            }
        }
    }

    // Clean up when client disconnects or sends "quit"
    shutdown(sock, SHUT_RDWR); // Close socket from both ends
    close(sock); // Close socket
    delClient(sock); // Remove active client from the List of active clients
    std::cout << "[Server] Client disconnected\n";
}

// Leader-Follower thread logic
void lf_worker(int listen_fd) {
    while (!g_stop.load()) 
    {
        // Wait to become leader
        {   
            std::unique_lock<std::mutex> lk(leader_mutex);
            leader_cv.wait(lk,[]{return !leader_active||g_stop.load();}); // Wait until no leader is active
            if (g_stop.load()) break; // Shut down atomic flag
            leader_active = true; // This thread becomes the Leader
        }

        // Accept connection
        int client_fd = accept(listen_fd,nullptr,nullptr);
        if (client_fd < 0) 
        {
            if (g_stop.load()) break;
                perror("accept");
            { 
                std::lock_guard<std::mutex> g(leader_mutex);
                leader_active = false; // Demote leader
                leader_cv.notify_one(); // Wake up one waiting thread to become the next Leader
            }
            continue; // Try again on failure
        }

        // Promote new leader
        // Release leadership so another thread can call accept()
        {   
            std::lock_guard<std::mutex> g(leader_mutex);
            leader_active = false; // Demote leader
            leader_cv.notify_one(); // Wake up one waiting thread to become the next Leader
        }

        handleClient(client_fd); // Handle request as worker (this thread was the leader)
    }
}

// Watch STDIN for “exit”
void stdinWatcher() {
    // Set STDIN to non-blocking mode
    int fl = fcntl(STDIN_FILENO,F_GETFL,0);
    fcntl(STDIN_FILENO,F_SETFL,fl|O_NONBLOCK);

    std::string buf; 
    char tmp[128];

    // Keep checking STDIN unless shutdown was triggered externally
    while (!g_stop.load()) 
    {
        ssize_t n = read(STDIN_FILENO,tmp,sizeof(tmp));

        if (n > 0) 
        {
            buf.append(tmp,n); // Append what was read to a buffer
            size_t pos;
            // Extract lines one-by-one (if multiple lines were entered)
            while ((pos=buf.find('\n'))!=std::string::npos) 
            {
                std::string line=buf.substr(0,pos); buf.erase(0,pos+1);
                // If the user types "exit", initiate graceful shutdown
                if (line=="exit") 
                {
                    std::cout<<"[Server] Shutting down...\n";
                    g_stop = true;

                    // Shutdown the listening socket to unblock accept()
                    if (g_listen_fd!=-1) 
                    { 
                        shutdown(g_listen_fd,SHUT_RDWR); 
                        close(g_listen_fd); 
                    }

                    // Shutdown and close all active client sockets
                    { 
                        std::lock_guard<std::mutex> g(clients_mtx);
                        for(int fd:clients) { 
                            shutdown(fd,SHUT_RDWR); 
                            close(fd);
                        } 
                    }

                    // Wake up all LF threads that may be waiting on condition variable
                    { 
                        std::lock_guard<std::mutex> g(leader_mutex);
                        leader_cv.notify_all(); // wake LF sleepers
                    }        
                    return; // Exit the watcher thread
                }
            }
        } 
        // Handle EOF or fatal read error
        else if (n==0||(n==-1&&errno!=EAGAIN&&errno!=EWOULDBLOCK)) {
            return;
        }
        // Prevent busy waiting: sleep briefly before checking STDIN again
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

int main() {

    // Create TCP socket for listening
    int listen_fd = socket(AF_INET,SOCK_STREAM,0);
    if (listen_fd<0) { 
        perror("socket"); return 1; 
    }

    g_listen_fd = listen_fd;

    // Setup server address struct
    sockaddr_in addr{}; 
    addr.sin_family=AF_INET;
    addr.sin_port=htons(PORT); // Bind to specified port
    addr.sin_addr.s_addr=INADDR_ANY; // Accept connections on any interface
    
    // Allow immediate reuse of the port after the server terminates
    int opt=1; 
    setsockopt(listen_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

    // Bind the socket to the address and port
    if (bind(listen_fd,reinterpret_cast<sockaddr*>(&addr),sizeof(addr))<0){
        perror("bind"); return 1;
    }

    // Start listening for incoming connections
    if (listen(listen_fd,10)<0){ perror("listen"); return 1; }

    std::cout<<"[Server] Listening on port "<<PORT<<std::endl;

    // Create new thead only for STDIN
    std::thread stdin_thread(stdinWatcher);

    // Create and launch a fixed number of LF worker threads
    std::vector<std::thread> workers;
    for(int i=0;i<THREAD_COUNT;++i) 
    {
        workers.emplace_back(lf_worker,listen_fd); // Each thread runs the LF worker logic
    }
        
    for(auto& t:workers) t.join();


    stdin_thread.join();

    std::cout<<"[Server] Shutdown complete\n";
    return 0;
}
