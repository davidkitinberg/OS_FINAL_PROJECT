#include "MSTAlgorithm.h"
#include <vector>
#include <stack>
#include <sstream>

/**
 * DFS over the whole graph to test connectivity.
 * We consider all vertices; if any vertex is unreachable, the graph is disconnected.
 */
static bool isConnectedAllVertices(const Graph& g) {
    const int n = g.V();
    if (n == 0) return true; // vacuously connected
    if (n == 1) return true; // single vertex

    const std::vector<int>* adj = g.raw();
    std::vector<char> seen(n, 0);

    // Find a start vertex that exists (0..n-1). For unweighted adjacency, 0 is fine.
    int start = 0;
    std::stack<int> st;
    st.push(start);
    seen[start] = 1;

    while (!st.empty()) {
        int u = st.top(); st.pop();
        for (int v : adj[u]) {
            if (!seen[v]) {
                seen[v] = 1;
                st.push(v);
            }
        }
    }

    // If any vertex is unseen, the graph is disconnected
    for (int i = 0; i < n; ++i) {
        if (!seen[i]) return false;
    }
    return true;
}

std::string MSTAlgorithm::run(const Graph& g) {
    const int n = g.V();
    std::ostringstream out;

    if (n == 0) {
        out << "MST weight (unit): 0  (empty graph)\n";
        return out.str();
    }
    if (!isConnectedAllVertices(g)) {
        out << "MST does not exist: graph is disconnected (spanning tree requires one connected component).\n";
        return out.str();
    }

    // Unweighted MST (unit edges): weight equals number of edges in any spanning tree = V - 1
    out << "MST weight (unit): " << (n - 1) << "\n";
    return out.str();
}
