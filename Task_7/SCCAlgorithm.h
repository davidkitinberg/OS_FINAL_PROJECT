#pragma once
#include "GraphAlgorithm.h"
#include <string>

/**
 * Strongly Connected Components (SCC) via Kosaraju’s algorithm.
 * Works on directed graphs (use Graph::addDirectedEdge).
 *
 * Result string includes:
 *  - Number of SCCs
 *  - The vertex list of each component (one line per SCC)
 */
class SCCAlgorithm : public GraphAlgorithm {
public:
    std::string name() const override { return "scc"; }
    std::string run(const Graph& g) override;
};
