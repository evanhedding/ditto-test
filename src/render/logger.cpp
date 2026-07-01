#include "render/logger.h"
#include <cstdio>

void Logger::draw(const RenderInput& in) {
    // Events (detection, transitions, state-changing merges) print every tick so
    // nothing important is missed between summaries.
    for (const Event& e : in.events)
        std::printf("  [t=%d] %s\n", e.tick, e.msg.c_str());

    // Periodic one-line snapshot: each agent's cell, mode and coverage size. The
    // differing coverage counts are how divergence-then-reconciliation shows up.
    if (in.tick % logEvery_ != 0) return;

    std::printf("t=%-5d |", in.tick);
    for (const Agent& a : in.agents)
        std::printf(" %s(%d,%d)%c[cov %d]", a.name().c_str(),
                    in.grid.x(a.cell()), in.grid.y(a.cell()),
                    modeName(a.mode())[0], a.model.coveredCount());

    std::printf(" | links:");
    if (in.links.empty()) std::printf(" none (fully partitioned)");
    for (const Link& l : in.links) std::printf(" %d-%d", l.a, l.b);
    std::printf("\n");
}
