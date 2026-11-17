#pragma once

#include "app/AppPaths.hpp"
#include "app/Settings.hpp"
#include "debugger/DebuggerPart.hpp"

// Calc-U-1600's debugger set-up (LIB-10 design §10.3, §10.6, §10.7): the
// library's DebuggerPart with
// - the level `cpu` only (CpuOnlyRules; D-17: pause-on-start fires when the
//   processor leaves key readiness, as FS 15 found it);
// - the trace session on SharpTraceRecorder: the trace folder where the host
//   has one (Qt6, the harness's run folder; LibraryParts sets it from the
//   host's places, with [trace] max-file-size-mb MiB for every trace,
//   AC-43), none on the Web, and no hand-over (LIB-10 D-10: a trace fails
//   there, AC-40);
// - none of the run-control steps (pause:, resume:, step:, pause-on-start:,
//   debug-panel:) on any Sharp model (AC-19, AC-40); no panel views;
// - the symbol readers of the Sharp listings (AC-51).

namespace cu1600 {

calcu::debugger::DebuggerSetup sharpDebuggerSetup();

} // namespace cu1600
