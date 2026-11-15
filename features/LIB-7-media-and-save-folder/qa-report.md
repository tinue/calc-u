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
| AC-12 | Pass | `ac12_pass_through_written_when_given_back_if_written` (`autosave_tests.cpp`) |
| AC-13 | Pass | `ac13_written_before_leaving_place`, `ac13_written_before_operation` (written when Reset has started, before its end), `ac13_written_before_machine_change` (`takeMachine`, and an attach through LIB-18), `ac13_written_on_suspend`, `ac13_written_on_close` (`AppSession::close()`) (`autosave_tests.cpp`) |
| AC-14 | Pass | `ac14_failed_write_one_notice_until_success`, `ac14_next_change_tries_again` (`settings::testing::failBeforeRename`) (`autosave_tests.cpp`) |
| AC-15 | Pass | `ac15_picker_template_not_written_lives_in_place`, `ac15_preset_named_template_not_written` (by file and by name, a bundled one too), `ac15_in_file_medium_not_written_unless_saveas` (`as: live`: that instance from then on) (`autosave_tests.cpp`) |
| AC-16 | Pass | `ac16_offered_for_template_that_can_change`, `ac16_asks_write_protect_only_when_switch_off` (`save_tests.cpp`); drawn: `ac16_name_and_save_dialog_asks_name_and_switch` (Qt6; the `asks:` question and the confirmed second send, D-6); `ac16 name and save asks the name` (`media.spec.js`, M-42) |
| AC-17 | Pass | `ac17_rules_table` (every row × column on `planSave`), `ac17_file_form_writes_that_file`, `ac17_file_form_refused_for_bundled_and_user_template`, `ac17_name_and_save_asks_before_replacing` (`save_tests.cpp`) |
| AC-18 | Pass | `ac18_place_holds_saved_medium_no_power_cycle`, `ac18_live_autosaved_template_not` (`save_tests.cpp`) |
| AC-19 | Pass | `ac19_protect_in_file_at_once_and_power_cycle`, `ac19_no_command_switches_it_off` (`save_tests.cpp`) |
| AC-20 | Pass | `ac20_started_name_asked_first_file_at_first_write` (a holding place; on the pass-through reader the started card is armed and its file written when it has passed) (`save_tests.cpp`) |
| AC-21 | Pass | `ac21_reference_mapping_name_file_mem_side`, `ac21_bare_value_refused`, `ac21_old_file_keys_refused` (`preset_media_tests.cpp`) |
| AC-22 | Pass | `ac22_in_file_media_from_header_key_saved_only_by_saveas` (the test header key `test-cards:` through `defineInFile`; gone with the next load, D-10) (`preset_media_tests.cpp`) |
| AC-23 | Pass | `ac23_missing_outdated_unreadable_refused_at_read`, `ac23_undefined_mem_is_fresh` (`preset_media_tests.cpp`) |
| AC-24 | Pass | `ac24_slot_instance_fitted_before_start_and_autosaved`, `ac24_step_inserts_instance` (`preset_media_tests.cpp`) |
| AC-25 | Pass | `ac25_template_file_never_written_wherever`, `ac25_instance_by_file_written_in_place_rerun_starts_from_it`, `ac25_log_names_kind_and_name_tag` (`preset_media_tests.cpp`) |
| AC-26 | Pass | `ac26_steps_table`, `ac26_pass_through_step_finishes_when_passed_within_timeout` (`preset_media_tests.cpp`); as commands: `ac26_steps_as_commands`, `ac26_mapping_line_as_step` (`library/harness/tests/media_commands_tests.cpp`) |
| AC-27 | Pass | `ac27_armed_never_fed_logged_at_next_build` (`preset_media_tests.cpp`) |
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

