# LIB-17: QA report

Results per acceptance criterion and edge case of `spec.md`, per column.

## Library (M-21, 2026-11-14)

Built and run on an M4 Pro, macOS 27.2, Apple clang, CMake, Ninja, with
other builds running at the same time. The code is `library/pacing/`
(namespace `calcu::pacing`, in `calcu_library`); the tests are
`library/pacing/tests/`, compiled into `calcu_library_tests` (CTest entry
of the same name, label `frequent`). They drive LIB-1's test machine
`test-seg` (100,000 ticks per second; a model switch to `test-dot` at
1,000,000) through `TestSliceRunner`, a stand-in for the machine session,
on a wall clock supplied by the test (`ScriptedClock`). No test reads the
host's clock, so no result depends on the speed of the host. A fixed slice
in the tests is 20 ms (2,000 ticks).

Result of the run: `calcu_library_tests` 66 tests (26 of them LIB-17's),
665 checks passed, 0 failed, about 1.4 s, of which about 1.1 s are the two
five-hour scripted long runs. The same in the library-only tree
(`-DCALCU_PRODUCT_59=OFF -DCALCU_PRODUCT_1600=OFF -DCALCU_FRONTEND_QT=OFF`).

The bound of AC-1 (design D-1), asserted at the end of every call from the
start of the stretch: a fixed slice, one slice plus the last instruction
(with jitter the deviation lies in (−S/2, S + instruction]); the call, the
time between two calls plus the last instruction (right after a call, 0 to
less than one instruction).

| ID | Result | Test case (file in `library/pacing/tests/`) |
|---|---|---|
| AC-1 | Pass | `ac1_call_slice_keeps_wall_time_at_60_hz`, `ac1_fixed_slice_keeps_wall_time` (without and with jitter) (`pacer_tests.cpp`; 10 s at 60 Hz) |
| AC-2 | Pass | `ac2_uneven_calls_do_not_add_up` (`pacer_tests.cpp`; 100 s, ± 4 ms timer jitter, a 30 ms oversleep every 50 calls, both kinds); `ac2_ac3_library_longrun_scripted_fixed`, `_call` (`pacing_longrun_tests.cpp`; 5 h, ± 4 ms, a 50 ms oversleep every 9,000 calls, a 3 s stall every 250,000 calls; only the stalls drop the lag) |
| AC-3 | Pass | `ac3_overshoot_does_not_add_up` (`pacer_tests.cpp`; instructions of 7 and 37 ticks, overshoots of more than 5 slices in sum, both kinds); the long runs above with instructions of 97 ticks |
| AC-4 | Pass | `ac4_lag_over_limit_dropped_whole` (`pacer_tests.cpp`; 90 ms late is caught up, 500 ms late is dropped and the call runs one slice at most, then AC-1 from the moment of that call) |
| AC-5 | Pass | `ac5_full_speed_stops_at_14_ms_of_the_supplied_clock` (`pacer_tests.cpp`; 1 ms of host time per slice gives 14 slices; a 40 ms slice still runs once; the call profile's full-speed slice is 1/60 s, D-2) |
| AC-6 | Pass | `ac6_back_to_regular_neither_waits_nor_lags` (`pacer_tests.cpp`) |
| AC-7 | Pass | `ac7_full_speed_keeps_tick_count` (`pacer_tests.cpp`) |
| AC-8 | Pass | `ac8_profile_slice_fixed_or_the_call_jitter_and_paced_flag` (`pacer_tests.cpp`) |
| AC-9 | Pass | `ac9_jitter_offsets_within_half_a_slice_and_do_not_add_up` (`jitter_tests.cpp`; 20,000 draws, mean within 500 ± 15 of 0 to 1,000; 1,000 slice ends each 0 to S/2 after its grid point) |
| AC-10 | Pass | `ac10_jitter_same_seed_same_sequence` (the draw checked against `minstd_rand`'s defining formula, so one sequence on every platform), `ac10_jitter_unseeded_differs` (`jitter_tests.cpp`) |
| AC-11 | Pass | `ac11_jittered_slice_ends_hit_more_than_one_loop_position` (`jitter_tests.cpp`; a loop of 40 ticks in a slice of 2,000: one position without jitter, more than one with it, over 50 slice ends) |
| AC-12 | Pass | `ac12_waiting_at_full_speed_waits_one_slice_after_last_unpaced` (exactly 20 ms without jitter, 20 to 30 ms with it; then one paced slice per 20 ms), `ac12_not_waiting_returns_to_full_speed`, `ac12_profile_without_flag_never_paced` (`pacer_tests.cpp`) |
| AC-13 | Pass | `ac13_start_afresh_no_lag_nothing_to_wait` (`pacer_tests.cpp`; after 5 s suspended, and after being ahead at full speed) |
| AC-14 | Pass | `ac14_new_machine_new_rate_and_profile` (`pacer_tests.cpp`; test-seg fixed → test-dot the call → back) |
| EC-1 | Pass | `ec1_call_slice_same_time_twice_runs_nothing` (`pacer_tests.cpp`; both kinds) |
| EC-2 | Pass | `ec2_toggling_full_speed_keeps_regular_stretches` (`pacer_tests.cpp`; 200 toggles, stretches of 1 to 4 calls) |
| EC-3 | Pass | `ec3_stopped_early_ends_the_call_resume_starts_afresh` (`pacer_tests.cpp`) |
| EC-4 | Pass | `ec4_call_slice_at_120_hz` (`pacer_tests.cpp`; both kinds) |
| EC-5 | Pass | `ec5_throttled_caller_runs_slow_without_failing` (`pacer_tests.cpp`; calls 150 ms and 1 s apart, both kinds; every call drops the lag and still runs one slice) |
| EC-6 | Pass | `ec6_slow_host_drops_lag_again_and_again` (`pacer_tests.cpp`; a host at half speed: more than 10 drops, the machine at 30 to 100 % of wall time) |

The tests were checked against four broken pacers (each change made, the
tests run, the change undone): the overshoot dropped (ten tests fail,
among them AC-1 to AC-4 and both long runs); the lag caught up instead of
dropped (the run does not end: EC-6's slow host falls behind without
limit); no jitter (AC-9 to AC-11 fail); R5 left out (AC-12 fails).

Technical Requirements (review): the pacer takes its clock from the
caller (`WallClock`); emulated time is compared with an absolute deadline
from the origin in integers (`ticksIn`), never with the last slice; the
pacer has no thread and no lock and reads no machine. `library/pacing/`
includes only `machine` and the standard library (LIB-1 design §2a).

### Deviations from the design

- **The first slice after a rebase ends on its grid point** (no jitter
  offset; the next ones are jittered). With R3 as written, a jittered first
  slice (`T₁ = S + r₁`) waits until `r₁` of wall time has passed. At a lag
  drop that is the same call (D = 0), so under EC-5 (every call drops) a
  jittered machine would almost never run. It also makes AC-13's "nothing
  to wait out" hold for jittered profiles. One unjittered slice end per
  start, drop or full-speed stretch does not affect AC-9 to AC-11.
- **A call that drops the lag on a profile whose slice is the call runs
  one slice of 1/60 s** (the D-2 value). R1 + R2 as written run nothing
  at that call (nothing is due at the moment of the drop), so under EC-5
  such a machine would never run, against "the machine runs slower". This
  is not a catch-up: the slice runs from the moment of the drop, as a
  fixed-slice machine runs its first slice at once. **For the user to
  confirm** at the part boundary; the alternative reading (nothing runs, a
  throttled tab stands still) is a one-line change in `Pacer::paced`.
- **"The wall end of the last unpaced slice" (R5)** is the clock read after
  each unpaced slice that did not report waiting; when a call switches from
  regular to full speed it starts as the call's start.
- **Switching from full speed back to regular** needs no rebase of its
  own: R4 has rebased at the end of the last full-speed call, and after a
  paced waiting stretch its origin is a regular one, which pacing goes on
  from.
- **`PaceResult`** is the sketch; "stopped early" stays inside the call.
  `kCallSlicesPerSecond = 60` names the D-2 value.
- **`ticksIn` and `wallTimeOf`** (wall duration to ticks and back) are free
  functions in `library/pacing/WallClock.hpp`, as the design allows; LIB-1's
  `EmulatedTime` has no such conversion and `library/machine/` is
  unchanged.
- **Test machine (LIB-1 D-8):** `test-seg`'s build option is
  `TestSegTiming`, handed in as `BuildRequest::content`; with it every
  instruction has one fixed length and `TestMachine::loopPosition()` gives
  the instruction of a loop of settable length. Built by
  `buildTimedTestSeg(length, loop)`. Test code only
  (`library/machine/tests/TestMachines.*`); without the option `test-seg`
  is unchanged and LIB-1's tests pass as before.
- **AC-8's case** is in `pacer_tests.cpp` (`ac8_…`), not `jitter_tests.cpp`
  as task L1 implies; it checks profiles in use by the pacer.

Not done in M-21 (other columns): AC-15 to AC-19 (Calc-U-59, M-33) and
AC-20 to AC-22 (Calc-U-1600, M-31).

## Calc-U-59 (M-33, 2026-11-15, branch `m33-sessions-59`)

The TI machines on the library's pacer through the app session as the
Swift and Qt6 frontends host it (`cu59::makeAppSession`; profiles in
`products/calc-u-59/app/PacingProfiles.hpp`). Tests:
`products/calc-u-59/tests/pacing_tests.cpp`, `pacing_longrun_after_tests.cpp`
(test clock, never the host's, except the Checkpoint run).

| ID | Result | How it was checked |
|---|---|---|
| AC-15 | Pass | `lib17_ac15_pacing_profiles_ti` (20 ms, jitter, paced while waiting, for the three models; a TI-59 keeps 0.99 to 1.01 of wall time over 2 s) |
| AC-16 | Pass | `lib17_ac16_full_speed_paced_at_key_readiness`, `lib17_ac16_full_speed_races_a_computation` (more than twice regular), `lib17_ac16_printer_wait_is_not_waiting` (a printing loop runs above 1.5× at full speed), `lib17_ac16_card_wait_is_waiting` (TI-59, `Write` without a card), `lib17_ac16_preset_wait_at_full_speed_on_idle_is_not_paced` (an operation that reports "not waiting": one emulated second in under 0.5 s on an idle machine) |
| AC-17 | Pass | `lib17_ac17_ti58_error_blink_pc_varies_over_50_slice_ends` |
| AC-18 | Pass (≤ 0.3 %) | `lib17_ac18_pacing_longrun_scripted_ti`, an hour per model, the calls of the "before" test: TI-59 rate 0.999994 against 2.3.0's 0.998681 (0.131 %), TI-58 0.999996 against 1.000000 (0.000 %), TI-58C 0.999996 against 0.998985 (0.101 %); worst \|emulated − wall\| within a stretch 0.0298 / 0.0295 / 0.0298 s. The differences are 2.3.0's cut to whole ticks (ledger 59-33). The real-clock run (Checkpoint) was not run in M-33 |
| AC-19 | Not run (Checkpoint) | iOS and macOS read `ticks` over attach (the XCUITest's client and `calcu attach` exist; the one-minute test was not written); Qt6 `--pacing-probe` and the Web test not built |

The "before" tests stay on 2.3.0's batches (`CooperativePacing`, the
cooperative half of `EmulationRunner`), whose real-clock form now pumps at
60 Hz instead of using the runner's thread.

### Deviations from the design

- **The Web keeps 2.3.0's batches until M-34** (`CooperativePacing`
  standalone with `Pacing.hpp`), not a library `Pacer` inside
  `CooperativePacing`: the reference runs drive `SessionController`
  cooperatively and stay byte for byte as recorded; `Pacing.hpp` and
  `TIPacingHints` therefore stay too. Ledger rows 59-32 and 59-33 hold for
  Swift and Qt6 from M-33 and for the Web from M-34.
- **`SliceRequest::due`** reaches the KeySequencer through the
  `TIKeySequencerHook` (a `SliceHook` that acts once per slice end).
