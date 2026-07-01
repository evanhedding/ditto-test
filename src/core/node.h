#pragma once
#include <string>
#include <utility>

// ---------------------------------------------------------------------------
// Node: a deliberately thin base, styled after a ROS2 node, to give each agent
// a named identity and a single per-tick callback. This is *cosmetic only* --
// there is no DDS, no topics, no pub/sub, no extra process. Real ROS2 is parked
// (§10) precisely because that machinery fights the in-sim partition model.
//
// The "executor" is the Simulation, which calls spin_once() on each node once
// per tick during the MOVE phase (§8.4).
// ---------------------------------------------------------------------------
class Node {
public:
    Node(int id, std::string name) : id_(id), name_(std::move(name)) {}
    virtual ~Node() = default;

    virtual void spin_once() = 0;  // per-tick actuation callback

    int id() const { return id_; }
    const std::string& name() const { return name_; }

protected:
    int id_;
    std::string name_;
};
