# LIB-10: QA report

Results per acceptance criterion and edge case of `spec.md`, per column.

## Library (M-40, 2026-11-15, branch `m40-debugger`)

Built and run on an M4 Pro, macOS 27.2, Apple clang, CMake, Ninja,
RelWithDebInfo: `build/tests` (`-DCALCU_FRONTEND_QT=OFF`) and `build/qt`
(Qt6 from Homebrew), and the library's Web test product in `build/web`
(Homebrew Emscripten; Playwright in Chromium and WebKit).

The code:
- `library/machine/DebugTarget.hpp` (new): the debug-target interface and
  the machine's side (`InstructionEvent`, `InstructionObserver`, `Checks`,
  `DebugStop`, `Debugging`); `Machine::debugging()` (default `nullptr`)
  and `Machine::debugTarget(id)`; `checkDebugging()` in
  `machine/Conformance.*`.
- `library/debugger/` (new area, `calcu::debugger`, in `calcu_library`):
  `Levels.*` (`LevelRules`, `CpuOnlyRules`), `Breakpoints.*`
  (`BreakpointTable`), `RunControl.*` (the gate and G1 to G14),
  `RunState.*` (the `run-state` and `trace` payloads and the session's
  names), `TraceSession.*` (`TraceFile`, `TraceRecorder`,
  `TraceSession`), `Profile.*`, `Symbols.*`, `DebuggerPart.*` (the
  session commands and requests of §3.6), `DebuggerSteps.cpp` (the steps
  of AC-19 on LIB-6's step place), `ButtonRule.*`.
- `library/harness/DebugCommands.cpp` (new), `Commands.hpp`,
  `FirstCommands.cpp` (`wait`, AC-21), `Product.hpp`
  (`HarnessProduct::debuggerPart()`), `Run.cpp` (the run's trace folder
  and limit, the steps in the vocabulary).
- `library/qt6/RunControlButtons.*` (in `calcu_qt6`) and
  `library/web/components/RunControlButtons.jsx` (in
  `COMPONENT_SOURCES`).
- Sessions (each its own commit): `SessionPart::machineRestarted` with
  `sessions::Restart` and `SessionContext::tellRestarted`
  (`SessionPart.hpp`, `AppSession.cpp`; LIB-18 §8); an operation's
  `OperationContext::run` gives way while the run control pauses
  (`Operation.cpp`); `MachineSession::setMachine` clears the paused flag
  (`MachineSession.*`). `startup/StartupPart.cpp` tells the parts of a
  Reset and a Cold boot.
- Test stand-ins: the test processor `library/machine/tests/TestCpu.*`
  behind `test-seg`'s `test-cpu` and `test-dot`'s `main` and `inner`,
  switched on by the build option `TestProgram` (or `enableCpu`); the
  test rules, trace format and symbol table
  (`library/debugger/tests/TestDebugParts.hpp`), the session rig
  (`DebugRig.hpp`: the preset rig's parts with the DebuggerPart), the
  rule's table `button_cases.json`; the harness's test product got the
  test processor in both models and a DebuggerPart; the library's Web test
  product a DebuggerPart without a trace folder (hand-over).

The tests: `library/machine/tests/debug_target_tests.cpp`,
`library/debugger/tests/*_tests.cpp`,
`library/harness/tests/debug_commands_tests.cpp` in `calcu_library_tests`
(Frequent; 98 new `TEST`s, 727 in all), the CTest entry
`calcu_cli.ac50_refused_expect_ends_one_shot_nonzero` (Frequent),
`library/qt6/tests/run_control_buttons_tests.cpp` in `calcu_qt6_tests`
(Frequent, offscreen) and `library/web/tests/run-control.spec.js` in
`LibraryWebPlaywright` (Longer, Chromium and WebKit).

| ID | Result | Evidence |
|---|---|---|
| AC-1 | Pass | `ac1_one_target_per_processor_named_as_descriptor` (one target on test-seg, two on test-dot, named as the descriptor lists them; none without the option), `ac1_target_states_registers_spaces_code_space_offers_qualifier_now`, `conformance_check_debugging` (`machine/tests/debug_target_tests.cpp`); `ac20_targets_registers_set_register` (the `targets` block) |
| AC-2 | Pass | `ac2_registers_read_written_where_writable`, `ac2_peek_no_effect_unreadable_device_cell`, `ac2_refused_poke_changes_nothing`; `ac20_peek_poke_marks_unreadable_refuses_at_first` |
| AC-3 | Pass | `ac3_next_address_and_pre_execution_state` |
| AC-4 | Pass | `ac4_history_newest_first_with_registers_before` (newest first, the registers before, 8 kept); `ac20_history_disassemble_with_text` |
| AC-5 | Pass | `ac5_session_has_one_run_control_unused`, `ac5_paused_no_emulated_time_display_kept`, `ac5_stopped_advance_gives_ticks_run` (`debugger/tests/run_control_tests.cpp`); a preset's `wait:` on the paused machine passes in host time and ends paused: `ac13_armed_start_fires_in_operation` |
| AC-6 | Pass | `ac6_pause_lands_where_rules_say_reply_then` (the reply when `program` has landed at the next boundary; `cpu` at once), `ac6_resume_whichever_level_owned`, `ac6_unknown_level_refused` |
| AC-7 | Pass | `ac7_cpu_step_one_instruction_of_named_or_current_target`, `ac7_cpu_step_repeats_waiting_instruction_by_rule`, `ac7_program_step_by_rule`, `ac7_running_while_stepping`, `ac7_step_refused_when_not_paused`, `ac7_hit_ends_steps` |
| AC-8 | Pass | `ac8_step_over_call_runs_routine`, `ac8_step_out_runs_to_return`, `ac8_breakpoint_inside_still_pauses`, `ac8_refused_without_offer` (port of Calc-U-1600 `run_control_tests.cpp:159` onto the test processor); `ac20_step_over_out_refused_without_offer` |
| AC-9 | Pass | `ac9_armed_fires_at_rule_event_once`, `ac9_arming_one_disarms_other_off_both_pause_both`, `ac9_default_cpu_rule_leaves_key_readiness` (`debugger/tests/start_tests.cpp`) |
| AC-10 | Pass | `ac10_pauses_before_instruction`, `ac10_qualifier_narrows`, `ac10_resume_and_step_pass_it_once`, `ac10_two_targets_only_the_one_to_run_passes` (`debugger/tests/breakpoint_tests.cpp`; ports of `debug_target_tests.cpp:111-193, 464`) |
| AC-11 | Pass | `ac11_read_write_both_pause_after_instruction`, `ac11_fetch_is_no_access`, `ac11_refused_without_offer` (ports of `debug_target_tests.cpp:269-301`) |
| AC-12 | Pass | `ac12_test_says_go_on` (a setter's `HitTest` asked at each hit; the machine went on twice) |
| AC-13 | Pass | `ac13_no_breakpoint_or_watch_in_operation`, `ac13_commanded_pause_step_resume_in_operation` (a preset's `pause:`, `step:`, `resume:`), `ac13_armed_start_fires_in_operation` (the library form of `test_cdebug.ti59`: armed inside the load, fired by the key that starts the program, inside its hold, C-1 of FS 5) |
| AC-14 | Pass | `ac14_pause_stops_all_targets_names_current` (test-dot: neither target runs; the run state names the current one) |
| AC-15 | Pass | `ac15_reset_and_cold_boot_while_paused_run`, `ac15_armed_stays_across_reset`, `ac15_new_machine_not_paused_nothing_armed`, `ac15_breakpoints_rebound_by_target_name_rest_dropped` |
| AC-16 | Pass | `ac16_resume_sets_clocks_and_starts_afresh` (paced: test-dot's clock set from the host once, pacing started afresh, the pause not made up), `ac16_paused_survives_suspend_and_continue` |
| AC-17 | Pass | `ac17_run_state_fields_for_each_reason` (pause, step, breakpoint and watch with the IDs and setters, pause-on-start, entry; the armed level, pause-at-start) |
| AC-18 | Pass | `ac18_rule_cases_from_table`, `ac18_step_repeats_four_a_second_while_held` (`debugger/tests/button_rule_tests.cpp`); `ac18_qt6_buttons_follow_run_state_and_send_commands` (`calcu_qt6_tests`, offscreen: the table, an app session, a held step repeating at 250 and 500 ms, the product's labels); `ac18 react buttons follow the run state` (`LibraryWebPlaywright`, Chromium and WebKit: the table, the test product's session, a held step). Placed in a shell by LIB-14 (M-43) and by the products (M-45, M-53, M-58) |
| AC-19 | Pass | `ac19_each_step_does_what_its_ac_says`, `ac19_level_view_count_out_of_range_refused_at_read`, `ac19_step_not_paused_logs_and_goes_on` (OQ-4), `ac19_undefined_steps_refused_for_a_model` (test-dot stands for a Sharp model), `ac19_trace_defined_for_every_model`, `ac19_no_step_waits_for_a_running_machine`, `ac19_debug_panel_is_a_frontend_step` (`debugger/tests/steps_tests.cpp`). With them LIB-6 AC-19's debugger steps and AC-63's `debug-panel:` pass (LIB-6 qa-report updated) |
| AC-20 | Pass | `ac20_targets_registers_set_register`, `ac20_peek_poke_marks_unreadable_refuses_at_first`, `ac20_break_watch_on_off_list_own_only`, `ac20_history_disassemble_with_text`, `ac20_profile_counts_and_report`, `ac20_step_over_out_refused_without_offer`, `ac20_pause_at_start`, `ac20_state_run_state_and_trace`, `ac20_step_not_paused_refused` (`harness/tests/debug_commands_tests.cpp`, the test product) |
| AC-21 | Pass | `ac21_wait_ends_at_pause_gives_time_and_state`, `ac21_wait_while_paused_ends_at_once` |
| AC-22 | Pass | `ac22_trace_records_every_instruction_until_off`, `ac22_ends_with_its_preset_or_new_machine`, `ac22_replaces_file_one_at_a_time`, `ac22_slash_refused_at_read`, `ac22_cannot_make_file_step_fails` (`debugger/tests/trace_tests.cpp`) |
| AC-23 | Pass | `ac23_button_name_is_products` (`trace {button: true}`: the product's name, replaced at each start). The products' buttons send it in M-45 and M-53 |
| AC-24 | Pass | `ac24_limit_ends_complete_file_notice` (1 MB: the file ends complete within the limit, the notice of D-11), `ac24_new_machine_log_line` |
| AC-25 | Pass | `ac25_trace_topic_whoever_started` (the command, the button, a preset's, paced) |
| AC-26 | Pass (library part) | `ac26_ended_trace_in_memory_handed_with_keep_file` (no folder, hand-over: `KeepFile` with the name, the bytes and the media type; without hand-over the start fails, D-10); `ac26 an ended trace is a download` (`LibraryWebPlaywright`, LIB-15's test product, Chromium and WebKit). Calc-U-59's Web uses it in M-53 |
| AC-45 | Pass | `ac45_other_level_step_takes_the_pause` |
| AC-46 | Pass | `ac46_paused_before_first_instruction_after_reset_cold_boot_build`, `ac46_once_and_separate_from_pause_on_start` |
| AC-47 | Pass | `ac47_ids_owners_remove_only_own`, `ac47_hit_names_ids_and_owners`; `ac20_break_watch_on_off_list_own_only` (the harness's `break off` leaves a debugger's) |
| AC-48 | Pass | `ac48_written_view_last_value_or_none_no_effect`; `ac20_peek_poke_marks_unreadable_refuses_at_first` (`io-written` cell with none) |
| AC-49 | Pass | `ac49_every_bank_and_vbank_by_name_no_switch`, `ac49_range_across_banks_refused`; through the commands `ac20_peek_poke_marks_unreadable_refuses_at_first` (`bank=`, `vbank=`, a range across banks refused, no bank switched) |
| AC-50 | Pass | `ac50_peek_into_file_gives_count`, `ac50_expect_register_refused_with_value`, `ac50_expect_memory_refused_at_first_difference`; `calcu_cli.ac50_refused_expect_ends_one_shot_nonzero` (exit status 1, the commands after it not run) |
| AC-51 | Pass | `ac51_symbol_names_as_addresses_and_in_output` (`break`, `disassemble`, `peek`, `break off` by name; `name+offset` in `history` and `disassemble`), `ac51_unknown_name_refused` (an unknown name; a file no reader takes) |
| EC-1 | Pass | `ec1_pause_while_paused_done_nothing_changes` |
| EC-2 | Pass | `ec2_resume_while_running_done` |
| EC-3 | Pass | `ec3_past_end_of_space_refused`; `ac20_break_watch_on_off_list_own_only` |
| EC-4 | Pass | `ec4_several_hits_pause_once_lowest_address_named` (a two-cell store hits two watches: one pause, the lower address named) |
| EC-5 | Pass | `ec5_step_from_breakpoint_carries_it_out` |
| EC-6 | Pass | `ec6_large_count_ended_by_pause_reset_new_machine` |
| EC-7 | Pass | `ec7_preset_ends_paused_stays_paused` |
| EC-8 | Pass | `ec8_no_writable_folder_step_fails_with_reason` (no folder; a folder that cannot be written) |
| EC-9 | Pass | `ec9_peek_poke_outside_space_refused`; `ac20_peek_poke_marks_unreadable_refuses_at_first` |

Column status: every criterion (AC-1 to AC-26, AC-45 to AC-51) and edge
case (EC-1 to EC-9) of the Library column passes. The Library column is
**Done** (plan §5.1: M-40). The product columns are worked in M-45
(Calc-U-1600) and M-53 (Calc-U-59).

### Exit check of M-40

Library tests: `calcu_library_tests` 727 `TEST`s, 0 failed; Frequent in
`build/tests` (31 entries) and `build/qt` (32 entries, with
`calcu_qt6_tests` at 11 `TEST`s) pass; `LibraryWebPlaywright` 96 passed,
2 skipped (the run-control cases in both engines among them);
`LibraryWebNodeTests` pass. No product holds a DebuggerPart yet (design
§4): their harness entries are unchanged.

### Golden runs

M-40 touches no core and no product code. Both products' golden replays
pass unchanged in their Frequent suites (`CalcU59Tests`, `CoreTests`).
`Machine::debugging()` has a default, so both products' machines build
unchanged.

### Deviations from the design, chosen by the simpler structure

- **Where the restart is told.** Reset and Cold boot are carried out in
  `StartupPart::restart` (LIB-18), not in `MachineControl`; the parts are
  told from there, through `SessionContext::tellRestarted` (new, in
  `sessions`), which the app session implements by calling
  `SessionPart::machineRestarted` on every part. A power cycle is no
  restart (AC-46 names Reset, Cold boot and a build).
- **An operation on a paused machine (§3.11, G12, D-8).** Two more
  `sessions` changes than §3.11 lists, each in its own commit:
  `OperationContext::run` gives the host its call and ends with what ran
  while the run control keeps the machine paused (before, it failed with
  "No machine is running."), so that a typing wait looks at its key again
  (the key of C-1 comes up on the paused machine) and what needs the
  machine waits until it runs; and `MachineSession::setMachine` clears the
  paused flag (AC-15), so that an operation that builds a new machine runs
  it at once. In virtual time the operation then does not finish, as D-8
  says; its reply carries no run state (D-8 says it does): the run state
  is read with `state run-state`.
- **Interface details.** `Debugging` has a `const target(i)` overload
  (the conformance check reads through it). `LevelRules` and
  `TraceRecorder` have public virtual destructors (owned by the part and
  by a `unique_ptr`). `TraceRecorder::machineGone()` is new: on a new
  machine the trace ends without touching the old machine's core.
  `DebuggerSetup` holds `TraceSetup` (recorder, folder as
  `Paths::ensure` gives it, limit, hand-over), the symbol readers, and
  per model whether the run-control steps are defined, the panel's views
  and the frontend's `showPanel`; `setTraceFolder` and `setTraceLimit`
  for a host that makes its folder later (the harness).
- **The default `cpu` start rule after a restart.** A restart arms the
  rule afresh (`startArmed`), so a Reset by itself does not count as the
  processor leaving key readiness; the TI rules of AC-30 override it in
  M-53.
- **Entry does not disarm.** The pause at the next start (AC-46) leaves
  an armed pause-on-start armed: the two stay separate (AC-46); every
  other pause disarms both (AC-9).
- **Run-control commands need a target.** `pause`, `step`, `resume`,
  `pause-on-start`, `profile` and the target commands are refused with
  "The machine offers no debug target." on a machine whose
  `debugging()` is `nullptr`, as §4 says for the products until their
  columns.
- **Harness forms.** `step`, `step <count>`, `step <level> [<count>]` and
  `step { level: …, count: … }`; `pause` without a level is `cpu`. `wait`
  gives the run state when the machine is paused at its end (it ended at
  a pause, or at once on a paused machine); a wait that ran its full time
  keeps LIB-11's reply. `peek … file=` writes a relative path into the
  run's folder for traces and screenshots (LIB-11 AC-8). `break off
  <target> <address>` takes the harness's breakpoints at that address. A
  `poke` refused at a cell writes back the cells before it, so a refused
  poke changes nothing. A watch on a `-written` view is refused (the view
  is read-only).
- **The test processor.** Beyond §6: `st2` (two cells in one
  instruction, for EC-4), the option `bare` (targets without the optional
  parts, for the refusals of AC-8, AC-11 and AC-20), the inner target's
  program at 0x50 (a loop on itself unless given). The machine's tick
  count and key readiness are kept per instruction while the processor
  runs, so the observer sees the machine after each instruction. The
  harness's test product runs it in both models.
- **The Playwright projects.** `run-control-chromium` and
  `run-control-webkit` in `library/web/tests/playwright.config.js`; the
  rule is reached in the page as `CalcUTest.buttonUse` (the one global of
  the test product's `calc.js`).
