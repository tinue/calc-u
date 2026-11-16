# LIB-16 Help content: design
Status: approved by the user at the Part C2/D boundary (2026-11-16)

## 1. Scope

| Column | Step | Tests from (plan §3) | Done in (plan §5.1) | In this design |
|---|---|---|---|---|
| Library | M-52 | AC-1–8, AC-14, AC-15; EC-1–3 | M-52 (proposed M-60, §10) | designed |
| Calc-U-1600 | M-52 | AC-11–13 | M-61 (the asset in the release) | designed |
| Calc-U-59 | M-60 | AC-9, AC-10, AC-16 | M-64 | listed only (§4.2) |

- M-52 waits for M-46 to M-51 (the chapters describe what those steps
  build) and for M-49 (AC-12's pictures are taken through attach). It
  touches no core and no app code.
- The help tool is this feature's only code (spec: "Library"). The rest
  of the step is writing: the shared parts once, Calc-U-1600's product
  parts converted from its `docs/`.
- Not here: the site that renders the asset (LIB-19, M-62); the Help
  menu item (LIB-14 AC-20, built in M-43, its address in M-49);
  developer documentation, which stays in `docs/developer/` and
  `docs/background/` (`help.md`, non-responsibilities).

## 2. Modules and files

### 2.1 The help tool (new: `tools/` at the repository root)

A Python 3 program on the standard library only (D-3), so that it runs
wherever the workflows run, without an install.

| File | Holds |
|---|---|
| `tools/help` | the entry, executable: `tools/help build <product> [--version <v>] [--out <dir>]`, `tools/help check <product>` (AC-6). `<product>` is the product's name or short form, any case, as `calcu --product` takes it (`calc-u-1600`, `Calc-U-1600`, `1600`) |
| `tools/helptool/manifest.py` | reads the shared manifest and the product's (§3.1) |
| `tools/helptool/markdown.py` | headings, links, pictures and fenced blocks of a Markdown file (no full renderer: only what the checks need) |
| `tools/helptool/anchors.py` | the anchor of a heading by GitHub's rule (AC-15; §3.4) |
| `tools/helptool/checks.py` | the checks of AC-7, AC-2, AC-3, AC-5, AC-14 (§3.5) |
| `tools/helptool/asset.py` | `toc.json` and the zip (§3.3) |
| `tools/helptool/tests/test_*.py` | `unittest` cases (§6), with small help trees in `tools/helptool/tests/fixtures/` |
| `tools/README.md` | what is in `tools/` and how to run it |

CMake (root `CMakeLists.txt`, with `find_package(Python3)`): the CTest
entries `LibraryHelpTool` (the unit tests), `CalcU1600HelpCheck`
(`tools/help check calc-u-1600`) and `CalcU1600HelpBuild` (`tools/help
build calc-u-1600 --out <build tree>/help`), all Frequent (a few
seconds together).

### 2.2 The help sources

| Folder | Holds |
|---|---|
| `library/help/` (new) | the shared parts (§4.1), `help.yaml` (the shared manifest: the ten chapters of AC-1 with number, title, parts, and the shared files of each), `anchor_cases.txt` (headings and their anchors, for the tool's tests and for LIB-19's renderer later) |
| `products/calc-u-1600/help/` (new) | the product parts (§4.2), `help.yaml` (the product's manifest), `images/`, `downloads/` (the presets a tutorial offers), `settings-keys.txt` (§3.5, written by a test) |

The folders follow ADR 0036: the shared chapters with the library, the
product chapters with the product. They are not under `docs/`, which at
the root is the specification and in a product is developer
documentation (D-1).

## 3. Seams and interfaces

### 3.1 The manifests

`library/help/help.yaml`:

```yaml
format-version: 1
chapters:
  - { number: 1, title: Introduction, parts: [product] }
  - { number: 2, title: Getting started, parts: [shared, product], shared: [02-getting-started.md], follows: [LIB-14, LIB-5] }
  - { number: 3, title: Models and machine control, parts: [shared], shared: [03-models-and-machine-control.md], follows: [LIB-18, LIB-12] }
  # … 4 to 10 as AC-1's table; `follows:` the specs of AC-3
```

`products/calc-u-1600/help/help.yaml`:

```yaml
format-version: 1
product: Calc-U-1600
short: "1600"
site: https://calc-u.org/1600/
version-from: ../release.yaml          # its `version:` preset and file (D-9)
names: [Calc-U-1600, PC-1500, PC-1500A, PC-1600, CE-150, CE-158, CE-1600P, CE-1600F, …]   # AC-2 (D-7)
chapters:
  1: [01-introduction.md]
  2: [02-getting-started.md]
  4: [04-media.md]
  # …
  9: [09-product-topics.md, 09-basic-programs.md, 09-machine-code.md, 09-com-ports.md, 09-host-drive.md, 09-keyword-modules.md, 09-module-definitions.md, 09-models.md]
  10: [10-reference.md, 10-preset-sections.md, 10-module-definition-format.md, 10-media-files.md, 10-keyboard-map.md]
downloads: [downloads/…]
```

Both are read with a small YAML reader for this subset (flow mappings,
block sequences and mappings, scalars), in `manifest.py`; the files keep
to it, and the tool refuses anything else with its line.

### 3.2 Links (AC-15, EC-2, EC-3)

- In the source a link to another help file of the same product, to a
  heading or to a picture is relative to the file, as GitHub shows it:
  from `products/calc-u-1600/help/04-media.md` to the shared part
  `../../../library/help/04-media.md#name--save`.
- A shared part may not name a product's file (AC-2). To point at the
  product's part of another chapter (EC-3: what differs by model) it
  writes `chapter:<n>` as the link target; the tool resolves it to the
  first product file of chapter n, and fails when the product has none.
- A link with a scheme (`https:`, `mailto:`) is outside the help: not
  followed, never a failure (EC-2). An `http:` link is a warning.
- At build time the tool rewrites every relative link to the file's
  place in the asset (§3.3), so the asset's links are relative there.

### 3.3 The asset (AC-6, OQ-2)

`Calc-U-1600-help.zip`, built into `--out` (default `build/help/`):

```
toc.json
shared/<the shared files>
product/<the product files>
product/images/<pictures>
product/downloads/<files>
```

`toc.json`:

```json
{"product": "Calc-U-1600", "version": "1.0.0", "format-version": 1,
 "chapters": [
   {"number": 1, "title": "Introduction",
    "files": [{"path": "product/01-introduction.md", "shared": false}]},
   {"number": 2, "title": "Getting started",
    "files": [{"path": "shared/02-getting-started.md", "shared": true},
              {"path": "product/02-getting-started.md", "shared": false}]}
 ]}
```

In each chapter the shared files come first, then the product's, each
in manifest order. The version is `--version`, else the product's
version read as ReleaseIt reads it (`release.yaml`'s `version:`; for
Calc-U-1600 `set(APP_VERSION "…")` in `frontends/qt6/CMakeLists.txt`,
for Calc-U-59 the first `MARKETING_VERSION` of its Xcode project;
D-9). The zip's entries have a fixed time stamp and order, so two
builds of one tree give the same bytes.

### 3.4 Anchors (AC-15)

GitHub's rule for Markdown headings: the heading's text without
Markdown marks, lower case; every character that is not a letter, a
digit, a space, a hyphen or an underscore removed; each space a hyphen;
a second heading with the same anchor gets `-1`, a third `-2`. The rule
is written once in `anchors.py`; its cases are `library/help/anchor_cases.txt`
(one line per heading and anchor, umlauts, `&`, `…`, `▸` and repeated
headings among them), which LIB-19's renderer tests against too (M-62).

### 3.5 The checks (AC-2, AC-3, AC-5, AC-7, AC-14)

`check` runs all; `build` runs all and then writes the asset. A failure
names the file and the line; a warning is printed and does not change
the exit status (0 without failures, 1 with, 2 for a usage error).

| # | Check | Fails / warns | AC |
|---|---|---|---|
| C1 | the chapters of `library/help/help.yaml` (the one list of AC-1's chapters, their parts and AC-3's specs; Part D) are numbered 1 to n in order, none missing, each with a title and the parts `shared`, `product` or both, and each has the parts its row names; a chapter with both parts may lack the product part (EC-1) | fails | AC-1, AC-7, EC-1 |
| C2 | every relative link leads to a file of this help, every `#anchor` to a heading of that file, every picture to a file | fails | AC-7, AC-15 |
| C3 | every `.md` and picture under the two help folders is in a manifest (the manifest, `anchor_cases.txt`, `settings-keys.txt` aside) | fails | AC-7 |
| C4 | every picture is used by a file | fails | AC-7 |
| C5 | no shared file names a word of any product's `names:` (whole words, case as written) and no shared file has a picture other than a Mermaid block | fails | AC-2, AC-7 |
| C6 | each shared file starts with `<!-- follows: LIB-14, LIB-5 -->`, the specs of its chapter in AC-3's table | fails | AC-3 |
| C7 | every key of `settings-keys.txt` is written as `` `[section] key` `` in chapter 8: the family's in the shared part, the product's in the product part | fails | AC-14 |
| C8 | every word that `docs/GLOSSARY.md` lists after `_Avoid_:` (the words before a parenthesis) is listed where it stands | warns | AC-5 |
| C9 | a `chapter:<n>` link resolves (§3.2) | fails | EC-3 |

**The settings keys (C7).** The tool cannot read the C++ declaration,
so a Frequent test of the product writes it out:
`lib16_ac14_settings_keys_file_matches_declaration` (`CoreTests`)
compares `products/calc-u-1600/help/settings-keys.txt` with
`shared::settingsDecl()` and fails when they differ
(`CALCU1600_HELP_KEYS=1` writes it anew). One line per key,
`family [startup] model` or `product [debug] dap-port`; a family key is
one that a `calcu::settings::family::` function makes
(`library/settings/Declaration.hpp:52-64`). A family key whose name the
product gives is written once in its general form, `family [preset]
<model>`, `family [folders] <kind>`, and the product's names then as
product lines (`product [folders] basic-programs`), so the shared part
describes the rule and the product part the names, and no shared file
names a model (AC-2).

**The product names (C5).** `names:` lists the product's name, its
models and every peripheral of its catalogue and module definitions.
`lib16_ac2_help_names_cover_the_catalogue` (`CoreTests`) fails when a
model or peripheral of `sharp::sharpCatalogue()` with the bundled module
definitions is missing from it (D-7).

## 4. Content

### 4.1 The shared parts (Library, M-52)

Written new, from the specs (AC-3) and in the glossary's terms, one file
per chapter that has a shared part. No product's name, model or
peripheral, no window picture; Mermaid where a diagram helps (AC-4).

| File | Chapter | Follows (AC-3) | Holds |
|---|---|---|---|
| `02-getting-started.md` | 2 | LIB-14, LIB-5 | the window: faceplate, control bar, panel area; the menus File, Edit, Machine, Help and their items; "Loading…"; notices; the settings dialog in brief; keys on the faceplate and the keyboard, Paste Text |
| `03-models-and-machine-control.md` | 3 | LIB-18, LIB-12 | model and variant, Reset, Cold boot, the power cycle after attaching, the startup model; the ROM full set and what a missing file shows; what differs by model points at `chapter:9` (EC-3) |
| `04-media.md` | 4 | LIB-7, LIB-13 | templates and instances, Name & Save, insert and eject, the save folder, outdated files; opening and dropping files |
| `05-peripherals-and-paper.md` | 5 | LIB-2, LIB-8 | places and peripherals, attach and detach on the bar and in Peripherals ▸; the paper panel, Copy, Cut |
| `06-presets.md` | 6 | LIB-6 | what a preset is, the header, the steps, waiting, `expect:`, startup presets, Examples ▸ |
| `07-debugger.md` | 7 | LIB-10 | pause, step, resume, the run control buttons, Resume in the Machine menu, traces and their size limit, the debugger steps of a preset |
| `08-settings.md` | 8 | LIB-9 | the settings file, where it lies, the family's keys (C7) |
| `10-reference.md` | 10 | LIB-6 AC-1, LIB-9 | the grammar of the preset format once; the rules of the settings file once; the glossary of the terms the help uses |

### 4.2 Calc-U-1600's product parts (M-52)

Converted from `products/calc-u-1600/docs/` (AC-11), rewritten where
the rows of Part D changed what they describe (AC-3; the ledger's rows
1600-1 to 1600-37 are the list of such places) and renamed by
`docs/glossary-mapping.md`'s visible renames ("memory card" → memory
module, "custom memory cards" → module definitions, "Reset All" → Cold
boot, "Default presets" → Startup presets, "Battery-card saves" → Save
folder, "Startup device" → Startup model, "Samples folder" → Examples
folder):

| Today | Goes to |
|---|---|
| `docs/User-Guide.md` §1 (`:29-60`) | `01-introduction.md` |
| §2 (`:61-191`; models, ROM choices, Reset, keyboard and faceplate, Copy Screen and Paste Text, dropping files) | `02-getting-started.md` (the faceplate, the keyboard map in short; the rest is in the shared parts of 2, 3 and 4) |
| §3 Plotters (`:192-236`) | `05-peripherals.md` (CE-150, CE-1600P, CE-158 as devices) |
| §4 Loading a BASIC program (`:237-373`) | `09-basic-programs.md` (with the cassette) |
| §5 Memory modules and floppy disks (`:374-525`) | `04-media.md`; "A folder as a drive" (`:460-483`) to `09-host-drive.md` |
| §6 Machine-code programs (`:526-584`) | `09-machine-code.md` |
| §7 COM ports (`:585-673`) | `09-com-ports.md` |
| §8 Presets (`:674-911`) | `06-presets.md` (the Sharp header keys and steps, the examples); the reference part (§8.9) to `10-preset-sections.md` |
| §9.1 Settings (`:914-940`) | `08-settings.md` (the product's keys, C7) |
| §9.2 Inspector (`:941-994`) | `07-debugger.md` |
| §9.3 Custom memory cards (`:995-`) | `09-module-definitions.md` |
| `docs/Debugger.md` | `07-debugger.md` (VS Code, the INSPECTOR) |
| `docs/Keyboard-Mapping.md` | `10-keyboard-map.md` |
| `docs/Memory-Card-Definition-Spec.md` | `09-module-definitions.md` |
| `docs/Memory-Card-Definition-Format.md` | `10-module-definition-format.md` |
| `docs/PC1600-Host-Drive.md` | `09-host-drive.md` |
| `docs/PC1500-Keyword-Modules.md` | `09-keyword-modules.md` |
| (new, EC-3) | `09-models.md`: what Reset, Cold boot and a power cycle keep per model and module (C1600-1 AC-6; rows 1600-4, 1600-16, 1600-17) |
| (new, AC-13) | `10-media-files.md`: the floppy disk file (LIB-7 AC-37) and the tape definition (LIB-7 AC-42); `10-reference.md`: the settings file of this product, links to the shared reference |

`09-product-topics.md` is the chapter's opening page that lists its
files. AC-13's reference chapter: the preset format with the Sharp
sections and the YAML of `debug:` and `bus-rom:` (C1600-4 AC-20) in
`10-preset-sections.md`; the module definition; the floppy disk file
and the tape definition; the settings file; the keyboard map (LIB-5
AC-29).

**The old files.** The files of the table are moved with `git mv` into
`products/calc-u-1600/help/` and then rewritten, so their history stays
(D-8); `docs/images/guide/` moves to `help/images/`. What links to them
is changed in the same step: `products/calc-u-1600/README.md`,
`vscode/calcu1600-debug/README.md`, `examples/README.md`,
`examples/setup/README.md`, the comments in
`examples/memory/memory-cards/*`, `examples/dwx/*.card.yaml` and the
bundled module definitions (`frontends/qt6/resources/cards/*.card.yaml`),
the core headers that name `Memory-Card-Definition-*.md` or
`PC1600-Host-Drive.md` (`core/Connector/*.hpp`, `core/Preset/*.hpp`,
`core/PC1500/PC1500PresetLoader.hpp`, `core/PC1600/PC1600PresetLoader.*`,
`core/Debug/AttachKeys.hpp`; comments only), `firmware/pc1600-hostdrive/hostdrive.asm`
(a comment; its `.lst` is generated) and the developer documents
(`docs/developer/README.md`, `Building.md`, `Debugger-Internals.md`,
`Floppy-Image-Format.md`, `PC1600-Host-Drive-Internals.md`,
`PC1600-Serial-Port.md`, `screenshots/README.md`). `CHANGELOG.md`'s past
entries and `docs/background/` stay as they are (history).

**The pictures (AC-12).** The scenarios of
`docs/developer/screenshots/guide/` are regrouped by help chapter, one
per chapter that has pictures, and write into
`products/calc-u-1600/help/images/` (`out:`):

| Scenario today | Scenario after M-52 |
|---|---|
| `01-introduction` | `01-introduction` |
| `02-getting-started` | `02-getting-started` |
| `05-modules` | `04-media` |
| `03-plotter`, `07-com-ports` (the CE-158's printer panel) | `05-peripherals` |
| `08-presets` | `06-presets` |
| `09-appendix` (the INSPECTOR) | `07-debugger` |
| `09-appendix` (settings), `07-com-ports` (serial ports) | `08-settings` |
| `04-basic`, `06-machine-code` | `09-product-topics` |

They run through attach (LIB-14 AC-44, M-49), so they show the family's
menus and bar. They are taken by hand on macOS once in M-52 (the two
`method: system` shots need the permissions of the Calc-U-1600 memory
`project_scripted_screenshots`) with `tools/make_screenshots.sh`, whose
Checkpoint entry is renamed `CalcU1600HelpPictures`. The Longer entries
`CalcU1600Shots.*` follow the new names.

### 4.3 Calc-U-59 (M-60; listed only)

Its product parts are converted from calc-u-site's pages (AC-9), its
reference chapter (AC-10), and its pictures by hand where they changed
(AC-16). The tool and the shared parts of M-52 serve it unchanged; M-60
adds `products/calc-u-59/help/` and its CTest entries. The skill
`calc-u-59-docs` changes then.

## 5. Adapters

None: no old call is wrapped. The old user documents are moved, not
kept beside the help (D-8).

## 6. Tasks

Tests before the code. The tool's cases are `unittest` cases in
`tools/helptool/tests/` (CTest `LibraryHelpTool`, Frequent), each on a
small help tree under `fixtures/`; names `test_acN_…`, `test_ecN_…`.
The product's are in `CoreTests` (`tests/help_tests.cpp`, Frequent) and
the CTest entries of §2.1.

| Task | What | ACs/ECs | Test |
|---|---|---|---|
| L1 | Manifests, `toc.json`, the zip | AC-1, AC-6 | `test_manifest.py`: `test_ac1_ten_chapters_in_order_with_their_parts`; `test_asset.py`: `test_ac6_zip_holds_files_pictures_downloads_toc`, `test_ac6_toc_lists_chapters_files_shared_product_version`, `test_ac6_two_builds_same_bytes`, `test_ac6_product_by_name_or_short_form`; the version presets (D-9, Part D §12): `test_d9_version_of_the_xcode_preset`, `test_d9_version_of_the_cargo_and_regex_presets_and_an_unknown_one` |
| L2 | Anchors, links | AC-15, AC-7 (links), EC-2 | `test_anchors.py`: `test_ac15_every_line_of_anchor_cases`; `test_links.py`: `test_ac15_relative_links_rewritten_into_the_asset`, `test_ac7_link_to_missing_file_heading_picture_fails_with_place`, `test_ec2_external_link_not_followed` |
| L3 | The other checks of AC-7 | AC-7, AC-2, EC-1 | `test_checks.py`: `test_ac7_missing_chapter_fails`, `test_ac7_file_not_in_toc_fails`, `test_ac7_unused_picture_fails`, `test_ac2_shared_part_naming_a_product_word_fails`, `test_ac2_picture_in_shared_part_fails`, `test_ec1_chapter_with_shared_part_only` |
| L4 | Specs followed | AC-3 | `test_ac3_shared_part_names_its_specs` (C6); the content review in the qa-report: per shared file, the ACs of its specs it covers |
| L5 | Settings keys | AC-14 | `test_ac14_undescribed_key_fails` (C7); `CoreTests`: `lib16_ac14_settings_keys_file_matches_declaration` |
| L6 | Glossary words | AC-5 | `test_ac5_avoid_words_listed_as_warnings_exit_zero` (C8, a `GLOSSARY.md` fixture) |
| L7 | Markdown and pictures | AC-4 | `test_ac4_non_markdown_source_fails`, `test_ac4_mermaid_block_allowed_in_shared_part` |
| L8 | `chapter:` links | EC-3 | `test_ec3_chapter_link_resolves_to_product_part`, `test_ec3_chapter_link_without_product_part_fails` |
| L9 | The shared parts written | AC-1, AC-2, AC-3, EC-3 | `CalcU1600HelpCheck` passes with them (the shared parts are checked within a product's help) |
| L10 | The release | AC-8 | `CalcU1600HelpBuild` (the test suite builds the asset); `.github/workflows/calc-u-1600.yml`: a job `help` (ubuntu, Python 3) runs `tools/help build calc-u-1600` and uploads the artifact `Calc-U-1600-help`; `release.yaml` gets `{ artifact: Calc-U-1600-help, file: Calc-U-1600-help.zip, as: Calc-U-1600-help.zip }`; one CI dispatch at the merge. Calc-U-59's asset in the suite: M-60 (§10) |
| P1 | Calc-U-1600's product parts | AC-11 | `CalcU1600HelpCheck`; the qa-report's table of AC-11, row by row: where each section of today went |
| P2 | Pictures | AC-12 | the regrouped scenarios as `CalcU1600Shots.*` (Longer) and `CalcU1600HelpPictures` (Checkpoint, run once by hand in M-52); `CalcU1600HelpCheck` (every picture used, every use a file) |
| P3 | Reference chapter | AC-13 | `CalcU1600HelpCheck`; the qa-report lists each format of AC-13 with its file and version |
| P4 | Product names | AC-2 | `CoreTests`: `lib16_ac2_help_names_cover_the_catalogue` |

Coverage: Library AC-1 to AC-8, AC-14, AC-15 and EC-1 to EC-3 in L1 to
L10 (AC-8's Calc-U-59 half in M-60); Calc-U-1600 AC-11 to AC-13 in P1 to
P3.

## 7. Tools and agent knowledge (changed in the same step)

| File | Change |
|---|---|
| `products/calc-u-1600/docs/developer/README.md` | where the help lives (`help/`, `library/help/`), `tools/help check`, the scenarios for its pictures (plan M-52) |
| `products/calc-u-1600/CLAUDE.md` | one line: user help is `help/` (product parts) and `library/help/` (shared parts), checked by `tools/help check calc-u-1600` in Frequent; a user-visible change updates its chapter in the same step |
| `products/calc-u-1600/README.md` | the user guide is at `https://calc-u.org/1600/` and in `help/` |
| `products/calc-u-1600/docs/developer/screenshots/README.md` | the scenarios per help chapter, `out:` in `help/images/` |
| `CLAUDE.md` (root) | under Key conventions, one line: help is Markdown in `library/help/` and `products/<product>/help/`, built by `tools/help` (ADR 0036) |
| `docs/test-groups.md` | `LibraryHelpTool`, `CalcU1600HelpCheck`, `CalcU1600HelpBuild`, `CalcU1600HelpPictures` |
| memory `calc-u-1600/feedback_docs_three_levels.md` | the user level is the help (`help/`, on calc-u.org/1600), no longer `docs/` |
| memory `calc-u-1600/feedback_user_manuals_use_case_structure.md` | the help has the family's ten chapters (LIB-16 AC-1); inside a chapter, use cases first |
| memory `calc-u-1600/project_scripted_screenshots.md` | one scenario per help chapter, into `help/images/` |

## 8. Golden runs and risks

- **Golden runs** are not touched: no core, no app code.
- **The amount of writing.** Eight shared parts are new text, and the
  product parts are rewritten for the changes of Part D. The checks
  catch structure, not meaning; the content review (L4, P1) is the
  check of AC-3 and AC-11. The user reviews the help with Part D's end.
- **Shared parts without a product.** Every example belongs in a
  product part (HL-1 (b)); a shared text that needs one points to the
  product's chapter. C5 catches a product word, not a product-only idea.
- **Pictures through attach** are not byte-identical (LIB-14 Part D,
  D.11): the pictures in `help/images/` change only when they are taken
  again on purpose (Checkpoint), not at every run.
- **The anchor rule** must match calc-u-site's renderer (LIB-19, M-62);
  `anchor_cases.txt` is the one list both test against.
- **CI.** The workflow gets a job and `release.yaml` an artifact: one CI
  dispatch at the merge (plan §1).

## 9. Decided (best guess)

- **D-1.** The shared parts in `library/help/`, the product parts in
  `products/<product>/help/` (ADR 0036: with the library, with each
  product).
- **D-2.** A manifest per side lists the chapters and files in order;
  the table of contents and C3 come from it, and a chapter may have
  several files.
- **D-3 (OQ-6).** The tool is Python 3 on the standard library, at the
  repository's `tools/help`: the workflows have Python, and no install
  or build is needed.
- **D-4 (AC-6, AC-15).** The asset holds `shared/` and `product/`;
  links are written as GitHub shows them in the source and rewritten
  into the asset's places at build time.
- **D-5 (EC-3).** A shared part points into the product's chapter with
  `chapter:<n>`, the one link form a shared part has for product
  material.
- **D-6 (AC-14).** The declared keys reach the tool through a file that
  a product test writes and checks, so the tool needs no C++.
- **D-7 (AC-2).** The words a shared part may not use are each
  product's `names:`, checked against the catalogue by a product test.
- **D-8.** The old user documents are moved (history kept), not left
  beside the help; developer documents stay where they are.
- **D-9.** The asset's version is read as ReleaseIt reads it, from the
  product's `release.yaml`; `--version` overrides it. Since Part D every
  preset of ReleaseIt (`src/version.rs`): `xcode` (the first
  `MARKETING_VERSION = …;`, the app's; the test targets after it sit at
  1.0), `cmake`, `cargo` (each with its default file) and `regex`
  (`version.regex`, its first group); M-53 to M-60 need `xcode`.
- **D-10.** Warnings (AC-5) are printed and leave the exit status alone
  (OQ-3).

## 10. Plan notes (proposals; the plan is not edited)

- AC-8 asks that "the test suite builds both products' assets", and
  Calc-U-59's chapters come in M-60. By plan §1 ("a criterion that names
  a feature built later passes in that feature's step") the Library
  column is Done in M-60, not M-52: §5.1's row LIB-16 would read
  "1–8, 14–15: M-52 / M-60".
- M-52 "Knowledge, ledger" gains the files of §7 (the root and product
  `CLAUDE.md`, the README, the three memory files). No ledger row: the
  Help menu item is row 1600-10's.
- M-52 changes the workflow and `release.yaml` (the help job and its
  artifact), so it dispatches CI at its merge.

## 11. Questions for the user

None that split the design.

## 12. Part D (departure)

Part D's simplification (`docs/reviews/d-simplification.md` S-6, P-9):

- The chapter list was in three places: `CHAPTERS` in `manifest.py`,
  `library/help/help.yaml`, and C1's check that the two agree. It is in
  `help.yaml` only: each chapter there also names the specs its shared
  part follows (`follows:`, AC-3, for C6). C1 checks the list itself
  (order, none missing, titles, parts); `test_ac1_ac3_the_repository_manifest_has_the_ten_chapters`
  holds the repository's list against AC-1's table and AC-3. A retitled
  chapter is no failure of the tool any more (`test_ac1_a_retitled_chapter_fails`
  went; `test_ac1_a_chapter_out_of_order_or_with_other_parts_fails` came).
  Calc-U-1600's asset is byte for byte the same.
- `version()` reads every ReleaseIt preset (D-9), not only `cmake`:
  `test_asset.py` `test_d9_version_of_the_xcode_preset`,
  `test_d9_version_of_the_cargo_and_regex_presets_and_an_unknown_one`.
  Calc-U-59's `release.yaml` (`preset: xcode`) reads 3.0.0.

## 13. Part E: the Calc-U-59 column

Status: Draft for the Part D/E boundary (2026-11-16)

Completes §4.3. Product paths are under `products/calc-u-59/`; site
paths are calc-u-site's at `acd629c`.

### 13.1 Scope

| Column | Step | Tests from (plan §3) | Done in (plan §5.1) |
|---|---|---|---|
| Calc-U-59 | M-60 | AC-9, AC-10, AC-16 (and AC-2, AC-14 for this product) | M-64 (the asset in the release) |
| Library | M-60 (its last part) | AC-8's second asset | M-60 (§10) |

- **The source.** calc-u-site is checked out at
  `/Users/me/Development/public/calc-u-site`; its `HEAD` is `acd629c`
  ("Merge branch '59/2.3.0'", checked 2026-11-16). M-60 reads it at that
  commit (`git show acd629c:59/…`) and changes nothing there: `59/Pages.jsx`
  (2241 lines, the pages' text), `59/routes.js:24-39` (the routes of
  AC-9), `59/assets/` (eight pictures), `59/tutorial-files/` (the six
  presets of the tutorial), `59/articles/`.
- **Also in M-60:** the user's ruling Q-3 (`docs/reviews/d-simplification.md`
  §10; LIB-16 decision log 2026-11-16): the shared chapter 2 keeps only
  what both products have (13.5).
- M-60 waits for M-52 (the tool, the shared parts) and M-54 to M-59 (the
  chapters describe what those steps build: presets of version 2,
  cards, the debugger, the family window, the Swift frontend). It
  touches no core and no app code.
- Not here: the site that renders the asset (LIB-19 59, M-65); the
  software pages, articles and the web calculator's page, which stay the
  site's (OQ-1).

### 13.2 Modules and files

| File | Holds |
|---|---|
| `products/calc-u-59/help/help.yaml` (new) | the product's manifest: `product: Calc-U-59`, `short: "59"`, `site: https://calc-u.org/59/`, `version-from: ../release.yaml` (the `xcode` preset, 3.0.0; §12), `names:` (13.6), `chapters:` (13.3), `downloads:` (13.4) |
| `products/calc-u-59/help/*.md` (new) | the product parts (13.3) |
| `products/calc-u-59/help/images/` | the pictures (13.7) |
| `products/calc-u-59/help/downloads/` | the six tutorial presets, version 2 (13.4) |
| `products/calc-u-59/help/settings-keys.txt` | written and checked by `lib16_ac14_settings_keys_file_matches_declaration` (C7) |
| `products/calc-u-59/tests/help_tests.cpp` (new) | the product's help tests, in `CalcU59Tests` (D-13) |
| `products/calc-u-59/CMakeLists.txt` | `CalcU59HelpCheck` (`tools/help check calc-u-59`) and `CalcU59HelpBuild` (`tools/help build calc-u-59 --out <build tree>/help`), both Frequent, beside Calc-U-1600's (`products/calc-u-1600/CMakeLists.txt:206-208`) |
| `.github/workflows/calc-u-59.yml` | a job `help` (ubuntu, Python 3) that builds the asset and uploads the artifact `Calc-U-59-help`, as Calc-U-1600's (`.github/workflows/calc-u-1600.yml:197-201`) |
| `products/calc-u-59/release.yaml` | the artifact `{ artifact: Calc-U-59-help, file: Calc-U-59-help.zip, as: Calc-U-59-help.zip }` (AC-8) |
| `library/help/02-getting-started.md`, `products/calc-u-1600/help/02-getting-started.md` | Q-3 (13.5) |
| `products/calc-u-59/reference/StateFileFormat.md` | moved into the help with `git mv` and rewritten as `10-preset-sections.md` (AC-9's last row; D-18) |

### 13.3 The product parts (AC-9, AC-10)

Converted once, by hand, from the JSX text of `59/Pages.jsx` to
Markdown (D-11); rewritten where Parts C to E changed what they
describe (the rows 59-1 to 59-47 of the ledger and the rows M-57 and
M-58 add are the list of such places), in the glossary's terms and with
the visible renames of `docs/glossary-mapping.md` ("Clean" → Cold boot,
"Open Preset…" → Load Preset…, "Startup Calculator" → Startup model,
"Card Folder" → Save folder, "state file" → preset).

| Today (calc-u-site `59/`) | Goes to |
|---|---|
| `/` `HomePage` (`Pages.jsx:65-173`); `/what-is-a-ti-59/` `AboutTi59Page` with `CardBankTable` (`:174-408`) | `01-introduction.md` (the product, the three models, what is emulated) |
| `/getting-started/` overview (`:410-430`); `/reference/` "The calculator" (`:1556-1574`) | `02-getting-started.md` (the faceplate, the keyboard in short, the window on each system, the More button on the iPhone and in the browser's portrait layout) |
| `/install/…`, four pages (`InstallMobilePage` to `InstallAndroidPage`, `:517-672`) | `02-installing.md` (product part of chapter 2; OQ-1) |
| `/getting-started/printer/` (`:431-498`): the magnetic cards (`:450-498`) | `04-magnetic-cards.md` (with C59-7's controls and the saved media on the Web) |
| `/getting-started/printer/` (`:431-449`): the printer; `/reference/` "The printer" (`:1575-1584`) | `05-printer.md` (the PC-100C, its PRINT, ADV and TRACE, the button on the bar) |
| `/presets/` `PresetsPage` (`:673-941`) | `06-presets.md` (the TI sections, examples, startup presets, labelling cue cards) |
| `/presets/tutorial/` `PresetTutorialPage` (`:942-1374`) and its six files | `06-preset-tutorial.md` and `downloads/` (13.4) |
| `/debugger/` `DebuggerPage` (`:1375-1547`); `/reference/` "The debug pane" (`:1585-1599`) | `07-debugger.md` (the CALCULATOR, CPU and LOG tabs, FREEZE, STEP, RESUME as the run-control buttons, traces, the ASM overlay) |
| `/reference/` "Settings" (`:1600-1655`) | `08-settings.md` (the product's keys, C7) |
| `/modules/` `ModulesPage` (`:1777-1856`) | `09-modules.md` |
| `/faq/` (`FAQS`, `:1656-1776`) | `09-faq.md` (OQ-1) |
| (new; AC-9's chapter 9) | `09-product-topics.md` (the chapter's opening page, a list of its files); `09-cue-cards.md`; `09-constant-memory.md` (the TI-58C's constant memory, C59-3); `09-fast-mode.md` (a user's summary of `59/articles/fast-mode.md`, which stays on the site and is linked); `09-models.md` (what differs by model: what Reset, Cold boot and a power cycle keep, the card reader on the TI-59 only, the partition (row 59-22); EC-3) |
| `KeyboardLegend` of the play page (`:2178-…`) | `10-keyboard-map.md` (the keyboard map of LIB-5 AC-21) |
| `reference/StateFileFormat.md` (product) | `10-preset-sections.md` (the TI sections of the preset format, version 2; LIB-6, C59-2) |
| (new; AC-10) | `10-reference.md` (the list of the formats with their versions, the settings file of this product, links to the shared reference); `10-keystroke-programs.md` (`.keystrokes.yaml`, `docs/formats/preset.md` §4.1.1); `10-media-files.md` (the magnetic card, `docs/formats/media.md`; the constant memory file `ti58c.memory.yaml`, C59-3; the Solid State ROM module file `.rom.yaml`, `docs/formats/modules.md` §2); `10-asm-overlay.md` (C59-4) |

The manifest's chapters:

```yaml
chapters:
  1: [01-introduction.md]
  2: [02-getting-started.md, 02-installing.md]
  4: [04-magnetic-cards.md]
  5: [05-printer.md]
  6: [06-presets.md, 06-preset-tutorial.md]
  7: [07-debugger.md]
  8: [08-settings.md]
  9: [09-product-topics.md, 09-models.md, 09-modules.md, 09-cue-cards.md, 09-constant-memory.md, 09-fast-mode.md, 09-faq.md]
  10: [10-reference.md, 10-preset-sections.md, 10-keystroke-programs.md, 10-media-files.md, 10-asm-overlay.md, 10-keyboard-map.md]
```

A link of today's pages to a site page that stays the site's (the
software pages, the articles, `/play/`) becomes a full `https://calc-u.org/59/…`
address (AC-15, EC-2).

### 13.4 The six tutorial presets

`59/tutorial-files/` at `acd629c`: `tutorial-1-setup.ti59`,
`tutorial-2-partition.ti59`, `tutorial-3-cuecard.ti59`,
`tutorial-4-program.ti59`, `tutorial-5-registers.ti59`, `tutorial.ti59`.
They are presets of 2.3.0 (`tutorial-1-setup.ti59:9-11`: `MODEL:`,
`SOLID-STATE-MODULE:`, `PRINTER:`), which version 2 refuses (LIB-6
AC-8; row 59-1). M-60 upgrades them by the rules M-54 upgrades the 57
examples with (`docs/formats/preset.md` §9.1; C59-2 AC-10, its decision
log), into `help/downloads/` under the same names with the new
extension the upgrade gives, and rewrites the tutorial's text, which
teaches the old keywords step by step, for version 2. Checked by
`lib16_ac9_tutorial_presets_load` (D-15).

### 13.5 The shared chapter 2 (Q-3)

`library/help/02-getting-started.md` keeps only what both products have:

| Today | After M-60 |
|---|---|
| "the menus File, Edit, Machine and Help" (`:7`), the diagram's "Menus: File · Edit · Machine · Help" (`:16`) | "the menus File, Machine and Help, and Edit where the product has it"; the diagram's box reads "Menus: File · Edit · Machine · Help, or the More button" |
| the table's row Edit (`:41`) and the bullet on Copy Screen and Paste Text (`:50-52`) | move to Calc-U-1600's part, into its section "Copy Screen and Paste Text" (`products/calc-u-1600/help/02-getting-started.md:91-…`) |
| "By typing text" (`:145`) and the section "Typing text" (`:161-170`) | move to the same section of Calc-U-1600's part; what it already says there is not said twice |
| (new, after "On macOS, Settings…, About and Quit…", `:62-63`) | a section "Without a menu bar": a window without a menu bar (a phone, a tablet, the browser's portrait layout) keeps Reset, Preset and Settings as buttons on the control bar and has a More button (⋯) with every other item: Examples, the items for media, Cold boot (which asks first), Resume, Help, and About where the system has one; an item that cannot be used now is greyed out there too (LIB-14 AC-29, M13 to M18) |

The shortcuts (`:65-77`) stay: both products' desktop frontends have
them. The links `#typing-text` (`:52`, `:145`) leave the shared part
with the text. No other shared file links there (checked with `grep`
on `7bdd51c`). `CalcU1600HelpCheck` and `CalcU59HelpCheck` both pass on
the result.

### 13.6 The names of AC-2

`names:` lists the product, its models and every peripheral of its
catalogue by the descriptor's name: `Calc-U-59`, `TI-59`, `TI-58`,
`TI-58C`, `PC-100C`, the 14 Solid State ROM modules by their names
(`core/TICatalogue.cpp:111-121`, `p.name = m.name`), and `TMC0501`.
`lib16_ac2_help_names_cover_the_catalogue` fails when a model or
peripheral of `tiCatalogue()` is missing from it (D-14).

### 13.7 The pictures (AC-16)

| Picture (`59/assets/`) | Used today | Decision |
|---|---|---|
| `app-icon.png` | `Pages.jsx:144` | moved as it is |
| `app-screenshot.png` | `:84`, `:1619` | taken anew by hand: it shows the window, its bar and no menus |
| `cuecard-single-label.png`, `cuecard-key-grid.png` | `:769`, `:916` | moved as they are (the cue card above the keys, which did not change) |
| `iphone-debug.png`, `ipad-13-2752x2064.png`, `iphone-duo-2852x2006.png`, `ipad-2752x2064-asm.png` | `:1507-1531` | moved as they are where they show no control bar; a picture that shows the bar is taken anew (seen when it is moved; D-17) |
| (new) | — | two pictures by hand: the macOS window with its menus, the iPhone's bar with the More menu open |

Pictures by hand are taken from the Swift app, which builds from the
main checkout only (handover, "Findings not raised yet"); the
`calc-u-59/screenshot_convention.md` memory names the devices.

### 13.8 Adapters

None: no old call is wrapped. `reference/StateFileFormat.md` is moved,
not kept beside the help (D-8).

### 13.9 Tasks

Tests before the content where a test can say it. `CalcU59Tests`
(`tests/help_tests.cpp`, Frequent); the CTest entries of 13.2.

| Task | What | ACs | Test |
|---|---|---|---|
| P59-1 | The product parts | AC-9, AC-3 | `CalcU59HelpCheck`; the qa-report's table of AC-9, row by row: where each page and section of 13.3 went |
| P59-2 | The tutorial presets | AC-9 | `lib16_ac9_tutorial_presets_load` (each of the six from `help/downloads/` is read and built on its model without a refusal; where the tutorial's text states what the calculator shows after a step, the test checks it, in the test, not in the file) |
| P59-3 | The reference chapter | AC-10 | `CalcU59HelpCheck`; the qa-report lists each format of AC-10 with its file and version |
| P59-4 | Pictures | AC-16 | `CalcU59HelpCheck` (every picture used, every use a file); the qa-report lists each picture as moved or taken anew |
| P59-5 | Settings keys | AC-14 | `lib16_ac14_settings_keys_file_matches_declaration` (`help/settings-keys.txt` against `cu59::settings::settingsDecl()`, `app/Settings.cpp:87`) |
| P59-6 | Product names | AC-2 | `lib16_ac2_help_names_cover_the_catalogue` |
| P59-7 | Both assets | AC-8 | `CalcU59HelpBuild` beside `CalcU1600HelpBuild`; the workflow's `help` job and `release.yaml`'s artifact; one CI dispatch at the merge |
| P59-8 | Q-3 | AC-1, AC-2, AC-3 | `CalcU1600HelpCheck` and `CalcU59HelpCheck` pass; the qa-report's content review of the shared parts against Calc-U-59 (each shared file: a sentence that does not hold for Calc-U-59 is moved to Calc-U-1600's part, as 13.5 does) |

Coverage: AC-9, AC-10, AC-16 (3 of 3), with AC-2, AC-8 and AC-14 for
this product.

### 13.10 Tools and agent knowledge (changed in the same step)

| File | Change |
|---|---|
| `products/calc-u-59/.claude/skills/calc-u-59-docs/SKILL.md` | the user guide is `help/` (product parts) and `library/help/` (shared parts), checked by `tools/help check calc-u-59`, no longer calc-u-site's `59/Pages.jsx` (`SKILL.md:3, 15, 41-45, 71`); step 5 of the Release Documentation Update Workflow (`:118`) updates the help here instead of the site's pages; the software pages and articles stay the site's (D-19) |
| `products/calc-u-59/CLAUDE.md` | one line, as Calc-U-1600's: user help is `help/` and `library/help/`, checked in Frequent; a user-visible change updates its chapter in the same step |
| `products/calc-u-59/README.md` | the user guide is at `https://calc-u.org/59/` and in `help/` |
| the files that link to `reference/StateFileFormat.md` | point at `help/10-preset-sections.md` (the skill `calc-u-59-statefiles`, `reference/*.md`; found with `grep` in M-60) |
| `docs/test-groups.md` | `CalcU59HelpCheck`, `CalcU59HelpBuild`, the new cases of `CalcU59Tests` |
| `tools/README.md` | the examples name `calc-u-59` beside `calc-u-1600` |

No memory file of the Calc-U-59 index names the site's guide (its
descriptions, checked 2026-11-16).

### 13.11 Golden runs and risks

- **Golden runs** are not touched: no core, no app code.
- **The amount of writing.** About 2000 lines of JSX text become
  Markdown, and most pages describe 2.3.0's bar, the long press for
  "Clean" and version-1 presets. The checks catch structure; the
  content review (P59-1, P59-8) is the check of AC-3 and AC-9.
- **The tutorial** teaches the format step by step; its version-2 text
  is new writing, checked by P59-2 and the review.
- **Pictures by hand** need the Swift app on macOS and a simulator, from
  the main checkout, not a `.claude/worktrees/` worktree.
- **CI.** The workflow gets a job and `release.yaml` an artifact: one CI
  dispatch at the merge (plan §1).

### 13.12 Decided (best guess)

- **D-11.** The source is calc-u-site at `acd629c`, found at
  `/Users/me/Development/public/calc-u-site` with that commit as its
  `HEAD`; it is read with `git show acd629c:…` and converted by hand
  once (ADR 0036), with no converter tool.
- **D-12.** The product parts are split by topic into several files per
  chapter (13.3), as Calc-U-1600's are; chapter 3 has no product part
  (it is shared only), and what differs by model is `09-models.md`
  (EC-3).
- **D-13.** The product's help tests run in `CalcU59Tests` (about 8 s of
  its 60 s budget, `docs/reviews/d-simplification.md` §11), not in a
  program of their own as Calc-U-1600's had to.
- **D-14 (AC-2).** `names:` holds the descriptors' names, not the
  modules' two-letter IDs, which are no words a shared part would use.
- **D-15 (AC-9).** The tutorial presets are upgraded by M-54's rules and
  checked by loading each and by the results the tutorial's text
  states; no reference run, since they are not among the 57 examples
  of M-9.
- **D-16 (Q-3).** The Edit menu and "Typing text" move into Calc-U-1600's
  existing section "Copy Screen and Paste Text"; the shared part gains
  the section "Without a menu bar".
- **D-17 (AC-16, OQ-4).** A picture is taken anew when it shows the
  control bar or the menus; the others move as they are. Two new
  pictures show the menus and the More menu.
- **D-18.** `reference/StateFileFormat.md` moves into the help (history
  kept), as Calc-U-1600's user documents did (D-8); the developer
  references point at it.
- **D-19.** From M-60 the user guide's source is the help in this
  repository; calc-u-site's `59/` pages stay as they are at `acd629c`
  until LIB-19 59 (M-65) renders the asset.
- **D-20 (OQ-1).** The four install pages are one file of chapter 2's
  product part, `02-installing.md`.

### 13.13 Ledger rows and plan notes

- **Ledger.** None: the help changes no behaviour. The Help item's
  address is row 59-14's.
- **Plan notes (proposals; the plan is not edited).** M-60 "Does" gains
  the ruling Q-3 (13.5); it changes the workflow and `release.yaml`, so
  it dispatches CI at its merge; "Knowledge, ledger" gains the files of
  13.10. §5.1's LIB-16 row reads "1–8, 14–15: M-52 / M-60" (§10, already
  proposed).

### 13.14 Questions for the user

None that split the design.
