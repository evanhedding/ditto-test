#pragma once
#include <memory>
#include <vector>
#include "config.h"
#include "core/grid.h"
#include "core/agent.h"
#include "comms/sync.h"
#include "render/renderer.h"

// ---------------------------------------------------------------------------
// Simulation (§7): owns the world and drives the per-tick loop (§8.4). It is the
// "executor" that ticks every node and runs the sync + termination checks.
// ---------------------------------------------------------------------------
class Simulation {
public:
    explicit Simulation(const Config& cfg);
    void run();

private:
    bool allGathered() const;  // success condition (§8.5)

    Config                                    cfg_;
    Grid                                      grid_;
    std::mt19937                              rng_;
    int                                       startCell_;
    int                                       pilotCell_;
    int                                       muleId_ = -1;  // first writer (§8.2)
    std::vector<Agent>                        agents_;
    InProcessSync                             sync_;
    std::vector<std::unique_ptr<IRenderer>>   renderers_;
};
