#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <map>
#include <memory>
#include <string>
#include <cstring>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdint>

#include "Graph.h"
#include "AlgorithmFactory.h"

constexpr int PORT = 12345;
constexpr size_t BUFFER_SIZE = 1 << 16;

// Helper functions for I/O
static bool readAll(int fd, void* buf, size_t n) {
    uint8_t* p = static_cast<uint8_t*>(buf);
    while (n) {
        ssize_t r = read(fd, p, n);
        if (r <= 0) return false;
        p += r; n -= r;
    }
    return true;
}
static bool writeAll(int fd, const void* buf, size_t n) {
    const uint8_t* p = static_cast<const uint8_t*>(buf);
    while (n) {
        ssize_t w = write(fd, p, n);
        if (w <= 0) return false;
        p += w; n -= w;
    }
    return true;
}


// Thread-safe producer-consumer queue
template<typename T>
class ThreadQueue {
    std::queue<T> q;
    std::mutex m;
    std::condition_variable cv;
public:
    void push(const T& val) {
        std::lock_guard<std::mutex> lock(m);
        q.push(val);
        cv.notify_one(); // Wake up one waiting consumer
    }
    T pop() {
        std::unique_lock<std::mutex> lock(m);
        cv.wait(lock, [&]{ return !q.empty(); }); // Wait for data
        T val = std::move(q.front()); q.pop();
        return val;
    }
};


// Task Struct represents a unit of work passed between pipeline stages
struct Task {
    int clientFd; // Client socket
    std::string algorithm; // Algorithm name
    Graph graph; // Graph to run algorithm on
};

// One queue for each algorithm worker
ThreadQueue<Task> algoQueues[4]; // 0=mst, 1=scc, 2=maxflow, 3=hamilton

// Shared result queue (for response stage)
ThreadQueue<std::pair<int, std::string>> resultQueue;

////////////////// Worker Threads //////////////////

// Algorithm computation stage
void algorithmWorker(int index, const std::string& algoName) {
    while (true) {
        Task task = algoQueues[index].pop(); // Wait for task
        auto alg = AlgorithmFactory::create(algoName);
        std::string res = alg->run(task.graph); // Run algorithm
        resultQueue.push({task.clientFd, res}); // Push result to responder
    }
}

// Final stage: sends responses back to client
void responseWorker() {
    while (true) {
        auto [fd, result] = resultQueue.pop(); // Wait for result
        int32_t len = htonl(result.size());
        if (!writeAll(fd, &len, 4) || !writeAll(fd, result.data(), result.size())) {
            close(fd); // Close socket on failure
            std::cout << "[Server] Disconnected client on write\n";
        }
    }
}


// Initial Receiver Thread: First stage of pipeline: accepts and parses client requests
void connectionHandler(int clientFd) {
    while (true) {
        // Read number of vertices and edges
        int32_t v_net, e_net;
        if (!readAll(clientFd, &v_net, 4) || !readAll(clientFd, &e_net, 4)) break;
        int V = ntohl(v_net), E = ntohl(e_net);

        // Build graph from edge list
        Graph g(V);
        for (int i = 0; i < E; ++i) {
            int32_t u_net, v2_net;
            if (!readAll(clientFd, &u_net, 4) || !readAll(clientFd, &v2_net, 4)) return;
            g.addEdge(ntohl(u_net), ntohl(v2_net));
        }

        // Read algorithm name (length-prefixed)
        int32_t len_net;
        if (!readAll(clientFd, &len_net, 4)) break;
        int len = ntohl(len_net);

        std::string algo(len, '\0');
        if (!readAll(clientFd, algo.data(), len)) break;

        // Handle "all" command: send same graph to all algorithm workers
        if (algo == "all") 
        {
            for (const auto& [index, name] : std::map<int, std::string>{{0,"mst"},{1,"scc"},{2,"maxflow"},{3,"hamilton"}}) {
                algoQueues[index].push(Task{clientFd, name, g});
            }
        } 
        else // Map algorithm name to queue index
        {
            int idx = (algo == "mst") ? 0 :
                      (algo == "scc") ? 1 :
                      (algo == "maxflow") ? 2 :
                      (algo == "hamilton") ? 3 : -1;
            if (idx == -1) continue; // Invalid algo, skip
            algoQueues[idx].push(Task{clientFd, algo, g});
        }
    }
    close(clientFd);
    std::cout << "[Server] Client disconnected\n";
}

// ---- Main Server ----
int main() {
    // Create socket
    int srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv < 0) { perror("socket"); return 1; }

    // Setup address
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    // Allow quick reuse of port
    int opt = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Bind and listen
    if (bind(srv, (sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); return 1; }
    if (listen(srv, 10) < 0) { perror("listen"); return 1; }

    std::cout << "[Server] listening on " << PORT << '\n';

    // Start worker threads
    std::thread(responseWorker).detach();
    std::thread(algorithmWorker, 0, "mst").detach();
    std::thread(algorithmWorker, 1, "scc").detach();
    std::thread(algorithmWorker, 2, "maxflow").detach();
    std::thread(algorithmWorker, 3, "hamilton").detach();

    // Accept client connections in loop
    while (true) {
        sockaddr_in cli;
        socklen_t len = sizeof(cli);
        int c = accept(srv, (sockaddr*)&cli, &len);
        if (c < 0) { perror("accept"); continue; }

        // Start receiver thread per client
        std::thread(connectionHandler, c).detach();
    }
}
