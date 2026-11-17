# LIB-5: QA report

Results per acceptance criterion and edge case of `spec.md`, per column.

## Library (M-36, 2026-11-15)

Built and run on an M4 Pro, macOS 27.2, Apple clang, CMake, Ninja,
RelWithDebInfo (`build/tests`, `-DCALCU_FRONTEND_QT=OFF`). The code is
`library/typing/` (namespace `calcu::typing`, in `calcu_library`), the
harness commands `key` and `type` in `library/harness/TypingCommands.cpp`
(registered by `addFirstCommands`), and the LIB-3 additions of design §3.5:
`CommandOutcome::Deferred`, `SessionContext::currentCall()` and
`SessionContext::reply(CallId, …)` (`library/sessions/SessionPart.hpp`,
`AppSession.cpp`), `HarnessHost::command` waiting in virtual time for a
deferred reply (at most an hour of emulated time, D-5), and
`MachineSession::nextDue()` (see Deviations). `SliceHook::drop()` and
`MachineSession::dropInput()` came with M-35 (LIB-18 §3.4); M-36 added the
same lines in its own commit and the merge kept one copy.

The tests are `library/typing/tests/*_tests.cpp`, the deferred-reply case
in `library/sessions/tests/command_tests.cpp` and the `key`/`type` cases in
`library/harness/tests/commands_tests.cpp`, all in `calcu_library_tests`
(Frequent). They run `test-type` (`TypingTestMachine.*`: LIB-1's test-seg
at 100,000 ticks per second with the keys typing needs, a scripted key
readiness, the key log with tick counts, typing support with the latches
`shift` and `kbii` and a sample count), the test profile and map
(`TestProfiles.hpp`: a minimum hold of 1,000 ticks, typed text 1,000 down
and 500 up, 300 more after `shift`/`kbii`, readiness polled every 50
ticks) and the rig `TypingRig.hpp` (an app session with the typing part
and the sessions' test part, its test frontend, the scripted clock), paced
and in virtual time. The harness cases run the harness's test product,
which got a typing part (test-seg: digits 0-3, `clr` as ENTER).

Result of the run: `calcu_library_tests` 423 tests, 0 failed. The whole
Frequent group passed (28 of 28).

| ID | Result | Test case (file in `library/typing/tests/` unless named) |
|---|---|---|
| AC-1 | Pass | `ac1_early_key_up_carried_out_at_min_hold`, `ac1_key_up_never_dropped` (paced), `ac1_typed_press_held_its_cadence_at_least_the_minimum` (`holds_tests.cpp`) |
| AC-2 | Pass | `ac2_live_key_down_at_once_when_not_ready`, `ac2_live_key_after_engine_hold_and_gap` (`live_tests.cpp`) |
| AC-3 | Pass | `ac3_next_press_after_up_gap_and_readiness` (`queue_tests.cpp`) |
| AC-4 | Pass | `ac4_held_until_let_go`, `ac4_let_go_before_start_is_one_min_press`, `ac4_auto_repeat_ignored` (`live_tests.cpp`) |
| AC-5 | Pass | `ac5_two_keys_held_together_both_down`, `ac5_engine_never_overlaps` (`live_tests.cpp`) |
| AC-6 | Pass | `ac6_same_inputs_same_key_log_paced_full_virtual` (`determinism_tests.cpp`): a script of ten inputs run paced at regular speed, paced at full speed and in virtual time; each run's inputs replayed in virtual time at the ticks they were given at give the same key log, tick for tick, twice |
| AC-7 | Pass | `ac7_frontend_event_mapped_by_model_map` (`map_tests.cpp`; the event crosses the session as a Value and back) |
| AC-8 | Pass | `ac8_sequence_presses_key_up_does_nothing` (`map_tests.cpp`) |
| AC-9 | Pass | `ac9_live_drops_waiting_but_finishes_must_finish` (the closing `kbii` of an accent is typed, its `a` and the next character dropped), `ac9_live_input_stops_a_key_command` (`live_tests.cpp`) |
| AC-10 | Pass | `ac10_focus_lost_releases_after_min_drops_waiting` (`live_tests.cpp`) |
| AC-11 | Pass | `ac11_text_typed_with_profile_cadence` (`queue_tests.cpp`) |
| AC-12 | Pass | `ac12_paste_first_line_skips_unknown_no_enter` (`text_tests.cpp`) |
| AC-13 | Pass | `ac13_latched_key_released_first_or_skipped` (with the latches read when the character's first press is due, E13), `char_keys_without_support_same_has_a_key_answer` (D-14) (`text_tests.cpp`) |
| AC-14 | Pass | `ac14_first_press_rule_inside_an_operation`, `ac14_first_press_at_once_overrides_the_rule` (`operation_tests.cpp`), `ec3_first_press_bound_starts_all_the_same` (`queue_tests.cpp`); in a preset (M-38): `lib5_ac14_first_press_after_skip_wait_at_once` (`library/presets/tests/typing_tests.cpp`: right after `skip-wait:` the first press goes down at once, the later ones wait for key readiness, and without it the step waits). The Sharp models in the Calc-U-1600 column |
| AC-15 | Pass | `ac15_key_names_refused_nothing_typed_screen_key_unknown` (`part_tests.cpp`), `ac15_key_names_and_refusal` (`harness/tests/commands_tests.cpp`); in a preset, found when it is read, before anything is built (M-38): `lib5_ac15_key_name_refused_when_read_before_build` (`library/presets/tests/typing_tests.cpp`), `ac7_keys_text_places_and_files_checked_before_build` (LIB-6) |
| AC-16 | Pass | `ac16_type_text_and_enter_refuses_unknown_nothing_typed` (`text_tests.cpp`), `ac16_type_absent_without_char_profile` (`part_tests.cpp`, D-8), `ac16_type_presses_enter_refuses_unknown_char` (`harness/tests/commands_tests.cpp`) |
| AC-17 | Pass | `ac17_key_replies_when_last_key_up_not_idle`, `ac17_type_replies_when_enter_up`, `harness_key_gives_up_after_an_hour` (`harness/tests/commands_tests.cpp`); `type_and_wait_same_log_as_outside` (`operation_tests.cpp`); `deferred_reply_comes_later_and_not_busy` (`sessions/tests/command_tests.cpp`); in a preset the step has finished only when the machine is idle as well (M-38): `lib5_ac17_key_step_finishes_at_idle_command_at_key_up` (`library/presets/tests/typing_tests.cpp`), `ac28_key_and_type_finish_at_idle` (LIB-6) |
| AC-18 | Pass | `ac18_live_dropped_while_busy_not_pressed_after` (`part_tests.cpp`) |
| AC-19 | Pass | `ac19_held_keys_lists_live_only` (`part_tests.cpp`) |
| AC-38 | Pass | `ac38_key_and_type_not_busy_stopped_by_live` (`part_tests.cpp`) |
| AC-39 | Pass | `ac39_raw_key_down_not_engines` (`part_tests.cpp`) |
| EC-1 | Pass | `ec1_key_up_of_other_key_does_nothing` (`live_tests.cpp`) |
| EC-2 | Pass | `ec2_unmapped_types_nothing` (`map_tests.cpp`) |
| EC-3 | Pass | `ec3_waits_without_limit_until_dropped`, `ec3_first_press_bound_starts_all_the_same` (`queue_tests.cpp`); `harness_key_gives_up_after_an_hour` (D-5) |
| EC-4 | Pass | `ec4_committed_text_each_char_one_press` (`text_tests.cpp`) |
| EC-5 | Pass | `ec5_reset_power_cycle_cold_boot_replace_all_up_nothing_waits` (`reset_tests.cpp`: `dropInput()` before reset, power cycle and cold boot, and a replaced machine) |
| EC-6 | Pass | `ec6_two_holders_one_key_down_until_last` (`live_tests.cpp`) |
| AC-20 (library part) | Pass | `held_key_on_paused_machine_comes_up_after_rest_in_host_time`, `at_once_in_virtual_time`, `waiting_press_waits_for_run` (`pause_tests.cpp`) |

Column status: every criterion of the column that can be tested without a
preset of format 2 passes; AC-15's "found when the preset is read" passes
with LIB-6 L (M-38), as design D-1 plans. The column stays In progress.

**M-38 (2026-11-15, branch `m38-presets`).** The preset parts of AC-14,
AC-15 and AC-17 pass with LIB-6's loader (rows above; the cases in
`library/presets/tests/typing_tests.cpp`, `calcu_library_tests`, 575
`TEST`s, 0 failed; the Frequent group 30 of 30). Every criterion and edge
case of the Library column passes: the Library column is Done.

### Deviations (Library)

- **Polling on the engine's own ticks.** A press that waits for key
  readiness, a TI press waiting to be read and the TI's wait for readiness
  after a release look again `readinessPoll` ticks after the tick the
  engine last acted at, not after wherever a host's slice happened to
  start. Without this the readiness was sampled at different ticks paced
  and in virtual time, and AC-6 failed. Not visible.
- **`MachineSession::nextDue()`** (new, additive): the harness cuts its
  wait for a deferred reply at the next tick a hook is due at, so the
  reply of `key`/`type` comes at the key-up that carries it out, not up to
  1/60 s later (AC-17 "the reply comes when the last key is up").
- **`StartRule::AsQueued`**: the sketch's two-value `StartRule` gets a
  third value for "by the two flags" (PC-1500, TI; design §3.2's comment).
- **Live keys and other presses.** A press that is not live input (typed
  text, a sequence, a command) waits while the user holds any key the
  engine put down, so the engine never makes two keys overlap that the
  user did not hold together (AC-5); a second live key does not wait for a
  key the user holds (AC-5, AC-23).
- **Gaps on a paused machine.** While the machine is paused, a live key
  does not wait for the gap after a release (the gap cannot pass in
  ticks); presses that wait still wait for the machine to run (AC-20).
- **Reasons.** A press dropped by focus loss replies "The focus was
  lost."; by `drop()` or a new machine "The machine started over."; by
  live input "Stopped by a key." (D-9). A character without a key refuses
  `type` with "No key types the character "x"."
- **Fixed with the Calc-U-59 column:** with a hold-until-sampled rule the
  first look at the end of the minimum hold was not scheduled, so every TI
  press was held the full 1 s; it is now due at once. The library has no
  case of its own for the sample rule; the TI cases cover it
  (`lib5_ac20_ti_profile`, `lib5_tr_idle_tap_stays_100ms_adv_as_without_the_rule`).
- **`TypingProfile::names`** (new): a `key` of several names whose names
  act on each other, for AC-26 (a `2nd` written before a second function
  is pressed once).
- **The harness lists `type` for every product** whose typing product
  says it types text (`TypingProduct::typesText()`); on a model without
  characters the session command answers "Unknown command: type." (D-8).
  The `--help` list is one list for all products and shows both commands.

## Calc-U-59 (M-36, 2026-11-15)

Same machine and build. The code: `app/TITyping.*` (`cu59::typingProfile`,
`TIKeyboardMap`, `typingProduct`, the names of `preset.md` §8.1 and
§8.1.1), `app/TIKeystrokePlayback.*` (the KEYSTROKES lane over the engine,
adapter until M-54), `TITypingSupport` (`core/TIAdapters.*`, capability
`TMC0501/typing-support`) and a per-key count in `core/TMC0501.*` of the
scans that found a key pressed (nothing the machine does changes),
`TIFreezeProbe` and `TIKeyTraceObserver`. `KeySequencer` and `KeyboardMap`
are gone, and so are `c59.press-key`, `c59.release-key`,
`c59.keyboard-down/-up/-release` and `heldKey` of `c59.run-state`. The
typing part is in every session (`makeAppSession`: Qt6, Swift, tests; the
Web engine; the harness, in virtual time). The reset, model switch, module
and printer flows call `MachineSession::dropInput()` (EC-5). Tests:
`tests/typing_tests.cpp` (22), the playback cases in
`shared_keys_tests.cpp`, `web_part_tests`, `harness_tests`.

Results: Frequent 28 of 28 (`CalcU59Tests` 282 tests). Longer:
`CalcU59WebPlaywright` 91 passed, 5 skipped; `CalcU59ReferenceRuns`
passed; `CalcU59RomMissingBuild` passed. The Qt6 app builds; it has no
typing test of its own (`CalcU59QtTyping` was not written). The Swift app
could not be built from this worktree (`xcodebuild` stops at planning, "no
XCFramework found", also with `-derivedDataPath build/dd-59` after
`build_xcframework.sh` succeeded): Swift is untested.

**T59-0** (`lib5_tr_sample_rule_measurement`): a tap of R/S on
`Lbl A Prt GTO A` at 0, 75, 150 and 225 ms after a printed line, on the
TI-59, TI-58 and TI-58C, at regular and at full speed:

| Reads needed | R/S stops the loop | Held |
|---|---|---|
| 0 (100 ms alone) | 12 of 24 | 100 ms |
| 1 | 24 of 24 | 100–266 ms |
| 2 | 24 of 24 | up to 596 ms |

Chosen (D-3): one read, at most 1 s (`kTISamplesNeeded`, `TITyping.hpp`).
At the idle loop a tap stays 100 ms. Adv feeds two lines with every rule,
the 100 ms alone included, not one as the design guessed
(`lib5_tr_idle_tap_stays_100ms_adv_as_without_the_rule`).

| ID | Result | Test case (`tests/typing_tests.cpp` unless named) |
|---|---|---|
| AC-20 | Pass | `lib5_ac20_ti_profile`, `lib5_ac20_freeze_inside_hold_key_up_on_frozen_machine` |
| AC-21 | Pass | `lib5_ac21_the_26_bindings`, `lib5_ac21_letter_by_carried_letter`, `lib5_ac21_command_modifiers_map_nothing`; `lib5_ac24_web_keys_through_the_typing_engine` (`web_part_tests.cpp`, an AZERTY layout); Playwright (`play-calculator.spec.js`, AZERTY) |
| AC-22 | Pass (Web; Qt6 built only; Swift untested) | `lib5_ac22_host_key_reaches_the_machine_through_the_session`; Playwright host-key cases. iOS sends no host keys (unchanged) |
| AC-23 | Pass | `lib5_ac23_second_key_does_not_release_first`; Playwright two-key case |
| AC-24 | Pass (Web, C++; Qt6 built only; Swift untested) | `lib5_ac24_screen_key_through_the_engine_minimum_hold`, `lib5_ac24_web_keys_through_the_typing_engine`; Playwright short-click case |
| AC-25 | Pass | `lib5_ac25_rs_stops_running_program`, `lib5_ac25_rs_stops_printing_loop_every_phase` (3 models × 2 speeds × 4 phases), `lib5_ac25_gto_acted_on` |
| AC-26 | Pass (harness; in a preset: M-54) | `lib5_ac26_names_of_preset_md_8_1`, `lib5_ac26_2nd_before_second_function_once`, `lib5_ac26_old_names_refused`; `lib5_harness_key_on_a_ti59` (`harness_tests.cpp`) |
| AC-27 | Pass | `lib5_ac27_sequence_key_waits_readiness_held_min`, `lib5_ac27_key_up_deferred`, `lib5_ac27_shift_binding_two_presses_new_key_drops_second`, `lib5_ac27_map_rules` |
| EC-5 (TI) | Pass | `lib5_ec5_reset_and_model_switch_release_held_keys` |
| AC-16, D-8 (no `type` on a TI) | Pass | `lib5_ac26_old_names_refused`, `lib5_harness_key_on_a_ti59` |

Column status: AC-22 and AC-24 are not shown in the Swift frontend, and
AC-17/AC-26 in a preset pass with LIB-6 59 (M-54, D-1). The column stays
In progress.

**Reference runs, golden runs (R-1).** No reference or golden file was
changed. `CalcU59ReferenceRuns` names the differences it accepts, each
with its reason, and passes: 36 accepted, 0 not accepted.
- Every example: the driver's wall time on the "recorded after N s" line.
- `debug/printer-timing.ti59`: the `Elapsed:` lines (row 59-23). Its paper
  is now 104 lines, as the reference: the finding of M-34 (106 lines, two
  more feeds) is fixed by holding a preset's key 100 ms of emulated time
  and until read.
- The `Elapsed:` lines of `calendar-01` to `-09`, `selftest-speed-check`,
  `globe-print` and `picard`: time lines since M-33 (LIB-17 D-3), and row
  59-23.
- `historic/life.ti59`: register 114 is 2, not 5. It is the same before
  and after M-36 (a baseline of `origin/main` was recorded to tell), so
  M-36 does not change it; it has differed since M-33 (the 30-minute cut
  counts driver time).
- Finding, the same on `main`: `screentest_test_58_glyphs.ti58` and
  `screentest_test_cdebug.ti59` never end: the preset ends frozen
  (FreezeOnStart fires) and the session steps an operation only while the
  machine runs. For LIB-10 59 (M-53).

Exit check: `lib5_exit_printer_timing_same_paper_at_regular_and_full_speed`
runs `printer-timing.ti59` through the session at regular speed, at full
speed and as written: the same 104-line paper each time. The TI golden
runs are equal (`c59_1_ac4_golden_runs_through_the_machine_interface`).

### Deviations (Calc-U-59)

- The Web page's pointer and key-grid buttons keep their `c59.web.*`
  commands; `TIWebPart` hands them to the engine as on-screen keys. Host
  keys and focus loss are the library's commands.
- On the Mac the Swift frontend sends the key's base character as the
  physical identity (SwiftUI gives no key code); Qt6 sends the native scan
  code, or Qt's key code when there is none.
- The TI map also maps nothing with the Windows or Super key held, as
  2.3.0's Qt6 app did.
- Loading a state does not call `dropInput()`: doing so stopped a running
  preset whenever a second load was refused (`lib3_ac32`). Reset, model
  switch, module and printer changes drop input.
- Two keys on the same row of the matrix: the ROM rejects the pair and
  does not read the later key, so it is held until the 1 s maximum
  (`lib5_ac23` shows it).
- `session_plays_keystrokes_in_real_time` runs on `SessionRig` (a bare
  `EmulatorSession` has no typing engine now).

## Calc-U-1600 (M-36, 2026-11-15)

Same machine and build. The code: `app/SharpTyping.*`
(`shared::typingProfile` per model, `SharpKeyboardMap`, `typingProduct`),
typing support on both Sharp machines (prompt readiness; on the PC-1600
the latches `shift` and `kbii`), in the `Kinds` of
`PC1500InterfaceMachine.cpp` and `PC1600InterfaceMachine.cpp`.
`SharpTypingHook`, `HostKeyMap`, `KeyPasteFeeder`/`buildPasteSteps`/
`PastePacing`, `LegacyCalls::readLatches`, the kind `c1600.key-queue` and
the PC-1500's core key queue are gone (`KeyLatches` and `kbiiSequence`
stay for the preset typers until M-44/M-46). `SharpAppPart` owns a typing
part (paced) and puts it first; the harness product has its own in virtual
time. `discardMachine`, `resetToPromptJob` and `powerCycleAroundJob` call
`dropInput()`. Qt6 and the Web page send `host-key-down/-up`,
`screen-key-down/-up`, `paste`, `host-text` (Qt6's input-method commit),
`focus-lost`; the SHIFT tap stays in each frontend and is one screen press
of `shift`; the debugger's `typeCommand` is the engine's `typeText` with
ENTER; Qt6 waits with the `typing` request. Tests: `tests/typing_tests.cpp`
(15), the ported `key_paste_tests.cpp`, `shared_host_key_map_tests.cpp`,
`shared_machine_session_tests.cpp`, Web `typing.spec.js`.

Results: Frequent 28 of 28 (`CoreTests` 875 tests). Longer
(`-DCALCU_FRONTEND_QT=ON`): `CalcU1600WebPlaywright` 55 passed, 2 skipped
(3 new in `typing.spec.js`); `CalcU1600Shots.01`–`09` passed;
`CalcU1600DapSmoke` passed; `CalcU1600WasmSmoke` passed (run by hand on its
own Web build); `CalcU1600RomMissingBuild` not run.

| ID | Result | Test case (`tests/typing_tests.cpp` unless named) |
|---|---|---|
| AC-14 | Pass (commands; in a preset: M-46) | `lib5_ac14_type_starts_at_readiness_pc1600_bound_15s`, `lib5_ac14_type_starts_at_once_pc1500` |
| AC-28 | Pass (commands; in a preset: M-46) | `lib5_ac28_cadence_4_4_and_6_after_shift_pc1600` (the profiles of the three models; shift, gap, key measured on the PC-1600) |
| AC-29 | Pass | `lib5_ac29_named_keys_and_shift_forms_per_model`, `lib5_ac29_command_modifiers_nothing_altgr_types`, `test_characters`; Web "host keys type through the product keyboard map" |
| AC-30 | Pass | `lib5_ac30_characters_per_model`, `lib5_ac30_shift_and_skip`, `lib5_ac30_kbii_case_exact`, `test_pc1500_lacks_digit_row_legends` |
| AC-31 | Pass | `lib5_ac31_cursor_key_held_rom_repeats_pc1500` (the exit check: Left held 2 s on the PC-1500A and the PC-1600 moves the cursor to the start of the line), `lib5_ac31_every_key_held_pc1500a`, `lib5_ac31_waiting_press_at_key_readiness_pc1500` |
| AC-32 | Withdrawn | — |
| AC-33 | Pass (C++, Web, Qt6) | `lib5_ac33_shift_tap_is_one_shift_press`; Web "a SHIFT tap is one SHIFT press"; Qt6 `CalcU1600Shots.lib5-ac33` (`tests/gui/lib5-ac33.shots.yaml`, 2026-11-16, below) |
| AC-34 | Pass | `lib5_ac34_paste_releases_latched_shift_skips_under_kbii`, `lib5_ac34_second_paste_appended`, `lib5_ac34_kbii_respects_latches`, `lib5_ac34_pc1600_paste_line_no_enter` |
| AC-35 | Pass | `lib5_ac35_prompt_readiness_and_latches` |
| AC-36 | Pass | `lib5_ac36_keys_dropped_during_preset_operation`, `ledger_1600_9_keys_dropped_and_reset_stops_an_operation`; Web "keys during a load are dropped" |
| AC-37 | Pass | `test_timed_tap_pc1600` (an instant press and release is one key), `test_minimum_key_hold_pc1500a`, `_pc1600` (a double tap is two; a fast sequence in order), `test_host_keys` and the map cases, the `key_paste_tests.cpp` cases, `lib5_ac12_pc1600_paste_multiline_types_first_line`, `lib5_ac12_pc1500_paste_multiline_types_first_line`, `test_switch_and_paste_*` |
| AC-40 | Pass | `lib5_ac40_on_breaks_running_program` (PC-1500A and PC-1600 show BREAK) |
| AC-41 | Pass (commands; in a preset: M-46) | `lib5_ac41_break_alias_ent_refused` |
| `key`, `type` on a Sharp machine | Pass | `lib5_ac17_harness_key_and_type_on_a_sharp_machine` (`PRINT 6*7` shows 42; `key ent` and an unknown character refused) |

Column status: AC-33 is not shown in the Qt6 frontend, and the preset
parts of AC-14, AC-28 and AC-41 pass with LIB-6 1600 (M-46, D-1). The
column stays In progress.

**Golden runs.** Equal. The driver's `keys` on the PC-1500 taps through
the engine with the PC-1500 profile instead of `c1600.key-queue`. To give
0.8.1's queue exactly, the driver advances one instruction before it
presses and switches the profile's readiness wait off.
**Finding:** with the full AC-31 profile (presses that wait start at key
readiness), `pc1500a-ce150` differs after `type lprint 123`: two cycle
counts (11310032 → 11310016, 11570035 → 11570025) and one count of audio
samples (9600 → 9601, all zero); the display, the paper and the memory
checksum are the same. A press waited for key readiness there. The core is
unchanged: with the old cadence the run is identical. The golden file was
not re-recorded.

**Sharp reference runs.** The M-4 references are CLI runs (no
`lib3_reference_examples_through_session` case exists); the M-4 procedure
re-run with this tree's `pc1500_cli` and `pc1600_cli`: 32 of 32 equal in
`.out` and `.log`, the `syncclock` wall time aside (masked as in C1600-1's
qa-report).

### Deviations (Calc-U-1600)

- Typing support is in the two InterfaceMachines' `Kinds`, not in a new
  `core/Machine/SharpTypingSupport.*`.
- Frames are `n × (rate / 60)`, as the cores and the preset typers count
  them (`kPC1500CyclesPerFrame`), not the library's `frames()`
  (`n × rate / 60`): the 4 + 4 frames stay exactly today's.
- The KBII sequence of a live host key is built as with every latch off
  (the map has no machine); paste and typed text read the latches when
  each character is due (E13).
- The Qt6 key handler sends every key but a bare modifier to the session
  and passes a key with a command modifier on to the window as well (before
  it sent only keys it could resolve).
- The debugger's reset cancels typing with the engine's `focusLost()`.
- `MainWindow::releaseHeldKeys()` checks that the controller still exists
  (a crash at window teardown found by the Shots run).

## Ledger and status

Rows 59-13, 59-23, 59-40, 59-41, 1600-8, 1600-9 and 1600-29 of
`docs/behaviour-changes.md` are in effect from M-36. CI was not
dispatched (plan §1). All three INDEX columns stay In progress (D-1).

### Calc-U-1600: the preset clauses (M-46, 2026-11-16, branch `m46-presets-1600`)

| ID | Result | Test case |
|---|---|---|
| AC-14 (in a preset) | Pass | `lib5_ac14_in_a_preset_type_at_readiness_pc1600_at_once_pc1500` |
| AC-28 (in a preset) | Pass | `lib5_ac28_in_a_preset_cadence` |
| AC-41 (in a preset) | Pass | `lib5_ac41_ent_refused_in_a_preset`; `ctrl`, `kbii` and `bs` are no key names on the PC-1600 (`lib5_ac41_break_alias_ent_refused`, LIB-6 AC-44) |

The preset clauses pass. AC-33's Qt6 test (`CalcU1600QtTyping`) is still
not written, so the column stays **In progress**.

### Calc-U-1600: AC-33 in the Qt6 frontend (2026-11-16, branch `lib5-qt-typing-1600`)

The design's `CalcU1600QtTyping` (T16-4) is the check scenario
`tests/gui/lib5-ac33.shots.yaml`, CTest `CalcU1600Shots.lib5-ac33`
(Longer, Qt trees only, offscreen; ADR 0032), like the other GUI criteria.
The scenario runner got the steps `host-key-down`/`host-key-up` (`shift`
or one character: the key events the window gets from a host keyboard) and
`expect-lcd`/`expect-no-lcd` (text in one row of the LCD).

| ID | Result | Test case |
|---|---|---|
| AC-33 | Pass | `lib5-ac33-tap` (PC-1600: SHIFT down and up, then 1, 1 shows `'1`: one SHIFT press, for the next key only); `lib5-ac33-held` (SHIFT held 600 ms: `11`, no `'`); `lib5-ac33-modifier` (SHIFT with A in between: `A1`, no `'`) |

With the tap's press removed from `MainWindow::keyReleaseEvent`, the tap
shot fails (`11`); no product change was needed. Results: `build/qt`
(`-DCALCU_FRONTEND_QT=ON`) Frequent 36 of 36; `CalcU1600Shots.lib5-ac33`,
`.lib8-ac36`, `.lib10-ac41` passed; `build/tests` (Qt OFF) Frequent 34 of 34.

**Deviation:** the test is a check scenario named
`CalcU1600Shots.lib5-ac33`, not an entry `CalcU1600QtTyping`.

Every Calc-U-1600 criterion of LIB-5 passes: the column is **Done**.

### Calc-U-59: the preset clauses (M-54, 2026-11-16, branch `m54-presets-59`)

The KEYSTROKES lane is gone (`app/TIKeystrokePlayback.*`,
`EmulatorSession`'s playback, `Hooks::act`); a preset's `key:` steps go
through the engine (LIB-6's `KeyItem`).

| ID | Result | Test case |
|---|---|---|
| AC-17 (in a preset) | Pass | `lib5_ac17_in_a_preset_key_finishes_at_idle` |
| AC-26 (in a preset) | Pass | `lib5_ac26_names_of_8_1_in_a_preset` (every key ID, other spelling and printer name in a `key:` step), `lib5_ac26_2nd_before_second_function_once_in_a_preset`, `lib5_ac26_old_names_refused_when_read` |

`SliceRequest::due` (D-16): the machine session still hands it to every
`SliceHook::act`, a library interface, so it stays as the pacer's own
field; §5's row is closed as no adapter. The finding of LIB-6's Calc-U-59
section: a `key:` step finishes when its last key is up
(`TypeOptions::untilReleased`), so a key whose program never returns no
longer holds the preset (only the TI profile waits for readiness after a
release). Column status: **In progress** until AC-22 and AC-24 are shown
in Swift (M-58, D-15).

### M-58 (2026-11-17, branch `m58-swift-59`; Swift)

| ID | Result | Test case |
|---|---|---|
| AC-22 | Pass (Swift on macOS; none on iOS, by code) | `ShellBridgeTests.test_lib5_ac22_host_key_reaches_the_machine` (macOS unit tests: the host key 1 through the bridge's `host-key-down` holds the key 1 in `held-keys`, its key-up lets it go); on iOS `physicalKeyboard` adds nothing (unchanged) |
| AC-24 | Pass (Swift) | `ShellBridgeTests.test_lib5_ac24_screen_key_minimum_hold` (a press of the shell's pointer rule let go at once still reaches the machine: `screen-key-down`/`-up` through the typing engine, the display shows 1); XCUITest `test_lib14_ac24_a_key_under_the_pointer` (a short tap on 1 on the faceplate shows 1) |

Every criterion of the column passes in every frontend: **Done**.
