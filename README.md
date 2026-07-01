# Multi-Agent Autonomous Search

## Overview

## How to compile and run

Requires a C++17 compiler and CMake (3.16+). raylib is fetched and built
automatically the first time you configure, so no separate install step is needed
— the first configure just takes a minute or two.

```sh
# 1. Configure (run once, and again only after editing CMakeLists.txt).
#    This is the step that fetches + builds raylib the first time.
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 2. Build (run after each code change).
cmake --build build -j

# 3. Run.
./build/search_sim
```

Day-to-day, the loop is just steps 2 and 3:

```sh
cmake --build build -j && ./build/search_sim
```

### Run options

```sh
./build/search_sim                 # live raylib view + structured logs
./build/search_sim --headless      # logs only, no window
./build/search_sim --seed 99       # reproduce a specific run
./build/search_sim --agents 5      # override the number of searchers
```

Each run prints its `seed=` at startup; pass that value back via `--seed` to replay
an interesting run exactly. All other tunables live in `src/config.h`.

### Reading the view

- **Left panel** — the god's-eye world. Agent dots use an identity-coloured halo
  with a mode-coloured centre (blue = SEARCHING, gold = MULE, green = CONVERGING).
  Green lines are live comms links; grey cells are searched; an orange outline marks
  cells where agents currently disagree. The pilot turns solid red once detected.
- **Right column** — one mini-map per node, each showing that node's own replica in
  its identity colour, updating in real time as it discovers tiles or syncs.

Close the window (or reach the success/timeout condition) to stop.

## Sync strategy & conflict resolution

Every searcher keeps its own copy of what the team has learned. When two searchers
come within range, they combine copies. The key design choice is that combining two
copies can **never conflict** — not because we detect and resolve clashes, but
because the shared data is built so it *can't* disagree in the first place. (The
technical name for this kind of data is a CRDT, a conflict-free replicated data
type — see `core/crdt_state.h`.)

There are three pieces of shared knowledge, each built to only ever move forward:

- **Searched cells** — a set that only grows. A cell never goes back to "unsearched",
  so combining two copies is just pooling the two sets together.
- **Pilot location** — written once. Because the target doesn't move and detection is
  reliable, everyone who finds it records the *same* cell, so there's nothing to
  disagree about.
- **Each searcher's status** — only ever advances (searching → relaying → regrouping),
  so combining just keeps the further-along value.

Because every piece only moves in one direction, combining is completely
order-independent: it doesn't matter who syncs with whom, how many times, or in what
order — everyone converges on the same answer. That's what makes recovery from a
dropout automatic, with no special reconnect logic anywhere in the code.

## Connectivity & partition model

There's no central server and no always-on connection. Each tick, we simply check
which searchers are close enough to talk (within a fixed range). Those in range
exchange copies; those out of range don't. There's no "lost message" handling
because being disconnected is the normal state — making contact is the notable
event, not losing it.

As searchers roam, the group naturally splits into disconnected clusters and later
merges back together. Because syncing can't conflict, any knowledge that drifted
apart while two searchers were separated snaps back into agreement the instant they
reconnect. This fragment-and-reform behavior is the heart of the demo, and you can
watch it happen live in the per-node panels.

## Architecture

The code is organized so the "thinking" part — what each searcher knows and decides
— is kept completely separate from how it communicates and how it's displayed. The
folders mirror that split:

- **`core/`** — the searcher logic and the shared-knowledge structure. Knows nothing
  about networking or graphics.
- **`comms/`** — decides who's in range and performs the copy exchange.
- **`render/`** — the on-screen view and the text logs.
- **`sim/`** — the main loop that ties it all together.

The payoff is that the networking sits behind a single, well-defined seam: the
hand-rolled in-program sync could be swapped for a real networking platform later
without touching any searcher logic. The display works the same way — the logs and
the live window are just two interchangeable views of the same underlying data.

## Agent behaviour

Every searcher runs the *identical* code — there's no leader and no one handing out
assignments. Each one decides for itself, based only on what it currently knows.
There are three behaviors:

- **Searching** — explore the area, marking cells as it covers them.
- **Relaying** (the "mule") — whoever first finds the pilot keeps roaming instead of
  rushing back. By staying on the move it naturally runs into the searchers who
  haven't heard the news yet and passes it along.
- **Regrouping** — once a searcher learns where the pilot is, it heads straight there.

Crucially, no searcher is ever *told* to switch behavior. Each switches itself the
moment its own knowledge changes: the finder becomes the relay simply by being the
finder, and the others start regrouping the instant the news reaches them. The run
ends when everyone knows the location and has gathered there.

## Observability

## Key trade-offs & assumptions

These are deliberate simplifications that keep the first version honest and robust:

- **Known area, unknown target.** We assume the rough area to search is known (it
  defines the grid), but not where the pilot is within it.
- **Stationary target, reliable detection, open terrain.** These keep the shared
  knowledge strictly one-directional, which is what makes syncing conflict-free. A
  moving target or false alarms would reintroduce genuine disagreements and require a
  larger redesign.
- **Hand-rolled sync instead of a real platform.** For simulating dropouts on a
  single machine, rolling our own is simpler than fighting a real networking stack
  that's designed to *stay* connected. In the field, that same auto-reconnecting
  behavior is exactly what you'd want — noted as future work.
- **A single relay.** Guarantees everyone is eventually reached, at some cost to
  speed. Multiple relays would be faster but raise a harder question: how does the
  team agree that everyone is done?
- **Positions stay private.** We never share live positions — they change constantly
  and would break the conflict-free property. The relay finds stragglers by sweeping
  the area, not by tracking where they are.

## How it scales and where it breaks

## What I'd do next

## Reflection
