#pragma once
#include <string>

// ---------------------------------------------------------------------------
// Agent mode. Ordinal and MONOTONIC: it only ever increases. This is what lets
// the modes register merge by element-wise max with no conflict (see §4/§9).
// ---------------------------------------------------------------------------
enum Mode { SEARCHING = 0, MULE = 1, CONVERGING = 2 };

inline const char* modeName(Mode m) {
    switch (m) {
        case SEARCHING:  return "SEARCHING";
        case MULE:       return "MULE";
        case CONVERGING: return "CONVERGING";
    }
    return "?";
}

// All tunables in one place, seeded for reproducibility (§12).
struct Config {
    int      gridW         = 60;     // grid width  (cells) -- from search radius
    int      gridH         = 40;     // grid height (cells)
    int      numAgents     = 6;      // searchers (§3)
    double   commsRange    = 10.0;    // COMMS_RANGE: Euclidean, in cell units (§5)
    double   convergeRadius = 1.5;   // "gathered" tolerance around pilot (§8.5)
    int      maxTicks      = 8000;   // safety cap; success is the intended outcome
    unsigned seed          = 12345;  // single RNG seed -> deterministic run
    int      startX        = 2;      // all agents start co-located here (§3/§8)
    int      startY        = 2;
    bool     headless      = false;  // true = logs only, no raylib window
    int      logEvery      = 20;     // print a position summary every N ticks
};
