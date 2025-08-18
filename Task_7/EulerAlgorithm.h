#pragma once
#include "GraphAlgorithm.h"
#include <string>

/**
 * Eulerian Circuit/Path checker as a Strategy.
 * Returns:
 *  - "Eulerian Circuit"
 *  - "Eulerian Path"
 *  - "Not Eulerian"
 * Also prints odd-degree vertices when circuit does not exist.
 */
class EulerAlgorithm : public GraphAlgorithm {
public:
    std::string name() const override { return "euler"; }
    std::string run(const Graph& g) override;
};