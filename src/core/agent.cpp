#include "core/agent.h"

Agent::Agent(int id, const Grid& grid, int startCell, int numAgents, std::mt19937& rng)
    : Node(id, "agent_" + std::to_string(id)),
      model(grid.size(), numAgents),
      grid_(&grid), rng_(&rng), cell_(startCell) {
    model.coverage[startCell] = true;  // the shared start cell counts as searched
}

bool Agent::setMode(Mode m) {
    if (m <= mode_) return false;      // monotonic: never moves backward
    mode_ = m;
    model.modes[id_] = m;              // record my own mode so merge propagates it
    return true;
}

bool Agent::deriveMode() {
    // Non-finder that now knows the pilot -> converge immediately.
    if (mode_ == SEARCHING && model.pilot >= 0)
        return setMode(CONVERGING);

    // Mule terminates once its replica shows no agent still SEARCHING. It reads
    // this straight off the merged modes register -- that is what it is for.
    if (mode_ == MULE) {
        for (Mode m : model.modes)
            if (m == SEARCHING) return false;   // someone is still out there
        return setMode(CONVERGING);
    }
    return false;
}

int Agent::pickUnsearchedNeighbor() const {
    std::vector<int> options;
    for (int n : grid_->neighbors(cell_))
        if (grid_->isFree(n) && !model.coverage[n]) options.push_back(n);
    if (options.empty()) return -1;
    std::uniform_int_distribution<int> pick(0, (int)options.size() - 1);
    return options[pick(*rng_)];
}

int Agent::randomFreeNeighbor() const {
    std::vector<int> options;
    for (int n : grid_->neighbors(cell_))
        if (grid_->isFree(n)) options.push_back(n);
    if (options.empty()) return cell_;
    std::uniform_int_distribution<int> pick(0, (int)options.size() - 1);
    return options[pick(*rng_)];
}

void Agent::spin_once() {
    if (mode_ == CONVERGING) {
        if (model.pilot >= 0 && cell_ != model.pilot)
            cell_ = grid_->bfsStep(cell_, [&](int c) { return c == model.pilot; });
        return;
    }

    // SEARCHING and MULE share the same movement: explore the frontier. The mule
    // does NOT beeline -- following the shrinking frontier is what sweeps it
    // toward the still-searching stragglers (§8.2).
    model.coverage[cell_] = true;
    int next = pickUnsearchedNeighbor();
    if (next < 0) {  // boxed in by searched cells -> route to nearest global frontier
        next = grid_->bfsStep(cell_, [&](int c) { return !model.coverage[c]; });
        if (next == cell_) {
            // No frontier left anywhere in MY replica -- the search is done as far
            // as I can tell. Two cases, both needed to keep convergence (§9):
            if (model.pilot >= 0)
                // I'm the mule and know the pilot: head there to gather. This also
                // reconnects me with the converged team so I finally learn everyone
                // has arrived -- otherwise I'd wander out of contact forever.
                next = grid_->bfsStep(cell_, [&](int c) { return c == model.pilot; });
            else
                // I don't know the pilot yet: wander to bump into someone who does.
                next = randomFreeNeighbor();
        }
    }
    cell_ = next;
}
