# LIB-11: QA report

Results per acceptance criterion and edge case of `spec.md`, per column.

## Library (M-28, 2026-11-15, branch `m28-harness`)

Built and run on an M4 Pro, macOS 27.2, Apple clang, CMake, Ninja,
RelWithDebInfo (`build/tests`, Qt off), and in the library-only tree
(`-DCALCU_PRODUCT_59=OFF -DCALCU_PRODUCT_1600=OFF`). The code is
`library/harness/` (`calcu::harness`) and `library/formats/Flow.*`,
`Duration.*`; the program `calcu` and its test form `calcu_test` are in
`library/harness/cli/`. The library cases are in `library/harness/tests/`
and `library/formats/tests/formats_tests.cpp` (`calcu_library_tests`); they
run the product made for the tests (`TestProduct.*`: Calc-U-Test, `test`,
models `test-seg` with the machine state and `test-dot`, the test font and
segment rule, the topic `test.count`, the commands `test mapping`,
`test operation`, `test fail`, `test count`). The process cases are CTest
entries `calcu_cli.<case>` (`library/harness/tests/cli/cli_cases.cmake`
with `calcu_test`). The sources of `library/harness/` and the new
`library/formats/` files also compile with Emscripten (`em++ -std=c++20
-fwasm-exceptions -Wall -Wextra`).

Result: `calcu_library_tests` 292 `TEST`s (52 of them LIB-11's), 2,505
checks passed, 0 failed, 0.64 s; `calcu_cli.*` 6 of 6 and
`calcu_serve_python` pass. The whole Frequent group passes (20 of 20 CTest
entries, 39 s wall time with `-j8`; Calc-U-1600's `CoreTests` 41 s).

| ID | Result | Test case |
|---|---|---|
| AC-1 | Pass | `calcu_cli.ac1_product_by_name_or_short_form_any_case` (`Calc-U-Test`, `calc-u-test`, `TEST`, `test`); the products register through `calcu_harness_add_product` and the deferred table (`calcu --product 59`, `--product 1600` in one program) |
| AC-2 | Pass | `ac2_commands_part_on_then` (`protocol_tests.cpp`); `calcu_cli.ac2_one_shot_runs_in_order_and_exits_zero`, `calcu_cli.ac2_first_refusal_exits_nonzero_with_reason` (`wait 1s then test fail then ticks`: exit 1, the reason, `ticks` not run). The clause "the machine the product starts with (LIB-18)" passes in M-35; until then the product's start (adapter A-1) |
| AC-3 | Pass | `calcu_cli.ac3_serve_one_reply_per_line_until_quit` (a `\r\n` line, an empty line, a refused line, `quit`, a line after it) |
| AC-4 | Pass | `ac4_plain_reply_is_lines`, `ac4_json_reply_has_the_three_keys_for_every_command` (every registered command, done and refused, one line each) |
| AC-5 | Pass | `ac5_refusal_carries_the_reason`, `ac5_line_that_is_no_command_is_refused` (and serve goes on), `ac5_notices_only_through_state` |
| AC-6 | Pass | `ac6_two_runs_give_the_same_replies` (18 commands, plain and `--json`, 30 ms of wall time between the runs, byte for byte), `ac6_no_pacer_and_no_wall_clock` (wall time passing moves nothing; the session is hosted in virtual time and its wall clock, which stands at 0, is not read by any command) |
| AC-7 | Pass | `ac7_operation_runs_to_its_end_before_the_reply` (2 s operation; busy idle again before the reply) |
| AC-8 | Pass (the harness side) | `ac8_defaults_and_no_settings_file_read_or_written`, `ac8_run_folder_empty_removed_and_in_no_reply`, `ac8_traces_and_screenshots_go_to_the_run_folder`, `ac8_options_name_settings_save_and_output_folders`, `ac8_bundled_roms_through_the_lookup`. Bundled media and presets, and the traces and screenshots themselves, pass in M-41, M-38, M-40 (design §1) |
| AC-9 | Pass | `ac9_wait_reports_time_passed_within_one_instruction`, `ac9_key_down_and_up`, `ac9_unknown_key_is_refused`, `ac9_ticks_gives_count_and_seconds`, `ac9_capabilities_with_kind_id_owner_and_places`, `ac9_quit_ends_serve`, `ac9_lcd_reads_display_topic`, `ac9_audio_records_all_sound_also_of_an_operation`. `wait` ending at once while the debugger pauses passes in M-40 |
| AC-10 | Pass | `ac10_dot_matrix_rows_indicators_and_cells_not_text`, `ac10_inverted_glyph_is_text`, `ac10_segment_positions_left_to_right_by_product_rule`, `ac10_display_off_says_so` |
| AC-11 | Pass (library and product topics) | `ac11_state_reads_library_and_product_topics` (`display`, `busy`, `notices`, `log`, `test.count`), `ac11_stream_gives_what_was_appended_since_last_read`. Other features' topics pass in their steps |
| AC-12 | Pass (the form with a duration) | `ac12_wait_takes_the_steps_duration_forms` (`formats_tests.cpp`); the bare `wait` passes in M-38 (LIB-6 AC-34) |
| AC-13 | Pass | `ac13_product_command_under_product_word`, `ac13_common_command_at_top_level` |
| AC-21 | Pass (build, operations, the option) | `ac21_fixed_instant_by_default_at_build_and_after_operation`, `ac21_clock_option_names_instant`, `ac21_clock_host_hands_host_time`; `syncclock:` passes in M-38 |
| AC-22 | Pass (with a test product command) | `ac22_flow_mapping_as_in_a_preset` (the three examples of the spec), `ac22_step_becomes_command_by_dropping_dash_and_colon`; the real mapping commands pass in M-41 |
| AC-23 | Pass | `ac23_dot_matrix_rows_of_ones_and_zeros`, `ac23_segment_positions`, `ac23_indicator_bits_in_descriptor_order`, `ac23_display_off`, `ac23_json_arrays` |
| AC-24 | Pass (library; test machine) | `ac24_restore_continues_identically` (same run and another run), `ac24_restore_refused_for_other_configuration_changes_nothing` (another version, another model in the header, not a state), `ac24_refused_without_the_kind`, `ac24_host_files_not_in_state`. The products offer the kind with their own steps |
| EC-1 | Pass | `ec1_missing_rom_ends_at_start_naming_files` (two files named; `calcu` exits 2 with the message on standard error) |
| EC-2 | Pass | `ec2_duration_without_unit_is_refused` (`formats_tests.cpp`), `ec2_wait_without_unit_is_refused` |
| EC-3 | Pass | `calcu_cli.ec3_serve_ends_at_end_of_input` (WAV header completed), `ec3_wav_complete_when_input_ends` (two `audio` files, D-8) |
| EC-4 | Pass | `calcu_cli.ec4_unknown_product_lists_products` (an unknown product and no `--product`, D-5) |
| EC-5 | Pass | `ec5_state_of_no_topic_is_refused` |
| EC-6 | Pass | `ec6_audio_on_machine_without_sound_is_refused`; on a TI-59 in `harness_drives_a_ti59` |
| EC-7 | Pass | `ec7_repeated_key_down_or_up_is_done_and_changes_nothing` |

The Python helper (D-2, no AC): `calcu_serve_python` passes
(`library/harness/python/tests/test_calcu_serve.py`, JSON and plain).

### Exit check of M-28

`calcu` drives a TI-59 and a Sharp machine headless:
`harness_drives_a_ti59` (`CalcU59Tests`; `wait`, `key-down`/`key-up 7`,
`lcd` shows `          7 ` as `ti59.py`'s `display()` does, `ticks`) and
`harness_drives_a_sharp_machine` (`CoreTests`; the PC-1500A of the default
start, D-1: `NEW0? :CHECK` read from the dots with the ROM font, answered
with keys, `display`). The PC-1600 follows in M-35 (model switch, LIB-18).
By hand: `calcu --product 1600 wait 2s then lcd` shows `NEW0? :CHECK`,
`indicators: DE G PRO I`, `not text: 0`.

### Golden runs

Both products' golden replays pass unchanged in their Frequent suites
(`golden_runs_tests.cpp` of `CalcU59Tests` and `CoreTests`). M-28 touches
no core; no reference was re-recorded.

### Deviations from the design, chosen by the simpler structure

- `HarnessProduct` (§3.6): `lookUpRoms()` instead of `bundle()` (the
  product's own lookup: Calc-U-59 needs its text reader), `startupRequest`
  also takes the `RomSet`, `startUp(Machine&)` (Calc-U-1600's builder runs
  nothing, so A-1 makes the first cold boot), `version()` (for AC-24's
  header), and `sessionParts()` instead of `addToSession(AppSession&)`
  (LIB-3 takes its parts at construction). The Calc-U-1600 font (A-4) is
  read by such a part when the machine changes.
- `Protocol.cpp` is part of `Commands.cpp`; `StatePart.*` and
  `MachineState.hpp` are new files (AC-24, added to the design on
  2026-11-14 without files). The generic kind `machine-state` lives in
  `calcu::harness`, the feature that builds it (LIB-1 design §2a); the test
  machines offer it only with `offerState(true)`, so the other areas' lists
  stay as they are.
- The product entries are targets of their own, `calcu59_harness` and
  `calcu1600_harness` (`app/HarnessProduct.cpp`), not part of
  `calcu59_shared` and `calcu_shared`, so the Swift, Qt6 and Web builds
  carry none of them and no source path is compiled into them.
- `FirstCommands.cpp` reads and writes the files of `audio`, `state-save`
  and `state-restore` itself (`WavRecorder` too); §8 wants file access in
  `Run.cpp` only. Standard C++ only, it compiles for Emscripten and iOS.
- Calc-U-1600's ROMs: until M-27 the entry runs the library's lookup over
  the folders the tests use (`roms/`, `firmware/pc1600-hostdrive/`) and the
  catalogue without module definitions for the full set.
- After `key-down` the display topic shows the change at the next `wait`
  (LIB-3 reads the display when time passes), as in today's frontends.
- The first read of a stream topic carries no reset marker from the
  listen's replay.
- `version()` gives the pin's version from each product's `CHANGELOG.md`
  (2.3.0, 0.8.1); there is no version in the code yet. Open for the user.
