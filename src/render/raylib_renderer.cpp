#include "render/raylib_renderer.h"
#include "raylib.h"

// Mode -> colour, the key the main world view is read by (§11).
static Color modeColor(Mode m) {
    switch (m) {
        case SEARCHING:  return SKYBLUE;
        case MULE:       return GOLD;
        case CONVERGING: return LIME;
    }
    return GRAY;
}

// Stable per-NODE identity colour, used for that node's mini-map and as a halo
// around its dot in the main view (so panel and world stay easy to correlate).
static Color agentColor(int i) {
    static const Color pal[] = {
        {   0, 130, 200, 255 }, { 245, 130,  48, 255 }, {  60, 180,  75, 255 },
        { 145,  30, 180, 255 }, {  70, 200, 200, 255 }, { 240,  50, 230, 255 },
        { 180, 180,  40, 255 }, { 170, 110,  40, 255 },
    };
    return pal[i % (int)(sizeof(pal) / sizeof(pal[0]))];
}

RaylibRenderer::RaylibRenderer(const Grid& grid) {
    originY_ = 40;  // HUD strip along the top
    (void)grid;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);  // let the user drag-resize the window
    InitWindow(1500, 850, "Multi-Agent Autonomous Search");
    SetTargetFPS(30);
}

RaylibRenderer::~RaylibRenderer() { CloseWindow(); }

bool RaylibRenderer::wantClose() { return WindowShouldClose(); }

void RaylibRenderer::draw(const RenderInput& in) {
    const Grid& g = in.grid;
    const int n = (int)in.agents.size();

    // Split the window: world view on the left, per-node panels on the right.
    int panelW = (int)(GetScreenWidth() * 0.30f); if (panelW < 320) panelW = 320;
    int mainW  = GetScreenWidth() - panelW;

    // Rescale the main view's cells to its region each frame (resizing just works).
    int byW = mainW / g.w(), byH = (GetScreenHeight() - originY_) / g.h();
    cellPx_ = byW < byH ? byW : byH;
    if (cellPx_ < 1) cellPx_ = 1;

    auto px = [&](int cx) { return cx * cellPx_; };
    auto py = [&](int cy) { return cy * cellPx_ + originY_; };

    BeginDrawing();
    ClearBackground(RAYWHITE);

    // --- Main world view (god's-eye) ---------------------------------------

    // Cells: shade the team's union coverage; outline cells where agents disagree.
    for (int c = 0; c < g.size(); ++c) {
        int searchedBy = 0;
        for (const Agent& a : in.agents) searchedBy += a.model.coverage[c] ? 1 : 0;
        int X = px(g.x(c)), Y = py(g.y(c));
        if (searchedBy > 0)
            DrawRectangle(X, Y, cellPx_, cellPx_, Color{ 225, 225, 225, 255 });
        if (searchedBy > 0 && searchedBy < n)  // divergent: not everyone knows yet
            DrawRectangleLines(X, Y, cellPx_, cellPx_, ORANGE);
    }
    // Faint grid lines on top.
    for (int x = 0; x <= g.w(); ++x) DrawLine(px(x), originY_, px(x), py(g.h()), Color{ 235,235,235,255 });
    for (int y = 0; y <= g.h(); ++y) DrawLine(0, py(y), px(g.w()), py(y), Color{ 235,235,235,255 });

    // Comms links between in-range pairs -- thick + bright so the mesh reads fast.
    float linkW = cellPx_ * 0.18f; if (linkW < 2.0f) linkW = 2.0f;
    for (const Link& l : in.links) {
        const Agent& a = in.agents[l.a];
        const Agent& b = in.agents[l.b];
        Vector2 pa = { (float)(px(g.x(a.cell())) + cellPx_ / 2), (float)(py(g.y(a.cell())) + cellPx_ / 2) };
        Vector2 pb = { (float)(px(g.x(b.cell())) + cellPx_ / 2), (float)(py(g.y(b.cell())) + cellPx_ / 2) };
        DrawLineEx(pa, pb, linkW, Color{ 0, 158, 47, 255 });
    }

    // Pilot: god's-eye ground truth, faint until detected then solid red + ring.
    int pX = px(g.x(in.pilotCell)), pY = py(g.y(in.pilotCell));
    if (in.pilotKnown) {
        DrawRectangle(pX, pY, cellPx_, cellPx_, RED);
        DrawRectangleLinesEx(Rectangle{ (float)pX, (float)pY, (float)cellPx_, (float)cellPx_ }, 3, YELLOW);
    } else {
        DrawRectangle(pX, pY, cellPx_, cellPx_, Color{ 230, 41, 55, 70 });
        DrawRectangleLinesEx(Rectangle{ (float)pX, (float)pY, (float)cellPx_, (float)cellPx_ }, 2, RED);
    }

    // Agents: identity-colour halo + mode-colour centre (so both read at once).
    int r = cellPx_ * 0.42f; if (r < 4) r = 4;
    for (int i = 0; i < n; ++i) {
        int cx = px(g.x(in.agents[i].cell())) + cellPx_ / 2;
        int cy = py(g.y(in.agents[i].cell())) + cellPx_ / 2;
        DrawCircle(cx, cy, r, agentColor(i));                 // identity ring
        DrawCircle(cx, cy, r - 2, modeColor(in.agents[i].mode()));  // mode fill
    }

    // --- Per-node panels (right column) ------------------------------------
    // One mini-map per node = that node's local replica, in its identity colour.
    int cols = (n <= 4) ? 1 : 2;
    int rows = (n + cols - 1) / cols;
    int cardW = panelW / cols;
    int cardH = (GetScreenHeight() - originY_) / rows;
    for (int i = 0; i < n; ++i) {
        int cardX = mainW + (i % cols) * cardW;
        int cardY = originY_ + (i / cols) * cardH;
        DrawText(TextFormat("%s  %s", in.agents[i].name().c_str(),
                            in.agents[i].model.pilot >= 0 ? "[knows pilot]" : ""),
                 cardX + 6, cardY + 3, 12, agentColor(i));
        int mapX = cardX + 6, mapY = cardY + 18;
        int mw = cardW - 12, mh = cardH - 24;
        int mcell = (mw / g.w() < mh / g.h()) ? mw / g.w() : mh / g.h();
        if (mcell < 1) mcell = 1;
        drawNodeView(in, in.agents[i], i, mapX, mapY, mcell);
    }

    // HUD legend.
    DrawText(TextFormat("tick %d   halo=node id / fill: blue=SEARCHING gold=MULE "
                        "green=CONVERGING   orange=divergent   red=pilot (%s)", in.tick,
                        in.pilotKnown ? "FOUND" : "not yet found"),
             8, 12, 16, DARKGRAY);

    EndDrawing();
}

void RaylibRenderer::drawNodeView(const RenderInput& in, const Agent& a, int idx,
                                  int ox, int oy, int mcell) {
    const Grid& g = in.grid;
    Color col = agentColor(idx);

    // Background = the still-unknown (fog) area for this node.
    DrawRectangle(ox, oy, g.w() * mcell, g.h() * mcell, Color{ 245, 245, 245, 255 });

    // Tiles THIS node believes are searched -- grows via self-discovery and merges.
    for (int c = 0; c < g.size(); ++c)
        if (a.model.coverage[c])
            DrawRectangle(ox + g.x(c) * mcell, oy + g.y(c) * mcell, mcell, mcell, col);

    // Pilot, but only if this node knows it (divergent knowledge is visible here).
    if (a.model.pilot >= 0)
        DrawRectangle(ox + g.x(a.model.pilot) * mcell, oy + g.y(a.model.pilot) * mcell,
                      mcell, mcell, RED);

    // This node's own position.
    DrawCircle(ox + g.x(a.cell()) * mcell + mcell / 2,
               oy + g.y(a.cell()) * mcell + mcell / 2, 3, BLACK);

    DrawRectangleLines(ox, oy, g.w() * mcell, g.h() * mcell, LIGHTGRAY);
}
