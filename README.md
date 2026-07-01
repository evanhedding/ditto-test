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

### Run options

```sh
./build/search_sim                 # live raylib view + structured logs
./build/search_sim --headless      # logs only, no window
./build/search_sim --seed 99       # reproduce a specific run
./build/search_sim --agents 5      # override the number of searchers
```

Each run prints its `seed=` at startup; pass that value back via `--seed` to replay
an interesting run exactly. All other tunables live in `src/config.h`.

## Summary of operation

This simulation is designed to have a configurable number of independent nodes set out from 
the same location in search of a pilot that gets randomly placed somewhere in the world. 
The nodes do have knowledge of the map size and boundaries, but do not know where the pilot
is until one of them physically searches that cell.

They have a distance-based network that simply connects and disconnects based on distance
in the map. When two nodes sync they merge a conflict-free state struct that provides each 
node with information of the other's searched cells, pilot knowledge, and mode status of 
other nodes. 

The node motion is very simplistic and each moves into a random unsearched neighbor at each
step during search mode. When a one node finds the pilot, it switches to Mule mode and leaves
to find the other nodes still searching. These nodes, once informed, switch to Converge mode
and move straight to the pilot's location. Once all nodes stand informed, the mule switches
and converges as well.

![demo](/src/images/demo.gif)

## Reading the view

- **Left panel** — the world-view. Agent dots use an identity-coloured halo
  with a mode-coloured centre (BLUE = SEARCHING, GOLD = MULE, GREEN = CONVERGING).
  GREEN lines are live comms links; GREY cells are searched; the pilot is the RED square.
- **Right column** — one mini-map per node, each showing that node's own replica in
  its identity colour, updating in real time as it discovers tiles or syncs.

## Sync strategy & conflict resolution

Every searcher keeps its own state model of what the full team has learned by simply
keeping track of all cells that have been searched, whether the pilot has been found,
and which searchers are still searching. When two searchers come within range for communication, they sync states. The key design choice here is that combining two
states cannot conflict since states are communitive. (This is what utilizes the conflict-free replicated data type — see `core/crdt_state.h` for code)

Three pieces of shared knowledge, each built to only ever move forward:

- **Searched cells** — a set that only grows. A cell never goes back to "unsearched",
  so combining two copies is just pooling the two sets together.
- **Pilot location** — written once. Because the target doesn't move and detection is
  reliable, everyone who finds it records the same cell, so there's nothing to
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

![Two nodes with world state diverging](/src/images/unsynced.png)

![Two nodes with world state synced](/src/images/synced.png)

## Architecture

- **`core/`** — the searcher logic and the shared-knowledge structure. Knows nothing
  about networking or graphics.
- **`comms/`** — decides who's in range and performs the copy exchange.
- **`render/`** — the on-screen view and the text logs.
- **`sim/`** — the main loop that ties it all together.

## Agent behaviour

Every searcher runs identical but entirely independent control code. This was meant to emulate ROS nodes or separate machines running independently in the real world. Each one decides for itself, based only on what it currently knows.

Three modes of operation for each node:

- **SEARCH** — explore the area, marking cells as it covers them.
- **MULE** — whoever first finds the pilot keeps roaming instead of
  rushing back. By staying on the move it naturally runs into the searchers who
  haven't heard the news yet and passes it along.
- **CONVERGE** — once a searcher learns where the pilot is, it heads straight there.

Crucially, no searcher is ever *told* to switch behavior. Each switches itself the
moment its own knowledge changes: the finder becomes the mule simply by being the
finder, and the others start regrouping the instant the news reaches them. The run
ends when everyone knows the location and has gathered there.

## Key trade-offs & assumptions

These are deliberate simplifications that keep the first version honest and robust:

- **Known area, unknown target.** We assume we know the world map ahead of time. 
  Each node has knowledge of the unsearched cells and the boundaries of the map. 
  This is so the search could be made more simplistic for each node and still achieve
  successful convergence in a reasonable amount of time. If the world map was unknown
  from the start, more complex search algorithms would be needed.
- **Stationary target, reliable detection, open terrain.** These keep the shared
  knowledge strictly one-directional, which is what makes syncing conflict-free. A
  moving target or other dynamic complexities would make it so the sync step as-is
  would no longer be conflict free, necessitating changes either to the CRDT or mode operations.
- **Hand-rolled sync instead of a real platform.** Originally, I intended on using 
  Ditto's SDK or ROS2 nodes + DDS, but while planning out the project, it was unclear
  how exactly to simulate network dropouts and reconnections, as both of those libraries
  are meant to run on real hardware. So to save time in design and potential debugging
  I chose to make a very simple sync logic that just disconnected and reconnected based
  on pure distance.
- **A single relay.** Guarantees everyone is eventually reached, at some cost to
  speed. Multiple relays would be faster but raises other difficulties like the endless
  cycle potential of having lost mules still trying to find searchers that don't exist. 
  The single mule setup at least guarantees the mule will converge once it finds the 
  final searcher. The one edge case though is that the single mule could still get "lost"
  if the last searchers find the pilot on their own while the mule still thinks they're 
  out there. The mule will still converge eventually, it just may take a while for it to 
  finishing searching the whole map.
- **Positions stay private to each node.** I never share or pass positions between the 
  nodes. This was the easiest way to remain conflict-free during syncs, and still works
  for full convergence since nodes will always communicate once within range. But a more
  elegant solution, and likely a real-world one, would be if the CRDT was able to track
  "last known" search locations of each node. This would facilitate faster mule searches
  and ultimate convergance, especially on large maps.

## How it scales and where it breaks

The program appears to work quite well, especially on smaller maps with 4+ nodes. As map
size increases, the system still converges but takes longer depending on number of nodes. 
Increasing node count or communication distance is what makes large maps still easily
convergable in decent time. But keeping node count fixed and limited comm distance, large
maps converge slowly. 

Where the system breaks is on large maps with modest node count when the single mule is
out searching for stragglers but never finds them because the stragglers themselves found 
the pilot indepently. In this scenario the mule has to search the entire map and either 
happen to move in range of the pilot to sync and see that everyone has converged, or he
needs to finish searching the entire map, which may take a long time if large.

## Reflection

For a project like this I leaned on AI heavily for both architecture planning and actual 
code implementation. Even though I have background in manually programming every aspect 
of this project, the time constraints and nature of the task made substantial AI assistance 
a no-brainer. I like to use Claude's' online dashboard to plan architecture at a high level. 
There I was able to outline the project goals and insert my own plan for both the simulation
and the core searcher control functionality. I find that high-level planning like this is 
not only paramount for helping Claude write correct code but also incredibely useful in 
developing a successful and modern-optimal design.

Where AI impressed during planning:

- Pointing out that Ditto's C++ SDK is Linux-only and ROS2 on modern macOS is difficult and 
  error-prone. Both of these would have caused me problems trying to make run on my mac, so 
  moving away from those would be a time-saving strategic move.
- We brainstormed various ways to simulate network dropouts and reconnections between nodes,
  still with the goal of using actuall Ditto code, via Rust or Swift libraries. But even then
  Claude was wise in acknowledging that these libraries are meant to work on real hardware 
  and it was unclear how easily we'd be able to induce network disconnections while all nodes
  are running on the same computer. So instead of wasting time on a path that may or may not
  have worked easily, we settled on simple network simulation based on node distance.

Where AI disappointed during planning:

- After planning out most of the architecture, I caught a mistake where Claude had intended 
  to implement a non-CRDT sync style when I explicitly designed the nodes to be conflict-free. 
  Claude had assumed timestamps, global clocks, and conflict resolution to be necessary simply
  based on previous model knowledge of other distributed systems. I had to explain how I 
  designed these nodes to operate conflict-free naturally, and that clocks were
  unnecessary because conflicts *cannot* arise by design.

With the architecture honed and decided, I used Claude Code to go ahead and build the simulation,
along with most of the files necessary for the first version. This is really where AI-for-coding 
shines for someone like me who prefers C/C++ programs that might require more code and files, as 
well as a build system like CMake to even begin. In the past I would have leaned toward python 
for a rapid prototype, but with the help of AI, a well-defined spec sheet can result in a working 
C++ program just as easy as a python one.

Where AI impressed during implementation:

- Working program compiled on first attempt.
- Map rendered exactly as intended. Was able to tweak easily and rapidly as needed. 

Where AI disappointed during implementation:

- Files and classes were poorly named and required manual rewriting and organization.
- Some of the node control code came out different from the design, but was easy to diagnose and 
  fix. (i.e. nodes would sit stuck when surrounding by already-searching cells)

*What I would build if I had another day*

There are many avenues of complexity that could be added. Especially on the autonomous-search side
of things. I would like the expand the world size and increase node count. More complex search
patterns could be implemented and nodes could coordinate in groups with mules running group to group.
Nodes could have a buddy-system type state memory so that after the pilot is found, each node just
needs to find and inform their buddies for convergence. All of these, and many others, could be fun
to implement for a more optimal convergence path.

On the other side, the communication layer, more complexity would be interesting to add, especially
paired with increasing map size and node count. We could introduce obstacles that cut off LOS 
communication in addition to distance. I could even implement each node as a separate thread and 
actually mimic a ROS node properly while putting the network check onto the node itself.

And of course, if I had even more time, a proper implementation with actual Ditto code could likely 
be found and would be very interesting to see operate in real time.
