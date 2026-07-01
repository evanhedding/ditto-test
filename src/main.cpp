#include <cstdlib>
#include <cstring>
#include <random>
#include "config.h"
#include "sim/simulation.h"

// Tiny CLI override of the most-used tunables (everything else lives in Config).
//   ./search_sim [--seed N] [--agents N] [--headless]
//
// Default is a FRESH random run each launch; pass --seed N to reproduce one
// exactly. The chosen seed is printed at startup so any run can be replayed.
int main(int argc, char** argv) {
    Config cfg;
    cfg.seed = std::random_device{}();  // random unless overridden below
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--seed") && i + 1 < argc)   cfg.seed = (unsigned)std::atoi(argv[++i]);
        else if (!std::strcmp(argv[i], "--agents") && i + 1 < argc) cfg.numAgents = std::atoi(argv[++i]);
        else if (!std::strcmp(argv[i], "--headless"))          cfg.headless = true;
    }
    Simulation(cfg).run();
    return 0;
}
