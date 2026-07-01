#include "comms/sync.h"
#include "comms/comms.h"

std::vector<Link> InProcessSync::step(std::vector<Agent>& agents, const Grid& grid,
                                      double range, int tick,
                                      std::vector<Event>& events) {
    std::vector<Link> links;
    for (size_t i = 0; i < agents.size(); ++i) {
        for (size_t j = i + 1; j < agents.size(); ++j) {
            if (!canCommunicate(agents[i].cell(), agents[j].cell(), grid, range))
                continue;

            links.push_back({ (int)i, (int)j });

            // Bidirectional full-state merge. Because merge is a CRDT join, doing
            // it both ways leaves the two replicas identical and the order, count
            // and timing of contacts never matter (§4 merge properties).
            bool changed = agents[i].model.merge(agents[j].model);
            changed     |= agents[j].model.merge(agents[i].model);

            // Log only contacts that actually reconciled divergent state, so the
            // logs highlight real partition-healing rather than steady-state noise.
            if (changed)
                events.push_back({ tick, "merge: " + agents[i].name() +
                                         " <-> " + agents[j].name() });
        }
    }
    return links;
}
