#include "TIRunProbe.hpp"
#include "../core/TIDebugKinds.hpp"
#include "../core/TITypingSupport.hpp"
#include <algorithm>

namespace cu59 {

namespace {

constexpr std::uint16_t kFlagBusy = 0x8000;   // the printer busy bit (2.3.0's FLG_BUSY)
constexpr std::uint16_t kFlagIdle = 0x0001;   // the idle flag (2.3.0's FLG_IDLE)

// The ROM's print-wait spin loops of the TI-59's ROM (also the TI-58's;
// 2.3.0's tools/agentic/hotsteps.py:22). None is known for the TI-58C (D-6).
constexpr std::uint16_t kPrintWaitLoops[] = {0x154C, 0x154D, 0x1425, 0x1426, 0x1427, 0x17C1,
                                             0x17C2, 0x17C3, 0x15E4, 0x15E5, 0x1612, 0x1613};

} // namespace

RunNote noteRun(calcu::machine::Machine& m) {
    RunNote n;
    auto* cs = kindOf<CalculatorState>(m);
    if (!cs) return n;
    const CpuFrame f = cs->snapshotCPU();
    const auto* s = f.SCOM[0];
    n.step = ((s[7] & 0xF) * 100 + (s[6] & 0xF) * 10 + (s[5] & 0xF)) * 8 + (s[4] & 0xF);
    n.source = static_cast<std::uint8_t>(s[3] & 0xF);
    n.printerBusy = (f.flags & kFlagBusy) != 0;
    n.running = (f.fB & 1) != 0;
    n.pc = f.pc;
    n.displayOn = f.displayOn != 0;
    if (m.descriptor().model.value != "TI-58C")
        n.printWait = std::find(std::begin(kPrintWaitLoops), std::end(kPrintWaitLoops), f.pc) != std::end(kPrintWaitLoops);
    const auto r = idleAndKeyDetectRange(m.descriptor().model);
    const bool inLoop = (f.pc >= r[0] && f.pc <= r[1]) || (f.pc >= r[2] && f.pc <= r[3]);
    n.cpuIdle = !n.running && inLoop && (f.flags & kFlagIdle) != 0;
    return n;
}

bool runIdle(calcu::machine::Machine& m) {
    if (!m.isKeyReady()) return false;
    const RunNote n = noteRun(m);
    return !n.running && !n.printerBusy;
}

} // namespace cu59
