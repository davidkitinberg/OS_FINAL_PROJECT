#include "SCCAlgorithm.h"
#include "Graph.h"
#include <vector>
#include <stack>
#include <sstream>

// DFS to fill finish order (on original graph)
static void dfsOrder(int u, const std::vector<int>* adj, std::vector<char>& seen, std::vector<int>& order) {
    seen[u] = 1;
    for (int v : adj[u]) {
        if (!seen[v]) dfsOrder(v, adj, seen, order);
    }
    order.push_back(u); // finished u
}

// DFS on the reversed graph, collect one component
static void dfsCollect(int u, const std::vector<std::vector<int>>& radj, std::vector<char>& seen, std::vector<int>& comp) {
    seen[u] = 1;
    comp.push_back(u);
    for (int v : radj[u]) {
        if (!seen[v]) dfsCollect(v, radj, seen, comp);
    }
}

std::string SCCAlgorithm::run(const Graph& g) {
    const int n = g.V();
    const std::vector<int>* adj = g.raw();

    std::ostringstream out;
    if (n == 0) {
        out << "SCC count: 0 (empty graph)";
        return out.str();
    }

    // First DFS pass: get vertices in decreasing finish time
    std::vector<char> seen(n, 0);
    std::vector<int> order;
    order.reserve(n);

    for (int u = 0; u < n; ++u) {
        if (!seen[u]) dfsOrder(u, adj, seen, order);
    }

    // Build reversed graph
    std::vector<std::vector<int>> radj(n);
    for (int u = 0; u < n; ++u) {
        for (int v : adj[u]) {
            // reverse edge v -> u
            radj[v].push_back(u);
        }
    }

    // Second pass: process in reverse finish order on reversed graph
    std::fill(seen.begin(), seen.end(), 0);
    std::vector<std::vector<int>> components;

    for (int i = n - 1; i >= 0; --i) {
        int u = order[i];
        if (!seen[u]) {
            std::vector<int> comp;
            dfsCollect(u, radj, seen, comp);
            components.push_back(std::move(comp));
        }
    }

    // Format output
    out << "SCC count: " << components.size() << "\n";
    for (size_t i = 0; i < components.size(); ++i) {
        out << "SCC " << i << ": ";
        for (size_t j = 0; j < components[i].size(); ++j) {
            out << components[i][j] << (j + 1 == components[i].size() ? "" : " ");
        }
        out << "\n";
    }
    return out.str();
}
