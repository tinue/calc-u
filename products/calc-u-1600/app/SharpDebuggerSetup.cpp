#include "app/SharpDebuggerSetup.hpp"

#include "app/SharpSymbolReaders.hpp"
#include "app/SharpTraceRecorder.hpp"

namespace shared {

calcu::debugger::DebuggerSetup sharpDebuggerSetup() {
    calcu::debugger::DebuggerSetup d;
    d.rules = nullptr;   // CpuOnlyRules (D-17)
    d.trace.recorder = [](calcu::machine::Machine& m) { return makeSharpTraceRecorder(m); };
    d.trace.handOver = false;   // LIB-10 D-10: the Web fails a trace (AC-40)
    d.symbolReaders = sharpSymbolReaders();
    d.runControlSteps = [](const calcu::machine::ModelId&) { return false; };   // AC-19 on the Sharp models
    return d;
}

} // namespace shared
