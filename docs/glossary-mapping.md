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
| Fit / Insert / Eject | "Fit Card", "Fit Fresh Card", "Eject Card": unchanged | module picker, floppy insert/eject: unchanged |

## Code names

| Glossary term | Calc-U-59 | Calc-U-1600 |
|---|---|---|
| App session | `SessionController` | `AppSession` |
| Machine session | `EmulatorSession` | `MachineSession` |
| Frontend interface | `Platform` (`cu59::SessionController`) | `Platform` (`shared::AppSession::Platform`) |
| App layer | `Shared/` | `Shared/` |
| Model | `Model` (Shared), `MachineVariant` (Core), `ModelInfo` | model (`startup/modelPreference`) |
| Startup model | `startupModel`, `lastUsedModel` | `startup/modelPreference`, `startup/lastUsedModel` |
| Preset, preset format | `StateFile`, "state file" | preset, `PresetFile` |
| Peripheral, module | — | `CardBase`, `ExpansionCard`, "card" |
| Memory module | — | `SoftwareDefinedCard`, `BatteryCardInstance` |
| Module definition | — | `MemoryCardDefinition`, `.card.yaml` |
| Solid State ROM module | `ModuleCatalog`, `ModuleImage`, `solidStateModuleID` | — |
| Magnetic card | `CardDocument`, `CardFile`, `.u59` | — |
| Keyboard map | `KeyboardMap`, `KeySequencer` | `HostKeyMap`, `PC1500KeyboardMap` |
| Host key | — | `HostKey` |
| Full speed | `FullSpeed:` (preset directive) | `setTurbo`, `turboRequested` |
| Regular speed | `RegularSpeed:` (preset directive) | — |
| Pause | `freeze`, `FreezeController`, `FreezeOwner` | `paused` (`DebugController`) |
| Cold boot | `cleanReset` | `resetJob(allReset = true)` |
| Reset | `softReset` | `resetJob(allReset = false)` |
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
