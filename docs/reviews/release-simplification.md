# Release prep: the simplification pass before the first releases

Status: survey (2026-11-18). §1–§5 were written before any code edit; §6
onwards report the result.

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
