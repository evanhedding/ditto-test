#pragma once
#include "render/renderer.h"

// Live 2D view (§11). Colour encodes mode; lines show the comms mesh fragmenting
// and re-forming; shaded cells show coverage, outlined cells show where replicas
// currently DISAGREE (the visible partition-then-reconcile signal).
//
// The left panel is the god's-eye world; the right column is one mini-map PER
// NODE, each drawing that node's OWN replica in its identity colour -- so you can
// watch a node's tiles fill from both self-discovery and merges, in real time.
class RaylibRenderer : public IRenderer {
public:
    explicit RaylibRenderer(const Grid& grid);
    ~RaylibRenderer() override;

    void draw(const RenderInput& in) override;
    bool wantClose() override;

private:
    // Draw one node's local replica as a mini-map at (ox,oy) with cell size mcell.
    void drawNodeView(const RenderInput& in, const Agent& a, int idx,
                      int ox, int oy, int mcell);

    int cellPx_;          // pixels per cell in the main (left) world view
    int originY_;         // top margin reserved for the HUD
};
