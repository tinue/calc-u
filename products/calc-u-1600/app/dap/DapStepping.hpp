#pragma once

#include <string>

#include "Debug/SourceMap.hpp"
#include "app/dap/DapTargets.hpp"
#include "debugger/RunControl.hpp"

// ── The DAP's steps ───────────────────────────────────────────────────────
//
// `next`, `stepIn` and `stepOut` of a thread are the library's `step` over,
// in and out of its target. Where the instruction the
// thread stands on has a source line and the client did not ask for single
// instructions, the step goes on until the source line changes or code
// without source is reached: one library step with a test, so it runs
// in the gate's pieces and ends at a breakpoint.

namespace cu1600::dap {

enum class StepKind { Instruction, In, Over, Out };

// The longest a line step runs: instructions of its thread.
inline constexpr std::uint64_t kLineStepLimit = 100'000'000;

// The source line of `thread`'s PC now; false: none.
bool locate(const DapTargets& targets, const debug::SourceMap& map, int thread, debug::SourceLocation* out);

// Starts the step; the run control replies (and the run state says paused,
// reason step) when it is over. `line`: by source line where there is one.
calcu::debugger::RunControl::Result step(calcu::debugger::RunControl& control, const DapTargets& targets,
                                         const debug::SourceMap& map, int thread, StepKind kind, bool line);

} // namespace cu1600::dap
