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
#include <cstdint>

constexpr int  PORT = 12345;
constexpr char SERVER_IP[] = "127.0.0.1";

// Helper
static bool writeAll(int fd, const void* buf, std::size_t n)
{
    const std::uint8_t* p = static_cast<const std::uint8_t*>(buf);
    while (n) {
        ssize_t w = write(fd, p, n);
        if (w <= 0) return false;
        p += w;  n -= static_cast<std::size_t>(w);
    }
    return true;
}
static bool readAll(int fd, void* buf, std::size_t n)
{
    std::uint8_t* p = static_cast<std::uint8_t*>(buf);
    while (n) {
        ssize_t r = read(fd, p, n);
        if (r <= 0) return false;
        p += r;  n -= static_cast<std::size_t>(r);
    }
    return true;
}

// Generates a simple random directed graph with V vertices and E edges
// Ensures no self-loops and no duplicate edges
static std::vector<std::pair<int,int>>
buildEdges(int V, int E, unsigned seed)
{
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> dist(0, V-1);
    std::set<std::pair<int,int>> s;
    while ((int)s.size() < E) {
        int u = dist(rng), v = dist(rng);
        if (u == v) continue; // no self-loops
        if (s.emplace(u,v).second) continue; // duplicate-safe insert
    }
    return {s.begin(), s.end()};
}

// Usage function
static void usage(const char* p) {
    std::cerr << "Usage: " << p << " -v <vertices> -e <edges> [-s seed]\n";
}

// Sends a request to the server for a given algorithm and graph, and prints the result
static void doRequest(int sock,
                      const std::string& algo,
                      int V, int E,
                      const std::vector<std::pair<int,int>>& edges)
{
    // Send number of vertices and edges (as network-order integers)
    std::int32_t v_net  = htonl(V);
    std::int32_t e_net  = htonl(E);
    if (!writeAll(sock, &v_net, 4) ||
        !writeAll(sock, &e_net, 4)) { throw std::runtime_error("send"); }

    // Send edge list
    for (auto [u,v] : edges) {
        std::int32_t u_net = htonl(u), v_net = htonl(v);
        if (!writeAll(sock, &u_net, 4) ||
            !writeAll(sock, &v_net, 4)) throw std::runtime_error("send");
    }

    // Send algorithm name as a string: first its length, then the raw characters
    std::int32_t len = htonl(static_cast<std::int32_t>(algo.size()));
    if (!writeAll(sock, &len, 4) ||
        !writeAll(sock, algo.data(), algo.size())) throw std::runtime_error("send");

    
    // Read a response from the server: [size][data] format
    auto readOne = [&]{
        std::int32_t n_net; if (!readAll(sock, &n_net, 4)) return std::string{};
        int n = ntohl(n_net);
        std::string s(n, '\0');
        if (!readAll(sock, s.data(), n)) return std::string{};
        return s;
    };

    // If running all algorithms, expect 4 results (one per algorithm)
    if (algo == "all") 
    {
        for (std::string name : {"mst","scc","maxflow","hamilton"})
        {
            std::cout << name << " → " << readOne();
        }
    } 
    else // Otherwise, just read one result
    {
        std::cout << readOne();
    }
}

int main(int argc, char* argv[])
{
    // Parse options: number of vertices, edges, and optional RNG seed
    int V=-1, E=-1; unsigned seed = 42;
    int opt;
    while ((opt = getopt(argc, argv, "v:e:s:")) != -1) {
        if (opt=='v') V = std::stoi(optarg);
        else if (opt=='e') E = std::stoi(optarg);
        else if (opt=='s') seed = std::stoul(optarg);
        else { usage(argv[0]); return 1; }
    }
    if (V<=0 || E<0) { usage(argv[0]); return 1; }

    // Create and connect TCP socket to the server
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{}; addr.sin_family = AF_INET; addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &addr.sin_addr);
    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect"); return 1;
    }
    std::cout << "Connected to " << SERVER_IP << ":" << PORT << '\n';

    // Loop: prompt for algorithm name and send request
    std::string algo;
    while (true) {
        std::cout << "algo> ";
        if (!std::getline(std::cin, algo) || algo=="quit") break;
        if (algo.empty()) continue;

        auto edges = buildEdges(V, E, seed); // Generate edges
        // Send request and handle errors
        try 
        { 
            doRequest(sock, algo, V, E, edges); 
        }
        catch (...) { 
            std::cerr << "connection lost\n"; break; 
        }
    }

    close(sock);
}
