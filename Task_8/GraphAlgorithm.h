#pragma once
#include <string>
#include "Graph.h"

/**
 * Base interface for any graph algorithm.
 * 
 * Every algorithm needs to say what it's called (like "mst", "scc", or "maxflow")
 * and be able to run on a given Graph object, returning the result as a string
 * that can be shown directly to the user.
 */
class GraphAlgorithm {
public:
    virtual ~GraphAlgorithm() = default;

    // Return a short identifier for the algorithm (e.g. "mst", "scc").
    virtual std::string name() const = 0;

    // Run the algorithm on the given graph and return a result string.
    virtual std::string run(const Graph& g) = 0;
};
