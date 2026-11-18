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

## The server, the hosts and the remaining criteria (M-50, 2026-11-16, branch `m50-dap-1600`)

Built and run on macOS (Apple silicon) in three root trees of the worktree:
`build/tests` (Debug, the tests at `-O1`), `build/rel` (RelWithDebInfo, for
the Frequent times) and `build/qt` (`-DCALCU_FRONTEND_QT=ON
-DCALCU_TESTS_LONGER=ON`); `tools/build_wasm_check.sh` passed (the Web build
has no `app/dap/`).

The code: `products/calc-u-1600/app/dap/` gained `DapPart` (the server, a
session part after the `DebuggerPart`), `DapSession` (the requests and
events), `DapSetup.cpp` (the set-up, Build & Load, `restart`,
`calcu1600/load`, `calcu1600/reset` as operations of the session),
`DapConfig`, `DapJson`, `DapWire`, `DapTransport`. The Qt6 frontend keeps
only `frontends/qt6/app/debug/DapSocket` (a `QTcpServer`); the harness hosts
the same part with `calcu --product 1600 serve --dap <port>`
(`library/harness/ServeAddOn.hpp`, `library/harness/attach/LocalStream`).
`DapServer`, the QJson `DapSession`, `DebugController` and
`LegacyCalls::loadProgram` are gone.

Tests: `CalcU1600DapTests` (`tests/dap/dap_tests.cpp`, 27 `TEST`s, Frequent,
1.4 s RelWithDebInfo), `CalcU1600DapPortCheck` (Frequent, 0.03 s),
`serve_addon_tests.cpp` in `calcu_library_tests` (2 `TEST`s, Frequent),
`CalcU1600DapSmoke` (`tools/dap_smoke.py --harness`, Longer, 7.6 s in the
group run, all scenarios passed), `CalcU1600DapSmokeQt` (`--app`, Longer,
33.3 s, all passed), `CalcU1600Shots.c1600-3-ac18` (Longer, 3.1 s, passed).
`CoreTests` runs what it ran (57.1 s alone, RelWithDebInfo; `listing_tests.cpp`'s
cases renamed `c1600_3_ac23_…`).

| AC | Result | Test |
|---|---|---|
| AC-1 | Pass | `app/dap/` names no Qt type and no machine class (`CalcU1600MachineIncludeCheck`); the part is in `SharpAppPart` (`c1600_3_ac2_listens_…`: `dapPart()` set natively) and among the harness's parts (`CalcU1600DapSmoke`); the Emscripten tree compiles `app/dap/` out (`tools/build_wasm_check.sh`: `SharpAppPart` without a `DapPart`); the Qt6 app runs it (`CalcU1600DapSmokeQt`) |
| AC-2 | Pass | `c1600_3_ac2_listens_on_127_0_0_1_by_setting_or_run_port` (`[debug] dap-enabled`/`dap-port`, the run port, no setting changed), `c1600_3_ac2_port_taken_status_says_so`; the hosts bind 127.0.0.1 (`DapSocket`: `QHostAddress::LocalHost`; `LocalStream`: `INADDR_LOOPBACK`) |
| AC-3 | Pass | `c1600_3_ac3_second_client_told_and_closed` (the sentence a host sends a second client before it closes it); `LocalStream` and `DapSocket` close it |
| AC-4 | Pass | `c1600_3_ac4_initialize_pauses_configuration_done_resumes`, `c1600_3_ac4_stop_on_entry_stays_paused_entry`; smoke `main` |
| AC-5 | Pass | `c1600_3_ac5_project_block_is_the_base_keys_win`, `c1600_3_ac5_unknown_key_passed_over_numbers_by_preset_rule_bare_hex_refused`; smoke `project_run`, `renum_template_run` (the extension's added `listings`/`symbols`) |
| AC-6 | Pass | `c1600_3_ac6_boot_debug_with_program_refused`, `c1600_3_ac6_preset_loaded_then_paused_command_typed_when_running`, `c1600_3_ac6_reset_stops_at_the_reset_vector_ac24_over_when_it_stops`, `c1600_3_ac6_listings_bound_to_their_cpu_with_qualifier_ec7_told`; smoke `bus_rom_run` (`boot: debug`: the power-on module reset stops at `HDRESET`; the bank-7 listing of the host-drive ROM) |
| AC-7 | Pass | `c1600_3_ac7_build_and_load_entry_and_call_in_setup_waits_for_configuration_done` (ledger 1600-20), `c1600_3_ac7_entry_rules_and_ec3_headerless_without_address`; smoke `program_run` (clean start from the preset, entry rules, numbers), `pc1600_run` (headerless at the listing's lowest address), `lh5803_run` (`cpu: lh5803`, XCALL) |
| AC-8 | Pass | `c1600_3_ac8_messages_held_during_an_operation_answered_in_order`; smoke `renum_template_run` (breakpoints sent while the attach still loads) |
| AC-9 | Pass | `c1600_3_ac9_model_switch_keeps_the_session_invalidated_and_line` |
| AC-10 | Pass | `c1600_3_ac10_disconnect_closed_socket_and_dialog_end_the_session` (all three ways: the setter `dap`'s breakpoints gone, the machine runs on; the dialog's `terminated` first); `CalcU1600Shots.c1600-3-ac18` (Disconnect in the dialog) |
| AC-11 | Pass | `c1600_3_ac11_requests_memory_references_unsupported_ec6_ac15` (threads, frames, scopes, Registers/Flags/Banks with the SC7852's four pages, read/write memory, disassemble, evaluate, numeric memory references, "Unsupported request"); smoke `main`, `rom_run`, `pc1600_run` |
| AC-12 | Pass | M-45 (`dap_layer_tests.cpp`); smoke `program_run` (`next` 89 → 90, `stepIn` → 94, `stepOut` back into the ROM) |
| AC-13 | Pass | `c1600_3_ac6_listings_bound_to_their_cpu_with_qualifier_ec7_told` (bytes that differ: told how many); `listing_tests.cpp` (`c1600_3_ac23_…`); smoke `program_run` (line 88 goes to 89) |
| AC-14 | Pass | M-45 (`dap_layer_tests.cpp`); smoke (function, instruction and line breakpoints) |
| AC-15 | Pass | `c1600_3_ac11_…` (`evaluate`; the error names `nosuch`); M-45 `c1600_3_ac23_expressions` |
| AC-16 | Pass | `c1600_3_ac16_stopped_once_with_reason_ids_and_pause_from_elsewhere` (reason, thread, `hitBreakpointIds`, `allThreadsStopped`, once; a pause of the run control from elsewhere told); the set-up's pauses not told (`c1600_3_ac4_initialize_…`); `memory`, `invalidated`, `terminated` (`c1600_3_ac11_…`, `c1600_3_ac9_…`, `c1600_3_ac10_…`) |
| AC-17 | Pass | `c1600_3_ac17_reset_display_text_quit` (`c1600/display-text` without a DAP session, with the harness's keys (R-4); Reset and Cold boot with and without `stop`; quit ends the session and the program); `load` answers range, entry, command: `c1600_3_ac7_entry_rules_…`, smoke `program_run` |
| AC-18 | Pass | `CalcU1600Shots.c1600-3-ac18` (the status line; Disconnect usable only with a client; Disconnect sends it away; the app started with `--dap 32177`) |
| AC-19 | Pass | the smoke against the server on both hosts: attach, Build & Load (`calcu1600/load`), Reset and Stop, All Reset and Stop, the RENUM template on the fixture ROM (`tests/fixtures/dap/pc1500-bus-rom/`, built once with sdaslh5801 as `extension.js` builds it; no skip); the four templates filled as `scaffold.js` fills them and checked: `calcu_preset_check.CalcU1600Templates` (M-46); the extension's description of `program.after` gives `none` (`package.json`) |
| AC-20 | Pass | `CalcU1600DapSmoke`: the ten scenarios against the harness, each printed `c1600_3_ac20_<scenario>: passed` |
| AC-21 | Pass | `c1600_3_ac21_status_texts_on_the_topic`, `c1600_3_ac2_port_taken_status_says_so`, `c1600_3_ec1_…` ("Off") |
| AC-22 | Pass | `CalcU1600DapSmoke` (a client attaches to `calcu serve --dap` without a window; the scenarios that run the machine pass); `serve_addon_events_and_time_while_wanted` (emulated time only while wanted, no wall clock), `serve_addon_lines_between_tasks_and_cannot_start` |
| AC-23 | Pass | M-45 (`dap_layer_tests.cpp`), `listing_tests.cpp` (the `c1600_3_ac23_` prefix added) |
| AC-24 | Pass | `c1600_3_ac6_reset_stops_at_the_reset_vector_ac24_over_when_it_stops` (the attach is answered once the machine stands at the vector; stepping works); `c1600_3_ac7_build_and_load_…` (breakpoints work after the set-up) |
| AC-25 | Pass | `c1600_3_ac25_rom_project_load_redoes_the_setup`; smoke `bus_rom_run` (host-drive ROM: Build & Load stops there again) |

| EC | Result | Test |
|---|---|---|
| EC-1 | Pass | `c1600_3_ec1_port_out_of_range_not_listening` (refused, a notice, not listening, also over the setting); `CalcU1600DapPortCheck` (the harness: exit status 2 with the reason); the Qt6 app writes the reason to standard error (`main.cpp`) |
| EC-2 | Pass | `c1600_3_ec2_bad_key_refused_with_reason` (`bin`, `after`, `cpu`, `slot`, `reset`) |
| EC-3 | Pass | `c1600_3_ac7_entry_rules_and_ec3_headerless_without_address` (C1600-2's sentence and the hint) |
| EC-4 | Pass | `c1600_3_ec4_startup_preset_for_another_model_refused` |
| EC-5 | Pass | `c1600_3_ec5_request_finished_before_the_end` |
| EC-6 | Pass | `c1600_3_ac11_…` (`setVariable` on frame 1 refused, on frame 0 done) |
| EC-7 | Pass | `c1600_3_ac6_listings_bound_to_their_cpu_with_qualifier_ec7_told` |

M-50's exit check, "VS Code debugs a program as before": `vscode_session`
in `tools/dap_smoke.py` (printed `m50_exit_vscode_session`) sends what VS Code
sends for a launch configuration of the extension: `initialize` with VS
Code's arguments, `launch` with VS Code's own keys, `setBreakpoints` before
the launch is answered, `setFunctionBreakpoints`, `setExceptionBreakpoints`,
`configurationDone`; at each stop `threads`, `stackTrace`, `scopes`,
`variables`; `next`; `continue`, `pause`, `disconnect`. It passed against
the harness (`CalcU1600DapSmoke`) and the Qt app (`CalcU1600DapSmokeQt`).

### What stayed manual

- VS Code itself driving the server (the Run and Debug view, the editor's
  breakpoint gutter, the Disassembly view, the extension's commands Build &
  Load, Reset and Stop, Create Debug Project…): the request sequences are
  replayed by `tools/dap_smoke.py`; VS Code is not run. The extension's code
  is unchanged but for one description.
- CLion's DAP recipe (`docs/Debugger.md`), never tried (TODO.md, as before).
- Windows: the Qt6 app's server (`QTcpServer`) is built in CI but run by no
  test here; the harness has no `--dap` there (D-3).

### Deviations

- **Tests in a program of their own.** The design's T3 to T12 cases are in
  `tests/dap/dap_tests.cpp`, the CTest `CalcU1600DapTests` (Frequent), not in
  `CoreTests`, whose Frequent budget is spent; fewer, broader cases than the
  design names, each criterion above with its case. The design's
  `harness_tests.cpp` cases `c1600_3_ac22_…` are `CalcU1600DapSmoke` (Longer:
  17 s with the scenarios' pauses by hand, D-5) and the library's serve
  add-on tests.
- **`DapTransport`.** `listen(port, busy)` takes the bytes a second client
  gets; the host closes it (no `SecondClient` input, no `lastWords`).
- **The serve add-on.** Its I/O thread posts tasks into the loop
  (`ServeLoop{post, quit}`), not `take(event, session)`; the harness hands a
  product its run's preset part, machine control and settings
  (`HarnessProduct::runParts`), which the DAP's set-up needs.
- **One pause at the end of the set-up** (S8), not one after each step: the
  steps run inside one operation, so nothing sees the difference; an
  operation in virtual time must not ask for the machine after a pause
  (LIB-10 G12).
- **Library changes the set-up needed in virtual time** (the harness runs an
  operation in one go and tells the parts of a new machine only after it):
  `PresetsPart::loadWithin` (a preset loaded inside another part's
  operation); `TypingEngine::setMachine` takes the session's paused state for
  a new machine (it kept the old machine's, so a clean start's keys waited
  forever after a DAP pause); `TypingPart` ignores a second `machineChanged`
  for the machine it follows already (it dropped the starting command typed
  in the set-up).
- **`DapSetup.hpp`** is not there: `DapSetup.cpp` holds `DapSession`'s set-up members.
- **Golden runs.** No core code but `ProgramLoader`'s refusal text changed;
  the golden replays in `CoreTests` passed unchanged.
