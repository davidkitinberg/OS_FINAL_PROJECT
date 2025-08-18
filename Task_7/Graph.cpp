#include "Graph.h"
#include <stdexcept>

// Constructor
Graph::Graph(int n) : numVertices(n), adjacencyList(n) {
    if (n < 0) {
        throw std::invalid_argument("Number of vertices must be non-negative.");
    }
}

// Vertex bounds checker
void Graph::validateVertex(int v) const {
    if (v < 0 || v >= numVertices) {
        throw std::out_of_range("Vertex index out of bounds.");
    }
}

// Get number of vertices
int Graph::V() const {
    return numVertices;
}

// Add an undirected edge
void Graph::addEdge(int u, int v) {
    validateVertex(u);
    validateVertex(v);
    if (u == v) {
        throw std::invalid_argument("Self-loops are not supported in this version.");
    }
    adjacencyList[u].push_back(v);
    adjacencyList[v].push_back(u);
}

// Add a directed edge
void Graph::addDirectedEdge(int u, int v) {
    validateVertex(u);
    validateVertex(v);
    adjacencyList[u].push_back(v);
}

// Return raw pointer for compatibility with `vector<int> adj[]`
std::vector<int>* Graph::raw() {
    return adjacencyList.data();
}

const std::vector<int>* Graph::raw() const {
    return adjacencyList.data();
}

// Get degree of a vertex
int Graph::degree(int v) const {
    validateVertex(v);
    return static_cast<int>(adjacencyList[v].size());
}

// Remove all edges
void Graph::clear() {
    for (auto& vec : adjacencyList) {
        vec.clear();
    }
}
