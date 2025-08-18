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
#include <algorithm>      // std::remove
#include <fcntl.h>        // O_NONBLOCK
#include <errno.h>
#include <chrono>
#include <sys/socket.h>   // shutdown()
#include "Graph.h"
#include "AlgorithmFactory.h"

constexpr int PORT = 12345;
constexpr int THREAD_COUNT = 4;

// Leader-Follower coordination
std::mutex              leader_mutex;
std::condition_variable leader_cv;
bool                    leader_active = false;

// --- graceful-shutdown bookkeeping ---
std::atomic<bool> g_stop{false};
int               g_listen_fd = -1;
std::mutex        clients_mtx;
std::vector<int>  clients;
// --------------------------------------

// ---------- helpers ----------
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
static void addClient(int fd) {
    std::lock_guard<std::mutex> g(clients_mtx);
    clients.push_back(fd);
}
static void delClient(int fd) {
    std::lock_guard<std::mutex> g(clients_mtx);
    clients.erase(std::remove(clients.begin(), clients.end(), fd), clients.end());
}
// ------------------------------

// Handles a single client request
void handleClient(int sock) {
    addClient(sock);

    while (true) {
        int32_t v_net, e_net;
        if (!readAll(sock, &v_net, 4) || !readAll(sock, &e_net, 4)) break;
        int V = ntohl(v_net), E = ntohl(e_net);

        Graph g(V);
        for (int i = 0; i < E; ++i) {
            int32_t u_net, v2_net;
            if (!readAll(sock, &u_net, 4) || !readAll(sock, &v2_net, 4)) {
                shutdown(sock, SHUT_RDWR); close(sock); delClient(sock); return;
            }
            g.addEdge(ntohl(u_net), ntohl(v2_net));
        }

        int32_t len_net;
        if (!readAll(sock, &len_net, 4)) break;
        std::string algo(ntohl(len_net), '\0');
        if (!readAll(sock, algo.data(), algo.size())) break;
        if (algo == "quit") break;

        if (algo == "all") {
            for (std::string name : {"mst","scc","maxflow","hamilton"}) {
                auto alg = AlgorithmFactory::create(name);
                std::string res = alg->run(g);
                int32_t n = htonl(static_cast<int32_t>(res.size()));
                if (!writeAll(sock,&n,4)||!writeAll(sock,res.data(),res.size())){
                    shutdown(sock, SHUT_RDWR); close(sock); delClient(sock); return;
                }
            }
        } else {
            auto alg = AlgorithmFactory::create(algo);
            std::string res = alg->run(g);
            int32_t n = htonl(static_cast<int32_t>(res.size()));
            if (!writeAll(sock,&n,4)||!writeAll(sock,res.data(),res.size())){
                shutdown(sock, SHUT_RDWR); close(sock); delClient(sock); return;
            }
        }
    }

    shutdown(sock, SHUT_RDWR); close(sock);
    delClient(sock);
    std::cout << "[Server] Client disconnected\n";
}

// Leader-Follower worker
void lf_worker(int listen_fd) {
    while (!g_stop.load()) {
        {   std::unique_lock<std::mutex> lk(leader_mutex);
            leader_cv.wait(lk,[]{return !leader_active||g_stop.load();});
            if (g_stop.load()) break;
            leader_active = true;
        }

        int client_fd = ::accept(listen_fd,nullptr,nullptr);
        if (client_fd < 0) {
            if (g_stop.load()) break;
            perror("accept");
            { std::lock_guard<std::mutex> g(leader_mutex);
              leader_active = false; leader_cv.notify_one(); }
            continue;
        }

        {   std::lock_guard<std::mutex> g(leader_mutex);
            leader_active = false; leader_cv.notify_one(); }

        handleClient(client_fd);
    }
}

// Watch STDIN for “exit”
void stdinWatcher() {
    int fl = ::fcntl(STDIN_FILENO,F_GETFL,0);
    ::fcntl(STDIN_FILENO,F_SETFL,fl|O_NONBLOCK);

    std::string buf; char tmp[128];
    while (!g_stop.load()) {
        ssize_t n = ::read(STDIN_FILENO,tmp,sizeof(tmp));
        if (n > 0) {
            buf.append(tmp,n);
            size_t pos;
            while ((pos=buf.find('\n'))!=std::string::npos) {
                std::string line=buf.substr(0,pos); buf.erase(0,pos+1);
                if (line=="exit") {
                    std::cout<<"[Server] Shutting down...\n";
                    g_stop = true;

                    if (g_listen_fd!=-1) { shutdown(g_listen_fd,SHUT_RDWR); close(g_listen_fd); }

                    { std::lock_guard<std::mutex> g(clients_mtx);
                      for(int fd:clients){ shutdown(fd,SHUT_RDWR); close(fd);} }

                    { std::lock_guard<std::mutex> g(leader_mutex);
                      leader_cv.notify_all(); }        // wake LF sleepers
                    return;
                }
            }
        } else if (n==0||(n==-1&&errno!=EAGAIN&&errno!=EWOULDBLOCK)) {
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

int main() {
    int listen_fd = ::socket(AF_INET,SOCK_STREAM,0);
    if (listen_fd<0){ perror("socket"); return 1; }
    g_listen_fd = listen_fd;

    sockaddr_in addr{}; addr.sin_family=AF_INET;
    addr.sin_port=htons(PORT); addr.sin_addr.s_addr=INADDR_ANY;
    int opt=1; ::setsockopt(listen_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    if (::bind(listen_fd,reinterpret_cast<sockaddr*>(&addr),sizeof(addr))<0){
        perror("bind"); return 1;
    }
    if (::listen(listen_fd,10)<0){ perror("listen"); return 1; }

    std::cout<<"[Server] Listening on port "<<PORT<<std::endl;

    std::thread stdin_thread(stdinWatcher);

    std::vector<std::thread> workers;
    for(int i=0;i<THREAD_COUNT;++i) workers.emplace_back(lf_worker,listen_fd);

    for(auto& t:workers) t.join();
    stdin_thread.join();

    std::cout<<"[Server] Shutdown complete\n";
    return 0;
}
