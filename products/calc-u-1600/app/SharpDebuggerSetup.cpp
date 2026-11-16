#include "app/SharpDebuggerSetup.hpp"

#include "app/SharpSymbolReaders.hpp"
#include "app/SharpTraceRecorder.hpp"

namespace shared {

calcu::debugger::DebuggerSetup sharpDebuggerSetup(const AppPaths* paths, const Settings* settings, bool hasTraceFolder) {
    calcu::debugger::DebuggerSetup d;
    d.rules = nullptr;   // CpuOnlyRules (D-17)
    d.trace.recorder = [](calcu::machine::Machine& m) { return makeSharpTraceRecorder(m); };
    if (hasTraceFolder && paths) d.trace.folder = calcu::debugger::traceFolderOf(paths->places());
    if (settings) d.trace.limitMb = [settings] { return static_cast<long>(settings->traceMaxFileSizeMB()); };
    d.trace.handOver = false;   // LIB-10 D-10: the Web fails a trace (AC-40)
    d.symbolReaders = sharpSymbolReaders();
    d.runControlSteps = [](const calcu::machine::ModelId&) { return false; };   // AC-19 on the Sharp models
    return d;
}

} // namespace shared
