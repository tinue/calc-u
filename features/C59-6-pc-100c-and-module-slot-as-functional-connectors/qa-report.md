# C59-6: QA report

Results per acceptance criterion and edge case of `spec.md`.

## M-25 (2026-11-15, branch `m25-connectors`)

Built and run on an M4 Pro, macOS 27.2, Apple clang, CMake, Ninja,
RelWithDebInfo (`build/tests`, Qt off). Machine-level cases are in
`products/calc-u-59/tests/peripherals_tests.cpp` (new), the app session's in
`shared_controller_tests.cpp`, the descriptors in `descriptor_tests.cpp`, the
golden replays in `golden_runs_tests.cpp`; all in `CalcU59Tests`, Frequent.
Frequent: 12 of 12 CTest entries pass (`CalcU59Tests` 228 `TEST`s, 36,412
checks, 0 failed).

What the step built (design §2–§4): `core/PrinterPinOut.hpp` and
`core/ModuleSlotPinOut.hpp` (the functional pin-outs), `core/PC100C.*` and
`core/SolidStateRomModule.*` (the peripherals, moved out of `TMC0501`:
the character tables, buffer, ready gate, busy count, pending code line, the
module image and address register), `core/PrinterOutput.hpp` (the product
kind `c59.printer-output`). `TMC0501` holds a `TiConnectors` (`wire()`),
reads the key rows through `keyRow(digit)` and passes the printer and module
words to the pin-outs. `TIMachine` plugs and unplugs both through LIB-2's
`plug()`/`unplug()`, lists them in `plugged()` and offers `printer/controls`
and `printer/c59.printer-output`. The catalogue holds the PC-100C and the 14
modules. `TIPeripheralsAdapter` is gone.

### Golden runs

All 18 runs of `products/calc-u-59/tests/golden-runs/` pass unchanged
through the places (`c59_1_ac4_golden_runs_through_the_machine_interface`):
the printer runs `ti59-printer`, `ti59-print-with-printer`,
`ti59-print-without-printer` and the new `ti58c-printer`; the module runs
`ti59-module-ml`, `ti59-module-le`; the slot runs `ti59-switch-on`,
`ti59-no-module`, `ti58-no-module`; every other run, which uses the Master
Library by the driver's default. `ti58c-printer` (TI-58C with the PC-100C,
the D10 wiring) was recorded from the imported code before the core was
touched, as the first commit of the step (`ff6dbd6`, design D-5). No
`.golden` file was re-recorded.

The empty slot reads EXT = 0 on every module word and drives no address
register (`nullptr` in `TiConnectors`): the no-module runs pass, so design
D-2's fallback (an image of zeros with today's address register) was not
needed.

The 57 reference runs of M-9 (`reference_runs_tests.cpp`, recorded with
`CALCU59_REFERENCE_RECORD=1` into the tree and compared, then restored):
display, model, partition, printed lines, cards, log and RAM equal the
references for every example, with the state files' `PRINTER:` and
`SOLID-STATE-MODULE:` now applied before the state. Only the lines
`playback: ended, recorded after … s` and `Elapsed: …` differ, by up to a
second and a few milliseconds; two recordings of `main` (174b864) differ
from each other in the same lines (23 files), so they depend on the wall
clock and are not a finding.

### Results

| ID | Result | Test case (file) |
|---|---|---|
| AC-1 | Pass (core and app session); the paper output kind and the paper panel: LIB-8 59, M-53 (design §1, D-1) | `c59_6_ac1_printer_plugged_and_removed` (each model: attach, the capabilities `controls` and `c59.printer-output` and the plug entry are there, a second PC-100C and a module in `printer` are refused, detach, none is left, the power cycle runs) (`peripherals_tests.cpp`); `c59_6_ac1_paper_kept_across_detach_and_attach` (a printed line stays on the app session's paper through a detach, an attach and a model switch) (`shared_controller_tests.cpp`) |
| AC-2 | Pass | `c59_6_ac2_rom_finds_the_printer_as_on_the_real_device` (each model: 2nd Prt prints one line with the PC-100C and none without; the TI-58C through its D10 pad; the TI-59's error blink 0.899 s with and 0.449 s without) (`peripherals_tests.cpp`); `c59_1_ac5_error_blink_period_*` (`core_tests.cpp`, the PC-100C now attached at the build) |
| AC-3 | Pass | the golden runs with a printer (above): lines, characters, dots, `rows=`/`partial=` as recorded, read at the PC-100C's `c59.printer-output` |
| AC-4 | Pass | `c59_6_ac4_busy_counts_instructions_idle_once` (the PC-100C alone: busy for exactly 4165 instruction cycles after a line, 1370 after a feed; in a TI-59: `FLG_BUSY` up for 4165 instructions after 2nd Prt); `c59_6_ac4_printer_interrupt_partial_rows` (was `printer_interrupt_partial_rows`, `core_tests.cpp`) |
| AC-5 | Pass | `c59_6_ac5_print_key_prints_display` (was `printer_print_key_emits_line`, `core_tests.cpp`); `c59_6_ac5_adv_feeds_while_held` (a 2 s hold feeds more than a 0.3 s one); `c59_6_ac5_trace_prints_while_on` (4 + 5 = prints three lines with TRACE on, none off); `lib2_ac26_pc100c_print_adv_trace` (PRINT and ADV keys, TRACE switch) (`peripherals_tests.cpp`) |
| AC-6 | Pass (with the 2.3.0 state-file directives); the version-2 key `printer-trace:`: LIB-6 59, M-54 | `c59_6_ac6_trace_survives_reset_power_cycle_cold_boot` (TRACE from the attach request, kept through the three operations; PRINT released) (`peripherals_tests.cpp`); `c59_6_ac6_trace_kept_by_app_session` (off at the start; kept through Reset, Cold boot, a model switch, a detach and an attach; a new app session starts off), `c59_6_ac6_state_file_sets_trace_off_unless_it_sets_it` (a load sets it off, `PrinterTrace: on` sets it) (`shared_controller_tests.cpp`) |
| AC-7 | Pass | golden run `ti59-print-without-printer` (display and tick counts of 2.3.0) |
| AC-8 | Pass (the 14 IDs, names, files, the list in `menu-sort` order); `slot-1: { name: }` and the refusal of `{ file: }`: LIB-6 59, M-54; the picker: LIB-7 59, M-55 / C59-8, M-58 | `c59_6_ac8_fourteen_modules_by_id_description_menu_sort` (the spec's table against the catalogue and each file's `module-name` and `description`; `allModuleMetadata` from the files: ML, LE, MU, EE, RP, ST, RE, …; each module plugs into `slot-1` on each model) (`peripherals_tests.cpp`); `lib1_ac32_pc100c_and_14_modules` (`descriptor_tests.cpp`) |
| AC-9 | Pass | golden runs `ti59-module-ml` and `ti59-module-le` (display, RAM, printed lines); `c59_6_ac9_module_program_runs_from_the_slot` (the fetch latch moves inside the module) (`peripherals_tests.cpp`) |
| AC-10 | Pass | golden runs `ti59-no-module`, `ti58-no-module`; `c59_6_ac10_empty_slot_runs_and_blinks_on_pgm_01` (each model: 2nd Pgm 01 SBR = blinks with an empty slot; 2nd Pgm 30 SBR = blinks with the Master Library; 1 + 1 = does not); `lib2_ac25_empty_slot_machine_runs` (`peripherals_tests.cpp`). The log line "Solid-state module library was not loaded" is gone: an empty slot is a normal machine |
| AC-11 | Pass (core and app session); the attach/detach command of LIB-18: M-54 | `c59_6_ac11_module_change_then_power_cycle_like_fresh_build` (ML → LE by one replace and the power cycle: 2nd Pgm 01 SBR = shows and prints what a machine built with LE does, not what ML does), `c59_6_ac11_printer_attach_detach_then_rom_finds_it` (blink 0.449 → 0.899 → 0.449 s after each power cycle) (`peripherals_tests.cpp`); `c59_6_ac11_change_module_power_cycles` (the TI-59 loses a register after a module change and after a detach; the slot emptied) (`shared_controller_tests.cpp`) |
| EC-1 | Pass | `c59_6_ec1_module_without_rom_refused_file_named` ("The Surveying could not be attached: SY.rom.yaml is missing.", the file in `missingRoms`, nothing plugged, nothing follows) (`peripherals_tests.cpp`). A running product does not meet it: the full set is looked up at the start (LIB-12 AC-5) |
| EC-2 | Pass | `c59_6_ec2_detach_mid_line_keeps_rows_then_power_cycle` (1200 instructions into a line: the code line ends with 2 rows and part of row 3, the detach says a power cycle follows, it runs, no paper capability is left) (`peripherals_tests.cpp`); the app session takes those rows before the detach (`EmulatorSession::setPrinter`) |
| EC-3 | Pass (core); key readiness in typing: LIB-5 59, M-36 | `c59_6_ec3_ti58c_key_readiness_with_and_without_printer` (the TI-58C reaches key readiness and keeps it in the scan loop with and without the PC-100C, also after 2nd Prt) (`peripherals_tests.cpp`); `c59_1_ac18_key_ready_in_scan_loop_each_model_with_and_without_printer` (`machine_tests.cpp`). C59-1 A-5 holds: the static TI-58C ranges are enough, no wired-printer input was needed |

Technical Requirements (review): `TMC0501` holds no printer state and no
module image, only the `TiConnectors` pointers and the `libExecPC` latch;
the PC-100C and the module see only their pin-outs' calls. The printer
pin-out names the fit table's signals, the module slot's its eight
(comments of `core/PrinterPinOut.hpp`, `core/ModuleSlotPinOut.hpp`). The
busy count is per instruction: `TMC0501::step()` gives one Φ1 per
instruction, an idle one included (AC-4).

### Other checks of the step

- Frequent (`ctest --test-dir build/tests`): 12 of 12 pass, 66.7 s
  (Calc-U-59's part 25.4 s, `CalcU59Tests` 8.4 s).
- Web: `frontends/web/build.sh` (Emscripten, Homebrew `emcc`) builds;
  `run-tests.sh`: 65 passed, 3 skipped (the module list now comes from the
  controller's ROM set; "the installed app loads any module and any preset
  offline" passes).
- Swift: `build_xcframework.sh` and `xcodebuild … -destination
  platform=macOS build`: BUILD SUCCEEDED (the module list through
  `TISessionController.allModules`). Not run on iOS.
- Qt6 (`build/all`, Homebrew Qt): builds; starts offscreen and draws the
  calculator with the PC-100C panel (`--grab`).
- Not run: GCC 13 and MSVC (CI, not dispatched).

### Tools and knowledge

- `tools/headless/capi.cpp` on attach/detach and the capabilities;
  `h_load_module` takes the module by its ID from the catalogue (no longer
  from `cuecards.txt`). `ti59.py` names the key `ADV` and keeps `PAPER` as
  an alias (until M-59); its README says so. Run by hand: `ti59.py 7 Prt
  ADV PRINT --printer` (prints, feeds, prints), `Pgm 0 1 SBR = --module LE`,
  `4 Prt --variant TI58C --printer`, `--module ZZ` (refused);
  `tools/agentic/linetimes.py examples/debug/printer-interrupt-bits.ti59`
  and `ti59file.py examples/calendar/calendar-09-dispatch.ti59` (113 lines,
  printer floor 27.6 s). `coverage.py build` was not run (it records every
  scenario); it uses only `h_trace_sw` and the calls above.
- Knowledge: `calc-u-59-core` skill; `reference/CoreArchitecture.md`
  (Component Overview, keyboard signals, Solid-State Library Module,
  Printer (PC-100C) with the PC-100A note, Threading Model);
  `reference/SharedArchitecture.md` (Places, ROM Loading);
  `reference/DebugAPI.md`; `docs/glossary-mapping.md` (code names
  `SolidStateRomModule`, `PC100C`); two product memories
  (`real_hardware_timing.md`, `printer_timing_audio.md`).
- Ledger: rows 59-11 and 59-25 are visible from this step; row 59-25 now
  also names the model switch (AC-6 lists it; today a new machine set TRACE
  off). Confirmed by the user, 2026-11-15.

### Adapters

Removed: C59-1's `TIPeripheralsAdapter` (`peripheralsOf()`); LIB-12's A-2
(`moduleRomEntries()`: the module peripherals carry the 14 `RomEntry`s) and
A-3 (`moduleRomFile` over `cuecards.txt`: now the catalogue's IDs).
Added, as design §5 lists: `c59.printer-output` (until M-53), the app
session's printer and module calls (`EmulatorSession::setModule`,
`setPrinter`, `printerControl`; until M-54/M-57/M-58), the 2.3.0
directives on places (until M-54), the module list from the files for
today's menus (until M-55/M-58), `EmulatorSession::moduleImage()` (until
M-53).

### Deviations from the design

- **The 14 module descriptors are static in `tiCatalogue()`**, not read from
  the module files and added with `addPeripheral` (design §3.6, D-3). The
  full ROM set must know the module files and their md5 before the lookup
  reads them; with the entries in the catalogue, `fullSet(tiCatalogue())`
  takes them from there and A-2 can go. The catalogue's ID and name of each
  module are the spec's table (AC-8); `c59_6_ac8_…` checks them against each
  file's `module-name` and `description`. The picker's list (description,
  `menu-sort`) is read from the files, as designed. `productData` holds no
  `menu-sort` (nobody reads it before LIB-7).
- **Components.** The machine descriptors no longer list `CROM` and
  `printer-interface`; the PC-100C's descriptor lists a functional
  component `PC-100C`, each module's its ROM chip (`TMC0541` …), functional.
- **`FLG_BUSY` stays a CPU flag**: the CPU copies the connector's Busy into
  it after each Φ1 and printer word, because the trace format carries the
  flags word and `tools/agentic` reads `FLG_BUSY` from it (design §3.1
  allows it "if a trace format needs it").
- **`PrinterOutput::endLineHere()`** is added to the product kind: the app
  session ends a line in progress and takes its rows before the detach
  destroys the PC-100C (EC-2). `unplug()` calls it too.
- **`MachinePeripherals`** (`app/ProductRoms.hpp`) is the build request's
  module, printer and TRACE for `buildMachine`, `EmulatorSession` and the
  tests; its default is 2.3.0's core (a PC-100C, no module), so that tests
  that built a bare machine keep their timing. `moduleRequest()` and
  `printerRequest()` make the attach requests.
- **The state file's TRACE**: a load sets TRACE off, then its keystrokes may
  set it (design §4, row 59-25); today a load kept it unless it had a
  `PRINTER:` line.
- **The empty slot in the Settings choice**: `changeModule("")` empties the
  slot (no frontend offers it yet; the picker of M-55 will).
- **Tests kept under their C59-1 names**: `c59_1_ac5_error_blink_period_*`
  stay in `core_tests.cpp` (they are C59-1 AC-5's); AC-2 has its own case.
- **Swift**: the module list comes from `TISessionController.allModules`
  (`EmulatorViewModel.allModules()`), no longer from
  `TIFileFormats.allModuleMetadata(cuecards)`; `ROMLoader.swift` loses
  `loadAllModuleMetadata`. Web: `staticData(controller)` takes the list
  from the controller's ROM set.

### Open (Done in M-58)

C59-6 stays In progress (design D-1): the clauses of AC-1 (paper output
kind, LIB-8 59, M-53), AC-6 and AC-8 (version-2 preset keys, LIB-6 59,
M-54), AC-8 (the picker, LIB-7 59, M-55; C59-8, M-58), AC-11 (LIB-18's
attach/detach command, M-54) and EC-3 (LIB-5's typing, M-36) pass in those
steps.

**M-58 (2026-11-17):** AC-8's picker in the Swift frontend: the picker of
`slot-1` on the bar, from the control state (C59-8 AC-3, AC-12):
`swift_shell_tests.cpp` `c59_8_ac3_bar_per_model` (None and the 14 modules
on each model), XCUITest `FamilyStructureTests.test_lib14_ac8_the_bar`
(LE chosen on the iPhone simulator's bar is plugged); the Settings row
"Solid State Module" and `allModules` went (row 59-9). The other clauses
passed in M-53 to M-55 (above). Every criterion passes: **Done**.

## Release prep step 5 (2026-11-17, branch `rp5-one-rule`)

| ID | Result | Test case |
|---|---|---|
| AC-12 | Pass | `c59_6_ac12_set_control_without_a_pc100c_refused` (`CalcU59Tests`) |
