#pragma once
#include <vector>
#include "config.h"

// ---------------------------------------------------------------------------
// CrdtState: the per-agent CRDT replica (§4) -- the heart of the design.
//
// Every field is a join-semilattice element, so merge() is commutative,
// associative and idempotent. Conflicts therefore CANNOT occur by construction:
// two replicas that diverged during a partition heal on contact with no clock,
// no tie-break and no special-case reconnect code.
//
// Pure data + merge. No I/O, no knowledge of the renderer or the sync layer.
// ---------------------------------------------------------------------------
class CrdtState {
public:
    CrdtState(int numCells, int numAgents);

    // 1. coverage -- grow-only set of searched cells. Merge = union.
    std::vector<bool> coverage;

    // 2. pilot -- write-once register. -1 = unknown. Stationary target + reliable
    //    detection => every writer writes the same cell, so there is never a tie.
    int pilot = -1;

    // 3. modes -- per-agent monotonic register (agentId -> Mode). Merge = max.
    //    Its real job is termination detection, not conflict handling (§8.2).
    std::vector<Mode> modes;

    // Merge `other` into this replica. Returns true if anything changed, which
    // the caller uses purely to log "a reconciliation actually happened".
    bool merge(const CrdtState& other);

    int coveredCount() const;  // for logs / convergence reporting
};
