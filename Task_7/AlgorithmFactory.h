#pragma once
#include <memory>
#include <string>
#include "GraphAlgorithm.h"

class AlgorithmFactory {
public:
    static std::unique_ptr<GraphAlgorithm> create(const std::string& name);
};
