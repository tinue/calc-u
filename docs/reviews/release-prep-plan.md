# Release prep: plan (2026-11-17)

The six release-prep steps of the Part E review (`docs/reviews/e-simplification.md`
§10, the user's answers of 2026-11-17). The plan was written from a survey of the
tree at `17a66c0` and was not put up for approval (user: "write it, don't ask me
to approve it"). Details are decided by spec → today's behaviour → simpler
structure. Each decision this plan takes beyond the user's answers is marked
**(decided)**.

Paths are relative to the repository root. `59/` means `products/calc-u-59/`, `1600/`
means `products/calc-u-1600/`, `~MEM/` means the Calc-U project memory folder
(`~/.claude/projects/-Users-me-Development-public-Calc-U/memory/`). The counts in the
file lists are matching lines.

**Never changed:** `1600/tests/reference/v0.8.1-examples/`, every golden-run reference
(`products/*/tests/golden-runs/`; the `  lcd |…|` label in the `.golden` files is
written by `golden_runs_tests.cpp:213` and stays), the 2.3.0 reference texts
(`59/tests/reference/v2.3.0-examples/`), `printer-timing.ti59`. Also history, not
edited: `docs/reviews/*` other than this file, `features/*/qa-report.md` rows already
written, past `CHANGELOG.md` entries, `1600/docs/background/plans/*`, the version 1 → 2
migration tables of `docs/formats/preset.md` (the old names they quote are the old
format's), `HANDOVER.md` history bullets.

---

## 1. Step 1: product words `c59`, `c1600`

### 1.1 Old → new

| # | Old | New | Notes |
|---|---|---|---|
| 1a | harness/`calcu` command word `ti59 <sub>` | `c59 <sub>` | 14 subs: program, keycodes, registers→1b, coverage, partition, run-time, asm, line-times, step-cost, hot-steps, dump, step-trace, heatmap, log-level. Registered `59/app/TIHarnessCommands.cpp` (`r.addProduct("ti59", …)`) |
| 1b | `ti59 registers` (data registers) | `c59 data-registers` | the session command behind it `c59.registers` → `c59.data-registers` **(decided**: one name for one thing). The common CPU `registers` stays |
| 1c | command word `pc1600 <sub>` | `c1600 <sub>` | 10 subs: serial, tape→2, dump-basic, inspect, call, load-code, host-drive, file-kind, load-basic, type-basic. `1600/app/SharpHarnessCommands.cpp:560-573` |
| 1d | `--product 59` / `--product 1600` (`shortName()`) | `--product c59` / `--product c1600` | `findProduct` (`library/harness/Run.cpp:345`) also takes the full name (`Calc-U-59`, `Calc-U-1600`), kept. `59`/`1600` are no longer accepted **(decided**: no old forms before the first release). `calcu_add_preset_check(… PRODUCT 59|1600)` → `PRODUCT c59|c1600`; Python `Calcu(prog,"59")`, `Session(workdir,"1600")` → `"c59"`, `"c1600"` |
| 1e | DAP requests `calcu1600/load`, `/reset`, `/screen`, `/quit` | `c1600/load`, `/reset`, `/screen`, `/quit` | `1600/app/dap/DapSession.cpp:160-201`, `DapSetup.cpp:125,141`; the rule sentence in C1600-3 spec (`prefix calcu1600/`) changes with a decision-log row |
| 1f | VS Code debug type `calcu1600` | `c1600` | `package.json` `type`, `onDebugResolve:`, `onDebugDynamicConfigurations:`, `debugType == '…'`; `extension.js` registrations; `scaffold.js:69` (generated launch.json); `tools/dap_smoke.py` `adapterID`; `tests/dap/dap_tests.cpp:158` |
| 1g | VS Code command IDs `calcu1600.buildAndLoad`, `.resetAndStop`, `.allResetAndStop`, `.createProject`, `.createProjectHere`; settings `calcu1600.port`, `.zasmPath`, `.sdccBinPath`, `.romListings`, `.romSymbols`; problem matchers `calcu1600-sdas`, `calcu1600-zasm`, owner `calcu1600` | `c1600.…`, `c1600-sdas`, `c1600-zasm`, owner `c1600` | **(decided)**: "one word per product everywhere" covers what a user types or sees in `settings.json`/`tasks.json`. |
| 1h | 59 debug-panel topics `calculator-debug`, `program-listing`, `cpu-debug`, `heatmap`, `asm-overlay`, `debug-view` | `c59.calculator-debug`, `c59.program-listing`, `c59.cpu-debug`, `c59.heatmap`, `c59.asm-overlay`, `c59.debug-view` | `59/app/TIDebugPart.hpp:57-62` and the `PayloadKind kKind` strings at 101-140. Native frontends use the constants; Web `Debugger.jsx` uses strings |
| 1i | command `c59.cpu-debug` (`TIDebugPart.hpp:68`, "the CPU tab is shown") | `c59.show-cpu-debug` | **(decided)**: the topic of 1h takes the name; a command and a topic of one name would read as one thing. Precedent `c59.show-debug-panel` |
| 1j | common harness command `lcd` | `display-text` | `library/harness/FirstCommands.cpp:217`; reply line word follows |
| 1k | request `c1600.lcd-text` | `c1600.display-text` | `1600/app/SharpWebPart.cpp:120-124`; Web test helper `screen()` |
| 1l | glossary: no entry for the product word | new entry **Product word**: `c59`, `c1600` (future `c1403`): the harness product command word, the `--product` value, the DAP request prefix, the VS Code debug type; the dotted prefix `c59.`/`c1600.` of product topics, commands and requests | `docs/GLOSSARY.md` + decision-log row; "Mnemonic/Listing/Assembly" (l.732) cites `ti59 keycodes` → `c59 keycodes` |

**Kept (not the product word):** the extensions `.ti59`, `.pc1600` and extension
lists (`ti59 ti58 ti58c`); file and tool names (`ti59.py`, `pc1600_cli`,
`ti59-research`, `pc1600-*.golden`, template folders `pc1600-program`); machine and
model names (`kPC1600`, `PC1600KeyLayout`); the VS Code `currentFile: 'pc1600'` value;
the faceplate JSON flag `{"pc1600": …}` (`SharpShellProduct.cpp:134`); icon ids; the
Web `localStorage` prefix `calcu1600/` and the bundle id `ch.erzberger.calcu1600.*`;
pty names `calcu1600.serial`; CMake targets `calcu1600*`; the VS Code extension's
folder, package name and publisher `calcu1600-debug`/`calcu1600` **(decided**: its
identity; renaming it makes a new extension, the `.vsix` name with it); the `lcd`
faceplate/component ids (`KeyRect{"lcd"}`, `ComponentId{"lcd"}`, `data-calcu="lcd"`,
`calcu-lcd`, Qt objectName), `lcd::`/`Lcd*` identifiers, the old CLI options
`--lcd-text`/`--lcd-png` in history docs; the Python `Machine.lcd` property of
`1600/tools/machine/` (the user's test API, ruled to pass unchanged; its
`Session` call underneath sends `display-text`); `asm-overlay` as the file kind
(`TIFileKinds.cpp:51`) and `heatmap` as the `c59 heatmap` sub; `Briefing.md:347`.

How to tell the product word apart in a grep: a word boundary not after `.`, `_`, `/`
or `-`, followed by a space and one of the subcommands, or quoted as the first
argument of `addProduct`/`find({…})`, or backticked in prose about commands.

### 1.2 Files (from the survey)

**1a/1b `ti59`**
- code: `59/app/TIHarnessCommands.cpp`, `TIPresetCommands.cpp`, `TIHarnessCommands.hpp`, `HarnessProduct.hpp`, `Keycodes.hpp`, `TIPresetKeys.hpp`, `TIPresetCommands.hpp`; `59/core/TITypingSupport.hpp`; `59/tools/calcu59.py`, `fastmode_matrix.py`, `coverage.py`, `calendar/compare.py`, `calendar/years.py`; `library/harness/Protocol.hpp:16`
- tests: `59/tests/harness_product_commands_tests.cpp`, `harness_tests.cpp`, `preset_content_tests.cpp`, `machine_state_tests.cpp`, `ti_presets_tests.cpp`, `card_flows_tests.cpp`, `media_tests.cpp`, `help_tests.cpp`, `check_scripts.py`; `59/frontends/swift/Calc-U-59UITests/FamilyStructureTests.swift`
- presets+examples: `59/examples/assembly/README.md`, `59/examples/calendar/CALENDAR_PROGRAMS.md`
- docs+help: features C59-9 (spec, design), C59-2 (spec, design), C59-4 (spec, design), LIB-11 (spec, design), LIB-6 design, INDEX.md; `docs/contracts/harness.md`, `docs/test-groups.md`, `docs/glossary-mapping.md`, `docs/GLOSSARY.md`, `docs/migration-plan.md`; `59/reference/Harness.md`, `DebugAPI.md`, `CoreArchitecture.md`, `59/README.md`
- skills+agent: `59/CLAUDE.md`, skills `calc-u-59-core`, `calc-u-59-presets`, `calc-u-59-swift`, `calc-u-59-app`; ~MEM `calc-u-59/run_time_measurement.md`, `calc-u-59-memories.md`, `calc-u-59/headless.md`, `calc-u-59/headless_printer_lines.md`

**1c `pc1600`**
- code: `1600/app/SharpHarnessCommands.cpp/.hpp`, `HarnessProduct.hpp`, `BasicDump.hpp`, `SerialScripts.hpp`; `1600/core/Connector/PrintableBytes.hpp`, `core/Debug/Inspect/Inspector.hpp`, `core/PC1500/PC1500BasicLoader.hpp`; `1600/tools/machine/machine.py`; `library/harness/HarnessHost.hpp`, `Protocol.hpp`; `1600/CMakeLists.txt:213`
- tests: `1600/tests/harness/product_commands_tests.cpp` (also test names `…under_pc1600…`), `tests/harness_tests.cpp`, `machine_state_tests.cpp`, `harness/harness_entry_tests.cpp`, `app/tests/loader_part_tests.cpp`, `tests/support/PC1600BasicTyper.cpp`, `tools/machine/test_driver.py`
- presets+examples: `1600/examples/interfaces/ce158_demo.pc1500a`
- docs+help: C1600-5 (spec, design), C1600-2 design, LIB-11 (spec, design), INDEX.md; `docs/contracts/harness.md`, `docs/migration-plan.md`, `docs/glossary-mapping.md`, `docs/test-groups.md`; `1600/docs/developer/PC1600-Serial-Port.md`, `App-Layer.md`, `1600/docs/background/Decisions.md`; `1600/headless/README.md`, `1600/TODO.md`, `1600/tools/machine/README.md`; help `1600/help/07-debugger.md`, `09-com-ports.md`, `09-host-drive.md`
- skills+agent: `1600/CLAUDE.md`; ~MEM `calc-u-1600/project_fast_basic_binary_loader`, `feedback_headless_dir_for_probe_output`, `project_cassette_tape`, `project_loader_matrix_pc1500`, `project_ce158_interface`, `project_pc1600_native_basic_and_preset_program`, `project_pc1600_serial_port`

**1d `--product`**
- code: `.run/calcu-59.run.xml`, `.run/calcu-1600.run.xml`; `library/harness/Protocol.hpp`, `ServeAddOn.hpp`, `cli/Cli.cpp:14,20-24`, `python/calcu_serve.py`, `python/calcu_machine.py`; `59/app/HarnessProduct.cpp:52-53`, `1600/app/HarnessProduct.cpp:159-160`; `59/CMakeLists.txt`, `1600/CMakeLists.txt`, `library/presets/RunPresetCheck.cmake:28`; `59/tools/calcu59.py`, `check_no_old_tools.cmake`, `coverage.py`, `fastmode_matrix.py`, `read_trace.py`; `1600/tools/dap_smoke.py`, `1600/tools/machine/machine.py:30`
- tests: `59/tests/harness_rom_tests.py`, `calcu_runs.cmake`, `harness_tests.cpp`, `reference_runs_tests.cpp`, `rom_missing_build.sh`, `TIPresetRig.hpp`; `1600/tests/dap/dap_port_check.sh`, `harness/HarnessRun.hpp`, `harness/product_commands_tests.cpp`, `machine_state_tests.cpp`, `read_trace_check.sh`; `library/harness/tests/cli/cli_cases.cmake`
- presets+examples: `59/examples/fastmode-sst.ti59`, `1600/dev/presets/trace_demo.pc1500a`, `1600/examples/interfaces/ce158_demo.pc1500a`
- docs+help: LIB-11 design, LIB-6 design, C1600-3 design, C59-2 design, C59-9 design, LIB-10 design, C1600-4 design; `docs/test-groups.md`, `docs/behaviour-changes.md` (as source text only; rows not rewritten), `docs/glossary-mapping.md`; `1600/docs/developer/Building.md`, `Debugger-Internals.md`, `Cloud-Sessions.md`, `LCD-Text.md`, `1600/docs/background/README.md`; `1600/help/07-debugger.md`, `09-com-ports.md`, `1600/headless/README.md`; `59/reference/Harness.md`, `59/rom/TI58C-ROM-Guide.md`, `TI59-ROM-Guide.md`, `59/README.md`, `59/TODO.md`
- skills+agent: `59/CLAUDE.md`, `1600/CLAUDE.md`, skills `calc-u-59-core`, `calc-u-59-presets`; `59/memory/sbr444-quirk-analysis.md`; ~MEM: 17 files with `--product` (grep)

**1e–1g DAP and VS Code**
- code: `1600/app/dap/DapSession.cpp/.hpp`, `DapSetup.cpp`, `DapConfig.hpp`, `DapPart.cpp`, `DapTransport.hpp`; `1600/tools/dap_smoke.py`; `1600/vscode/calcu1600-debug/package.json`, `extension.js`, `scaffold.js`; `1600/tools/build_hostdrive_rom.sh:7`
- tests: `1600/tests/dap/dap_tests.cpp`
- docs+help: C1600-3 (spec, design); `docs/inventory/calc-u-1600.md`, `divergence-matrix.md` (inventory: history, left); `1600/docs/developer/Debugger-Internals.md`, `LCD-Text.md`, `README.md`; `1600/vscode/calcu1600-debug/README.md`; `1600/help/07-debugger.md` (type at 448, settings/commands 8×)
- skills+agent: `1600/CLAUDE.md:29`; ~MEM `calc-u-1600/project_dap_debugger_plan`, `calc-u-1600-memories.md`, `feedback_gui_test_launch_quit`, `feedback_lcd_text_before_images`

**1h/1i 59 topics**
- code: `59/app/TIDebugPart.hpp/.cpp`, `TIAppPart.hpp`; `59/frontends/qt6/app/EmulatorController.cpp`; `59/frontends/swift/Bridge/TIAppSession.mm`; `59/frontends/web/src/Debugger.jsx`
- tests: `59/tests/debug_panel_tests.cpp`, `session_tests.cpp`, `harness_tests.cpp` (`state asm-overlay|debug-view|calculator-debug|cpu-debug`), `debugger_tests.cpp`, `part_flows_tests.cpp`, `SessionRig.hpp`, `web_part_tests.cpp`
- docs+help: `59/reference/AppArchitecture.md`, `AppLayer.md`, `DebugAPI.md`, `FrontendParity.md`, `NewGUIGuide.md`, `WebClient.md`; C59-4 (spec, design); LIB-10 design; `docs/glossary-mapping.md`
- skills+agent: skills `calc-u-59-swift`, `calc-u-59-web`; `59/CLAUDE.md:100`

**1j/1k `lcd`, `c1600.lcd-text`**
- code: `library/harness/FirstCommands.cpp`, `Protocol.hpp`, `DisplayText.hpp`, `python/calcu_machine.py`, `python/calcu_serve.py`; `59/app/HarnessProduct.hpp`, `TIPresetRules.hpp`, `59/tools/coverage.py`, `fastmode_matrix.py`; `1600/app/HarnessProduct.hpp`, `LcdTextPart.hpp`, `SharpDisplayLines.hpp`, `SharpWebPart.cpp/.hpp`, `core/Display/DisplayOutputText.hpp`, `tools/machine/machine.py`
- tests: `library/harness/tests/protocol_tests`, `run_tests`, `display_text_tests`, `startup_commands_tests`, `cli/cli_cases.cmake`, `attach/tests/attach_cli_tests`, `python/tests/test_calcu_serve.py`; `59/tests/harness_tests`, `calcu_runs.cmake`, `harness_product_commands_tests`, `machine_state_tests`, `preset_content_tests`, `ti_presets_tests`, `harness_rom_tests.py`; `59/frontends/qt6/tests/window_tests`; Swift UITests `AttachTests`, `CardScenarioTests`, `FamilyStructureTests`, `PresetLoadTests`; `1600/tests/harness/harness_entry_tests`, `preset_content_tests`, `sharp_presets_tests`, `harness_tests`, `harness/product_commands_tests`, `machine_state_tests`, `example_references_tests`, `media_kinds_tests`, `typing_tests`; `1600/frontends/web/tests/helpers.js`, `play.spec.js`, `session.spec.js`
- docs+help: LIB-11 (spec, design), C59-9 design, C59-5 design, LIB-18 design, LIB-6 design, C1600-3 design, C1600-4 (spec, design), C59-2 spec; `docs/test-groups.md`, `docs/contracts/harness.md`, `docs/adr/0026-…` (a note, not reworded); `1600/docs/developer/LCD-Text.md`, `README.md`, `screenshots/README.md`, `Web-Client.md:96`, `1600/tools/machine/README.md`; `59/reference/Harness.md`
- skills+agent: `1600/CLAUDE.md`, `59/CLAUDE.md`, skills `calc-u-59-uitesting`, `calc-u-59-presets`; ~MEM `calc-u-1600/feedback_lcd_text_before_images`, `calc-u-1600-memories.md`

**Outside the repository (step 1, user: yes)**
- `~/.claude/skills/PC1500/SKILL.md:299-306`: `calcu --product 1600` → `--product c1600`, `then lcd` → `then display-text`, `pc1600 load-code|dump-basic|inspect|call` → `c1600 …` (`registers` stays).
- `~/Development/sharp/Sharp1500-1600-Ref/skills/sharp-pc1500-1600/SKILL.md:128`: `headless/pc1500_cli`, `headless/pc1600_cli` are gone → name `calcu --product c1600` (Calc-U monorepo, `products/calc-u-1600`).
- `~/Development/public/calc-u-site/.claude/skills/site-59/SKILL.md:104`: `reference/StateFileFormat.md` is gone → `docs/formats/preset.md` (Calc-U monorepo).
- Memory notes of calc-u-site and SharpDataExchange: the survey found none of the old names in them (`calcu1600.serial` in SharpDataExchangeRust is a pty name, kept). The review's "two memory notes" are re-checked at the step; any stale path to the old repositories is corrected.
- `~/Development/sharp/calcu-info` (the user's driver, the old CLI): not edited; its `MIGRATION.md` gets no change (the repo's `1600/tools/machine/` is the driver now).

### 1.3 Ledger, specs, formats, help
- Ledger: 59-58 (`calcu` command word `c59`, `--product c59`, `c59 data-registers`, `display-text`, prefixed debug topics — developer-visible), 1600-50 (`c1600` word, `--product c1600`, DAP `c1600/…`, VS Code type/commands/settings `c1600`, `display-text`). Users of `launch.json` re-create it: the row says so.
- Decision-log rows: LIB-11 (product word, `display-text`, `--product`), C59-9 (word, `data-registers`), C1600-5 (word), C1600-3 (DAP prefix, debug type), C59-4 (topic names, `c59.show-cpu-debug`). No new ACs: the existing ACs are edited in place (names only).
- `docs/contracts/harness.md`: names only (H-1/H-2 contents are step 6).
- Help: `1600/help/07-debugger.md`, `09-com-ports.md`, `09-host-drive.md`; `59/help` hits of `ti59 …` if any (help_tests).
- Glossary: entry "Product word" + decision-log row; `docs/glossary-mapping.md` rows.

### 1.4 Test gate
Frequent; local CI base + web-1600 (the Web helper and request change) + web-59
(`Debugger.jsx` topics); Qt tests (59 `EmulatorController`); Swift mac build +
UnitTests + UIRegressionTests (`TIAppSession.mm` constants, UITests strings);
`CalcU1600DapTests` (Frequent) and the Longer `CalcU1600DapSmoke`/`DapSmokeQt`;
`CalcU1600MachineDriver` (Longer, the Python driver).

---

## 2. Step 2: files and media

### 2.1 Old → new

| # | Old | New | Where |
|---|---|---|---|
| 2a | module definition suffix `.card.yaml` | `.module.yaml` | `kCardFileSuffix` `1600/core/Connector/MemoryCardCatalog.hpp:16`, `kCardSuffix` `1600/core/Machine/SharpRoms.cpp:16`; Qt GLOB `1600/frontends/qt6/CMakeLists.txt:179,189`; `1600/frontends/web/build.sh:45`; `.github/workflows/calc-u-1600.yml:585`; `tools/machine/machine.py:41,50`, `cardimage.py` |
| 2b | module definition: `format-version: 1` only; a `format:` key is refused | `format: memory-module` + `format-version: 1`; the writers and every shipped file carry it; the reader accepts it and refuses another value; a file without `format:` is still read **(decided**: the CE-502B definition is fetched unchanged from tinue/PC-1500-ROM and checked by MD5) | `MemoryCardDefinition.hpp:1354` (`requireOnlyKeys`), `docs/formats/modules.md` §1.2 |
| 2c | `.card.yaml` read | not read (no exception); ledger row 1600-1's "Module definitions stay readable" is amended | — |
| 2d | user-visible "card definition" texts ("card definition must be a mapping", "card definition needs …", `requireFormatVersion(…,"card",…)`, `describedAt = "the Memory Card Definition Format"`) | "module definition …", "the module definition format" | `MemoryCardDefinition.hpp:1349-1366`, `SharpMediaKinds.cpp:283`. The C++ class names stay (user: core-internal) |
| 2e | CE-502B `roms/CE-502B.card.yaml` (fetched) | stored as `CE-502B.module.yaml` by `roms/fetch_roms.sh` (content and MD5 unchanged); lookup `SharpRoms.cpp:34` | `fetch_roms.sh:105`; `1600/roms/README.md`. Existing local ROM folders: renamed by hand in the main checkout |
| 2f | shipped `.card.yaml` (14): `1600/frontends/qt6/resources/cards/{ce151,ce155,ce1600m,ce1601m,ce161,ce1638,ce163f,superram}`, `1600/examples/dwx/CE-1601M - Progs`, `1600/examples/memory/memory-cards/pc1500-maxed-out`, `1600/tools/machine/cards/{ram-1m,ram-256k,ram-512k}` | `….module.yaml`, each with `format: memory-module` | `git mv`; references in presets `examples/dwx/DiskWorks.pc1600`, `make_diskworks_media.pc1600`, `examples/memory/memory-cards/pc1500-maxed-out.pc1500` |
| 2g | cassette `format: cassette-tape` | `format: cassette` | `SharpMediaKinds.cpp:183-184` (reader + error), `:252` (writer) |
| 2h | cassette suffix `.tape.yaml` | `.cassette.yaml` | `kTapeFileSuffix` `SharpMediaKinds.hpp:23`; Qt GLOB `CMakeLists.txt:183-184`; template `1600/frontends/qt6/resources/cards/empty.tape.yaml` → `empty.cassette.yaml` |
| 2i | name key `tape-name` | `cassette-name` | `SharpMediaKinds.cpp:188,191,193,253`; `docs/formats/README.md` §2 |
| 2j | preset insert step `tape:` and eject/`saveas place:` word `tape` (`PresetWords{"tape",…}`) | `cassette` | `SharpMediaKinds.cpp:340`; the session place word the frontends send `"tape"`: `SharpAppPart.cpp:229,296`, `SharpHarnessCommands.cpp:368,370`, Qt `MediaControls.cpp:139,182,185`, Web `CalcU1600.jsx:81,93,323,330,335` (`name-and-save-tape` → `name-and-save-cassette`, "Tape name:" → "Cassette name:") |
| 2k | kind label "cassette tape", error texts "not a tape definition (expected 'format: cassette-tape')", "The tape definition format", "Cassette tapes" | "cassette", "not a cassette definition (expected 'format: cassette')", "the cassette definition format", "Cassettes" | `SharpMediaKinds.cpp:184,330-338`; kind id is already `cassette` (`kTapeKind`) |
| 2l | harness recording kind id `cassette-tape` (label "cassette recording", `.wav`) | `cassette-recording` **(decided**: must differ from the kind `cassette`) | `SharpHarnessCommands.cpp:311` |
| 2m | media place `cassette` (`MediaPlaceId{"cassette"}`) | `cassette-recorder` | `1600/core/Machine/SharpMediaPlaces.hpp:89`; `SharpCatalogue.cpp:145,168` (only the first field; the second is the kind `cassette`); `PC1500InterfaceMachine.cpp:149`; `PC1600InterfaceMachine.cpp:251`; `SharpHarnessCommands.cpp:322`; Qt `MainWindow.cpp:121` (`registerMediaControls`); Web test id `bar-media-cassette` → `bar-media-cassette-recorder` |
| 2n | setting `[storage] tape-folder`, default `Tapes`, `Folder::Tapes` | `[storage] cassette-folder`, default `Cassettes`, `Folder::Cassettes` | `library/settings/Declaration.cpp:123-124` (description "The folder for cassettes; empty: Cassettes in the save folder."), `Declaration.hpp`, `Paths.cpp:37-39`, `Paths.hpp:15,26`; `library/media/Kind.hpp:98`; `library/shell/SettingsModel.cpp:50-51`, `SettingsPart.cpp:83`; `library/web/components/shellModel.js:371`; `1600/app/AppPaths.*`, `Settings.cpp`, `SharpMediaKinds.cpp:334`; Qt `AppPaths.hpp` |
| 2o | harness `pc1600 tape record <file.wav>` / `pc1600 tape status`, reply lines `tape: playing/recording/empty`, value key `tape` | `c1600 cassette record` / `c1600 cassette status` (with step 1's word), reply `cassette: …`, key `cassette` | `SharpHarnessCommands.cpp:337-347,567-568` |
| 2p | "tape definition" in docs and help | "cassette definition" | see files |
| 2q | glossary "Cassette tape" (`.tape.yaml`, "tape definition"), "Place" (no cassette example), "Media place", l.648 "the tape folder" | entry **Cassette**: the medium, "stored as a cassette definition (`.cassette.yaml`) that names its sound file (`.wav`)"; _Avoid_: tape image, WAV, cassette tape (as the name of the file); "Place" and "Media place" name the `cassette-recorder`; "the cassette folder" | `docs/GLOSSARY.md` + decision-log row (user, 2026-11-17, Q-6) |
| 2r | media writers' hex prefixes | already `0x` for integers (magnetic card `partition`, `checksum`); the `$` of addressed-hex rows is the encoding, kept; nothing writes `&`. Only a sentence in `docs/formats/README.md`: "writers use `0x`" | survey §C |
| 2s | `saved:` + leading comment on every medium | cassette: add `saved:` (reader accepts it); memory module instance: `saved:` key written (reader accepts it) in place of the `# last-saved:` comment, leading comment kept (`# Calc-U-1600 memory module instance …`); magnetic card: add a leading comment `# Calc-U-59 magnetic card`; constant memory `ti58c.memory.yaml`: add the comment and `saved:`; floppy: already both | `SharpMediaKinds.cpp:252-256`, `BatteryCardInstance.hpp:256-312`, `MemoryCardDefinition.hpp:1354`, `59/app/TIMagneticCardKind.cpp:227-258`, `59/app/ConstantMemoryFile.cpp:88-104`; `docs/formats/media.md`, `modules.md` |

**Kept:** the C++ class names (`CardBase`, `ExpansionCard`, `SoftwareDefinedCard`,
`BatteryCardInstance`, `MemoryCardDefinition`, `CardCodec`, `kCardFormatVersion`);
the core cassette mechanism names (`TapeDeck`, `TapeDeckAccess`, `TapeContent`,
`TapeControl`, lamps `tape`/`tapeName`, `data-lamp="tape"`, `calcu-tape-counter`,
fixtures `tests/fixtures/tape/`): a cassette holds a tape; the module kind id
`battery-backed-module` (not in the answer); the bundled-template folder names
`resources/cards/`, `tools/machine/cards/`, `examples/memory/memory-cards/`
**(decided**: folders, not formats; renaming them moves many paths for no user
gain); Calc-U-59's `.magnetcard.yaml`, `format: ti59-magnetic-card`, `card-name`
(already one spelling); every "tape" meaning the PC-100C's paper (`paper tape`,
`printer tape`, `tape width`, `number of tapes`).

### 2.2 Files (from the survey)

**`.card.yaml` / module definition**
- code: `.github/workflows/calc-u-1600.yml`; `1600/app/AppPaths.hpp`, `SharpMediaKinds.cpp/.hpp`; `1600/core/Connector/BatteryCardInstance.hpp`, `ExpansionCard.hpp`, `ExpansionConnector.hpp`, `MemoryCardCatalog.hpp`, `MemoryCardDefinition.hpp`, `NamedFileCatalog.hpp`, `SoftwareDefinedCard.hpp`; `1600/core/Machine/SharpCatalogue.hpp`, `SharpRoms.cpp`, `core/PC1600/PC1600Memory.hpp`, `core/SharpRomReader.hpp`; `1600/frontends/qt6/app/AppPaths.cpp/.hpp`, `qt6/CMakeLists.txt`, `web/build.sh`; `1600/roms/fetch_roms.sh`; `1600/tools/machine/cardimage.py`, `machine.py`
- tests: `library/presets/tests/fixtures/bad/by-file.test-seg`, `library/presets/tests/header_tests.cpp`; `1600/app/tests/app_session_tests.cpp`, `media_session_tests.cpp`, `opening_tests.cpp`, `settings_tests.cpp`; `1600/frontends/web/tests/play.spec.js`, `rom-missing.spec.js`, `session.spec.js`; `1600/tests/battery_card_instance_tests`, `ce151_tests`, `ce155_tests`, `ce161_tests`, `ce1638_tests`, `ce163f_tests`, `ce502b_tests`, `descriptor_tests`, `example_references_tests`, `inspector_tests`, `media_kinds_tests`, `memory_card_tests`, `pc1600_basicloader_tests`, `pc1600_machine_tests`, `pc1600_slot_module_tests`, `rom_lookup_tests`, `rom_missing_build.sh`, `TestCards.hpp`
- presets+examples: the 14 files of 2f; `1600/examples/dwx/DiskWorks.pc1600`, `make_diskworks_media.pc1600`, `examples/memory/memory-cards/pc1500-maxed-out.pc1500`, `1600/examples/README.md`, `examples/setup/README.md`
- docs+help: `docs/behaviour-changes.md` (1600-1 amended; 1600-45 text stays: history of that row), `docs/contracts/capabilities.md`, `contracts/media.md`, `docs/formats/modules.md`, `formats/preset.md` (not the migration tables), `formats/README.md`, `docs/glossary-mapping.md` (row ".card.yaml: open" → decided), `docs/walkthroughs/first-steps.md`; features LIB-1 (spec, design), LIB-7 (spec, design), LIB-12 (spec, design), LIB-13 (spec, design), LIB-16 design, C1600-1 design, C1600-4 design; `1600/CHANGELOG.md` (an Unreleased entry only), `1600/README.md`, `1600/roms/README.md`, `1600/THIRD-PARTY-NOTICES.md`, `1600/TODO.md`; help `1600/help/04-media`, `06-presets`, `07-debugger`, `09-module-definitions`, `10-media-files`, `10-module-definition-format`, `10-preset-sections`, `10-reference`; `1600/docs/developer/App-Layer`, `Building`, `PC1600-Core-Limitations`, `README`; `1600/docs/background/CE-161-Program-Module`, `PC1500-Card-Disk-Idea` (background: left)
- skills+agent: `1600/CLAUDE.md`, `prompts/phase-6.md` (history, left), ~MEM `calc-u-1600/project_pre_1_0_format_decisions.md`, `project_pc1600_ramdisk_geometry_table.md`

**Cassette (`.tape.yaml`, `cassette-tape`, `tape-name`, `tape:` step, place, folder, `pc1600 tape`, "tape definition")**
- code: `1600/app/SharpMediaKinds.cpp/.hpp`, `SharpAppPart.cpp`, `SharpHarnessCommands.cpp/.hpp`, `AppPaths.*`, `Settings.cpp`; `1600/core/Machine/SharpMediaPlaces.hpp/.cpp`, `SharpCatalogue.cpp`, `PC1500InterfaceMachine.cpp`, `PC1600InterfaceMachine.cpp`; `1600/frontends/qt6/CMakeLists.txt`, `qt6/app/MediaControls.cpp`, `MainWindow.cpp`, `AppPaths.hpp`; `1600/frontends/web/src/CalcU1600.jsx`; `library/settings/Declaration.cpp/.hpp`, `Paths.cpp/.hpp`; `library/shell/SettingsModel.cpp`, `SettingsPart.cpp`; `library/media/Kind.hpp`; `library/web/components/shellModel.js`, `MediaPicker.jsx` (comment)
- tests: `library/settings/tests/paths_tests.cpp`, `TestProduct.hpp`; `library/web/tests/shell_model.test.js`; `library/media/tests/autosave_tests`, `saved_media_tests`; `library/shell/tests/menu_model_tests`; `1600/app/tests/app_session_tests`, `opening_tests`, `SessionFixture.hpp`, `settings_tests`; `1600/frontends/web/tests/play.spec.js`, `shell.spec.js`; `1600/tests/harness/harness_entry_tests`, `harness/product_commands_tests`, `machine_state_tests`, `media_kinds_tests`, `preset_content_tests`, `sharp_presets_tests`, `session_tests`, `tests/help/help_tests`; `59/tests/help_tests.cpp` (the shared settings key)
- presets+examples: the template `empty.tape.yaml`; no shipped preset uses `tape:`
- docs+help: `docs/GLOSSARY.md`, `docs/formats/media.md` §3, `formats/preset.md` (step table l.366-367, feature table l.615), `formats/README.md`, `docs/contracts/media.md`, `contracts/settings.md`, `contracts/harness.md`, `docs/test-groups.md`; features LIB-7 (spec, design), LIB-9 (spec, design), LIB-11 (spec, design), LIB-13 (spec, design), LIB-14 design, LIB-16 (spec, design), C1600-4 spec, C1600-5 (spec, design); help `library/help/08-settings.md`, `1600/help/04-media`, `06-presets`, `09-basic-programs`, `10-media-files`, `10-preset-sections`, `10-reference`, `settings-keys.txt`; `1600/docs/developer/App-Layer.md`, `1600/headless/README.md`
- skills+agent: `1600/CLAUDE.md`; ~MEM `calc-u-1600-memories.md`, `calc-u-1600/project_cassette_tape.md`, `feedback_headless_dir_for_probe_output.md`

**`saved`/comment (2s):** `1600/app/SharpMediaKinds.cpp`, `1600/core/Connector/BatteryCardInstance.hpp`, `MemoryCardDefinition.hpp`; `59/app/TIMagneticCardKind.cpp`, `59/app/ConstantMemoryFile.cpp`; their tests (`media_kinds_tests`, `battery_card_instance_tests`, `memory_card_tests`, 59 `magnetic_card_tests`, `card_flows_tests`, constant-memory tests); `docs/formats/media.md`, `modules.md`; help `10-media-files.md` of both products.

### 2.3 Ledger, specs, formats, help
- Ledger: 1600-1 amended ("Module definitions are `.module.yaml` with `format: memory-module`; `.card.yaml` is not read"), new 1600-51 (module definitions renamed and shipped files updated; the cassette names: `.cassette.yaml`, `format: cassette`, `cassette-name`, step/eject `cassette`, place `cassette-recorder`, `[storage] cassette-folder` default `Cassettes`, menus and dialogs say "cassette"; `saved:` on cassettes and modules), new 59-59 (magnetic card and constant memory files get the leading comment, the constant memory `saved:`).
- Decision-log rows: LIB-7 (cassette names, `saved` + comment on every medium), LIB-9 (`cassette-folder`), LIB-12 (`.module.yaml`, `format: memory-module`), LIB-13 (suffixes), C1600-5 (`cassette record|status`), LIB-11 if it names the step. ACs that name `.card.yaml`/`.tape.yaml` are edited in place.
- `docs/formats/modules.md` (suffix, `format:` key), `media.md` §3 (cassette), `media.md` (comment and `saved` rule for every medium), `preset.md` (step `cassette`), `README.md` (suffix list, `0x` writer sentence).
- Help: the chapters listed above; `tools/help check` both products.

### 2.4 Test gate
Frequent; local CI base + web-1600 (+ help-1600, help-59); Qt tests of both products
(MediaControls, resources GLOB); `CalcU1600MachineDriver` (Longer, cards renamed);
Swift mac build + UnitTests + UIRegressionTests (the 59 magnetic card writer is used
by the Swift app; `CardTests`).

---

## 3. Step 3: Calc-U-59 topics

### 3.1 Old → new

| # | Old | New | Notes |
|---|---|---|---|
| 3a | topic `c59.printer` `{connected, trace}` (`59/app/TIAppPart.hpp:77,105-111`, `TIAppPart.cpp:146,237-240`, value at `:40`) | dropped. `connected` is already in `control-state` (places, `panels` paper owner). `trace` goes into `control-state` as a new library field **`controls`**: for each plugged-in peripheral with controls, `{place, peripheral, id, form, on}` from its `ControlInfo` (`library/peripherals/Controls.hpp`) | **(decided)**: the survey found no control positions in `control-state` today and `capabilities` is a request; a library field (not a product entry) because `ControlInfo` is the library's and the paper panel (`library/qt6/PaperPanel`) already reads it. The TRACE position kept while the PC-100C is detached (C59-6 AC-6) stays in `TIAppPart`; nothing shows it while detached |
| 3b | topic `c59.led` (`TILedState{LedFrame led; calcIndicatorOpacity}`) | `c59.calc-indicator`, same payload | **(decided)**: `display` carries the raw machine frame; the LED frame is `FrameSampler`'s persistence model (held glyph, smoothed levels) and the opacity is smoothed, so `display` cannot carry them without changing what every `expect:`/screenshot reads. The frame stays in the topic's native payload; its value form stays `{calc-indicator}` |
| 3c | `c59.cue-card` | unchanged | — |

Subscribers to change: Swift `59/frontends/swift/Bridge/TIAppSession.mm:244,246,516,730-739,787`,
`TIAppSession.h:259`, `App/EmulatorViewModel.swift:121,388,664`,
`Views/PrinterView.swift:157` (keep the `TIChangePrinter` bit, raised by paper too);
Qt `59/frontends/qt6/app/EmulatorController.cpp:30,32,114,119,186,197-199`,
`.hpp:56-58`, `MainWindow.cpp:129-140`, `KeyboardWidget.cpp:78`; Web
`59/frontends/web/src/Paper.jsx:9,20` (reads `controls` from `control-state`);
`59/app/TIWebPart.cpp:62,186-197`, `.hpp:22`.

### 3.2 Files
- code: `library/control/ControlState.hpp/.cpp` (+ the part that fills it), `59/app/TIAppPart.hpp/.cpp`, `TIAppPartFlows.cpp`, `TIWebPart.cpp/.hpp`, the frontends above
- tests: `59/tests/session_tests.cpp`, `part_flows_tests.cpp`, `web_part_tests.cpp`, `session_thread_tests.cpp`, `harness_tests.cpp`; `59/frontends/qt6/tests/qt_frontend_tests.cpp`, `window_tests.cpp`; library control-state tests (new `controls` field); `59/frontends/web/tests/play-calculator.spec.js:546-569` (TRACE through the UI)
- docs: `59/reference/AppLayer.md:115`, `WebClient.md:80-82`, `NewGUIGuide.md:63`, `FrontendParity.md:68`; LIB-2 or LIB-4 spec/design where `control-state` is defined (decision-log row + a new AC for `controls`, next free number); C59-6 decision-log row
- skills: `59/.claude/skills/calc-u-59-web/SKILL.md:64`, `calc-u-59-qt/SKILL.md:71`, `calc-u-59-app/SKILL.md:38-39`

### 3.3 Ledger, specs, gate
- Ledger: none for users (topics are developer-facing; the harness `state c59.printer` disappears → one sentence added to 59-58 of step 1, which lists the developer-visible names).
- Gate: Frequent; local CI base + web-59; Qt tests; Swift mac build + UnitTests + UIRegressionTests.

---

## 4. Step 4: Web embed

### 4.1 Old → new

| Old | New |
|---|---|
| global/component `PlayCalculator` (`59/frontends/web/src/PlayCalculator.jsx:215,365`; `build.js:35,37`) | `CalcU59` (file `src/CalcU59.jsx`) |
| `PlayCalculator.portraitWidth(h)` | `CalcU59.portraitWidth(h)` |
| `CalcU1600` | unchanged; static `version` kept |
| shared props (`library/web/src/embed.js:21` `embedProps`: `width, height, keyboard, model, preset, workerUrl`) | in the order `width`, `keyboard`, `layout`, `preset`, then `height`, `model`, `workerUrl`, then product extras. `layout` moves into `embedProps` **(decided)**: `"portrait"`, `"wide"`, `"auto"` for Calc-U-59; Calc-U-1600 has one layout and takes only `"auto"` (its default; any other value is ignored with a console warning) |
| 59 extra `settings` | kept as Calc-U-59's extra |
| 59 `module` prop (2.3.0, ignored, LIB-15 AC-19) | still ignored |
| test-only commands: `c59.web.key-button-down/-up` (`59/app/TIWebPart.cpp:86-90`, used only by `59/tests/web_part_tests.cpp:224-226`) | deleted |
| `c1600.upload` (`1600/app/SharpWebPart.cpp:76-85`), `c1600.display-text` (step 1) | registered only when the CMake option **`CALCU_WEB_TEST_AIDS`** is ON (default ON, as `CALCU_ATTACH` for test trees); the products' `frontends/web/build.sh --release` builds with it OFF; the release packaging (both workflows' Web zip, `release.yaml`) builds with `--release`; a check (CTest `CalcU59WebNoTestAids` / `CalcU1600WebNoTestAids`, like `CalcU59NoTestAids`) greps a release `dist/` for the names **(decided)**: the CI tests the test dist and packages a release dist |

Calc-U-1600's `model` keeps its index form (no answer asked for a change).
`[data-panel=paper]` is already the hook in both; `[data-panel=printer]` in Calc-U-1600 is
the CE-158 printer panel, not stale. calc-u-site follows in M-62 (not touched).

### 4.2 Files
- code: `59/frontends/web/src/PlayCalculator.jsx` → `CalcU59.jsx`, `build.js`, `index.html:41,45`, `calc.css:1,4`, `src/page.jsx` if it names the component; `library/web/src/embed.js`, `library/web/app/app.jsx:120`; `1600/frontends/web/src/CalcU1600.jsx`, `index.html:35`, `build.sh`; `59/frontends/web/build.sh`; `59/app/TIWebPart.cpp`, `1600/app/SharpWebPart.cpp/.hpp`; `59/CMakeLists.txt`, `1600/CMakeLists.txt`, `library/CMakeLists.txt` (option); `.github/workflows/calc-u-59.yml`, `calc-u-1600.yml`; `products/*/release.yaml`
- tests: `59/frontends/web/tests/prerender.spec.js`, `session.spec.js`, `shell.spec.js`, `play-calculator.spec.js`; `1600/frontends/web/tests/prerender.spec.js`, `session.spec.js`, `play.spec.js`; `59/tests/web_part_tests.cpp`; the two new no-test-aids checks
- docs: `59/reference/WebClient.md`, `FrontendParity.md`, `59/README.md:135`; `1600/docs/developer/Web-Client.md`; `docs/contracts/frontend.md` FE-7; LIB-15 spec/design (decision-log row; a new AC for the shared props and the release handle, next free number); `docs/inventory/*` (history, left)
- skills: `59/.claude/skills/calc-u-59-web/SKILL.md:103`

### 4.3 Ledger, gate
- Ledger: 59-60 (the embed component is `CalcU59`; `PlayCalculator` is gone; site authors rename), 1600-52 (`layout` prop; the release build's handle has no `c1600.upload`/`c1600.display-text`).
- Gate: Frequent; local CI base + web-59 + web-1600; a `--release` Web build of each product and the no-test-aids checks; GitHub CI (workflow change) at the end.

---

## 5. Step 5: one rule per product pair

| # | Rule | Change |
|---|---|---|
| 5a | Qt6: raise the window on a drop and on Open With, accept the drop's proposed action, open deferred | moved into `library/qt6/FileOpening` (a helper both MainWindows call: raise/activate unless `Start`; a drop handler that accepts and defers); `59/frontends/qt6/app/MainWindow.cpp:143-145,208-219` take it, `1600/frontends/qt6/app/MainWindow.cpp:596-622` call it; the header sentence "stays the product's" (`FileOpening.hpp:17-21`) changes. Tests: a 59 Qt drop test and a `QFileOpenEvent` test (as 1600's `window_tests.cpp:620-645`) |
| 5b | Web: a drop of exactly one file is opened, more are ignored | `library/web/components/useShellProduct.jsx:21-30,85-99`: one rule, `CALCU_DROP_FIRST_FILE` removed; `59/frontends/web/src/PlayCalculator.jsx:322` (`CalcU59.jsx` after step 4). Test: 59 `play-calculator.spec.js` two-file drop ignored |
| 5c | trace folder and limit read at each trace start; refusal "The trace folder cannot be written: <reason>." | `library/parts/LibraryParts.cpp:24-48` (`withTraces`): one path, folder and limit read from the settings file at each trace start, `Paths::ensure` with the write probe; the refusal ends with "."; `placesAtStart` no longer applies to traces (it still freezes the save and screenshot folders, unchanged); Calc-U-59's own gate `kTraceFolderNotWritable` (`TIDebugPart.hpp:85`, `.cpp:614`) and `traceAvailable()` (`TIAppPart.cpp:56-70`) are removed or take the library's answer. Swift: the session asks the frontend for the folder at each trace start (`HostDirs` gets a callback in place of the start-time `chosenTraceFolder`; `TIAppSession.mm:440,449-473`; `EmulatorViewModel.swift:277,492-496`); the bookmark logic stays. Qt6: the comment `59/frontends/qt6/app/EmulatorController.cpp:60-64` goes. Tests: `library/debugger/tests/trace_tests.cpp:162,170`, `59/tests/debug_panel_tests.cpp:137`, a new test: a folder changed after start is used by the next trace (both products) |
| 5d | `[trace] max-file-size-mb` per LIB-9 AC-11 (missing, zero or negative → 50; MiB) | `library/settings/Declaration.cpp:129-134`: no `min = 1` (0 and negatives are accepted and mean 50), description "The largest trace file in MiB; zero or less: 50."; the label stays "Maximum file size, in MB" in the dialog (LIB-14 AC-11 text) |
| 5e | Calc-U-59 Web `screenshot:` = a download | `59/frontends/web/bindings.cpp:64` sets no screenshot folder (KeepFile → download, as Calc-U-1600); `59/frontends/web/src/page.jsx:26-47` passes only traces to the LOG tab's list (filter by the keep-file's kind/suffix). Test: a Web preset with `screenshot:` offers a download, nothing written to the save folder |
| 5f | `set-control` without a PC-100C refused "No PC-100C is plugged in." | code already (`library/peripherals/PeripheralsPart.cpp:30`); add a test asserting the sentence (59) |

- Ledger: 59-61 (Qt6 raise + accept on drop and Open With; Web two-file drop ignored), 59-62 (trace folder and limit read at each trace start, the refusal sentence; Swift asks the folder each time), 1600-53 (trace refusal sentence "The trace folder cannot be written: cannot write to the folder …."), 59-63 (Web screenshot step downloads), 59-64 (`set-control` refusal without PC-100C). The `max-file-size-mb` 0 → 50 rule in 59-62/1600-53.
- Decision-log rows: LIB-13 (AC-6, AC-14, AC-21: one rule; the Calc-U-59 subsections edited), LIB-10 or LIB-9 (trace folder per trace; the sentence), LIB-14 AC-11 (log), LIB-6/LIB-9 (screenshot, AC-17 / D-13), LIB-2/C59-6 (`set-control`). New ACs if a rule has none (next free: LIB-13 AC-24, LIB-9 AC-36, LIB-14 AC-45, LIB-10 AC-53, LIB-6 AC-69).
- Help: `library/help/07-debugger.md` / trace sentences, `08-settings.md` (`max-file-size-mb`), the Web drop sentence in `59/help` if any.
- Gate: Frequent; local CI base + web-59 + web-1600; Qt tests of both products (drop/FileOpen); Swift mac build + UnitTests + UIRegressionTests.

---

## 6. Step 6: documents

| # | Change | Where |
|---|---|---|
| 6a | §9.3 card bytes "copied unchanged, in the same order"; check every row of §9.3 against `59/app/TIMagneticCardKind.cpp` (readSide 51-99, writer 227-262) and fix what differs; add a worked example from a real card: `59/tests/reference/v2.3.0-examples/historic_pdcm-make-cards.ti59.txt` card `PDCM-1.U59` (SIDE 1, `R000: 30 19 75 49 90 90 90 39`) in version 1 and its format 3 text as the codec writes it (generated by the codec, not by hand) | `docs/formats/preset.md` §9.3; `59/help/10-media-files.md:214` mirror |
| 6b | harness contract H-1: the option (a) list matches the reworded decision (no "a preset is a recorded harness session"; command names as registered: `preset`, the kind words `card`/`floppy`/`cassette`, `arm`, `full-speed`/`regular-speed`, `display-text`); H-2: no common `dump` (it is `c59 dump`, `c1600 dump-basic`): the example becomes a real common command (`registers`); the fit table row for `dump` corrected | `docs/contracts/harness.md:56-91,127-133` + decision-log row |
| 6c | help without internal paths and "M-47": `1600/help/09-module-definitions.md:169,172` (source paths, "since M-47"), `:352,580-589` (`frontends/qt6/resources/cards/…` → "the bundled templates"); GitHub links to `Ref/Shared/...` (`:80,574`) kept if public; `<!-- follows: -->` lines stripped from the built asset by `tools/helptool/asset.py` (the check still requires them in the source) | `tools/helptool/asset.py:53-71` + a helptool test; `tools/help check|build` |
| 6d | ADR 0001 note: Sharp BASIC, SDE and cassette code stay in Calc-U-1600 until a second product needs them (the status line already says it; the Consequence paragraph gets the note) | `docs/adr/0001-one-product-per-model-family.md` |
| 6e | `[folders] other` removed (declared, unused) | `59/app/Settings.hpp:38`, `Settings.cpp:49-50,96-102`; `59/tests/settings_tests.cpp:59,73,131`; `59/help/08-settings.md:64`, `help/settings-keys.txt:20`; LIB-9 design D-1 note + decision-log row; ledger none (no user saw it) |
| 6f | trace size per LIB-9 AC-11 in the docs | done in step 5d; step 6 checks `docs/contracts/settings.md`, `library/help/08-settings.md` |

- Gate: Frequent (help tests, settings tests); local CI base + help-59 + help-1600.

---

## 7. Order, branches and expected conflicts

| Wave | Steps (one worktree agent each, branch from `main`) | Why |
|---|---|---|
| A | **2** `rp2-files-media`, **5** `rp5-one-rule` (parallel) | 2 goes before 1 (shared: `SharpHarnessCommands.cpp`, `product_commands_tests`, `harness_entry_tests`, `1600/CLAUDE.md`, `contracts/harness.md`, help). 5 is independent of 1–3 except `TIAppPart.cpp` and the Web component |
| B | **1** `rp1-product-words`, **3** `rp3-topics` (parallel, from main after A) | 1 after 2; 3 after 2 (`docs/formats`, skills); 1 and 3 share `EmulatorController.cpp`, `TIAppSession.mm`, `harness_tests.cpp`, `session_tests.cpp`, the 59 skills, `59/reference/*` — small, additive conflicts, I resolve at the merge |
| C | **4** `rp4-web-embed`, **6** `rp6-docs` (parallel, from main after B) | 4 after 1 (`c1600.display-text`), 3 (`Paper.jsx`/`TIWebPart`) and 5 (`PlayCalculator.jsx` drop mode; the file is renamed by 4). 6 after 1 and 2 (`contracts/harness.md`, help `09-module-definitions.md`, names in H-1) |

Expected conflicts at the merges: `docs/behaviour-changes.md` (rows appended: keep
both, numbers per this plan: 59-58 step 1, 59-59 step 2, 59-60 step 4, 59-61…64
step 5; 1600-50 step 1, 1600-51 step 2, 1600-52 step 4, 1600-53 step 5);
`docs/GLOSSARY.md` decision log; `docs/test-groups.md` counts (add deltas);
`library/settings/Declaration.cpp` (2n and 5d); `59/app/TIAppPart.*` (3, 5);
`59/frontends/swift/Bridge/TIAppSession.mm` (1, 3, 5); skills of Calc-U-59.

**Swift:** xcodebuild fails in worktrees, so agents of steps 1, 2, 3, 5 write the
Swift changes without building them; I build and test Swift in the main checkout after
each such merge (`bin/simulators reset --role iphone`, mac build, UnitTests,
UIRegressionTests) and fix what fails on `main`.

**After each merge (`--no-ff`):** Frequent on the merged tree; the step's gate (above);
`ci/linux/run.sh` (+ web jobs in their own run); push; remove the worktree and branch.
After each rename step: a grep for the old names; a hit not listed as kept here is a bug.

**At the end:** GitHub CI on both workflows (packaging/platform code changed);
the Longer group (`build/groups`, `caffeinate -d`, both Web `dist/` built first); then
`HANDOVER.md` (release prep done, what is open). Part F (M-61) is not started.

## 8. Agent brief (all steps)
Read root `CLAUDE.md`, this plan's section for the step, the specs/designs it names
(+ §9 Decided), `features/LIB-1-machine-interface-and-descriptors/design.md` §2a;
tests first where a rule changes (`acN_…`); edit specs per "How rulings are applied"
(decision-log row, new ACs the next free number, nothing renumbered); ledger rows with
the numbers of §7; agent knowledge in the same step (skills, `CLAUDE.md`, memory);
commit on the branch; build (`build/tests`, RelWithDebInfo, Qt OFF) + Frequent, plus
the Qt tree when Qt code changes (`build/groups` or a Qt ON tree); never run
`ci/linux/run.sh`, never dispatch GitHub CI, never commit ROMs or a `node_modules`
symlink; copy the ROM folders from the main checkout; GCC 13 (no temporaries inside
`co_await`; argument order), MSVC, Emscripten; `/usr/bin/grep`; crash dialogs show on
the user's screen.
