#include "EulerAlgorithm.h"
#include "Graph.h"
#include <vector>
#include <queue>
#include <sstream>

using std::vector;

static void dfs(int u, const vector<int> adj[], vector<char>& seen) {
    std::queue<int> q;
    q.push(u);
    seen[u] = 1;
    while (!q.empty()) {
        int x = q.front(); q.pop();
        for (int v : adj[x]) if (!seen[v]) { seen[v] = 1; q.push(v); }
    }
}

std::string EulerAlgorithm::run(const Graph& g) {
    std::ostringstream out;
    const int n = g.V();
    const vector<int>* adj = g.raw();

    // Find a start vertex with non-zero degree
    int start = -1;
    for (int i = 0; i < n; ++i) if (!adj[i].empty()) { start = i; break; }

    // No edges at all → trivially Eulerian Circuit
    if (start == -1) {
        out << "Eulerian Circuit";
        return out.str();
    }

    // Check connectivity (ignoring isolated vertices)
    vector<char> seen(n, 0);
    dfs(start, adj, seen);
    for (int i = 0; i < n; ++i) {
        if (!adj[i].empty() && !seen[i]) {
            out << "Not Eulerian\nGraph is not connected (ignoring isolated vertices).";
            return out.str();
        }
    }

    // Count odd-degree vertices
    int odd = 0;
    vector<int> oddVertices;
    for (int i = 0; i < n; ++i) {
        if ((int)adj[i].size() % 2 != 0) {
            ++odd;
            oddVertices.push_back(i);
        }
    }

    if (odd == 0) {
        out << "Eulerian Circuit";
    } else if (odd == 2) {
        out << "Eulerian Path\nVertices with odd degree: ";
        for (int u : oddVertices) out << u << ' ';
    } else {
        out << "Not Eulerian\nVertices with odd degree: ";
        for (int u : oddVertices) out << u << ' ';
    }
    return out.str();
}
