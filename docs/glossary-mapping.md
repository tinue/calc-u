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
- "app session (code: `SessionController`)"

The code names come from the tables below. When a rename lands, the
parenthesis is removed with it.

## Visible renames

| Glossary term | Calc-U-59 today | Calc-U-1600 today |
|---|---|---|
| Cold boot | "Clean" (Reset with ⌘ held) | "Reset All" (Machine menu, ⇧⌘R) |
| Pause / Resume | "FREEZE", "RESUME" (debug panel) | "Paused (debugger)" (title bar): unchanged |
| Debug panel | CALCULATOR / CPU / LOG tabs: unchanged | "Inspector" (panel, User Guide §9.2) |
| Memory module | — | "memory card", "card" in the User Guide |
| Module definition | — | "custom memory cards", `Memory-Card-Definition-*.md` |
| Solid State ROM module | "Solid State Module" (Settings, all frontends) | — |
| Full speed | "full speed": unchanged | "full speed" in the User Guide: unchanged ("turbo" is code only) |
| Medium, media | — | "media" (e.g. "Saved Media…"): unchanged |
| Name & Save | — (cards written in place) | "Name & Save": unchanged |
| Startup preset | — | "Default presets" (Settings, Qt and web) |
| Save folder | "Card Folder" (Settings, Qt6) | "Battery-card saves" (Settings ▸ Storage) |
| Startup model | "Startup Calculator" (Settings) | "Startup device" (Settings) |
| Load Preset… | "Open Preset…" (File menu, Qt6); the button "Preset" | "Load Preset…": unchanged |
| Examples folder | — | "Samples folder" (Settings ▸ General) |
| Host drive | — | "Mount Directory…", "Host drive folder": unchanged |
| Arm | "Fit Card" → "Arm Card", "Fit Fresh Card" → "Arm Fresh Card" | — |
| Side | sides 1 and 2 of a magnetic card → 0 and 1 | sides A and B of a floppy disk → 0 and 1 |
| Insert / Eject | "Eject Card": unchanged | module picker, floppy insert/eject: unchanged |

## Code names

| Glossary term | Calc-U-59 | Calc-U-1600 |
|---|---|---|
| App session | `SessionController` | done (M-31): the library's `calcu::sessions::AppSession`; the product's part `shared::SharpAppPart` |
| Machine session | `EmulatorSession` | done (M-31): the library's `calcu::sessions::MachineSession` |
| Frontend interface | `Platform` (`cu59::SessionController`) | done (M-31): messages of the app session (topics, commands, host services) |
| App layer | `Shared/` | `Shared/` |
| Machine | `TIMachine` (`calcu::machine::Machine`, C59-1) | `PC1500InterfaceMachine`, `PC1600InterfaceMachine` (`calcu::machine::Machine`, C1600-1; built by `SharpMachineBuilder` from `sharp::sharpCatalogue()`); `PC1500Machine`, `PC1600Machine` core-internal |
| Model | `ModelId` (descriptor, `tiCatalogue()`); `cu59::Model` kept as the app enum (`modelId`, `modelFrom`), `ModelInfo` for file-format facts (no ROM file names since M-24); `MachineVariant` core-internal | `calcu::machine::ModelId` (descriptor, `sharpCatalogue()`); `shared::Model` kept as the app enum until M-49 (`modelIdOf`, `modelOf`, `variantIdOf` in `app/MachineModel.hpp`); model (`startup/modelPreference`) |
| Startup model | `startupModel`, `lastUsedModel` | `startup/modelPreference`, `startup/lastUsedModel` |
| Preset, preset format | `StateFile`, "state file" | preset, `PresetFile` |
| Peripheral, module | — | `CardBase`, `ExpansionCard`, "card" |
| Memory module | — | `SoftwareDefinedCard`, `BatteryCardInstance` |
| Module definition | — | `MemoryCardDefinition`, `.card.yaml` |
| Solid State ROM module | `SolidStateRomModule` (the peripheral in `slot-1`, C59-6), `ModuleCatalog`, `ModuleImage`, `solidStateModuleID`; the printer: `PC100C` (the peripheral in `printer`, C59-6) | — |
| ROM lookup | `calcu::roms::lookUp` (library, LIB-12), through `cu59::lookUpRoms` (`app/ProductRoms`); the full set `cu59::fullRomSet()`, the files read `calcu::roms::RomSet`, the reader `TIRomReader` (M-24; was `RomLoader`, `ResourceReader`) | `calcu::roms::lookUp` through `sharp::lookUpRoms` (`core/Machine/SharpRoms`); the full set `sharp::fullRomSet()`, `calcu::roms::RomSet`, the reader `SharpRomReader` (the CE-502B's module definition); the preset loaders' `PresetRoms` (M-27; was `BundledRomCatalog`, `SharpRomImages`, `romDirs`) |
| Magnetic card | `CardDocument`, `CardFile`, `.u59` | — |
| Keyboard map | `KeyboardMap`, `KeySequencer` | `HostKeyMap`, `PC1500KeyboardMap` |
| Host key | — | `HostKey` |
| Full speed | `FullSpeed:` (preset directive) | the `speed {full}` command (M-31); `setTurbo`, `turboRequested` in the frontends |
| Regular speed | `RegularSpeed:` (preset directive) | — |
| Pause | `freeze`, `FreezeController`, `FreezeOwner` | `paused` (`DebugController`) |
| Cold boot | `Machine::coldBoot()` (was `cleanReset`) | `Machine::coldBoot()` (was `resetJob(allReset = true)`, `allReset()`) |
| Reset | `Machine::reset()` (was `softReset`) | `Machine::reset()` (was `resetJob(allReset = false)`) |
| Power cycle | `Machine::powerCycle()` (2.3.0's core reset) | `Machine::powerCycle()` (was `MachineSession::powerCycleAround`; the sequence `core/Machine/PowerCycleSequence`) |
| Host link, serial port file | — | `calcu::peripherals::PtyHostLink`, `HostLinks` (was `PtySerialLink`, `LazySerialLink`); the machine's side `SerialPortCapability` (serial port kind) |
| Real-time clock | — | `calcu::peripherals::RealTimeClock`, `setClocksFromHost` (was `seedClock`, `seedClockFromHostTime`; `seedClock` stays core-internal) |
| Feature ID | — | — (`KIT-n` in early docs, now `LIB-n`, `C59-n`, `C1600-n`) |
| Harness | `tools/headless/`, `tools/agentic/` | `pc1500_cli`, `pc1600_cli`, probes |

## Same word, different meaning (resolved)

| Word | Was | Now |
|---|---|---|
| card | 59: magnetic card, cue card; 1600: memory module and any peripheral | Magnetic card, Cue card; Module, Peripheral |
| module | 59: Solid State ROM module; 1600: memory module | Module, with both kinds named |
| full speed | 59 hardware docs: TI-58C idle at the undivided clock; emulator gesture | Full speed is the emulator gesture only; hardware text says "undivided clock" |
| family | product family, chip family, group of models | Family, Chip family, Model family |
| Shared | per-product `Shared/` vs code used by both products | App layer vs Library |
| Platform | OS and architecture vs frontend callback interface | Platform vs Frontend interface |
