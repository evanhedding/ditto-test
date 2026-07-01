#pragma once
#include <vector>
#include "core/agent.h"
#include "core/grid.h"
#include "core/events.h"

// Everything a renderer needs for one frame. Read-only snapshot of the world.
struct RenderInput {
    const Grid&               grid;
    const std::vector<Agent>& agents;
    const std::vector<Link>&  links;
    const std::vector<Event>& events;   // events produced this tick
    int  tick;
    int  pilotCell;     // true location (Simulation knows it)
    bool pilotKnown;    // has any agent detected it yet? (reveal gate)
};

// ---------------------------------------------------------------------------
// IRenderer (§7): the observability seam. Agent/world logic is agnostic to it.
// Impls: RaylibRenderer (live view) and Logger (structured logs). Both can run.
// ---------------------------------------------------------------------------
class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual void draw(const RenderInput& in) = 0;
    virtual bool wantClose() { return false; }  // user closed the window?
};
