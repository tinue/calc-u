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

Result of the run: `calcu_library_tests` 423 tests, 3,360 checks passed, 0
failed. The whole Frequent group passed (see the end of this report).

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
| AC-14 | Pass (engine part) | `ac14_first_press_rule_inside_an_operation`, `ac14_first_press_at_once_overrides_the_rule` (`operation_tests.cpp`), `ec3_first_press_bound_starts_all_the_same` (`queue_tests.cpp`); the Sharp models in the Calc-U-1600 column; in presets with LIB-6 (M-38, M-46) |
| AC-15 | Pass (commands; the preset read: M-38) | `ac15_key_names_refused_nothing_typed_screen_key_unknown` (`part_tests.cpp`), `ac15_key_names_and_refusal` (`harness/tests/commands_tests.cpp`). "In a preset this is found when the preset is read" is LIB-6 L's loader (design D-1) |
| AC-16 | Pass | `ac16_type_text_and_enter_refuses_unknown_nothing_typed` (`text_tests.cpp`), `ac16_type_absent_without_char_profile` (`part_tests.cpp`, D-8), `ac16_type_presses_enter_refuses_unknown_char` (`harness/tests/commands_tests.cpp`) |
| AC-17 | Pass (harness; in a preset: LIB-6) | `ac17_key_replies_when_last_key_up_not_idle`, `ac17_type_replies_when_enter_up`, `harness_key_gives_up_after_an_hour` (`harness/tests/commands_tests.cpp`); `type_and_wait_same_log_as_outside` (`operation_tests.cpp`); `deferred_reply_comes_later_and_not_busy` (`sessions/tests/command_tests.cpp`) |
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
- **The harness lists `type` for every product** whose typing product
  says it types text (`TypingProduct::typesText()`); on a model without
  characters the session command answers "Unknown command: type." (D-8).
  The `--help` list is one list for all products and shows both commands.
