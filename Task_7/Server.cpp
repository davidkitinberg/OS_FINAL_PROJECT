#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>
#include <cctype>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <algorithm>
#include <cerrno>

#include "Graph.h"
#include "AlgorithmFactory.h"
#include "GraphAlgorithm.h"

constexpr int PORT = 12345;
constexpr int BUFFER_SIZE = 1 << 16; // 64KB

static bool isDirectedAlgo(const std::string& algo) {
    // SCC and MaxFlow should use directed edges; others default to undirected.
    std::string a = algo;
    for (auto& c : a) c = std::tolower(static_cast<unsigned char>(c));
    return (a == "scc" || a == "maxflow");
}

static void writeAll(int fd, const std::string& s) {
    const char* p = s.c_str();
    size_t left = s.size();
    while (left > 0) {
        ssize_t n = write(fd, p, left);
        if (n <= 0) break;
        p += n; left -= static_cast<size_t>(n);
    }
}

// Keep the client connected; process multiple requests until "quit"
void handleClient(int clientSock) {
    std::vector<char> buf(BUFFER_SIZE, 0);

    while (true) {
        int bytesRead = recv(clientSock, buf.data(), buf.size() - 1, 0);
        if (bytesRead <= 0) {
            std::cerr << "Client disconnected or recv failed.\n";
            break;
        }
        buf[bytesRead] = '\0';

        std::istringstream input(std::string(buf.data(), bytesRead));

        // First token is algorithm name
        std::string algoName;
        if (!(input >> algoName)) {
            writeAll(clientSock, "Bad request: expected <algo> <v> <e> ...\n");
            continue; // stay connected and wait for the next request
        }

        // Quit command (case-insensitive)
        std::string lower = algoName;
        for (auto& c : lower) c = std::tolower(static_cast<unsigned char>(c));
        if (lower == "quit") {
            writeAll(clientSock, "bye\n");
            break; // close the connection
        }

        int v, e;
        if (!(input >> v >> e)) {
            writeAll(clientSock, "Bad request: expected <algo> <v> <e>\n");
            continue;
        }
        if (v <= 0 || e < 0) {
            writeAll(clientSock, "Invalid graph parameters.\n");
            continue;
        }

        Graph g(v);
        const bool directed = isDirectedAlgo(algoName);

        bool bad = false;
        for (int i = 0; i < e; ++i) {
            int u, w;
            if (!(input >> u >> w)) {
                writeAll(clientSock, "Bad request: not enough edges provided.\n");
                bad = true; break;
            }
            if (u < 0 || u >= v || w < 0 || w >= v) {
                writeAll(clientSock, "Invalid edge.\n");
                bad = true; break;
            }
            if (directed) g.addDirectedEdge(u, w);
            else g.addEdge(u, w);
        }
        if (bad) continue;

        // Strategy via Factory
        std::string response;
        try {
            auto algo = AlgorithmFactory::create(algoName);
            response = algo->run(g);
            response.push_back('\n');
        } catch (const std::exception& ex) {
            response = std::string("Error: ") + ex.what() + "\n";
        }

        writeAll(clientSock, response);
        // loop back and wait for the next request from the same client
    }

    close(clientSock);
    std::cout << "Client served and disconnected.\n";
}

int main() {
    // Create a TCP socket
    int serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock < 0) {
        std::cerr << "Failed to create socket.\n";
        return 1;
    }

    // Reuse address for quick restarts
    int yes = 1;
    setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    // Setup server address struct
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port   = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket to the address and port
    if (bind(serverSock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Bind failed.\n";
        return 1;
    }

    // Start listening for incoming connections
    if (listen(serverSock, 16) < 0) {
        std::cerr << "Listen failed.\n";
        return 1;
    }

    std::cout << "Server is running on port " << PORT << "\n";


    // make STDIN non-blocking so select() works cleanly
    int stdinFlags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (stdinFlags == -1 ||
        fcntl(STDIN_FILENO, F_SETFL, stdinFlags | O_NONBLOCK) == -1) {
        std::cerr << "Failed to set STDIN non-blocking.\n";
        close(serverSock);
        return 1;
    }


    // Keep accepting clients in an infinite loop
    bool running = true;
    while (running) {

        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(serverSock, &readfds);   // watch the listening socket
        FD_SET(STDIN_FILENO, &readfds); // watch STDIN for “quit”


        int maxFd = std::max(serverSock, STDIN_FILENO) + 1;
        int ready  = select(maxFd, &readfds, nullptr, nullptr, nullptr);
        if (ready < 0 && errno != EINTR) {
            std::cerr << "select() failed.\n";
            break;
        }

        // Check if user typed something on STDIN
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            std::string cmd;
            std::getline(std::cin, cmd); // safe: select() said data is ready
            if (cmd == "quit") {
                std::cout << "[Server] Shutdown requested\n";
                running = false; // leave loop after any active client
            }
        }

        // Wait for a client to connect
        if (running && FD_ISSET(serverSock, &readfds)) {
            sockaddr_in clientAddr{};
            socklen_t   clientLen = sizeof(clientAddr);
            int clientSock = accept(serverSock,
                                    (struct sockaddr*)&clientAddr,
                                    &clientLen);
            if (clientSock < 0) {
                std::cerr << "Accept failed.\n";
                continue;
            }
            std::cout << "Client connected.\n";

            // Handle this client (sequentially, not multi-threaded)
            handleClient(clientSock);
        }

    }

    close(serverSock);
    return 0;
}
