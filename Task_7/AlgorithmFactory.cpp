#include <memory>
#include "AlgorithmFactory.h"
#include "EulerAlgorithm.h"
#include "MSTAlgorithm.h"
#include "SCCAlgorithm.h"
#include "MaxFlowAlgorithm.h"
#include "HamiltonianAlgorithm.h"
#include <algorithm>
#include <stdexcept>

std::unique_ptr<GraphAlgorithm> AlgorithmFactory::create(const std::string& raw) {
    std::string name = raw;
    
    // normalize to lowercase
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c){ return std::tolower(c); });

    if (name == "euler")     return std::make_unique<EulerAlgorithm>();
    if (name == "mst")       return std::make_unique<MSTAlgorithm>();
    if (name == "scc")       return std::make_unique<SCCAlgorithm>();
    if (name == "maxflow")   return std::make_unique<MaxFlowAlgorithm>();
    if (name == "hamilton")  return std::make_unique<HamiltonianAlgorithm>();

    throw std::invalid_argument("Unknown algorithm: " + raw);
}