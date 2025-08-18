#include <iostream>
#include <getopt.h>
#include <random>
#include <set>
#include <algorithm>
#include "Graph.h"
#include "EulerChecker.h"

using namespace std;

// Helper to parse CLI args
void parseArgs(int argc, char* argv[], int& V, int& E, unsigned& seed) {
    int opt;
    while ((opt = getopt(argc, argv, "v:e:s:")) != -1) {
        switch (opt) {
            case 'v':
                V = std::stoi(optarg);
                break;
            case 'e':
                E = std::stoi(optarg);
                break;
            case 's':
                seed = static_cast<unsigned>(std::stoul(optarg));
                break;
            default:
                cerr << "Usage: " << argv[0] << " -v <vertices> -e <edges> -s <seed>\n";
                exit(EXIT_FAILURE);
        }
    }
}

// Generate a random undirected graph with V vertices and E edges
void generateRandomGraph(Graph& g, int V, int E, unsigned seed) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> dist(0, V - 1);

    std::set<std::pair<int, int>> edgeSet;

    int added = 0;
    while (added < E) {
        int u = dist(rng);
        int v = dist(rng);

        if (u == v) continue;

        auto edge = std::minmax(u, v);
        if (edgeSet.count(edge)) continue;

        g.addEdge(edge.first, edge.second);
        edgeSet.insert(edge);
        ++added;
    }
}

int main(int argc, char* argv[]) {
    int V = -1, E = -1;
    unsigned seed = 42;

    parseArgs(argc, argv, V, E, seed);

    if (V <= 0 || E < 0 || E > V * (V - 1) / 2) {
        cerr << "Invalid parameters. Make sure:\n";
        cerr << "  - V > 0\n";
        cerr << "  - 0 <= E <= V*(V-1)/2\n";
        return 1;
    }

    Graph g(V);
    generateRandomGraph(g, V, E, seed);

    cout << "Generated Graph with " << V << " vertices and " << E << " edges.\n";

    int result = isEulerian(g);
    if (result == 2) {
        cout << "Result: Eulerian Circuit\n";
    } else if (result == 1) {
        cout << "Result: Eulerian Path\n";
    } else {
        cout << "Result: Not Eulerian\n";
    }

    return 0;
}
