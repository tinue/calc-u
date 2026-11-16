# LIB-18: QA report

Results per acceptance criterion and edge case of `spec.md`, per column.

## Library (M-35, 2026-11-15)

Built and run on an M4 Pro, macOS 27.2, Apple clang, CMake, Ninja,
RelWithDebInfo (`build/tests`, `-DCALCU_FRONTEND_QT=OFF`). The code is
`library/startup/` (namespace `calcu::startup`, in `calcu_library`) and
the harness commands in `library/harness/StartupCommands.cpp`; the tests
are `library/startup/tests/` and `library/harness/tests/startup_commands_tests.cpp`,
compiled into `calcu_library_tests` (Frequent). They use LIB-1's test
machines (`test-seg`: 100,000 ticks per second, places `slot-1`,
`printer`, `aux`, memory = key presses kept by reset and power cycle;
`test-dot`: 1,000,000 ticks per second, variants V1 and V2, switched off by
`off`, places `a`, `b`, `card-1`, `card-2`, a real-time clock), the new
build option `TestStartUp` (the start-up's length, or never ready), the
test product `TestProductMachines.hpp` (built-in defaults: test-seg with
the test printer in `printer`, test-dot bare; a build counter; a readiness
bound of 1 s unless set), the stand-in loader `TestPresetLoader.hpp` (the
spec's stand-in for LIB-6: builds a scripted plan through
`MachineControl`, or cannot read it, or fails after building) and
`TestHook.hpp` (a `SliceHook` holding text and keys, counting `drop()`).
Both hostings are used: the harness's virtual time (an operation in one
go) and paced host calls on a scripted clock (an operation in parts, busy
in between).

Result of the run: `calcu_library_tests` 373 tests (62 of them LIB-18's:
55 in `library/startup/tests/`, 4 in `startup_commands_tests.cpp`, plus
`can_replace_answers_as_replace_does`, `ac16_ending_command_that_starts_nothing_stops_nothing`
and `ac2_run_starts_with_the_startup_model_under_default_settings`), 3,003
checks passed, 0 failed. The whole Frequent group passed: 28 of 28 CTest
entries. CI was not dispatched (plan §1: at the end of the part).

| ID | Result | Test case (file in `library/startup/tests/` unless named) |
|---|---|---|
| AC-1 | Pass | `ac1_one_build_with_model_variant_and_plugs` (one build with V2, a printer and an 8k module; the ROM images from the RomSet), `ac1_no_power_cycle_for_what_it_was_built_with`, `ac1_clock_given_host_time_at_build`, `ac1_ready_when_key_ready` (over at the first run piece after readiness), `ac1_refused_build_changes_nothing`, `refusal_names_the_refused_place` (LIB-6 AC-52), `last_build_gives_plan_and_origin`, `test_product_machines_build_each_test_model` (`build_tests.cpp`) |
| AC-2 | Pass (Library part) | `ac2_old_machine_and_its_memory_gone`, `ac2_full_speed_and_held_keys_do_not_carry_over` (`build_tests.cpp`). The clause "what was saved as a medium stays saved" passes with LIB-7 in M-41 (design §1, D-19) |
| AC-3 | Pass | `ac3_startup_model_named`, `ac3_last_gives_last_model`, `ac3_nothing_recorded_gives_fallback` (`last-model` written to the file) (`start_tests.cpp`) |
| AC-4 | Pass (stand-in loader) | `ac4_startup_preset_loaded`, `ac4_no_startup_preset_builtin_default`, `ac4_no_loader_builtin_default` (D-9) (`start_tests.cpp`); with LIB-6's loader in M-38 |
| AC-5 | Pass | `ac5_start_is_one_operation_busy_until_ready` (paced: busy at once, reply at the end, one build), `ac5_one_build_when_model_named`, `ac5_one_build_when_preset_named`, `ac5_unknown_model_notice_and_startup_model`, `ac5_second_start_refused` (D-8) (`start_tests.cpp`) |
| AC-6 | Pass | `ac6_model_switch_from_startup_preset`, `ac6_model_switch_builtin_default` (`switch_tests.cpp`) |
| AC-7 | Pass (Library part) | `ac7_variant_switch_keeps_plugs_in_places`, `ac7_variant_switch_memory_empty_no_preset` (`switch_tests.cpp`). The clause "the same media in them" passes with LIB-7 in M-41 |
| AC-8 | Pass | `ac8_same_model_done_nothing_built`, `ac8_same_variant_done` (`switch_tests.cpp`) |
| AC-9 | Pass | `ac9_last_model_after_command_preset_and_start`, `ac9_unwritable_settings_keep_it_for_the_run` (`switch_tests.cpp`) |
| AC-10 | Pass | `ac10_attach_is_one_operation_over_at_key_readiness` (paced), `ac10_attach_named_and_default_variant`, `ac10_detach_place`, `ac10_attach_to_full_single_place_is_one_replace_one_power_cycle`, `ac10_switched_off_machine_nothing_follows` (`change_tests.cpp`) |
| AC-11 | Pass | `ac11_memory_through_power_cycle_and_new_plug_found` (`change_tests.cpp`) |
| AC-12 | Pass | `ac12_refusal_changes_nothing_no_power_cycle_reason` (the machine's rule `kTogether`), `ac12_full_place_refused_by_can_replace_before_the_operation` (`change_tests.cpp`); `can_replace_answers_as_replace_does` (`library/machine/tests/attach_tests.cpp`) |
| AC-13 | Pass | `ac13_reset_cold_boot_power_cycle_are_operations_over_at_key_readiness` (paced; Reset and power cycle keep the test memory, Cold boot empties it), `ac13_plugs_stay`, `ac13_clock_given_host_time_at_end` (`reset_tests.cpp`) |
| AC-14 | Pass (seam) | `ac14_text_and_held_keys_dropped` (`TestHook`: Reset, Cold boot, power cycle, and the power cycle after an attach) (`reset_tests.cpp`). LIB-5's typing engine implements `drop()` in M-36 |
| AC-15 | Pass | `ac15_each_command_at_top_level_with_its_operations_reply`, `ac15_attach_variant_as_in_a_preset` (`test-module:8k`), `ac15_attach_place_path` (`a/jack`), `ac15_refusal_exits_nonzero` (one-shot) (`library/harness/tests/startup_commands_tests.cpp`) |
| AC-16 | Pass | `ac16_reset_cold_boot_model_stop_running_operation_first`, `ac16_stopped_command_replied_stopped` (a long startup preset stopped by Reset: "Stopped."), `ac16_other_commands_refused_while_running` (`variant`, `attach`, `detach` of an empty place, `power-cycle`, `start`), `ac16_model_to_running_model_leaves_operation_running` (`running_tests.cpp`); `ac16_ending_command_that_starts_nothing_stops_nothing` (`library/sessions/tests/operation_tests.cpp`, D-6) |
| EC-1 | Pass (stand-in loader) | `ec1_preset_gone_outdated_or_other_model_gives_default_and_notice` (`start_tests.cpp`) |
| EC-2 | Pass (stand-in loader) | `ec2_last_built_by_preset_uses_its_startup_preset` (`start_tests.cpp`) |
| EC-3 | Pass | `ec3_ready_at_a_prompt_ends_the_wait` (`reset_tests.cpp`); by hand in the harness: the PC-1500A's start ends at `NEW0? :CHECK` |
| EC-4 | Pass | `ec4_text_being_typed_dropped_with_the_old_machine` (`build_tests.cpp`) |
| EC-5 | Pass | `ec5_detach_chain_one_power_cycle`, `detach_empty_place_done` (`change_tests.cpp`) |
| EC-6 | Pass (stand-in loader) | `ec6_preset_fails_while_running_machine_stays_notice` (`start_tests.cpp`) |
| EC-7 | Pass | `ec7_never_ready_ends_after_bound_with_log_line_machine_left` (bound 0.01 s on test-seg, checked in ticks; "The test-seg was not ready for keys after 0.01 seconds; it is left as it is.") (`reset_tests.cpp`) |

Coverage: AC-1 to AC-16 and EC-1 to EC-7 of the Library column. The
column stays In progress (`features/INDEX.md`): AC-2's and AC-7's media
clauses pass with LIB-7 in M-41, where the column becomes Done (D-19);
AC-14's typing clause is tested on the seam until LIB-5 (M-36).

### The clauses that passed later (M-36, M-38, M-41; design §1, D-19)

Checked again on 2026-11-15 on branch `m41-media` (`calcu_library_tests`
729 tests, 0 failed; Frequent 30 of 30 in `build/tests`, 31 of 31 in
`build/qt`):

| ID | Clause | Result | Test case |
|---|---|---|---|
| AC-2 | "what was saved as a medium stays saved (LIB-7)" | Pass (M-41) | `lib18_ac2_what_was_saved_as_a_medium_stays_saved` (a disk's unsaved change written before the build, there after two model switches) (`library/media/tests/preset_media_tests.cpp`) |
| AC-7 | "the same media in them" | Pass (M-41) | `lib18_ac7_variant_switch_keeps_the_media` (a disk in the drive and a module medium in `card-1` of test-dot carried to V2 with their unsaved content, still bound to their instances; `MachineControl::addCarry`, LIB-7 S-7a) (same file) |
| AC-14 | "the keys the typing engine holds" | Pass (M-36) | LIB-5's `ec5_reset_power_cycle_cold_boot_replace_all_up_nothing_waits` (`library/typing/tests/reset_tests.cpp`: the engine's `drop()` through `dropInput`) |
| AC-4, EC-1, EC-6 | "loading a preset (LIB-6)" | Pass (M-38) | `startup_preset_built_by_the_loader`, `startup_preset_outdated_gives_default_and_notice`, `startup_preset_failing_while_run_keeps_machine` (`library/presets/tests/startup_tests.cpp`); AC-6 and EC-2 with the stand-in loader the spec allows (above) |

With M-41, `build` and `change` call `SessionPart::beforeMachineChange`
through `MachineSession::aboutToChange` (LIB-7 §3.12; `setMachine` and
`takeMachine` call it, `change` before an attach, detach or replace), and
`MachineControl::addCarry` adds to a carried plug's request. Every criterion
and edge case of the Library column passes: the column is **Done**
(`features/INDEX.md`; D-19).

### Harness and products (design §3.6, P59, P16)

`Server::start` builds no machine itself: it adds `StartupPart` after
`PeripheralsPart` and `MachineStatePart` and sends `start`; a start that
is not done ends the run with its reason. `HarnessProduct::startupRequest`
and `startUp` (LIB-11 A-1) are gone; `machines()` is new. Both product
entries implement it with the adapter `HarnessMachines` (Calc-U-59: the
model with the PC-100C, TRACE off, no module, the TI profile, 25 s;
Calc-U-1600: the bare model, the Sharp profile, `kSoundRate`, 7 s / 7 s /
27 s, and `startUp` = `SharpBuildColdBoot`).

- `harness_switches_to_a_ti58` (`CalcU59Tests`): `model TI-58`, `lcd`
  shows `0`, the PC-100C in `printer`, the TI-58 runs; `model ti-59`; an
  unknown model refused.
- `harness_drives_a_pc1600`, `harness_variant_switch_old` (`CoreTests`):
  `model PC-1600`, `lcd` reads `>` with the PC-1600's font, the place
  `host-drive`; `variant old` gives the PC-1600 `Old`.
- `harness_drives_a_ti59` and `harness_drives_a_sharp_machine` pass
  unchanged.

### Golden runs

M-35 changes no core and no frontend; both products' golden replays pass
unchanged in Frequent.

### Deviations from the design, chosen by the simpler structure

- D-11 (full speed does not carry over): `SessionContext::setCommandedSpeed`
  changes nothing while an operation runs (LIB-3 D-6), and every build of
  LIB-18 runs inside one. `MachineControl::build` therefore sets regular
  speed at once when no operation runs, and otherwise `StartupPart::operationEnded`
  sets it when the operation ends (or is stopped). No LIB-3 change.
- `MachineControl::change` returns `Task<void>` as designed; a change the
  machine refuses although `check()` passed fails the operation with the
  machine's reason (an exception inside the operation, caught by the
  session). Not reachable from the commands, which ask `check()` first.
- `MachineControl::build` checks each plug of a plan on the machine's own
  places before the builder sees it (unknown peripheral, unknown variant,
  no such place, does not fit), so that the refusal names the place
  (LIB-6 AC-52); a place on a peripheral is left to the builder.
- `start` with a named preset and no loader builds the built-in default
  without a notice (D-9 read for the named preset too).
- The startup tests declare their own settings product (`testDecl()` in
  `TestProductMachines.hpp`: models `test-seg`, `test-dot`) instead of
  `library/settings/tests/TestProduct.hpp`, whose models are not those of
  the test catalogue.
- The harness's test product (`library/harness/tests/TestProduct.*`)
  declares `[startup] last-model`, and its `startModel` is the fallback
  model of its declaration (LIB-11 AC-2, "under the default settings").
- A settings file named with `--settings` gets `[startup] last-model`
  written by the run, as a product writes it (AC-9; LIB-9). LIB-11's
  `ac8_options_name_settings_save_and_output_folders` checked that the
  file stayed unchanged; it now checks that only `last-model` was added.
- LIB-3: `tp.stop` of the sessions' test part starts an operation that
  ends at once (D-6: a Done handler no longer stops); `tp.stop-nothing` is
  the Done form. `ac16_ends_running_operation_command` and LIB-2's
  `ac23_set_from_host_at_build_and_after_operation` count the second end.
- LIB-11's tests that read `ticks` or the display right after the start
  (`commands_tests.cpp`, `display_text_tests.cpp`, `protocol_tests.cpp`,
  `run_tests.cpp`, `calcu_cli.*`, `calcu_serve_python`) now see the
  machine at key readiness: one run piece, 1,667 ticks on test-seg and
  16,668 on test-dot (design §8).

## Calc-U-1600 (M-46, 2026-11-16, branch `m46-presets-1600`)

Built and run on an M4 Pro (macOS 27.2, Apple clang, Ninja,
RelWithDebInfo): `build/tests` (`-DCALCU_FRONTEND_QT=OFF`, Frequent 34 of
34, `CoreTests` 51 s), `build/qt` (`-DCALCU_FRONTEND_QT=ON
-DCALCU_TESTS_LONGER=ON`), the Web `dist/` with Homebrew Emscripten and
`frontends/web/run-tests.sh`.

The code (design §10.3): `app/SharpProductMachines.*` (new: the
`ProductMachines` of Calc-U-1600: the built-in default, the pacing, the
bounds of key readiness, the carried host-drive folder, `addRoms` for the
host-drive ROM); `SharpAppPart` owns the library's `StartupPart`; the
start, `model`, `variant`, `attach`, `detach`, `reset`, `cold-boot` are
its commands in every frontend and the harness; `c1600.select-module`,
`c1600.mount-host-drive`, `c1600.ce1600p-rom` are plug changes through
`MachineControl::change`; `c1600.pick-model`, `c1600.pc1500-rom`,
`c1600.pc1600-rom`, `c1600.toggle-peripheral`, `c1600.reset` and the
adapters of `LegacyCalls` for them are gone. `SharpBuilder` cold boots
every machine it builds (D-23). Tests: `tests/machine_control_tests.cpp`.

| ID | Result | Test case |
|---|---|---|
| AC-25 | Pass | `lib18_ac25_no_settings_pc1500a_new0_check` |
| AC-26 | Pass | `lib18_ac26_builtin_default_variant_and_nothing_plugged`, `lib18_ac26_version1_startup_preset_gives_default_and_notice` (EC-1) |
| AC-27 | Pass | `lib18_ac27_model_switch_keeps_no_module_or_peripheral`, `lib18_ac27_ce1600p_rom_choice_back_to_new` |
| AC-28 | Pass | `lib18_ac28_variant_switch_replugs_printer_interface_host_drive_modules`, `lib18_ac28_disk_in_drive_again_same_side`, `lib18_ac28_no_startup_preset_loaded` |
| AC-29 | Pass | `lib18_ac29_module_replace_power_cycle_memory_kept_new0_check` |
| AC-30 | Pass | `lib18_ac30_printer_ce158_attach_detach_power_cycle_memory_kept`, `lib18_ac30_host_drive_mount_unmount_power_cycle` |
| AC-31 | Pass | `lib18_ac31_plugged_is_replace_by_other_variant`, `lib18_ac31_unplugged_choice_holds_for_next_attach` |
| AC-32 | Pass | `lib18_ac32_reset_keeps_memory`, `lib18_ac32_cold_boot_empties_memory_ce151_ce155_battery_module_kept`; Web `play.spec.js` `lib18_ac32 cold boot menu item` |
| AC-33 | Pass | `lib18_ac33_off_machine_change_nothing_follows_found_on_switch_on` |
| Ledger 1600-9 | Pass | `ledger_1600_9_reset_and_model_switch_stop_a_preset`, `ledger_1600_9_keys_dropped_and_reset_stops_an_operation` |

Column status: every criterion of the column passes: **Done**.

### Deviations and findings (Calc-U-1600)

- Mounting a second host-drive folder while one is mounted is a replace
  with a power cycle (0.8.1 swapped the folder live); ledger row 1600-39.
- A start without a full ROM set is refused with the lookup's text
  (`StartupSetup::romsMissing`), not started with a notice.
- The CE-1600P ROM choice of a frontend (`m_ce1600pChoice`) lives in
  `SharpAppPart`; `ProductMachines` carries only the host-drive folder.
- Finding: the CE-150's power-on feeds its paper; an attach now replies
  when the machine is ready for keys, so the paper panel is no longer empty
  right after the attach. `tests/gui/lib8-ac36.shots.yaml` checks the name
  only at that point.

## Calc-U-59 (M-54, 2026-11-16, branch `m54-presets-59`)

Built and run as C59-2's M-54 section says (`build/tests`, `build/qt`, the
Web `dist/`; Swift type-checked only).

The code (design §11.2, §11.3): `app/TIProductMachines.*` (the TI
catalogue and builder, the built-in default of each model: the Master
Library module, no printer; the TI-58C's constant memory as build content;
the TI pacing profile, no sound, 25 s readiness bound). `TIAppPart` owns
the library's parts (`parts::LibraryParts`, LIB-6 D-40) and follows each
new machine at `machineChanged` (a new `EmulatorSession` around the
session's machine, which neither builds nor owns it any more; the module
and the printer read from `Machine::plugged()`, D-26; the own cue card;
the constant memory keeper). `c59.start-machine`, `c59.reset`,
`c59.clean-reset`, `c59.change-module`, `c59.printer-connected`,
`c59.toggle-printer-connection` are gone: the frontends send `start`
(`makeAppSession` sends it for the native hosts), `model`, `reset`,
`cold-boot`, `attach`, `detach`. The harness entry's `HarnessMachines` is
gone: it uses `TIProductMachines`. Tests: `tests/machine_control_tests.cpp`
(on `tests/AppRig.hpp`), the ported cases of `tests/part_flows_tests.cpp`,
`tests/typing_tests.cpp`, `tests/harness_tests.cpp`.

| ID | Result | Test case |
|---|---|---|
| AC-17 | Pass | `lib18_ac17_no_settings_ti59`; the Web's start (`lib15_ac16 …`, Playwright) |
| AC-18 | Pass | `lib18_ac18_builtin_default_ml_no_printer` (each model), `lib18_ac18_2_3_0_startup_preset_gives_default_and_notice`, `lib18_ac18_startup_preset_builds_its_machine`, `harness_starts_with_ml_and_no_printer` |
| AC-19 | Pass | `lib18_ac19_ti58c_without_startup_preset_gets_the_file_at_start_and_switch`, `lib18_ac19_ti58c_from_preset_cold_booted`; C59-3 T5 |
| AC-20 | Pass | `lib18_ac20_switch_carries_no_module_no_printer`, `lib18_ac20_log_and_debug_settings_stay`; the paper per printer: LIB-8 59 (M-53) |
| AC-21 | Pass | `lib18_ac21_module_attach_detach_power_cycle_ti59_loses_ti58c_keeps`, `lib18_ac21_printer_attach_detach_power_cycle`, `lib18_ac21_trace_kept_through_detach_and_attach`, `lib18_ac21_trace_set_again_in_the_attached_pc100c`, `c59_6_ac11_change_module_power_cycles` |
| AC-22 | Pass | `lib18_ac22_reset_keeps_program_and_data_ti58c_power_cycle`, `lib18_ac22_cold_boot_empties_all_constant_memory_too`, `part_reset_keeps_the_memory_and_clean_reset_clears_it` |
| AC-23 | Pass (the stand-alone card; the fitted magnetic card in M-55, C59-7 AC-25) | `lib18_ac23_paper_module_printer_trace_own_card_kept`, `lib18_ac23_power_cycle_keeps_own_card` |
| AC-24 | Pass | `lib18_ac24_preset_for_another_model_builds_it_last_model_follows`, `lib18_ac24_last_model_follows_a_preset_build` |
| AC-16 (row 59-39) | Pass | `ledger_59_39_reset_and_model_switch_stop_a_preset`, `lib5_ec5_reset_and_model_switch_release_held_keys`, `lib10_ac30_part_cpu_freeze_on_start_sees_reset_and_clean_reset` |

Column status: every criterion of the column passes: **Done**.

### Deviations and findings (Calc-U-59)

- `makeAppSession(…, start = true)` sends `start` for the native hosts
  (Qt6, Swift, the tests' SessionRig); a host that wants its notices
  listens first and passes `start = false` (tests/AppRig.hpp). The Web
  engine builds its own session and its page sends `start`.
- `Settings::setLastUsedModel`, `lastUsedModel` and `startupPreset` stay
  in `app/Settings.*` (the settings tests read them); no frontend calls
  `setLastUsedModel` any more: `StartupPart` writes `[startup] last-model`.
- No Settings row sets the startup preset yet (D-28): `[preset] TI-59`
  (`TI-58`, `TI-58C`) in `Calc-U-59.ini`, e.g.
  `examples:startup/default-ti59.ti59`; M-57 and M-58 add the rows.
- The TRACE position follows the PC-100C's switch after every call and is
  put into a PC-100C that an `attach` plugs into the running machine
  (D-27), not into one a build makes.
