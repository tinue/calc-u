# Release prep: the simplification pass before the first releases

Status: done (2026-11-18). §1–§5 were written and committed (`15140dd`)
before any code edit; §6 onwards report the result.

Range: `cac66d1` (the Part E simplification report) to `64bcd67` (the end of
the release prep). The `simplified` tag did not exist, so the range starts at
`cac66d1`; at the end the tag is set to the last commit of this pass. The
range holds the six release-prep steps (`docs/reviews/release-prep-plan.md`),
the pytest driver, the faceplate art and the F-4/F-5/F-8 fixes: 31 commits,
480 files, +8 698 −4 497.

Rules (user, 2026-11-18): breaking changes allowed (0.9.0 reads no old
formats; no compatibility shims); nothing of `Briefing.md` §3 or of the
answers in `docs/reviews/e-simplification.md` §10 is reopened; details by
spec → today's behaviour → simpler structure. Golden-run references and
`tests/reference/*` never change.

`59/` is `products/calc-u-59/`, `1600/` is `products/calc-u-1600/`, `~MEM/`
the project memory folder.

The survey: four read-only passes (leftovers and duplication; one
vocabulary check of every public name against `docs/GLOSSARY.md`; the
documents against the code; the surfaces that freeze, judged by me).

## 1. Duplication and leftovers

| # | What | Where | Why | Breaking | Gate |
|---|---|---|---|---|---|
| L-1 | `CALCU_WEB_TEST_AIDS` changes nothing in Calc-U-59 (no source has the `#ifdef`; `c59.web.key-button-*` were deleted), yet 59 carries the `--release` handling in `build.sh`, `tests/web_no_test_aids.sh` (a grep for deleted names that cannot fail), CTest `CalcU59WebNoTestAids`, CI steps; the two check scripts are 90 % alike | `59/frontends/web/build.sh:15-26,47-51`, `59/tests/web_no_test_aids.sh`, `59/CMakeLists.txt:214`, `.github/workflows/calc-u-59.yml`, `ci/linux/jobs.sh:65` | a mechanism with no effect | no | local CI web-59, GitHub CI |
| L-2 | `c59.trace` is a gate that is always open (`entry(kTrace, true, "")`) since release prep 5; every frontend still reads it | `59/app/TIDebugPart.cpp:617`, Qt `LogWidget.cpp:75`, Swift `TIAppSession.mm:899`, `DebugView.swift:163`, Web `Debugger.jsx:289`, tests | a concept with no meaning left | no | Frequent, Qt, Web 59, Swift |
| L-3 | The PC-100C's controls looked up three times by hand-written place and control strings | Qt `EmulatorController.cpp:176-196`, Swift `TIAppSession.mm:787-794`, Web `Paper.jsx:18`; `peripherals::controlsOf` has a second, non-const overload | constants exist (`kTIPrinterPlace`, `PC100C::kTrace`); a `ControlState::controlsAt(place)` beside `place()` serves all | no | Frequent, Qt, Swift, Web 59 |
| L-4 | Calc-U-59 alone keeps its places of the start (`placesAtStart`): its save and screenshot folders do not follow a change in Settings, against LIB-14 AC-11 ("a change is applied … at once"); the last per-product rule of the places | `LibraryParts.hpp:66-72`, `LibraryParts.cpp:35-46`, `TIAppPart.cpp:85` | one rule, a setup field and a second `Paths` gone | visible (59: a new save folder is used at once): ledger 59-66 | Frequent, Qt, Swift |
| L-5 | Swift's two trace callbacks arrive two ways: `HostDirs::chosenTraceFolder` (places) and `TIAppSetup::traceAvailable` → `LibraryPartsSetup::traceAvailable` → `traceFolderOf(paths, available)` | `HostDirs.hpp`, `LibraryParts.hpp:66`, `TIAppPart.hpp:148`, `TraceSession.cpp:37`, `TIAppSession.mm:440-475` | both are the host's places; one home (`HostDirs`), `Paths::ensure(Traces)` answers | no | Frequent, Swift |
| L-6 | Five media writers write the header by hand (comment, `format`, `format-version`, name, `saved`); the floppy writes `saved:` always, the others when given | `TIMagneticCardKind.cpp:229`, `ConstantMemoryFile.cpp:89`, `SharpMediaKinds.cpp:252`, `FloppyImageFile.hpp:109`, `BatteryCardInstance.hpp:256-312` | one `media::headerText(…)`; one `saved` rule | bytes of saved files may change, still read | Frequent (media tests), Swift `CardTests` |
| L-7 | Cassette and module identifiers still named for tapes and cards where they name the file: `kTapeKind`, `kTapeFileSuffix`, `TapeDefinition`, `TapeCodec`; `kCardSuffix` in `SharpRoms.cpp` repeats `kCardFileSuffix`; CMake `CARD_YAML_FILES`/`TAPE_FILES`; Web test id `tape-new` | `1600/app/SharpMediaKinds.*`, `1600/core/Machine/SharpRoms.cpp:16-21`, `MemoryCardCatalog.hpp:16`, `1600/frontends/qt6/CMakeLists.txt:179-184`, `CalcU1600.jsx:89` | half-done rename (the Kept list keeps the deck mechanism, not the file) | no | Frequent, Qt, Web 1600 |
| L-8 | "Card YAML" (INSPECTOR button, request `c1600.card-yaml`, `CardYamlDump`, "Module card dump:") and the Web Settings note "modules, disks and tapes" missed by the renames | Qt `DebugPanel.cpp:82`, Web `Panels.jsx:74`, `SharpWebPart.cpp:106`, `SharpShellProduct.hpp:46`; help `07-debugger.md:680`, `09-module-definitions.md:34,49`, `10-module-definition-format.md:288` | the glossary avoids "card" for a module | visible labels: ledger 1600-54 | Qt, Web 1600, help |
| L-9 | Two `calcu` finders in Python (`CALCU` then `build/all`; `CALCU59_CALCU` then `build/tests`/`build/tools`, configuring a tree) | `library/harness/python/calcu_machine.py:37-42`, `59/tools/calcu59.py:30-48` | one finder in `calcu_serve.py` | developer env name | Frequent `calcu_serve_python`, 59 tools |
| L-10 | The "the WAV holds N files, which one?" choice written twice per frontend (before the range) | `1600/frontends/qt6/app/MainWindow.cpp:395-405,629-643`, `CalcU1600.jsx:202-204,250-256` | one function each | no | Qt, Web 1600 |
| L-11 | `layout` checked only by Calc-U-1600 (Calc-U-59 takes any unknown value as portrait) | `CalcU1600.jsx:156-161`, `CalcU59.jsx:217-228` | `embedProps` checks against the product's layouts | 59: a console warning | `embed_props.test.js`, Web |
| L-12 | Stale comments and design text: "tape folder(s)", `chosenTraceFolder` as a string, `traceDirectory`/`traceMaxBytes`, the `lcd` command | `LibraryParts.*`, `1600/app/AppPaths.hpp:12`, `SharpAppPart.hpp:117`, LIB-9 design:145, C59-5 design:346, `harness/Commands.hpp:57`, `display_text_tests.cpp` | — | no | build |

## 2. Vocabulary (public names against the glossary)

| # | Name | Problem | Decision |
|---|---|---|---|
| V-1 | place `cassette-recorder` vs the word `cassette` in `eject:`, `saveas: {place:}` and the media commands (`MediaPart.cpp:341`: the kind's eject word where one place holds the kind) | the glossary's Place entry says presets name `cassette-recorder`: wrong | by design (LIB-7 AC-21): glossary entry **Place word**, Place and preset.md §5.2.1/§7 corrected |
| V-2 | "Card YAML", `c1600.card-yaml` | avoids "card" | L-8: "Module YAML", `c1600.module-yaml` |
| V-3 | "Mount Directory…", entry `c1600.mount-directory` beside the command `c1600.mount-host-drive` | the glossary's Host drive avoids "mounted directory" | label "Host Drive…", entry `c1600.host-drive` (ledger 1600-54) |
| V-4 | DAP `c1600/screen` answers what `display-text` answers with other keys (`rows/status/unparsed/poweredOn`) | two names, two shapes | `c1600/display-text` with the harness's keys (`DisplayText.cpp`) (ledger 1600-55) |
| V-5 | harness `switch <peripheral> <control> on\|off` is the session's `set-control` | two names for one action | harness `set-control` (ledgers 59-67, 1600-55) |
| V-6 | harness `key <name>` text: "key names" | the glossary avoids "key name" | text "key ID"; no rename |
| V-7 | preset step `syncclock:` | breaks the kebab-case rule of `docs/formats/README.md` §1 | `sync-clock:` (ledger 1600-55; the startup preset updated) |
| V-8 | floppy: kind `floppy-disk`, word `floppy`, `disk-name` | — | kept (files already in users' hands since 0.8; the glossary says `floppy` is the place word, V-1) |
| V-9 | settings labels "Examples folder" (1600, `[folders] presets`), "Basic folder" | label vs key, "BASIC" | "Presets folder", "BASIC folder" (ledger 1600-54); 59's "Segment Style" for `led-style` kept (the 2.3.0 label) |
| V-10 | missing entries: Battery-backed module, Cassette recording, Data register, Machine state, Project preset, Calc indicator, Control, Profile, Place word | users meet them | glossary entries + decision-log row |
| V-11 | `c59.live-debug` beside `c59.show-cpu-debug` | one pattern for "show this tab" | `c59.show-calculator-debug` (internal) |
| V-12 | `c1600.ce1600p-rom` beside `c1600.ce1600p-next-variant`; request `c1600.plan-code` beside payload `c1600.code-plan` | word order, the glossary's Variant | `c1600.ce1600p-variant`, `c1600.code-plan` (internal) |
| V-13 | Web `model` (Calc-U-1600 takes an index or a name; LIB-15 AC-16 and FE-7: the name) and `preset` (Calc-U-1600 doubles a given `examples:` prefix; Calc-U-59 takes both) | spec vs code | name only; both prefixes taken (ledger 1600-55) |
| V-14 | glossary errors: ROM lookup ("the user's ROM folder", none exists), Debug panel vs View (INSPECTOR), FREEZE (59's 2.3.0 label) vs Pause | — | entries corrected, the exceptions named |

Rejected: `[window] printer-width` → `paper-width`, 59 prop `settings` →
`settingsMenu`, VS Code `c1600.port` → `dapPort`, `disk-name`, `.rom.yaml`,
`c59.debug-view`, `c1600.tape-deck`, `state-save`/`state-restore` (§9).

## 3. The surfaces that freeze at the release

| Surface | Judgement | Why |
|---|---|---|
| control-state `controls` (`{place, peripheral, id, form, on}`) | **keep** | generic (any peripheral's `ControlInfo`), already read by three frontends; only the lookups around it are simplified (L-3) |
| `CALCU_WEB_TEST_AIDS` split | **change** | keep the option and the check for Calc-U-1600; Calc-U-59's copy goes (L-1) |
| `HostDirs` trace callback | **keep**, with L-5 | the sandbox's grant is the frontend's, asked per trace (Q-2 answer); the iCloud gate moves beside it |
| keep-file `kind` | **keep** | one optional argument ("trace"); the Web page needs it to list traces; documented in the host services contract |
| module definition's optional `format:` | **keep** | the CE-502B file comes from PC-1500-ROM unchanged and its MD5 is checked at every ROM lookup (`SharpRoms.cpp:34`); written always; the help's "add `format:`" sentence corrected (D-2) |
| `saved:` / leading comment | **keep the rule, change the writing** | the rule holds on every medium (docs/formats/media.md matches); the five writers become one helper (L-6) |
| shared Qt drop helper | **keep** | both MainWindows are one-liners now; `bringToFront` stays (tested) |
| `CalcU59` / `CalcU1600` props | **change** | `model` by name only, `preset` prefix both ways (V-13), `layout` checked for both (L-11) |

## 4. Documents against the code

| # | Mismatch | Fix | Gate |
|---|---|---|---|
| D-1 | the cassette pictures of help ch. 9 show "Tape name:"/"Tape:" | finding: retaken with the shots at M-64 (pictures need the screen; not in this pass) | — |
| D-2 | 1600 help `10-media-files.md:28` "module definitions of 0.8 are read as they are"; `10-module-definition-format.md:13` "add `format:`" | "are not read; rename them"; "`format:` may be added" | help check |
| D-3 | `docs/contracts/settings.md` names `Tapes/`, `<product>` in lower case, ST-4 without "zero or less: 50" | corrected (+ decision-log row) | — |
| D-4 | the outdated-file notice names "Floppy disks", "Cassettes" (no such headings) and Calc-U-59 the path `docs/formats/media.md §1` | `the User Guide, "Media files", "<heading>"` for all; a test pins them (ledgers 59-67, 1600-54) | Frequent |
| D-5 | `contracts/capabilities.md` step `tape:`, Q-2 `.card.yaml`, Controls row without `controls` | corrected | — |
| D-6 | `contracts/media.md` place "cassette", `Tapes/` | corrected | — |
| D-7 | 59 help omits `saved` (constant memory) and the leading comment (magnetic card) | added | help check |
| D-8 | `contracts/harness.md` fit-table rows: `ti59`, `pc1600 dump-basic`, `tape record`, "a preset is a recorded harness session" | corrected (+ decision-log row) | — |
| D-9 | 59 help `08-settings.md:56` "three kinds" of folder | "two" | help check |
| D-10 | `formats/README.md`: `&`/`0x`/`$` hex everywhere, fixed values in any case | "media byte values: `0x` or decimal"; "`format:` values match exactly" | — |
| D-11 | `modules.md` §1.2 `template` "never written into a saved instance"; §1.8 comments kept at every save (only the first) | corrected, help mirror too | help check |
| D-12 | cassette and floppy writers accept a name with `"` (unreadable YAML); the docs forbid it | refused as the magnetic card does (ledger 1600-54) | Frequent |
| D-13 | `library/help/08-settings.md:57` "every product has these keys" | "the product's part lists which it has" | help check |
| D-14 | shipped help links into `frontends/qt6/resources/cards` | "the bundled templates"; a help check refuses source paths and `M-nn` | helptool tests |
| D-15 | `help.md` "tapes"; `frontend.md` fit table without `controls`, `c59.calc-indicator`; `peripherals.md` row on TRACE superseded | corrected | — |
| D-16 | memory notes `calc-u-1600/project_cassette_tape.md:22`, `project_pc1600_ramdisk_geometry_table.md:31` | current names | — |

## 5. Plan: groups, one short branch each

Ranked by payoff. Ledger rows reserved: 59-66 (L-4), 59-67 (V-5, D-4),
1600-54 (L-8, V-3, V-9, D-4, D-12), 1600-55 (V-4, V-5, V-7, V-13).

| Group | Branch | Items | Gate |
|---|---|---|---|
| R-1 | `rs1-59-places` | L-2, L-3, L-4, L-5, V-11 | Frequent, Qt, local CI + web-59, Swift |
| R-2 | `rs2-1600-words` | L-7, L-8, L-10, V-3, V-9, V-12, D-4, D-12 | Frequent, Qt, local CI + web-1600, help |
| R-3 | `rs3-tooling` | L-1, L-9 | Frequent, local CI + web-59 + web-1600 |
| R-4 | `rs4-public-words` | V-4, V-5, V-6, V-7, V-13, L-11 | Frequent, DAP smoke, local CI + web jobs, Swift (UITests use `switch`?) |
| R-5 | `rs5-media-header` | L-6 | Frequent, Swift `CardTests` |
| R-6 | `rs6-docs` | V-1, V-10, V-14, D-2, D-3, D-5…D-11, D-13…D-16, L-12 | help check, helptool tests |

R-1, R-2, R-3 run in parallel (worktree agents); then R-4 and R-5; R-6 last
so the glossary and contracts name the final state.

## Questions

None: no answer splits the design. Every decision above follows the spec,
today's behaviour or the simpler structure; the breaking ones are allowed
by the user's rule and get ledger rows.

## 6. Changes made

Each group: one implementing agent in a worktree, build and Frequent on
the branch; merged `--no-ff` by me; on `main` Frequent, then the group's
gate. "All" is `git diff --shortstat` of the merge; "code" leaves out
Markdown and pictures.

| Group | Merge | Changes | All | Code |
|---|---|---|---|---|
| R-3 | `8d5f00c` | L-1: Calc-U-59's `--release`, `web_no_test_aids.sh`, CTest `CalcU59WebNoTestAids`, its CI steps gone (its Web zip is the tested `dist/`); Calc-U-1600 keeps option and check. L-9: one `find_calcu()` in `calcu_serve.py` (`$CALCU`, then `build/tests`, `build/all`, `build/tools`) for `calcu_machine` and `calcu59.py`; `CALCU59_CALCU` → `CALCU` | +69 −102 | +48 −82 |
| R-1 | `915cf3c` | L-2: `c59.trace` entry and its four readers gone. L-3: `ControlState::controlsAt`, the frontends on the constants, the non-const `controlsOf` gone. L-4: `placesAtStart` gone, Calc-U-59 reads its places when used (`TIFileAccess` takes the save folder per use); test `lib14_ac11_save_folder_changed_after_the_start_is_used_at_once`; ledger 59-66. L-5: `HostDirs::iCloudAvailable` beside `chosenTraceFolder`; `traceAvailable` (two setups) and `traceFolderOf`'s parameter gone, also `Settings::traceLocation()`. V-11: `c59.show-calculator-debug` | +308 −198 | +209 −166 |
| R-2 | `ad48991` | L-7: `kCassetteKind`, `kCassetteFileSuffix`, `CassetteDefinition`, `CassetteCodec`; one `kModuleFileSuffix`; CMake `MODULE_FILES`/`CASSETTE_FILES`; `cassette-new`. L-8: "Module YAML", `c1600.module-yaml`, `ModuleYamlDump`, "Module dump:", the Web note "cassettes"; help ch. 9 says "module". L-10: one `pickWavFile` per frontend. V-3: "Host Drive…", `c1600.host-drive`. V-9: "Presets folder", "BASIC folder". V-12: `c1600.ce1600p-variant`, `c1600.code-plan`. D-4: the outdated notice names the User Guide's headings (pinned by tests). D-12: `"` in a cassette or disk name refused. Ledger 1600-54, 59-67 | +336 −283 | +243 −199 |
| R-5 | `1f9c4ec` | L-6: `library/media/Header` (`headerText`, `quotedScalar`) writes the header of the magnetic card, constant memory, cassette and floppy; one `saved` rule (when a time is given). Bytes of every written file unchanged | +101 −29 | +101 −29 |
| R-4 | `6bd4c6f` | V-5: harness `set-control` (was `switch`). V-6: "key ID" texts. V-7: `sync-clock:`. V-4: DAP `c1600/display-text` on the harness's serialiser (`displayTextNow`). V-13: Calc-U-1600 `model` by name only; one `bundlePathOf` in `embed.js`, `preset` with or without `examples:`. L-11: `embedProps(props, {name, layouts, layout})` warns for both products. Ledger 1600-55, 59-67 | +313 −202 | +227 −135 |
| R-6 | `38aef7f` | Glossary: nine entries (Place word, Battery-backed module, Cassette recording, Data register, Machine state, Project preset, Calc indicator, Control, Profile), Place, ROM lookup, Debug panel, Pause, Kind corrected. Formats README, preset.md, modules.md; contracts settings, capabilities, media, harness, help, frontend (keep-file `kind`), peripherals; help D-2, D-7, D-9, D-13, D-14; help check C10 (source paths and `M-<n>` refused; LIB-16 AC-18); the last two "key name" texts; L-12 comments | +250 −69 | +66 −6 |

Total since the end of the release prep (`64bcd67`):
+1 490 −875 (all), +828 −605 in `library/` and `products/` without
Markdown and pictures.

## 7. Candidates rejected

| Candidate | Reason |
|---|---|
| A library script for both no-test-aids checks | Only Calc-U-1600 has test aids after L-1 |
| `BatteryCardInstance` on the media header helper | It splices a template's text in place; a fresh header reorders lines (a byte change of saved module files) |
| One helper for the readers' accepted header keys | No clear saving |
| Merging `calcu59.Run` into `calcu_machine.Session` | Different jobs (plain-text runs with preset text and paper helpers vs a queued, refusal-checking session) |
| Deleting the tests that assert old names are refused | Cheap guards against a rename coming back |
| A Qt drop event filter in place of the three one-line overrides | No lines saved |
| `format:` required in module definitions | The CE-502B file is MD5-checked at every ROM lookup; changing it means a second checksum |
| `[window] printer-width` → `paper-width`, 59 prop `settings` → `settingsMenu`, VS Code `c1600.port` → `dapPort`, `disk-name` → `floppy-name`, `.rom.yaml`, `state-save`/`state-restore`, `c59.debug-view`, `c1600.tape-deck`, `led-style` label | Settings files and media already in users' hands, conventions of their host (VS Code), or internal names with an exact meaning; no rule of the spec asks for them |
| The magnetic card's `format:` read in any case | C59-7 AC-3 and its test require it; documented as the exception |
| `tapeFiles`/`tapeKinds` in `FileKindReply` | Internal field naming the files in a WAV; low value |

## 8. Findings

- F-1: `1600/tests/reference/v0.8.1-examples/startup_default-pc1600.pc1600.log:51`
  logs `syncclock:`; a re-run of that reference set by hand would log
  `sync-clock:` (V-7). Not changed (reference); no test compares it.
- F-2: the local CI web jobs fail under heavy load (load average up to 71
  while agents built): web-59 15 of 160 once, web-59 and web-1600 one test
  each once; all green on the re-run with the machine quiet.
- F-3: the help no longer says where a user gets a bundled module template
  file (the link into `frontends/` went, D-14); Saved Media… exports only the
  user's own media. For the user: ship the templates in the examples zip
  (M-61) or name them in the help?
- F-4: `c1600/display-text` has no `cursor {row, col}` (the harness's reply
  has none; the cell under the cursor is read as its character).
- F-5: the cassette pictures of help ch. 9 still show "Tape name:"/"Tape:"
  (D-1): retake at M-64.
- F-6: LIB-16 AC-18 (help check C10) is a new criterion from D-14, not a
  user ruling: for the user's review.
- F-7: a Calc-U-1600 Web `?model=` with an unknown name starts with the last
  model used, not the default (seen by R-4's test; unchanged behaviour).

## 9. Checks at the end

- Frequent on `38aef7f`: 51 of 51.
- Local Linux CI after every merge: every job green (base, help, web-59,
  web-1600), the web jobs after a re-run where F-2 hit.
- Swift (main checkout, `bin/simulators reset` by CTest): macOS build,
  `CalcU59SwiftUnitTests`, `CalcU59UIRegression` after R-1+R-2;
  `CalcU59SwiftUnitTests` again after R-4+R-5: all pass.
- Qt: `calcu_qt6_tests`, `calcu_qt6_scenarios`, `CalcU59QtTests`,
  `CalcU1600QtTests` pass.
- `CalcU1600DapSmoke`, `CalcU1600DapSmokeQt`, `CalcU1600MachineDriver` pass.
- GitHub CI on `38aef7f`: Calc-U-1600 44253179658, Calc-U-59 44253183060,
  both green.
- Longer (`build/groups`, both `dist/` built first, `caffeinate -d`): 33 of
  36; the three failures are `CalcU1600Shots.01/02/05` (Screen Recording and
  Automation permissions of the launching app; run them from iTerm2).
- Golden replays equal; no golden-run reference or `tests/reference/*` file
  changed (`git diff cac66d1 -- '*/golden-runs/*.golden' '*/tests/reference/*'`
  is empty).

## 10. Line counts

Same method as the C1, D and E reports (`.cpp .hpp .h .swift .js .jsx .mm
.py CMakeLists.txt .cmake`, tracked, without `node_modules`, `dist`,
`build`, `vendor`).

| | Range start (`cac66d1`) | Release prep end (`64bcd67`) | Now (`38aef7f`) |
|---|---|---|---|
| `library/` | 66 975 | 67 494 | 67 710 |
| `library/` without tests | 36 656 | 36 935 | 37 016 |
| Calc-U-1600 `app/` (without tests) | 9 811 | 9 837 | 9 851 |
| Calc-U-1600 `app/tests` | 2 614 | 2 632 | 2 633 |
| Calc-U-1600 `core/` | 30 203 | 30 223 | 30 215 |
| Calc-U-1600 `tests/` | 30 227 | 30 338 | 30 378 |
| Calc-U-1600 Qt6 frontend | 6 391 | 6 478 | 6 478 |
| Calc-U-1600 Web frontend | 2 538 | 2 594 | 2 591 |
| Calc-U-59 `app/` | 10 709 | 10 659 | 10 642 |
| Calc-U-59 `core/` | 6 524 | 6 524 | 6 524 |
| Calc-U-59 `tests/` | 14 703 | 14 854 | 14 873 |
| Calc-U-59 Qt6 frontend | 4 336 | 4 384 | 4 384 |
| Calc-U-59 Swift frontend | 9 899 | 9 947 | 9 940 |
| Calc-U-59 Web frontend | 3 202 | 3 246 | 3 261 |

This pass is about names and rules more than size: three concepts went
(the always-open trace gate, the places of the start, Calc-U-59's
test-aids build), two inputs found one home (the Swift trace callbacks, the
media header), and every public name now has one spelling in the code, the
documents and the glossary. The library grew by 81 lines outside its tests
(the header helper, `controlsAt`, `bundlePathOf`, the layout check); the
products' app layers and frontends are within a few dozen lines of before.
