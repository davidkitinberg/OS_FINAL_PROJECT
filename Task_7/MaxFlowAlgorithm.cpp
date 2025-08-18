#include "MaxFlowAlgorithm.h"
#include "Graph.h"

#include <vector>
#include <queue>
#include <limits>
#include <sstream>

static int edmondsKarp_unitCap(const Graph& G, int s, int t) {
    const int n = G.V();
    if (n == 0 || s < 0 || t < 0 || s >= n || t >= n) return 0;
    if (s == t) return 0;

    // Build capacity matrix from Graph adjacency: each edge contributes capacity 1
    const std::vector<int>* adj = G.raw();
    std::vector<std::vector<int>> cap(n, std::vector<int>(n, 0));
    for (int u = 0; u < n; ++u) {
        for (int v : adj[u]) {
            // parallel edges sum capacities
            ++cap[u][v];
        }
    }

    int maxflow = 0;
    std::vector<int> parent(n, -1);

    auto bfs = [&](int source, int sink) -> int {
        std::fill(parent.begin(), parent.end(), -1);
        parent[source] = -2;  // sentinel
        std::queue<std::pair<int,int>> q; // (vertex, bottleneck so far)
        q.push({source, std::numeric_limits<int>::max()});

        while (!q.empty()) {
            int u = q.front().first;
            int flow_so_far = q.front().second;
            q.pop();

            for (int v = 0; v < n; ++v) {
                if (parent[v] == -1 && cap[u][v] > 0) {
                    parent[v] = u;
                    int new_flow = std::min(flow_so_far, cap[u][v]);
                    if (v == sink) return new_flow;
                    q.push({v, new_flow});
                }
            }
        }
        return 0; // no augmenting path
    };

    while (true) {
        int aug = bfs(s, t);
        if (aug == 0) break; // no more augmenting paths

        maxflow += aug;
        // backtrack and update residual capacities
        int v = t;
        while (v != s) {
            int u = parent[v];
            cap[u][v] -= aug;
            cap[v][u] += aug;
            v = u;
        }
    }

    return maxflow;
}

std::string MaxFlowAlgorithm::run(const Graph& g) {
    std::ostringstream out;
    const int n = g.V();
    if (n <= 1) {
        out << "Max flow (0->" << n-1 << ", unit capacities): 0";
        return out.str();
    }

    int flow = edmondsKarp_unitCap(g, /*source=*/0, /*sink=*/n-1);
    out << "Max flow (0->" << (n-1) << ", unit capacities): " << flow;
    return out.str();
}
