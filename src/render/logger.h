#pragma once
#include "render/renderer.h"

// Structured stdout logger (§11). A live view is nice; clear logs are sufficient
// on their own, so this is a first-class renderer, not an afterthought.
class Logger : public IRenderer {
public:
    explicit Logger(int logEvery) : logEvery_(logEvery) {}
    void draw(const RenderInput& in) override;

private:
    int logEvery_;  // print the per-tick position summary every N ticks
};
