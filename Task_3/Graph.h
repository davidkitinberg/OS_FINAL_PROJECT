#pragma once
#include <vector>

/**
 * Simple undirected graph class.
 * Stores adjacency lists and allows adding edges.
 */
class Graph {
private:
    int numVertices;
    std::vector<std::vector<int>> adjacencyList;

    void validateVertex(int v) const;

public:
    // Constructor
    Graph(int n);

    // Return number of vertices
    int V() const;

    // Add undirected edge u <-> v
    void addEdge(int u, int v);

    // Add directed edge u -> v
    void addDirectedEdge(int u, int v);

    // Return raw pointer to adjacency list array
    std::vector<int>* raw();
    const std::vector<int>* raw() const;

    // Get degree of a vertex
    int degree(int v) const;

    // Clear all edges
    void clear();
};
