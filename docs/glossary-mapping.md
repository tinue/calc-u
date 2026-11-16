# Glossary mapping

How today's names in Calc-U-59 (`v2.3.0`) and Calc-U-1600 (`v0.8.1`) map to
the terms in [`GLOSSARY.md`](GLOSSARY.md). Briefing §1.5 requires every
rename of a visible label, menu item or help text to be listed here; those
rows are marked **visible**. Code renames are listed so later documents
can refer to them; they happen during migration (phase 6), not now.

Sources: the naming-conflicts table in
[`inventory/divergence-matrix.md`](inventory/divergence-matrix.md), the
product inventories, and the products' code at the pins.

## Naming code before the renames

Until migration (phase 6) renames the code, every document, skill, memory
file and comment uses the glossary term. Where it names code, it adds the
current code name after the term:

- "memory module (code: `SoftwareDefinedCard`)"
- "app session (code: `TIAppPart`)"

The code names come from the tables below. When a rename lands, the
parenthesis is removed with it.

## Visible renames

| Glossary term | Calc-U-59 today | Calc-U-1600 today |
|---|---|---|
| Cold boot | "Clean" (Reset with ⌘ held) | done (M-46): "Cold boot" (Machine menu, ⇧⌘R; Qt6 and Web; the shell's Machine menu since M-49, no shortcut on the Web); was "Reset All" |
| Pause / Resume | "FREEZE", "RESUME" (debug panel) | "Paused (debugger)" (title bar): unchanged |
| Debug panel | CALCULATOR / CPU / LOG tabs: unchanged | "Inspector" (panel, User Guide §9.2) |
| Memory module | — | "memory card", "card" in the User Guide |
| Module definition | — | "custom memory cards", `Memory-Card-Definition-*.md` |
| Solid State ROM module | "Solid State Module" (Settings, all frontends) | — |
| Full speed | "full speed": unchanged | "full speed" in the User Guide: unchanged ("turbo" is code only) |
| Medium, media | — | "media" (e.g. "Saved Media…"): unchanged |
| Name & Save | — (cards written in place) | "Name & Save": unchanged |
| Startup preset | — | done (M-46): "Startup presets" (Settings, Qt6 and Web); was "Default presets" |
| Save folder | "Card Folder" (Settings, Qt6) | done (M-47): "Save folder:" (Settings ▸ Storage, Qt6); was "Battery-card saves:" |
| Startup model | "Startup Calculator" (Settings) | done (M-49): "Startup model" (Settings ▸ General, Qt6 and Web; the choices by the model's name); was "Startup device" |
| Load Preset… | "Open Preset…" (File menu, Qt6); the button "Preset" | "Load Preset…": unchanged (Ctrl+O / ⌘O in Qt6 since M-49) |
| Examples folder | — | done (M-49): "Examples folder" (Settings ▸ Folders, Qt6); was "Samples folder" |
| Host drive | — | "Mount Directory…", "Host drive folder": unchanged |
| Arm | "Fit Card" → "Arm Card", "Fit Fresh Card" → "Arm Fresh Card" | — |
| Side | sides 1 and 2 of a magnetic card → 0 and 1 | done (M-47): sides A and B of a floppy disk → 0 and 1 (the side button, the disk file `0:`/`1:`, `floppy: { side: }`) |
| Insert / Eject | "Eject Card": unchanged | module picker, floppy insert/eject: unchanged |

## Code names

| Glossary term | Calc-U-59 | Calc-U-1600 |
|---|---|---|
| Shell | — | done (M-49): the Qt6 window from `calcu::qt6::Shell` over `calcu::qt6::SessionHost` (in `MachineController`), the Web page from the React set (`useCalcuShell`); the product's data `app/SharpShellProduct`; was `MainWindow::buildMenuBar`, `ControlBar`, `SettingsDialog`, `AboutDialog`, `Operations`, `EmulationPacer`, the Web's `MenuBar.jsx`, `ControlBar.jsx`, `engine.js` |
| Control state | — | done (M-49): the library's `control-state` (`calcu::control::ControlPart`) with `shared::SharpControlRules` and its product entries (`c1600.disk-controls`, `c1600.ce158-printer-panel`, `c1600.host-drive-folder`, `c1600.mount-directory`, `c1600.ce1600p-next-variant`); was `shared::ControlState`, `controlStateOf`, the topics `c1600.control-state`, `c1600.machine`, `c1600.ui` |
| App session | done (M-33, M-34): `calcu::sessions::AppSession` with `cu59::TIAppPart` (every frontend; `SessionController` taken apart into `TIAppPart` in M-34) | done (M-31): the library's `calcu::sessions::AppSession`; the product's part `shared::SharpAppPart` |
| Listener | done (C1 simplification): `calcu::sessions::Listener` in the Qt6 `EmulatorController`, on the Swift `SessionThread` (was its own sink and reply map, `HostedSession`/`AppSessionHost`), the harness and the tests | done (C1 simplification): `calcu::sessions::Listener` in the Qt6 `calcu::qt6::SessionHost` (since M-49), `tools/wasm_smoke` and the tests (was `SharpHost`; the session is made by `makeSharpSession`) |
| Machine session | done (M-33, M-34): `calcu::sessions::MachineSession` (every frontend; `EmulatorSession` keeps the TI runtime around the machine, adapters `TIFreezeGate`, `TIKeystrokePlayback` (was `TIKeySequencerHook`, M-36)) | done (M-31): the library's `calcu::sessions::MachineSession` |
| Frontend interface | done (M-33, M-34): messages and commands of the app session (every frontend; `TIAppSession` is the Swift bridge, was `TISessionController`; the Web's through the library's `WorkerHost`) | done (M-31): messages of the app session (topics, commands, host services) |
| App layer | `Shared/` | `Shared/` |
| Machine | `TIMachine` (`calcu::machine::Machine`, C59-1) | `PC1500InterfaceMachine`, `PC1600InterfaceMachine` (`calcu::machine::Machine`, C1600-1; built by `SharpMachineBuilder` from `sharp::sharpCatalogue()`); `PC1500Machine`, `PC1600Machine` core-internal |
| Model | `ModelId` (descriptor, `tiCatalogue()`); `cu59::Model` kept as the app enum (`modelId`, `modelFrom`), `ModelInfo` for file-format facts (no ROM file names since M-24); `MachineVariant` core-internal | `calcu::machine::ModelId` (descriptor, `sharpCatalogue()`); done (M-49): `app/` and the frontends name models by `ModelId` (`shared::sharpModels()`, `sharpModel()` in `app/Settings`), the frontends draw them from the control state; `shared::Model`, `PC1600RomVersion`, `CE1600PRomVersion`, `modelKey` and `app/MachineModel.hpp` gone |
| Startup model | `startupModel`, `lastUsedModel` | `startup/modelPreference`, `startup/lastUsedModel` |
| Preset, preset format | `StateFile`, "state file" | done (M-46): `calcu::presets` (the library's reader and engine, `PresetsPart`) with `shared::SharpPresetRules` and the keywords of `app/SharpPresetKeys`; the preset list from `examples/bundle.yaml`; was `PresetFile`, `PresetRunner`, the model's preset loaders, `PresetCatalog`, `examples/bundled.yaml` |
| Peripheral, module | — | `CardBase`, `ExpansionCard`, "card" |
| Memory module | — | `SoftwareDefinedCard`, `BatteryCardInstance` |
| Module definition | — | `MemoryCardDefinition`, `.card.yaml` |
| Solid State ROM module | `SolidStateRomModule` (the peripheral in `slot-1`, C59-6), `ModuleCatalog`, `ModuleImage`, `solidStateModuleID`; the printer: `PC100C` (the peripheral in `printer`, C59-6) | — |
| ROM lookup | `calcu::roms::lookUp` (library, LIB-12), through `cu59::lookUpRoms` (`app/ProductRoms`); the full set `cu59::fullRomSet()`, the files read `calcu::roms::RomSet`, the reader `TIRomReader` (M-24; was `RomLoader`, `ResourceReader`) | `calcu::roms::lookUp` through `sharp::lookUpRoms` (`core/Machine/SharpRoms`); the full set `sharp::fullRomSet()`, `calcu::roms::RomSet`, the reader `SharpRomReader` (the CE-502B's module definition); the preset loaders' `PresetRoms` (M-27; was `BundledRomCatalog`, `SharpRomImages`, `romDirs`) |
| Magnetic card | `CardDocument`, `CardFile`, `.u59` | — |
| Keyboard map | done (M-36): `cu59::TIKeyboardMap` (`app/TITyping`), a `calcu::typing::KeyboardMap`; was `KeyboardMap`, `matrixCodesFor` | done (M-36): `shared::SharpKeyboardMap` (`app/SharpTyping`), a `calcu::typing::KeyboardMap`; was `HostKeyMap`, `resolveHostKey` (`PC1500KeyboardMap` is the faceplate's key layout, not a keyboard map) |
| Typing engine | done (M-36): `calcu::typing::TypingEngine` in `calcu::typing::TypingPart`, with `cu59::typingProfile`; was `KeySequencer` (its KEYSTROKES lane is the adapter `TIKeystrokePlayback` until M-54) | done (M-36): `calcu::typing::TypingEngine` with `shared::typingProfile`; was `SharpTypingHook`, `KeyPasteFeeder`, the PC-1500 key queue (`c1600.key-queue`, `enqueueKey`) |
| Host key | done (M-36): `calcu::typing::HostKeyEvent` | done (M-36): `calcu::typing::HostKeyEvent`, `NamedKey`, `Modifiers`; was `HostKey`, `HostModifiers` |
| Full speed | `FullSpeed:` (preset directive) | the `speed {full}` command (M-31); `setTurbo`, `turboRequested` in the frontends |
| Regular speed | `RegularSpeed:` (preset directive) | — |
| Pause | `freeze`, `FreezeController`, `FreezeOwner` (the library's side from M-40: `calcu::debugger::RunControl`, the `run-state` topic; the TI moves onto it in M-53) | done (M-45): the `run-state` topic of `calcu::debugger::RunControl` (`MachineController::machinePaused()`); was `paused` (`DebugController`), `SharpDebuggerGate` |
| Cold boot | `Machine::coldBoot()` (was `cleanReset`) | `Machine::coldBoot()`, the library's `cold-boot` command (M-46; was `resetJob(allReset = true)`, `c1600.reset {all}`, `allReset()`) |
| Reset | `Machine::reset()` (was `softReset`) | `Machine::reset()` (was `resetJob(allReset = false)`) |
| Power cycle | `Machine::powerCycle()` (2.3.0's core reset) | `Machine::powerCycle()` (was `MachineSession::powerCycleAround`; the sequence `core/Machine/PowerCycleSequence`) |
| Paper, paper panel | — | done (M-45): the library's paper (`calcu::paper::Paper`, `PaperPart`) and paper panel (Qt6 `calcu::qt6::PaperPanel`, React `CalcuPaperPanel`); the plotters' `sharp::PlotterPaperOutput`; was `PlotterPaperWidget`, `PlotterPaper` (Web, `Panels.jsx`), `PaperKind`, `PaperGeometry`, the `c1600.plotter-paper` kind |
| Host link, serial port file | — | `calcu::peripherals::PtyHostLink`, `HostLinks` (was `PtySerialLink`, `LazySerialLink`); the machine's side `SerialPortCapability` (serial port kind) |
| Real-time clock | — | `calcu::peripherals::RealTimeClock`, `setClocksFromHost` (was `seedClock`, `seedClockFromHostTime`; `seedClock` stays core-internal) |
| Plan | — | done (M-44, C1600-2): `sharp::loaders::planCode` (`core/Loaders`) over `machinecode::planLoad`, the request `c1600.plan-code` (`app/SharpLoaderPart`); was `machinecode::plan()` / `Plan` (the menu's second plan) and `shared::MachineCodeLoad` (`app/MachineCodeLoad`, gone) |
| Program file kind | — | done (M-44, C1600-2): `sharp::loaders::fileKind` over `programfile::classify` (`core/ProgramFile`), the request `c1600.file-kind`; was `shared::programChoice` for a WAV's files |
| Open (a file), file kind | `c59.open-dropped`, `c59.drop-action`, `dropActionFor` (M-56) | done (M-48, LIB-13): the library's `open` and `open-would-take` (`calcu::opening::OpenPart`, owned by `shared::SharpAppPart`), the file kinds `shared::BasicProgramKind`, `MachineCodeKind` (`app/SharpFileKinds`), the topic `c1600.program-opened`, Qt6 `MainWindow::openFile`; was `dropfile::classify` (`core/DropFile`), `c1600.drop-target`, `openDroppedFile`, `droppableFile`, `drainPendingDrop`, the Web's `openDropped`, `loadPresetFiles`, `MAX_DROP_BYTES` |
| Loader memory access | — | done (M-44, C1600-2): the kind `c1600.loader-memory`, `sharp::LoaderMemory` (`peek`, `mode`, `programArea`, `writeBuiltIn`, `writeSlot`, `slotRam`, `module`; was `peek`/`poke`), over the core in `core/Machine/SharpLoaderMemory`; the adapters `legacy::loadBasicProgram`, `loadMachineCode`, `machineCodeLoadState`, `userRam` gone |
| Feature ID | — | — (`KIT-n` in early docs, now `LIB-n`, `C59-n`, `C1600-n`) |
| Server, DAP session | — (no DAP, Briefing §2) | done (M-50, C1600-3): the server `shared::dap::DapPart` (`app/dap/`, a session part: the topic `c1600.dap`, the commands `c1600.dap-disconnect`, `c1600.dap-port`), one `shared::dap::DapSession` per client, its socket a host's `DapTransport` (Qt6 `DapSocket`, the harness's `serve --dap`); was `DapServer`, `DapSession` (QJson) and `DebugController` in `frontends/qt6/app/debug/` |
| Harness | `tools/headless/`, `tools/agentic/` | `pc1500_cli`, `pc1600_cli`, probes |
| Medium, kind of medium, media place | `CardFile`, `TIFileAccess` (M-55) | done (M-47): the library's `calcu::media::MediaPart` (topic `media`, commands `media.*`) with `app/SharpMediaKinds` (kinds `battery-backed-module`, `floppy-disk`, `cassette`) and `core/Machine/SharpMediaPlaces` (`FloppyPlace`, `CassettePlace`, `SlotModuleMedium`); was `ModuleBay`, `FloppyBay`, `TapeBay`, `MediaLibrary` ("bays"), `c1600.select-module`, `MediaSave.hpp` |

## Same word, different meaning (resolved)

| Word | Was | Now |
|---|---|---|
| card | 59: magnetic card, cue card; 1600: memory module and any peripheral | Magnetic card, Cue card; Module, Peripheral |
| module | 59: Solid State ROM module; 1600: memory module | Module, with both kinds named |
| full speed | 59 hardware docs: TI-58C idle at the undivided clock; emulator gesture | Full speed is the emulator gesture only; hardware text says "undivided clock" |
| family | product family, chip family, group of models | Family, Chip family, Model family |
| Shared | per-product `Shared/` vs code used by both products | App layer vs Library |
| Platform | OS and architecture vs frontend callback interface | Platform vs Frontend interface |
