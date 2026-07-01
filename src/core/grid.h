#pragma once
#include <vector>
#include <functional>

// ---------------------------------------------------------------------------
// Grid / World (§2, §6).
// One structure serving three roles: occupancy layer (empty in v1), the graph
// for BFS pathfinding, and the coordinate space the coverage map lives in.
// A cell is an integer id = y * W + x.
// ---------------------------------------------------------------------------
class Grid {
public:
    Grid(int w, int h);

    int  w() const { return W; }
    int  h() const { return H; }
    int  size() const { return W * H; }

    int  cell(int x, int y) const { return y * W + x; }
    int  x(int cellId) const { return cellId % W; }
    int  y(int cellId) const { return cellId / W; }

    bool inBounds(int x, int y) const { return x >= 0 && x < W && y >= 0 && y < H; }

    // A cell is traversable if it is not a wall. Occupancy is all-false in v1,
    // but every pathfinding query already consults it so walls drop in later
    // without touching callers (§2, §6).
    bool isFree(int cellId) const { return !wall[cellId]; }

    // 4-connected in-bounds neighbours of a cell.
    std::vector<int> neighbors(int cellId) const;

    // BFS over FREE cells from `from`, to the nearest cell satisfying isGoal.
    // Returns the first cell to step to along that shortest path, or `from` if
    // already at a goal or no goal is reachable. This single primitive covers
    // both "route to nearest frontier" and "beeline to the pilot" (§6).
    int bfsStep(int from, const std::function<bool(int)>& isGoal) const;

private:
    int W, H;
    std::vector<bool> wall;  // occupancy layer (the parked obstacles seam, §10)
};
