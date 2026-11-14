# LIB-17 Pacing: design
Status: approved by the user at the Part B/C1 boundary (2026-11-14)

## 1. Scope

All three columns are worked in Part C1.

| Column | Step | Tests from (plan §3) | Done in (plan §5.1) |
|---|---|---|---|
| Library | M-21 | AC-1 to AC-14; EC-1 to EC-6 | M-21 |
| Calc-U-1600 | M-31 (with LIB-3 1600, LIB-15 1600) | AC-20 to AC-22 | M-31 |
| Calc-U-59 | M-33 (with C59-5, LIB-3 59) | AC-15 to AC-19 | M-33 |

No AC of this spec lands outside these three steps. M-34 (LIB-15 59)
removes the last adapter of this feature (§5) and runs AC-19's Web test
again. It adds no criterion.

The steps this design uses, besides M-21, M-31 and M-33:
- M-10 (FS 14): the "before" long-run tests exist and pass on the old
  pacers (`products/calc-u-59/tests/pacing_longrun_tests.cpp`,
  `products/calc-u-1600/tests/pacing_longrun_tests.cpp`).
- M-18 (LIB-1 L): the tick, LIB-1's emulated time (spec AC-7, AC-10) and
  the advance of a machine.
- M-22 (LIB-3 L): the machine session. It owns the pacer, calls it, and
  tells it the speed, start afresh and "waits for input" (LIB-3 AC-21 to
  AC-23).

## 2. Modules and files

Library (M-21), new folder `library/pacing/` of `calcu_library`,
namespace `calcu::pacing` (LIB-1 §2a; it uses `machine` for the
emulated-time type). Its tests are compiled into `calcu_library_tests`
(Frequent):

| File | Holds |
|---|---|
| `library/pacing/WallClock.hpp` | `WallTime` (monotonic nanoseconds) and `WallClock`, the clock function the caller supplies |
| `library/pacing/PacingProfile.hpp` | `PacingProfile` (AC-8) and `Speed` |
| `library/pacing/SliceJitter.hpp/.cpp` | the jittered slice ends (AC-9 to AC-11), the successor of `cu59::BatchJitter` |
| `library/pacing/Pacer.hpp/.cpp` | `SliceRunner`, `SliceRequest`, `SliceResult`, `PaceResult`, `Pacer` |
| `library/pacing/tests/TestSliceRunner.hpp` | a stand-in for the machine session that drives the test machine (§6) |
| `library/pacing/tests/pacer_tests.cpp` | AC-1 to AC-8, AC-12 to AC-14, EC-1 to EC-6 |
| `library/pacing/tests/jitter_tests.cpp` | AC-9 to AC-11 |
| `library/pacing/tests/pacing_longrun_tests.cpp` | AC-2, AC-3 over hours of scripted wall time, both kinds of profile |
| `library/CMakeLists.txt` (changed) | the sources of `library/pacing/` and its tests added to `calcu_library` and `calcu_library_tests` |

Calc-U-1600 (M-31):

| File | Change |
|---|---|
| `products/calc-u-1600/app/PacingProfiles.hpp/.cpp` (new) | `shared::pacingProfile(model)`: the profiles of AC-20 |
| `products/calc-u-1600/app/Pacer.hpp/.cpp` | deleted. `runEmulatedSeconds` and `runUntilPasteDone` are unpaced runs and leave with LIB-3 1600 (the harness and `--shots` advance directly; LIB-11) |
| `products/calc-u-1600/app/AppSession.*`, `MachineSession.*` | replaced by the library's sessions in M-31 (LIB-3). `m_pacer`, `pacer()` and the `restart()` calls go |
| `products/calc-u-1600/frontends/qt6/app/EmulationPacer.*` | stays as the Qt timer that calls the library session (LIB-3 AC-34). `setTurbo` sends the speed command (LIB-3 AC-21). The pacer access goes |
| `products/calc-u-1600/frontends/web/bindings.cpp` | `setTurbo` → the speed command, through the LIB-15 shell |
| `products/calc-u-1600/tools/wasm_smoke.cpp` | `shared::runEmulatedSeconds` → the library session's unpaced advance (LIB-3) |
| `products/calc-u-1600/app/tests/shared_pacing_tests.cpp` | `test_absolute_deadline` and `test_stall_and_restart` move to the library tests, `test_audio_pump` stays. New: `lib17_ac20_pacing_profiles_sharp` |
| `products/calc-u-1600/tests/pacing_longrun_tests.cpp` | the "after" test of AC-21 (§6) |
| `products/calc-u-1600/frontends/qt6/app/main.cpp` | debug-build option `--pacing-probe <s>` (§5, AC-22) |
| `products/calc-u-1600/frontends/web/tests/pacing.spec.js` (new) | AC-22 on the Web |

Calc-U-59 (M-33):

| File | Change |
|---|---|
| `products/calc-u-59/app/PacingProfiles.hpp/.cpp` (new) | `cu59::pacingProfile(Model)`: the profiles of AC-15 |
| `products/calc-u-59/app/Pacing.hpp/.cpp` | deleted (`kBatchSeconds`, `batchTargetCycles`, `BatchJitter`, `runBatch`, `shouldSleep`). The per-instruction freeze and breakpoint check of `runBatch` is already in the TI machine's advance since M-19, through C59-1's `TIDebugTargetAdapter::setRunControl(TIStepObserver*)` and `takeStop()` (C59-1 §4, §5; reconciled). `shouldSleep`'s inputs, C59-1's adapter `TIPacingHints`, go with it (C59-1 §5: removed in M-33) |
| `products/calc-u-59/app/EmulationRunner.hpp/.cpp` | deleted. Its threaded mode is replaced by the library session; its cooperative mode by `CooperativePacing` (§5) until M-34 (the Web keeps `SessionController` from M-33 to M-34, LIB-3 D-1) |
| `products/calc-u-59/app/Threading.hpp` | `Threads` goes with the runner if no other user is left (LIB-3 59 / C59-5 design); the pacer has no threads |
| `products/calc-u-59/app/EmulatorSession.*` | `m_runner` → `CooperativePacing` (Web only, until M-34) |
| `products/calc-u-59/tests/shared_runtime_tests.cpp` | `pacing_batches_are_cycle_weighted_with_carry`, `pacing_sleep_rule`, `runner_keeps_real_time` are replaced by the AC-15 to AC-17 tests below |
| `products/calc-u-59/tests/shared_cooperative_tests.cpp` | `cooperative_runner_*` move to `pacing_tests.cpp` (their library parts are in `library/pacing/tests/`) |
| `products/calc-u-59/tests/pacing_tests.cpp` (new) | AC-15 to AC-17 |
| `products/calc-u-59/tests/pacing_longrun_tests.cpp` | the "after" test of AC-18 (§6) |
| `products/calc-u-59/frontends/qt6/app/main.cpp` | debug-build option `--pacing-probe <s>` (§5, AC-19) |
| `products/calc-u-59/frontends/web/tests/pacing.spec.js` (new) | AC-19 on the Web |
| `products/calc-u-59/frontends/swift/Calc-U-59UITests/` (new test, as C59-5 lays out attach) | AC-19 for iOS and macOS, through attach |

## 3. Seams and interfaces

The interfaces below are sketches. `EmulatedTime` and `TickRate` stand
for LIB-1's emulated time (spec AC-7, AC-10), with the names LIB-1's
design gives them.

```cpp
namespace calcu::pacing {

using WallTime  = std::chrono::nanoseconds;      // monotonic, any epoch
using WallClock = std::function<WallTime()>;     // supplied by the caller; never goes back (TR; ADR 0009)

enum class Speed { Regular, Full };

struct PacingProfile {                           // AC-8
    std::optional<std::chrono::microseconds> slice;  // fixed length; nullopt = "the call"
    bool jitter = false;                         // AC-9 (fixed slices only)
    bool fullSpeedPacedWhileWaiting = false;     // AC-12
};

struct SliceRequest {
    std::uint64_t ticks;   // at least this many (LIB-1 AC-7)
    WallTime due;          // regular speed: the wall time the slice's end is due; full speed: the time it runs (§5)
};
struct SliceResult {
    std::uint64_t ticksRun;   // as the machine reports (LIB-1 AC-7)
    bool stoppedEarly;        // debugger pause or debug target (LIB-10); the call ends
    bool waitsForInput;       // LIB-3 AC-22: key readiness or a media place waiting, in this slice
};

class SliceRunner {           // implemented by LIB-3's machine session
public:
    virtual SliceResult runSlice(const SliceRequest&) = 0;
protected:
    ~SliceRunner() = default;
};

struct PaceResult {
    std::uint64_t ticksRun = 0;
    int slices = 0;
    bool ranUnpaced = false;  // a full-speed slice ran: the session throws its sound away (LIB-3 AC-20)
    bool lagDropped = false;  // AC-4, for tests and the log
};

class Pacer {
public:
    static constexpr std::chrono::milliseconds kLagLimit{100};           // AC-4
    static constexpr std::chrono::milliseconds kFullSpeedAllowance{14};  // AC-5
    // Slice length at full speed for a profile whose slice is the call: 1/60 s of
    // emulated time, as in Calc-U-1600 today (Pacer.cpp:54). D-2.

    explicit Pacer(WallClock clock, std::optional<std::uint32_t> seed = {});  // AC-10
    void setMachine(const PacingProfile&, TickRate);   // AC-14; includes startAfresh()
    void startAfresh();                                // AC-13
    PaceResult pace(Speed, SliceRunner&);              // one call of the host
};

}  // namespace calcu::pacing
```

**Ownership and threads.** The machine session owns one `Pacer`
(glossary: a machine session is a machine with its run loop, pacing and
input queue). Every call, the clock function included, runs on the
session's thread: the GUI thread for Qt6 and the Web (LIB-3 AC-28,
AC-34), the session thread for Swift (ADR 0015). The pacer has no lock
and no thread (ADR 0009, `frontend.md` I-1). It never reads a machine.
It knows emulated time only from `ticksRun` (AC-7: pacing never changes
a tick count).

**State.** The pacer keeps an origin (wall `W0` and the ticks run since
then, `E`), the index `n` of the next slice and its jittered end, and a
"waiting at full speed" flag. Due ticks are
`D = ticksAt(now − W0)`. They are computed in integers as whole seconds
× rate + (remaining ns × rate) / 10⁹, so there is no overflow for any
catalogue rate over years, and no floating point in the deadline (TR:
an absolute deadline). If LIB-1's emulated-time type converts a wall
duration to ticks, the pacer uses that conversion. If not, the
conversion is a free function in `calcu::pacing`.

**Rules of `pace()`**

| # | Rule | Serves |
|---|---|---|
| R1 | Regular speed, any profile: if `D − E` is more than `kLagLimit` in ticks, rebase on now (`W0 = now`, `E = 0`, `n = 0`). The lag is dropped whole; nothing is run to catch up | AC-4, EC-5, EC-6 |
| R2 | Regular, slice "the call": if `D > E`, run one slice of `D − E` ticks; `E += ticksRun`, so the overshoot is carried | AC-1, AC-2, AC-3, EC-1, EC-4 |
| R3 | Regular, fixed slice `S` (ticks = rate × slice, exact: 9,100 / 7,680 / 7,880 on the TI-59 / TI-58 / TI-58C): the end of slice n is `Tₙ = n·S + rₙ`, with `rₙ = 0` without jitter. While `Tₙ₊₁ ≤ D + S`: run `Tₙ₊₁ − E` ticks. The ends are absolute, so call errors, jitter and overshoot do not add up | AC-1 to AC-3, AC-9, EC-4 |
| R4 | Full speed: run slice after slice (fixed: the next `Tₙ₊₁ − E`; the call: 1/60 s) until the clock passes the call's start + `kFullSpeedAllowance`, with at least one slice per call. At the end, rebase on the clock's time | AC-5, AC-6, AC-7, EC-2 |
| R5 | Full speed with `fullSpeedPacedWhileWaiting`: when a slice reports `waitsForInput`, rebase on (the wall end of the last unpaced slice, the ticks before this slice) and treat this slice as slice 1. From then on R3 applies within the call, and R4's final rebase is skipped. The first slice that does not report it returns to R4 | AC-12, AC-16 |
| R6 | `stoppedEarly` ends the call. The session does not call while the debugger pauses the machine, and calls `startAfresh()` when it resumes | EC-3 |
| R7 | `startAfresh()` = rebase on the clock, with no lag and nothing to wait out. The jitter generator goes on (it is not reseeded), so a seeded run stays reproducible across a model switch. `setMachine()` takes the new rate and profile, then starts afresh | AC-13, AC-14 |

**Jitter.** `SliceJitter` draws `rₙ` evenly from 0 to S/2 ticks (AC-9).
It takes the caller's seed, or `std::random_device` when none is given
(AC-10). It uses `std::minstd_rand`, which the standard defines exactly,
with its own draw (`rng() % (S/2 + 1)`) in place of
`std::uniform_int_distribution`, whose results differ between libc++,
libstdc++ and MSVC. A seed therefore gives one sequence on every
platform. This differs from 2.3.0's `BatchJitter`, which uses the
distribution (§8).

## 4. Product side

**Calc-U-1600 (M-31).** The library app session and machine session
replace `shared::AppSession` and `shared::MachineSession` (LIB-3). The
machine session's `runSlice` does what `MachineSession::advance` does
today (`app/MachineSession.cpp:461-478`: paste frames, due keys, the
debugger slice through `runActive`) and returns the ticks actually run
(LIB-1 AC-7). The overshoot is no longer dropped (AC-3, AC-21). The
profiles (AC-20): PC-1500, PC-1500A and PC-1600 get slice "the call",
no jitter, and full speed never paced. Gone:
- `shared::Pacer` with `cyclesDue`, `kMaxCatchUpSeconds` and the
  0.1-second run before the drop (`app/Pacer.cpp:32-40`; AC-4 drops the
  lag whole);
- `kTurboSliceSeconds`, replaced by the library's `kFullSpeedAllowance`
  (the same 14 ms);
- the rebase on a new clock rate (`cyclesDue`), replaced by
  `setMachine()`.

Stays: the Qt6 `EmulationPacer` as the 60 Hz timer, its frozen mode for
`--shots` (no pacing: LIB-11, Out of Scope), and `AudioPump`, which is
fed `PaceResult::ranUnpaced` through LIB-3.

**Calc-U-59 (M-33).** The Swift and Qt6 frontends host the library
session (C59-5, LIB-3 AC-28). The session calls `pace()` at the host's
tick, not on the free-running emulation thread
(`app/EmulationRunner.cpp:106-127`). This is the loss of top speed that
the spec's decision log accepts. The profiles (AC-15): TI-59, TI-58 and
TI-58C get a fixed slice of 20 ms, jitter, and full speed paced while
waiting. "Waits for input" for a slice (AC-16, LIB-3 AC-22) is the TI
machine's key readiness (C59-1 AC-18, through LIB-1 AC-42) or "needs a
card now". Until C59-7 the card part comes from the adapter
`TICardWaitAdapter` (§5), LIB-3's `WaitProbe` for the TI machine, which
builds on C59-1's `TICardReaderAdapter` (reconciled with C59-1). A printer wait does not count, as with
`kPolledHardware` today (`core/TMC0501.hpp:127`). The exception of
AC-16: while a preset's `wait:` with a duration runs at full speed, the
machine session reports "not waiting". The session reports this, not
the pacer, so the pacer's rule stays one rule. Until LIB-6 59 (M-54),
the session gets this from the KeySequencer adapter that LIB-3 59's
design names. Gone: `Pacing.*`, `EmulationRunner.*` (both modes; the
cooperative mode is first reduced to `CooperativePacing`), `BatchJitter`
and `kFullSpeedSlice` (10 ms; the Web gets 14 ms, AC-5). Stays:
`FreezeController`, which is LIB-10 / C59-4's, inside the machine's
advance through C59-1's adapter. Also gone in M-33: C59-1's adapter `TIPacingHints` (polled hardware,
`inIdleAndKeyDetect()` for `shouldSleep`), whose last user is
`Pacing.cpp`. From M-33 "waits for input" is key readiness as the spec
states it (AC-16, C59-1 AC-18), no longer today's wider ranges, so a held
key's release-gate spin runs at full speed (D-3; ledger row). Also stays: the TI-58C persist debounce
(`batchDone` → `m_ramDirty`), which becomes "after a call with
`slices > 0`" in the session (LIB-3 / C59-3).

The unit changes from weighted instruction steps at `instructionHz` to
LIB-1 ticks (resonator periods, C59-1). The 20 ms slice is now an exact
whole number of ticks, so 2.3.0's cut to whole steps
(`app/Pacing.hpp:21-23`) and its 0.13 % / 0.10 % slow rate go (AC-18).

## 5. Adapters

| Adapter | File | Wraps | Removed in |
|---|---|---|---|
| `CooperativePacing` | `products/calc-u-59/app/CooperativePacing.hpp/.cpp` | what remains of `EmulationRunner`'s cooperative mode for `SessionController` (Web and its tests): `start`/`stop`/`pump(now, afterSlice)`/`post`/`sync` over a `calcu::pacing::Pacer` with a `SliceRunner` on the TI machine (LIB-3 D-1: the old Web stack stays one step) | M-34 (`SessionController` goes) |
| `SliceRequest::due` | `library/pacing/Pacer.hpp` | the wall time at each slice end that the KeySequencer needs today (`EmulatorSession::pump`, `app/EmulatorSession.cpp:52-60`: holds in wall time) | M-36 (LIB-5: holds in emulated time, ledger 59-23), if nothing else reads it |
| `TICardWaitAdapter` | `products/calc-u-59/app/TICardWaitAdapter.hpp` | "needs a card now": a LIB-3 `WaitProbe` over C59-1's `TICardReaderAdapter::isWaitingForCard()` and `isInCardWait()` (`core/TIAdapters.hpp`, reached by `cardReaderOf(Machine&)`; the `cardWait` range of `app/MachineModelInfo.hpp:68-75` is in `core/TIRomRanges.hpp` since M-19) (reconciled with C59-1) | M-55 (C59-7: a media place that waits, LIB-7; C59-1 removes `TICardReaderAdapter` there too) |
| `--pacing-probe <s>` | `frontends/qt6/app/main.cpp` of both products, debug builds | not an old call. A test hook: after `<s>` of wall time it sends the session's request for emulated time (the one LIB-11's `ticks` uses), prints ticks, seconds and wall time, and exits 0 or 1 | when the Qt6 frontend gets attach (LIB-14: built in M-43, product columns M-49 / M-57), where the check becomes an attach client |

The Calc-U-1600 pacer needs no adapter. M-31 replaces it in one step,
together with `AppSession`.

## 6. Tasks

Tests come before the code. The groups are those of
`docs/test-groups.md`. The library tests use `TestSliceRunner` on LIB-1's
test machine (LIB-1 AC-24 rule; dependency D2). The test machine has a
fixed instruction length, so the overshoot is known, and a loop whose
length can be set: LIB-1's `test-seg` with its build option for both
(LIB-1 D-8). `TestSliceRunner` adds a script of `waitsForInput` per
slice.

| Task | What | ACs/ECs | Test (file, case, group) |
|---|---|---|---|
| L1 | Types, profile, `SliceJitter` | AC-8, AC-9, AC-10 | `jitter_tests.cpp`: `ac9_jitter_offsets_within_half_a_slice_and_do_not_add_up`, `ac10_jitter_same_seed_same_sequence`, `ac10_jitter_unseeded_differs`; Frequent |
| L2 | R3 with jitter on a loop | AC-11 | `jitter_tests.cpp`: `ac11_jittered_slice_ends_hit_more_than_one_loop_position`; Frequent |
| L3 | R2: slice "the call" | AC-1, EC-1, EC-4 | `pacer_tests.cpp`: `ac1_call_slice_keeps_wall_time_at_60_hz`, `ec1_call_slice_same_time_twice_runs_nothing`, `ec4_call_slice_at_120_hz`; Frequent |
| L4 | R3: fixed slice | AC-1, AC-2, AC-3 | `pacer_tests.cpp`: `ac1_fixed_slice_keeps_wall_time`, `ac2_uneven_calls_do_not_add_up`, `ac3_overshoot_does_not_add_up` (both kinds); Frequent |
| L5 | R1: lag | AC-4, EC-5, EC-6 | `pacer_tests.cpp`: `ac4_lag_over_limit_dropped_whole`, `ec5_throttled_caller_runs_slow_without_failing`, `ec6_slow_host_drops_lag_again_and_again`; Frequent |
| L6 | R4: full speed | AC-5, AC-6, AC-7, EC-2 | `pacer_tests.cpp`: `ac5_full_speed_stops_at_14_ms_of_the_supplied_clock`, `ac6_back_to_regular_neither_waits_nor_lags`, `ac7_full_speed_keeps_tick_count`, `ec2_toggling_full_speed_keeps_regular_stretches`; Frequent |
| L7 | R5: paced while waiting | AC-12 | `pacer_tests.cpp`: `ac12_waiting_at_full_speed_waits_one_slice_after_last_unpaced`, `ac12_not_waiting_returns_to_full_speed`, `ac12_profile_without_flag_never_paced`; Frequent |
| L8 | R6, R7: start afresh, new machine, pause | AC-13, AC-14, EC-3 | `pacer_tests.cpp`: `ac13_start_afresh_no_lag_nothing_to_wait`, `ac14_new_machine_new_rate_and_profile`, `ec3_stopped_early_ends_the_call_resume_starts_afresh`; Frequent |
| L9 | Library long run | AC-2, AC-3 (hours) | `pacing_longrun_tests.cpp`: `ac2_ac3_library_longrun_scripted_fixed`, `ac2_ac3_library_longrun_scripted_call` (5 h scripted, jitter ±4 ms, an oversleep every 9,000 calls, stalls); Frequent |
| P16-1 | Sharp profiles | AC-20 | `app/tests/shared_pacing_tests.cpp`: `lib17_ac20_pacing_profiles_sharp`; Frequent |
| P16-2 | "After" long run on the library session | AC-21 | `tests/pacing_longrun_tests.cpp`: `lib17_ac21_pacing_longrun_scripted_pc1500a`, `_pc1600` (on, then off; ≤ 100 ppm over the run at `kCpuHz` / `kTStateHz`, the AC-1 bound over a stretch); Frequent. `lib17_ac21_pacing_longrun_real_clock_*`; Checkpoint (`CalcU1600PacingRealClock`, 600 s) |
| P16-3 | Frontends | AC-22 | `CalcU1600QtPacingProbe` (`--pacing-probe 60`, offscreen) and `frontends/web/tests/pacing.spec.js` `a PC-1600 keeps one minute`; Checkpoint, run once in M-31 for the qa-report (D-5) |
| P59-1 | TI profiles | AC-15 | `tests/pacing_tests.cpp`: `lib17_ac15_pacing_profiles_ti`; Frequent |
| P59-2 | "Waits for input" | AC-16 | `tests/pacing_tests.cpp`: `lib17_ac16_full_speed_paced_at_key_readiness`, `lib17_ac16_full_speed_races_a_computation`, `lib17_ac16_printer_wait_is_not_waiting`, `lib17_ac16_card_wait_is_waiting` (TI-59), `lib17_ac16_preset_wait_at_full_speed_on_idle_is_not_paced`; Frequent |
| P59-3 | Blink | AC-17 | `tests/pacing_tests.cpp`: `lib17_ac17_ti58_error_blink_pc_varies_over_50_slice_ends` (port of `cooperative_runner_batch_ends_do_not_lock_onto_a_loop`); Frequent |
| P59-4 | "After" long run | AC-18 | `tests/pacing_longrun_tests.cpp`: `lib17_ac18_pacing_longrun_scripted_ti` (TI-59, TI-58, TI-58C, 1 h each; the measured rate against FS 14's recorded 2.3.0 rate, `int(instructionHz × 0.02) / (instructionHz × 0.02)`: pass ≤ 0.3 %; 0.3 to 1 % prints a notice for the user and the qa-report; fail > 1 %); Frequent. `lib17_ac18_pacing_longrun_real_clock_ti`; Checkpoint (`CalcU59PacingRealClock`) |
| P59-5 | Frontends | AC-19 | iOS and macOS: an XCUITest that drives `calcu` over attach (C59-5) and reads `ticks` before and after 60 s; Qt6: `CalcU59QtPacingProbe`; Web: `frontends/web/tests/pacing.spec.js` `a TI-59 keeps one minute` (through `CooperativePacing` and a binding for the tick count, then through LIB-15 in M-34); Checkpoint, run once in M-33 (D-5) |

Case names follow LIB-1 §2a (`acN_`/`ecN_` in the library, `lib17_acN_` in the products; reconciled). Coverage: L1 to L9 cover AC-1 to AC-14 and EC-1 to EC-6 (20 of 20).
P16-1 to P16-3 cover AC-20 to AC-22 (3 of 3). P59-1 to P59-5 cover
AC-15 to AC-19 (5 of 5).

**How AC-19 and AC-22 read a running frontend's emulated time** (TR,
decided here): through the session's request for emulated time, the
same one that LIB-11's `ticks` uses. Swift reads it over attach (M-33).
Qt6 reads it through `--pacing-probe` until it has attach. The Web reads
it through Playwright `page.evaluate` on the frontend's message channel
(LIB-15 in Calc-U-1600; in Calc-U-59 a binding on `CooperativePacing`
until M-34). The wall time is the test's own clock. The bound is that
of AC-1.

## 7. Tools and agent knowledge

Today's tools are unaffected. `tools/headless/` (Calc-U-59) and
`pc1500_cli`, `pc1600_cli` and the probes do not pace: they advance
directly (moved onto the interface in M-19 and M-26). `wasm_smoke.cpp`
moves in M-31 (§2).

Agent knowledge that changes in the same step:

| Step | Files |
|---|---|
| M-21 | none (new folder; `docs/GLOSSARY.md` already has slice, lag and pacing profile) |
| M-31 | `products/calc-u-1600/docs/developer/Shared-Architecture.md` (rows `Pacer` and `AppSession`, lines 11, 21, 55, 59, 82, 98), `Web-Client.md`, `Cloud-Sessions.md` (plan M-31); the glossary-mapping row "Full speed: `setTurbo`, `turboRequested`" once the code takes the term; memory `calc-u-1600/project_shared_layer_wasm.md` (names `Pacer`) |
| M-33 | `products/calc-u-59/reference/SharedArchitecture.md` (§ "Emulation Loop and Timing", lines 39-40, 123, 133, 140, 159-176), the `calc-u-59-core` skill (line 35 points pacing at that file), the `calc-u-59-shared`, `calc-u-59-swift` and `calc-u-59-qt` skills (threading), `frontends/swift/App/EmulatorViewModel.swift:79` (comment names `Shared/Pacing.cpp shouldSleep`); `docs/glossary-mapping.md` rows for `FullSpeed:`, `RegularSpeed:` stay (preset directives, LIB-6) |
| M-34 | `calc-u-59-web` skill, `reference/WebClient.md` (`CooperativePacing` goes) |

`products/calc-u-1600/docs/background/*` (plans and `Decisions.md`)
are history. They stay as they are and are not renamed.

## 8. Golden runs and risks

- **Golden runs** (`products/calc-u-59/tests/golden-runs/`, 34 files;
  `products/calc-u-1600/tests/golden-runs/`) advance machines directly
  and do not go through a pacer. They guard M-31 and M-33 only against
  changes to the session around the pacer. A difference is a finding.
- **The TI reference runs of M-9** (`products/calc-u-59/tests/reference/v2.3.0-examples/`,
  `tests/reference_runs_tests.cpp`) drive `SessionController`
  cooperatively on a supplied clock, so they go through the pacer. After
  M-33 they go through `CooperativePacing`. The slice ends move: they are
  in ticks, no longer cut to whole steps, and the jitter draw is new.
  Key holds stay in wall time until M-36. An `Elapsed:` line or a
  printout timed by key holds may therefore differ. A difference is a
  finding, not a new reference (plan §1). The runs are not seeded today
  (`jitterSeed` defaults to `random_device`), so an output that depended
  on the jitter would already be flaky.
- **Frequent budget.** The 1600 "after" test must cover a switched-off
  PC-1600 (AC-21). Today's scripted default is 120 s
  (`kDefaultScriptedSeconds`), and the PC-1600 switches itself off after
  about 10 minutes. Running 11 minutes at 3.58 MHz in Frequent may break
  the 60 s budget (Calc-U-1600 Frequent: 37 s). The test switches the
  machine off by its power key half way through instead of waiting for
  the auto-off (D-4).
- **Checkpoint load.** AC-19 and AC-22 add six one-minute runs. The
  Longer group (about 6 of 10 minutes used) cannot take them, so they go
  to Checkpoint (D-5).
- **Seeded jitter changes sequence** (§3): no test pins 2.3.0's
  sequence. `cooperative_runner_batch_ends_do_not_lock_onto_a_loop` tests
  a property (seed 59).
- **Top speed** of Calc-U-59 Swift and Qt6 at full speed falls (spec
  decision log). Not measured; a much slower preset load would be
  visible to the user (ledger row, D-6).

## 9. Decided (best guess)

- **D-1 (was OQ-1; AC-1, AC-3).** R3 as drafted: AC-1's bound reads "one
  slice plus the last instruction", the overshoot no rule can avoid.
- **D-2 (was OQ-2; AC-5, AC-8).** The full-speed slice of a profile whose
  slice is the call is a library value of 1/60 s, as Calc-U-1600 today;
  no profile field (AC-8 names none).
- **D-3 (was OQ-3; AC-16).** Up to M-33 today's ranges stay behind
  `TIPacingHints` (C59-1 D-2); from M-33 "waits for input" is key
  readiness or a card needed now, as AC-16 states. A held key's
  release-gate spin is no longer paced at full speed (ledger row).
- **D-4 (was OQ-4; AC-21).** The scripted test switches the machine off
  by its power key half way through; it stays Frequent.
- **D-5 (was OQ-5; AC-19, AC-22).** Checkpoint, run once in the step for
  the qa-report; Longer has no room.
- **D-6 (was OQ-6).** The user-visible pacing changes get ledger rows: the
  lower top speed of Calc-U-59 Swift and Qt6, the Calc-U-59 Web
  full-speed allowance from 10 to 14 ms, Calc-U-1600 no longer running
  0.1 s after a stall, the TI-59 / TI-58C no longer 0.13 % / 0.10 % slow.
- **D-7 (was OQ-7).** Closed before (C59-1 design §4, §5): the freeze and
  breakpoint checks are in the TI machine's advance from M-19.
- **D-8 (was OQ-8).** Pacing profiles stay in each product's app layer
  (`PacingProfiles.*`); the descriptor holds no pacing data.
