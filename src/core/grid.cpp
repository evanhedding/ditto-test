#include "core/grid.h"
#include <queue>

Grid::Grid(int w, int h) : W(w), H(h), wall(w * h, false) {}

std::vector<int> Grid::neighbors(int cellId) const {
    int cx = x(cellId), cy = y(cellId);
    static const int dx[4] = { 1, -1, 0, 0 };
    static const int dy[4] = { 0, 0, 1, -1 };
    std::vector<int> out;
    for (int i = 0; i < 4; ++i) {
        int nx = cx + dx[i], ny = cy + dy[i];
        if (inBounds(nx, ny)) out.push_back(cell(nx, ny));
    }
    return out;
}

int Grid::bfsStep(int from, const std::function<bool(int)>& isGoal) const {
    if (isGoal(from)) return from;

    std::vector<int> parent(size(), -2);  // -2 = unvisited, -1 = start
    std::queue<int> q;
    parent[from] = -1;
    q.push(from);

    int goal = -1;
    while (!q.empty()) {
        int c = q.front(); q.pop();
        if (isGoal(c)) { goal = c; break; }
        for (int n : neighbors(c))
            if (parent[n] == -2 && isFree(n)) { parent[n] = c; q.push(n); }
    }
    if (goal < 0) return from;  // nothing reachable -> stay put

    // Walk parents back from the goal to find the first step out of `from`.
    int cur = goal;
    while (parent[cur] != from) cur = parent[cur];
    return cur;
}
