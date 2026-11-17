#pragma once

// The product commands of Calc-U-59 under `c59` (LIB-11 AC-13; harness.md
// H-2): C59-2's program memory (`partition`, `program`, `registers`), C59-4's
// debug panel (`asm`, `dump`, `log-level`, `heatmap`) and C59-9's (design §3):
// `coverage`, `line-times`, `run-time`, `hot-steps`, `step-trace`,
// `step-cost`, `keycodes list|disassemble|assemble`. Every handler reaches
// the machine through the session's commands and the capabilities (the
// debug target's profile, c59.program-memory, c59.calculator-state, the
// paper output), never a machine class (C59-1 AC-17). In the harness only
// (calcu59_harness), not in the frontends' builds.

namespace calcu::harness {
class CommandRegistry;
}

namespace cu59 {

void addTIHarnessCommands(calcu::harness::CommandRegistry&);

} // namespace cu59
