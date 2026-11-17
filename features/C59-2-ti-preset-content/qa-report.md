# QA report: C59-2 TI preset content

Results per acceptance criterion and edge case of `spec.md`.

## M-54 (2026-11-16, branch `m54-presets-59`)

Built and run on an M4 Pro (macOS 27.2, Apple clang, Ninja,
RelWithDebInfo): `build/tests` (`-DCALCU_FRONTEND_QT=OFF`, Frequent),
`build/qt` (`-DCALCU_FRONTEND_QT=ON`), the Web `dist/` with
`frontends/web/build.sh` and `frontends/web/run-tests.sh`. Swift: type-checked
only (`swiftc -typecheck`, `clang++ -fsyntax-only` for the bridge;
`xcodebuild` finds no XCFramework from a worktree).

The code (design §2, §3): the product kinds `c59.program-memory` and
`c59.constant-memory` in `core/TIProductKinds.hpp` (implemented by
`core/TIMemoryKinds.*` on `TIMachine`), `app/Keycodes.*` (the compact and
the listing notation, the keystroke program file), `app/Bcd.*`
(`parseDecimal`), `app/CueCardYaml.*` (the stand-alone cue card block,
read and written), `app/TIPresetKeys.*` (the sections `partition:`,
`program:`, `data:`, `cue-card:` and the step `printer-trace:`),
`app/TIPresetCommands.*` (`ProgramMemoryPart`: `c59.partition`,
`c59.program`, `c59.registers`) and the harness commands `ti59 partition`,
`ti59 program`, `ti59 registers` (`app/HarnessProduct.cpp`). The reader of
2.3.0 is gone (`app/StateFile.*`, `app/StateApplier.*`,
`app/TIKeystrokePlayback.*`, `app/ConstantMemory.*`,
`tests/fixtures/statefiles/`, the 150 `STATE` records of
`tests/golden/formats.txt`). 53 examples upgraded to version 2, the three
startup presets `examples/startup/default-ti59.ti59`, `-ti58.ti58`,
`-ti58c.ti58c`, and `examples/bundle.yaml`. Tests:
`tests/preset_content_tests.cpp`, `tests/harness_tests.cpp`,
`tests/reference_runs_tests.cpp`, the ported cases of
`tests/part_flows_tests.cpp`.

| ID | Result | Test case |
|---|---|---|
| AC-1 | Pass | `c59_2_ac1_only_listed_steps_written`, `c59_2_ac1_first_stage_as_today` (sum.ti59's RAM equals its 2.3.0 reference, 120 registers); every example through C59-2 AC-11 |
| AC-2 | Pass | `c59_2_ac2_rounds_up_to_80_at_most_960`, `c59_2_ac2_point_part_not_read`, `c59_2_ac2_none_keeps_rom_partition` (479, 239, 239), `c59_2_ac2_not_a_number_or_above_last_step_refused` |
| AC-3 | Pass | `c59_2_ac3_compact_and_listing_notations`, `c59_2_ac3_mnemonic_not_read_even_a_number`, `c59_2_ac3_one_digit_or_word_refused_with_line` (the line of the token inside the file), `c59_2_ac3_step_beyond_last_refused`, `c59_2_ac3_keystroke_program_file_read_by_same_rules`, `c59_2_ac3_file_breaking_its_format_refused` |
| AC-4 | Pass | `c59_2_ac4_whole_number_digit_for_digit_cut_after_13`, `c59_2_ac4_other_values_rounded_to_13`, `c59_2_ac4_no_number_refused`, `c59_2_ac4_60_to_99_on_ti58_refused_h_off_ti58c_refused`, `c59_2_ac4_h00_to_h03_are_not_60_to_63` |
| AC-5 | Pass | `c59_2_ac5_keys_defaults_tokens_prime_any_case`, `c59_2_ac5_template_banks_id_refused` |
| AC-6 | Pass (C++ and Web); Qt6 scenario in M-57 | `c59_2_ac6_topic_says_fitted_card`, `c59_2_ac6_no_section_no_card`; Web: `lib6_ac42 bundled example sum shows its cue card` (`frontends/web/tests/play-calculator.spec.js`) shows the title of `cue-card:` (design §1: the Qt6 scenario comes with LIB-14 59) |
| AC-7 | Pass | `c59_2_ac7_sections_after_keys_are_reloads`, `c59_2_ac7_preset_plays_its_keys`, `c59_2_ac7_reload_keeps_the_program`, `c59_2_ac7_reload_after_the_first_run` |
| AC-8 | Pass | LIB-6 AC-53 and AC-40 of the Calc-U-59 column: `lib6_ac53_ti_idle_rule_values`, `lib6_ac36_*`, `lib6_ac40_screenshot_is_the_classic_segment_picture` (reference picture `tests/reference/display/ti59-123456789.5-.png`, made by this code, for the user's review at the E/F boundary, LIB-6 D-39); nothing is done after the build (the library's default) |
| AC-9 | Pass | `c59_2_ac9_partition_program_registers_write_and_read`, `c59_2_ac9_refused_where_the_section_refuses`, `c59_2_ac9_count_reads_keycodes` (`count=<n>`, Q-2) |
| AC-10 | Pass for 53 examples; the four card examples in M-55 | `c59_2_ac10_no_2_3_0_preset_left` (the four of design §1 excepted by name), `calcu_preset_check.CalcU59Presets` (Frequent, the same four excluded); `debug/example-brief.ti59` line 16 corrected; `historic/trace-quirk.ti59` keeps its extension (EC-6) |
| AC-11 | Pass (53 examples; 8 named findings, below; the four card examples wait for M-55) | `c59_2_ac11_reference_runs_of_the_examples` (`CalcU59ReferenceRuns`, Longer, 7 s): 0 of 57 differ, 45 equal in display, partition, raw RAM and paper, 8 named, 4 waiting |
| AC-12 | Not in M-54 | M-65 (the site's copies) |
| AC-13 | Pass (nine examples; `globe-print` in M-55) | `c59_2_ac13_bundle_lists_three_startup_and_todays_examples_in_groups` (each startup preset: ML and the PC-100C, nothing else) |
| AC-14 | Pass on the Web; Qt6 and Swift bundle the files, their scenarios in M-57 / M-58 | Web: `frontends/web/build.js` copies `bundle.yaml` and its files into `dist/presets/`; Playwright `lib6_ac42 …` load from it. Qt6: `frontends/qt6/CMakeLists.txt` copies the files beside the program (built, `Calc-U-59.app/Contents/Resources/examples`). Swift: `frontends/swift/copy_examples.sh` in the XCFramework phase, checked by `check_xcode_project.sh` (Frequent); not built here. `CalcU59BundledFiles` (design T11, Longer) is not written: the Qt6 and Swift scenarios of LIB-6 AC-42 (M-57, M-58) load a bundled example |
| AC-15 | Pass | `c59_2_ac15_names_short_described_once_unique_per_model` (`checkBundle`, the preset list: 12 entries) |
| AC-16 | Pass | `c59_2_ac16_apply_state_ti58_default_partition_and_limits`, `c59_2_ac16_reload_changes_only_the_listed_steps_and_registers`, `c59_2_ac16_data_registers_match_the_rom`, and the `REG`, `BCDD`, `BCDT`, `MATH` records (`bcd_regline_math_match_swift`, unchanged) |
| AC-17 | Pass | `c59_2_ac17_listed_on_every_model`, `c59_2_ac17_step_register_extra_partition_read_back` |
| AC-18 | Pass | `CalcU59CalendarScripts` (section "Calendar scripts" below) |
| EC-1 | Pass | `c59_2_ec1_steps_beyond_partition_written` |
| EC-2 | Pass | `c59_2_ec2_data_before_partition_in_file_order` |
| EC-3 | Pass | `c59_2_ec3_h_registers_written_after_cold_boot_else_as_startup_left` |
| EC-4 | Pass | `c59_2_ec4_second_replaces_first` |
| EC-5 | Pass | `c59_2_ec5_model_printer_card_unknown_keys` |
| EC-6 | Pass | `c59_2_ec6_trace_quirk_keeps_extension_and_model` |

### The reference runs (AC-11; design §8)

The 2.3.0 records of M-9 are unchanged; no reference was recorded anew.
The comparison is new (D-16, D-17): the example's version 2 file through
`calcu --product 59` in-process, `preset`, then the bare `wait`; display,
partition, the raw RAM (120 registers) and the paper (`paper text`,
`paper marks`); `kAccepted` is gone. Findings, each named in
`tests/reference_runs_tests.cpp` with the parts that may differ:

- `debug/example-brief.ti59` (display, RAM): AC-10's correction of line 16.
  The M-9 record was made from the uncorrected 2.3.0 file (2.3.0 skipped
  `11.`), so step 002 now holds 11 and Lbl A runs (design §8 foresaw it).
- `picard.ti59`, `switzerland.ti59`, `historic/texas-print.ti59`,
  `screentest/test_texas.ti59`, `historic/printer-quirks.ti59`,
  `historic/fast-grafik-3d-plot.ti59` (paper marks): `paper text` is the
  record's line for line; the `.paper-marks` of M-39 were drawn from those
  text lines (LIB-8 AC-39's converter), but these programs print with the
  printer interrupt or plot rows, so the real paper's dots and reach are
  not text rows. The same reason as the three examples of LIB-8 D-13 that
  have no `.paper-marks`. **For the user:** delete these six
  `.paper-marks` (as LIB-8 D-13 did for three), or keep them as findings.
- `screentest/test_58_glyphs.ti58` (display): the screen test ends with
  `- pause: cpu` in the error blink; `wait: 400ms` now counts emulated time
  (LIB-6 AC-20) and the pause lands by LIB-10's rules, so the display is
  caught in another phase of the blink. RAM, partition and paper are the
  record's.

`historic/life.ti59` is only loaded (it never stops). Waiting for M-55:
`globe-print.ti59`, `historic/characters-fastmode.ti59`,
`historic/pdcm-card-tricks.ti59`, `historic/pdcm-make-cards.ti59`.

### The upgrade (AC-10; design §4)

No converter is in the repository (D-10). The 53 files were drafted by a
throwaway script by AC-10's table (matrix codes → key IDs, `21` and its
key one step, a `21` at the end of a line joined to the next key, the
first `keys:` with `- regular-speed:` unless 2.3.0 started with
`FullSpeed:`, directives → steps, `...` → `# ...`, a one-digit keycode
written with two digits, sections reordered partition, program, data, cue
card within a stage, `Template`/`Banks`/`IdAlign` dropped, `Id` kept as a
comment), then checked by `check-preset` and the reference runs, and the
comments read by hand. Decided per file (D-12): `- let-it-finish:` after
the last key where 2.3.0 ran it at full speed. One file changed beyond the
table: `screentest/test_cdebug.ti59` ends with the key whose program
pause-on-start stops; its trailing `Wait: 1.5s` (there for XCUITest's
playback status, which FS 4 removed) went, since a paused machine is never
idle and the preset would fail on it.

### Calendar scripts (AC-18)

`CalcU59CalendarScripts` (Frequent, 2 s): `tools/agentic/calendar/fastcal.py`
writes `calendar-09` byte for byte as the tracked version 2 file;
`years.py` finds the year in it (the four `- key:` digit steps before the
first step that names A) and its rows check, through `calcu serve`.
`tools/agentic/ti59file.py` is gone; the tools load presets through
`calcu` (`tools/agentic/calcu59.py`); `stepcost.py` and `fastmode_matrix.py`
use the helpers moved into `tools/headless/ti59.py` until M-59. The
calendar project's number for 2026 (LIB-6 §11.7): 75.94 s with the old
tool on the 2.3.0 file and 75.94 s through `calcu` (61 of 61 rows): no
difference.

### Deviations from the design

- The kinds' interfaces are in `core/TIProductKinds.hpp`, not
  `app/ProgramMemory.hpp`: the core implements them and cannot include
  `app/` (as Calc-U-1600's `core/Machine/SharpProductKinds.hpp`).
- The commands of AC-9 are a session part of their own
  (`ProgramMemoryPart`), in the app session's parts and in the harness
  entry's, rather than commands of `TIAppPart`: the harness entry makes its
  own library parts until M-59 and has no `TIAppPart`.
- The small texts of the refusals are inline in the tests, not files under
  `tests/fixtures/presets/`.
- `TIMemoryAdapter` keeps its writes: the app layer no longer calls them,
  but the core tests and the golden runs' replay of 2.3.0's start-up do
  (`tests/golden_runs_tests.cpp` now inlines 2.3.0's `powerOn`, unchanged in
  effect: the golden runs are equal).
- `CalcU59BundledFiles` is not written (AC-14 above).

## Ledger

Rows that became visible: 59-1 (presets of 2.3.0 refused with the hint),
59-3, 59-4, 59-5, 59-26. No new row.

## M-55 (2026-11-17, branch `m55-cards-59`): the four card examples

| ID | Result | Test case |
|---|---|---|
| AC-10 | Pass for all 57 examples | `c59_2_ac10_no_2_3_0_preset_left` (no example excepted), `calcu_preset_check.CalcU59Presets` without exclusions; the four card examples upgraded by C59-7 AC-30 (C59-7's qa-report) |
| AC-11 | Pass (57 examples; 10 named findings) | `CalcU59ReferenceRuns`: 0 of 57 differ; new findings `globe-print.ti59` (paper marks of a printer-interrupt picture, as the six of M-54) and `historic/characters-fastmode.ti59` (the record was taken while the table was still printing); `pdcm-card-tricks` and `pdcm-make-cards` equal, their card banks too |
| AC-13 | Pass (ten examples) | `c59_2_ac13_bundle_lists_three_startup_and_todays_examples_in_groups` with `globe-print.ti59` back in "Printer pictures"; `c59_2_ac15_…` (13 entries) |

The 2.3.0 file the tests of an outdated preset read is now the fixture
`tests/fixtures/v2.3.0/globe-print.ti59`. Column status stays In progress:
AC-12 (the site's copies) is M-65's.
