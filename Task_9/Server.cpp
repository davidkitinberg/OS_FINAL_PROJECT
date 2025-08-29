#include <iostream>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <map>
#include <string>
#include <cstring>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdint>

#include "Graph.h"
#include "AlgorithmFactory.h"

constexpr int   PORT = 12345;
constexpr size_t BUF = 1 << 16;

// Helper functions for I/O
static bool readAll(int fd, void* buf, size_t n) {
    auto* p = static_cast<uint8_t*>(buf);
    while (n) {
        ssize_t r = ::read(fd, p, n);
        if (r <= 0) return false;
        p += r; 
        n -= r;
    }
    return true;
}
static bool writeAll(int fd, const void* buf, size_t n) {
    const auto* p = static_cast<const uint8_t*>(buf);
    while (n) {
        ssize_t w = ::write(fd, p, n);
        if (w <= 0) return false;
        p += w; 
        n -= w;
    }
    return true;
}

// Thread-safe queue data structure helper
template<typename T>
class ThreadQueue {
    std::queue<T> q;
    std::mutex m;
    std::condition_variable cv;
public:
    void push(T v) {
        { std::lock_guard<std::mutex> lk(m); q.push(std::move(v)); }
        cv.notify_one();
    }
    bool pop(T& out, const std::atomic<bool>& down) {
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk, [&]{ return !q.empty() || down; });
        if (q.empty()) return false;
        out = std::move(q.front());
        q.pop();
        return true;
    }
};

// Task Struct represents a unit of work passed between pipeline stages
struct Task {
    int clientFd; // Client socket
    std::string algorithm; // Algorithm name
    Graph graph; // Graph to run algorithm on

    // default sentinel → makes the type default-constructible
    Task() : clientFd(-1), algorithm(), graph(0) {}
    Task(int fd, std::string alg, const Graph& g)
        : clientFd(fd), algorithm(std::move(alg)), graph(g) {}
};

std::atomic<bool> shuttingDown{false};

// One queue for each algorithm worker
ThreadQueue<Task> algoQ[4]; // 0 mst 1 scc 2 maxflow 3 hamilton
ThreadQueue<std::pair<int,std::string>> resultQ; // Shared result queue (for response stage)

////////////////// Worker Threads //////////////////

// Algorithm computation stage
void algorithmWorker(int idx, const std::string& name)
{
    Task t;
    while (algoQ[idx].pop(t, shuttingDown)) 
    {
        if (t.clientFd == -1) break; // sentinel
        auto alg = AlgorithmFactory::create(name);
        std::string res = alg->run(t.graph); // Run algorithm
        resultQ.push({t.clientFd, std::move(res)}); // Push result to responder
    }
}

// Sends responses back to client
void responseWorker()
{
    std::pair<int,std::string> job;
    while (resultQ.pop(job, shuttingDown)) // Wait for result
    {
        if (job.first == -1) break; // sentinel
        int32_t len = htonl(job.second.size());
        if (!writeAll(job.first, &len, 4) ||
            !writeAll(job.first, job.second.data(), job.second.size()))
            close(job.first); // Close socket on failure
    }
}

// Initial Receiver Thread: First stage of pipeline: accepts and parses client requests
void connectionHandler(int cfd)
{
    while (!shuttingDown) {

        // Read number of vertices and edges
        int32_t v_net, e_net;
        if (!readAll(cfd, &v_net, 4) || !readAll(cfd, &e_net, 4)) break;
        int V = ntohl(v_net), E = ntohl(e_net);

        // Build graph from edge list
        Graph g(V);
        for (int i = 0; i < E; ++i) {
            int32_t u_net, v2_net;
            if (!readAll(cfd, &u_net, 4) || !readAll(cfd, &v2_net, 4)) { close(cfd); return; }
            g.addEdge(ntohl(u_net), ntohl(v2_net));
        }

        // Read algorithm name (length-prefixed)
        int32_t len_net; 
        if (!readAll(cfd, &len_net, 4)) break;
        std::string algo(ntohl(len_net), '\0');
        if (!readAll(cfd, algo.data(), algo.size())) break;

        // Handle "all" command: send same graph to all algorithm workers
        if (algo == "all") 
        {
            for (const auto& [i,n] : std::map<int,std::string>{{0,"mst"},{1,"scc"},{2,"maxflow"},{3,"hamilton"}})
                algoQ[i].push({cfd, n, g});
        } 
        else // Map algorithm name to queue index
        {
            int i = (algo=="mst")?0:(algo=="scc")?1:(algo=="maxflow")?2:(algo=="hamilton")?3:-1;
            if (i >= 0) algoQ[i].push({cfd, algo, g});
        }
    }
    close(cfd);
}

// ─────────────── stdin “quit” watcher ───────────────
void stdinWatcher(int listenFd)
{
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == "quit") {
            std::cout << "[Server] Shutdown requested\n";
            shuttingDown = true;
            shutdown(listenFd, SHUT_RDWR);
            close(listenFd);

            for (auto& q : algoQ) q.push(Task{});       // poison pills
            resultQ.push({-1,""});
            return;
        }
    }
}


int main()
{
    // Create socket
    int srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv < 0) { perror("socket"); return 1; }

    // Allow immediate reuse of the port after the server terminates
    int opt = 1; 
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Setup server address struct
    sockaddr_in addr{}; 
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT); 
    addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket to the address and port
    if (bind(srv,(sockaddr*)&addr,sizeof(addr)) < 0) { 
        perror("bind"); return 1; 
    }

    // Start listening for incoming connections
    if (listen(srv,10) < 0) { perror("listen"); return 1; }

    std::cout << "[Server] listening on " << PORT << '\n';

    // Start worker threads
    std::vector<std::thread> th;
    th.emplace_back(stdinWatcher, srv);
    th.emplace_back(responseWorker);
    th.emplace_back(algorithmWorker,0,"mst");
    th.emplace_back(algorithmWorker,1,"scc");
    th.emplace_back(algorithmWorker,2,"maxflow");
    th.emplace_back(algorithmWorker,3,"hamilton");

    // Accept client connections in loop
    while (!shuttingDown) {
        sockaddr_in cli; 
        socklen_t cl = sizeof(cli);
        int c = accept(srv,(sockaddr*)&cli,&cl);
        if (c < 0) {
            if (shuttingDown) break;
            perror("accept"); continue;
        }
        th.emplace_back(connectionHandler,c); // Start receiver thread per client
    }

    for (auto& t : th) t.join();
    return 0;
}
