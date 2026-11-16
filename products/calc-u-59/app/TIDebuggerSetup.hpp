#pragma once
#include "TILevelRules.hpp"
#include "debugger/DebuggerPart.hpp"
#include <functional>
#include <optional>
#include <string>

// Calc-U-59's debugger set-up (LIB-10 design §12.3; spec AC-28 to AC-33):
// the library's DebuggerPart with
// - the levels `cpu` and `program` of TILevelRules (owned by the caller);
// - the trace session on TITraceRecorder: the trace folder where the host
//   has one (Qt6, Swift, the harness), none on the Web, where a trace is kept
//   in memory and handed over with KeepFile when it ends (`handOver`,
//   AC-26, LIB-10 D-10); [trace] max-file-size-mb MiB for every trace;
// - the run-control steps on every model; the panel views `calculator`,
//   `cpu` and `log`, shown through `showPanel` (adapter L-E4, until M-56);
// - no symbol readers (D-5: the TI has none).

namespace cu59 {

struct TIDebuggerOptions {
    std::function<std::optional<calcu::settings::Paths::Ensured>()> traceFolder;   // unset: none
    std::function<long()> traceLimitMb;                                             // unset: 50
    bool handOver = false;
    std::function<void(calcu::sessions::SessionContext&, const std::string& view)> showPanel;
};

calcu::debugger::DebuggerSetup tiDebuggerSetup(TILevelRules& rules, TIDebuggerOptions options);

} // namespace cu59
