#include "TIDebuggerSetup.hpp"
#include "TITraceRecorder.hpp"

namespace cu59 {

calcu::debugger::DebuggerSetup tiDebuggerSetup(TILevelRules& rules, TIDebuggerOptions o) {
    calcu::debugger::DebuggerSetup d;
    d.rules = &rules;
    d.trace.recorder = [](calcu::machine::Machine& m) { return makeTITraceRecorder(m); };
    d.trace.folder = std::move(o.traceFolder);
    d.trace.limitMb = std::move(o.traceLimitMb);
    d.trace.handOver = o.handOver;
    d.runControlSteps = [](const calcu::machine::ModelId&) { return true; };
    d.panelViews = [](const calcu::machine::ModelId&) { return std::vector<std::string>{"calculator", "cpu", "log"}; };
    d.showPanel = std::move(o.showPanel);
    return d;
}

} // namespace cu59
