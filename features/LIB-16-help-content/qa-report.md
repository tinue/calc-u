# LIB-16: QA report

Results per acceptance criterion and edge case of `spec.md`, per column.

## Library and Calc-U-1600 (M-52, 2026-11-16, branch `m52-help`)

Built and run on an M4 Pro, macOS 27.2, Apple clang, CMake, Ninja, in the
tree `build/tests` (Debug at `-O1`, `-DCALCU_FRONTEND_QT=OFF`,
`-DCALCU_PRODUCT_59=OFF`) and the Qt6 app tree `build/app-1600` (Release,
`tools/build_app.sh`); Python 3.15 (Homebrew) for the help tool.

What M-52 made:
- the help tool `tools/help` (`build`, `check`) and `tools/helptool/`
  (`yamlsubset.py`, `manifest.py`, `markdown.py`, `anchors.py`,
  `checks.py`, `asset.py`), Python 3 on the standard library (D-3); its
  cases in `tools/helptool/tests/` on the fixture tree
  `tests/fixtures/good/` (CTest `LibraryHelpTool`, Frequent);
- the shared parts `library/help/02-getting-started.md`,
  `03-models-and-machine-control.md`, `04-media.md`,
  `05-peripherals-and-paper.md`, `06-presets.md`, `07-debugger.md`,
  `08-settings.md`, `10-reference.md`; `help.yaml`; `anchor_cases.txt`;
- Calc-U-1600's product parts in `products/calc-u-1600/help/` (22 files,
  `help.yaml`), its pictures in `help/images/`, `settings-keys.txt`;
  `tests/help/help_tests.cpp` (CTest `CalcU1600HelpTests`), the CTest
  entries `CalcU1600HelpCheck`, `CalcU1600HelpBuild` (Frequent);
- the workflow job `help` (`.github/workflows/calc-u-1600.yml`) and the
  artifact `Calc-U-1600-help` in `products/calc-u-1600/release.yaml`;
- the screenshot scenarios regrouped per help chapter
  (`docs/developer/screenshots/guide/`), the Checkpoint entry
  `CalcU1600HelpPictures`.

Test results: `LibraryHelpTool` 34 cases pass (0.5 s);
`CalcU1600HelpTests` 3 `TEST`s, 40 checks pass; `CalcU1600HelpCheck` and
`CalcU1600HelpBuild` pass (no failure, 920 warnings of C8, below). The
whole Frequent group of the tree: see the step's report.

### Library

| ID | Result | Evidence |
|---|---|---|
| AC-1 | Pass | `test_manifest.py`: `test_ac1_ten_chapters_in_order_with_their_parts`, `test_ac1_ac3_the_repository_manifest_has_the_ten_chapters` (`library/help/help.yaml`, the one list, against AC-1's table and AC-3's specs; Part D, design §12), `test_ac1_a_chapter_out_of_order_or_with_other_parts_fails` (Part D; it replaces `test_ac1_a_retitled_chapter_fails`), `test_ac1_a_product_part_of_a_shared_only_chapter_fails`; `CalcU1600HelpCheck` |
| AC-2 | Pass | C5: `test_checks.py`: `test_ac2_shared_part_naming_a_product_word_fails`, `test_ac2_a_word_containing_a_name_is_not_the_name`, `test_ac2_picture_in_shared_part_fails`; `test_links.py`: `test_ac2_shared_part_naming_a_product_file_fails`; `CalcU1600HelpTests`: `lib16_ac2_help_names_cover_the_catalogue` (P4); the eight shared parts pass `CalcU1600HelpCheck` |
| AC-3 | Pass | C6: `test_ac3_shared_part_names_its_specs`; content review below |
| AC-4 | Pass | `test_ac4_non_markdown_source_fails`, `test_ac4_mermaid_block_allowed_in_shared_part`; the help is Markdown with pictures in `help/images/`, Mermaid in shared 02 and 04 and product 01 |
| AC-5 | Pass | C8: `test_ac5_avoid_words_listed_as_warnings_exit_zero` (warnings, exit 0; a word that is itself a glossary term is not listed: decision log, 2026-11-16) |
| AC-6 | Pass | `test_asset.py`: `test_ac6_zip_holds_files_pictures_downloads_toc`, `test_ac6_toc_lists_chapters_files_shared_product_version`, `test_ac6_version_option_wins`, `test_ac6_two_builds_same_bytes`, `test_ac6_product_by_name_or_short_form`, `test_ac6_check_writes_no_asset`; `tools/help build calc-u-1600` writes `Calc-U-1600-help.zip` (0.9.0 from `frontends/qt6/CMakeLists.txt`); the version of every ReleaseIt preset (D-9, Part D): `test_d9_version_of_the_xcode_preset`, `test_d9_version_of_the_cargo_and_regex_presets_and_an_unknown_one` |
| AC-7 | Pass | `test_ac7_missing_chapter_fails`, `test_ac7_missing_product_only_chapter_fails`, `test_ac7_file_not_in_toc_fails`, `test_ac7_unused_picture_fails`, `test_ac7_link_to_missing_file_heading_picture_fails_with_place` (each failure names the file and the line) |
| AC-8 | Pass for Calc-U-1600 | `CalcU1600HelpBuild` (the suite builds the asset); the workflow job `help` builds it and `release.yaml` attaches `Calc-U-1600-help.zip` (CI at the merge, plan §1). Calc-U-59's asset in the suite: M-60, where the column becomes Done |
| AC-14 | Pass | C7: `test_ac14_undescribed_key_fails`, `test_ac14_missing_keys_file_fails`; `CalcU1600HelpTests`: `lib16_ac14_settings_keys_file_matches_declaration`, `lib16_ac14_every_declared_key_is_in_the_keys_file`; the exit check "every settings key is described": `CalcU1600HelpCheck` passes with the 21 lines of `settings-keys.txt` (11 family, in `library/help/08-settings.md`; the product's in `help/08-settings.md`) |
| AC-15 | Pass | `test_anchors.py`: `test_ac15_every_line_of_anchor_cases` (`library/help/anchor_cases.txt`, 20 headings); `test_links.py`: `test_ac15_relative_links_rewritten_into_the_asset`, `test_ac15_a_link_into_the_shared_part_follows_the_github_anchor` |
| AC-18 | Pass (2026-11-18, release simplification R-6) | `test_checks.py`: `test_ac18_source_path_fails`, `test_ac18_milestone_fails`, `test_ac18_what_a_user_gets_passes` (C10); `tools/help check calc-u-59` and `calc-u-1600` pass after Calc-U-1600's two links into `frontends/qt6/resources/cards` became "the bundled templates" |
| EC-1 | Pass | `test_ec1_chapter_with_shared_part_only` |
| EC-2 | Pass | `test_ec2_external_link_not_followed` (`http:` is a warning) |
| EC-3 | Pass | `test_ec3_chapter_link_resolves_to_product_part`, `test_ec3_chapter_link_without_product_part_fails`; shared 03 points at `chapter:9`, whose `09-models.md` holds what differs by model |

AC-3, content review (L4): per shared part, the specs it follows and what
it covers.

| Shared part | Follows | Covers |
|---|---|---|
| `02-getting-started.md` | LIB-14, LIB-5 | the window, the menus File, Edit, Machine, Help and their items, shortcuts, the control bar, "Loading…", notices, the settings dialog, keys on the faceplate and the keyboard, full speed, Paste Text |
| `03-models-and-machine-control.md` | LIB-18, LIB-12 | model and variant, Reset and Cold boot, what follows a change of hardware (the power cycle), the startup model, the ROMs and a missing file; points at chapter 9 (EC-3) |
| `04-media.md` | LIB-7, LIB-13 | templates and instances, the save folder, choosing a medium, Name & Save, insert and eject, outdated files, opening and dropping files, Saved Media |
| `05-peripherals-and-paper.md` | LIB-2, LIB-8 | places and what fits them, attach and detach, a peripheral's controls, serial ports, clocks, the paper panel, Copy and Cut |
| `06-presets.md` | LIB-6 | loading a preset, refusals, a first preset, the steps, waiting, checking the display, startup presets, Examples |
| `07-debugger.md` | LIB-10 | running and paused, pause, step, resume, traces and their size limit, the debugger steps of a preset |
| `08-settings.md` | LIB-9 | the settings dialog, the settings file and where it lies, the family's keys, folders |
| `10-reference.md` | LIB-6 AC-1, LIB-9 | the preset format's grammar (with B59-49's lower-case keywords), the settings file's rules, the glossary of the help's terms |

### Calc-U-1600

| ID | Result | Evidence |
|---|---|---|
| AC-11 | Pass (by review) | `CalcU1600HelpCheck`; the table below: where each part of `docs/` at `b4ea8c8` went |
| AC-12 | Pass, four pictures excepted | the scenarios of `docs/developer/screenshots/guide/`, one per help chapter with pictures, run through harness attach by `tools/make_screenshots.sh` (the Checkpoint `CalcU1600HelpPictures`, by hand on macOS, 2026-11-16): 22 pictures written anew with the family's menus and bar. The four `method: system` shots (`01-app-overview`, `02-rom-menu-pc1500`, `02-machine-menu`, `03-peripherals-menu`) could not be taken: the agent's process has no Automation/Accessibility grant (the Calc-U-1600 memory `project_scripted_screenshots`); they stay 0.8's pictures until taken by hand (`tools/make_screenshots.sh 01-introduction 02-getting-started 05-peripherals` from iTerm2) |
| AC-13 | Pass (by review) | `CalcU1600HelpCheck`; the formats below |

AC-11: where the documents of today went.

| Today (`docs/`) | Help file |
|---|---|
| `User-Guide.md` §1 | `01-introduction.md` |
| §2 (models, ROMs, Reset, keyboard and faceplate, Copy Screen, Paste Text, dropping files) | `02-getting-started.md` (with the browser version and its parity table, B1600-47); dropping files to `04-media.md`; what each model keeps to `09-models.md` |
| §3 Plotters | `05-peripherals.md` (with the CE-158's printer port from §7) |
| §4 Loading a BASIC program | `09-basic-programs.md` (with the cassette) |
| §5 Memory modules and floppy disks | `04-media.md`; "A folder as a drive" to `09-host-drive.md` |
| §6 Machine-code programs | `09-machine-code.md` |
| §7 COM ports | `09-com-ports.md` |
| §8 Presets | `06-presets.md`; §8.9 to `10-preset-sections.md` |
| §9.1 Settings | `08-settings.md` |
| §9.2 Inspector | `07-debugger.md` ("The INSPECTOR") |
| §9.3 Custom memory cards | `09-module-definitions.md` |
| `Debugger.md` | `07-debugger.md` |
| `Keyboard-Mapping.md` | `10-keyboard-map.md` |
| `Memory-Card-Definition-Spec.md` | `09-module-definitions.md` |
| `Memory-Card-Definition-Format.md` | `10-module-definition-format.md` |
| `PC1600-Host-Drive.md` | `09-host-drive.md` |
| `PC1500-Keyword-Modules.md` | `09-keyword-modules.md` |
| (new, EC-3) | `09-models.md` |
| (new) | `09-product-topics.md` (the chapter's opening page), `10-reference.md`, `10-media-files.md` |

The moved files keep their history (`git mv`, D-8); every link to them in
the product (README, examples, module definitions' comments, core header
comments, developer documents, the VS Code extension's README) points at
the help. The text follows the Part D rows of the ledger (1600-1 to
1600-46) and `docs/glossary-mapping.md`'s visible renames.

AC-13: the reference chapter's formats.

| Format | File | Version |
|---|---|---|
| preset, Sharp sections, `debug:`, `bus-rom:` | `10-preset-sections.md` | preset format 2 |
| module definition | `10-module-definition-format.md` | 1 |
| floppy disk file | `10-media-files.md` | 2 |
| tape definition | `10-media-files.md` | 1 |
| settings file | `10-reference.md` (shared `10-reference.md`, `08-settings.md`) | 1 |
| keyboard map | `10-keyboard-map.md` | LIB-5 AC-29 |

### Notes

- **Warnings (C8).** The check lists 920 places of an "Avoid" word, most
  of them common words in another sense (code, load, slot, host, default,
  part, drive, card, port). They leave the exit status alone (OQ-3); the
  CTest entries count them only. Narrowing the glossary's lists is for the
  user's review of Part D.
- **Not Done here.** By the plan (§5.1) the Library column is Done in M-60
  (AC-8 builds both products' assets) and Calc-U-1600's in M-61 (the
  asset in a release). AC-9, AC-10, AC-16 are Calc-U-59's (M-60).

## Calc-U-59 and the end of the Library column (M-60, 2026-11-17, branch `m60-help-59`)

Built and run on an M4 Pro, macOS 27.2, Apple clang, CMake, Ninja, in the
tree `build/tests` (Debug at `-O1`, `-DCALCU_FRONTEND_QT=OFF`); Python 3
for the help tool. The source of the conversion: calc-u-site at
`acd629c`, read with `git archive` (`59/Pages.jsx`, `59/routes.js`,
`59/assets/`, `59/tutorial-files/`, `59/articles/fast-mode.md`); nothing
changed there.

What M-60 made:
- Calc-U-59's product parts in `products/calc-u-59/help/` (22 files,
  `help.yaml` with `names:`), its pictures in `help/images/` (the eight of
  the site), the six tutorial presets in `help/downloads/` (format 2),
  `settings-keys.txt`;
- `tests/help_tests.cpp` in `CalcU59Tests` (D-13): `lib16_ac14_settings_keys_file_matches_declaration`,
  `lib16_ac14_every_declared_key_is_in_the_keys_file`,
  `lib16_ac2_help_names_cover_the_catalogue`, `lib16_ac9_tutorial_presets_load`;
  the CTest entries `CalcU59HelpCheck`, `CalcU59HelpBuild` (Frequent);
- the workflow job `help` (`.github/workflows/calc-u-59.yml`) and the
  artifact `Calc-U-59-help` in `products/calc-u-59/release.yaml`;
- Q-3 (13.5): the shared chapter 2 keeps what both products have.

Test results: below.

### Library: AC-8

| ID | Result | Evidence |
|---|---|---|
| AC-8 | Pass | `CalcU1600HelpBuild` and `CalcU59HelpBuild` (the test suite builds both products' assets; `Calc-U-59-help.zip` 3.0.0 from the Xcode preset, 45 entries); the workflow jobs `help` of both workflows and both `release.yaml` artifacts (CI at the merge, plan §1) |

With AC-8's second asset every criterion of the Library column passes
(the other rows above, M-52, are unchanged and pass again with Q-3's
edits: `LibraryHelpTool`, `CalcU1600HelpCheck`, `CalcU1600HelpBuild`).
The column is Done (plan §5.1).

### Calc-U-59

| ID | Result | Evidence |
|---|---|---|
| AC-9 | Pass (by review) | `CalcU59HelpCheck`; the table below, page by page; `lib16_ac9_tutorial_presets_load` (P59-2) |
| AC-10 | Pass (by review) | `CalcU59HelpCheck`; the formats below |
| AC-16 | Open | the eight pictures moved; four of them show 2.3.0's control bar and are to be taken anew by hand, and the two new ones are not taken yet (below; decision log 2026-11-17) |
| AC-2 (this product) | Pass | `lib16_ac2_help_names_cover_the_catalogue` (P59-6: the three models, the PC-100C and the 14 modules of `tiCatalogue()` in `names:`, with `TMC0501`); C5 passes on the eight shared parts with Calc-U-59's names |
| AC-14 (this product) | Pass | `lib16_ac14_settings_keys_file_matches_declaration`, `lib16_ac14_every_declared_key_is_in_the_keys_file` (P59-5); C7 passes: the 25 lines of `settings-keys.txt`, the family's in `library/help/08-settings.md`, the product's in `help/08-settings.md` |

AC-9: where today's pages went (P59-1).

| Today (calc-u-site `59/`) | Help file |
|---|---|
| `/` Home, `/what-is-a-ti-59/` with the bank table | `01-introduction.md` (the bank table as Markdown) |
| `/getting-started/` overview; `/reference/` "The calculator" | `02-getting-started.md` (the calculator, the control bar, the menus and the More button, the window on each system, the keyboard, opening files) |
| `/install/…`, four pages | `02-installing.md` |
| `/getting-started/printer/`, "Magnetic cards" | `04-magnetic-cards.md` |
| `/getting-started/printer/`, the printer; `/reference/` "The printer" | `05-printer.md` |
| `/presets/` | `06-presets.md` (rewritten for format 2) |
| `/presets/tutorial/` and its six files | `06-preset-tutorial.md`, `downloads/` (upgraded, P59-2) |
| `/debugger/`; `/reference/` "The debug pane" | `07-debugger.md` |
| `/reference/` "Settings" | `08-settings.md` |
| `/modules/` | `09-modules.md` |
| `/faq/` | `09-faq.md` (answers for 3.0.0; a new one for 2.3.0 files and one for the module) |
| (new) | `09-product-topics.md`, `09-models.md` (EC-3), `09-cue-cards.md`, `09-constant-memory.md`, `09-fast-mode.md` (a summary of `59/articles/fast-mode.md`, linked) |
| the play page's keyboard legend | `10-keyboard-map.md` (checked against `app/TITyping.cpp`) |
| the file formats (`reference/StateFileFormat.md`, deleted in M-54) | `10-preset-sections.md`, from `docs/formats/` (decision log 2026-11-17) |
| `/play/`, `/software/`, the articles | stay the site's (OQ-1); linked as `https://calc-u.org/59/…` |

The text follows the ledger's Calc-U-59 rows (59-1 to 59-57) and
`docs/glossary-mapping.md`'s visible renames: Cold boot for "Clean" (no
long press, no ⌘/Ctrl), Load Preset…, Startup model, Save folder, Arm
Card and Arm Fresh Card, sides 0 and 1, `.magnetcard.yaml`,
`ti58c.memory.yaml`, the module picker and the PC-100C button on the bar,
the More button, presets of format 2 with key IDs.

AC-10: the reference chapter's formats (P59-3).

| Format | File | Version |
|---|---|---|
| preset, TI sections | `10-preset-sections.md` | preset format 2 |
| keystroke program file `.keystrokes.yaml` | `10-keystroke-programs.md` | 1 |
| magnetic card `.magnetcard.yaml` | `10-media-files.md` | 3 |
| constant memory file `ti58c.memory.yaml` | `10-media-files.md` | 1 |
| Solid State ROM module file `.rom.yaml` | `10-media-files.md` | 1 |
| ASM overlay | `10-asm-overlay.md` | `Calc-U-59 ASM 2.0` |
| settings file `Calc-U-59.ini` | `10-reference.md` (shared `10-reference.md`, `08-settings.md`) | 1 |
| keyboard map | `10-keyboard-map.md` | LIB-5 AC-21 |

AC-16: the pictures (P59-4).

| Picture | Used in | State |
|---|---|---|
| `app-icon.png` | `01-introduction.md` | moved as it is |
| `cuecard-single-label.png`, `cuecard-key-grid.png` | `06-presets.md`, `09-cue-cards.md` | moved as they are (the cue card did not change) |
| `iphone-debug.png` | `07-debugger.md` | moved as it is (the debug page alone, no bar) |
| `app-screenshot.png` | `01-introduction.md` | moved; **to be taken anew**: 2.3.0's bar |
| `ipad-13-2752x2064.png` | `02-getting-started.md` | moved; **to be taken anew**: 2.3.0's bar, the PC-100C in the paper's header |
| `iphone-duo-2852x2006.png` | `05-printer.md` | moved; **to be taken anew**: as above |
| `ipad-2752x2064-asm.png` | `07-debugger.md` | moved; **to be taken anew**: as above |
| (new) the macOS window with its menus; the iPhone's bar with the More menu open | — | **not taken** |

They are taken by hand from the main checkout (the
`calc-u-59/screenshot_convention.md` memory names the devices), the
latest at M-64's Checkpoint ("User Guide screenshots, retaken only where
something visible changed"); then AC-16 passes and, with the asset in the
release, the column is Done (M-64).

P59-8, Q-3 and the content review of the shared parts against
Calc-U-59 (AC-1, AC-2, AC-3):

| Shared part | Against Calc-U-59 | Change |
|---|---|---|
| `02-getting-started.md` | the Edit menu, Copy Screen, Paste Text and "Typing text" are Calc-U-1600's; a window without a menu bar (iOS, the Web's portrait layout) was not described | moved to Calc-U-1600's section "Copy Screen and Paste Text" (the Edit menu, the left-out character, the key that stops it, `type:`; what it said already is not repeated); the intro and the diagram name "Edit where the product has it" and the More button; the Edit row points at the product's part; the new section "Without a menu bar" (LIB-14 AC-29, C59-8 AC-5); Settings… names the Settings button; the keys' third way is "the product's own ways" (`chapter:2`) |
| `03-models-and-machine-control.md` | holds: variants, clocks and serial ports are conditional ("a model that comes in variants", "a machine with a real-time clock"); Reset and Cold boot point at chapter 9 (`09-models.md`) | none |
| `04-media.md` | holds; Saved Media is behind the More button in the Web's portrait layout (row 59-52) | "the File menu (or the More button, where the window has no menu bar)" |
| `05-peripherals-and-paper.md` | holds: the PC-100C is a one-peripheral place with a button, the module slot a picker; the text/marks control; serial ports and clocks are conditional | none |
| `06-presets.md` | holds; Load Preset… is the Preset button and Examples is behind More where there is no menu bar | one bullet under "Loading a preset" |
| `07-debugger.md` | holds: the levels `program` and `cpu`, the run-control buttons, traces, the steps | none |
| `08-settings.md` | the Settings button where there is no menu bar | named in "The Settings dialog" |
| `10-reference.md` | holds (the grammar, the settings file) | none |

`CalcU1600HelpCheck` and `CalcU59HelpCheck` pass on the result.

### Test results

- `CalcU59Tests lib16`: 4 `TEST`s, 77 checks pass (0.8 s); the whole
  `CalcU59Tests`: 595 `TEST`s, 41027 checks pass (190 s alone in this
  Debug tree; the help's cases add about 1 s; M-59 measured 32 s alone in
  RelWithDebInfo).
- `CalcU59HelpCheck` 0.3 s, `CalcU59HelpBuild` 0.4 s; `LibraryHelpTool`,
  `CalcU1600HelpTests`, `CalcU1600HelpCheck`, `CalcU1600HelpBuild` pass.
- The Frequent group of `build/tests` (`ctest -j8`): 50 of 50 pass,
  209.7 s wall.
- The six tutorial presets pass `calcu --product 59 check-preset`.

### Notes

- **Warnings (C8).** 732 places of an "Avoid" word in Calc-U-59's help
  with the shared parts, most of them "card" (the glossary avoids the bare
  word; the help uses it for the magnetic card, as the TI-59's own
  manuals do), "calculator" (the glossary's "machine", which reads oddly
  for these calculators) and common words (part, button, default, app).
  They leave the exit status alone (OQ-3); narrowing the glossary's lists
  is for the user's review of Part E.
- **Not Done here.** Calc-U-59's column is Done in M-64 (plan §5.1): the
  asset in the release, and AC-16's pictures.
