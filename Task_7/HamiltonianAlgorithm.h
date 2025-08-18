#pragma once
#include "GraphAlgorithm.h"
#include <string>

/**
 * Hamiltonian Circuit algorithm (cycle) via simple backtracking.
 */
class HamiltonianAlgorithm : public GraphAlgorithm {
public:
    std::string name() const override { return "hamilton"; }
    std::string run(const Graph& g) override;
};
