#pragma once
#include <vector>
#include "core/agent.h"
#include "core/grid.h"
#include "core/events.h"

// ---------------------------------------------------------------------------
// ISyncLayer (§7): given the agents and the connectivity predicate, perform
// this tick's merges and report the links that formed. THIS INTERFACE IS THE
// SINGLE SWAP POINT for a real platform (Ditto/DDS) later -- agent logic never
// references the concrete implementation.
// ---------------------------------------------------------------------------
class ISyncLayer {
public:
    virtual ~ISyncLayer() = default;
    virtual std::vector<Link> step(std::vector<Agent>& agents, const Grid& grid,
                                   double range, int tick,
                                   std::vector<Event>& events) = 0;
};

// v1 implementation: for every in-range pair, a bidirectional full-state merge.
// Full-state (not delta) is fine -- the replica is tiny (§4). Delta sync parked.
class InProcessSync : public ISyncLayer {
public:
    std::vector<Link> step(std::vector<Agent>& agents, const Grid& grid,
                           double range, int tick,
                           std::vector<Event>& events) override;
};
