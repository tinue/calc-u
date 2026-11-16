# C1600-2: QA report

Results per acceptance criterion and edge case of `spec.md`.

## Calc-U-1600 (M-44, 2026-11-16, branch `m44-program-files`)

Built and run on an M4 Pro, macOS 27.2, Apple clang, CMake, Ninja,
RelWithDebInfo, in two trees: `build/tests` (`-DCALCU_FRONTEND_QT=OFF`;
Frequent: 32 of 32 entries pass) and `build/qt` (`-DCALCU_FRONTEND_QT=ON`;
34 of 34 pass). The Web client with Homebrew Emscripten
(`frontends/web/build.sh`) and Playwright (`frontends/web/run-tests.sh`,
Chromium and WebKit).

The code:
- `core/Machine/SharpProductKinds.hpp`: `sharp::LoaderMemory` in the shape
  of AC-14 (`peek` per processor, `mode`, `programArea`, `writeBuiltIn`,
  `writeSlot`, `slotRam`, `module`, `internalRam`; every write all or
  nothing); `core/Machine/SharpLoaderMemory.*` implements it over the core
  (`PC1500LoaderMemory`, `PC1600LoaderMemory`), listed by both interface
  machines and made by the format-1 preset runner over its core;
- `core/Loaders.*` (`sharp::loaders`): `fileKind`, `image`, `loadBasic`,
  `planCode` (the one plan: `machinecode::planLoad` with the state read
  from the kind), `cpuNote`, `where`, `loadCode`, `refusal`; on the kind
  and on a `calcu::machine::Machine`;
- `core/BasicTyping.*`: `sharp::loaders::typeProgram` over the typing
  engine inside an operation;
- `core/ProgramFile.*`: `programfile::refusal` (one sentence per kind, for
  BASIC and for machine code), the header's name; `core/MachineCodeFile.*`:
  `planLoad` in the order of AC-8's table, the range always checked,
  `machinecode::refusal`; `plan()`, `Plan`, `OutsideBank0`,
  `acceptLengthMismatch`, `checkRange` gone;
- the per-model writers on the kind: `pc1500LoadBasicPayload`,
  `pc1600LoadBasicPayload`, `loadPC1500MachineCode` (all or nothing, OQ-2),
  `loadPC1600MachineCode`, `pc1600LoadState`, `pc1600SlotGeometry`,
  `pc1600BasicAreas`, `pc1500UserRam`;
- the format-1 presets (`core/Preset/PresetRunner.*`, both preset loaders)
  and the debugger's Build & Load (`core/Debug/ProgramLoader.*`) call
  `sharp::loaders`; `PresetMachine` lost `typeBasicProgram`,
  `loadBasicFile`, `codeTarget`, `codeState`, `loadMachineCode` and got
  `loaderMemory()`; `legacy::applyPresetJob` takes the typing hand-off;
  the adapters `legacy::loadBasicProgram`, `loadMachineCode`,
  `machineCodeLoadState`, `userRam` are gone;
- `app/SharpLoaderPart.*` (the commands and requests of design §3.7) in
  `SharpAppPart::sessionParts()` and in the harness's; `app/SharpTopics.hpp`
  with `FileKindReply`, `CodePlanReply`, `CodeLoadedReply`;
  `app/MachineCodeLoad.*` and the old `c1600.load-basic {bytes, path}`,
  `c1600.load-machine-code`, and the Web page's stateful `c1600.open-program`,
  `load-basic-picked`, `plan-machine-code`, `check-address`,
  `load-planned-machine-code` are gone;
- `app/HarnessProduct.cpp`: `pc1600 file-kind`, `load-basic`, `type-basic`,
  `load-code`;
- Qt6 `MainWindow` and `MachineCodeLoadDialog`, Web `engine.js`,
  `CalcU1600.jsx`, `Dialogs.jsx` on the new commands, in today's menus.

| ID | Result | Test (file: case) |
|---|---|---|
| AC-1 | Pass | `tests/program_files_tests.cpp`: `c1600_2_ac1_every_kind_by_content_not_name`, `_headerless_looks_like_lh5801_or_z80` (the Z-80 guess and no guess; SDE has no LH5801 sample in the tests, the LH5801 branch is the same mapping), `_other_with_words`; the classifier's own cases stay in `program_file_tests.cpp` |
| AC-2 | Pass | `program_files_tests.cpp`: `c1600_2_ac2_ce158_header_fields`, `_pc1600_header_fields_with_bank`, `_start_zero_or_ffff_none` |
| AC-3 | Pass | `program_files_tests.cpp`: `c1600_2_ac3_wav_files_in_tape_order_with_names`, `_wav_none_readable_is_other_with_words` |
| AC-4 | Pass | `basic_fastloader_tests.cpp`: `c1600_2_ac4_same_memory_as_typed_pc1500`, `c1600_2_ac4_nothing_else_changes_pc1500`; `pc1600_basicloader_tests.cpp`: `c1600_2_ac4_same_memory_as_typed_pc1600` (the work area of typed and loaded programs, stock, CE-155, S0 across banks, S1, MODE 1 cases), `c1600_2_ac4_nothing_else_changes_mode_area_kept` |
| AC-5 | Pass | `pc1600_basicloader_tests.cpp`: `c1600_2_ac5_listing_tokenized_per_model_and_mode`, `_mode1_non_ascii_refused`, `_tokenized_file_per_machine_and_mode`, `test_pc1600_bbin_file`; `basic_fastloader_tests.cpp`: `c1600_2_ac5_pc1500_refuses_pc1600_file` |
| AC-6 | Pass | `basic_fastloader_tests.cpp`: `c1600_2_ac6_pc1500_program_start`, `c1600_2_ac6_refused_room_nothing_written_pc1500`, `c1600_2_ac6_refused_names_pointer_pc1500`; `pc1600_basicloader_tests.cpp`: `c1600_2_ac6_s0_parts_in_table_order_line_not_split`, `c1600_2_ac6_s1_s2_in_module`, `c1600_2_ac6_refused_names_pointer_or_room_nothing_written`, `c1600_2_ac6_slot_without_program_module_refused` |
| AC-7 | Pass | `tests/basic_typing_tests.cpp` (a session with the typing part and the loader part, `SessionFixture`): `c1600_2_ac7_lines_typed_with_enter_by_engine_pc1500`, `_pc1600`, `_memory_equals_old_typer` (both models), `_empty_line_left_out`, `_char_without_key_refuses_before_typing`, `_waits_prompt_then_end_stable`, `_line_over_79_not_typed_pc1600`, `_names_lines_not_stored`, `_no_line_stored_fails_with_pro_hint`, `_mode_not_pressed`, `_preset_typed_program_through_engine` |
| AC-8 | Pass | `machine_code_file_tests.cpp`: `c1600_2_ac8_row1_not_code_basic_hint` to `_row10_rom_or_module_where_instead` (one per row), `_rows_checked_in_order`, `_same_plan_menu_preset_debugger_harness` |
| AC-9 | Pass | `machine_code_file_tests.cpp`: `c1600_2_ac9_cpu_by_header_caller_mode`, `_addresses_in_cpu_space` |
| AC-10 | Pass | `machine_code_file_tests.cpp`: `c1600_2_ac10_written_any_bank_switched_in`, `_not_ram_nothing_written_pc1500`, `_not_ram_nothing_written_pc1600`, `_crossing_bfff_both_checked` |
| AC-11 | Pass | `machine_code_file_tests.cpp`: `c1600_2_ac11_parts_of_reply`, one case per row of the NEW table (`_new_pc1500_inside_basic`, `_new_pc1500_outside_none_needed`, `_new_selected_s1_s2`, `_new_s0_first_part_with_note`, `_new_later_part_or_above_warning`, `_new_elsewhere_not_used`), `_call_forms_call_callbank_xcall`, `_start_address_header_else_first_byte` |
| AC-12 | Pass | `machine_code_file_tests.cpp`: `c1600_2_ac12_work_area_warning_loaded` |
| AC-13 | Pass | `app/tests/loader_part_tests.cpp`: `c1600_2_ac13_run_types_start_and_waits_idle`, `_without_start_address_nothing_started`, `_bound_reached_done_with_log_line` (the bound set to 2 s, D-5). Within a preset the auto-run keeps today's `typeLine` and `waitUntilBasicIdle` with its hour until M-46 (design §3.6) |
| AC-14 | Pass | `tests/loader_memory_tests.cpp`: `c1600_2_ac14_listed_by_every_sharp_model`, `_peek_without_effect`, `_peek_lh5803_view_pc1600`, `_mode_and_area_pc1600`, `_write_built_in_and_slot_any_bank_switched_in`, `_slot_ram_and_module_size_banks`, `_write_not_stored_says_so_writes_nothing` |
| AC-15 | Pass (M-49) | The two items in the File menu of the LIB-14 shell, group FileKinds (`app/SharpShellProduct`): Qt6 `CalcU1600QtTests` `c1600_2_ac15_file_items_and_filters` (the items, the dialogs' filters `BASIC Programs (*.bas *.bbin *.wav)`, `Machine Code (*.bin *.wav)`, the start folder of `[folders]` remembered by kind), `lib14_ac34_qt_menus_in_family_order` (their place); Web `shell.spec.js` `c1600_2_ac15 the file items` |
| AC-16 | Pass | Web: `C1600-2 AC-16 a WAV with several programs asks which one`; the list from `c1600.file-kind` (`loader_part_tests.cpp`: `c1600_2_ac22_two_files_listed`). Qt6: the same flow through `QInputDialog` (built; scenario in M-49, AC-20) |
| AC-17 | Pass | Web: `C1600-2 AC-17 a BASIC program is loaded, a refusal is a notice`; Qt6: `Operations::run` shows the refusal (built) |
| AC-18 | Pass | Web: `C1600-2 AC-18 headerless code asks the address, then NEW and CALL`; the plan per text (`loader_part_tests.cpp`: `c1600_2_ac22_address_checked_as_typed`); Qt6: `MachineCodeLoadDialog` asks `c1600.plan-code` on each change (built) |
| AC-19 | Pass | Web: `C1600-2 AC-19 the loaded dialog and the CALL in the input line, not started`; Qt6: `showMachineCodeLoaded` then `pasteText` (built) |
| AC-20 | Pass (M-49) | Web: the two scenarios carry their IDs (`C1600-2 AC-18 …`, `C1600-2 AC-16 …`). Qt6 (`CalcU1600QtTests`): `c1600_2_ac16_qt_wav_with_several_programs_asks_which_one` (a WAV with two files: the dialog lists two), `c1600_2_ac18_qt_headerless_code_asks_the_address` (a headerless `.bin`: the address dialog). They drive the real window's File items with the file dialog replaced, not attach: a dialog of the flow is a UI step, and nothing of the machine is stepped |
| AC-21 | Pass | `tests/harness_tests.cpp`: `c1600_2_ac21_listed_under_pc1600` (and `--help`), `_file_kind`, `_load_basic_with_n`, `_type_basic_lines_not_stored`, `_load_code_options_and_run` |
| AC-22 | Pass | The ported files pass: `program_file_tests.cpp`, `basic_fastloader_tests.cpp`, `pc1600_basicloader_tests.cpp`, `machine_code_file_tests.cpp`, `presetloader_trace_tests.cpp`; 0.8.1's app session cases in `loader_part_tests.cpp`: `c1600_2_ac22_one_wav_one_image`, `_two_files_listed`, `_address_checked_as_typed` |
| AC-23 | Pass | `machine_code_file_tests.cpp`: `c1600_2_ac23_address_by_hand_else_header`, `_length_by_hand_else_payload`, `_bank_from_header_or_area`, `_headerless_no_bank` |
| AC-24 | Pass | `loader_part_tests.cpp`: `c1600_2_ac24_load_at_once_not_operations`, `_type_and_run_are_operations`, `_refused_while_another_runs` |
| AC-25 | Pass | `loader_part_tests.cpp`: `c1600_2_ac25_same_sentence_menu_harness_preset` (the menu's command, `pc1600 load-code`, and a format-1 preset give the same sentence; the preset's has `section 1/1: ` before it) |
| EC-1 | Pass | `program_files_tests.cpp`: `c1600_2_ec1_empty_refused_as_empty_by_every_loader` |
| EC-2 | Pass | `program_files_tests.cpp`: `c1600_2_ec2_partial_header_refused_saying_so` |
| EC-3 | Pass | `basic_fastloader_tests.cpp`: `c1600_2_ec3_new0_check_hint_to_run_new`; `pc1600_basicloader_tests.cpp`: `c1600_2_ec3_uninitialised_pointer_hint_pc1600` |
| EC-4 | Pass | `program_files_tests.cpp`: `c1600_2_ec4_wav_first_file_by_default`; a preset reads a WAV's first file (`PresetRunner.cpp`, `readProgramFile`) |
| EC-5 | Pass | `basic_fastloader_tests.cpp`: `c1600_2_ec5_code_to_basic_loader_hint` |
| EC-6 | Pass | `loader_part_tests.cpp`: `c1600_2_ec6_menu_and_harness_refused_while_busy` |

Column status: Done (M-49, 2026-11-16, branch `m49-shell-1600`: AC-15
and the Qt6 half of AC-20 pass; until then In progress).

### Exit check: every loader through the harness

By hand with `calcu --product 1600` (the program of `build/tests`), besides
the `c1600_2_ac21_*` cases: on the PC-1500A `pc1600 file-kind` on a `.bbin`,
a WAV and a headerless file; `key cl then type NEW0 then pc1600 load-basic`
of a listing; `pc1600 load-code` refused without an address and loaded
with `address=7C01`; `type-basic` with a line not stored (named in the
refusal); on the PC-1600 (`model PC-1600`, `key mode`, `type NEW`)
`type-basic` of a listing, `load-code` with `address=`, `length=`,
`cpu=lh5803`, into the work area (a `warning:` line) and `run` on a header
with an address to start at (`started: yes`, the `CALL` on the display).
Every reply was as expected.

### Reference runs and the loader matrix (R1, design §8)

- The 32 examples of `tests/reference/v0.8.1-examples/run.sh`, run with
  the CLIs of this tree: all 32 `.out` files equal the references (the
  copy's path normalised to `<copy>`); DiskWorks needs `firmware/` next to
  the CLIs for the host-drive ROM, as in the product. The CLIs run presets
  without a session, so a typed `program:` there still uses the old typer
  (Deviations, 1); the new typer's memory is compared with the old one's
  in `c1600_2_ac7_memory_equals_old_typer` and through a preset in the
  session (`c1600_2_ac7_preset_typed_program_through_engine`).
- `dev/loader-matrix/run_pc1500.sh`: 72 of 80 pass; the 8 that fail are
  the known machine-code OVER cells (the loader refuses a file that does
  not fit, the ROM writes on).
- `dev/loader-matrix/run_pc1600.sh`: 250 of 250 pass.

### Golden runs

`golden_runs_tests.cpp` passes unchanged (in CoreTests); no reference was
recorded again. No difference.

### Deviations

1. **The old typer stays for presets run without a session.** A typed
   `program:` goes through the new typer whenever the preset runs in the
   app session (Qt6, Web, the tests' `SessionFixture`; the hand-off of
   `legacy::applyPresetJob`). `pc1500_cli`, `pc1600_cli`, the probes and
   the loader matrices run presets with `job::runToCompletion` and have no
   typing engine; for them `PresetMachine::typeBasicProgramWithoutSession`
   keeps `typeBasicProgramTextJob` until M-46, when they load presets
   through the library's loader. So `typeBasicProgramTextJob` of both
   typers does not go in M-44 (design §5 lists it as removed here); the
   plan's M-44 text allows the old typer for presets until M-46.
2. **`type-basic` replies with the lines in its reason.** A command that
   fails has a reason and no result, so the lines not stored are named in
   the sentence (`<file>: 2 program line(s) not stored (…): "A=2", "B=3".`)
   instead of `not stored:` lines (design §3.7's table).
3. **`type-basic` takes text.** It takes a BASIC listing or what the
   classifier calls plain text (a line without a number is then typed and
   reported as not stored); anything else is refused ("Only a BASIC
   listing (text) can be typed in."). The spec does not say.
4. **Sentences that the menu did not have** were written new, in the
   menu's style: the address needed (row 5: "The file has no machine-code
   header, so it needs an address to load at." with the proposal on a
   PC-1600), the length given by hand (row 6), the LH5803 range for an
   address by hand ("The load address &X + n bytes is outside the LH5803's
   RAM (&0000-&7FFF)."). A length that disagrees with the header names both
   numbers, also when fewer bytes follow ("…says n bytes of code, but m
   follow it."; the menu said "promises more code than the m bytes"). A
   preset's hint "Add an explicit 'length:' to override." is gone (AC-25:
   the same sentence). BASIC refusals start with a capital and end with a
   stop.
5. **`load-code … run` loads at once and then starts the operation.**
   The load is done in the command's handler (refused while another
   operation runs, AC-24); only the start (typing the command and waiting
   for idle) is the operation. Without an address to start at it replies
   at once with `started: no`.
6. **The harness's `file-kind`** prints `load:` and `start:` for machine
   code only (BASIC headers have none), and `looks-like:` and `damaged:`
   lines besides those of design §3.7. `load-code` adds `started: yes|no`.
   The work-area warning of AC-12 is a `warning:` line and stays in the
   NEW note as today (`machinecode::advice` unchanged), so the harness
   shows it twice; the dialogs show the note only.
7. **Test files:** `basic_typing_tests.cpp` lies in `tests/`, not
   `app/tests/` (it builds core machines for the old typer's memory, which
   `CalcU1600MachineIncludeCheck` forbids in `app/`). The old typer's
   memory is computed in the test, not kept as a fixture file
   (`tests/fixtures/typed-*.bin`): the old typer still exists (deviation 1).
   `program_file_tests.cpp` keeps its classifier and drop cases;
   `program_files_tests.cpp` holds the criteria's cases.
8. **The address dialog says where the code goes** for every address that
   holds, the internal RAM too (today only for a module or the work area):
   ledger row 1600-38.
9. **`typeBasicProgramTextJob` outlived M-46** (deviation 1): after the
   CLIs went (M-51) its only users are tests (`c1600_2_ac7_memory_equals_old_typer`
   and the golden runs). Part D (P-4, `ds-core`) moved both old typers to
   `tests/support/`, out of `calcu_core`; their behaviour is unchanged.

### Findings for the user

- None that block. Playwright: 61 passed, 2 skipped (the ROM-missing
  build and the one-minute pacing case, as before), the five `C1600-2`
  scenarios in both projects (root, sub-path); `tools/build_wasm_check.sh`
  passes.
- The Qt6 menu flows were built, not run by hand (no GUI launched by the
  agent); their scenarios come with M-49 (AC-20).
- An address given with `cpu=lh5803` on a PC-1600 in MODE 0 (only the
  debugger and the harness name the processor) gets today's advice for
  the Z-80 address it maps to; for LH5803 &4000 (Z-80 &C000) that is the
  warning about the system's reserve. Not changed (`advice` as today).
