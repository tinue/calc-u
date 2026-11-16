# QA report: C59-3 TI-58C constant memory

Results per acceptance criterion and edge case of `spec.md`.

## M-54 (2026-11-16, branch `m54-presets-59`)

Built and run as C59-2's M-54 section says (`build/tests`, `build/qt`, the
Web `dist/`; Swift type-checked only).

The code (design §2, §3): the kind `c59.constant-memory`
(`core/TIProductKinds.hpp`, `TIConstantMemoryCapability` in
`core/TIMemoryKinds.*`, capability `RAM/c59.constant-memory` on the TI-58C
only), `app/ConstantMemoryFile.*` (constant memory format 1),
`app/ConstantMemoryKeeper.*` (what a build gets, which machine is from the
file, the writes, the notices), `TIProductMachines::builtInDefault(TI-58C)`
handing `contentForBuild()` to LIB-18, `TIAppPart` telling the keeper the
builds (`machineChanged`), the calls (`afterCall`) and the moments
(`beforeMachineChange`, `suspending`, `closing`). `app/ConstantMemory.*`,
`StateApplier::powerOn` and `c59.persist-constant-memory` are gone; the
file is `ti58c.memory.yaml` (`TIFileAccess::kConstantMemoryFile`).
Frontends: the Swift bridge's `cardWritten` leaves out `ti58c.memory.yaml`;
Qt6's `AppPaths::constantMemoryFile()` is gone and its Settings row names
the new file; the Web keeps it in the save folder. Tests:
`tests/constant_memory_tests.cpp` (on `tests/AppRig.hpp`: the app session
as a native frontend hosts it, the save folder in memory, the rig's wall
clock), `tests/shared_formats_tests.cpp`, `tests/machine_control_tests.cpp`,
Web `frontends/web/tests/storage.spec.js`.

| ID | Result | Test case |
|---|---|---|
| AC-1 | Pass | `c59_3_ac1_listed_on_ti58c_only`, `c59_3_ac1_counter_changes_with_content_only` |
| AC-2 | Pass | `c59_3_ac2_partition_and_display_format_come_back` (2 Op 17 and Fix 2, quit, started again: 320 steps and 7.00), `c59_3_ac2_memory_in_place_before_the_processor` |
| AC-3 | Pass | `c59_3_ac3_file_at_root_of_save_folder_not_a_medium` |
| AC-4 | Pass | `c59_3_ac4_format_1_r000_to_r063_h_as_r060_r063`, `c59_3_ac4_own_card_stored_module_card_not`, `c59_3_ac4_register_above_r063_refused` |
| AC-5 | Pass | `c59_3_ac5_written_within_half_a_second_wall_time`, `c59_3_ac5_not_written_without_change` |
| AC-6 | Pass | `c59_3_ac6_written_at_once_before_replace_on_suspend_on_quit` |
| AC-7 | Pass | `c59_3_ac7_start_and_model_switch_get_the_file`, `c59_3_ac7_cue_card_of_the_file_fitted_again`; `lib18_ac19_ti58c_without_startup_preset_gets_the_file_at_start_and_switch` |
| AC-8 | Pass | `c59_3_ac8_preset_or_startup_preset_zeroed_preset_card`, `lib18_ac19_ti58c_from_preset_cold_booted` |
| AC-9 | Pass | `c59_3_ac9_cold_boot_zeroes_then_startup` |
| AC-10 | Pass | `c59_3_ac10_preset_machine_never_writes`, `c59_3_ac10_cold_boot_of_file_machine_file_follows_card_kept` |
| AC-11 | Pass | `c59_3_ac11_reset_power_cycle_memory_and_card_stay_file_not_read` |
| AC-12 | Pass | `c59_3_ac12_ti59_ti58_neither_read_nor_write` |
| AC-13 | Pass (C++ and Web; Swift by its build only) | `c59_3_ac13_the_host_folder_is_the_one_written` (the save folder and FileIO each host passes); Web `c59_3_ac13 the constant memory is ti58c.memory.yaml in the save folder` (Playwright); Qt6 and Swift hand their folder as before (Swift not run from the worktree: the user's `xcodebuild` runs, see M-54's report) |
| AC-14 | Pass | `c59_3_ac14_round_trip_zero_registers_left_out`, `c59_3_ac14_model_switch_away_and_back_keeps_memory`; Web `c59_3_ac14 kept across a page load` |
| EC-1 | Pass | `c59_3_ec1_no_file_zeroed_written_at_first_change` |
| EC-2 | Pass | `c59_3_ec2_outdated_or_no_version_not_read_notice` (no version, version 0, 2.3.0's layout), `c59_3_ec2_ti58c_mem_ignored_without_notice` |
| EC-3 | Pass | `c59_3_ec3_damaged_line_nothing_used_notice` |
| EC-4 | Pass | `c59_3_ec4_failed_write_notice_once_next_change_retries` |
| EC-5 | Pass | `c59_3_ec5_no_register_lines_valid` |
| EC-6 | Pass | `c59_3_ec6_startup_preset_not_loadable_gets_the_file` |
| EC-7 | Pass | `c59_3_ec7_failed_preset_writes_nothing` |
| EC-8 | Pass | Web `c59_3_ec8 2.3.0's localStorage memory is not read` (Playwright) |

### Row 59-15

T2 shows the order of the real calculator: the memory is in place before
the processor starts and the ROM's start-up restores the partition and
Fix from E000 (`rom/TI58C-ROM-Guide.md`, "Power-on"). 2.3.0's order (the
start-up first, then the RAM) is not run again: its code is gone (design
§8). The row's "to be confirmed" is answered.

### Deviations from the design

- D-1: the counter is computed when it is asked (between two advances):
  it changes when R000–R063 differ from what was seen at the last read,
  instead of a count on the RAM write path; the advances carry no cost and
  the golden runs are untouched. What AC-1 asks holds.
- The keeper decides "not written when nothing changed" by comparing the
  content (64 registers and the own card) with what it last wrote, so a
  change undone within the half second writes nothing.
- A file that cannot be read at all (the host's FileIO says so) gets the
  damaged notice's form without a line.
- `c59_3_ac13_…` is one C++ case for all hosts (the folder and FileIO each
  hands in); there are no per-frontend Qt6 or Swift scenarios.

## Ledger

Rows that became visible: 59-1 (`ti58c.mem` ignored), 59-12, 59-15, 59-16,
59-17 (the constant memory file keeps the fitted own card). No new row.
