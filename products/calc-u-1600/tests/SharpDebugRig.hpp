#pragma once
#include <chrono>
#include <memory>
#include <string>

#include "../app/SharpDebuggerSetup.hpp"
#include "../core/Machine/SharpCatalogue.hpp"
#include "../core/PC1500/PC1500Machine.hpp"
#include "../core/PC1600/PC1600Machine.hpp"
#include "debugger/DebuggerPart.hpp"
#include "sessions/AppSession.hpp"
#include "sessions/MachineSession.hpp"

// A library app session in virtual time with
// only the DebuggerPart of Calc-U-1600 (app/SharpDebuggerSetup) around a
// Sharp machine the test builds -- a PC-1500A without ROM for the memtest
// fixture, or one from the ROM set. The run control is the gate of every
// advance; the DAP's layer (app/dap/) works on its table.

namespace sharpdebugtest {

struct Rig {
    calcu::debugger::DebuggerPart debugger{cu1600::sharpDebuggerSetup()};
    std::unique_ptr<calcu::sessions::AppSession> session;
    calcu::machine::Machine* machine = nullptr;

    explicit Rig(std::unique_ptr<calcu::machine::Machine> m) {
        calcu::sessions::AppSessionSetup s;
        s.clock = [] { return calcu::pacing::WallTime{0}; };
        s.hostTime = [] { return std::chrono::system_clock::time_point{}; };
        s.parts = {&debugger};
        s.hosting = calcu::sessions::Hosting::VirtualTime;
        session = std::make_unique<calcu::sessions::AppSession>(std::move(s));
        machine = m.get();
        session->machineSession().setMachine(std::move(m), {}, 48000);
    }
    ~Rig() {
        if (session) session->close();
    }

    calcu::debugger::RunControl& rc() { return debugger.runControl(); }
    calcu::machine::Debugging& debugging() { return *machine->debugging(); }
    bool paused() { return session->machineSession().paused(); }
    calcu::machine::Ticks advance(calcu::machine::Ticks t) { return session->advanceUnpaced(t); }
    // Advances in pieces of `slice` ticks until the run control pauses, at
    // most `maxSlices` of them.
    bool runUntilPaused(int maxSlices = 2000, calcu::machine::Ticks slice = 20000) {
        for (int i = 0; i < maxSlices && !paused(); ++i) advance(slice);
        return paused();
    }
};

// A Sharp model's machine without ROMs (its core runs what the test pokes).
inline std::unique_ptr<calcu::machine::Machine> bareMachine(const char* model) {
    if (std::string(model) == "PC-1600") return std::make_unique<PC1600Machine>();
    return std::make_unique<PC1500Machine>(std::string(model) == "PC-1500A" ? PC1500Variant::PC1500A
                                                                             : PC1500Variant::PC1500);
}

} // namespace sharpdebugtest
