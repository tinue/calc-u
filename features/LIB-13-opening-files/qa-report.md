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
| AC-4 | Pass | `ac4_medium_checked_first_hint_passed_on`, `ac4_used_where_it_lies`, `ac4_template_not_written`, `ac4_holding_place_inserted_after_eject`, `ac4_pass_through_follows_product`, `ac4_module_one_slot_plugged`, `ac4_module_two_slots_refused`, `ac4_no_place_now_refused_says_where`, `ac4_upload_imported_then_placed`, `ac4_upload_name_taken_asks_then_replace` (`open_tests.cpp`, stand-ins); `open_medium_through_lib7_part` (LIB-7's part: inserted in `disk` from where it lies and written back there, not copied; an upload imported then inserted, the name taken asks; a card on the test-dot refused saying where it goes; an outdated disk with LIB-7's hint) (`real_parts_tests.cpp`) |
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
