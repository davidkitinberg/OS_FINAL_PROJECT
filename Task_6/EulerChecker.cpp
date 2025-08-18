#include "EulerChecker.h"
#include <vector>
#include <iostream>

using std::vector;

// DFS to explore the graph
static void dfs(int u, const vector<int> adj[], vector<bool>& visited) {
    visited[u] = true;
    for (int v : adj[u]) {
        if (!visited[v]) {
            dfs(v, adj, visited);
        }
    }
}

// Main function to check Eulerian status
int isEulerian(const Graph& g) {
    int v = g.V();
    const vector<int>* adj = g.raw();

    vector<bool> visited(v, false);

    // Find a vertex with non-zero degree to start DFS
    int start = -1;
    for (int i = 0; i < v; i++) {
        if (!adj[i].empty()) {
            start = i;
            break;
        }
    }

    // No edges at all → trivially Eulerian Circuit
    if (start == -1) {
        return 2;
    }

    // Check connectivity (ignoring isolated vertices)
    dfs(start, adj, visited);
    for (int i = 0; i < v; i++) {
        if (!adj[i].empty() && !visited[i]) {
            std::cout << "Graph is not connected (ignoring isolated vertices).\n";
            return 0;
        }
    }

    // Count number of vertices with odd degree
    int odd = 0;
    vector<int> oddVertices;
    for (int i = 0; i < v; i++) {
        if (adj[i].size() % 2 != 0) {
            odd++;
            oddVertices.push_back(i);
        }
    }

    if (odd == 0) {
        return 2; // Eulerian circuit
    } else {
        std::cout << "Vertices with odd degree: ";
        for (int u : oddVertices) {
            std::cout << u << " ";
        }
        std::cout << std::endl;

        if (odd == 2) return 1; // Eulerian path
        return 0; // Neither
    }
}
