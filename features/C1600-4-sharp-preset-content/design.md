# C1600-4 Sharp preset content: design
Status: approved by the user at the Part C2/D boundary (2026-11-16)

## 1. Scope

| Row | Step | Tests from (plan §3) | Done in (plan §5.1) |
|---|---|---|---|
| C1600-4 | M-46 (with LIB-6 1600, LIB-18 1600) | AC-1–20 (AC-4 per C-7), EC-1–6 | M-47 (AC-15's disk and waiting presets, AC-16 for them) |

M-46 is one step with three designs: this one (the product's keys,
sections and preset files), the Calc-U-1600 column of LIB-6
(`features/LIB-6-presets/design.md` §10: the product's rules, the
reader of version 1 gone, the frontends and tools on the library's
loader) and that of LIB-18 (`features/LIB-18-machine-control-and-startup/design.md`
§10: the start, the switches and the resets). This design names what
only Calc-U-1600 has; the other two name the wiring.

- **Waits for** (plan M-46): M-4 (FS 13, the reference runs of the 32
  examples, `products/calc-u-1600/tests/reference/v0.8.1-examples/`),
  M-8 (C-7: which models start in program mode, AC-4), M-44 (C1600-2:
  the loaders that `program:` calls), M-45 (LIB-8 1600 and LIB-10 1600:
  `paper marks`, `peek`, the `trace:` step on the Sharp models, which
  AC-16 and the upgrade of `trace:` presets use).
- Uses the library as it is on `main` after C2 (`60e5b09`):
  `library/presets/` (LIB-6 L, Done), `library/media/` (LIB-7 L, Done;
  its product kinds come only in M-47).
- Paths below without a prefix are under `products/calc-u-1600/`. Line
  numbers are at `60e5b09`; the spec's `Core/`, `Shared/`, `Qt6/`,
  `Web/` paths are at the pin `b4ea8c8` (`core/`, `app/`,
  `frontends/qt6/`, `frontends/web/` here).
- Hardware facts are not restated here; they are in the research
  skills `PC1500` (memory maps, bus, LH5801/LH5803 ME, PV, PU) and
  `pc1500-build` (assembling the test ROMs and programs a test needs).

**What waits for M-47** (plan question 6, applied): the presets that name
a removable medium or save one cannot be read before LIB-7's Calc-U-1600
kinds exist, since `floppy:`, `cassette:`, `eject:` and `saveas:` are steps
of those kinds (LIB-7 §3.8, `library/media/PresetMedia.cpp:443-494`) and
`slot-N: { file: x.module.yaml }` is read through the kind of its suffix.
They are, by the rule of LIB-6 AC-51 ("the presets that can only be
upgraded when LIB-7 … is built"):

| Preset | Why it waits |
|---|---|
| `examples/dwx/DiskWorks.pc1600` | `floppy-file:`, `slot-2-file:` |
| `examples/dwx/make_diskworks_media.pc1600` | `floppy:`, `saveas:` (two) |
| `examples/memory/memory-cards/pc1500-maxed-out.pc1500` | `slot-1-file:` |
| `examples/startup/default-pc1600.pc1600` | `floppy: Formatted` |
| `tests/fixtures/presets/startup-pc1600.pc1600` | `floppy: Formatted` |

The last two are not in the spec's list of three (D-3, Plan note P-1).
Every other preset is upgraded in M-46. No preset of the repository uses
`- trace:` in a way LIB-10 1600 (M-45) does not read (the dev presets
`dev/presets/trace_demo.pc1500a` and the CE-1638 setups are upgraded by
LIB-10's row of AC-15's table in M-46).

## 2. Modules and files

### New in `app/`

| File | Holds |
|---|---|
| `app/SharpPresetKeys.hpp/.cpp` | `shared::addSharpKeywords(presets::Vocabulary&, const SharpKeywordSetup&)`: the section `program:` (AC-1–5), the header keys `host-drive:` (AC-6, AC-7), `slot-1-rom:`, `slot-2-rom:` (AC-8), `bus-rom:` (AC-9, AC-10), `debug:` (AC-11, AC-12, AC-20); their `ProductValue`s `SlotRomValue`, `DebugBlock` (§3; `HostDriveValue` and `BusRomList` went in Part D simplification P-6, §11) |
| `app/SharpDebugBlock.hpp/.cpp` | `shared::readDebugBlock(const formats::YamlNode&, ReadContext&) -> std::shared_ptr<const DebugBlock>` (AC-11, AC-14 "starts with `debug:`") and `shared::debugBlockOf(path)` for the debugger (AC-12; §3.4) |
| `tests/preset_content_tests.cpp` | AC-1–14, AC-19, AC-20, EC-1–6 (§6) |
| `tests/example_references_tests.cpp`, program `CalcU1600ExampleReferences` | AC-16 (§6, §8) |
| `tests/reference/ReferenceOut.hpp/.cpp` | the reader of the M-4 `.out` sections (display text, BASIC pointers, program area, machine-code area) for AC-16 |
| `examples/bundle.yaml` | the bundle file of AC-17 (`docs/formats/bundle.md`) |

### Changed

| File | Change |
|---|---|
| `core/Machine/SharpCatalogue.cpp:79-81, 111-115, 171-179` | the place `bus-rom` on both models (60-pin, the bus pin-out, any number); the peripheral `bus-rom` (fits the bus pin-out of both models; no ROM entry of its own: its bytes come with the request, `BusRomRequest`) (AC-10; C1600-1 OQ-2 "C1600-1 then gets the place") |
| `core/Machine/SharpMachine.hpp:30-46`, `SharpMachine.cpp`, `SharpMachineBuilder` | `BusRomRequest{bytes, bank \| address, me, pv, pu}` and `SlotRomRequest{bytes}` as `PeripheralRequest::content`; the builder puts the bus ROMs in front of the chain, the last item first (AC-10: "a ROM that comes later is found before an earlier one, and before a peripheral's ROM"; today `core/Preset/PresetBusRomLoader.hpp:58-62`); a module request with `SlotRomRequest` builds its card from the definition with those bytes as its ROM (today `core/Preset/PresetRunner.cpp:23-47`, `makePresetModuleCard`). A `bus-rom` request without content is refused: "A bus ROM is plugged in by a preset only." (D-5) |
| `app/ControlState.cpp` | the place `bus-rom` gives no control (AC-10, OQ-2) |
| `app/HarnessProduct.cpp` | `presetRules()` (LIB-6 §10), and through it these keywords |
| the 32 examples, `tests/fixtures/presets/*` (5), `dev/hardware-checks/*.pc1500a` (3), `dev/presets/*` (4), `docs/developer/screenshots/presets/*` (9), `vscode/calcu1600-debug/presets/*` (2), `vscode/calcu1600-debug/templates/*/debug.*` (4) | version 2 by AC-15's table, except the five of §1 (M-47) |
| `tools/dap_smoke.py:232, 280, 301`, `dev/tape-matrix/run.py:44, 67, 91`, `dev/loader-matrix/pc1500_loader_matrix.cpp:66`, `pc1600_loader_matrix.cpp:90`, every test that writes a preset as text (`tests/preset_tests.cpp`, `pc1600_preset_tests.cpp`, `presetloader_trace_tests.cpp`, `ce150_tests.cpp:273`, `ce158_tests.cpp:309`, `lh5801_tests.cpp:935`, `memory_card_tests.cpp:966, 991`, `job_tests.cpp:217`, `lcd_screenshot_tests.cpp:139-160`, `session_tests.cpp:52`, `typing_tests.cpp:317`, `app/tests/shared_app_session_tests.cpp:47, 83, 367`) | they write version 2 (AC-15 "presets that a tool or a test writes"); the tests that check the old reader go or are ported (§6, AC-19) |
| `vscode/calcu1600-debug/scaffold.js:14-17, 58` | unchanged code; the templates it fills are version 2 (AC-15) |
| `examples/README.md`, `examples/setup/README.md:17-21`, comments in the presets (`interfaces/ce158_demo.pc1500a:7-8` and the like) | corrected where they name a key, a tool or a format that changed (AC-15) |
| `examples/bundled.yaml` | deleted (AC-17) |

### Deleted (the reader of version 1; LIB-6 §10 names the rest)

`core/Preset/PresetBusRomLoader.hpp`, `PresetDebugBlock.hpp` (its rules
move to `app/SharpDebugBlock.*`), `PresetInterface.hpp`, `PresetRomSet.hpp`,
and the `program:`, `bus-rom:`, `debug:` parts of `PresetFile.cpp`
(`parseProgramBlock` 393-537, `parseYamlBlock` 542-566, `parseBusRoms`
576-642). `core/Preset/PresetMedia.hpp` and `core/PC1600/PC1600PresetMedia.hpp`
stay until M-47 (LIB-7 A-3: the CLIs' `saveas`).

## 3. Seams and interfaces

### 3.1 What the product registers (TR: "through the place LIB-6 AC-29 gives")

```cpp
namespace shared {
struct SharpKeywordSetup {
    bool hostFolders = true;                   // AC-7: false in the Web frontend
    const ProgramLoaders* loaders;             // C1600-2 (M-44): kind, BASIC, typing in, machine code
};
void addSharpKeywords(calcu::presets::Vocabulary&, const SharpKeywordSetup&);   // called from SharpPresetRules::addKeywords (LIB-6 §10)
}
```

| Keyword | Kind | Read checks (at read, with the line; AC-14) | Carried out | After |
|---|---|---|---|---|
| `program:` | section, a block | AC-1's fields once each; exactly one of `file:` / `text:`; `text:` not with `typed:`, `address:`, `length:`; the file read and its kind told by C1600-2's classifier (AC-2); the fields its kind takes; `text:` with no line (EC-1); no block, an unknown field (EC-2); a relative path without a folder (EC-5, LIB-6 EC-5) | the loader for the kind (AC-3); `text:` typed line by line by C1600-2's typing loader, which reports lines not stored (AC-4); the lines of `text:` as LIB-6 AC-1's block scalar (AC-5) | `Idle` (EC-3: the next item starts when the program has finished; a machine-code program that never returns fails at `timeout:`, LIB-6 AC-57) |
| `host-drive:` | header key under the place name `host-drive` (D-1) | `{ folder: <path> }` only; the folder exists (relative to the preset's folder, `~/`); the model has the place (else LIB-6 AC-62's sentence); `hostFolders` false: "The host drive is not available in this frontend." (AC-7) | the request for the place `host-drive` with `HostDriveRequest{folder}` (`core/Machine/SharpMachine.hpp:41-45`), through `ReadContext::addPeripheral` (D-2, §11) | — |
| `slot-1-rom:`, `slot-2-rom:` | header key, read after the other header keys | `{ file: }` only; a module named for the slot (`ReadContext::pluggedAny`); that module a ROM module of one region (`preset.md` §3.2); the file's size that region's size; `slot-2-rom` on the PC-1600 only | the slot's request gets `SlotRomRequest{bytes}`, read anew at each load (TR) | — |
| `bus-rom:` | header key, a list | each item `file:` and exactly one of `bank:` (4–7, PC-1600 only; EC-6 "the PC-1500 and PC-1500A take `address:`") or `address:` (&0000–&FFFF), with `address:` optionally `me:`, `pv:`, `pu:` (0 or 1); the file readable, not empty, not larger than its bank or than the room to &FFFF; numbers by `formats::parseNumber` | one request per item at the place `bus-rom`, in list order (the builder reverses it, §2) | — |
| `debug:` | header key, a mapping | AC-11's keys in camelCase (`docs/formats/README.md` §1), their values and paths; an error starts "debug: " (AC-14) | nothing (AC-12); kept as `DebugBlock` for the debugger | — |

`program:` and every header key of this table are marked by
`SharpModelRules::applies()` with `preset.md` §7 (LIB-6 §10). A `debug:`
block with a mistake refuses the preset also without a debugger (EC-4).

### 3.2 Into the machine plan (AC-6, AC-8, AC-10)

The library's `machinePlan(LoadPlan)` (`library/presets/Engine.cpp:26-32`)
turns only the place keys into requests. M-46 adds one hook to LIB-6
(LIB-6 §10.3): `ModelRules::amendPlan(const LoadPlan&, startup::MachinePlan&)`,
empty by default, called at the end of `machinePlan`. Calc-U-1600's
rules read their kept values (`LoadPlan::productHeader`) and add the
host drive's and the bus ROMs' requests and the slot ROMs' bytes.
(Part D simplification P-6, §11: the header keys add them through
`ReadContext` as they are read; `amendPlan` is gone.) A
build refusal of such a request names no key's line; it is given at
the line of `model:` (LIB-6 qa-report, deviation "AC-52"). Every check
that can be made without a machine is made when the preset is read
(§3.1), so this happens only when the build itself refuses.

### 3.3 The loaders (AC-3, AC-4)

`program:` calls C1600-2's loaders (M-44) through one interface the
section holds, `shared::ProgramLoaders` (seam S-1, §3.5): classify bytes
(kind), and per kind an awaitable inside the operation (`load(op,
machine, ProgramRequest)` with `typed`, `address`, `length`) that
returns ok or the loader's reason. A failure stops the preset with that
reason at the section's line (LIB-6 AC-17). For `text:`, the typing
loader reports the lines that were not stored; the section fails with
"no line was stored; is the machine in program mode?" or names the
lines (AC-4; today `core/PC1600/PC1600BasicTyper.cpp:333-340`).
Program mode is the preset's own `- key: mode` (AC-4); C-7 of M-8
confirmed which models start where, and the upgrade (AC-15) keeps each
preset's `key: mode` as it is.

### 3.4 The `debug:` block for the debugger (AC-12)

The Qt6 debugger reads the block today with `parsePresetFile`
(`frontends/qt6/app/debug/DapSession.cpp:212-248`, at 221).
`shared::debugBlockOf(path)` reads the preset with the library's reader
(`presets::readPreset` with the product's vocabulary) and returns the
kept `DebugBlock`, or the read error. `DapSession::effectiveConfig`
calls it. C1600-3 (M-50) moves `DapSession` into the app layer and
keeps this call.

### 3.5 Seams assumed of the parallel D designs

| # | Of | What this design takes | If it is not so |
|---|---|---|---|
| S-1 | C1600-2 (M-44) | one classifier and one awaitable per loader inside an operation, on the loader memory access kind, with a reason sentence on failure; the typing loader reports lines not stored | `program:` wraps whatever entry M-44 leaves; the section's checks stay the same |
| S-2 | LIB-10 1600 (M-45) | the step `trace:` on the Sharp models, and `peek` for AC-16 | AC-16 compares the program area through `pc1600 …` dumps of M-51 instead, and C1600-4 Done stays M-47 either way |
| S-3 | LIB-8 1600 (M-45) | `paper marks` for the CE-150 and the CE-1600P | — (LIB-8 AC-39's replies exist) |

## 4. The shipped presets (AC-15, AC-17, AC-18)

**The upgrade.** By hand, file by file, by AC-15's table and `preset.md`
§9.2, in one commit per folder, each folder checked by
`calcu check-preset` before the next (§6, T7). No converter is written:
62 files, most of them a few lines, and the rows that need judgement
(`skip-wait:`, `let-it-finish:`, D-4) are the ones a converter cannot
make (`docs/research/preset-conversion-check.md` §3). Rules settled here:

- A bare `- wait:` as the last item becomes `- let-it-finish:`; elsewhere
  it goes (the nine plotter examples end with it today; `globus` and
  `lissajou-1500` are named in the conversion check).
- `pc1600-hanoi-run` (screenshots) gets `- skip-wait:` before `- wait: 3s`
  (conversion check §3).
- A header key given twice: the last is kept (AC-15).
- `slot-N: CE-1600M` → `slot-N: { name: CE-1600M }`: read in M-46 by
  LIB-6's own `{ name: }` match against the catalogue's module
  definitions (bundled templates; LIB-6 D-22). From M-47 LIB-7's
  resolver reads the same text and finds the same templates.
- `host-drive: S3` → `host-drive: { folder: S3 }` (DiskWorks, in M-47).
- The VS Code templates are upgraded by hand with their `{{name}}`
  placeholders (AC-15) and checked filled in (LIB-6 AC-51; the fill
  rule of `scaffold.js:58`, C1600-3 AC-19), through `FILL` of
  `calcu_add_preset_check` (LIB-6 D-20).
- The five presets of §1 stay at version 1 in M-46 and are excluded by
  name from the suite check; M-47 upgrades them (`- floppy: { name:
  Formatted }` in a first `keys:` section, `saveas:` as a mapping).
- `examples/dwx/Progs.floppy.yaml` goes to version 2 in M-47, with the
  bundled disks (LIB-7 §10).

**The bundle file** `examples/bundle.yaml` (AC-17): `startup:` the three
startup presets, `groups:` BASIC (`basic/hanoi.pc1600` with
`basic/hanoi.bas`), Plotter (the five with their `.bas`), Machine code
(`machine-code/memtest_1500a.pc1500a` with `memtest_1500a.bin`), in
today's order (`examples/bundled.yaml:12-31`). `startup/default-pc1600.pc1600`
waits for M-47 (§1): it is left out of `startup:` in M-46 and added in
M-47 (plan question 6: "they stay out of the bundle file until their
step"). Between the two steps the Settings offer no bundled startup
preset for the PC-1600, and a PC-1600 whose settings still name it
starts as its built-in default with the notice of LIB-18 EC-1. Not a
release (plan §4); not a ledger row.

**Bundling** (AC-17, row 1600-21): `frontends/qt6/CMakeLists.txt:214-252`
and `frontends/web/build.js:60-79` read `bundle.yaml`'s `{ file: … }`
references (presets and their files) and copy exactly those, not their
folders. The CMake side reads the file with a regular expression as
today; a check (`c1600_4_ac17_bundled_files_are_the_bundle_files`, §6)
compares the Qt `resources/examples` and the Web `dist/presets` lists
with `readBundleFile`'s. `release.yaml:31` (the examples zip) is M-61's
(plan §5.2, 1600-21).

**The name check** (AC-18): the library's `checkBundle` gains
`presetNameProblems` (LIB-6 §3.7: "moves with `app/PresetCatalog.*` in
M-46"): a `preset-name` over 18 code points, no `description`, listed
twice, a name twice per model. `app/PresetCatalog.*` goes.

## 5. Adapters

This row removes, with LIB-6 1600 and LIB-18 1600 (their §10 list the
rest): `core/Preset/PresetBusRomLoader.hpp` (the bus ROMs as a card put
in front, now the builder's), `PresetDebugBlock.hpp`, `LegacyCalls::applyPresetJob`
(`core/Machine/LegacyCalls.hpp:52-55`) and `syncPlugged` (`:59`).
It adds none.

## 6. Tasks

Tests come before the code. `CoreTests` (Frequent) unless the row says
otherwise; ROM-gated cases `SKIP_UNLESS` the ROMs, as today. Cases run
the product's harness entry in virtual time (`calcu --product 1600`
in-process, `tests/harness_tests.cpp`'s helper) or the product's rules
on `presets::readPreset` directly. Names `c1600_4_acN_…`.

| Task | What | ACs/ECs | Test (file: cases) |
|---|---|---|---|
| T1 | `program:` read | AC-1, AC-2, AC-5, EC-1, EC-2, EC-5 | `preset_content_tests.cpp`: `c1600_4_ac1_fields_once_one_of_file_text`, `c1600_4_ac1_text_not_with_typed_address_length`, `c1600_4_ac1_sections_in_file_order_any_number`, `c1600_4_ac2_kind_told_at_read_field_it_does_not_take_refused`, `c1600_4_ac2_unreadable_empty_not_a_program_refused`, `c1600_4_ac2_tape_definition_is_not_a_program_file`, `c1600_4_ac5_blank_and_hash_lines_left_out_space_hash_typed`, `c1600_4_ec1_text_without_lines_refused`, `c1600_4_ec2_no_block_or_unknown_field_refused`, `c1600_4_ec5_relative_path_without_folder_refused` |
| T2 | `program:` carried out | AC-3, AC-4, EC-3 | `c1600_4_ac3_basic_listing_tokenized_loaded`, `c1600_4_ac3_typed_true_typed_in`, `c1600_4_ac3_machine_code_at_address_and_started`, `c1600_4_ac3_loader_failure_fails_preset_with_reason`, `c1600_4_ac4_text_typed_in_program_mode`, `c1600_4_ac4_not_in_program_mode_fails_naming_lines`, `c1600_4_ec3_second_section_after_first_finished`, `c1600_4_ec3_code_not_returned_fails_at_timeout` (`timeout: 1`) |
| T3 | `host-drive:` | AC-6, AC-7 | `c1600_4_ac6_folder_shown_as_s3` (`FILES "S3:"` lists a file of a temporary folder), `c1600_4_ac6_missing_folder_or_not_pc1600_refused_at_read`, `c1600_4_ac7_refused_where_frontend_mounts_no_folder` (rules with `hostFolders = false`) |
| T4 | ROM images, bus ROMs | AC-8, AC-9, AC-10, EC-6 | `c1600_4_ac8_slot_rom_bytes_read_at_every_load` (the file changed between two loads), `c1600_4_ac8_no_module_twice_unreadable_wrong_size_refused`, `c1600_4_ac8_module_with_slot_rom_never_saved` (no autosave, the bay does not write it; M-47 again through LIB-7's `saveas`), `c1600_4_ac9_forms_and_ranges`, `c1600_4_ac9_twice_refused`, `c1600_4_ac10_later_rom_found_first_and_before_peripheral` (two bus ROMs and a CE-158 at one address; the test ROMs built per `pc1500-build`), `c1600_4_ac10_listed_in_plugged_no_control`, `c1600_4_ac10_attach_command_refused`, `c1600_4_ec6_bank_on_pc1500_refused` |
| T5 | `debug:` | AC-11, AC-12, AC-20, EC-4 | `c1600_4_ac11_keys_values_paths_from_preset_folder`, `c1600_4_ac11_unknown_key_wrong_value_refused`, `c1600_4_ac12_load_does_nothing_with_block`, `c1600_4_ac12_debug_block_of_for_the_debugger`, `c1600_4_ac20_blocks_by_the_file_syntax` (every block form of today's tests), `c1600_4_ec4_mistake_refused_without_debugger` |
| T6 | Errors, today's tests | AC-13, AC-14, AC-19 | `c1600_4_ac14_line_key_and_expected_debug_and_bus_rom_prefixed`; AC-13 is LIB-6 AC-45, AC-46, AC-48 (LIB-6 §10.6); AC-19: the cases of `preset_tests.cpp:480-517, 568-655, 732-850` and `pc1600_preset_tests.cpp:846-1019` (at the pin; today's `test_slot_rom_keys`, `test_parser_*`, `test_loader_machine_binary_*`, the bus-rom and debug cases) ported onto the new reader as `c1600_4_ac19_<old name>`; the old files lose them |
| T7 | The upgrade | AC-15 | `calcu_preset_check.CalcU1600Presets` (LIB-6 §10.6, Frequent) over every file of the table of §2, the templates filled; `EXCLUDE` the five of §1 (M-47 removes the exclusions); `c1600_4_ac15_no_version_1_left` (a scan of the tracked files for `format-version: 1` in a preset, the five excepted until M-47) |
| T8 | References | AC-16 | `example_references_tests.cpp`, program `CalcU1600ExampleReferences` (Longer): per example, in a copy of `examples/` in a temporary folder, `preset examples:<path>` (the bundle copy) or the file, then `wait`, then `lcd`, `paper marks`, `peek` of the ranges the `.out` inspector views list (BASIC pointers, program area, machine-code area); compared with `.out` and `.paper-marks`; the RTC cells and tick counts left out; the intended differences of `docs/research/preset-conversion-check.md` §4 excepted by name with their reason (`Calculat` and the like); the five of §1 listed as waiting (M-47) |
| T9 | Bundle, names | AC-17, AC-18 | `c1600_4_ac17_bundle_lists_todays_entries` (`readBundleFile` gives today's list and order), `c1600_4_ac17_bundled_files_are_the_bundle_files` (CTest `CalcU1600BundledFiles`, `cmake -P` over the Qt and Web build trees; Longer: it needs both builds), `c1600_4_ac18_names_short_described_once_unique_per_model` (`checkBundle`; ports `app/tests/shared_preset_catalog_tests.cpp:34-99`) |

Coverage: T1–T9 cover AC-1 to AC-20 and EC-1 to EC-6. AC-15 and AC-16
for the five presets of §1 pass in M-47 (LIB-7 §10.6, T8 there).

## 7. Tools and agent knowledge

**Tools.** `tools/dap_smoke.py`, `dev/tape-matrix/run.py` and the two
loader matrices write version 2 (§2). `pc1500_cli`, `pc1600_cli`,
`pc1600_plotter_probe` load through the library's loader (LIB-6 §10.4).
`tools/check_presets.sh` goes (LIB-6 §10).

**Agent knowledge in M-46** (with LIB-6 §10.7 and LIB-18 §10.7):

| File | Change |
|---|---|
| `products/calc-u-1600/docs/User-Guide.md` (§8 Presets, `:674-911`) | the version 2 keys of this design and `preset.md`; the full rewrite into help chapters is M-52 |
| `products/calc-u-1600/docs/Debugger.md` | `slot-N-rom: { file: }`, `bus-rom:`, `debug:` in version 2 |
| `products/calc-u-1600/docs/PC1600-Host-Drive.md` | `host-drive: { folder: }` |
| `products/calc-u-1600/docs/developer/screenshots/README.md` | presets in version 2 (plan M-46) |
| `products/calc-u-1600/vscode/calcu1600-debug/README.md` | the templates' format |
| memory `calc-u-1600/project_pc1600_native_basic_and_preset_program.md` | "the preset author owns the mode" stays; `typeBasicProgramText` → C1600-2's typing loader, `program: { text: }` |
| memory `calc-u-1600/project_pre_1_0_format_decisions.md` | `slot-N-rom: { file: }`; presets of version 1 refused with the hint (ADR 0037), not loaded with a pop-up |
| memory `calc-u-1600/feedback_pre_1_0_breaking_changes.md` | one line: version 1 presets get the outdated hint of LIB-6 AC-8 (ADR 0037); renamed keys inside version 2 still fail as unknown keys |
| `calc-u-1600-memories.md` (the index) | the descriptions of the three entries above |

**Ledger rows that become visible in M-46:** 1600-1 (presets), 1600-3
(the syntax: `printer:`, `{ name: }`, `{ file: }`, `{ folder: }`),
1600-11 (a host drive folder or a ROM image that does not fit refuses
the preset when it is read), 1600-21 (the bundle file; plan §5.2). The
media rows wait for M-47 (LIB-7 §10.7). No new row.

## 8. Golden and reference runs, risks

- **Reference runs (AC-16).** The M-4 records are CLI runs at 0.8.1; the
  new runs go through the harness at the idle machine. Expected
  differences beyond §4 of the conversion check: the display after the
  last item where version 1 waited for it and version 2 does not (row
  1600-30); these are named per example with their reason, never by
  changing a reference (CLAUDE.md: a difference is a finding).
- **The bus order** moves from the preset loader into the builder. The
  golden runs build no bus ROM, so they cannot differ; T4 checks the order.
- **`slot-N: { name: }` in M-46** finds only catalogue modules
  (bundled templates). A user whose startup preset names a saved
  instance by name gets it refused until M-47 (no release in between).
- **Hand upgrade of 62 files.** Each folder is checked by
  `check-preset` and, for the examples, by T8 in the same step.
- **`CalcU1600ExampleReferences` in Longer.** It runs the 32 examples
  at full speed in virtual time; `Calculat` and the plotter examples
  take longest. It runs in the step (memory longer-tests-allowed) and at
  the end of Part D.

## 9. Decided (best guess)

- **D-1 (`host-drive:`).** A product header key under a place's name;
  LIB-6's reader asks the vocabulary before the descriptor's places
  (LIB-6 §10.3, D-22 of LIB-6 already says such a key is the product's).
- **D-2 (into the plan).** `ModelRules::amendPlan` (one hook) rather
  than header keys that write into the plan: the reader stays the only
  writer of `LoadPlan`, and the product's values are kept as LIB-6 keeps
  `debug:`. Superseded by Part D simplification P-6 (§11).
- **D-3 (what waits).** A preset waits for M-47 when it names a
  removable medium or a module by file or saves one; that is AC-51's
  rule, which adds `default-pc1600` and the `startup-pc1600` fixture to
  the three the spec names (Plan note P-1).
- **D-4 (no converter).** The upgrade is by hand; the suite check and the
  reference runs check it.
- **D-5 (a bus ROM only by a preset).** The `bus-rom` peripheral has no
  ROM entry and no content of its own; a request without
  `BusRomRequest` is refused by the builder, so neither the harness's
  `attach` nor a control can plug one in (AC-10, OQ-2).
- **D-6 (one place, many ROMs).** `bus-rom` is one place that takes any
  number of bus ROMs, in plug order; the order rule is the builder's
  (C1600-1 AC-17's exception for bus ROMs).

**Plan note P-1.** M-46's "Does" lists "the presets that waited" by the
last row of AC-15's table only. Add: the presets with a `floppy:`
header (`startup/default-pc1600.pc1600`, `tests/fixtures/presets/startup-pc1600.pc1600`)
wait for M-47 too; `default-pc1600` is out of `bundle.yaml` until then.
§5.1 is unchanged (C1600-4 Done in M-47).

## 10. Questions for the user

None that splits this design. (The golden-run question of M-36 is asked
in LIB-6 §10.10.)

## 11. Part D simplification P-6 (2026-11-16, branch `ds-names`)

Departure from D-2 and §3.2 (`docs/reviews/d-simplification.md` S-8):
`host-drive:` and `bus-rom:` hand their requests to the reader as they
are read (`ReadContext::addPeripheral`), `slot-N-rom:` the content of
its module's request (`ReadContext::setContentAt`); the reader writes
them into the `LoadPlan` (`headerPeripherals`), and `machinePlan` puts
them after the place keys' requests in the order of the model's places
(the host drive before the bus ROMs, whatever the order in the file),
the bus ROMs in list order: the order `amendSharpPlan` gave. `amendPlan`,
`amendSharpPlan`, `HostDriveValue` and `BusRomList` are gone; the slot
ROM is still kept for `checkSharpHeader` (D-32). LIB-6 design D-33,
§10.9a; test `c1600_4_ac10_header_peripherals_in_plan_order`.
