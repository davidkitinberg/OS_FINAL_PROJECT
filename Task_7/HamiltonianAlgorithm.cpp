#include "HamiltonianAlgorithm.h"
#include "Graph.h"
#include <vector>
#include <sstream>

// Build an adjacency matrix so we can check if an edge exists in O(1).
static std::vector<std::vector<char>> buildAdjMatrix(const Graph& g) {
    int n = g.V();
    const std::vector<int>* adj = g.raw();
    std::vector<std::vector<char>> A(n, std::vector<char>(n, 0));
    for (int u = 0; u < n; ++u) {
        for (int v : adj[u]) A[u][v] = 1;
    }
    return A;
}

// Backtracking helper: try to place vertex at position `pos` in path.
// path[0] is fixed to 0 to avoid counting rotations of the same cycle.
// `used[v]` marks if v is already in the path.
// If we manage to place all vertices and there’s an edge back to the start,
// we’ve found a Hamiltonian cycle.
static bool backtrackHamilton(
    const std::vector<std::vector<char>>& A,
    std::vector<int>& path,
    std::vector<char>& used,
    int pos
) {
    const int n = (int)A.size();
    if (pos == n) {
        // All vertices are placed, now check if the last one connects back to 0.
        return A[path[n - 1]][path[0]] != 0;
    }

    // Try every possible next vertex
    for (int v = 1; v < n; ++v) // skip 0 since it's already fixed at start
    {
        if (!used[v] && A[path[pos - 1]][v]) {
            used[v] = 1;
            path[pos] = v;
            if (backtrackHamilton(A, path, used, pos + 1)) return true;
            used[v] = 0; // undo choice if it didn’t work
        }
    }
    return false;
}

std::string HamiltonianAlgorithm::run(const Graph& g) {
    std::ostringstream out;
    const int n = g.V();

    if (n == 0) { out << "No Hamiltonian circuit (empty graph)"; return out.str(); }
    if (n == 1) { out << "Hamiltonian circuit: 0 -> 0"; return out.str(); }

    auto A = buildAdjMatrix(g);

    // For an undirected Hamiltonian cycle, every vertex should have degree >= 2 (this isn’t a complete test).
    {
        const std::vector<int>* adj = g.raw();
        bool obviouslyNo = false;
        for (int u = 0; u < n; ++u) 
        {
            if ((int)adj[u].size() < 1) { obviouslyNo = true; break; }
        }
        if (obviouslyNo) {
            out << "No Hamiltonian circuit";
            return out.str();
        }
    }

    std::vector<int> path(n, -1);
    std::vector<char> used(n, 0);

    path[0] = 0; // Fix start to 0 (break rotational symmetry)
    used[0] = 1;

    if (backtrackHamilton(A, path, used, 1)) {
        out << "Hamiltonian circuit: ";
        for (int i = 0; i < n; ++i) {
            out << path[i] << " ";
        }
        out << "0"; // close the cycle by returning to the start
    } else {
        out << "No Hamiltonian circuit";
    }
    return out.str();
}
