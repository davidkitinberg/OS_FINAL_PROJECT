#pragma once
#include "Graph.h"

/**
 * Checks if the graph contains:
 *   0 → Not Eulerian
 *   1 → Eulerian Path (but not a circuit)
 *   2 → Eulerian Circuit
 *
 * Based on classic DFS + degree counting.
 */
int isEulerian(const Graph& g);
