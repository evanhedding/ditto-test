#include "sim/simulation.h"
#include "render/logger.h"
#include "render/raylib_renderer.h"
#include <cstdio>
#include <cmath>

Simulation::Simulation(const Config& cfg)
    : cfg_(cfg), grid_(cfg.gridW, cfg.gridH), rng_(cfg.seed) {

    startCell_ = grid_.cell(cfg.startX, cfg.startY);

    // Pilot: one random cell, stationary, anywhere but the start (so detection
    // is not instant). Seeded RNG -> reproducible (§2).
    std::uniform_int_distribution<int> anyCell(0, grid_.size() - 1);
    do { pilotCell_ = anyCell(rng_); }
    while (pilotCell_ == startCell_ || !grid_.isFree(pilotCell_));

    // Agents: all co-located, identical code (§3). Pointers they hold into grid_
    // and rng_ stay valid; reserve so the vector never reallocates under them.
    agents_.reserve(cfg.numAgents);
    for (int i = 0; i < cfg.numAgents; ++i)
        agents_.emplace_back(i, grid_, startCell_, cfg.numAgents, rng_);

    renderers_.push_back(std::make_unique<Logger>(cfg.logEvery));
    if (!cfg.headless)
        renderers_.push_back(std::make_unique<RaylibRenderer>(grid_));

    std::printf("seed=%u  agents=%d  grid=%dx%d  pilot=(%d,%d)  comms_range=%.1f\n",
                cfg.seed, cfg.numAgents, cfg.gridW, cfg.gridH,
                grid_.x(pilotCell_), grid_.y(pilotCell_), cfg.commsRange);
}

bool Simulation::allGathered() const {
    for (const Agent& a : agents_) {
        if (a.model.pilot < 0) return false;                  // doesn't know yet
        double dx = grid_.x(a.cell()) - grid_.x(pilotCell_);
        double dy = grid_.y(a.cell()) - grid_.y(pilotCell_);
        if (std::sqrt(dx * dx + dy * dy) > cfg_.convergeRadius) return false;
    }
    return true;  // everyone knows AND has reached the pilot (§8.5)
}

void Simulation::run() {
    for (int tick = 1; tick <= cfg_.maxTicks; ++tick) {
        std::vector<Event> events;

        // 1. SENSE/DETECT -- before sync, so a fresh fix can propagate this tick.
        for (Agent& a : agents_) {
            if (!a.onPilot(pilotCell_)) continue;
            a.setPilot(pilotCell_);
            if (muleId_ < 0) {  // first writer becomes the mule (ties: lowest id, first here)
                muleId_ = a.id();
                a.setMode(MULE);
                events.push_back({ tick, a.name() + " DETECTED pilot -> MULE" });
            }
        }

        // 2. SYNC -- merge every in-range pair (propagates coverage/pilot/modes).
        std::vector<Link> links = sync_.step(agents_, grid_, cfg_.commsRange, tick, events);

        // 3. DERIVE MODE -- each agent switches itself from its merged replica.
        for (Agent& a : agents_)
            if (a.deriveMode())
                events.push_back({ tick, a.name() + " -> " + modeName(a.mode()) });

        // 4. MOVE -- one step per mode (the ROS2-style spin_once callback).
        for (Agent& a : agents_) a.spin_once();

        // 5. OBSERVE -- render + log.
        bool known = false;
        for (const Agent& a : agents_) known |= (a.model.pilot >= 0);
        RenderInput in{ grid_, agents_, links, events, tick, pilotCell_, known };
        for (auto& r : renderers_) r->draw(in);

        // 6. TERMINATION.
        if (allGathered()) {
            std::printf("SUCCESS at tick %d: all %d agents know the pilot and have gathered.\n",
                        tick, cfg_.numAgents);
            return;
        }
        for (auto& r : renderers_)
            if (r->wantClose()) return;  // user closed the window
    }
    std::printf("TIMEOUT after %d ticks: did not fully converge.\n", cfg_.maxTicks);
}
