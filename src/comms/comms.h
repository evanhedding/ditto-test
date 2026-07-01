#pragma once
#include "core/grid.h"

// ---------------------------------------------------------------------------
// Connectivity predicate (§5).
//
// v1 rule: pure Euclidean range between cell centres. Out of range simply means
// merge is never called -- loss is the baseline, contact is the explicit act.
//
// Kept deliberately extensible: a `&& hasLineOfSight(a, b, grid)` term can be
// appended here later (parked obstacles, §10) without touching any caller.
// ---------------------------------------------------------------------------
inline bool canCommunicate(int cellA, int cellB, const Grid& g, double range) {
    double dx = g.x(cellA) - g.x(cellB);
    double dy = g.y(cellA) - g.y(cellB);
    return (dx * dx + dy * dy) <= range * range;
}
