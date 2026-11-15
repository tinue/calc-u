# LIB-6: QA report

Results per acceptance criterion and edge case of `spec.md`, per column.

## Library (M-38, 2026-11-15, branch `m38-presets`)

Built and run on an M4 Pro, macOS 27.2, Apple clang, CMake, Ninja,
RelWithDebInfo (`build/tests`, `-DCALCU_FRONTEND_QT=OFF`). The code is
`library/presets/` (`calcu::presets`, in `calcu_library`): `Source.*`,
`ProductRules.hpp`, `Vocabulary.*`, `Reader.*`, `LibraryKeywords.cpp`,
`Expect.*`, `Engine.*`, `IdleWatch.*`, `BundleFile.*`, `PresetsPart.*`,
`PresetChecks.cmake` with `RunPresetCheck.cmake`; the harness commands in
`library/harness/PresetCommands.cpp` and the bare `wait` in
`FirstCommands.cpp`; the changes of design §2 in other areas:
`formats/Yaml.hpp` (`YamlOptions`, `YamlNode::rawLine`),
`formats/Number.*` (`parseNumber`), `sessions/Operation.*`,
`SessionPart.hpp`, `AppSession.cpp` (`OperationContext::hostWait`, D-17;
two commits of their own), `peripherals/PeripheralsPart.*` (`set-control`,
D-16), `harness/Product.hpp`, `Run.*`, `Reply.hpp`, `cli/Cli.cpp`,
`typing/TypingPart.hpp` (`product()`), and the test machines
(`TestSegProgram`, `test.memory`, D-25).

The tests are `library/presets/tests/*_tests.cpp` and
`library/harness/tests/preset_commands_tests.cpp` in `calcu_library_tests`
(Frequent), the CTest entries `calcu_cli.ac34_check_preset_exit_status`
and `calcu_preset_check.LibraryPresetFixtures`. They run
`library/presets/tests/PresetRig.hpp` (an app session with PeripheralsPart,
PresetsPart, StartupPart and TypingPart; test-seg at 100 kHz with a
program started by key `1` and the memory `test.memory`, test-dot at
1 MHz; typing profiles whose presses wait for key readiness) and
`TestPresetRules.hpp` (idle: key ready for 2 ms on test-seg, 20 frames on
test-dot; the picture is the display output as bytes; the comparison is
the display text of LIB-11's test product; the keywords `test-header:`,
`test-memory:`, `test-panel:` (a frontend step), `printer-trace:` on the
test printer's `trace` switch, `test-undefined:`), in virtual time and
paced on the session tests' scripted clock. Fixtures:
`library/presets/tests/fixtures/` (`bad/`: 22 files, one per class of
error, each naming its error in its first line; `bundle/`; `good/`; a
template filled by `fill.cmake`). The harness cases run the harness's test
product, which got the test rules and two bundled presets.

Result of the run: `calcu_library_tests` 575 `TEST`s (120 new: 105 in
`library/presets/tests/`, 11 harness, 2 sessions, 2 formats), 4,222 checks
passed, 0 failed, 3.6 s wall time. The whole Frequent group passed (30 of
30 CTest entries; 43 s wall time with `-j8`).

| ID | Result | Test case (file in `library/presets/tests/` unless named) |
|---|---|---|
| AC-1 | Pass | `ac1_block_and_flow_forms_of_readme_section_1`, `ac1_quotes_without_escapes`, `ac1_block_scalar_skips_blank_and_hash_lines`, `ac1_hash_without_space_is_text`, `ac1_raw_values_of_type_expect_not_expect`, `ac1_tab_anywhere_refused`, `ac1_unknown_key_refused_at_every_level`, `ac1_hex_forms_and_booleans`, `ac1_paths_from_preset_folder_and_home`, `ac1_names_any_case_hyphen_required`, `ac1_header_key_once_sections_repeat` (`reader_tests.cpp`); `test_yaml_options_raw_step_verbs_and_repeated_top_level_keys`, `test_number_forms` (`formats/tests/yaml_tests.cpp`) |
| AC-2 | Pass | `ac2_header_keys_and_defaults`, `ac2_preset_name_at_most_18_written_stem_any_length`, `ac2_place_keys_from_the_descriptor` (`header_tests.cpp`) |
| AC-3 | Pass (library part) | `ac3_peripheral_fits_place_with_variant`, `ac3_module_by_name_reference`, `ac3_place_not_named_is_empty`; `ac3_module_by_file_refused_until_lib7` (refused with a notice, D-22). A module by `file:` passes in M-41 (LIB-7 resolves it) |
| AC-4 | Pass | `ac4_keys_repeats_in_file_order` (`header_tests.cpp`), `ac1_header_key_once_sections_repeat` |
| AC-5 | Pass | `ac5_model_from_model_key_not_extension` |
| AC-6 | Pass | `ac6_first_error_in_file_order_with_line`, `ac6_syntax_error_after_semantic_error_reports_the_earlier`, `ac6_refused_load_leaves_machine_untouched_reply_and_notice` (no busy state, no build, the machine and its ticks unchanged), `ac6_bad_fixtures_each_refused_with_its_error` (`check_tests.cpp`) |
| AC-7 | Pass | `ac7_keys_text_places_and_files_checked_before_build` (a key name, a character, a variant, a missing file, a step that needs the printer: no build); the markers in `ac62_*`, the places of `skip-wait:` and `let-it-finish:` in `ac58_*`, `ac59_*` |
| AC-8 | Pass | `ac8_missing_or_old_version_refused_as_outdated` (no version, version 1, an unknown key before it, a 2.3.0 state file), `ac8_newer_version_refused`; `startup_preset_outdated_gives_default_and_notice` (`startup_tests.cpp`). The text of each product's `formatReference()` is set in M-46 and M-54 (design, open point) |
| AC-9 | Pass | `ac9_unknown_model_lists_the_models` |
| AC-10 | Pass | `ac10_one_machine_from_cold_boot_with_its_places` (one build, the variant, the places, memory from a cold boot, `BuildOrigin::Preset`), `ac10_old_machine_gone_items_start_at_key_readiness` (`build_tests.cpp`) |
| AC-11 | Pass | `ac11_same_preset_twice_same_memory_display_ticks_log` (`determinism_tests.cpp`) |
| AC-12 | Pass | `ac12_busy_live_input_dropped_second_load_refused`, `ac12_reset_cold_boot_switch_end_it_without_error` (a preset that hangs: `Stopped.`, no notice) (`operation_tests.cpp`) |
| AC-13 | Pass | `ac13_in_parts_paced_equals_one_go_harness` (log, reply, test memory, display, ticks) |
| AC-14 | Pass | `ac14_sections_in_file_order_header_anywhere` (`waits_tests.cpp`) |
| AC-15 | Pass (library part) | `ac15_section_finished_when_written`; machine code that starts itself is Calc-U-1600's `program:` (M-46) |
| AC-16 | Pass | `ac16_full_speed_while_built_and_carried_out` (paced: the busy topic says full speed throughout; 2 s of emulated time in under 60 host calls) |
| AC-17 | Pass | `ac17_failure_stops_machine_as_is_reply_notice_line`, `ac57_timeout_fails_the_item_at_both_speeds` (`waits_tests.cpp`) |
| AC-18 | Pass | `ac18_log_lines_file_machine_peripherals_items_end`, `ac18_failure_line_holds_the_error` (`log_tests.cpp`) |
| AC-19 | Pass (library part) | `ac19_key_waits_until_running_frontend_step_goes_on`, `ac19_wait_passes_in_host_time_while_paused` (paced), `ac19_wait_ends_at_once_in_virtual_time`, `ac19_implicit_wait_and_timeout_stand_still_while_paused`, `ac19_preset_that_ends_paused_stays_paused` (`pause_tests.cpp`; the pause by `MachineSession::setPaused`); `host_wait_passes_in_wall_time_while_paused`, `host_wait_ends_at_once_in_virtual_time` (`sessions/tests/operation_tests.cpp`). The debugger steps (LIB-10, M-40): `ac19_no_step_waits_for_a_running_machine` (every debugger step carried out on a machine paused by the run control, in virtual time), `ac13_armed_start_fires_in_operation` (a pause-on-start fired inside a load; a `wait:` then passes in host time and the preset ends paused), `ec7_preset_ends_paused_stays_paused` (`library/debugger/tests/steps_tests.cpp`, `run_control_tests.cpp`) |
| AC-20 | Pass | `ac20_wait_durations_in_emulated_time`, `ac20_wait_without_unit_refused` (`steps_tests.cpp`), `ac56_wait_runs_at_the_speed_in_force` |
| AC-21 | Pass | `ac21_wait_without_value_refused` |
| AC-22 | Pass | `ac22_regular_speed_busy_says_so_last_counts`, `ac22_ends_at_regular_speed_whatever_before`; `ac60_last_item_not_waited_for_regular_speed_after` |
| AC-23 | Pass | `ac23_elapsed_start_report_format_minutes_beyond_59`, `ac23_report_without_start_counts_from_build`; `ac34_elapsed_time` |
| AC-24 | Pass (library part) | `ac24_expect_after_item_finished_by_product_comparison`, `ac24_expect_failure_reason_holds_text_and_display`; each product's comparison in M-46 (C1600-4 AC-13) and M-54 (C59-2 AC-8) |
| AC-25 | Pass (library part) | `ac25_screenshot_into_the_folder`, `ac25_screenshot_kept_by_host_without_folder` (KeepFile), `ac25_name_empty_or_with_separator_refused`, `ac25_unwritable_folder_fails`; `ac34_screenshot_into_the_run_folder`. The save folder of Qt6 and Swift and the Web's download in M-46 and M-54 |
| AC-26 | Pass | `ac26_switch_step_refused`; `ac34_switch_sets_a_control` |
| AC-27 | Pass | `ac27_syncclock_sets_every_clock_and_logs`, `ac27_syncclock_refused_where_not_applies`; `ac34_syncclock_takes_the_run_clock` |
| AC-28 | Pass | `ac28_key_and_type_finish_at_idle`; `lib5_ac14_first_press_after_skip_wait_at_once`, `lib5_ac15_key_name_refused_when_read_before_build`, `lib5_ac17_key_step_finishes_at_idle_command_at_key_up` (`typing_tests.cpp`) |
| AC-29 | Pass | `ac29_header_key_section_step_of_a_product_read_checked_carried_out`, `ac29_their_errors_with_line` (`place_tests.cpp`) |
| AC-30 | Pass | `ac30_unknown_section_or_step_refused` (`partition:` of another product, `card:`, a step outside `keys:`) |
| AC-31 | Pass (library part) | `ac31_bundle_order_groups_startup_files`, `ac31_check_bundle_finds_missing_and_unlisted_files` (`list_tests.cpp`); every distribution built from each product's bundle file in M-46 and M-54 |
| AC-32 | Pass | `ac32_entries_with_bundle_path_model_name_description_kind_group`, `ac32_header_only_entry_naming_missing_file_listed`; `ac34_preset_list` |
| AC-33 | Pass | `ac33_bundle_path_in_load_and_startup_setting` (the load command, `[preset]`, a file named by a bundled preset, a bundle path that leaves the bundle) |
| AC-34 | Pass | `ac34_preset_done_or_refused_with_line`, `ac34_check_preset_one_line_per_file_run_fails`, `ac34_preset_list`, `ac34_wait_until_idle_reports_time_and_hour`, `ac34_full_and_regular_speed`, `ac34_elapsed_time`, `ac34_expect_and_not_expect_refused_with_display`, `ac34_screenshot_into_the_run_folder`, `ac34_printer_trace_refused_without_printer` (the test product's step), `ac34_switch_sets_a_control`, `ac34_syncclock_takes_the_run_clock` (`harness/tests/preset_commands_tests.cpp`); `calcu_cli.ac34_check_preset_exit_status`. Calc-U-59's own `printer-trace` in M-54 |
| AC-52 | Pass | `ac52_machine_refusal_reason_and_key_line_old_machine_kept` (the builder's refusal at the line of `model:`; a refusal that names its place at the line of its key; the machine before kept) |
| AC-53 | Pass (library part) | `ac53_idle_after_build_picture_from_the_rules` (idle, the after-build step and its failure, the picture, all from the rules); each product's in M-46 and M-54 |
| AC-55 | Pass | `ac55_build_only_ends_without_items_or_readiness` (a machine that is never ready: done at tick 0, no step, no after-build step) |
| AC-56 | Pass | `ac56_key_finishes_when_the_program_stops`, `ac56_wait_runs_at_the_speed_in_force`, `ac15_section_finished_when_written` |
| AC-57 | Pass | `ac57_timeout_fails_the_item_at_both_speeds`, `ac57_last_item_and_let_it_finish_not_bounded` |
| AC-58 | Pass | `ac58_skip_wait_next_starts_first_press_at_once`, `ac58_skip_wait_first_or_last_refused`; `lib5_ac14_first_press_after_skip_wait_at_once` |
| AC-59 | Pass | `ac59_let_it_finish_last_waits_until_idle`, `ac59_let_it_finish_elsewhere_refused` |
| AC-60 | Pass | `ac60_last_item_not_waited_for_regular_speed_after` (the program runs on; regular speed after, also after a failure, D-19) |
| AC-61 | Pass | `ac61_speed_command_changes_nothing_while_it_runs` (paced); live keys dropped in `ac12_busy_live_input_dropped_second_load_refused` |
| AC-62 | Pass | `ac62_keyword_not_for_model_refused`, `ac62_undefined_keyword_refused`, `ac2_place_keys_from_the_descriptor`, `ac27_syncclock_refused_where_not_applies` |
| AC-63 | Pass (library part) | `ac63_frontend_step_never_fails_passed_over_without_panel`, `ac19_key_waits_until_running_frontend_step_goes_on`; `debug-panel:` (LIB-10, M-40): `ac19_debug_panel_is_a_frontend_step`, `ac19_each_step_does_what_its_ac_says` (`library/debugger/tests/steps_tests.cpp`) |
| AC-64 | Pass | `ac64_elapsed_is_compute_holds_gaps_waits` (bound of D-9: compute + holds + the wait, at most a gap per press and one idle stretch per waited item more; paced and in virtual time the same line). The bounds against the products' examples come with the reference runs (B-10; M-46, M-54) |
| AC-65 | Pass | `ac65_absent_keyword_default_nothing_carried_over` (the test printer's `trace` switch off, the memory zero) |
| AC-66 | Pass | `ac66_not_expect_passes_and_fails`, `ec7_expect_display_off_fails` (D-14), `ac34_expect_and_not_expect_refused_with_display` |
| EC-1 | Pass | `ec1_unreadable_or_not_utf8_names_the_file` (a file that cannot be read, not UTF-8, a byte-order mark dropped, D-28; reply and notice name the file) |
| EC-2 | Pass | `ec2_header_only_ends_at_key_readiness` |
| EC-3 | Pass | `ec3_stopped_reply_machine_as_the_command_leaves_it`, `ac12_reset_cold_boot_switch_end_it_without_error` |
| EC-4 | Pass | `ec4_let_it_finish_without_limit_ended_by_reset` (a program that never ends, `timeout: 1`, 300 host calls, then Cold boot) |
| EC-5 | Pass | `ec5_content_without_folder_naming_relative_file_refused` |
| EC-6 | Pass | `ec6_suspended_makes_no_progress` |
| EC-7 | Pass | `ec7_expect_display_off_fails` |
| EC-8 | Pass | `ec8_empty_step_list_refused` |
| EC-9 | Pass | `ec9_missing_entry_left_out_and_logged` (a missing preset and an outdated one left out, a log line each; `checkBundle` fails on it) |
| EC-10 | Pass | `ec10_no_key_readiness_fails_before_any_item` |

The suite check (L14, the mechanism of AC-43 and AC-51):
`calcu_preset_check.LibraryPresetFixtures` passes (`calcu_test
check-preset` over `fixtures/good/*` and `fill/template.test-seg.in` filled
in by `fixtures/fill.cmake`). LIB-18's start with the real loader (L15):
`startup_preset_built_by_the_loader`,
`startup_preset_outdated_gives_default_and_notice`,
`startup_preset_failing_while_run_keeps_machine` (`startup_tests.cpp`).

Column status: every criterion and edge case of the column passes in its
library part. Clauses that name a feature built later pass in its step
(design §1, D-29): AC-3's module by `file:` (M-41, LIB-7), AC-19's
debugger steps and AC-63's `debug-panel:` (M-40, LIB-10; pass since M-40,
see the rows above); the products'
clauses of AC-24, AC-25, AC-31, AC-34 and AC-53 are tested again with the
product rows (M-46, M-54). The Library column stays **In progress**; it
becomes Done in M-41 (D-29).

### The clauses that passed in M-41 (LIB-7; design §1, D-29)

Checked on 2026-11-15 on branch `m41-media` (`calcu_library_tests` 729
tests, 0 failed):

| ID | Clause | Result | Test case |
|---|---|---|---|
| AC-3 | a module by `file:` | Pass (M-41) | `lib6_ac3_module_by_file_reference` (`library/media/tests/preset_media_tests.cpp`: `slot-1: { file: … }` resolved by LIB-7's resolver, `Vocabulary::setPlaceResolver`; a file that is no module medium refused at its line); without a resolver `file:` is refused as before: `ac3_module_by_file_refused_without_a_resolver` (renamed from `ac3_module_by_file_refused_until_lib7`, `header_tests.cpp`) |
| (bundle.md §3) | `checkBundle` accepts a file a preset names that is a bundled medium or module of the product | Pass (M-41) | `lib6_check_bundle_accepts_bundled_media` (same file; `Vocabulary::setBundledMedium`) |

`PlaceKeyRule::byFile` is on by default now; a product narrows a place
with `placeKey()` as before. A step on its own (`run-step`, the harness's
step commands) reads the file its reference names, absolute or relative to
the current folder (LIB-11 AC-22; it was refused). The column stays **In
progress**: AC-19's debugger steps and AC-63's `debug-panel:` pass with
LIB-10 in M-40 (D-29).

### Exit check of M-38

Library tests with sections built for the tests: the 105 cases of
`library/presets/tests/`, the 11 harness cases and the two CTest entries
above pass. `calcu --product 59|1600 preset …` is refused with "This build
reads no presets of format 2 yet." (no product returns rules before M-46,
M-54); today's tools read the old formats as before.

### Golden runs

M-38 touches no core and no product code; both products' golden replays
pass unchanged in their Frequent suites (`CalcU59Tests`, `CoreTests`).

### Deviations from the design, chosen by the simpler structure

- **`hostWait` needs a host wait in the job (D-17).** LIB-3's job could only
  wait for machine calls, and the session steps an operation only while
  the machine runs. Added: `yieldToHost()` (a pending wait for the host's
  next call, `Pending::host`), `Job::waitsOnHost()`; a slice ends at such a
  wait; `AppSession` steps an operation that waits on the host while the
  machine is paused; `SessionContext::wallTime()` and `virtualTime()`. A
  host wait gives the host at least one call (a loop of short waits on a
  clock that moves between two readings never gave it back; second
  `sessions` commit).
- **The reader parses each top-level item on its own** (D-3); the option
  `YamlOptions::repeatTopLevelKeys` exists but the reader does not need it.
  `YamlNode::rawLine` is a flag on the scalar. Comment lines at column 0
  between top-level items belong to the item before (a tab in one is
  reported at its line).
- **Load plan.** `LoadPlan::items` are `PlannedItem {item, skipWaitAfter}`
  with `LoadPlan::letItFinish`, and `keys:` is the reader's own section
  rather than a `SectionDef`. A keyword's reader reports its errors through
  `ReadContext::error` and returns no item. `PresetSource::readError()` and
  `readStep()` (one step against the running machine, run-step) are new;
  `readHeader` takes `const ProductRules&`.
- **`ItemResult::result`**: the text a step gives back (`elapsed-time
  report`), for the harness reply.
- **`run-step`**: a step that needs no running machine and has finished
  when it took effect (`elapsed-time`, `expect`, `not-expect`,
  `screenshot`, `syncclock`, `printer-trace`) is carried out inside the
  command and replies with its result; the others start an operation. So
  `expect` refused in the harness gives no notice.
- **`wait-idle` and the bare `wait`**: an operation's reply carries no
  payload (LIB-3), so the harness reads the ticks before and after and
  reports `hour reached` itself.
- **`check-preset`**: the request is refused with the first error, given
  `{ok: true}` otherwise. `Reply::failsRun` (new) writes the reply as done,
  one line per file, and ends the run as failed.
- **The display of `expect:` and `screenshot:`** is read from the machine
  (`Machine::readDisplay`) at the moment of the step: the `display` topic
  is refreshed only after a host call, in virtual time after the whole
  operation, so it would show the display from before the load. This is a
  second reader of the display beside the session (LIB-1 §3.5); on a core
  whose brightness is a duty since the last reading, the session's next
  window starts at the step.
- **Engine dependencies**: `EngineDeps` (machine control, typing part,
  screenshot folder, the `elapsed-time` baseline) held by `PresetsPart`
  for the running operation; the baseline is kept with the machine's
  generation (`ElapsedBase`), so a `start` taken in a preset counts on for
  a later `elapsed-time report` command on the same machine.
- **`set-control {peripheral, place?, control, on}`**: the owner by its
  peripheral ID (any case) and, when given, its place.
- **`HarnessProduct::typingPart()`** (new): the loader needs LIB-5's part,
  which the harness cannot find among `sessionParts()`. The harness wiring
  is in `Run.cpp` only; `HarnessHost.cpp` is unchanged.
- **AC-52**: a builder's refusal names no place, so it is given at the
  line of `model:`; a refusal of `MachineControl`'s own checks names its
  place and is given at the line of its key.
- **`checkBundle`** does not yet know a product's bundled media and modules
  (bundle.md §3, "or is a bundled medium or module"); none exists before
  LIB-7.
- **`calcu --help`** lists the preset commands with the library's fixed
  list of steps (a product's own steps appear in a run, not in the help).
- **Test support**: `TestSegProgram` is also set by
  `TestMachine::setProgram()` (the rig's builder sets it on every test-seg);
  the memory is a product kind `test.memory`, offered only with the
  program. `TestPresetRules.hpp` is header-only; the rig is
  `PresetRig.hpp` (not in the design's file list).
- **Wording** (not part of the contract, AC-18, D-23): the log names the
  preset by its path or bundle path; value-less steps are logged without
  a colon (`line 5 regular-speed: done`). `screenshot:` writes under the
  name as given (no extension added).
