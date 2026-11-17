# QA report: C59-7 Magnetic cards

Results per acceptance criterion and edge case of `spec.md`.

## M-55 (2026-11-17, branch `m55-cards-59`)

Built and run on an M4 Pro (macOS 27.2, Apple clang, Ninja,
RelWithDebInfo): `build/tests` (`-DCALCU_FRONTEND_QT=OFF`, Frequent),
`build/qt` (`-DCALCU_FRONTEND_QT=ON`, `CalcU59QtTests`), the Web `dist/`
with `frontends/web/build.sh` and `frontends/web/run-tests.sh`. Swift:
type-checked only (`swiftc -typecheck` of `App/` for macOS and the iOS
simulator, `clang++ -fsyntax-only` of `Bridge/*.mm`); `xcodebuild` and the
Swift tests run from the main checkout. Main (M-56, `7e84245`) merged into
the branch before the end.

The code (design §2): `core/TIMagneticCard.*` (`TIMagneticCardData`,
`TICardSideDef`, `buildCardBank`, `sideBank`), `core/TICardReaderPlace.*`
(the TI-59's `card-reader/media-place`), the product kind
`c59.library-program` (`core/TIProductKinds.hpp`, `TIMemoryKinds`),
`app/TIMagneticCardKind.*` (the kind and the format 3 codec),
`app/TIMagneticCardsKey.*` (`magnetic-cards:`), `app/ModuleCueCards.*`;
`TIAppPart` on the library's media part (the cue card shown, the corner
taps); the interim card flow, `app/CardFile.*`, `TIAppPartInternal.hpp`
(`TICardWaitAdapter`), `TICardReaderAdapter`, the Web's card commands and
topic, M-56's interim file kind `magnetic-card` (`CardFitter`,
`TIAppPart::openCard`) and `roms/solid-state/cuecards.txt` are gone. Tests:
`tests/magnetic_card_tests.cpp` (T1 to T4), `tests/card_flows_tests.cpp` (T5
to T9 and LIB-13 AC-12, EC-4 on cards), `tests/CardRig.hpp`; frontends: see
AC-13, AC-14, AC-26, AC-28.

| ID | Result | Test case |
|---|---|---|
| AC-1 | Pass | `c59_7_ac1_side_holds_any_bank_either_side`, `c59_7_ac27_a_side_can_hold_any_bank_and_the_corner_follows` |
| AC-2 | Pass | `c59_7_ac2_reads_format_3_both_sides_and_one_side` |
| AC-3 | Pass | `c59_7_ac3_refused_whole_missing_side_key_rows_unknown_key_cue_card_key`, `c59_7_ac3_keys_exact_fixed_values_any_case` |
| AC-4 | Pass | `c59_7_ac4_older_version_outdated_with_hint`, `c59_7_ac4_u59_and_bin_not_listed_not_read_not_changed`, `lib13_ac12_refused_on_ti58_and_ti58c_with_notice` (a `.u59` opened: no file of the product); Web `LIB-13 AC-15 …` (`old.u59`) |
| AC-5 | Pass | `c59_7_ac5_written_whole_in_key_order_only_written_side`, `c59_7_ac5_saved_set_comments_dropped`, `c59_7_ac5_template_never_written`, `c59_7_ac5_opened_card_written_back_where_it_lies`, `lib13_ac12_card_fitted_where_it_lies_written_back`, `lib13_ac12_template_card_opened_never_written` |
| AC-6 | Pass | `c59_7_ac6_reader_on_ti59_only`; `lib1_ac33_*` (`descriptor_tests.cpp`) |
| AC-7 | Pass | `c59_7_ac7_pass_reads_or_writes_and_gives_back`, `c59_7_ac7_card_not_taken_stays_until_the_rom_takes_it` |
| AC-8 | Pass | `c59_7_ac8_bank_read_at_idle_without_a_key` |
| AC-9 | Pass | `c59_7_ac9_waits_for_medium_in_write_not_at_idle` (the wait ends when a side is inserted; R/S ending it is the ROM's, not tested here: a `key: R/S` sent while the ROM is in the card wait was not read within the test's time, see Deviations) |
| AC-10 | Pass | `c59_7_ac10_eject_keeps_whole_bank_only` |
| AC-11 | Pass | `c59_7_ac11_fitted_cue_card_with_bank_numbers_machine_sees_nothing` |
| AC-12 | Pass | `c59_7_ac12_corner_feeds_side_lit_while_in_reader_stays_fitted`, `c59_7_ac12_corner_size_fifth_width_27_7_height`; Web `C59-7 AC-12` |
| AC-13 | Pass (Qt6, Web); Swift written, not run | Qt6 `c59_7_ac13_buttons_and_eject_by_card_state` (`CalcU59QtTests`); Web `c59_7_ac13 buttons`; Swift `CalculatorView` (accessibility labels), checked by type-check only |
| AC-14 | Pass (Qt6, Web); Swift written, not run | Qt6 `c59_7_ac14_dialog_lists_saved_cards_outdated_disabled_none_found`; Web `c59_7_ac14 arm card lists the saved cards`; Swift `CardPickerView` |
| AC-15 | Pass | `c59_7_ac15_fresh_card_title_is_name_replace_asked_written_at_first_write`, `c59_7_ac27_fresh_card_replaces_the_file_of_that_name_at_its_first_write_only`; library `ac17_kind_replace_question_words`; Web `C59-7 AC-15` |
| AC-16 | Pass | `c59_7_ac16_fit_replaces_nothing_while_a_card_passes`, `lib13_ec4_card_while_one_passes_refused_nothing_imported` |
| AC-17 | Pass | `c59_7_ac17_card_finishes_when_passed_and_fitted`, `c59_7_ac17_arm_finishes_when_fitted`, `c59_7_ac17_card_not_taken_fails_after_timeout_last_item_not_waited`, `c59_7_ac17_ti58_refuses_card_arm_and_magnetic_cards`, `c59_7_ac17_harness_commands_card_and_arm` |
| AC-18 | Pass | `c59_7_ac18_items_read_bank_program_data_protected_partition`, `c59_7_ac18_outside_the_bank_or_two_names_refused` |
| AC-19 | Pass | `c59_7_ac19_built_bank_is_the_roms_bank` (the two banks the ROM wrote, `tests/fixtures/cards/*.hex`), `c59_7_ac19_partition_of_the_item_or_the_calculators`, `c59_7_ac19_side_last_written_kept_through_reset_until_load` |
| AC-20 | Pass | `c59_7_ac20_undefined_mem_is_fresh_logged_saved_only_by_saveas`, `c59_7_ac20_one_name_one_card_two_steps_fill_two_sides`; `lib7_ac35_missing_name_or_file_refuses` |
| AC-21 | Pass | `c59_7_ac21_product_kind_gives_selected_program` |
| AC-22 | Pass | `c59_7_ac22_one_fitted_at_a_time_card_section_constant_memory`, `c59_7_ac22_module_card_of_selected_program_or_fitted`, `c59_7_ac22_follows_within_half_a_second`, `c59_7_ac22_no_stand_in_made_at_run_time`; the constant memory file's card: `constant_memory_tests.cpp` (C59-3 AC-7) |
| AC-23 | Pass | `c59_7_ac23_ml_25_and_le_21_preprinted`, `c59_7_ac23_every_program_of_every_module_has_a_card`; the one-time check P10 below |
| AC-24 | Pass | `c59_7_ac24_module_card_covers_corners_do_nothing` |
| AC-25 | Pass | `c59_7_ac25_reset_power_cycle_cold_boot_keep_fitted_passing_card_taken_out`, `c59_7_ac25_new_machine_nothing_fitted` |
| AC-26 | Pass (Web, the session); Swift written, not run | Web `c59_7_ac26 cards kept from one visit to the next`, `storage.spec.js` `lib7_ac33 …`, `ac20 cards survive a page load`; `web_part_tests.cpp` `lib15_ac20_cards_are_files_of_the_save_folder`; Swift `CardTests.test_c59_7_ac26_swift_cards_in_the_icloud_save_folder` (run from the main checkout) |
| AC-27 | Pass | the `c59_7_ac27_*` cases of `card_flows_tests.cpp` (written, read back; fed only on a corner; a read writes nothing; both sides in one file; the other side kept; a fresh card replacing a file only at its first write; `arm:`/`card:` finish; a preset's card read; a card written then inserted; an armed card never fed logged) and `lib13_ac12_*`; the `.u59` and version 1 cases went (spec, TR) |
| AC-28 | Web passes; Qt6 and Swift in M-57 and M-58 (D-11) | Web `C59-7 AC-15`, `C59-7 AC-12` |
| AC-29 | Pass | `c59_7_ac29_info_name_description_template_no_switch`; library `ac16_kind_without_name_and_save_never_offers_it` |
| AC-30 | Pass | the four examples upgraded (below); `calcu_preset_check.CalcU59Presets` without exclusions; `CalcU59ReferenceRuns`: 0 of 57 differ, 10 named findings, the four card examples compared in their in-file and written card banks too |
| EC-1 | Pass | `c59_7_ec1_other_partition_not_stored_display_blinks` |
| EC-2 | Pass | `c59_7_ec2_empty_side_zeroes_rom_errors_nothing_written`, `c59_7_ac27_blank_side_fed_at_partition_0_writes_nothing` |
| EC-3 | Pass | `c59_7_ec3_other_side_kept_as_the_file_holds_it`, `c59_7_ac27_write_keeps_the_side_changed_elsewhere_and_refuses_a_broken_file`; library `ac12_reread_before_write_hands_current_file_as_source` |
| EC-4 | Pass | `c59_7_ac27_write_keeps_the_side_changed_elsewhere_and_refuses_a_broken_file` (the notice "could not be saved"), library `ac12_reread_…` |
| EC-5 | Pass | `c59_7_ec5_bom_or_not_utf8_unreadable` |
| EC-6 | Pass | `c59_7_ec6_missing_outdated_unreadable_card_refuses_preset` |
| EC-7 | Pass | `c59_7_ec7_protected_bank_as_the_rom_does` |

Column status (INDEX): In progress; Done in M-58 (AC-28's Qt6 and Swift
scenarios, D-11; AC-13, AC-14 and AC-26 on Swift run from the main checkout).

### M-58 (2026-11-17, branch `m58-swift-59`; Swift)

The card controls are C59-8's bar (`CardControls` in
`Views/ControlBarView.swift`, following the File menu's card items, M17);
the scenarios of AC-28 are `Calc-U-59UITests/CardScenarioTests.swift`,
run on the iPhone 18 Pro Max simulator with the presets and the program's
keys through attach. Qt6's two scenarios (M-57 had left them) are
`CalcU59QtTests` `c59_7_ac15_qt6_fresh_card_written_and_listed` and
`c59_7_ac12_qt6_card_read_by_its_corner` (`window_tests.cpp`, on the real
window: what the bar's Arm Fresh Card and Arm Card do, the press on the
card's corner through the faceplate's hit rule, the card written and
listed; on a fresh machine read back: program memory holds Lbl A 1 2 3
R/S), offscreen, passing.

| ID | Result | Test case |
|---|---|---|
| AC-13 | Pass (Swift) | XCUITest `FamilyStructureTests.test_lib14_ac1_the_menus`, `test_lib14_ac8_the_bar` (Arm Card and Arm Fresh Card on the TI-59's bar, read "Crd", named for a screen reader), `test_c59_8_ec3_…` (none on the TI-58), `CardScenarioTests` (Eject Card while a side passes); `c59_8_ac18_…` (File ▸ Arm Card…, Arm Fresh Card… on macOS) |
| AC-14 | Pass (Swift) | `CardScenarioTests.test_c59_7_ac15_fresh_card_written_and_listed` (Arm Card lists the written card by its name), `test_c59_7_ac12_…` (chosen from the list, fitted) |
| AC-26 | Pass (Swift) | `CardTests.test_c59_7_ac26_swift_cards_in_the_icloud_save_folder` (macOS unit tests, M-58's run) |
| AC-28 | Pass (Web, Qt6, Swift) | Swift `CardScenarioTests.test_c59_7_ac15_fresh_card_written_and_listed` (Arm Fresh Card, a name, the corner pressed while the program waits in `CLR 1 2nd Write`, the card given back and listed by Arm Card), `test_c59_7_ac12_card_read_by_its_corner` (that card armed on a fresh machine, its corner pressed: A shows 123) |

Fixed on the way: the Swift card dialog named every element inside it
"Arm Card" (the container's `accessibilityLabel`), so its Cancel and Arm
buttons could not be told apart; the container is an element of its own
now, the buttons keep their names (`card-cancel`, `card-arm`, `card-name`).

Every criterion and edge case passes in every frontend: **Done**.

### The four card examples (AC-30)

Upgraded by a throwaway script by `preset.md` §9.1 and AC-30's rows (not in
the repository, as M-54's): `CARD: mem:X … END-CARD` → an item of
`magnetic-cards:` (side 0); `InsertCard: mem:X` → `- card: { mem: X }`;
`ArmCard: mem:X` → `- card: { mem: X }` after the key line that makes the
program wait (each was followed by the `R/S` that reaches `n Write`); the
card files `PDCM-1`, `PDCM-2` of `pdcm-make-cards` (which did not exist) →
in-file cards with the cue card of their first use, fed with `mem:` and
saved by `- saveas: { card: X, as: live, name: X }` at the end;
`Template`/`Banks` dropped, `ID` kept as a comment. `globe-print` is back in
`examples/bundle.yaml` (ten examples on the Web again, C59-2 AC-13),
`preset-name: Globe`. Reference runs: `pdcm-card-tricks` (5 in-file data
cards) and `pdcm-make-cards` (both sides of both written cards) equal the
M-9 records in every part; `globe-print` differs in its paper marks only,
as the other printer-interrupt pictures (named finding); `characters-fastmode`
is a named finding: the 2.3.0 record was taken 2.7 s after its playback
ended, while the table was still printing; the run now ends at idle, with
the whole table printed (checked by hand: the character table, then `0.`).
`examples/historic/PDCM-Emulator.md` and `PDCM.md` follow ("Arm Card", sides
0 and 1, the save folder).

### P10 of LIB-12 design §10 (one-time, before `cuecards.txt` was deleted)

Each module file's `cue-cards:` against `roms/solid-state/cuecards.txt`,
field by field (`Template` dropped): ML 25 and LE 21 pre-printed cards equal;
every other program of the 14 modules a stand-in with the module's
`description` as `title` and `module-name` as `id` (AG 17, AV 23, BD 11,
EE 19, MU 21, NG 30, RE 17, RP 64, SA 15, SE 9, ST 22, SY 25); no card of
`cuecards.txt` missing; 0 problems.

### Deviations from the design

- `libExecPC` (design §3.6: a register of the debug target) is a member of
  the product kind `c59.calculator-state` (M-56's), not a register: the
  register list of the TMC0501 target stays as LIB-10 has it.
- `rereadBeforeWrite`: a file that is missing at the write (deleted
  elsewhere) is written whole from the fitted card; one that cannot be read
  fails the write with a notice (EC-4).
- An in-file side that never passed and is saved by `saveas:` is built with
  its definition's partition, else the TI-59's power-on partition
  (60 program registers): the codec does not see the machine.
- `magnetic-cards:` takes `sides: {}` (a fresh card with a cue card), for
  AC-30's "card file that does not exist yet … with that cue-card".
- The cue card shown follows the media part's binding by an arm serial
  (`Binding::armed`, a library addition), so "the last of three events"
  (D-6) is told without a callback.
- The drop flow of design §3.7 (`media.open-file` until M-56) is LIB-13's
  `open` since the merge with M-56: a card goes through the library's
  MediaOpener.
- AC-9's "R/S ends the wait" has no test (above).

### Findings for the user

- `docs/formats/preset.md` §9.3 says a format 3 card row holds "its 8
  bytes in reverse order" against the 2.0 rows. The 2.0 rows held the bank
  bytes in order, and the bank bytes are already `README.md` §1.1's
  register-hex (checked with the ROM-written fixture `bank4-data.hex`:
  `R29 = 2.25e-7` is `22 05 … 47`); the codec writes the bank bytes in order,
  which is `README.md`'s encoding. §9.3's sentence looks wrong.
- `characters-fastmode` (above): the reference record catches a moment of a
  running print; a record taken at idle would compare it.
- Row 59-57 (new): the stand-in module card is titled with `description`
  (AC-23) where 2.3.0 used the module's `title`.
