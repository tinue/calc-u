#pragma once
#include "machine/Machine.hpp"
#include <string>
#include <vector>

// The four questions of the idle rule, answered from the machine's idle
// signals (Machine.hpp) alone, the same for every product:
// - takes keys: key readiness;
// - waits: it takes keys or awaits a medium, and nothing keeps it busy;
// - ended: it waits and its program has ended.
// How long it must wait to be settled is the product's (presets' IdleRule).

namespace calcu::machine {

struct IdleState {
    bool takesKeys = false;
    bool waits = false;
    bool ended = false;              // waits, and the program has ended
    std::vector<std::string> busy;   // what keeps it busy
};

IdleState idleState(const Machine&);

} // namespace calcu::machine
