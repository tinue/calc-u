# LIB-3: QA report

Results per acceptance criterion and edge case of `spec.md`, per column.

## Library (M-22, 2026-11-14)

Built and run on an M4 Pro, macOS 27.2, Apple clang, CMake, Ninja. The
code is `library/sessions/` (namespace `calcu::sessions`, in
`calcu_library`); the tests are `library/sessions/tests/`, compiled into
`calcu_library_tests` (CTest entry of the same name, label `frequent`).
They use LIB-1's test machines `test-seg` (100,000 ticks per second, key
readiness after 50 ticks, no sound) and `test-dot` (1,000,000 ticks per
second, a square-wave sound), a test product part with the prefix `tp.`
(`TestParts.hpp`), the test frontend (`TestFrontend.hpp`: a `TopicMirror`,
the raw message log, delivery at once or through a queue) and a scripted
wall clock (`ScriptedClock.hpp`: it moves only when the test moves it, plus
1 ms at every reading). No test reads the host's clock.

Result of the run: `calcu_library_tests` 176 tests (63 of them LIB-3's),
1,622 checks passed, 0 failed, 0.51 s in a RelWithDebInfo tree (about
1.5 s without a build type). The same in the library-only tree (`-DCALCU_PRODUCT_59=OFF -DCALCU_PRODUCT_1600=OFF
-DCALCU_FRONTEND_QT=OFF`). The whole Frequent group passed in the
RelWithDebInfo tree: Calc-U-59 23 s, Calc-U-1600 39 s. The sources of `library/sessions/` also compile
with Emscripten (`em++ -std=c++20 -fwasm-exceptions -Wall -Wextra`), and
the branch passed both product workflows (GCC 13, MSVC, Emscripten, Apple
clang).

| ID | Result | Test case (file in `library/sessions/tests/`) |
|---|---|---|
| AC-1 | Pass | `ac1_app_session_holds_one_machine_session` (one machine session; its input queue: a `SliceHook` acts at its tick), `ac1_machine_replaced_inside_session` (test-seg → test-dot → none; `machineChanged` per change) (`host_call_tests.cpp`) |
| AC-2 | Pass | `ac2_call_runs_machine_and_sends_changes` (60 calls a frame apart: about one second of emulated time, the display sent), `ac2_without_operation_runs_at_last_commanded_speed` (`host_call_tests.cpp`) |
| AC-3 | Pass | `ac3_no_call_no_run` (`host_call_tests.cpp`) |
| AC-4 | Pass | `ac4_at_once_and_queued_give_same_messages_in_same_order` (one script of commands, requests, an operation and host calls, with sound; two sessions; the message logs compared as text) (`host_call_tests.cpp`) |
| AC-5 | Pass | `ac5_state_sent_only_when_it_differs`, `ac5_command_reply_and_changed_state_at_once`, `ac5_machine_output_only_with_host_calls` (`topic_tests.cpp`) |
| AC-6 | Pass | `ac6_new_listener_gets_every_state_topic_once`, `ac6_late_listener_gets_log_since_reset_and_no_notices` (`topic_tests.cpp`) |
| AC-7 | Pass | `ac7_stream_sends_exactly_what_was_appended`, `ac7_stream_reset_marker` (`topic_tests.cpp`) |
| AC-8 | Pass | `ac8_request_gets_exactly_one_reply`, `ac8_request_without_answer_says_why` (`command_tests.cpp`) |
| AC-9 | Pass | `ac9_display_busy_notices_log_and_their_shapes` (`library_topics_tests.cpp`) |
| AC-10 | Pass | `ac10_notice_is_the_sentence_as_written` (`library_topics_tests.cpp`) |
| AC-11 | Pass | `ac11_log_lines_from_library_and_product_part` (a part's line and an operation's lines) (`library_topics_tests.cpp`) |
| AC-12 | Pass | `ac12_product_topic_command_request_travel_like_generic` (the payload the part set arrives as the same object, unread), `ac12_unknown_topic_ignored_by_mirror` (`command_tests.cpp`) |
| AC-13 | Pass | `ac13_command_done_or_refused`, `ac13_unknown_command_refused_changes_nothing` (`command_tests.cpp`) |
| AC-14 | Pass | `ac14_busy_while_operation_runs`, `ac14_one_part_per_call_within_allowance` (every call within 14 ms of the scripted clock plus the readings of one slice), `ac14_full_speed_unless_regular_stretch`, `ac14_busy_says_speed_now` (`operation_tests.cpp`) |
| AC-15 | Pass | `ac15_changes_sent_at_end_of_call_during_operation` (`operation_tests.cpp`) |
| AC-16 | Pass | `ac16_second_operation_refused`, `ac16_product_command_operation_refused_unread` (a second product part `tq.`), `ac16_other_commands_go_through`, `ac16_ends_running_operation_command` (`operation_tests.cpp`) |
| AC-17 | Pass | `ac17_nested_operation_not_refused` (a cold boot awaited inside the operation) (`operation_tests.cpp`) |
| AC-18 | Pass | `ac18_end_busy_reply_notice_start_afresh` (`operation_tests.cpp`) |
| AC-19 | Pass | `ac19_one_go_equals_parts` (test-dot; `Hosting::VirtualTime` against host calls on the scripted clock: same reply, tick count, key events and display, then the same answers to key-down, advance, key-up and `ticks`) (`operation_tests.cpp`) |
| AC-20 | Pass | `ac20_sink_told_rate_and_gets_regular_speed_sound` (start with the rate, 40 ms primed, every sample made), `ac20_full_speed_and_operation_sound_thrown_away`, `ac20_no_sink_thrown_away`, `ac20_recorder_gets_all` (`sound_tests.cpp`) |
| AC-21 | Pass | `ac21_speed_from_next_call`, `ac21_gesture_changes_nothing_while_operation_sets_speed` (`speed_tests.cpp`) |
| AC-22 | Pass | `ac22_waits_for_input_from_key_readiness`, `ac22_waits_for_input_from_wait_probe`, `ac22_operation_reports_not_waiting` (a profile paced while waiting: the operation that reports "not waiting" runs more than three times as many ticks per call) (`speed_tests.cpp`) |
| AC-23 | Pass | `ac23_start_afresh_on_create_continue_end_new_machine_resume` (`MachineSession::freshStarts()` counts the hand-offs; the absence of lag observed after a debugger pause) (`speed_tests.cpp`) |
| AC-24 | Pass | `ac24_suspended_no_run_continue_runs`, `ac24_session_never_suspends_itself` (`suspend_tests.cpp`) |
| AC-25 | Pass | `ac25_pause_survives_suspend_and_back`, `ac25_resume_while_suspended_does_not_run` (`suspend_tests.cpp`) |
| AC-26 | Pass | `ac26_listener_states_services` (`host_service_tests.cpp`) |
| AC-27 | Pass | `ac27_one_ask_one_answer_session_runs_meanwhile`, `ac27_not_busy_while_asking`, `ac27_service_not_offered_not_asked` (`host_service_tests.cpp`) |
| AC-39 | Pass | `ac39_ticks_request_gives_ticks_and_seconds`, `ac39_capabilities_request_gives_kind_id_owner_places_plugged` (`command_tests.cpp`) |
| EC-1 | Pass | `ec1_several_changes_send_the_last` (`topic_tests.cpp`) |
| EC-2 | Pass | `ec2_change_and_back_sends_nothing` (`topic_tests.cpp`) |
| EC-3 | Pass | `ec3_failure_halfway_then_commands_accepted` (a thrown exception, then an operation without a machine) (`operation_tests.cpp`) |
| EC-4 | Pass | `ec4_operation_no_progress_while_suspended_busy_stays` (`suspend_tests.cpp`) |
| EC-5 | Pass | `ec5_session_states_machine_rate_whatever_device` (8,000, 11,025 and 48,000 samples per second) (`sound_tests.cpp`) |
| EC-6 | Pass | `ec6_request_answered_while_busy` (`operation_tests.cpp`) |

Support cases: `value_round_trip_every_alternative` (`value_tests.cpp`),
`topics_request_lists_declared_topics` (`command_tests.cpp`),
`mirror_keeps_last_state_and_appended` (`mirror_tests.cpp`).

The tests were checked against three broken sessions (each change made,
the tests run, the change undone): a state topic sent without comparing
(AC-5 and EC-2 fail); the sound of full speed and of operations played
(AC-20 fails); a second operation not refused (the test program fails).

Technical Requirements (review): every class of `library/sessions/` is
single-threaded; it holds no lock, starts no thread and lets no exception
out of `AppSession` (a handler's or an operation's exception becomes a
refused reply or a failed operation). Everything a frontend gets is a
`Message`; there is no getter for a frontend besides `context()`, which
the design keeps for the host's own adapters. No generic payload names a
model-specific type. `library/sessions/` includes only `machine`,
`pacing` and the standard library.

### Deviations from the design

- **`OperationContext::run` returns `Task<Ticks>`**, not a
  `detail::RunAwaiter`: it is a small coroutine that issues the run
  requests of at most `kRunPiece` (1/60 s, `OperationContext::runPiece`)
  itself. An operation writes `co_await ctx.run(n)` either way. The
  generic awaitable of one machine call is `request(std::function<…>)`,
  as `job::run` is today; `JobForward` (M-31) can define `job::run` on it.
  `Job<T>` and `Task<T>` are copied from `core/Job.hpp` unchanged in
  behaviour; the product file is not changed in this step.
- **An operation starts inside the command that starts it**: its code
  runs up to its first run request there (an operation that never runs
  the machine ends and is replied to at once). Its parts then run in the
  host calls. The slices of the pacer come from the operation through a
  small interface `SliceSource` that the machine session calls in
  `runSlice` while an operation runs (the design says only "from the
  operation's requests"). A machine that changes inside an operation's
  slice ends the call.
- **`SessionContext` and `SessionSetup` are abstract interfaces**
  implemented by the app session; the design's sketches show plain
  classes. `SessionContext` also has `hostOffers(HostService)` (AC-26) and
  `hostTime()` (A-7).
- **A-7, the real-time clocks:** `AppSessionSetup::hostTime` is held and
  given to the parts through `SessionContext::hostTime()`; the session
  does not set any clock yet, because the real-time clock kind is LIB-2's
  (M-23).
- **An unknown request** is refused with "Unknown request: <name>."; D-7
  names only the command's sentence. Malformed arguments of the library's
  commands are refused with "The speed command needs full: true or
  false." and "The key command needs key: <name>."; an unknown key with
  "Unknown key: <name>.".
- **`TopicMirror::takeReset(topic)`** is added: a frontend part of the
  paper (LIB-8) needs to know that a stream started over.
- **`MachineSession` has a few members the sketch lacks:**
  `generation()` (the app session notices a new machine by it and then
  reads the display, closes the sound and calls `machineChanged`),
  `advance(atLeast, stoppedEarly)` (the one advance with hooks and gate),
  `waitsForInput()`, `setReportsWaiting()`, `soundRate()` and
  `freshStarts()` (a counter of the hand-offs to the pacer, for AC-23).
- **The audio sink is the first listener that offers `AudioSink`** and does
  not record; a listener with `recordsSound` gets `SoundStart` once per
  machine, at its first samples, and every sample after it (also from
  `advanceUnpaced`). The sink's sound of `advanceUnpaced` is thrown away
  (not real time). On a new machine an open sink gets `SoundStop`.
- **`listen()` flushes first**, so that what is pending goes to the
  listeners there are and the new one gets each state topic once. A
  `SinceReset` stream is replayed to it even when it is empty (one
  message with the reset marker).
- **`ScriptedClock`** is the sessions tests' own: LIB-17's moves only when
  the test or its slice runner moves it, and the session tests have no
  slice runner that moves the clock (the pacer's full speed runs until its
  allowance is used up on the clock). It steps 1 ms at every reading.
- **Test machines:** unchanged; the tests use `test-dot` for sound and
  `test-seg`'s key readiness as the design says.

Not done in M-22 (other columns): AC-28 to AC-33 (Calc-U-59, M-33, M-34)
and AC-34 to AC-38 (Calc-U-1600, M-31).

## Calc-U-59 (M-33, 2026-11-15, branch `m33-sessions-59`)

Built and run on an M4 Pro, macOS 27.2, RelWithDebInfo `build/tests`; the
Qt6 app (`build/qt`, Homebrew Qt 6) and the Swift app (Xcode, Debug) built.
The code is `products/calc-u-59/app/TIAppPart.*` (`cu59::TIAppPart`,
`makeAppSession`, the interim topics and commands), `TIFileAccess.*`,
`PacingProfiles.hpp`; the tests are `products/calc-u-59/tests/session_tests.cpp`
(with `AppSessionRig.hpp`: a test frontend with a topic mirror, a test
clock that moves 1 ms at every reading, a temporary save folder). The
whole Frequent group passes (26 of 26).

| ID | Result | How it was checked |
|---|---|---|
| AC-28 | Pass | Qt6: `EmulatorController` hosts `makeAppSession()` on the GUI thread (a 60 Hz timer), sends `c59.*` commands and reads a `TopicMirror`; run offscreen with a temporary `$HOME` and a preset `1 + 1 =`: the window shows 2 (`--grab`). `CalcU59NoOwnSession` (P59-1): no `EmulatorSession`, `EmulationRunner`, `TISessionController` or own `SessionController` in the Qt6 and Swift sources; no `std::thread` in `app/` but `SessionThread` |
| AC-29 | Pass | `lib3_ac29_mirror_holds_every_change_flag` (each of the fourteen flags' actions moves its interim topic) |
| AC-30 | Pass | `lib3_ac30_error_is_notice_diagnostic_is_log`, `lib3_ac30_card_and_constant_memory_files_without_frontend`, `lib3_ac30_module_id_not_saved`, `lib3_ac30_show_debug_panel_is_product_message` |
| AC-31 | M-34 (Qt6 part checked) | `CalcU59NoOwnSession` checks that Qt6 sends no suspend or continue; Swift: C59-5 AC-8; Web in M-34 |
| AC-32 | Pass | `lib3_ac32_preset_load_is_busy_and_refuses_second`, `lib3_ac32_printer_toggle_is_operation`, `lib3_ac32_updates_during_preset` |
| AC-33 | Pass | `lib3_ac33_no_sound_messages_and_no_sink_stated` (a frontend offering an audio sink gets no sound and no ask) |

### Deviations from the design

- **`TIAppPart` hosts `SessionController` instead of taking its flows
  apart** (design §2, §4). The controller runs in `Threading::Hosted`: the
  machine is released to the machine session (`EmulatorSession::releaseMachine`),
  which paces it; `CooperativePacing::advanceHosted` is `TIFreezeGate`, the
  KeySequencer runs at each slice end (`TIKeySequencerHook`), the card wait
  is a `WaitProbe` (`TICardWaitAdapter`), the controller's freeze stops the
  machine session through `setPaused`. Its `Platform` is the part: errors
  are notices ("title: message", split again by the frontends for today's
  alert), diagnostics log lines, the files `TIFileAccess`, the UI-thread
  queue a queue that runs on the session's thread. The change flags become
  snapshot payloads of the interim topics (equal when they are the same set,
  so EC-2 does not hold for them). So the behaviour is 2.3.0's by
  construction; M-34 takes the controller apart when the Web moves.
- **Interim topics:** `c59.model`, `c59.led` (the frame the frontends draw;
  the library's `display` topic carries the machine's output too),
  `c59.cue-card`, `c59.printer` (a state snapshot, not yet LIB-8's stream),
  `c59.card-reader`, `c59.run-state`, `c59.trace`, `c59.debug.live`,
  `c59.debug.cpu`, `c59.debug.heatmap`, `c59.debug.log`, `c59.debug.asm`,
  `c59.show-debug-panel` (stream); request `c59.drop-action`.
- **`SessionContext::commandedSpeed()` / `setCommandedSpeed()`** are new in
  the library: the part keeps the session's speed and the controller's full
  speed (KEYSTROKES `FullSpeed:`, the display press) in step. The frontends
  send `c59.full-speed`, which an operation follows too (a preset's playback
  at the speed the user or the preset sets, as today).
- **The preset operation** runs until the KEYSTROKES script has played
  (`KeySequencer::scriptPending`); the power-cycle operation until the ROM is
  ready for keys (at most 10 s of emulated time); both at the speed of the
  controller's full-speed flag, so a preset still runs at regular speed
  unless it asks (ledger 59-5 is LIB-6 59's).
- **`Threading::Threads` is gone, the file `Threading.hpp` stays** with
  `Cooperative` (Web, the controller's tests) and `Hosted`. `MachineGuard`
  and the session's mutexes stay until M-34 (never contended now).
