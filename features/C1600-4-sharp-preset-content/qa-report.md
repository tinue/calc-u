# QA report: C1600-4 Sharp preset content

Results per acceptance criterion and edge case of `spec.md`.

## M-46 (2026-11-16, branch `m46-presets-1600`)

Built and run on an M4 Pro (macOS 27.2, Apple clang, Ninja,
RelWithDebInfo): `build/tests` (Frequent 34 of 34, `CoreTests` 51 s),
`build/qt` (Qt6, `-DCALCU_TESTS_LONGER=ON`: `CalcU1600ExampleReferences`
passed, 81 s), the Web `dist/` and `frontends/web/run-tests.sh`.

The code (design §3): `app/SharpPresetKeys.*` (`program:` with `file:` and
`text:`, `host-drive:`, `slot-1-rom:`/`slot-2-rom:`, `bus-rom:`, `debug:`;
`checkSharpHeader`, `amendSharpPlan` (gone in Part D simplification P-6: the header keys add their requests through `ReadContext`, LIB-6 D-33)), `app/SharpDebugBlock.*`
(`readDebugBlock`, `debugBlockOf` for the DAP), the bus ROM in the core
(`kBusRom`, `kBusRomPlace`, `BusRomRequest`, `SlotRomRequest`), the 51
presets of the repository, the templates of the VS Code extension and
`examples/bundle.yaml` upgraded to version 2. Tests:
`tests/preset_content_tests.cpp`, `tests/example_references_tests.cpp`.

| ID | Result | Test case |
|---|---|---|
| AC-1 | Pass | `c1600_4_ac1_sections_in_file_order_any_number`, `c1600_4_ac1_fields_once_one_of_file_text`, `c1600_4_ac1_text_not_with_typed_address_length` |
| AC-2 | Pass | `c1600_4_ac2_kind_told_at_read_field_it_does_not_take_refused`, `c1600_4_ac2_unreadable_empty_not_a_program_refused`, `c1600_4_ac2_tape_definition_is_not_a_program_file` |
| AC-3 | Pass | `c1600_4_ac3_basic_listing_tokenized_loaded`, `c1600_4_ac3_typed_true_typed_in`, `c1600_4_ac3_machine_code_at_address_and_started`, `c1600_4_ac3_loader_failure_fails_preset_with_reason` |
| AC-4 | Pass (per C-7) | `c1600_4_ac4_text_typed_in_program_mode`, `c1600_4_ac4_not_in_program_mode_fails_naming_lines` |
| AC-5 | Pass | `c1600_4_ac5_blank_and_hash_lines_left_out_space_hash_typed` |
| AC-6 | Pass | `c1600_4_ac6_folder_shown_as_s3`, `c1600_4_ac6_missing_folder_or_not_pc1600_refused_at_read` |
| AC-7 | Pass | `c1600_4_ac7_refused_where_frontend_mounts_no_folder` |
| AC-8 | Pass | `c1600_4_ac8_slot_rom_bytes_read_at_every_load`, `c1600_4_ac8_no_module_twice_unreadable_wrong_size_refused`, `c1600_4_ac8_module_with_slot_rom_never_saved` |
| AC-9 | Pass | `c1600_4_ac9_forms_and_ranges`, `c1600_4_ac9_twice_refused` |
| AC-10 | Pass | `c1600_4_ac10_later_rom_found_first_and_before_peripheral`, `c1600_4_ac10_header_peripherals_in_plan_order` (P-6), `c1600_4_ac10_listed_in_plugged_no_control`, `c1600_4_ac10_attach_command_refused` |
| AC-11 | Pass | `c1600_4_ac11_keys_values_paths_from_preset_folder`, `c1600_4_ac11_unknown_key_wrong_value_refused` |
| AC-12 | Pass | `c1600_4_ac12_load_does_nothing_with_block`, `c1600_4_ac12_debug_block_of_for_the_debugger` |
| AC-13 | Pass | LIB-6 AC-45, AC-46, AC-48 of the Calc-U-1600 column (`features/LIB-6-presets/qa-report.md`) |
| AC-14 | Pass | `c1600_4_ac14_line_key_and_expected_debug_and_bus_rom_prefixed` |
| AC-15 | Pass (the six files since M-47) | `c1600_4_ac15_no_version_1_left` (without exceptions since M-47), `calcu_preset_check.CalcU1600Presets` (without exclusions since M-47), `calcu_preset_check.CalcU1600Templates`; M-47 upgraded `DiskWorks`, `make_diskworks_media`, `pc1500-maxed-out`, `startup/default-pc1600`, the test fixture `startup-pc1600`, `docs/developer/screenshots/presets/pc1600-modules` |
| AC-16 | Pass (all 32 since M-47; see the M-47 section) | `c1600_4_ac16_examples_match_their_references` (`CalcU1600ExampleReferences`, Longer): 28 examples compared with their M-4 references, equal; excepted with their reason: `Calculat` (its program never returns to BASIC), `ce150_demo` (version 1 ended the load in the middle of the plot: the paper is compared as a prefix), biorhythmus' second row (the `INPUT` cursor's blink phase); waiting: the four examples that need LIB-7 |
| AC-17 | Pass | `c1600_4_ac17_bundle_lists_todays_entries`; Qt6 and Web bundle the files of `bundle.yaml` only (ledger 1600-21) |
| AC-18 | Pass | `c1600_4_ac18_names_short_described_once_unique_per_model` |
| AC-19 | Pass | the `c1600_4_ac19_test_` cases (`preset_content_tests.cpp`) |
| AC-20 | Pass | `c1600_4_ac20_blocks_by_the_file_syntax` |
| EC-1 | Pass | `c1600_4_ec1_text_without_lines_refused` |
| EC-2 | Pass | `c1600_4_ec2_no_block_or_unknown_field_refused` |
| EC-3 | Pass | `c1600_4_ec3_second_section_after_first_finished`, `c1600_4_ec3_code_not_returned_fails_at_timeout` |
| EC-4 | Pass | `c1600_4_ec4_mistake_refused_without_debugger` |
| EC-5 | Pass | `c1600_4_ec5_relative_path_without_folder_refused` |
| EC-6 | Pass | `c1600_4_ec6_bank_on_pc1500_refused` |

Status: AC-15 and AC-16 waited for the presets that need LIB-7; *since
M-47 (below) every AC and EC passes: the row is **Done**.*

### Upgrade judgement calls (AC-15)

- A bare `- wait:` as the last item became `- let-it-finish:` (three
  `firmware_bootstrap_*`, nine plotter examples); elsewhere it went.
- `- skip-wait:` before a BREAK into a running program (`iterator`,
  `pc1600-hanoi-run`).
- `ce158_demo.pc1500a`: the comment lines that describe the `pc1500_cli`
  run were left.

### Deviations

- `docs/developer/screenshots/presets/pc1600-modules.pc1600` is a sixth
  preset that waits for LIB-7 (it names module files); the design's list
  has five.
- `default-pc1600` stays out of `examples/bundle.yaml` until M-47.

## The presets that waited (M-47, 2026-11-16, branch `m47-media-1600`)

With LIB-7 1600 (`features/LIB-7-media-and-save-folder/qa-report.md`, the
Calc-U-1600 section). The six files in format 2: `examples/dwx/DiskWorks.pc1600`
(`printer: CE-1600P`, `host-drive: { folder: S3 }`, `slot-1: { name: CE-1600M }`,
`slot-2: { file: CE-1601M - Progs.card.yaml }`, `- floppy: { file: Progs.floppy.yaml }`
first in its `keys:`), `examples/dwx/make_diskworks_media.pc1600` (`- floppy: { name: Formatted }`
first; `- saveas: { place: slot-2, as: template, file: CE-1601M - Progs.card.yaml }`,
`- saveas: { place: floppy, as: template, file: Progs.floppy.yaml }`),
`examples/memory/memory-cards/pc1500-maxed-out.pc1500` (`slot-1: { file: … }`,
`- wait: 2s`), `examples/startup/default-pc1600.pc1600` and the fixture
`tests/fixtures/presets/startup-pc1600.pc1600` (`- floppy: { name: Formatted }`
first), `docs/developer/screenshots/presets/pc1600-modules.pc1600` (`saveas:`
as a mapping). `examples/bundle.yaml` lists `startup/default-pc1600` in
`startup:`; `examples/dwx/Progs.floppy.yaml` and the bundled disks are disk
format version 2 (sides `0:`/`1:`).

| ID | Result | Test case |
|---|---|---|
| AC-15 | Pass | `c1600_4_ac15_no_version_1_left` (no exception left), `calcu_preset_check.CalcU1600Presets` (no exclusion), `c1600_4_ac17_bundle_lists_todays_entries` (with `startup/default-pc1600`), `c1600_4_ac18_names_short_described_once_unique_per_model` (the bundle check reads the media steps: `PresetReadRig` has the media part's vocabulary) |
| AC-16 | Pass | `CalcU1600ExampleReferences` (Longer): 32 of 32 compared, equal but the excepted ones; the four of M-47: `pc1500-maxed-out` (24,576 bytes of memory) and `default-pc1600` equal; `make_diskworks_media` equal in display, memory and paper, and the module and the disk its `saveas:` wrote equal to those of `examples/dwx/` but their time stamps (6 and 12 rows of bytes: the directory entries' dates of the files `SAVE` wrote, from the harness's fixed clock, not 0.8.1's; the comments of the module and the disk's description, AC-41, are left out of the comparison); `DiskWorks` equal in memory, its display excepted (finding below) |

Exit check of M-47: `calcu --product 1600 preset examples/dwx/DiskWorks.pc1600 then 'wait 5s' then lcd`
shows DiskWorks' file list of S3: (`DW.CFG`, `DWX.BIN`) and its soft keys
(`INF CPY KIL MSK S3: X:`).

Findings:
- **DiskWorks' display (AC-16).** The last item, `BLOAD"S3:DWX.BIN"`, is not
  waited for (LIB-6 AC-60) and the harness's `wait` finds the PC-1600 idle
  (key readiness) while DWX.BIN still comes from the host drive, before the
  banner of the reference is drawn; seconds later DiskWorks shows its file
  list. Excepted by name with this reason; the reference was not recorded
  anew.
- **A disk command and idle (`make_diskworks_media`).** `INIT"X:"` and
  `SAVE"X:…"` write the disk for seconds while the ROM reads the keyboard,
  which counted as idle (the M-46 finding of LIB-6 AC-45): the next item
  started too early and the first `SAVE"X:"` failed with ERROR 164. The
  PC-1600's idle rule now also asks that the CE-1600F's motor be off
  (`app/SharpPresetRules.cpp`; ledger row 1600-43); the reference then
  matches.
