# LIB-13: QA report

Results per acceptance criterion and edge case of `spec.md`, per column.

## Library (M-42, 2026-11-15, branch `m42-open-file`)

Built and run on an M4 Pro, macOS 27.2, Apple clang, CMake, Ninja,
RelWithDebInfo, in two trees: `build/tests` (`-DCALCU_FRONTEND_QT=OFF`) and
`build/qt` (`-DCALCU_FRONTEND_QT=ON`, Homebrew Qt 6); the Web test product
with Homebrew Emscripten in `build/web`.

The code:
- `library/opening/` (`calcu::opening`, in `calcu_library`; D-1):
  `OpenRequest.*` (`Way`, `OpenedFile`, `OpenRequest` to and from `Value`,
  the command `open`, the request `open-would-take`, `OpenReply`,
  `WouldTake`), `FileKind.hpp`, `Openers.hpp` (`PresetOpening`,
  `MediaOpening`, `MediumFile`), `LibraryOpeners.*` (`PresetsOpener` over
  LIB-6's `PresetsPart::loadPreset`, `MediaOpener` over LIB-7's
  `MediaPart::readOpened`, `placesFor`, `placesInCatalogue`, `refusesNow` and
  `openFile`), `Classify.*` (the tests of AC-1, `looksLikePreset`,
  `wouldTake`), `OpenPart.*` (`OpenSetup`, the part);
- `library/sessions/`: `OperationResult::result` (passed to the reply when
  the operation ends well) and `SessionContext::startOperation` (no reply at
  its end; a failure is a notice); a commit of their own;
- `library/presets/PresetsPart.*`: `loadPreset` (the command `load-preset`
  without its notice; the command adds it as before);
  `library/media/MediaPart.hpp`: `openFile` public;
- `library/harness/OpenCommands.cpp`, `Commands.hpp`, `Product.hpp`
  (`presetExtensions()`, `fileKinds()`, `openLimits()`), `Run.hpp`,
  `Run.cpp` (an `OpenPart` over the run's LIB-6 and LIB-7 parts, no upload
  folder);
- the adapter of §5: `products/calc-u-1600/core/DropFile.cpp`'s
  `looksLikePreset` calls `calcu::opening::looksLikePreset`
  (`test_drop_targets` of `program_file_tests.cpp` passes);
- LIB-1 design §2a: the area `opening` and its dependency line;
- tests: `library/opening/tests/` (`TestOpeners.hpp`, four `*_tests.cpp`),
  `library/sessions/tests/operation_tests.cpp` (three cases),
  `library/harness/tests/commands_tests.cpp` (one case);
  `library/media/tests/MediaFixture.hpp` got `FixtureOptions::settingsIni`
  and `moreParts`.

Result of the run: `calcu_library_tests` 923 tests (51 of them M-42's: 47
in `library/opening/tests/`, 3 in `operation_tests.cpp`, 1 in
`commands_tests.cpp`), 6,277 checks, 0 failed. Frequent: 32 of 32 CTest
entries in `build/tests`, 34 of 34 in `build/qt` (`calcu_qt6_tests` 47,
`calcu_qt6_scenarios` 9); `LibraryWebNodeTests` 1 of 1;
`LibraryWebPlaywright` 138 cases, 136 passed, 2 skipped (as before). CI was
not dispatched (plan §1).

| ID | Result | Test case (file in `library/opening/tests/` unless named) |
|---|---|---|
| AC-1 | Pass | `ac1_unreadable_large_hidden_refused_first`, `ac1_preset_extension_before_medium_and_kinds`, `ac1_medium_suffix_before_kinds`, `ac1_kinds_in_product_order`, `ac1_model_line_after_kinds`, `ac1_nothing_refused_with_notice_naming_file`, `ac1_names_compared_without_case` (`classify_tests.cpp`); through the real parts: `open_preset_through_lib6_part` (an extension in another case, a model line as an upload), `open_medium_through_lib7_part` (`real_parts_tests.cpp`) |
| AC-2 | Pass | `ac2_model_line_in_first_column`, `ac2_indented_or_commented_model_not_a_preset`, `ac2_zero_byte_or_bad_utf8_not_a_preset`, `ac2_bom_allowed` (`classify_tests.cpp`; the cases of Calc-U-1600 `tests/program_file_tests.cpp:198-208`, which still pass through the adapter) |
| AC-3 | Pass | `ac3_reply_says_preset_medium_kind` (a preset from its folder, an upload and a file without a path as content; a medium with its place; a kind, also one whose open is an operation, replied at its end), `ac3_reply_says_kept`, `ac3_reply_says_why_refused` (`open_tests.cpp`) |
| AC-4 | Pass | `ac4_medium_checked_first_hint_passed_on`, `ac4_used_where_it_lies`, `ac4_template_not_written`, `ac4_holding_place_inserted_after_eject`, `ac4_module_one_slot_plugged`, `ac4_module_two_slots_refused`, `ac4_no_place_now_refused_says_where`, `ac4_upload_imported_then_placed`, `ac4_upload_name_taken_asks_then_replace` (`open_tests.cpp`, stand-ins); `open_medium_through_lib7_part` (LIB-7's part: inserted in `disk` from where it lies and written back there, not copied; an upload imported then inserted, the name taken asks; a card on the test-dot refused saying where it goes; an outdated disk with LIB-7's hint), `ac4_pass_through_follows_product` (Part D P-3: a card on the test-seg fitted, not fed; fed by each side and written back where it lies; another refused while it passes) (`real_parts_tests.cpp`) |
| AC-5 | Pass (at the session) | `ac5_every_way_in_same_outcome_but_limit` (`ways_tests.cpp`: a kind, a medium and a refused file by drop, open-with, start, dialog and harness; only the limit differs). The wiring of each frontend's ways in is a product criterion (AC-14, AC-21; design §1) |
| AC-6 | Pass (at the session) | `ac6_would_take_by_tests_1_to_3_and_kind_names`, `ac6_would_take_ignores_content`, `ac6_size_at_drag_held_against_limit` (`ways_tests.cpp`) |
| AC-7 | Pass | `ac7_kept_while_operation_runs_opened_when_free`, `ac7_kept_before_machine_built`, `ac7_second_replaces_first` (a log line names the one replaced, D-6), `ac7_kept_file_refused_later_is_a_notice` (`open_tests.cpp`); `part_starts_operation_without_command`, `part_operation_refused_while_one_runs` (`library/sessions/tests/operation_tests.cpp`) |
| AC-8 | Pass (at the session) | `ac8_several_files_one_folder_preset_found_by_path`, `ac8_first_preset_by_name`, `ac8_no_preset_first_file_alone` (with and without an upload folder, D-7) (`open_tests.cpp`) |
| AC-9 | Pass | `ac9_open_path_replies_as_command` (`library/harness/tests/commands_tests.cpp`: a preset, a medium in `disk`, a file nothing opens, a missing file, the usage) |
| AC-10 | Pass | `ac10_limit_per_way_in`, `ac10_way_without_limit_takes_any_size` (`classify_tests.cpp`) |
| EC-1 | Pass | `ec1_outdated_preset_extension_is_preset_refused_with_hint` (`classify_tests.cpp`); through LIB-6: `open_preset_through_lib6_part` (one notice, the refusal's sentence) |
| EC-2 | Pass | `ec2_no_drive_refused_in_place_imported_as_upload` (`open_tests.cpp`) |
| EC-3 | Pass | `ec3_preset_alone_as_content_refused_when_it_names_files` (`open_tests.cpp`) |
| EC-4 | Pass | `ec4_place_passing_medium_refuses_nothing_imported` (`open_tests.cpp`) |
| EC-5 | Pass | `ec5_unreadable_file_refused_notice_names_it` (`classify_tests.cpp`) |
| EC-6 | Pass | `ec6_no_answer_nothing_imported_nothing_placed`, with `ac4_upload_name_taken_asks_then_replace` (`open_tests.cpp`) |
| EC-7 | Pass | `ec7_no_way_in_changes_open_folders` (`real_parts_tests.cpp`: a settings file with `[folders]`; a preset and a medium opened by every way; the file unchanged after the session is closed) |

Support: `open_request_value_round_trip` (`open_tests.cpp`, L1),
`operation_result_payload_in_reply` (`operation_tests.cpp`, L2).

Coverage: AC-1 to AC-10 (10 of 10) and EC-1 to EC-7 (7 of 7) pass. The
Library column is **Done** (`features/INDEX.md`).

### Deviations from the design, chosen by the simpler structure

- **`MediaOpening` names places by LIB-7's place words** (`std::string`),
  not by `MediaPlaceRef {ownerPlace, id}`: LIB-7's `placesFor` and
  `refusesNow` give and take place words, and `media.open-file` takes one.
- **`importFile` and `useImported` are one call, `importAndUse(file,
  place?, confirmed)`**: LIB-7's `media.open-file` with `upload` imports and
  then puts the medium into the place in one command (and only imports
  without a place, D-8), so the seam has its form.
- **`OpenSetup` is declared in `OpenPart.hpp`** as designed; `Classify.hpp`
  declares it forward. `OpenReply` also carries the file's name (`file`)
  besides `as` and `detail`, for the harness's line and AC-3.
- **`HarnessProduct::presetExtensions()`** (new; default none) besides
  `fileKinds()` and `openLimits()`: no product interface gives the preset
  extensions to the library yet. Without them a product's preset is told by
  its model line (test 5) in the harness; the products set theirs in M-48
  and M-56. The harness test product gives `.test-seg`, `.test-dot`.
- **The notice of a refusal is the open part's alone.** LIB-6's
  `load-preset` sends a notice for a read error itself; the opener calls the
  new `PresetsPart::loadPreset` (the same, without that notice), so a
  refused preset gives one notice, not two.
- **`OperationResult` has a constructor** `(ok = true, message = {}, result
  = {})` instead of being an aggregate: a third member made every
  `OperationResult{ok, message}` warn (`-Wmissing-field-initializers`).
- **A kept file whose opening asks** (a Web upload whose name is taken) gets
  a notice "`<name>` was not opened: `<question>`", since its command was
  replied to with `Kept` and nobody can answer; the design does not say.
- **L12 and L13 are in `real_parts_tests.cpp`**, not `open_tests.cpp`: they
  run on LIB-7's `MediaFixture`, the stand-in cases on LIB-3's `Rig`.
- **Several files without a preset among them**: the first file as handed
  over is opened alone, by the tests of AC-1 (D-7); files that are hidden
  or could not be read are not candidates for the preset.

### Not changed here

- No frontend sends `open` yet (Calc-U-1600 M-48, Calc-U-59 M-56; design §4,
  §5): the drops and Load Preset… of the products and of the library's shell
  are as before. No user-visible change, so no row in
  `docs/behaviour-changes.md`.
- Agent knowledge: `docs/test-groups.md` (the new cases), LIB-1 design §2a;
  no rename.


## Calc-U-1600 (M-48, 2026-11-16, branch `m48-opening-1600`)

Built and run on an M4 Pro, macOS 27.2, Apple clang, CMake, Ninja,
RelWithDebInfo, in `build/tests` (`-DCALCU_FRONTEND_QT=OFF`) and `build/qt`
(`-DCALCU_FRONTEND_QT=ON`, Homebrew Qt 6); the Web `dist/` with Homebrew
Emscripten in `build/web`.

The code (all under `products/calc-u-1600/` unless named):
- `app/SharpFileKinds.*` (new): the preset extensions, the limits (1 MiB for
  drop, Open With and the start argument, D-15), `programKindOf` (AC-17's
  rows: today's `dropfile::classify` without its preset test and without
  `.rom`), the kinds `BasicProgramKind` and `MachineCodeKind`, the stream
  topic `c1600.program-opened` and its payload (D4.2, D-11);
- `app/StartFile.*` (new): the file named at the start (D-14);
- `app/SharpAppPart.*`: owns the open part (over its `PresetsPart` and
  `MediaPart`, the two kinds, BASIC first) after the loaders among the
  parts, and the topic; `app/SharpWebPart.cpp`: `c1600.drop-target` gone, the
  open part's upload folder is `/uploads`; `app/HarnessProduct.cpp`:
  `presetExtensions()`, `fileKinds()`;
- `core/DropFile.*` deleted (the adapter of design §5 and D5);
  `core/Loaders.*`, `app/SharpLoaderPart.cpp`, `app/SharpTopics.hpp`: the
  file-kind reply names each WAV file's kind (`tapeKinds`);
- Qt6 `frontends/qt6/app/MainWindow.*`, `MachineController.*`, `main.cpp`:
  the drag asks `open-would-take`, the drop, Open With, the start argument
  and the Load Preset… dialog send `open` with their way; the program flows
  take bytes and run on the topic; `kMaxDropBytes`, `dropTargetOf`,
  `droppableFile`, `openDroppedFile`, `drainPendingDrop`, `m_pendingDrop`,
  `m_startupDone` gone; only the dialogs remember a folder (EC-7);
  `resources/macos/Info.plist.in` without `rom`;
- Web `frontends/web/src/CalcU1600.jsx`, `src/engine.js`: the drop and
  Upload Preset… send `open` (`upload: true`; `asks:` asked and sent again
  with `confirmed`), the program flow runs on the topic; `MAX_DROP_BYTES`,
  `pendingDrop`, `openDropped`, `loadPresetFiles`, `dropTarget` gone;
- `tools/check_open_with_types.sh` (new) and the CTest
  `CalcU1600OpenWithTypes` (Frequent);
- library: `MediaPart::readOpened` takes the path of the opened file and
  reads its companions beside it (see Deviations); `LibraryOpeners.cpp`
  passes it;
- tests: `app/tests/opening_tests.cpp` (new, 20 cases), the three
  `lib13_ac18_harness_open_…` cases of `tests/harness_tests.cpp`,
  `tests/program_file_tests.cpp` without `test_drop_targets` (moved, AC-22),
  five cases in `frontends/web/tests/play.spec.js`.

Result of the run: `build/tests` Frequent 35 of 35 CTest entries passed
(CoreTests 56.5 s with `-j4`, see Findings), `build/qt` Frequent 37 of 37
(offscreen); Longer: `CalcU1600WebPlaywright` 96 passed, 2 skipped (as
before; 10 of them the new LIB-13 cases on the root and the sub-path),
`CalcU1600WasmSmoke` passed. CI was not dispatched (plan §1).

| ID | Result | Test case (`app/tests/opening_tests.cpp` unless named) |
|---|---|---|
| AC-17 | Pass | `lib13_ac17_preset_extensions_media_suffixes`, `_kinds_asked_basic_then_code`, `_listing_and_tokenized_are_basic`, `_headered_code_is_code`, `_headerless_code_only_with_bin`, `_rom_is_not_code`, `_wav_goes_to_first_files_kind`, `_unreadable_wav_goes_to_basic_loader_which_says_why`, `_empty_and_other_taken_by_neither` |
| AC-18 | Pass | `lib13_ac18_gui_way_sends_program_opened_topic`, `lib13_ac18_kept_file_also_sends_topic`; `tests/harness_tests.cpp`: `lib13_ac18_harness_open_loads_first_wav_file`, `_harness_open_refuses_headerless` (also `.rom` refused), `_harness_open_preset_bas_and_headered_bin`; Web: `LIB-13 AC-18 a dropped WAV with several programs asks which one, then loads by its kind`, `LIB-13 AC-18 a dropped headerless .bin asks the address`; Qt6 (M-49, `CalcU1600QtTests`): `lib13_ac21_open_with_bin_runs_the_machine_code_flow` (a `.bin` opened as Open With runs the machine-code flow to its end), `c1600_2_ac16_qt_wav_with_several_programs_asks_which_one`, `c1600_2_ac18_qt_headerless_code_asks_the_address` (the flows the topic starts) |
| AC-19 | Pass | `lib13_ac19_disk_used_where_it_lies_written_back`, `lib13_ac19_upload_disk_imported_then_inserted`, `lib13_ac19_module_one_place_plugged_pc1500_two_places_refused_pc1600`, `lib13_ac19_tape_definition_into_cassette_wav_is_never_a_tape`; Web: `LIB-13 AC-19 a dropped disk file is imported and inserted`; Qt6 (M-49): `lib13_ac19_drop_onto_window_opens_preset` (a drag of a preset is taken, its drop loads it), `lib13_ac19_drop_disk_file_used_where_it_lies` (a `.floppy.yaml` dropped on a PC-1600 with the CE-1600P is in the drive; a file of no kind is not taken by the drag) |
| AC-20 | Pass (session, Web) | `lib13_ac20_over_1mib_refused_drop_openwith_start_not_dialog_harness`; Web: `LIB-13 AC-20 a dropped file over 1 MiB is refused with a notice` |
| AC-21 | Pass | `lib13_ac21_start_file_is_last_file_argument` (`calcu_library_tests`, `library/opening/tests/start_file_tests.cpp` since Part D simplification S-5); CTest `CalcU1600OpenWithTypes` (the three preset extensions Owner, `bas`, `bbin`, `bin` Alternate, no `rom`; fails on the old list); Web: `LIB-13 AC-8 Upload Preset opens the preset among its files`; Qt6 (M-49): `lib13_ac21_open_with_and_dock_drop_open_preset` (a `QFileOpenEvent`, as macOS sends for the Dock icon and Finder's Open With, through the app's `FileOpenFilter` to `open` with the way Open With), `lib13_ac21_open_with_bin_runs_the_machine_code_flow`, `lib13_ac21_start_with_a_file_name` (main's rule on the arguments, then `open` with the way Start before the start is over) |
| AC-22 | Pass | `lib13_ac22_drop_targets` (every case of the old `test_drop_targets` and the WAV cases, through the product's open part; `CODE.ROM` now refused, AC-17; "ignored" is now "refused") |
| EC-2 (product) | Pass | `lib13_ec2_disk_without_drive_refused_with_notice` |
| EC-7 (product) | Pass | `lib13_ec7_open_by_any_way_keeps_dialog_folders` (the remembered folders of the settings unchanged after `open` by drop, Open With, start and dialog); in Qt6 only the dialogs call `rememberOpenFile` |

Coverage: AC-17 to AC-22 have their tests and pass where they run without a
screen. The Qt6 hand checks of T16-5 went into M-49's tests on the real
window (user, 2026-11-16), see the rows above; the Open With event moved
from a `QApplication` subclass to an event filter on the application
(`frontends/qt6/app/FileOpenFilter.hpp`) so the tests send it the same
event. What stays untested: macOS itself (LaunchServices handing a Finder
drag, a Dock drop or Open With to the app, and the drag of a real Finder
window: the tests send the Qt events these become; the document types are
checked by `CalcU1600OpenWithTypes`), which needs a screen and a user
session. Column status: Done (M-49, 2026-11-16).

### Deviations from the design, chosen by the simpler structure

- **The kinds load through `sharp::loaders` directly** (the same calls as
  `c1600.load-basic` and `c1600.load-code`), not through a
  `SharpLoaderPart&`; they share one base, `ProgramFileKind`, and take the
  topic from `SharpAppPart::attach`. Without a topic (the harness) or with
  the way `harness` they load at once.
- **`c1600.upload` stays, for File ▸ Import Media… only** (design D5 had it
  go): the import puts a tape definition and its WAV side by side in
  `/uploads` before `media.import`, which `open` does not do. The drop and
  Upload Preset… no longer use it.
- **The file-kind reply names each WAV file's kind** (`tapeKinds`,
  C1600-2's reply): a WAV's chosen file goes to the flow of its own kind
  (AC-18), which the frontends could not tell otherwise.
- **A library change: an opened medium is read with its companions beside
  it** (`MediaPart::readOpened(name, bytes, path)`). Without it the check
  of AC-4 ("everything is checked first") refused every opened
  `.tape.yaml` ("its sound file … is missing") although `media.open-file`
  reads the WAV beside it. An upload has no path and is read as before.
- **The Qt6 drag sends the size** with `open-would-take`, and Qt reads at
  most 1 MiB + 1 byte of a file opened by a limited way, so a large file is
  refused by size without being read whole.
- **Two more visible changes got a ledger row** (1600-44): Load Preset…'s
  dialog and Upload Preset… open the chosen file as what it is (AC-5), and a
  WAV's chosen file goes to its own kind's flow (AC-18). Row 1600-15 is in
  effect from M-48.

### Findings

- **A `.tape.yaml` dropped alone in the Web frontend is refused** ("its
  sound file … is missing"): an upload carries no WAV, and a drag of two
  files is not taken (AC-6). Import Media… with both files is the way, as
  before M-48 (a tape could not be dropped at all then).
- **CoreTests is at 56.5–57.6 s in a `-j4` Frequent run** (54 s in M-47;
  the new cases take 1.6 s alone), close to the 60 s budget.

### Not changed here

- Golden runs and example references: not touched (design D8).
- Agent knowledge: `products/calc-u-1600/CLAUDE.md` (Harness: `open`; the
  open part among the parts), `docs/developer/Shared-Architecture.md`
  (`SharpFileKinds`, `StartFile`), `docs/developer/Web-Client.md` (Drag and
  drop), `docs/background/Decisions.md` (the two entries that named
  `core/DropFile` and `dropfile::classify`), `TODO.md`, `docs/User-Guide.md`
  (Dropping files), `docs/test-groups.md`, `docs/glossary-mapping.md` (row
  "Open (a file), file kind"). `Drag-And-Drop-Plan.md` stays history. No
  memory file named the old classes (checked); none changed.

## Library, Part D simplification P-3 (2026-11-16, branch `ds-pass-through`)

- `ac4_pass_through_follows_product` moved from `open_tests.cpp`, where it
  passed only because the stand-in skipped the reader, to
  `real_parts_tests.cpp` on LIB-7's real part: a card opened on the
  test-seg is fitted, not fed (AC-4, AC-12; LIB-7 design D-22), fed by
  side 0 and side 1 and written back where it lies each time; while it
  passes another card is refused (EC-4). The stand-in `TestMedia` no
  longer treats `reader` apart. Row AC-4 above says so; AC-12 itself is
  Calc-U-59's column (M-55).
