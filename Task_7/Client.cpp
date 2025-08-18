#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <random>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <getopt.h>

constexpr int PORT = 12345;
constexpr const char* SERVER_IP = "127.0.0.1";
constexpr int BUFFER_SIZE = 1 << 16; // 64 KB

// Generate a random SIMPLE directed edge list (no self-loops, no duplicates).
// The server is expected to know V and E from the request header.
static std::string buildEdgesOnly(int V, int E, unsigned seed) {
    std::ostringstream out;
    std::mt19937 rng(seed); // deterministic RNG with seed
    std::uniform_int_distribution<int> dist(0, V - 1); // pick vertices uniformly
    std::set<std::pair<int,int>> edges; // store unique edges

    // Keep going until we have E unique directed edges
    while ((int)edges.size() < E) {
        int u = dist(rng), v = dist(rng);
        if (u == v) continue; // skip self-loops
        auto edge = std::make_pair(u, v); // keep directed edge (u->v)
        if (edges.count(edge)) continue; // skip duplicates
        edges.insert(edge);
        out << u << " " << v << "\n"; // write edge to output
    }
    return out.str();
}

// Helper function that ensures we send ALL bytes
static bool sendAll(int sock, const std::string& s) {
    const char* p = s.c_str();
    size_t left = s.size();
    while (left > 0) {
        ssize_t n = ::send(sock, p, left, 0);
        if (n <= 0) return false;
        p += n; left -= (size_t)n;
    }
    return true;
}

// Receive one chunk from the server (single recv).
static bool recvOnce(int sock, std::string& out) {
    char buf[BUFFER_SIZE];
    int n = ::recv(sock, buf, sizeof(buf)-1, 0);
    if (n <= 0) return false;
    buf[n] = '\0';
    out.assign(buf, n);
    return true;
}

// Print usage instructions to stderr
static void usage(const char* prog) {
    std::cerr << "Usage: " << prog << " -v <vertices> -e <edges> [-s <seed>]\n";
}

int main(int argc, char* argv[]) {
    int V = -1, E = -1;
    unsigned seed = 42;

    // Parse command-line options: -v, -e, -s
    int opt;
    while ((opt = getopt(argc, argv, "v:e:s:")) != -1) {
        switch (opt) {
            case 'v': V = std::stoi(optarg); break;
            case 'e': E = std::stoi(optarg); break;
            case 's': seed = (unsigned)std::stoul(optarg); break;
            default: usage(argv[0]); return 1;
        }
    }
    if (V <= 0 || E < 0) { usage(argv[0]); return 1; }

    // Connect once (persistent)
    int sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { std::cerr << "socket failed\n"; return 1; }

    sockaddr_in addr{}; addr.sin_family = AF_INET; addr.sin_port = htons(PORT);
    if (::inet_pton(AF_INET, SERVER_IP, &addr.sin_addr) <= 0) {
        std::cerr << "bad server ip\n"; return 1;
    }
    if (::connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "connect failed\n"; return 1;
    }

    std::cout << "Connected to server " << SERVER_IP << ":" << PORT << "\n";
    std::cout << "Enter algorithm name (euler|mst|scc|maxflow|hamilton), or 'quit' to exit.\n";

    std::string algo;
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, algo)) break;

        // trim leading/trailing spaces
        while (!algo.empty() && isspace((unsigned char)algo.back())) algo.pop_back();
        size_t i = 0; while (i < algo.size() && isspace((unsigned char)algo[i])) ++i;
        algo = algo.substr(i);
        if (algo.empty()) continue;

        // If user wants to quit, notify server and break
        if (algo == "quit" || algo == "QUIT" || algo == "Quit") {
            if (!sendAll(sock, std::string("quit\n"))) std::cerr << "send failed\n";
            break;
        }

        // Build a new set of E edges
        std::string edges = buildEdgesOnly(V, E, seed++);
        std::ostringstream req;
        req << algo << " " << V << " " << E << "\n" << edges;

        // Send request to server
        if (!sendAll(sock, req.str())) { std::cerr << "send failed\n"; break; }
        
        // Wait for server response and print it.
        std::string resp;
        if (!recvOnce(sock, resp)) { std::cerr << "server closed or recv failed\n"; break; }
        std::cout << resp;
    }

    ::close(sock); // Close socket before exiting
    return 0;
}
