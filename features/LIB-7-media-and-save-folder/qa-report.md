# LIB-7: QA report

Results per acceptance criterion and edge case of `spec.md`, per column.

## Library (M-41, 2026-11-15, branch `m41-media`)

Built and run on an M4 Pro, macOS 27.2, Apple clang, CMake, Ninja,
RelWithDebInfo, in two trees: `build/tests` (`-DCALCU_FRONTEND_QT=OFF`)
and `build/qt` (`-DCALCU_FRONTEND_QT=ON`, Homebrew Qt 6).

The code:
- `library/media/` (`calcu::media`, in `calcu_library`): `Kind.hpp` (the
  seam of AC-46: `Content`, `MediumInfo`, `ReadOutcome`, `MediumCodec`,
  `KindDecl`, `PresetWords`, `ModuleListing`), `Names.*`, `MediaPlace.hpp`
  (`MediaPlace`, `ModuleMedium`), `Shelf.*`, `SaveRules.*`, `Writer.*`,
  `MediaTopics.*` (the topic `media`, the commands), `MediaPart.*` (the part,
  autosave A1 to A9, the wait probe), `PresetMedia.*` (the slot keys'
  resolver, the insert and arm steps of each kind, `eject:`, `saveas:`,
  the in-file media of a read), `Conformance.*` (LIB-1 D-1);
- `library/sessions/`: `SessionPart::beforeOperation`, `beforeMachineChange`,
  `closing`; `MachineSession::aboutToChange`; `AppSession::close()` (§3.12;
  commits of their own);
- `library/startup/MachineControl.*`: `change` calls `aboutToChange`,
  `addCarry` (S-7, S-7a; a commit of its own);
- `library/presets/`: `Vocabulary::setPlaceResolver`, `setBundledMedium`,
  `ReadContext::vocabulary()`, `kept()`, `PlaceKeyRule::byFile` on by
  default, `checkBundle` accepts a bundled medium, a step on its own reads
  the file its reference names (commits of their own);
- `library/harness/MediaCommands.cpp`, `Commands.hpp`, `Product.hpp`
  (`mediaKinds()`, `moduleListing()`), `Run.hpp`, `Run.cpp`;
- the Qt6 component set: `library/qt6/MediaPicker.*`, `NameAndSaveDialog.*`;
  the React component set: `library/web/components/MediaPicker.jsx`,
  `NameAndSaveDialog.jsx`, `SavedMediaDialog.jsx`, `media.css` (in
  `COMPONENT_SOURCES`, `COMPONENT_CSS`);
- tests: `library/media/tests/` (`MediaFixture.hpp`, `TestKinds.*`, nine
  `*_tests.cpp`), `library/harness/tests/media_commands_tests.cpp`,
  `library/qt6/tests/media_components_tests.cpp`; the test machines offer
  their media places with `TestBuilder::media` and `mediaTestCatalogue()`
  adds `test-memory-module` (the module medium, switch `write-protect`) and
  `test-rom-module` (`library/machine/tests/TestMachines.*`).

Result of the run: `calcu_library_tests` 729 tests (100 of them M-41's: 95
in `library/media/tests/`, 5 in `media_commands_tests.cpp`), 5,075 checks,
0 failed; `calcu_qt6_tests` 13 tests (3 of them M-41's), 0 failed. Frequent:
30 of 30 CTest entries in `build/tests`, 31 of 31 in `build/qt`;
`LibraryWebNodeTests` 4 of 4, and the component set compiled by
`dist.js` (`compileCalcJs` with the products' vendored Babel) with the scope
check clean. CI was not dispatched (plan §1).

| ID | Result | Test case (file in `library/media/tests/` unless named) |
|---|---|---|
| AC-1 | Pass | `ac1_bundled_is_template_and_never_written`, `ac1_user_template_by_its_file`, `ac1_instance_in_save_folder` (`shelf_tests.cpp`); `ac1_opened_file_written_where_it_lies` (an opened file used and written where it lies; an upload imported first, AC-30; the reads LIB-13 uses) (`saved_media_tests.cpp`) |
| AC-2 | Pass | `ac2_inner_name_wins`, `ac2_file_name_counts_only_without_inner_name` (`names_tests.cpp`) |
| AC-3 | Pass | `ac3_spaces_trimmed_and_runs_collapsed`, `ac3_refused_characters_and_control_characters`, `ac3_empty_refused`, `ac3_names_differing_in_case_are_one` (`names_tests.cpp`) |
| AC-4 | Pass | `ac4_file_is_name_and_suffix_in_save_folder`, `ac4_numbered_when_file_holds_another_name` (`names_tests.cpp`) |
| AC-5 | Pass | `ac5_bundled_first_then_save_folder`, `ac5_two_files_one_name_both_listed_and_found_by_rule` (OQ-1, also a save by the name), `ac5_saved_file_with_bundled_name_not_listed_and_logged` (D-8), `ac5_module_slot_one_list_of_names` (`shelf_tests.cpp`) |
| AC-6 | Pass | `ac6_place_states_kind_passage_sides`, `ac6_insert_eject_side_content_revision`, `ac6_waits_for_medium`, `ac6_write_protect_where_the_medium_has_one` (`place_tests.cpp`) |
| AC-7 | Pass | `ac7_kinds_templates_instances_with_place`, `ac7_place_holds_and_waits`, `ac7_reread_after_own_write_and_on_refresh`, `ac7_outside_change_not_seen_until_refresh` (`topic_tests.cpp`) |
| AC-8 | Pass | `ac8_order_empty_templates_instances_modules`, `ac8_tooltip_is_description`, `ac8_listing_label_and_sort_key` (`picker_tests.cpp`); drawn: `ac8_picker_draws_entries` (`library/qt6/tests/media_components_tests.cpp`); `ac8 picker lists the saved instance` (`library/web/tests/media.spec.js`, M-42) |
| AC-9 | Pass | `ac9_outdated_entry_disabled_with_hint`, `ac9_outdated_logged_once_per_start` (`picker_tests.cpp`); drawn: `ac9_outdated_entry_disabled_with_hint_tooltip` (Qt6); `ac9 outdated entry disabled with hint` (`media.spec.js`, M-42) |
| AC-10 | Pass | `ac10_old_medium_written_first`, `ac10_holding_place_new_medium_in_at_once`, `ac10_module_slot_is_a_replace_by_lib18` (one replace, the slot's power cycle), `ac10_unreadable_leaves_place_empty_with_notice` (`picker_tests.cpp`) |
| AC-11 | Pass | `ac11_written_within_half_a_second_at_60_hz` (the change at five points of the 60 Hz rhythm: written after 0.4 to 0.5 s), `ac11_not_written_while_nothing_changes`, `ac11_harness_written_at_end_of_command` (`autosave_tests.cpp`) |
| AC-12 | Pass | `ac12_pass_through_written_when_given_back_if_written`, `ac12_fed_again_inserts_what_the_last_pass_wrote` (the fitted card fed again is what the last pass wrote; a template's writes stay with it) (`autosave_tests.cpp`) |
| AC-13 | Pass | `ac13_written_before_leaving_place`, `ac13_written_before_operation` (written when Reset has started, before its end), `ac13_written_before_machine_change` (`takeMachine`, and an attach through LIB-18), `ac13_written_on_suspend`, `ac13_written_on_close` (`AppSession::close()`) (`autosave_tests.cpp`) |
| AC-14 | Pass | `ac14_failed_write_one_notice_until_success`, `ac14_next_change_tries_again` (`settings::testing::failBeforeRename`) (`autosave_tests.cpp`) |
| AC-15 | Pass | `ac15_picker_template_not_written_lives_in_place`, `ac15_preset_named_template_not_written` (by file and by name, a bundled one too), `ac15_in_file_medium_not_written_unless_saveas` (`as: live`: that instance from then on) (`autosave_tests.cpp`) |
| AC-16 | Pass | `ac16_offered_for_template_that_can_change`, `ac16_asks_write_protect_only_when_switch_off` (`save_tests.cpp`); drawn: `ac16_name_and_save_dialog_asks_name_and_switch` (Qt6; the `asks:` question and the confirmed second send, D-6); `ac16 name and save asks the name` (`media.spec.js`, M-42) |
| AC-17 | Pass | `ac17_rules_table` (every row × column on `planSave`), `ac17_file_form_writes_that_file`, `ac17_file_form_refused_for_bundled_and_user_template`, `ac17_name_and_save_asks_before_replacing` (`save_tests.cpp`) |
| AC-18 | Pass | `ac18_place_holds_saved_medium_no_power_cycle`, `ac18_live_autosaved_template_not` (`save_tests.cpp`) |
| AC-19 | Pass | `ac19_protect_in_file_at_once_and_power_cycle`, `ac19_no_command_switches_it_off` (`save_tests.cpp`) |
| AC-20 | Pass | `ac20_started_name_asked_first_file_at_first_write` (a holding place; on the pass-through reader the started card is fitted, fed by side 0 then side 1, and its file written each time it has passed) (`save_tests.cpp`) |
| AC-21 | Pass | `ac21_reference_mapping_name_file_mem_side`, `ac21_bare_value_refused`, `ac21_old_file_keys_refused` (`preset_media_tests.cpp`) |
| AC-22 | Pass | `ac22_in_file_media_from_header_key_saved_only_by_saveas` (the test header key `test-cards:` through `defineInFile`; gone with the next load, D-10) (`preset_media_tests.cpp`) |
| AC-23 | Pass | `ac23_missing_outdated_unreadable_refused_at_read`, `ac23_undefined_mem_is_fresh` (`preset_media_tests.cpp`) |
| AC-24 | Pass | `ac24_slot_instance_fitted_before_start_and_autosaved`, `ac24_step_inserts_instance` (`preset_media_tests.cpp`) |
| AC-25 | Pass | `ac25_template_file_never_written_wherever`, `ac25_instance_by_file_written_in_place_rerun_starts_from_it`, `ac25_log_names_kind_and_name_tag` (`preset_media_tests.cpp`) |
| AC-26 | Pass | `ac26_steps_table` (the arm step fits; a card step fits, feeds and leaves the card fitted; not while one passes), `ac26_pass_through_step_finishes_when_passed_within_timeout` (`preset_media_tests.cpp`); `ac26_fitted_card_fed_by_each_side_stays_fitted`, `ac26_nothing_fitted_or_fed_while_a_card_passes`, `ac26_eject_takes_passing_card_out_still_fitted`, `ac26_choose_fits_does_not_feed` (`media.arm`, `media.feed`, `media.choose`, `media.eject` on the reader; design D-22) (`place_tests.cpp`); as commands: `ac26_steps_as_commands`, `ac26_mapping_line_as_step` (`library/harness/tests/media_commands_tests.cpp`) |
| AC-27 | Pass | `ac27_armed_never_fed_logged_at_next_build` (a card that passed and stays fitted gives no line) (`preset_media_tests.cpp`) |
| AC-28 | Pass | `ac28_media_reads_topic`, `ac28_import_export_delete`, `ac28_run_has_empty_save_folder_and_bundled_media` (LIB-11 AC-8) (`media_commands_tests.cpp`) |
| AC-29 | Pass (M-42) | The items: LIB-14's menu model, `menu_model_tests.cpp` (Web with kinds: Import Media…, Saved Media…; Qt6 none, OQ-4; M-43) and `shell.spec.js` `ac29 file menu has import and saved media` (scripted topic; M-43). On the Web test product `Test-M` with LIB-7's MediaPart (`library/web/tests/media.spec.js`, Chromium and WebKit): `ac29 file menu has import and saved media` (the items there, Import Media… through the file picker into the save folder, the medium in Saved Media…), `ac29 media in persistent file system after reload` (LIB-15 AC-5). Qt6: `lib7_ac29_qt6_file_menu_without_media_items` (`library/qt6/tests/shell_widgets_tests.cpp`: no media items, also with kinds of medium) |
| AC-30 | Pass | `ac30_import_current_medium_under_inner_name`, `ac30_import_refused_name_taken_outdated_unreadable`, `ac30_import_with_companion_file`, `ac30_import_through_the_file_picker` (`saved_media_tests.cpp`) |
| AC-31 | Pass | `ac31_list_by_kind_and_name_marks_template_and_in_place`, `ac31_export_hands_files_to_keep_file`, `ac31_delete_asks_then_deletes_not_in_place` (`saved_media_tests.cpp`); drawn: `ac31 saved media lists downloads deletes after asking` (`media.spec.js`: `CalcuSavedMediaDialog` from File ▸ Saved Media…, a template marked, Download a download of the file, Delete after the question; M-42) |
| AC-46 | Pass | `ac46_codec_gives_all_the_library_knows`, `ac46_content_handed_through_unread` (`shelf_tests.cpp`) |
| EC-1 | Pass | `ec1_save_folder_made_at_first_write` (`autosave_tests.cpp`); an unwritable folder: AC-14 |
| EC-2 | Pass | `ec2_outside_change_overwritten_not_merged` (`autosave_tests.cpp`) |
| EC-3 | Pass | `ec3_unreadable_not_listed_one_log_line_per_start` (`shelf_tests.cpp`) |
| EC-4 | Pass | `ec4_second_delete_no_longer_there` (`saved_media_tests.cpp`) |
| EC-5 | Pass | `ec5_insert_ejects_held_first` (`preset_media_tests.cpp`) |
| EC-6 | Pass | `ec6_eject_empty_place_nothing` (`preset_media_tests.cpp`) |
| EC-7 | Pass | `ec7_saveas_protected_without_switch_fails` (`save_tests.cpp`) |
| EC-8 | Pass | `ec8_suspend_writes_every_pending_change` (two places) (`autosave_tests.cpp`) |
| EC-9 | Pass | `ec9_name_and_save_not_for_instance` (`save_tests.cpp`) |

Coverage: AC-1 to AC-28, AC-30, AC-31, AC-46 and EC-1 to EC-9 pass in the
library; AC-29 waits for LIB-14's File menu (M-43). The Library column stays
**In progress** (`features/INDEX.md`): it becomes Done in M-43 (design §1,
D-7). *Superseded by the M-42 section below: every AC of the column passes,
the column is Done.*

### Clauses of other features that pass here

- LIB-18 AC-2 ("what was saved as a medium stays saved") and AC-7 ("the
  same media in them"): `lib18_ac2_what_was_saved_as_a_medium_stays_saved`,
  `lib18_ac7_variant_switch_keeps_the_media` (a disk and a module medium
  carried over a variant switch, unsaved content included, still the same
  instances) (`preset_media_tests.cpp`). LIB-18's Library column is Done
  (its qa-report).
- LIB-6 AC-3, a module by `file:`: `lib6_ac3_module_by_file_reference`;
  bundle.md §3 "or is a bundled medium or module of the product":
  `lib6_check_bundle_accepts_bundled_media` (`preset_media_tests.cpp`).
- LIB-1 D-1, AC-40: `ac40_media_place_capability_per_media_place`
  (`place_tests.cpp`, `media::checkMediaPlaces`).
- LIB-11 AC-8 (bundled media in a run, an empty save folder of its own, or
  the one given) and AC-22 (`test-disk { file: … }`, `saveas {…}` as mapping
  lines): `media_commands_tests.cpp`.

### Deviations from the design, chosen by the simpler structure

- **`closing` comes from `AppSession::close()`, not from the destructor**
  (§3.12). A part the session does not own may be gone before the session
  (LIB-3's contract; Calc-U-59's `web_part_tests.cpp` destroys its Web part
  first): calling every part from `~AppSession` crashed `CalcU59Tests`. The
  host calls `close()` before it destroys the session (the harness's
  `Server::finish` does); no message goes out after it; the destructor calls
  no part and drops the `aboutToChange` callback before the owned parts go.
  The products call `close()` in their columns (M-47, M-55).
- **LIB-1 D-1's check is `media::checkMediaPlaces`** in
  `library/media/Conformance.*`, not a change of
  `library/machine/Conformance.cpp`, and its test is in `place_tests.cpp`:
  the machine area includes the standard library only and names no kind.
  LIB-1 design §3.10 says so now.
- **Seam S-1 by its fallback**: the part computes what fits a slot from the
  catalogue and marks every picker entry "not now" with LIB-3's sentence
  while an operation runs; it does not read LIB-4's control state (so
  `media` does not include `control`).
- **Seams S-4, S-5, S-6**: the slot keys' resolver is
  `Vocabulary::setPlaceResolver` (a `PlaceResolution`), the reference
  reader is the media area's own (`readReference` in `PresetMedia.cpp`), and
  the in-file media of a read are kept with the read
  (`ReadContext::keep`/`kept`, `InFileSet`), put into the part's store at
  their first use in a load; the store is cleared at every preset build and
  model switch (D-10). The binding of a slot key's medium is found after the
  build by its request's content (`MediaPart::registerModule`).
- **`PresetWords` has no `marks`**: LIB-6 AC-62's markers come from
  `ModelRules::applies(verb)`, as for every other step.
- **`addMediaCommands(CommandRegistry&, MediaPart*)`** takes the run's part
  (export writes the files beside `<path>` itself); `HarnessProduct` gets
  `mediaKinds()` and `moduleListing()`; the run's `Paths` for media use the
  run's save folder as passed (`FrontendKind::Swift`), since Qt6's rule would
  put it under the run's home.
- **A step on its own reads files** (`readStep`'s source): `{ file: }` in a
  harness command, absolute or relative to the current folder; it was refused
  before.
- **`PlaceKeyRule::byFile` is on by default**; without a resolver `file:` is
  refused as before (`ac3_module_by_file_refused_without_a_resolver`, renamed
  from `ac3_module_by_file_refused_until_lib7`).
- **`media.start-new` on a pass-through place arms** the started medium (the
  user feeds it); on a holding place it is inserted.
- **Web drawn cases**: the three React components are written and compiled;
  `library/web/tests/media.spec.js` (ac31, ac8 drawn, ac29) is not: it needs
  the Web test product to host a media part with the test kinds, which
  comes with the shell in M-43, where the Library column becomes Done (D-7).
- Test support: `TestBuilder::media`, `TestMediaPlace` (a card passes in
  200 ticks; `writeOnPass`, `holdsBack`, `refuse`), `TestModuleMedium`,
  `mediaTestCatalogue()` with `test-memory-module` and `test-rom-module`
  (a second module that is not a medium, for the listing order); the test
  kinds are compiled into `calcu_test_machines` (the harness's test product
  uses them with `TestProduct::media`).

### For the user (proposals)

- Plan §5.1: LIB-7 Library "Done in" M-43 (D-7); LIB-6 Library Done needs
  M-40's debugger steps as well (LIB-6 D-29).
- Open points kept: D-3 (only ASCII letters are folded), D-21 (no length
  limit, no check of names a host reserves).

## Library, completed (M-42, 2026-11-15, branch `m42-open-file`)

M-41 left the column In progress for AC-29 (the File menu items, LIB-14's,
M-43) and the browser cases of `library/web/tests/media.spec.js` (design
§2, L11), which needed a Web test product that hosts a media part. Both are
done here, after M-43:

- **The Web test product `Test-M`** (`library/web/tests/product/`, a fourth
  dist of `build.sh`): its session hosts LIB-7's `MediaPart` with the test
  kinds of `TestKinds.hpp` (no bundled folders) on the page's save folder,
  its machines with their media places (`TestBuilder::media`); its
  `TestShellPart` is made with `withMedia = false` (new third argument: no
  scripted `media` topic, no recording `media.import`), so the shell's menu
  and dialogs read the real topic. `CalcUTest.jsx` draws, for `Test-M`, a
  `CalcuMediaPicker` per place of the topic (with a Name & Save button and
  `CalcuNameAndSaveDialog` where the place offers it) as the control bar's
  media control, and `CalcuSavedMediaDialog` for File ▸ Saved Media….
- **`media.spec.js`** (projects `media-chromium`, `media-webkit`): `ac29 file
  menu has import and saved media`, `ac29 media in persistent file system
  after reload`, `ac8 picker lists the saved instance`, `ac9 outdated entry
  disabled with hint`, `ac16 name and save asks the name`, `ac31 saved media
  lists downloads deletes after asking`: 12 of 12 pass.
- **AC-29 in the Qt6 shell**: the spec's clause is the Web frontend's; the
  Qt6 shell shows neither item (OQ-4: no saved media list in Qt6), also when
  the product has kinds: `lib7_ac29_qt6_file_menu_without_media_items`
  (`calcu_qt6_tests`), passes.

Results: `LibraryWebPlaywright` 138 cases, 136 passed, 2 skipped (the
Chromium-root-only case of `storage.spec.js`, as before); `calcu_qt6_tests`
47 of 47. Every AC and EC of the Library column passes: the Library column
is **Done** (`features/INDEX.md`).

Deviations:
- The case names of design L11 are kept; `ac9` and `ac16` drawn cases were
  added besides the four named (the design's "drawn AC-8, AC-9, AC-16").
- `ShellDialogs`' `savedMedia` component gets `{shell, onClose}` while
  `CalcuSavedMediaDialog` takes `{media, send, ask, onClose}`: the test
  product wraps it (`CalcUTestSavedMedia`); a product does the same in its
  column (M-47, M-55).
- The save folder is checked with the request `tp.read`: the topic
  `tp.files` lists the folder only after the test product's own commands.

## Calc-U-1600 (M-47, 2026-11-16, branch `m47-media-1600`)

Built and run on an M4 Pro, macOS 27.2, Apple clang, CMake, Ninja,
RelWithDebInfo: `build/tests` (`-DCALCU_FRONTEND_QT=OFF`), `build/qt`
(`-DCALCU_FRONTEND_QT=ON`, Homebrew Qt 6, `-DCALCU_TESTS_LONGER=ON`), the
Web `dist/` (`frontends/web/build.sh`, Emscripten).

The code:
- `app/SharpMediaKinds.*`: the three kinds and their codecs (AC-37, AC-41,
  AC-42, AC-46): `battery-backed-module` (`.card.yaml` version 1,
  `HeldBy::ModuleSlot`; `isMedium` = a definition with `battery: true`;
  `moduleRequest` adds a saved instance's descriptor to the session's
  catalogue; `fitsPinOut` by `compatible-hosts`; the write is the splice of
  `BatteryCardInstance.hpp` into the file it came from, else into the
  bundled file of the module), `floppy-disk` (`.floppy.yaml` version 2,
  sides `0:`/`1:`, version 1 `Outdated` with the hint; written whole, the
  description kept), `cassette` (`.tape.yaml` version 1 with its WAV,
  `Folder::Tapes`; the WAV beside a copy, a definition written again keeps
  its sound file's path, D-26); `moduleListing()`.
- `core/Machine/SharpMediaPlaces.*`: the content types `ModuleContent`,
  `FloppyContent`, `TapeContent`; `FloppyPlace` (the CE-1600F's media
  place `floppy`), `CassettePlace` (the media place `cassette` of a
  CE-150 or CE-1600P: a tape without samples records, one with samples
  plays; `takeWriteNow` at each motor stop, `writesWhenAsked`), and
  `SlotModuleMedium` (a battery-backed module in a slot), offered by the
  interface machines and `SharpMachine`; `TapeDeck::loadForPlayBytes`,
  `takeStopped`; `wavMono16Bytes`, `readWavBytes`; `FloppyImageFile.hpp`
  version 2. `c1600.tape-deck` keeps the counter, `c1600.floppy-drive` the
  lamp; both are the interface's (no tape kind without a CE-150 or
  CE-1600P). The slots left the descriptors' `mediaPlaces` (D-22).
- `app/SharpAppPart`: `MediaPart` among the parts after `StartupPart`,
  `addMediaToPresets`; `c1600.select-module`, the topic `c1600.tapes` and
  `MachinePayload::moduleChanges` went (the counter and the floppy lamp are
  in `c1600.machine`); the CE-1600P's ROM switch puts the disk and the tape
  back. Deleted: `app/ModuleBay.*`, `FloppyBay.*`, `TapeBay.*`,
  `MediaLibrary.*`, `DirectoryScanCache.hpp`, `core/Connector/MediaSave.hpp`,
  the save helpers of `AppPaths`. `app/HarnessProduct`: `mediaKinds()`,
  `moduleListing()`. `app/SharpWebPart`: the bay and saved-media commands
  went.
- Qt6: the control bar's pickers from the topic `media` (groups, disabled
  outdated entries with their hint), `media.choose`, `media.turn`, the
  library's `NameAndSaveDialog`, the tape's Save as `media.start-new`;
  `MemoryModuleManager`, `FloppyDiskManager`, `TapeManager` deleted;
  "Save folder:"; the host calls `AppSession::close()`.
- Web: the pickers from the topic, the library's `CalcuNameAndSaveDialog`
  and `CalcuSavedMediaDialog`, File ▸ Import Media… (a tape with its WAV).
- `CliPresetRun` passes `--save-dir` as the run's save folder; the CLIs'
  `--tape-in`/`--tape-out` go through the cassette's media place.

Result: `CoreTests` 1004 tests (21 `lib7_` of M-47 plus the ported ones),
Frequent 34 of 34 in `build/tests` (`CoreTests` 54 s), 36 of 36 in
`build/qt`; `calcu_library_tests` 927 (two new of M-47); `calcu_qt6_tests`
49 (offscreen); Web Playwright 86 passed, 2 skipped; Longer in `build/qt`:
`CalcU1600Shots` 04 to 09 and the check scenarios (lib6, lib7, lib8, lib10)
passed, `CalcU1600ExampleReferences` 32 of 32. CI not dispatched.

| ID | Result | Test case |
|---|---|---|
| AC-37 | Pass | `lib7_ac37_three_kinds_suffixes_versions_name_keys`, `lib7_ac37_floppy_v2_sides_0_1_v1_outdated_with_hint`, `lib7_ac37_card_v1_read_and_written` (`tests/media_kinds_tests.cpp`); `test_floppy_file_*` of `ce1600f_tests.cpp` on version 2 |
| AC-38 | Pass | `lib7_ac38_picker_fits_the_slot_in_ac8_order`, `lib7_ac38_choose_is_replace_power_cycle_memory_kept` (one replace, its power cycle, NEW0? :CHECK), `lib7_ac38_other_slot_and_disk_untouched` (`app/tests/media_session_tests.cpp`); `lib18_ac29_module_replace_power_cycle_memory_kept_new0_check` |
| AC-39 | Pass | `lib7_ac39_turn_is_eject_and_insert_other_side`, `lib7_ac39_lamp_while_motor_runs`, `lib7_ac39_name_and_save_for_a_template_disk`; Web `lib7_ac44 the disk side button turns the disk over`; Qt6 shots `05-modules` (the side button reads "0") |
| AC-40 | Pass | `lib7_ac40_instance_name_replaced_after_asking`, `lib7_ac40_name_with_slash_or_colon_refused`, `lib7_ac40_write_protect_power_cycle` (row 1600-17); `lib7_ac40_qt6_prompts_instance_and_disk_name` (`calcu_qt6_tests`); Web `lib7_ac44 Name & Save can write-protect a CE-161, and only a card with the switch` |
| AC-41 | Pass | `lib7_ac41_card_file_kept_but_content_name_template_mark`, `lib7_ac41_disk_written_whole_description_kept`; `lib7_ac44_test_floppy_bay` (the description kept by Name & Save and autosave) |
| AC-42 | Pass | `lib7_ac42_definition_with_wav_is_a_medium` (any relative WAV, `position` reserved, a missing WAV named), `lib7_ac42_cassette_place_blank_records_tape_with_sound_plays` (written at the motor stop), `lib7_ac42_tape_flow_through_the_harness` (the empty tape offered; a hand-written definition around the fixture's WAV CLOADs; CSAVE onto the empty tape stays in the place, unsaved; `saveas` writes `Rec.tape.yaml` and `Rec.wav` side by side; a WAV alone not listed; `tape:` and `eject: tape`; the saved tape CLOADs), `lib7_ac44_test_tape_status_follows_calls` (start-new, the counter in `c1600.machine`, no file before the first write); Qt6 shots `04-basic` (a named new tape, CSAVE, CLOAD); Web `lib7_ac44 a new tape is recorded under its name` |
| AC-43 | Pass | Web `lib7_ac43 import card disk and tape with wav`, `lib7_ac43 saved media list`, `lib7_ac43 delete asks first`; `lib7_ac44_test_media_library` |
| AC-44 | Pass | Today's tests on the topic and commands: `lib7_ac44_test_module_bay`, `lib7_ac44_test_floppy_bay` (`media_session_tests.cpp`, was `shared_bay_tests.cpp`; the scan cache's test went with it), `lib7_ac44_test_media_library`, `lib7_ac44_test_tape_status_follows_calls`, `lib7_ac40_write_protect_power_cycle` (was `test_card_write_protect`) (`shared_app_session_tests.cpp`); `battery_card_instance_tests.cpp` (the splice and templates; `test_plan_media_save` went with `MediaSave.hpp`: the save rules are the library's `planSave`, AC-17 of the Library column, and pass for the modules in `lib7_ac40_*`); `tape_deck_tests.cpp` unchanged; `ce1600f_tests.cpp` on version 2; Web `play.spec.js` (`lib7_ac44 …`, `a second tab says its changes are not saved`), `session.spec.js` `ac25` |
| AC-45 | Pass | Qt6 `CalcU1600Shots.lib7-ac16-module-template-saved`, `CalcU1600Shots.lib7-ac8-picker-lists-instance` (`tests/gui/`; AC-16's "an instance afterwards" shown by the capture, the scenario language has no check of a disabled button); Web `lib7_ac16 module template saved under a name is an instance`, `lib7_ac8 picker lists it among the instances` |

Every AC of the Calc-U-1600 column passes: the column is **Done**
(`features/INDEX.md`).

### Clauses of other features that pass here

- C1600-4 AC-15, AC-16 (the six presets that waited; the four examples
  against their references): its qa-report, M-47 section. C1600-4 is Done.
- LIB-6 AC-47's `floppy:` clause (`lib6_ac47_floppy_without_ce1600p_refused`)
  and AC-51's six presets: its qa-report. LIB-6 1600 is Done.
- LIB-1 AC-37 (with D-22), AC-40's media place clause
  (`lib1_ac40_one_media_place_capability_per_media_place`): LIB-1 1600 Done.
- LIB-2 AC-32's save with the switch: LIB-2 1600 Done.
- LIB-18 AC-28 again with the media part: `lib18_ac28_variant_switch_replugs_printer_interface_host_drive_modules`
  (a saved instance in slot 2 carried over through `addCarry`),
  `lib18_ac28_disk_in_drive_again_same_side` (the disk, side 1, through the
  variant switch).

### Deviations from the design, chosen by the simpler structure

- **Library additions** (each with a test): `ReadOutcome::Status::NotMedium`
  (a `.card.yaml` without a battery is a module that is not a medium: the
  shelf neither lists nor logs it; `ec3_not_a_medium_neither_listed_nor_logged`);
  `MediaPlace::writesWhenAsked()` (a tape is written at its motor stops and
  at the writes that do not wait, not half a second after each sample;
  `ac11_writes_when_asked_wait_for_the_moment`); `KindDecl::fitsPinOut`
  (AC-8's "modules that fit the slot" for the shelf's media; computed
  without building a request each frame); a slot's picker lists a
  catalogue module of a bundled template's name once; a medium step needs
  a place of its kind (`placeFor`, LIB-6 AC-47).
- **The media place IDs.** Two media places of one peripheral (the
  CE-1600P's cassette and drive) get distinct capability IDs
  `printer/media-place/cassette`, `…/floppy` (`SharpMachine::byPeripheral`
  with a name).
- **A module keeps its card's name after Name & Save.** The machine's card
  keeps the module name of its definition; the medium's name is the
  binding's (the topic, the picker, the file). 0.8.1 re-read the card from
  the saved file, so the card bore the new name; nothing user-visible
  reads the card's name but the inspector's module dump label, which now
  uses the binding's name.
- **The core's catalogue scans stay.** `scanMemoryCardDirectory` (the
  catalogue's module definitions, the card codec's bundled source),
  `resolveModuleSpecByName`, `scanFloppyDirectory`, `resolveFloppyByName`,
  `namedFileName` stay in `core/Connector/` for the catalogue and their
  tests; no media code uses them (§10.2 deleted them). Part D (P-4,
  `ds-core`): all but `scanMemoryCardDirectory` deleted with their tests.
- **`legacy::moduleIn` and `atOffPoint` stay** for `SharpAppPart::slotCard`
  (the inspector dump) and the tests; `presetMedia` was gone already.
- **`AppPaths::instanceDir()` and `tapeDir()` stay** (the Settings dialog
  and LIB-9's tests); the media part reads `AppPaths::places()`.
- **The tape's record button** is `media.start-new` (D-24); the Web's and
  Qt6's question "replace?" comes from the `asks:` reply.
- **`c1600.machine`** carries the tape counter and the floppy lamp
  (`tape`, `floppyMotor`); the topic `c1600.tapes` went.

### Findings for the user

- **PC-1600 idle and the disk (LIB-6 1600).** `INIT"X:"` and `SAVE"X:…"`
  write for seconds while the ROM reads the keyboard; key readiness counted
  that as idle, so a preset's next item started early (`make_diskworks_media`:
  ERROR 164). The PC-1600's idle rule now also waits for the CE-1600F's
  motor (ledger row 1600-43). The broader M-46 finding (BASIC running
  counts as idle) stands: `DiskWorks`' display is excepted in AC-16.
- **Rows 1600-40 to 1600-43** are new (the cassette with a CE-150 on the
  PC-1600; a detached CE-1600P takes the disk out; the slot picker's
  groups; a preset waits for the disk motor).
- **CoreTests at 54 s** of the 60 s Frequent budget (51 s in M-46); the
  new media tests take about 3 s.

## Library, Part D simplification P-3 (2026-11-16, branch `ds-pass-through`)

A pass-through place's binding is the fitted medium (design D-22, which
lists the departures from the design before it; review
`docs/reviews/d-simplification.md` S-2). Rows AC-12, AC-20, AC-26 and
AC-27 above name the tests added and changed. Calc-U-1600 has no
pass-through place: nothing of it changed. No client read the topic's
`armed`, which is gone (`library/web/components`, `library/qt6`, the
products' frontends checked).

## Calc-U-59 (M-55, 2026-11-17, branch `m55-cards-59`)

Built and run as C59-7's qa-report says (Frequent, `build/qt`, the Web's
`run-tests.sh`; Swift type-checked, run from the main checkout). The card
itself (the place, the kind's codec, `magnetic-cards:`, the cue cards) is
C59-7's report; here the media model's side and the library additions
(design §11).

Library additions (§11.2; Calc-U-1600 keeps the defaults, its tests pass
unchanged): `KindDecl::rereadBeforeWrite`, `offersNameAndSave`,
`replaceQuestion` (`replaceQuestionFor`), `media::FileAccess`
(`library/media/FileAccess.*`; `MediaSetup::files`,
`LibraryPartsSetup::mediaFiles`; the shelf and the writer read, list and
write through it), `ModuleListing::slots` (a module slot that no kind of
medium fits gets a place with the empty slot and the modules), and
`Binding::armed` (a serial that changes with each fit, for the product's
cue card). One rule of the presets engine: `RunContext::notWaitedFor()`,
so a `card:` that is the last item is not waited for (C59-7 AC-17,
`preset.md` §6.3); two library tests that loaded a lone `test-card:` as
their last item now add `- wait: 10ms`.

| ID | Result | Test case |
|---|---|---|
| AC-32 | Pass | `lib7_ac32_one_kind_one_place_pass_through_sides_0_1`, `lib7_ac32_no_bundled_cards_fresh_card_template_without_file` (`tests/media_tests.cpp`) |
| AC-47 | Pass | `lib7_ac47_slot_picker_empty_and_14_modules_by_menu_sort_description_label` (each model), `lib7_ac47_choose_replaces_and_power_cycles`; library `ac8_slot_without_media_listed_by_the_listing`; the Qt6 and Web Settings module lists come from the `slot-1` picker (C59-6 §5's adapter gone); Swift with M-58 |
| AC-33 | Pass (session, Qt6, Web); Swift written, not run | `lib7_ac33_names_and_collisions_for_cards`; Qt6 `c59_7_ac14_…` (the cards of `AppPaths::saveDir()`, LIB-9's save folder); Web `storage.spec.js` `lib7_ac33 cards in the persistent save folder`; Swift `CardTests.test_c59_7_ac26_swift_cards_in_the_icloud_save_folder` through the coordinated access (main checkout) |
| AC-34 | Pass | `lib7_ac34_instance_written_when_given_back`, `lib7_ac34_read_pass_writes_nothing` |
| AC-35 | Pass | `lib7_ac35_card_and_arm_by_name_file_mem`, `lib7_ac35_missing_name_or_file_refuses`, `lib7_ac35_undefined_mem_fresh`, `lib7_ac35_card_file_written_in_place_unless_template`, `lib7_ac35_harness_card_mem_side_and_saveas_card` |
| AC-36 | Pass, but the File menu items (M-57, D-29) | Web `lib7_ac36 import a card file`, `lib7_ac36 saved media lists downloads deletes after asking`, `lib7_ac36 no per card download or upload in the card picker` (Import… and Saved Media… in the card dialog) |
| E1 (library) | Pass | `ac17_kind_replace_question_words`, `ac16_kind_without_name_and_save_never_offers_it` (`save_tests.cpp`), `ac12_reread_before_write_hands_current_file_as_source` (`autosave_tests.cpp`), `ac1_shelf_and_writer_through_the_hosts_file_access` (`shelf_tests.cpp`) |

Adapters removed (§11.5): A-4 (the interim TI card flow and the Web's card
commands and topic), A-5 (`TIFileAccess`'s cards; it keeps the constant
memory file), A-6 (`TICardWaitAdapter`, `TICardReaderAdapter`; the media
part is the wait probe), A-7 (`AppPaths::cardDir()`). `tools/headless/`
moves its `h_card_*` calls onto the place (§11.7). Column status: In
progress; Done in M-57 (AC-36's File menu, D-29).
