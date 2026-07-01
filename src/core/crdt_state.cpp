#include "core/crdt_state.h"
#include <algorithm>

CrdtState::CrdtState(int numCells, int numAgents)
    : coverage(numCells, false), modes(numAgents, SEARCHING) {}

bool CrdtState::merge(const CrdtState& o) {
    bool changed = false;

    // coverage: set union (a cell never goes back to un-searched).
    for (size_t i = 0; i < coverage.size(); ++i)
        if (o.coverage[i] && !coverage[i]) { coverage[i] = true; changed = true; }

    // pilot: write-once. Take it if we lack it; if both hold it they agree.
    if (pilot < 0 && o.pilot >= 0) { pilot = o.pilot; changed = true; }

    // modes: element-wise max over the ordinal enum.
    for (size_t i = 0; i < modes.size(); ++i)
        if (o.modes[i] > modes[i]) { modes[i] = o.modes[i]; changed = true; }

    return changed;
}

int CrdtState::coveredCount() const {
    return (int)std::count(coverage.begin(), coverage.end(), true);
}
