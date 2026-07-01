#pragma once
#include <string>
#include <vector>

// A comms link drawn this tick (every in-range pair, §11).
struct Link { int a, b; };

// A notable event this tick: detection, a mode transition, a state-changing
// merge, or final convergence. Renderers/loggers consume these (§11).
struct Event {
    int         tick;
    std::string msg;
};
