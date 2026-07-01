#pragma once
#include <random>
#include "core/node.h"
#include "core/grid.h"
#include "core/crdt_state.h"

// ---------------------------------------------------------------------------
// Agent (§3, §8). Autonomous and symmetric: same code for every agent, no
// leader, no central tasking. The only role asymmetry (the mule) is DERIVED
// from the replica, never assigned.
//
// Behaviour is state-driven: each agent reads its own local replica and
// switches its own mode. There is no command/RPC layer (§9).
// ---------------------------------------------------------------------------
class Agent : public Node {
public:
    Agent(int id, const Grid& grid, int startCell, int numAgents, std::mt19937& rng);

    // --- the three per-tick hooks the Simulation calls, in phase order (§8.4)

    // SENSE: am I standing on the pilot cell? (Writing the register + the mule
    // decision are orchestrated by the Simulation, which sees all agents.)
    bool onPilot(int pilotCell) const { return cell_ == pilotCell; }

    // DERIVE: recompute mode purely from the merged replica. Returns true if the
    // mode advanced (so the Simulation can log the transition).
    bool deriveMode();

    // MOVE: one step per current mode. This is the ROS2-style actuation callback.
    void spin_once() override;

    // --- mutation helpers (write-once / monotonic, mirrored into the replica)
    void setPilot(int cell) { if (model.pilot < 0) model.pilot = cell; }
    bool setMode(Mode m);    // returns true if the mode actually advanced

    // --- read access
    int  cell() const { return cell_; }
    Mode mode() const { return mode_; }

    CrdtState model;  // this agent's local replica (public: it is plain data)

private:
    int pickUnsearchedNeighbor() const;  // adjacent free + unsearched, or -1
    int randomFreeNeighbor() const;      // any adjacent free cell, or cell_

    const Grid*   grid_;
    std::mt19937* rng_;
    int           cell_;
    Mode          mode_ = SEARCHING;
};
