# C59-1: QA report

Results per acceptance criterion and edge case of `spec.md`.

## M-19 (2026-11-14, branch `m19-ti-machines`)

Built and run on an M4 Pro, macOS 27.2, Apple clang 21, CMake 4.5, Ninja.
The tests are in `products/calc-u-59/tests/`, program `CalcU59Tests`
(CTest entry of the same name, label `frequent`), plus the CTests
`CalcU59CoreSeam` and `CalcU59HeadlessSmoke`. Result of the run:
188 tests, all checks pass (after the user's ruling on AC-4, below); `CalcU59Tests` 7.5 s, the
Calc-U-59 part of Frequent 24.0 s.

| ID | Result | Test case (file in `products/calc-u-59/tests/`) |
|---|---|---|
| AC-1 | Pass | `c59_1_ac1_instruction_ticks_per_model` (`machine_tests.cpp`; 32 active, 128 in IDLE on the TI-59/58, only 32 on the TI-58C) |
| AC-2 | Pass | `c59_1_ac2_overshoot_below_one_instruction` (n = 1 … 10,000 and 200 fixed pseudo-random n up to 200,000; overshoot ≤ 127; return = growth; advance(0) = 0) |
| AC-3 | Pass | `c59_1_ac3_tick_count_goes_on_across_operations` (three models, Reset, power cycle, cold boot) |
| AC-4 | Pass | (user ruling 2026-11-14: in `ti59-fill-reset-coldboot` the PC after its `app-reset` is not compared; display, RAM and partition match 2.3.0)  `c59_1_ac4_golden_runs_through_the_machine_interface` (`golden_runs_tests.cpp`, 17 runs). 16 runs pass. `ti59-fill-reset-coldboot` differs at line 14, the line after its `app-reset`: golden `t=1199283 pc=1622`, replay `t=483 pc=1623`. `t` is compared as the difference to that line (design §8, D-3) and agrees on every later line (2104, 50770, …); the PC does not: the replay reaches the scan loop at another phase of it (after Reset the PC reads 1623/1622/1622/1623 against the golden's 1622/1623/1624/1622 on lines 14, 17, 23, 26). With the PC left out from that line to the next operation, every display line, the RAM and the partition at the end are those of 2.3.0 (checked once, not committed). The cause is AC-6: 2.3.0's Reset ran 300,000 instructions, Reset now ends at key readiness. AC-4 itself names the display output and the memory only; the design compares `t` and `pc` too. Not re-recorded and not compared away: for the user (see "Open") |
| AC-5 | Pass | `core_tests.cpp`: `c59_1_ac5_boot_reaches_key_readiness_all_models`, `c59_1_ac5_two_plus_three_equals_five`, `c59_1_ac5_control_nibble_glyphs`, `c59_1_ac5_error_blink_period_ti59`, `_ti58`, `_ti58c`, `c59_1_ac5_display_duty_normalised`, `c59_1_ac5_ti58c_keeps_memory`, `c59_1_ac5_start_up_clears_memory_except_ti58c` (the program that reaches the reset routine), `c59_1_ac5_start_up_clears_memory_except_ti58c_power_cycle`; the other 7 of the 17 (`rom_sentinels_and_sizes`, `rom_missing_file_throws`, `module_master_library_parses`, `card_insert_and_eject`, `printer_print_key_emits_line`, `printer_interrupt_partial_rows`, `breakpoint_sets_step_bit31`) through the adapters, names unchanged |
| AC-6 | Pass | `c59_1_ac6_reset_keeps_ram_and_partition_ti59_ti58` (4 Op 17 → 639.39 / 2 Op 17 → 319.19 kept, RCL addresses right), `c59_1_ac6_reset_equals_power_cycle_ti58c`, `c59_1_ac6_power_cycle_loses_memory_ti59_ti58`, `c59_1_ac6_power_cycle_keeps_constant_memory_ti58c` (RAM 0–63, H01–H03 written, kept), `c59_1_ac6_cold_boot_from_zeroed_memory`, `c59_1_ac6_constant_memory_in_place_before_start_up` (first step 11: a TI-58C built with `TIConstantMemory` has the RAM before its first instruction; 319.19 and Fix 2, 1 ÷ 3 = shows 0.33) |
| AC-7 | Pass | `c59_1_ac7_new_machine_equals_cold_booted` (RAM, SCOM, display at key readiness) |
| AC-8 | Pass | `c59_1_ac8_no_key_down_and_switched_on_display_after_each_operation` (two keys held into each operation; key lines clear; display "0") |
| AC-9 | Pass | `c59_1_ac9_45_key_ids_by_row_with_spellings` (the 45 IDs in row order on all three descriptors; `x<>t`, `x^2`, `sqrt`, `y^x`, `/`, `*` among the spellings; no `x2`) |
| AC-10 | Pass | `c59_1_ac10_each_key_id_presses_its_row_and_column` (all 45: the key line of row r, bit of column c as 2.3.0's `kKBits`; matrix code r×10+c round trip) |
| AC-11 | Pass | `c59_1_ac11_two_keys_down_both_in_the_matrix` (the machine side; the key sequencer side is LIB-5, M-36, row 59-13) |
| AC-12 | Pass | `c59_1_ac12_no_printer_keys` |
| AC-13 | Pass | `c59_1_ac13_twelve_positions_rightmost_first_segments_and_brightness` (12 positions, 8 segments, position brightness, DP's own brightness only; 123: index 0 side holds the 3; 1.5: one DP) |
| AC-14 | Pass | `c59_1_ac14_digit_minus_degree_quote_apostrophe_blank_and_zero_suppression` (the rules of `core/TISegments.hpp`; −23 on the machine, suppressed positions dark with no segment lit); the five pictures of a real TI-59 in `c59_1_ac5_control_nibble_glyphs` |
| AC-15 | Pass | `c59_1_ac15_c_indicator_apart_from_leftmost_position` (waiting: the number, C off; running `Lbl A GTO A`: positions dark, C on; the error blink of −1 ÷ 0: C on and a minus in position 11 in one output, each with a brightness) |
| AC-16 | Pass | `c59_1_ac16_no_sound_display_always_on` |
| AC-17 | Pass | CTest `CalcU59CoreSeam` (`tests/check_core_seam.sh`): no include of a core-internal header outside `core/` and `tests/` (app, frontends, tools), and `core/TIAdapters.hpp` declares none of the machine's own calls. Review: the app layer reaches model, advances and tick count, operations, keys and display through `calcu::machine::Machine` (`EmulatorSession`, `SessionController`, `FrameSampler`, `Pacing`, `StateApplier`, `KeySequencer` host) |
| AC-18 | Pass | `c59_1_ac18_key_ready_in_scan_loop_each_model_with_and_without_printer` (steady over 20,000 instructions), `c59_1_ac18_not_key_ready_while_a_program_runs` (`Lbl A GTO A`, with Pause, with Prt and the printer; 2 s each, three models) |
| AC-19 | Pass | `c59_1_ac19_restored_before_key_readiness` (the start-up clears the RAM; `isKeyReady()` false while the restore is pending; RAM and partition back when it turns true) |
| EC-1 | Pass | `c59_1_ec1_idle_advance_by_32_runs_one_of_128` |
| EC-2 | Pass | `c59_1_ec2_two_reads_without_advance_are_equal` |
| EC-3 | Pass | `c59_1_ec3_unknown_key_refused_state_unchanged` (`x2`, `PRINT`, `""`, and the spellings `x<>t`, `sqrt`, which are not IDs: key lines and tick count unchanged) |
| EC-4 | Pass | `c59_1_ec4_ti58_ti58c_find_their_card_switch_line` (after build and each operation key readiness; the TI-58's idle pass through 0x0459–0x045B) |

Support cases (no criterion): `c59_1_runbatch_stops_at_freeze_inside_advance`
(`runtime_tests.cpp`, design T11: FREEZE ON START lands on the same
instruction and tick count as 2.3.0's per-instruction `runBatch`), Pass;
`CalcU59HeadlessSmoke` (`ti59.py 5 + 7 =` against the CMake-built
`ti59headless`, display `12.`), Pass. The existing `shared_*_tests.cpp`
pass, with one assertion changed for AC-6 (below).

Other checks of the step:
- `build/all` (Qt frontends of both products): builds.
- Web: `frontends/web/build.sh` builds with `-fwasm-exceptions` (all 125
  compile lines of `build/web`); `ctest --test-dir build/groups -R
  CalcU59WebPlaywright`: Passed, 59 passed, 1 skipped, 77.1 s.
- `frontends/swift/build_xcframework.sh`: built (the library merged into
  `libCalcU59.a`, `library/**/*.hpp` under `Headers/machine/` …);
  `xcodebuild … -destination platform=macOS build` and `-destination
  'generic/platform=iOS Simulator' build`: BUILD SUCCEEDED.
- ThreadSanitizer (design §8, risk "the lock moves up"): `CalcU59Tests`
  built with `-fsanitize=thread` (`build/tsan`). The first run showed 3
  races, all in tests that read the machine without the guard while the
  runner ran (2.3.0's core locked those reads); the tests now go through
  `access()`/the guard. Second run: 0 warnings. The Qt app's smoke run under
  TSan was not done (the skill's rule: the user launches the apps).

## Deviations from the design

- `MachineGuard` holds a `std::recursive_mutex`, not a `std::mutex`, so a
  function holding an `Access` may call one that takes it again (several
  app paths nest, e.g. `DebugModel` builders under `access()`). Same
  removal steps.
- `FrameSampler` keeps the glyph of each position's last driven poll for
  `LedFrame::segments`, because `readDisplay()` applies the zero suppression
  (AC-14) while 2.3.0's frontends drew the latched glyph during the POV
  fade; the frontends show what they showed.
- A poll with no instruction since the previous tick counts as long as the
  previous poll (2.3.0's `pollTicks` of the repeated snapshot), not 0 s.
- `core/TIRomText.cpp` trims ASCII blanks only (the core cannot include
  `app/TextUtil.hpp`); the dumps are ASCII with known md5 values.
- `TIMachine::advance` consumes the core's breakpoint bit after every
  instruction, and stops on it only while run control is set.
- `KeySequencer`'s constructor no longer takes the model (it used it only
  for the scan-loop range, now `Host::isKeyReady()`).
- `h_new(model)` makes a handle; `h_load_model_roms` builds the machine (a
  machine cannot exist without its ROMs any more).
- `CALCU59_GOLDEN_RECORD=1` writes only a missing `.golden`, with
  `segments` lines (2.3.0's control/digit form cannot be produced any more).
- The XCFramework copies every `library/**/*.hpp`, the tests' headers
  included (the literal rule of LIB-1 §2a).
- Two tests carry today's two reset-routine tests (`…_except_ti58c` and
  `…_except_ti58c_power_cycle`): test names must be unique.

## Ruled (user, 2026-11-14)

- **AC-4 / golden `ti59-fill-reset-coldboot`:** the PC after `app-reset`
  differs (scan-loop phase, because Reset ends at key readiness, AC-6).
  Ruled: the PC after that run's `app-reset` is not compared; display,
  RAM and partition still must match, and do. The golden file is not
  re-recorded.
- **FREEZE ON START on a plain Reset (TI-59/58):** follows from AC-6;
  ledger row 59-34.
