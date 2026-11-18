#include "app/SharpDebuggerSetup.hpp"

#include "app/SharpSymbolReaders.hpp"
#include "app/SharpTraceRecorder.hpp"

namespace cu1600 {

calcu::debugger::DebuggerSetup sharpDebuggerSetup() {
    calcu::debugger::DebuggerSetup d;
    d.rules = nullptr;   // CpuOnlyRules
    d.trace.recorder = [](calcu::machine::Machine& m) { return makeSharpTraceRecorder(m); };
    d.trace.handOver = false;   // the Web fails a trace
    d.symbolReaders = sharpSymbolReaders();
    d.runControlSteps = [](const calcu::machine::ModelId&) { return false; };   // none on the Sharp models
    return d;
}

} // namespace cu1600
