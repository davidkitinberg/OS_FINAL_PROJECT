#pragma once
#include "GraphAlgorithm.h"
#include <string>

/**
 * Max Flow from source=0 to sink=n-1 using Edmonds–Karp (unit capacities).
 *
 * Assumptions:
 *  - The Graph's adjacency is interpreted as directed edges.
 *  - Each edge has capacity 1 (parallel edges add capacity).
 *  - If you used addEdge(u,v) for undirected graphs, you'll get capacity both ways.
 */
class MaxFlowAlgorithm : public GraphAlgorithm {
public:
    std::string name() const override { return "maxflow"; }
    std::string run(const Graph& g) override;
};
