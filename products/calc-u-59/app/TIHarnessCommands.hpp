#pragma once

// The product commands of Calc-U-59 under `c59` (docs/formats/harness.md):
// the program memory (`partition`, `program`, `registers`), the debug panel
// (`asm`, `dump`, `heatmap`) and the functions of the earlier
// tools: `coverage`, `line-times`, `run-time`, `hot-steps`, `step-trace`,
// `step-cost`, `keycodes list|disassemble|assemble`. Every handler reaches
// the machine through the session's commands and the capabilities (the
// debug target's profile, c59.program-memory, c59.calculator-state, the
// paper output), never a machine class. In the harness only
// (calcu59_harness), not in the frontends' builds.

namespace calcu::harness {
class CommandRegistry;
}

namespace cu59 {

void addTIHarnessCommands(calcu::harness::CommandRegistry&);

} // namespace cu59
