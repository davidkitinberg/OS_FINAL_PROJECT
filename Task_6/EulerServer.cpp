#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>
#include <netinet/in.h>
#include <unistd.h>

#include "Graph.h"
#include "EulerChecker.h"

constexpr int PORT = 12345;
constexpr int BUFFER_SIZE = 4096;

// Handle a single client request
void handleClient(int clientSock) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));

    // Read data sent from the client
    int bytesRead = read(clientSock, buffer, sizeof(buffer) - 1);
    if (bytesRead <= 0) {
        std::cerr << "Failed to read from client.\n";
        close(clientSock);
        return;
    }

    // Parse input as: number of vertices, number of edges, followed by edge list
    std::istringstream input(buffer);
    int v, e;
    input >> v >> e;

    // Check on graph parameters
    if (v <= 0 || e < 0) {
        std::string msg = "Invalid graph parameters.\n";
        write(clientSock, msg.c_str(), msg.size());
        close(clientSock);
        return;
    }

    // Build the graph
    Graph g(v);
    for (int i = 0; i < e; ++i) {
        int u, w;
        input >> u >> w;
        // Make sure edge endpoints are within valid range
        if (u < 0 || u >= v || w < 0 || w >= v) {
            std::string msg = "Invalid edge.\n";
            write(clientSock, msg.c_str(), msg.size());
            close(clientSock);
            return;
        }
        g.addEdge(u, w);
    }

    // Analyze the graph
    std::ostringstream response;
    int result = isEulerian(g);
    if (result == 2) {
        response << "Eulerian Circuit\n";
    } else if (result == 1) {
        response << "Eulerian Path\n";
    } else {
        response << "Not Eulerian\n";
    }

    // Send back the response
    std::string respStr = response.str();
    write(clientSock, respStr.c_str(), respStr.size());

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

    // Define server address info
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET; // IPv4
    serverAddr.sin_port = htons(PORT); // convert port to network byte order
    serverAddr.sin_addr.s_addr = INADDR_ANY; // bind to all available interfaces

    // Bind the socket to the given port
    if (bind(serverSock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Bind failed.\n";
        return 1;
    }

    // Put the socket into listening mode
    if (listen(serverSock, 5) < 0) {
        std::cerr << "Listen failed.\n";
        return 1;
    }

    std::cout << "Server is running on port " << PORT << "\n";

    // Keep accepting clients in an infinite loop
    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        // Wait for a client to connect
        int clientSock = accept(serverSock, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientSock < 0) {
            std::cerr << "Accept failed.\n";
            continue;
        }
        std::cout << "Client connected.\n";

        // Handle this client (sequentially, not multi-threaded)
        handleClient(clientSock);
    }

    // Close the server socket
    close(serverSock);
    return 0;
}
