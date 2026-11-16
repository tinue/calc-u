# C1600-3: QA report

Results per acceptance criterion and edge case of `spec.md`. The row is
done in M-50; M-45 built its layer in `app/dap/` early (design §1.1;
LIB-10 design §10.15, D-15) and tested the criteria of that layer. The
other criteria are reported in M-50.

## The layer in `app/dap/` (M-45, 2026-11-16, branch `m45-paper-debugger-1600`)

Built and run with LIB-10's Calc-U-1600 column (its qa-report gives the
trees and the tools).

The code: `products/calc-u-1600/app/dap/` (namespace `shared::dap`, no
Qt type, compiled in every tree but Emscripten, `cmake/CalcU.cmake`):
- `DapTargets.*`: the threads (the debug targets in the machine's order,
  from 1; a thread id other than 1 the last), registers by name with their
  halves and flags, banks, memory, decoding and history, over
  `machine::Debugging` and `sharp::SharpDebugTarget`;
- `DapBreakpoints.*` (from `core/Debug/BreakpointTable.*`): source-line,
  instruction, function and data breakpoints as breakpoints and watches of
  the library's table with the setter `dap`, the qualifier of their
  binding and a `HitTest` (condition, hit condition, log message); the
  DAP's IDs from the run state's hits (`dapIds`, `entryHit`); log lines
  and conditions that cannot be worked out through `takeOutput()`;
- `DapStepping.*` (from `core/Debug/RunControl.*`): `next`, `stepIn`,
  `stepOut` on the library's `step`; a source-line step is one step with a
  test (LIB-10 D-16).

The Qt `DapSession` and `DebugController` work on them until M-50 (LIB-10
adapter L-A1); `tools/dap_smoke.py --app` passes against the Qt app
(`CalcU1600DapSmoke`, Longer).

| AC | Result | Test |
|---|---|---|
| AC-12 | Pass | `dap_layer_tests.cpp`: `c1600_3_ac12_step_goes_on_until_the_line_changes` (one step ends once, on a new line; code without source ends a line step after one instruction), `c1600_3_ac12_instruction_granularity_one_instruction`; `c1600_3_ac23_line_breakpoint_and_line_steps` (89 → 90 → 94, an instruction step, step out) |
| AC-14 | Pass | `c1600_3_ac14_breakpoints_belong_to_dap` (the DAP's set again, the harness's kept), `c1600_3_ac14_log_message_writes_and_does_not_pause`, `c1600_3_ac14_unworkable_condition_pauses_and_says_why`, `c1600_3_ac14_other_bank_passed_status_only_while` (PU = 1 only: passed, status "Only while pu=1") |
| AC-23 | Pass | `dap_layer_tests.cpp`: `c1600_3_ac23_line_breakpoint_and_line_steps`, `_conditions_hit_counts_and_logpoints`, `_bank_qualified_listing`, `_hit_condition_parsing` (0.8.1's `run_control_tests.cpp:115-150, 171-211, 236-290`), `_expressions`, `_compiled_expressions_match_legacy`, `_hit_conditions_and_log_templates_match_legacy`, `_function_breakpoint_on_lh5803_symbol` (0.8.1's `debug_target_tests.cpp:84, 637-732`); `listing_tests.cpp` unchanged (the `c1600_3_ac23_` prefix is added in M-50 with the row's other cases) |

AC-1 to AC-11, AC-13, AC-15 to AC-22, AC-24, AC-25 and EC-1 to EC-7: M-50.

### Deviations

- **Today's cases on a session.** The memtest cases run in an app
  session with only Calc-U-1600's `DebuggerPart` around a ROM-less
  PC-1500A (`tests/SharpDebugRig.hpp`, `tests/MemtestRig.hpp`), paused at
  the harness's start; 0.8.1's run control started paused itself.
- **The target view** casts the library's targets to
  `sharp::SharpDebugTarget` for what the generic interface does not hold
  (registers by name, banks for display, the history's bytes).
- **`listing_tests.cpp`** keeps its names in M-45.
