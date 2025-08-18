#pragma once
#include "GraphAlgorithm.h"

/**
 * Minimal MST strategy for *unweighted* graphs.
 * Assumes each edge has weight = 1.
 *
 * Result:
 *  - If the graph is connected (all vertices reachable), MST weight = V - 1.
 *  - Otherwise, reports that an MST does not exist (graph is disconnected).
 */
class MSTAlgorithm : public GraphAlgorithm {
public:
    std::string name() const override { return "mst"; }
    std::string run(const Graph& g) override;
};
