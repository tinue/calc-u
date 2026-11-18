#pragma once

#include "app/AppPaths.hpp"
#include "app/Settings.hpp"
#include "debugger/DebuggerPart.hpp"

// Calc-U-1600's debugger set-up: the
// library's DebuggerPart with
// - the level `cpu` only (CpuOnlyRules; pause-on-start fires when the
//   processor leaves key readiness);
// - the trace session on SharpTraceRecorder: the trace folder where the host
//   has one (Qt6, the harness's run folder; LibraryParts sets it from the
//   host's places, with [trace] max-file-size-mb MiB for every trace),
//   none on the Web, and no hand-over (a trace fails there);
// - none of the run-control steps (pause:, resume:, step:, pause-on-start:,
//   debug-panel:) on any Sharp model; no panel views;
// - the symbol readers of the Sharp listings.

namespace cu1600 {

calcu::debugger::DebuggerSetup sharpDebuggerSetup();

} // namespace cu1600
