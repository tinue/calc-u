#pragma once
#include "machine/Machine.hpp"
#include "sessions/MachineSession.hpp"
#include <string>
#include <vector>

// A slice hook that holds typed text and keys as LIB-5's typing engine will
// (LIB-18 design §2, §3.4): what a test gave it waits, and drop() releases
// it all and is counted (AC-2, AC-14, EC-4). It acts on nothing.

namespace calcu::startup::test {

class TestHook final : public sessions::SliceHook {
public:
    machine::Ticks nextDue(machine::Ticks) const override { return sessions::kNever; }
    void act(machine::Machine&, machine::Ticks, pacing::WallTime) override {}
    void drop() override {
        ++drops;
        waiting.clear();
        held.clear();
    }
    std::string waiting;              // text still to be typed
    std::vector<machine::KeyId> held; // keys the engine holds
    int drops = 0;
};

} // namespace calcu::startup::test
