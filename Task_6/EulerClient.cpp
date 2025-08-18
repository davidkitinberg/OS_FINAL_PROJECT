#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <random>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <algorithm>
#include <getopt.h>

constexpr int PORT = 12345;
constexpr const char* SERVER_IP = "127.0.0.1";
constexpr int BUFFER_SIZE = 4096;

// Build a random simple graph
std::string buildGraphInput(int V, int E, unsigned seed = 42) {
    std::ostringstream out;
    out << V << " " << E << "\n";

    std::mt19937 rng(seed); // deterministic random generator
    std::uniform_int_distribution<int> dist(0, V - 1);

    std::set<std::pair<int, int>> edges; // keep track of unique edges

    // Keep generating random edges until we have exactly E unique ones
    while ((int)edges.size() < E) {
        int u = dist(rng);
        int v = dist(rng);

        if (u == v) continue; // skip self-loops

        auto edge = std::minmax(u, v); // store smaller->larger to avoid duplicates
        if (edges.count(edge)) continue; // skip if edge already exists

        edges.insert(edge);
        out << edge.first << " " << edge.second << "\n";
    }

    return out.str();
}

// Usage help
void printUsage(const char* progName) {
    std::cerr << "Usage: " << progName << " -v <vertices> -e <edges>\n";
}

int main(int argc, char* argv[]) {
    int V = -1, E = -1;
    unsigned seed = 42;

    // Parse command line arguments
    int opt;
    while ((opt = getopt(argc, argv, "v:e:")) != -1) {
        switch (opt) {
            case 'v':
                V = std::stoi(optarg);
                break;
            case 'e':
                E = std::stoi(optarg);
                break;
            default:
                printUsage(argv[0]);
                return 1;
        }
    }

    // Validate arguments
    if (V <= 0 || E < 0 || E > V * (V - 1) / 2) {
        printUsage(argv[0]);
        std::cerr << "Invalid arguments: V must be > 0, 0 <= E <= V*(V-1)/2\n";
        return 1;
    }

    // Build the random graph input string
    std::string input = buildGraphInput(V, E, seed);

    // Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Failed to create socket\n";
        return 1;
    }

    // Configure server address
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);

    // Try to connect to the server
    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Connection to server failed\n";
        return 1;
    }

    std::cout << "Connected to server.\n";
    std::cout << "Graph sent:\n" << input;

    // Send graph data to the server
    send(sock, input.c_str(), input.size(), 0);

    // Receive server’s response
    char buffer[BUFFER_SIZE] = {0};
    int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived > 0) {
        std::cout << "Server response:\n" << buffer << "\n";
    } else {
        std::cerr << "No response from server.\n";
    }

    // Close the socket and exit
    close(sock);
    return 0;
}
