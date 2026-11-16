#pragma once
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "../app/dap/DapBreakpoints.hpp"
#include "../app/dap/DapStepping.hpp"
#include "../app/dap/DapTargets.hpp"
#include "../core/Debug/Listing/Listing.hpp"
#include "../core/Debug/SourceMap.hpp"
#include "../core/PC1500/PC1500InterfaceMachine.hpp"
#include "../core/PC1500/PC1500Machine.hpp"
#include "SharpDebugRig.hpp"

// The real memtest program (fixtures/listings/sdas-lh5801/memtest_stock.bin,
// ENTRY 0x40C5) and its sdas listing on a PC-1500A with no ROM, in a session
// with Calc-U-1600's DebuggerPart, driven through the DAP's layer
// (app/dap/: DapBreakpoints, DapStepping, DapTargets) -- today's run-control
// cases (0.8.1's Core/tests/run_control_tests.cpp) on the library's run
// control (LIB-10 AC-44; C1600-3 AC-12, AC-14, AC-23). A small harness calls it:
//
//   4600  sjp 0x40C5      ; BE 40 C5
//   4603  bch 0x4603      ; 9E 02  -- parks here when memtest returns
//
// memtest tests the RAM between BOTTOM (&7867/8) and RAM_END_H (&7864),
// set up here as 4200-43FF, once (X = 1). The rig starts paused at 4600.

namespace memtest {

struct Rig {
    sharpdebugtest::Rig rig{sharpdebugtest::bareMachine("PC-1500A")};
    PC1500Machine& machine = static_cast<PC1500InterfaceMachine&>(*rig.machine).core();
    debug::SourceMap map;
    shared::dap::DapBreakpoints breakpoints;
    std::string asmFile;
    std::vector<std::string> output;   // what log messages and broken conditions wrote
    bool ok = false;

    Rig() {
        std::ifstream bin("tests/fixtures/listings/sdas-lh5801/memtest_stock.bin", std::ios::binary);
        std::vector<uint8_t> image((std::istreambuf_iterator<char>(bin)), std::istreambuf_iterator<char>());
        debug::Listing listing;
        std::string error;
        if (image.empty() || !debug::loadListing("tests/fixtures/listings/sdas-lh5801/memtest.rst", &listing, &error))
            return;
        asmFile = listing.files[0];
        ok = true;
        for (size_t i = 0; i < image.size(); i++) ok = ok && machine.memory().poke(uint16_t(0x40C5 + i), image[i]);
        const uint8_t harness[] = {0xBE, 0x40, 0xC5, 0x9E, 0x02};
        for (size_t i = 0; i < sizeof harness; i++) ok = ok && machine.memory().poke(uint16_t(0x4600 + i), harness[i]);
        ok = ok && machine.memory().poke(0x7867, 0x42) && machine.memory().poke(0x7868, 0x00) &&
             machine.memory().poke(0x7864, 0x44);
        LH5801& cpu = machine.cpu();
        cpu.setPC(0x4600);
        cpu.setSP(0x7AFF);
        cpu.setX(0x0001);
        const int id = map.addLoaded(1, listing, {}, 0x40C5, uint16_t(0x40C5 + image.size() - 1), "memtest.rst");
        ok = ok && map.verify(id, targets().bankMatch(), targets().codePeek()) == 0;
        rig.rc().pause(calcu::debugger::kCpu);   // a pause of `cpu` lands at once
    }

    shared::dap::DapTargets targets() { return shared::dap::DapTargets(rig.machine->debugging()); }
    shared::dap::DapBreakpoints::SymbolLookup symbols() {
        return [this](const std::string& name, int64_t* value) {
            uint16_t v = 0;
            if (!map.symbolValue(name, &v)) return false;
            *value = v;
            return true;
        };
    }
    void apply() { breakpoints.apply(rig.rc().table(), targets(), symbols()); }

    std::vector<shared::dap::BreakpointStatus> lineBreakpoint(int line, const std::string& condition = {},
                                                              const std::string& hit = {}, const std::string& log = {}) {
        shared::dap::DapBreakpoints::SourceRequest r;
        r.line = line;
        r.condition = condition;
        r.hitCondition = hit;
        r.logMessage = log;
        auto st = breakpoints.setSource(asmFile, {r}, map);
        apply();
        return st;
    }
    void clearBreakpoints() {
        breakpoints.clear();
        apply();
    }
    void resume() { rig.rc().resume(); }
    // Advances until the run control pauses; true if it did.
    bool runUntilStopped(int maxSlices = 2000) {
        const bool p = rig.runUntilPaused(maxSlices);
        for (std::string& l : breakpoints.takeOutput()) output.push_back(std::move(l));
        return p;
    }
    void step(shared::dap::StepKind kind, bool line) { shared::dap::step(rig.rc(), targets(), map, 1, kind, line); }
    calcu::debugger::RunState state() { return rig.rc().state(); }
    bool reasonIs(calcu::debugger::RunState::Reason r) {
        const auto s = state();
        return s.paused && s.reason && *s.reason == r;
    }
    std::vector<int> stopIds() { return breakpoints.dapIds(state().hits); }
    uint16_t pc() { return targets().pc(1); }
    int line() {
        debug::SourceLocation loc;
        return shared::dap::locate(targets(), map, 1, &loc) ? loc.line : 0;
    }
};

} // namespace memtest
