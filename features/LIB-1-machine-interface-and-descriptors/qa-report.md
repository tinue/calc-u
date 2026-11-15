# LIB-1: QA report

Results per acceptance criterion and edge case of `spec.md`, per column.

## Library (M-18, 2026-11-14)

Built and run on an M4 Pro, macOS 27.2, Apple clang 21, CMake 4.5, Ninja.
The test program is `calcu_library_tests` (CTest entry of the same name,
label `frequent`); the sources are `library/machine/tests/`, the machines
built for the tests `library/machine/tests/TestMachines.hpp`, `.cpp`
(design §6, T4). Result of the run: 40 tests, 290 checks passed, 0 failed,
0.18 s.

| ID | Result | Test case (file in `library/machine/tests/`) |
|---|---|---|
| AC-1 | Pass | `ac1_catalogue_gives_descriptors_without_machine_or_rom` (`descriptor_tests.cpp`) |
| AC-2 | Pass | `ac2_machine_descriptor_states_every_part` (`descriptor_tests.cpp`) |
| AC-3 | Pass | `ac3_peripheral_descriptor_and_one_added_later` (`descriptor_tests.cpp`) |
| AC-4 | Pass | `ac4_place_gives_connector_and_fitting_peripherals` (`descriptor_tests.cpp`) |
| AC-5 | Pass | `ac5_machine_reports_model_and_variant_for_its_life` (`build_tests.cpp`) |
| AC-6 | Pass | `ac6_two_machines_built_alike_agree` (`determinism_tests.cpp`; wall-time gaps of 0, 3 and 7 ms between the calls) |
| AC-7 | Pass | `ac7_advance_runs_whole_instructions_and_reports_them` (`time_tests.cpp`) |
| AC-8 | Pass | `ac8_tick_count_never_goes_back` (`time_tests.cpp`; across reset, power cycle, cold boot and keys down; a new machine at 0) |
| AC-9 | Pass | `ac9_no_advance_no_change` (`time_tests.cpp`; 50 ms of wall time) |
| AC-10 | Pass | `ac10_round_trip_every_catalogue_rate`, `ac10_whole_multiples_give_whole_seconds` (`emulated_time_tests.cpp`; the five rates, counts 0, 1, rate − 1, rate, k · rate, 2⁴⁰, 366 days and 2,000 fixed pseudo-random counts below it) |
| AC-11 | Pass | `ac11_operations_return_at_once_and_end_at_key_readiness` (`operations_tests.cpp`) |
| AC-12 | Pass | `ac12_key_down_up_at_tick_counts` (`keys_tests.cpp`) |
| AC-13 | Pass | `ac13_two_keys_down_together` (`keys_tests.cpp`) |
| AC-14 | Pass | `ac14_peripheral_controls_are_not_machine_keys` (`descriptor_tests.cpp`) |
| AC-15 | Pass | `ac15_segments_and_brightness_per_position_and_dp` (`display_tests.cpp`) |
| AC-16 | Pass | `ac16_dots_of_descriptor_size_in_cells` (`display_tests.cpp`) |
| AC-17 | Pass | `ac17_indicators_by_name_with_state` (`display_tests.cpp`) |
| AC-18 | Pass | `ac18_output_has_no_host_pixels_glyphs_colours` (`display_tests.cpp`; the shape of the types by `static_assert`, the dots 0 or 1). The absence of a pixel format, glyph or colour in `library/machine/Display.hpp` is also a review point |
| AC-19 | Pass | `ac19_samples_at_named_rate_independent_of_slicing` (one advance, slices of 100 and of 7 ticks; rates 8,000 and 11,025), `ac19_machine_without_sound_gives_none` (`sound_tests.cpp`) |
| AC-20 | Pass | `ac20_plugged_per_place_and_per_connector` (`capability_tests.cpp`; a chain of two peripherals on one connector) |
| AC-21 | Pass | `ac21_capabilities_listed_with_kind_id_owner` (`capability_tests.cpp`) |
| AC-22 | Pass | `ac22_peripheral_capabilities_only_with_peripheral` (`capability_tests.cpp`) |
| AC-23 | Pass | `ac23_product_kind_handed_out_like_generic` (`capability_tests.cpp`) |
| AC-24 | Pass (locally) | `ac24_library_tests_run_without_products` (`build_alone_tests.cpp`: the test machines have a segment display, a dot-matrix display, sound, peripherals and a product kind between them). Build side: `cmake -S . -B build/library -G Ninja -DCALCU_PRODUCT_59=OFF -DCALCU_PRODUCT_1600=OFF -DCALCU_FRONTEND_QT=OFF`, build and `ctest` pass (1 test). The same configure is the new CI job `library` in `.github/workflows/calc-u-59.yml` and `calc-u-1600.yml`; it has not run in CI yet (the branch is not pushed) |
| AC-25 | Pass | `ac25_debug_targets_from_descriptor` (`capability_tests.cpp`) |
| AC-42 | Pass | `ac42_key_readiness_only_after_start_up` (`operations_tests.cpp`); after the three operations also in `ac11_…` |
| AC-43 | Pass | `ac43_display_off_shows_nothing` (`display_tests.cpp`) |
| EC-1 | Pass | `ec1_missing_rom_names_the_file`, `ec1_missing_peripheral_rom_names_the_file` (`build_tests.cpp`) |
| EC-2 | Pass | `ec2_unknown_key_is_refused_state_unchanged` (`keys_tests.cpp`) |
| EC-3 | Pass | `ec3_repeated_down_or_up_changes_nothing` (`keys_tests.cpp`) |
| EC-4 | Pass | `ec4_compare_and_add_across_rates` (`emulated_time_tests.cpp`; one tick apart after 2⁴⁰ s, beyond what doubles tell apart) |
| EC-5 | Pass | `ec5_absent_kind_gives_empty_list` (`capability_tests.cpp`) |
| EC-6 | Pass | `ec6_no_variant_and_single_variant` (`descriptor_tests.cpp`) |
| EC-7 | Pass | `ec7_one_tick_whatever_clocks_inside` (`time_tests.cpp`; the test machine's inner block on a clock of its own). The PC-1600 case itself is C1600-1 AC-1 (M-26) |
| EC-8 | Pass | `ec8_advance_by_zero_runs_nothing` (`time_tests.cpp`) |

Support cases (no criterion, design §6 T13): `conformance_passes_for_test_machines`,
`conformance_names_a_wrong_key_list` (`conformance_tests.cpp`), Pass.

Technical Requirements (review, not tests): `library/machine/` reads no
wall time, starts no thread, takes no lock, throws no exception across its
interface and needs no RTTI; it includes the standard library only
(design §2a). The library names no product kind: `"test.counter"` lives in
the test machines only.

Green after the step (2026-11-14):
- `cmake -S . -B build/tests -G Ninja -DCALCU_FRONTEND_QT=OFF`, build,
  `ctest`: 6/6 passed, 63.8 s in all: `calcu_library_tests` 0.18 s;
  Calc-U-59 `CalcU59Tests` 8.3 s, `CalcU59TraceReader` 16.3 s,
  `CalcU59XcodeProjectCheck` 0.16 s (24.7 s); Calc-U-1600 `CoreTests`
  38.3 s, `CalcU1600CheckPresets` 0.2 s (38.5 s). Each product within 60 s.
- `cmake -S . -B build/all -G Ninja` (Qt6 found): both Qt apps build;
  `ctest` 6/6 passed.
- `build/web` (Homebrew Emscripten): `calcu_library` compiles with
  `-fwasm-exceptions`; `ti59-core`, `calcu1600-core` and `wasm_smoke` link.
- `products/calc-u-59/frontends/swift/build_xcframework.sh` builds the
  XCFramework (Release, three slices). The Swift app itself was not built:
  nothing in it references the library yet (M-19 adds the library to the
  XCFramework).

### Deviations from the design

- **No `Capability.cpp`.** Nothing of §3.7 needs a source file.
  `capabilityAs` and `capabilitiesOf` take the machine as a deduced template
  parameter (`template <class K, class M>`), so `Capability.hpp` does not
  need `Machine.hpp` (which includes it); the call stays
  `capabilityAs<K>(machine, id)`.
- **Small additions to the sketches:** `MachineDescriptor::connector()`,
  `PeripheralDescriptor::romsFor()` and `fitsPinOut()`; comparison
  operators on `PlacePath`, `ComponentOwner`, `PeripheralOwner`. `build()`
  hands the product's builder the request with the default variants filled
  in where none was named, and refuses an unknown model, variant or
  peripheral with a reason in `BuildResult::refusal`.
- **Test machines (T4).** The chain AC-20 needs is on `test-seg` (connector
  `bus`, places `printer` and `aux`, pin-out `test.bus`; the module slot
  `slot-1` on connector `slot`), because LIB-2 adds `test-dot`'s connectors
  in M-23; the names do not collide with LIB-2's additions. `test-dot` has
  instructions of 4 ticks and the same start-up of 50 ticks as `test-seg`.
  Generic kinds do not exist yet, so a test kind `"test-generic"` (no
  product prefix) stands in for one in AC-21 and AC-23. EC-6's single
  variant is a one-variant copy of `test-dot` made inside the test, not a
  third model in the test catalogue (D-8).
- **Root `CLAUDE.md`** (design §2 "Changed", §7: one line pointing to §2a)
  is not changed in this step: it was outside the scope given for M-18.
  Open for the user.

No point where the design and the spec disagree was found.

## Calc-U-59

AC-26 to AC-33: later. M-19 (with C59-1): AC-26–31, AC-33 without
peripherals; M-25 (with C59-6): AC-32, AC-33 with the PC-100C and a module;
the media place clause of AC-33 passes in M-55 (LIB-7; design D-1).

## Calc-U-1600

AC-34 to AC-41: M-26 (with C1600-1), below; the media place clause of
AC-40 passes in M-47 (LIB-7; design D-1).

## Calc-U-59, the M-19 part (2026-11-14, branch `m19-ti-machines`)

With C59-1 (its qa-report has the run). Tests in
`products/calc-u-59/tests/descriptor_tests.cpp`, program `CalcU59Tests`
(label `frequent`): 7 tests, 341 checks passed, 0 failed. AC-32 and AC-33
with peripherals are tested in M-25 (C59-6), the media place clause of
AC-33 in M-55 (D-1); the column stays In progress until then.

| ID | Result | Test case |
|---|---|---|
| AC-26 | Pass | `lib1_ac26_models_and_tick_rates` (TI-59, TI-58, TI-58C; no variant; 455,000 / 384,000 / 394,000) |
| AC-27 | Pass | `lib1_ac27_segment_display_and_no_sound` (12 positions, A–G and DP, DP's own brightness, a brightness per position, indicator C with brightness, no sound) |
| AC-28 | Pass | `lib1_ac28_rom_entries_per_model` (the three instruction files and two constant files per set, as the fetch list names them; the same names as the app's adapter until M-24) |
| AC-29 | Pass | `lib1_ac29_keys_connectors_places_debug_target` (45 key IDs of C59-1, no PRINT/ADV/TRACE; module slot "8 contacts" and printer connector "11 pads", functional, one each; places `slot-1`, `printer`; debug target TMC0501) |
| AC-30 | Pass | `lib1_ac30_card_reader_on_ti59_only` (pass-through, sides 0 and 1) |
| AC-31 | Pass | `lib1_ac31_components_and_depth` (TMC0501, ROM, RAM separate; SCOM, CROM, printer interface and, on the TI-59, the card reader folded into TMC0501; module slot and printer connector functional) |
| AC-33 | Pass (without peripherals) | `lib1_ac33_machine_conforms_without_peripherals` (`checkAgainstDescriptor` empty for the three models; nothing plugged; no capability before M-55) |

## Calc-U-59, the M-25 part (2026-11-15, branch `m25-connectors`)

With C59-6 (its qa-report has the run and the golden runs). Tests in
`products/calc-u-59/tests/descriptor_tests.cpp`, program `CalcU59Tests`
(label `frequent`): 9 tests, all passed. The media place clause of AC-33
passes in M-55 (D-1); the column stays In progress until then.

| ID | Result | Test case |
|---|---|---|
| AC-31 | Pass (updated) | `lib1_ac31_components_and_depth`: since C59-6 `CROM` and `printer-interface` are no longer components of the machine (the module and the printer are peripherals); SCOM and, on the TI-59, the card reader folded into TMC0501; module slot and printer connector functional |
| AC-32 | Pass | `lib1_ac32_pc100c_and_14_modules` (15 peripheral descriptors: the PC-100C fits the printer connector, no ROM, controls PRINT and ADV keys and TRACE switch, its paper the capability `c59.printer-output` when plugged in; each of the 14 modules fits the module slot with its one ROM file `<ID>.rom.yaml`, 5000 bytes, md5) |
| AC-33 | Pass (with and without the PC-100C and a module); media place clause: M-55 | `lib1_ac33_machine_conforms_with_printer_and_module` (every model with ML and the PC-100C, and empty: `checkAgainstDescriptor` empty, the plug entries, the two capabilities owned by the PC-100C and reachable), `lib1_ac33_machine_conforms_without_peripherals` (now built with nothing plugged) |

Deviation: the 14 module descriptors are static in `tiCatalogue()` rather
than added from the module files (C59-6 qa-report, "Deviations").

## Calc-U-1600, the M-26 part (2026-11-15, branch `m26-sharp-interface`)

With C1600-1 (its qa-report has the run). Tests in
`products/calc-u-1600/tests/descriptor_tests.cpp`, program `CoreTests`
(label `frequent`): 8 tests, 234 checks passed, 0 failed. The media place
clause of AC-40 is tested in M-47 (D-1); the column stays In progress
until then.

| ID | Result | Test case |
|---|---|---|
| AC-34 | Pass | `lib1_ac34_models_variants_rates` (PC-1500 A01/A03/A04, default A04; PC-1500A A04; PC-1600 New/Old, default New; 1,300,000 / 3,580,000) |
| AC-35 | Pass | `lib1_ac35_dot_matrix_indicators_sound` (156 × 7 with 14 indicators, 156 × 32 with 18; sound on all three) |
| AC-36 | Pass | `lib1_ac36_connectors_pinouts_places_debug_targets` (40-pin and 60-pin connectors, contact level; places `slot-1`, `slot-2`, `printer`, `interface`, `host-drive`; RS-232C; debug targets LH5801, SC7852, LH5803) |
| AC-37 | Pass | `lib1_ac37_module_slots_and_components` (each module slot a holding media place in the descriptor; components and depth) |
| AC-38 | Pass | `lib1_ac38_rom_entries_per_variant` |
| AC-39 | Pass | `lib1_ac39_peripheral_descriptors` (nine modules, CE-150, CE-158, CE-1600P New/Old with the CE-1600F, host drive; the CE-161's switch as a control) |
| AC-40 | Pass (without the media place clause) | `lib1_ac40_machines_conform` (`checkAgainstDescriptor` empty for each model and variant, bare and with each peripheral of AC-39 plugged in where it fits; the media place capabilities come with LIB-7, M-47, D-1) |
| AC-41 | Pass | `lib1_ac41_module_fit_table` |
