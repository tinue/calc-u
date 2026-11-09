# Inventory: divergence matrix

Capability × product: for every capability that either product inventory
covers, how Calc-U-59 and Calc-U-1600 do it today, where they contradict or
drift apart, and a *suggested* base side. Phase 1 output
(`docs/Process.md`, phase 1). **Facts only; the "Suggested base" column is a
suggestion, not a decision. Every fork is decided in phase 3 (ADRs).**

Built only from these inventory files (read on 2026-11-08), not from the
product repositories:

| Tag | File | Pins it records |
|---|---|---|
| `59` | `docs/inventory/calc-u-59.md` | Calc-U-59 `v2.3.0` @ `231792c` (= `dev-2.3.0` @ `915a2b5`); calc-u-site @ `a383fbc` (site now `acd629c`); `ti59-research` @ `bf423f9` |
| `1600` | `docs/inventory/calc-u-1600.md` | Calc-U-1600 `v0.8.1` (until tagged, `dev-0.8.1` @ `b4ea8c8`; read at `v0.8.0` @ `d976d3f`, delta folded in); `Sharp1500-1600-Ref` @ `43743ed`; CI run `42369839986` |
| `RT` | `docs/inventory/release-tooling.md` | ReleaseIt `main` @ `02c41c6`; both product pins |
| `Site` | `docs/inventory/calc-u-site.md` | calc-u-site `main` @ `acd629c` (read at `59/2.3.0` @ `a383fbc`, delta folded in) |
| `AK` | `docs/inventory/agent-knowledge.md` | used for naming conflicts only |

Citation style: `path:line` is in the product named by the column, at its
pin, exactly as the inventory cites it; `[59 §X]` names the inventory and
section the fact comes from. No product file was re-read for this matrix:
no direct contradiction between two inventories was found that needed it
(see Open questions for the inconsistencies found inside single
inventories).

Base-side values: **59**, **1600**, **merge** (take parts of both),
**neither** (a new design is needed; usually an ADR), **per product**
(nothing to share; the capability exists in one product only, or is
product material per Briefing §1.3).

## Main table

### Architecture and core

| # | Capability | Calc-U-59 | Calc-U-1600 | Divergence | Suggested base (suggestion) |
|---|---|---|---|---|---|
| M1 | Layers, language | `Core/`, `Shared/` (namespace `cu59`), `Bridge/` (ObjC++), `App/` (Swift), `Qt6/app/`, `Web/`; C++17, std only (`CMakeLists.txt:32,44-51`) [59 §Architecture] | `Core/`, `Shared/`, `Qt6/app/`, `Web/`; C++20, std only (`CMakeLists.txt:33`, `docs/developer/Shared-Architecture.md:7-25`) [1600 §Architecture] | Same Core/Shared/frontend split; C++17 vs C++20; Swift bridge layer only in 59 | **merge**: identical layering; the C++ standard is an ADR (Briefing §8: raise, never lower) |
| M2 | Layer rules | Non-GUI logic only in `Shared/`; bridge "translate types only"; `Shared/` decides no locations, must run without threads (`CLAUDE.md:60-65,79-80`) [59 §Architecture] | Non-GUI logic in Shared; frontends only translate types; Shared decides no locations (`Shared-Architecture.md:33-47`); English user messages, no translations (`:48-49`) [1600 §Architecture] | Same rules, written twice | **merge**: already the same rule set |
| M3 | Machine / core interface | `TI59Machine` facade over `ROM`, `RAM`, `TMC0501`; `Shared/` never touches `TMC0501` (`reference/CoreArchitecture.md:19-42,29-32`) [59 §Architecture, §Modelling depth] | No common machine base class; `MachineSession` holds a `PC1500Machine` and a `PC1600Machine` pointer side by side and branches 89 times (`Shared/MachineSession.hpp:382-383`); only `DebugTarget` / `MachineDebugTarget<Machine>` is common (`Core/Debug/MachineDebugTargets.hpp:21-48`) [1600 §Summary, §Both machines] | One facade per (single) machine vs no machine interface across two machines | **neither**: 59's facade covers one machine, 1600 has none; core-seam ADR (Briefing §6) |
| M4 | Modelling depth | CPU, SCOM, display/keyboard scan, Solid State module (CROM), card reader and PC-100C printer folded into class `TMC0501`; only `ROM`, `RAM` separate (`reference/CoreArchitecture.md:37`) [59 §Modelling depth] | Separate CPUs (`LH5801`, `LH5803`, `SC7852`), `PC1600BusArbiter`, `PC1600SubCpu`, `TC8576F`, `Upd1990ac`, displays; gate arrays and on-chip blocks folded into `PC1500Memory` / `PC1600Memory` (`Core/PC1600/PC1600Memory.hpp:442-562`) [1600 §Modelling depth] | Coarse single-class model vs chip-level for CPUs and several peripherals | **1600** as the structural pattern (parts as separate classes); modelling-depth ADR decides per machine |
| M5 | Interfaces between parts and devices | Printer connector, module slot, card reader all implicit (`Core/TI59Machine.hpp:15,77-83,93`); key matrix is the keyboard interface [59 §Interfaces] | Explicit, pin-numbered 40-pin memory slot(s) (`PinState`, `MemorySlotConnector`) and 60-pin system bus (`SystemBusPins`); `SerialLink`, `TapeDeck`, `AudioSink`, CPU bus interfaces explicit; CE-158 printer port, CE-1600P/F union, interrupt lines implicit (`Core/Connector/ExpansionConnector.hpp:21`, `TODO.md:254-297`) [1600 §Modelling depth] | Implicit everywhere vs explicit connectors | **1600**: matches Briefing §1.9 "explicit interfaces"; devices-keyed-by-connector ADR |
| M6 | Devices / peripherals | Printer is a flag on the facade (`setPrinterConnected`); no device objects [59 §Printer] | Peripherals are cards (`CardBase`, `ExpansionCard`, `SystemBusCard`, `SoftwareDefinedCard`); "cards know only the bus" (`docs/background/Decisions.md:505-530`) [1600 §Both machines, §Oddities] | No device model vs card-on-connector model | **1600** |
| M7 | Models and variants | TI-59, TI-58, TI-58C (`Core/MachineVariant.hpp`); per-model table `Shared/MachineModelInfo.hpp`; switch = new machine, RAM lost except TI-58C constant memory (`TODO.md:5`) [59 §Calculator models] | PC-1500 (ROM A01/A03/A04), PC-1500A (A04), PC-1600 (ROM New/Old), CE-1600P ROM New/Old; switch = fresh machine without peripherals (`User-Guide.md:73-93`); control rules in `shared::ControlState` (`Decisions.md:778-789`) [1600 §Machine control] | ROM revision selection only in 1600; model facts in a table (59) vs UI rules object (1600) | **merge**: same "switch = new machine" rule; ROM revision is 1600 scope |
| M8 | Reset and power | Reset and clean reset (Cmd/Ctrl held; 1 s long press on touch) (`reference/FrontendParity.md:23`); a preset load runs 300,000 power-on steps (ODD59-14) [59 §Further, §Oddities] | Reset and Reset All (`User-Guide.md:95-106`); startup boots straight into BASIC, no power-on step; no OFF/ON UI; peripheral attach runs a power cycle (`User-Guide.md:63-64,205-220`) [1600 §Machine control, §Printers] | Same two-level reset; different labels | **merge** |
| M9 | Clock and timing source | Constant `resonatorHz()`, instruction rates per model; TI-58C clock model of one unit (`reference/CoreArchitecture.md:55-78`) [59 §Modelling depth] | PC-1500 1.3 MHz, PC-1600 3.58 MHz T-states (`Core/PC1500/PC1500Clocks.hpp:9`, `Core/PC1600/PC1600Clocks.hpp:8`); RTC at emulated rate, seeded from host (`Decisions.md:322-328`) [1600 §Pacing] | Machine-specific | **per product** |

### Session, threading, pacing, input

| # | Capability | Calc-U-59 | Calc-U-1600 | Divergence | Suggested base (suggestion) |
|---|---|---|---|---|---|
| M10 | Session / controller | Every GUI owns one `cu59::SessionController` and implements its `Platform`; `EmulatorSession` below it [59 §Summary, `Shared/SessionController.hpp:65-67`] | Frontend implements `shared::AppSession::Platform`, builds `Settings`/`AppPaths`/`AppSession`, calls `tick()` at ~60 Hz, `start(op, done)` / `runNow(op, pump)` (`Shared-Architecture.md:70-101`); `MachineSession` below it [1600 §Architecture] | Same pattern (session object + platform interface), different names and drive model (runner thread vs frontend `tick()`) | **merge**: pattern identical; drive model follows the threading ADR |
| M11 | Threading | `Threading::Threads` (apps) or `Threading::Cooperative` (web, virtual-time tests), chosen by constructor argument (`Shared/Threading.hpp:4-14`); emulation thread per session, two key-sequencer lanes, timer; core mutexes `m_keyMutex`, `m_displayMutex`, `m_prnMutex`, `m_traceMutex` (`reference/CoreArchitecture.md:834-850`) [59 §Threading] | Core and Shared single-threaded; long work as `job::Task<T>` coroutines run sliced or to completion (`Core/Job.hpp:16-30`); "no threaded mode, unlike Calc-U-59" (`Decisions.md:734-738`); machine mutexes kept, uncontended (`:731`) [1600 §Threading] | Opposite choices; both have a cooperative path on the web | **neither**: threading ADR with the Swift/iOS frontend in mind (Process.md phase 3); possible spike (phase 5) |
| M12 | Pacing | 20 ms batches of cycle-weighted steps, absolute deadlines, random batch-end jitter (`BatchJitter`), lag > 100 ms restarts (`Shared/Pacing.hpp:9-33`); full speed by holding the display or `FullSpeed:`; full speed still sleeps in the scan loop (ODD59-5, ODD59-6) [59 §Pacing] | `shared::Pacer` ~16 ms ticks against an absolute deadline, stall > 0.1 s dropped; turbo while the LCD is held, up to 14 ms of frames (`Shared/Pacer.hpp:9-29`, `Decisions.md:212-217`) [1600 §Pacing] | Same principle (absolute deadline, drop lag ≈ 100 ms, hold display = fast); batch size, jitter (59 only), "full speed" vs "turbo" | **merge**: one pacer, per-machine parameters; jitter and scan-loop sleep stay as 59 behaviour |
| M13 | Host keyboard mapping | `Shared/KeyboardMap`: 26 of 45 keys, by produced character; Shift + letter = 2nd (ODD59-1, ODD59-2); not on iOS (`App/Views/KeyboardView.swift:178`) [59 §Keyboard] | `shared::HostKey` + modifiers + typed character → `resolveHostKey` (`Shared/HostKeyMap.hpp:1-30`); `hostKeyDown()` picks real press, Shift sequence, keystroke queue or KBII sequence (`Decisions.md:579-584`); Qt adapter `PC1500KeyboardMap` [1600 §Keyboard] | Both resolve on produced character; 1600 has a richer typing decision; key tables differ per product (Briefing §1.3) | **1600** for the mechanism (host key + typed character → per-model table), 59 key table unchanged (Briefing §1.3) |
| M14 | Key injection and hold | `KeySequencer`: ≥ 100 ms hold (real time), tap waits for the scan loop, one key at a time, auto-repeat ignored, focus loss releases (ODD59-4) [59 §Keyboard] | Every key held ≥ 2 emulated frames (`Decisions.md:585-595`); web keeps no hold of its own (`:808-812`) [1600 §Keyboard] | Hold measured in wall time vs emulated frames; sequencer in Shared vs session | **neither**: keyboard/typing contract (Process.md phase 4); both hold rules are documented oddities |
| M15 | Paste text / typed input | None found [59 §Keyboard] | Paste Text types the first clipboard line, never ENTER (`Decisions.md:605-607`); `KeyPasteFeeder` [1600 §Keyboard] | 1600 only | **per product** (1600) |
| M16 | On-screen keys / hit map | `Shared/KeyboardLayout` widened rects, `PointerTracker`; coordinates `reference/SharedArchitecture.md:191-202` [59 §Keyboard] | `Shared/KeyLayout/` per model, `FaceplateLayout`; web: nearest key within 24 px, multi-pointer (`Web-Client.md:117-124`) [1600 §Keyboard] | Hit maps are product material; hit-test strategy differs | **per product** for the maps; hit-test mechanism **merge** |
| M17 | Background behaviour | Swift (`scenePhase`) and web (`visibilitychange`) pause; Qt keeps running (ODD59-25) [59 §Threading] | Web pauses when the tab is hidden (`visibilitychange` → worker `pause`/`resume`, `Web/src/engine.js:180-181`); Qt keeps running, and only releases held keys when the window loses focus (`Qt6/app/MainWindow.cpp:733-736`); checked 2026-11-09 | Same split (web pauses, Qt runs); 59 Swift pauses too | **59** (pause where the platform expects it) |

### Presets, settings, formats

| # | Capability | Calc-U-59 | Calc-U-1600 | Divergence | Suggested base (suggestion) |
|---|---|---|---|---|---|
| M18 | Preset meaning | Applied to a *running* calculator: reset, power-on, RAM written, then KEYSTROKES played; keeps the current module and printer when absent (ODD59-8) (`reference/StateFileFormat.md:585-599`) [59 §Presets] | A script that *builds* a machine from a cold boot (model, ROM, modules, peripherals), then runs `keys:` / `program:` (`User-Guide.md:676-696`) [1600 §Presets] | Same word, different semantics (Briefing §1.5: "a preset means the same thing everywhere") | **neither**: preset ADR and glossary (phase 2) |
| M19 | Preset syntax | Text sections `MODEL:`, `PARTITION:`, `PROGRAM:`, `REGISTERS:`, `KEYSTROKES:`, `CUECARD:`, `CARD:`…`END-CARD`, `SOLID-STATE-MODULE:`, `PRINTER:`; `#` comments anywhere (`reference/StateFileFormat.md:17-45`, ODD59-9) [59 §Presets] | "YAML-shaped, not YAML", hand-written parser; only `debug:` and `bus-rom:` are YAML proper (`Decisions.md:389-394`); steps `key:`, `type:`, `wait:`, `expect:`, `saveas:`, `screenshot:`, `syncclock:`, `trace:` (`User-Guide.md:893`) [1600 §Presets] | Two unrelated grammars, both hand-parsed | **neither**: preset ADR |
| M20 | Preset versioning | None; parser checks extension and content (`Shared/StateFile.hpp:117-119`) [59 §Compatibility] | `format-version: 1` required, exact match (`Core/Preset/PresetFile.hpp:271`, `PresetFile.cpp:644-670`) [1600 §Formats] | Unversioned vs required version | **1600** (versioned envelope); 59 upgrade path is the preset ADR |
| M21 | Preset error handling | Bad lines reported, load continues (ODD59-7, `reference/StateFileFormat.md:43-45`) | All-or-nothing: a bad preset fails to parse before the machine starts (`Core/Preset/PresetFile.hpp:265-269`, checked 2026-11-09) | Tolerant vs strict | **neither**: preset ADR |
| M22 | Preset model selection | Extension `.ti59` / `.ti58` / `.ti58c` and `MODEL:` (`reference/StateFileFormat.md:17-45`) | Extension `.pc1500` / `.pc1500a` / `.pc1600`, but the parser goes by `model:` (`Decisions.md:417-419`) | Same extension-per-model convention; 1600 ignores the extension | **merge** |
| M23 | Preset scripting of GUI and runtime | Directives `Wait:`, `FullSpeed:`/`RegularSpeed:`, `Trace:`, `ZeroElapseTime:`/`ReportElapseTime:`, `ArmCard:`/`InsertCard:`, GUI controls `PrinterTrace:`, `CutPaper:`, `DebugPanel:`, `Freeze:`, `FreezeOnStart:`, `Step:`, `Resume:` (`reference/StateFileFormat.md:273-350`); reload stages (`:354-392`) [59 §Presets] | `wait:`, `trace:`, assertions `expect:`, `screenshot:`, `saveas: live\|template`, `syncclock:`; project settings in a `debug:` block (`Decisions.md:840-844`) [1600 §Presets, §Debugger] | Overlap only in wait and trace; 59 drives GUI panels, 1600 asserts and saves media | **merge**: common step engine, per-product step sets |
| M24 | Preset writer | None; presets are read only (`Shared/SessionController.hpp:65-67`) [59 §Presets] | Not written by the app; VS Code extension scaffolds templates; `examples/` by hand (`Decisions.md:680-681`) [1600 §Formats] | Neither product writes presets | **merge** (same fact) |
| M25 | Preset catalogue, bundled, defaults | Apps bundle no presets; web bundles 10 (`Web/presets.json:1-12`); no default-preset setting [59 §Compatibility] | `examples/bundled.yaml` with `defaults` (Settings ▸ Default presets) and `examples` (File ▸ Examples); default preset per model on start and model switch (`User-Guide.md:840-854`); stored as `examples:<file>` (`Decisions.md:433-438`) [1600 §Presets] | Catalogue only in 1600; 59 apps have none | **1600** for the catalogue mechanism; 59 scope stays as today (Briefing §1.3) |
| M26 | Preset loader in the GUI | Button "Preset" (Swift `NSOpenPanel` / iOS `fileImporter`), Qt File › Open Preset… (Ctrl+O), web picker with bundled list and upload; drag and drop except iOS; open-with Qt only (`App/Views/CalculatorView.swift:370-387`, `Qt6/app/MainWindow.cpp:56`, `Web/src/Dialogs.jsx:113-140`) [59 §GUI concepts] | Qt File ▸ Load Preset…, File ▸ Examples ▸ group (`Qt6/app/MainWindow.cpp:862-876`); web File ▸ Load Preset ▸ group + Upload Preset…, `?preset=` (`Web/src/CalcU1600.jsx:244-267`) [1600 §GUI concepts] | Bar button vs menu; "Open Preset" vs "Load Preset" | **neither**: GUI-sharing ADR (label per glossary) |
| M27 | Settings storage | Shared key names and resolution only (`Shared/SettingsKeys.hpp:13-42`); storage per GUI: UserDefaults, QSettings `Calc-U-59/Calc-U-59`, `localStorage` `calcu59/` [59 §Settings] | `shared::Settings` holds every key with its default, storage behind `shared::SettingsStore` (`Shared/Settings.hpp:10-17`); values as strings (`Decisions.md:775-776`); QSettings "Calc-U-1600", `localStorage` `calcu1600/` [1600 §Settings] | 1600 has the store abstraction in Shared; 59 only key names | **1600** |
| M28 | Settings scope | Includes hardware: `solidStateModuleID`, `startupModel`, `lastUsedModel`, `ledFontStyle`, `keyboardFeedback`, `traceMaxFileSizeMB` (`Shared/SettingsKeys.hpp:13-42`) | App preferences only; "hardware picks (model, modules) are on the control bar and not saved" (`Decisions.md:650-652`); keys incl. `startup/modelPreference`, `startup/lastUsedModel`, `trace/maxFileSizeMB`, `debug/dap*`, `preset/default/<model>` (`Shared/Settings.hpp:34-121`) | Module choice persisted in settings vs never saved | **neither**: GUI-sharing / family-coherence ADR |
| M29 | Settings dialog | Swift Settings scene / iOS sheet (gear); Qt File › Settings…; web gear button (`Qt6/app/MainWindow.cpp:60`, `Web/src/ControlBar.jsx:116`); web applies module on close (`Web/src/Dialogs.jsx:152-172`) [59 §Settings] | Qt app menu (macOS) / Edit ▸ Settings… elsewhere; web Edit ▸ Settings…; Close only, changes apply immediately (`User-Guide.md:916-938`) [1600 §Settings] | Menu location (File vs Edit); apply-on-close (59 web) vs immediate | **neither**: GUI-sharing ADR |
| M30 | File locations | Platform chooses: Swift iCloud Documents with local fallback, Qt `~/Calc-U-59` or override, web in-memory (`App/CardStorage.swift:9-13`, `Qt6/app/AppPaths.cpp:19-23`) [59 §Cards, §Constant memory] | Frontend passes `shared::HostDirs` (`Shared/AppPaths.hpp:17`); defaults `~/Calc-U-1600`, `~/Calc-U-1600/Tapes`; overrides `storage/instanceDirOverride`, `storage/tapeDirOverride` [1600 §Settings, §Cassette] | Same rule; 1600 has a shared `AppPaths` type | **1600** (`AppPaths`/`HostDirs`) |
| M31 | Own format versioning policy | Only card text (`Calc-U-59 Card 1.0`/`2.0`) and binary trace (v1/v2) versioned; presets, `ti58c.mem`, ASM overlay, settings, module catalog unversioned; a v1 card is written back as 1.0 (`Shared/CardFile.cpp:293-294`) [59 §Compatibility] | Every format the app reads carries a required `format-version`; missing/unknown refused; before 1.0 old forms fail, from 1.0 the previous version keeps working and files upgrade on next save (`Decisions.md:671-685`); trace binary v3; settings, `bundled.yaml`, `*.shots.yaml` unversioned [1600 §Formats] | Mostly unversioned vs consistently versioned; 1600's "from 1.0" rule equals Briefing §1.2's read-old/write-new | **1600** policy; 59 migration is the preset/format ADR |
| M32 | External formats | ROM text dumps (Datamath "ROM Recorder" text) only (`roms/calculator/TMC0582.txt:1-8`) [59 §Compatibility] | ROM images, cassette WAV, `.bas`, `.bbin`, `.bin` (PC-1600 / CE-158 headers, headerless), assembler listings, host-drive files, CE-158 raw output, PNG, DAP [1600 §Formats] | Product-specific sets | **per product**; the own/external tag is shared (Briefing §1.2) |

### Media, removable items, peripherals

| # | Capability | Calc-U-59 | Calc-U-1600 | Divergence | Suggested base (suggestion) |
|---|---|---|---|---|---|
| M33 | Removable-media framework | No framework; card flow in `SessionController`, files in `Shared/CardFile` [59 §Cards] | Bays in Shared: `ModuleBay`, `FloppyBay`, `TapeBay` with catalogues, attach, Name & Save, `saveas:`, debounced autosave; `MediaLibrary` (list/export/import/delete); picker cache `DirectoryScanCache` (`Shared-Architecture.md:57,64`) [1600 §Media] | Ad-hoc vs generic bays | **1600**, to be checked against the TI-59 card reader in the phase 4 fit table |
| M34 | Template vs instance | Card files are written back in place; a fresh card with an existing name asks first (ODD59-29) [59 §Cards] | Bundled items are templates, never written; Name & Save makes an instance that autosaves ~0.5 s after a change; bundled-first resolution (`User-Guide.md:484-522`, `Decisions.md:688-689`) [1600 §Media] | Different save model; both debounce ~0.5 s (59 `ti58c.mem`, 1600 instances) | **1600** model; 59 scope unchanged |
| M35 | Module loader (Solid State module vs memory module) | 14 module ROMs; one always inserted (default `ML`), no "no module"; chosen in Settings in every GUI; change = soft reset (`Shared/SettingsKeys.hpp:24,40-42`, `App/Views/SettingsView.swift:74`, `Qt6/app/SettingsDialog.cpp:37-41`) [59 §Solid State] | `.card.yaml` software-defined cards; one slot (PC-1500/1500A) or two (PC-1600); control-bar picker per slot with save icon, first entry "–empty–" (`User-Guide.md:381`, `Qt6/app/ControlBar.cpp:48`); choice restarts the machine (`User-Guide.md:378-402`, `Qt6/app/ControlBar.hpp:12,75`) [1600 §Memory modules] | Settings vs control-bar drop-down (Briefing §1.3); soft reset vs restart; ROM module vs memory definition | **neither**: GUI-sharing ADR (family coherence) |
| M36 | Module content format | Datamath ROM text `roms/solid-state/TMC05xx.txt` + own catalog `cuecards.txt` (unversioned) [59 §Solid State, §Compatibility] | Own `.card.yaml`, `format-version: 1` (`Core/Connector/MemoryCardDefinition.hpp:24-25,1353`) [1600 §Formats] | External ROM + own catalog vs own definition file | **per product** |
| M37 | Magnetic cards | TI-59 only; text card v2.0 (two sides), fit/feed by corner press, auto-eject, write-back merge; pickers list `.u59`/`.bin` (`reference/SharedArchitecture.md:282-319`, `Shared/CardFile.hpp:95-100`) [59 §Cards] | — | 59 only | **per product** (59), but on the shared media framework (M33) |
| M38 | Cue cards | Three templates, layout and draw list in `Shared/CueCard.cpp` [59 §Cue cards] | — | 59 only | **per product** |
| M39 | Persistent machine memory | `ti58c.mem` text, saved 0.5 s after a change and on switch/load/suspend (`reference/SharedArchitecture.md:394-406`) [59 §Constant memory] | Battery-card instances autosave (`BatteryCardInstance.hpp`); "Battery-card saves" folder [1600 §Memory modules, §Settings] | Analogous persistence, different carriers | **merge** (same debounce idea) |
| M40 | Floppy (CE-1600F) | — | `.floppy.yaml` (`format-version: 1` + `format:`), attaches with CE-1600P, A/B, no GCR (`User-Guide.md:448-458`) [1600 §Floppy] | 1600 only | **per product** |
| M41 | Cassette tape | — | Cassette bay (ROM `CLOAD`/`CSAVE` via WAV in emulated time) and fast-load via libsharpdx; never share code (`Decisions.md:91-124,471-477`); partial [1600 §Cassette] | 1600 only | **per product** |
| M42 | Host drive, serial ports | — | Host drive `S3:` with own ROM and protocol (Qt only); RS-232C / CE-158 as PTYs, macOS/Linux only [1600 §Host drive, §Serial] | 1600 only | **per product** |
| M43 | Program files | ASM overlay `.asm` loaded into the debugger at `0x1800–0x1FFF` (`reference/DebugAPI.md:380-397`) [59 §Debugger] | Load BASIC / Machine Code (`.bas`, `.bbin`, `.bin`, WAV), content-based loader choice via `sde_file_info` (`Decisions.md:465-470`) [1600 §Program files] | Different purpose | **per product** |
| M44 | Drag and drop, open-with | Drop on all GUIs except iOS; open-with Qt only (`Qt6/app/main.cpp:13-25`) [59 §Presets] | Drop routed by `Core/DropFile` classification (`Decisions.md:478-484`); macOS Dock / Open With; web drop ≤ 1 MB [1600 §Program files, §Other] | 1600 classifies in Core | **1600** (classification in a shared place) |
| M45 | Printer / plotter device | PC-100C: functional, inside `TMC0501` (`PRT.*` opcodes, BUSY in instruction cycles, measured dot timing); presence and keys as K-lines (`reference/CoreArchitecture.md:599-611,722-830`) [59 §Printer] | CE-150 and CE-1600P as cards on the 60-pin bus, shared `AlpsPlotterMechanism`; CE-158 printer port to a panel; one plotter at a time; attach = power cycle (`Core/Connector/Ce150Card.hpp:12-30`, `User-Guide.md:198-220`) [1600 §Printers] | Implicit opcode-level vs device card on connector | **1600** structure; whether the PC-100C connector becomes explicit is the modelling-depth ADR |
| M46 | Paper panel (GUI) | Connect toggle (resets), dot and text view, PRINT/ADV/TRACE, Copy text + 600 dpi image, Cut; starts connected (`reference/FrontendParity.md:51-59`) [59 §Printer] | Paper panel Copy (high-res, physical size) and Cut; CE-158 panel Save…/Clear (`User-Guide.md:222-233,653-655`) [1600 §Printers] | Near-identical Copy/Cut; 59 also has text view and printer keys | **merge** |
| M47 | SDE integration | — | `libsharpdx` 0.3.5: committed macOS and WASM `.a`, Linux/Windows downloaded from an SDE GitHub release (`tools/fetch_sharpdx.sh`); `.bbin`, `.floppy.yaml`, serial file names are SDE contracts [1600 §SDE, RT §SDE] | 1600 only | **per product** (1600); dependency handling feeds the CI ADR |

### Debugger, traces, display, audio

| # | Capability | Calc-U-59 | Calc-U-1600 | Divergence | Suggested base (suggestion) |
|---|---|---|---|---|---|
| M48 | In-app debugger / inspector | CALCULATOR, CPU, LOG tabs, ASM overlay; FREEZE, FREEZE ON START, STEP, RESUME; all three GUIs (web wide layout only; iPhone off by default) (`reference/FrontendParity.md:61-76`, `reference/DebugAPI.md:7-19`) [59 §Debugger] | INSPECTOR panel: Pointers, Memory ▾, Dump ▾, Card YAML, Clear, TRACE, LOG (placeholder); views built in Core (`Decisions.md:636-642`, `User-Guide.md:941-993`); web without TRACE/LOG [1600 §Debugger, §GUI concepts] | Interactive stepping debugger in the GUI vs read-only inspector panel | **neither**: GUI-sharing ADR; debugger contract (phase 4) |
| M49 | Breakpoints, DAP | Core PC breakpoints for headless tools only; session ignores them (ODD59-27); no DAP [59 §Debugger] | DAP server on 127.0.0.1:32168 (Qt only), conditions, hit counts, data breakpoints, listings, bank qualifiers; VS Code extension 0.2.0; DAP code written against QJson, not in Shared (`Debugger-Internals.md:10-12`, `Shared-Architecture.md:133-140`) [1600 §Debugger] | 1600 only; Calc-U-59 need not gain it (Briefing §1.3) | **1600** (only implementation); move out of Qt is design input |
| M50 | Debug model in code | `Shared/DebugModel`, `DebugSnapshots`, `DebugText`, `DebugLog`, `RomHeatmap`, `FreezeController`; `CpuFrame` [59 §Debugger] | `Core/Debug/` (`DebugTarget`, `CpuViews`, `RunControl`, `SourceMap`, `Listing/`, `Disasm/`, `Inspect/`); pre-execution snapshots (`Decisions.md:866-871`) [1600 §Debugger] | Debug model in Shared (59) vs Core (1600) | **neither**: debugger contract (phase 4) |
| M51 | CPU trace file | Binary v2, magic `TI59`, 125-byte `CpuFrame` records; manual and `Trace:` scripted; size limit setting `traceMaxFileSizeMB` (default 50, `Shared/SettingsKeys.hpp:21,36-37`); reader `tools/read_trace.py` (v1, v2) (`reference/DebugAPI.md:417-456`) [59 §Traces] | `TRACE.bin`, magic `PC15`, version 3, frames of both CPUs; INSPECTOR TRACE or preset `trace:`; rolls over at `trace/maxFileSizeMB` (default 50); reader `tools/read_trace.py` (`Core/PC1500/PC1500TraceFile.cpp:9-11`) [1600 §Other, §Formats] | Same idea (versioned binary, size limit, Python reader of the same name); frame layout per CPU | **merge**: one envelope, per-CPU frame |
| M52 | Display | LED 7-segment: per-strobe glyph and duty, `DisplayPov` brightness, `SevenSegment.hpp`, "Modernized"/"Classic" (`reference/CoreArchitecture.md:491-531`) [59 §Display] | Dot-matrix LCD: `PC1500Display` decode, `PC1600Display` HD61203/HD61102 model; `shared::DisplayFrame`, `LcdLayout`, `StatusLine`, `LcdText` (`Decisions.md:224-236`, `Shared-Architecture.md:67`) [1600 §Display] | Machine-specific technology | **per product**; frame hand-off to frontends is design input |
| M53 | Screen copy / text readout | Printer copy only [59 §Printer] | Copy Screen (image + Unicode text), `--lcd-text`, `calcu1600/screen` (`Decisions.md:663-669`) [1600 §Display] | 1600 only | **per product** |
| M54 | Audio | "Keyboard Feedback" 0/1/2 (off, haptic, click): iOS haptics + system sound 1104, web WebAudio click (portrait/PWA), Qt none; no emulated sound (`Shared/SettingsKeys.hpp:18`, `App/Views/KeyboardView.swift:51-73`) [59 §Audio] | `PiezoSampler` follows the ROM's buzzer line, 48 kHz; `shared::AudioPump` → frontend `AudioSink`; Qt `QAudioSink`, web AudioWorklet; turbo audio dropped (`Decisions.md:82-89,767-769`) [1600 §Audio] | Host key feedback vs machine sound; no overlap today | **1600** for an audio pipeline; 59 key feedback stays product scope |

### GUI concepts and frontends

| # | Capability | Calc-U-59 | Calc-U-1600 | Divergence | Suggested base (suggestion) |
|---|---|---|---|---|---|
| M55 | Window layout | macOS and Qt: calculator \| printer \| debugger side by side; iOS pages or side by side by width; web wide or portrait (`App/Views/CalculatorView.swift:226-273`, `Qt6/app/MainWindow.cpp:27-34`) [59 §Layouts] | Calculator (faceplate + LCD) on top, control bar below, panels at the bottom (INSPECTOR, paper, CE-158) (`User-Guide.md:33-37`, `Qt6/app/MainWindow.cpp:93-97`); web panels only if ≥ 220 px remain [1600 §GUI concepts] | Horizontal columns vs vertical stack | **neither**: family-coherence ADR |
| M56 | Control bar and menus | Bar: model picker, Preset, Crd / + Crd / Eject; Qt File menu holds Open Preset, Fit Card, Settings; Help › Calc-U-59 Help (`Qt6/app/MainWindow.cpp:56-70`, `ControlBar.cpp:46-75`) [59 §GUI concepts] | Menus File / Edit / Machine / Help; control bar with model combo (duplicates Machine ▸ Model, `TODO.md:522-523`), slot pickers, disk row, cassette bay, peripheral buttons (`Qt6/app/MainWindow.cpp:861-1016`) [1600 §GUI concepts] | Different menu structure and item names | **neither**: GUI-sharing ADR |
| M57 | Help / About | About in all GUIs; Help link to `https://calc-u.org/59/` in Qt and web, pending in Swift (`reference/FrontendParity.md:82-84`) | Help ▸ About only [1600 §GUI concepts] | 59 links to the site; 1600 has no site | **59** (once `/1600/` exists) |
| M58 | Frontend parity tracking | `reference/FrontendParity.md` table per feature and GUI; `tools/check_parity.sh` flags one-GUI branches [59 §Summary, §Architecture] | No parity table or check named in the inventory; "the web mirrors the Qt window" (`README.md:34-36`) [1600 §GUI concepts] | 59 only | **59** |
| M59 | Swift frontend and bridge | SwiftUI app (macOS, iOS, iPadOS) via ObjC++ `Bridge/` "translate types only" (`CLAUDE.md:65`) [59 §Architecture] | None (three Objective-C++ helpers in the Qt app on macOS, e.g. `MacClipboardImage.mm`) [1600 §Architecture] | 59 only; Briefing §1.4 names a later Swift frontend for 1600 | **59** for the bridge pattern; Swift bridge boundary ADR |
| M60 | Qt6 frontend | Qt Widgets, C++17; ships Windows and Linux; macOS build dev-only (ODD59-24); `--grab` screenshot (`Qt6/app/main.cpp:68-77`) [59 §Architecture] | Qt Widgets/Multimedia/Network, C++20; ships macOS, Windows, Linux; DAP server here; `--shots` scenarios [1600 §Architecture, §Tests] | Different Qt module set and platform scope | **merge** for shared widgets; platform scope per product (Briefing §4.2) |
| M61 | Web shell (embedding) | `Web/dist/` self-contained, location-independent; component `PlayCalculator` (props `layout`, `width`, `keyboard`); `MANIFEST.json`; styles under `.calcu-client` (`reference/WebClient.md:113-131,260-277`) [59 §Web shell, Site §Embedding] | `Web/dist/` location-independent; component `CalcU1600` (props `width`, `height`, `keyboard`, `model`, `preset`, `workerUrl`); `files.json`, `MANIFEST.json`; `.calcu-client` (`Web-Client.md:54-90`) [1600 §Web shell, Site §Embedding] | Same contract shape; component name and props differ | **merge** |
| M62 | Web build and runtime | Vendored React 18 + Babel, no npm deps (`Web/build.sh`, `Web/build.js`); engine in a Web Worker, `Threading::Cooperative`; PWA precaches ROMs, modules, presets [59 §Architecture, §Web shell] | Vendored React 18 + Babel, "as in Calc-U-59" (`Decisions.md:824-825`); engine in one Web Worker; PWA `dist/app/` [1600 §Web shell] | Already copied from 59 | **merge** (59 is the origin) |
| M63 | Web persistence | `localStorage` (`calcu59/`), cards in an in-memory folder for the page session, download to keep (ODD59-23) [59 §Cards, §Settings] | IDBFS with `autoPersist`; Web Lock `calcu1600-storage`, one writing tab; "Saved Media…" (`Decisions.md:800-811`) [1600 §Web shell] | Session-only vs persistent | **1600** mechanism; 59 web scope stays (ODD59-23) |
| M64 | Web feature subset | Classic LED only; debugger only in wide layout (`reference/WebClient.md:21-27`) | No DAP, serial, host drive, TRACE/LOG, Quit, window size, folder settings (`Web-Client.md:105-113`) | Product-specific gaps | **per product** |
| M65 | Web publication | `calc-u.org/59/calc/` (2.3.0 at `acd629c` and live; `/59/beta/` removed) [Site §Embedding] | Nothing on calc-u.org; the developer's preview at `https://tinue.github.io/1600/` is outside this project [1600 §Web shell, Site §Embedding] | Hosted on calc-u.org vs elsewhere | **59** (site pin mechanism `web-client.json`) |

### ROMs, tests, docs, release, CI

| # | Capability | Calc-U-59 | Calc-U-1600 | Divergence | Suggested base (suggestion) |
|---|---|---|---|---|---|
| M66 | ROM handling | Committed text dumps under `roms/` (`.github/workflows/build.yml:9-10`); bundled in Xcode and Qt resources (`Qt6/CMakeLists.txt:100-116`) [59 §Compatibility, RT] | 21 ROM files fetched by `tools/fetch_roms.sh`, md5-verified, not committed; build succeeds without them, nothing boots; release packages bundle them; own host-drive ROM (`roms/README.md:1-8`) [1600 §ROMs] | Committed vs fetched; text vs binary | **1600**: already separable, closest to Briefing §8 ROM removability (design only) |
| M67 | Version source | `MARKETING_VERSION` in the Xcode project; Qt reads it by regex; web stamps it into `MANIFEST.json` (`CLAUDE.md:86-87`, `Qt6/CMakeLists.txt:36-44`) [59 §Architecture, RT] | `set(APP_VERSION "0.8.1")` in `Qt6/CMakeLists.txt:39` (ReleaseIt `cmake` preset) [RT] | Different single source | **per product**; versioning ADR |
| M68 | Unit / integration tests | `CalcU59Tests`: 150 `TEST` cases, own minimal harness (`tests/TestHarness.hpp:8-10`), CTest, real ROMs, cooperative virtual time, `FakePlatform`; 6.55 s in CI [59 §Tests] | `CoreTests`: ~788 test functions, hand-rolled runner (one `main()` in `Core/tests/lh5801_tests.cpp:1558`), CTest; 1 min 47 s in CI; `job_tests.cpp` sliced vs straight [1600 §Tests] | Two hand-rolled harnesses | **neither**: testing-concept ADR (one harness, Briefing §1.10) |
| M69 | Golden tests | `tests/golden/formats.txt`, `keynames.txt`; generator `tools/goldens/` from old Swift code (`tools/goldens/README.md:15-25`) [59 §Tests] | No golden files; assertions in code; `expect:` steps in presets; LCD text / PNG checks [1600 §Tests] | Golden files vs in-code / in-preset assertions | **merge** |
| M70 | Web GUI tests | Playwright `play-calculator.spec.js` (28), `offline.spec.js`, `prerender.spec.js`; Chromium `root`/`subpath`, offline in WebKit; 1.5 min in CI (`Web/playwright.config.js:12-18`) [59 §Tests] | Playwright `play`, `offline`, `prerender` specs (21 `test(`); Chromium root/sub-path, WebKit offline; 31 s in CI (`Web/playwright.config.js:13-19`) [1600 §Tests] | Same structure and projects | **merge** |
| M71 | Native GUI tests and screenshots | XCUITest 21 tests (local only), two test plans; `bin/run-screenshots` for App Store; Qt `--grab` [59 §Tests] | Qt `--shots` scenarios (9 `*.shots.yaml`, local, macOS images) for the User Guide; `tools/dap_smoke.py` (local) [1600 §Tests] | Swift vs Qt tooling; App Store vs docs images | **neither**: testing-concept ADR (GUI strategy, Briefing §1.10) |
| M72 | Smoke tests | — | WASM smoke `tools/wasm_smoke.cpp` in CI (44 s) [1600 §Tests] | 1600 only | **1600** |
| M73 | Headless harness | `tools/headless/`: flat C API over `TI59Machine` + `ti59.py` (ctypes); drives `Core/` directly, not the session; `tools/agentic/` with its own Python preset parser (`tools/agentic/ti59file.py:1-10`) [59 §Headless] | `pc1500_cli`, `pc1600_cli` (`--preset`, `--dump-mem`, `--lcd-text`, `--wav`, `--tape-in/-out`, …); build their own preset environment, not `AppSession` (`Shared-Architecture.md:137-138`); probes [1600 §Tests] | Python over C API vs C++ CLIs; both bypass the session; 59 duplicates the preset parser in Python | **merge**; capabilities must stay (Briefing §8) |
| M74 | One-time verification matrices | — | `dev/loader-matrix/`, `dev/tape-matrix/`, `dev/hardware-checks/` [1600 §Tests] | 1600 only | **per product** |
| M75 | CI workflow | `build.yml`, `workflow_dispatch` only; jobs `core-tests` (no Qt, `-DCALCU59_BUILD_QT=OFF`), `web` (emsdk 6.0.12, `setup-emsdk@v16`, `setup-node@v7`), `linux` ×2, `windows` ×2; no macOS job; ~3 min 53 s [RT §Calc-U-59] | `build.yml`, `workflow_dispatch` only; `core-tests` needs apt Qt6 (root CMake adds `Qt6/`), fetches ROMs and sharpdx; `web` (emsdk 6.0.13, `setup-emsdk@v14`, `setup-node@v4`); `linux` ×2, `mac-aarch64`, `windows` ×2; ~7 min 46 s [RT §Calc-U-1600] | Same job shape; tool versions, Qt dependency of `core-tests`, ROM fetch, macOS job | **merge** (59's Qt-free `core-tests`; 1600's per-job fetch for ROMs); repository/CMake/CI ADR |
| M76 | Release tooling | ReleaseIt, preset `xcode`; `xcode-dmg` local + `github-dispatch`; `appstore:` worktree; `post_release: bin/install-release.sh`; no `certs:` [RT §Calc-U-59] | ReleaseIt, preset `cmake`; `github-dispatch` only; `certs:` + `renew-certs`; `extra_assets` (`Calc-U-1600-examples.zip`) [RT §Calc-U-1600] | Different step kinds; both rely on S1–S16 single-product assumptions | **neither**: ReleaseIt needs monorepo support (RT S1–S16); versioning/release ADR |
| M77 | macOS signing | Local Developer ID signing and notarisation (`src/build/xcode_dmg.rs:1-67`) [RT] | In CI from 6 repository secrets (`build.yml:396-535`) [RT] | Local vs CI; fixed by Briefing §8 (build location stays) | **per product** (Briefing §8) |
| M78 | Release asset names | `Calc-U-59-2.3.0.dmg` (versioned), others unversioned (`Calc-U-59-linux-x86_64.AppImage`, …) [RT] | `Calc-U-1600-mac-aarch64.dmg` (arch, no version), others same pattern as 59 [RT] | DMG naming differs | **merge** |
| M79 | Bundled examples | `examples/` hand-copied to the site; web bundles 10 presets [59 §Compatibility, Site §Duplicated] | `examples/` shipped as `Calc-U-1600-examples.zip` release asset; bundled presets and cards in app resources [1600 §Other, RT] | Site download vs release zip | **neither**: site-sharing decision (Briefing §6, phase 3/4) |
| M80 | Help-site material | User guide only in calc-u-site `59/` (React JSX `Pages.jsx`, prerendered); repository supplies web zip, examples, `reference/StateFileFormat.md` (`README.md:15-21`) [59 §Help-site, Site §/59/] | User docs in the product repository: `docs/User-Guide.md` (967 lines), `Keyboard-Mapping.md`, `Debugger.md`, card definition specs; Markdown with Mermaid; generated screenshots; no site (`README.md:52-80`) [1600 §Help-site] | Site JSX outside the product vs Markdown inside the product | **neither**: how calc-u-site shares material (Briefing §6, phase 3/4) |
| M81 | Deliberate-oddity records | Spread over `reference/*.md` (29 items collected as ODD59-1…29) [59 §Oddities] | `docs/background/Decisions.md` (891 lines) + `docs/developer/PC1600-Core-Limitations.md` (444 lines) [1600 §Oddities] | One decisions file vs spread | **1600** shape (one decisions file per product; Process.md phase 3) |
| M82 | Agent knowledge | 9 skills in `.claude/skills/`, `CLAUDE.md`, in-repo `memory/`, `reference/*.md` [AK §Summary] | No skills; `CLAUDE.md` (1408 bytes), `docs/developer/` (11), `Decisions.md`, 24 plans, `SessionStart` hook [AK §Summary] | Skills vs docs | **neither**: skill-structure ADR |
| M83 | Naming | See "Naming conflicts" below [AK §Product-specific names, 59/1600 "Product-specific names"] | idem | Same names for different things, different names for the same thing | **neither**: glossary (phase 2) |

### Naming conflicts

From the inventories' "Product-specific names" entries and
`agent-knowledge.md` §"Product-specific names for shared concepts".

| Concept | Calc-U-59 name | Calc-U-1600 name | Conflict |
|---|---|---|---|
| App-level session | `SessionController` | `AppSession` | different names, same role (M10) |
| Machine-level session | `EmulatorSession` | `MachineSession` | different names, same role (Process.md phase 2 example) |
| Frontend interface | `Platform` (`cu59::SessionController`) | `Platform` (`shared::AppSession::Platform`) | same name, similar role |
| Setup file | "preset" in the GUI, "state file" for the format (deliberately distinct, memory file `state_vs_preset_terminology.md`) | "preset" (a cold-boot script) | same word, different semantics (M18) |
| "module" | Solid State Software module (ROM in the module slot) | memory module (a `.card.yaml` card in a memory slot) | same word, different things |
| "card" | magnetic card (`.u59`), cue card | memory card / software-defined card on a connector (`CardBase`) | same word, different things |
| Removable-item container | none ("fit" a card, "insert" a module) | "slot", "bay", "media" | 1600-only vocabulary |
| Fast run | "full speed" (`FullSpeed:`) | "turbo" | different names, same gesture (M12) |
| Model | `Model`, `MachineVariant`, `ModelInfo` | model (`startup/modelPreference`) | variant vs model |
| Debug panel | CALCULATOR / CPU / LOG tab | INSPECTOR | different concepts (M48) |
| Trace reader | `tools/read_trace.py` | `tools/read_trace.py` | same name, different formats (M51) |
| Settings for model at start | `startupModel`, `lastUsedModel` | `startup/modelPreference`, `startup/lastUsedModel` | near-identical |
| Keyboard map | `KeyboardMap`, `KeySequencer` | `HostKeyMap`, `HostKey`, `PC1500KeyboardMap` (serves all models) | different names |

## Contradictions

Points where the two products made opposite choices. Each names the ADR
candidate from `docs/Process.md` phase 3 that it feeds.

| # | Contradiction | Calc-U-59 | Calc-U-1600 | ADR candidate (Process.md phase 3) |
|---|---|---|---|---|
| C1 | Threads vs coroutine jobs | Threaded runner with a cooperative mode (M11) | Single-threaded, coroutine jobs, explicitly "no threaded mode, unlike Calc-U-59" (`Decisions.md:734-738`) | Threading model (with the Swift/iOS frontend in mind) |
| C2 | Machine interface | One facade `TI59Machine` | No interface; two typed pointers and per-machine branches (M3) | The core seam |
| C3 | Explicit vs implicit interfaces | Printer connector, module slot, card reader implicit inside `TMC0501` (M4, M5) | Pin-numbered connectors, peripherals as cards (M5, M6) | Modelling depth and explicit interfaces; Devices keyed by connector type or by machine |
| C4 | Module loading | Settings, persisted (`solidStateModuleID`), always one module (M35, M28) | Control-bar drop-down per slot, never saved (`Decisions.md:650-652`) | GUI sharing and family coherence |
| C5 | Format versioning | Presets, `ti58c.mem`, ASM overlay, settings unversioned; v1 cards written back as v1 (M31) | Every read format has a required `format-version`; upgrade on save from 1.0 (`Decisions.md:671-685`) | Preset format, versioning and the Calc-U-59 read-old/write-new upgrade |
| C6 | What a preset is | Applied to a running calculator; absent module/printer kept (M18) | Builds a machine from a cold boot (M18) | Preset format, versioning and upgrade (plus glossary, phase 2) |
| C7 | Preset error policy | Tolerant: bad line reported, load continues (ODD59-7) | All-or-nothing at parse time (`Core/Preset/PresetFile.hpp:265-269`) (M21) | Preset format, versioning and upgrade |
| C8 | Window layout | Columns side by side (M55) | Vertical stack with control bar and bottom panels (M55); partly hardware-driven and lasting: the TI-59 is a portrait calculator, the PC-1600 landscape (user, 2026-11-09) | GUI sharing and family coherence |
| C9 | Debugger concept | In-app stepping debugger in every GUI; breakpoints only headless (M48, M49) | External DAP debugger (VS Code) plus read-only INSPECTOR (M48, M49) | GUI sharing and family coherence (debugger contract in phase 4) |
| C10 | Settings scope | Settings hold hardware choices (M28) | Settings hold app preferences only (M28) | GUI sharing and family coherence |
| C11 | Key hold unit | ≥ 100 ms wall time (ODD59-4) | ≥ 2 emulated frames (`Decisions.md:585-595`) (M14) | none listed; keyboard / typing-strategy contract (phase 4) |
| C12 | Debug model location | `Shared/` (M50) | `Core/Debug/` (M50) | The core seam (debugger contract, phase 4) |
| C13 | ROMs in the repository | Committed text dumps (M66) | Not committed, fetched and md5-checked (M66) | History archive, product naming, ROM removability |
| C14 | CI `core-tests` | Configured without Qt (M75) | Needs Qt because root CMake adds `Qt6/` (M75) | Repository layout, CMake structure, CI |
| C15 | C++ standard | C++17 | C++20 (M1) | none listed in Process.md; Briefing §8 requires an ADR for every raise |
| C16 | Help material home | Site JSX in calc-u-site (M80) | Markdown in the product repository (M80) | none listed; Briefing §6 "How calc-u-site shares common material" (phase 3/4) |
| C17 | Web persistence | Page-session cards, `localStorage` settings (M63) | IDBFS, Web Lock, persistent saved media (M63) | GUI sharing and family coherence (settings and paths contract, phase 4) |
| C18 | Test harness | Own minimal harness (M68) | Separate hand-rolled runner (M68) | Testing concept |

## Already shared or near-identical

| Item | Evidence |
|---|---|
| Layering Core / Shared / frontends; "frontends only translate types"; Shared decides no file locations | M1, M2 (59 `CLAUDE.md:60-65`; 1600 `Shared-Architecture.md:33-47`) |
| Session object plus a frontend-implemented `Platform` interface | M10 |
| Pacing on absolute deadlines, lag ≈ 100 ms dropped, holding the display runs fast | M12 (59 `Shared/Pacing.hpp:9-33`; 1600 `Shared/Pacer.hpp:9-29`) |
| Host keys resolved on the produced character | M13 (59 ODD59-2; 1600 `Shared/HostKeyMap.hpp:1-30`) |
| Three models per product; model switch = fresh machine | M7 |
| Two-level reset (Reset / clean reset; Reset / Reset All) | M8 |
| One preset extension per model; presets are read, never written by the app | M22, M24 |
| Startup-model settings (`startupModel`/`lastUsedModel` vs `startup/modelPreference`/`startup/lastUsedModel`) | M28 |
| Trace size limit setting; versioned binary CPU trace; Python reader `tools/read_trace.py` | M51 |
| Debounced (~0.5 s) autosave of persistent machine memory | M34, M39 |
| Paper panel with Copy (high-resolution image) and Cut | M46 |
| Drag and drop of files onto the window (both products, except 59 iOS) | M44 |
| Web client: vendored React 18 + Babel, no npm deps, engine in a Web Worker, location-independent `dist/`, installable offline PWA, `MANIFEST.json`, styles under `.calcu-client` | M61, M62 (1600 `Decisions.md:824-825` "as in Calc-U-59"; Site §Embedding) |
| Playwright suites with the same three specs (play, offline, prerender) and projects (Chromium root/sub-path, WebKit offline); `Web/run-tests.sh` | M70 |
| CTest-driven C++ tests with real ROMs on Ubuntu 24.04 in CI | M68, M75 |
| `build.yml` dispatched only (`workflow_dispatch`), same job set (`core-tests`, `web`, Linux ×2 via linuxdeploy AppImage, Windows ×2 via Qt 6.8.2 + Inno Setup, unsigned) | M75 (RT) |
| ReleaseIt cycle (`-pre` tag, `--rc`, `--release`, `--next`), `github-dispatch` step, asset names `<Product>-linux-<arch>.AppImage`, `<Product>-windows-<arch>.exe`, `<Product>-web.zip` | M76, M78 (RT) |
| Release notes suffix for Windows (unsigned) / Linux (`chmod +x`) with the same wording | Site §Duplicated (59 `release.yaml:31-35`, 1600 `release.yaml:19-23`) |
| Settings in QSettings (org/app = product name) and `localStorage` with a `calcu<n>/` prefix | M27 |
| `release.yaml` at the repository root, `dev-x.y.z` branches, `vX.Y.Z` tags, one CHANGELOG | RT §Single-product assumptions |
| Help topics covered by both products' docs: presets, debugger, Windows/Linux install, dropping files, PWA install, keyboard, settings, removable items | Site §Duplicated (topic table) |

## Findings against the briefing

Places where an inventory contradicts or qualifies `Briefing.md` (or
`docs/Process.md`). Facts only; resolution belongs to the phase named in the
briefing or to the user.

| # | Briefing | What the briefing says | What the inventory found | Source |
|---|---|---|---|---|
| F1 | §1.2, §6, §7 | "Old format" = every explicitly versioned format the released Calc-U-59 reads; unversioned legacy formats are dropped; success criterion: Calc-U-59 reads all existing files in a versioned format | The preset format has no version marker; every bundled preset (10 web presets) and every hosted preset (39 in `59/software-files/`, 6 in `59/tutorial-files/`, plus two 10-file copies in `59/calc/presets/` and `59/beta/presets/`) is unversioned. Applied literally, §1.2 drops all of them. `ideas/PresetConvergence.md:55-58` proposes an optional `FORMAT-VERSION: 2`. **Resolved 2026-11-08:** presets as 2.3.0 reads them are version 1 (Briefing §1.2) | 59 §Compatibility, §Open questions; Site §Downloadable files |
| F2 | §1.2 | "Anything it writes uses the new format" | A version 1 card (text 1.0 or binary) is written back as text 1.0 (`Shared/CardFile.cpp:293-294`, ODD59-12). **Open, phase 3** (Briefing §6) | 59 §Compatibility |
| F3 | §1.2 | Unversioned legacy formats are no longer read | `ti58c.mem` is unversioned and lives on users' devices (iCloud, `~/Calc-U-59`, `localStorage`); dropping it loses TI-58C memory on upgrade. Settings stores and ASM overlay files are also unversioned. **Resolved 2026-11-08/09:** `ti58c.mem` and ASM overlays are version 1; settings become a settings file (Briefing §1.2) | 59 §Compatibility, §Open questions |
| F4 | §1.2 | Formats tagged own or external; "SDE exchange formats" are external | `.floppy.yaml` is tagged own but is also read and written by SDE; ASM overlay files wrap third-party listings (Sladký) in an own wrapper. The own/external line is ambiguous for both. **Resolved 2026-11-09:** both own; `.floppy.yaml` spec owned by Calc-U, SDE follows; ASM overlays version 1 (Briefing §1.2) | 1600 §Formats; 59 §Open questions |
| F5 | §1.9 | `Decoder.asm` does not run because the TI-59 chips are not modelled individually | Two of three causes are chip modelling (single digit counter, keyboard rows on the same counter); the third is in the shared layer: `Shared/SevenSegment.hpp:43` maps hex digits A–F to 0–5. **Resolved 2026-11-08:** Briefing §1.9 names the further cause | 59 §Why Decoder.asm does not run |
| F6 | §1.9 | PC-100C: 12 contact springs, one unconnected; calculator 11 pads | The PC-100A service manual calls the connector "14-contact" (two power) and lists 12 signal contacts incl. `I/A` (`ti59-research/PC100C/service_manual.txt:2-45`); which contact is unconnected is not stated. **Resolved 2026-11-09:** the briefing is right (counted by hand on the devices); "14-contact" is a typo in the manual | 59 §Interfaces, §Open questions |
| F7 | §1.9 | 8-pin slot for the Solid State Software module | The slot pin-out was not found in `ti59-research` or the product. **Resolved 2026-11-09:** source https://www.datamath.org/Chips/TMC0540.htm (Briefing §1.9) | 59 §Interfaces, §Open questions |
| F8 | §4.2 (table) | Calc-U-59 Qt6 Linux: "Qt6 (6.8.2 in CI)" | Linux CI uses Ubuntu 24.04 packages, which installed Qt 6.4.2 (CI run `42388503215`); only Windows uses 6.8.2. Calc-U-1600 Linux uses the same apt route and macOS an unpinned Homebrew Qt (*unverified* version). **Resolved 2026-11-08:** Briefing §4.2 gives 6.4.2 for Linux | 59 §Tests; 1600 §Architecture, §Open questions; RT |
| F9 | §4.2, §8 | Calc-U-59: Swift macOS app built locally, "everything else from GitHub Actions"; §8 "everything else on GitHub Actions" | The iOS/iPadOS app is also built locally (`release ios`: archive, export, upload to TestFlight). **Resolved 2026-11-08:** Briefing §4.2 and §8 name the local iOS build | RT §App Store, §What is built where |
| F10 | §4.2 | Calc-U-1600 web client: "Released, not yet published" | Published as a preview at `https://tinue.github.io/1600/` (private repository `tinue/tinue.github.io`), from a `dev-0.8.0` build (tree `88f45ff`), not the `v0.8.0` tree. **Resolved 2026-11-09:** the preview is kept outside this project; briefing §4.2 updated | 1600 §Web shell; Site §Calc-U-1600 |
| F11 | §4.2, §4.3, §7 | Calc-U-59 web client "embedded by calc-u.org"; the site must reflect a released product | `59/calc/` embeds 2.2.0 (pin and live), although 2.3.0 is released; 2.3.0 exists only as the `-pre` preview in `59/beta/`. **Resolved 2026-11-09:** calc-u-site `acd629c` pins 2.3.0, `/59/beta/` removed | Site §Summary, §Embedding |
| F12 | §4.2 | Calc-U-1600 frontend "Web (WASM)" vs Calc-U-59 "Web (React + WASM)" | Calc-U-1600's web client is React + WASM too (vendored React 18). **Resolved 2026-11-08:** Briefing §4.2 says React + WASM | 1600 §Web shell |
| F13 | §4.2 | Calc-U-1600 "pulls SDE release artifacts into its CI builds" | True for Linux and Windows only; the macOS and WASM libraries are committed binaries; it is a release-asset download, not an Actions artifact pull. **Resolved 2026-11-08:** Briefing §4.2 states it | RT §SDE artifact pull |
| F14 | §4.4 | `Calc-U-1600/TODO.md` (`dev-0.8.0`): 728 lines | 732 lines at `v0.8.0` and `b4ea8c8`. **Resolved:** the briefing says 732 | 1600 §Open questions |
| F15 | §5.1 | TI knowledge "spread over several smaller skills" | No TI research skill exists outside Calc-U-59; the "smaller skills" are the Calc-U-59 product skills (chiefly `calc-u-59-core`, `calc-u-59-statefiles`). **Resolved 2026-11-08:** Briefing §5.1 states it | AK §Summary |
| F16 | §1.3 | "Code duplicated between [the GUIs] moves to the shared layer (for example the preset loader)" | In both products the preset parser already lives in one place (59 `Shared/StateFile.*`; 1600 `Core/Preset/`); the duplicate found is 59's Python parser in `tools/agentic/ti59file.py`. Between the products the two preset formats share no code. **Resolved 2026-11-09:** the example stands; it means one preset loader for both products instead of one per product | 59 §Presets, §Headless; 1600 §Presets |
| F17 | Process.md phase 2 | Example conflict `EmulatorSession` vs `MachineSession` | The pairs are `SessionController` ↔ `AppSession` and `EmulatorSession` ↔ `MachineSession`. **Resolved 2026-11-08:** `docs/Process.md` phase 2 names both pairs | AK §Product-specific names |

## Open questions

Collected from all inventories and deduplicated. Tags: `[59]`, `[1600]`,
`[RT]`, `[Site]`, `[AK]`, `[matrix]` (raised while writing this file).

### Formats and compatibility
- ~~Preset syntax (F1), `ti58c.mem` (F3), ASM overlays (F4), settings~~:
  answered 2026-11-08/09, Briefing §1.2. `[59]`
- Card v1 write-back as 1.0 vs "write new" (F2): recorded as fact, decision in phase 3. `[59]`
- ~~`.floppy.img` still around?~~ No; dropped, no converter. ~~`.floppy.yaml`
  own or external?~~ Own, SDE follows (Briefing §1.2). `[1600]`

### Hardware facts
- ~~Module slot pin-out source (F7)~~: https://www.datamath.org/Chips/TMC0540.htm. `[59]`
- Which PC-100C spring is unconnected (F6; the count is settled). `[59]`
- Research sources disagree on the TI-59 chip set (TMC0599 vs TMC0598 RAM,
  TMC0571 as SCOM vs ROM, 240 kHz vs 455 kHz) and on the display decode
  table; which is authoritative for the phase 4 fit tables? `[59]`
  Answered 2026-11-09: https://www.datamath.org/Chips/TMS0500-CS.htm;
  research notes and code are corrected against it (Briefing §1.9).

### Tests and CI
- XCUITest and goldens-generation elapsed times: unknown. `[59]`
- Local elapsed time of `tools/run_tests.sh`, the screenshot scenarios and
  `dap_smoke.py`: unknown. `[1600]`
- ~~Qt version in Calc-U-1600 CI (F8)~~: Linux 6.4.2, macOS Homebrew 6.12.0
  (unpinned), Windows 6.8.2 (run `42369839986` logs; Briefing §4.2). `[1600]`
- Do the Calc-U-1600 screenshot scenarios run on Linux/Windows? *unverified*. `[1600]`
- ~~Do VS Code extension tests exist?~~ No: no test files under `vscode/`,
  no `scripts` in `package.json`; `check_presets.sh:8-9` says templates are
  checked "once filled in" (checked at `b4ea8c8`). `[1600]`
- ~~Build on tag push?~~ No; CI stays dispatched by ReleaseIt. `[RT]`

### Release and repository
- Release location and versioning model: open, phase 3 (Briefing §6:
  source vs release repository, independent product releases). `[RT]`
- ~~`release -C <subdir>` in a monorepo?~~ Yes: per-product subdirectories
  (Briefing §6). `[RT]`
- Role of `bin/run-screenshots` / `bin/simulators` in the App Store
  pipeline: covered in `59` §Tests and §Release material, not in `RT`;
  no conflict. `[RT]` (answered by `[59]`)
- ~~`TODO.md` 728 vs 732 lines (F14)~~: the briefing now says 732. `[1600]`

### Site
- ~~calcu59.ch redirect~~: hosting provider, kept while the domain is paid;
  never used or mentioned in the new project (Briefing §4.3). `[Site]`
- ~~`59/calc/` pin (F11)~~: done in calc-u-site `acd629c`. `[Site]`
- ~~Is `https://tinue.github.io/1600/` part of the release material (F10)?~~
  Answered 2026-11-09: kept by the developer, not part of this project;
  the web client goes to calc-u.org/1600. `[Site]`
- ~~`/1600/` content outline (`rustling-napping-pelican.md`)~~: superseded,
  discarded (answered 2026-11-09). `[Site]`
- ~~Number of `check.js` assertions~~: 1302 checks, all passing, at
  `acd629c` (run 2026-11-09, node 26.13.0); both "400-odd" and "1100+" are
  stale. `[Site]`

### Agent knowledge
- ~~Is `calc-u-1403h-ui` in scope?~~ Answered 2026-11-09: no, out of scope
  for now, as is Calc-U-16 (Briefing §1.11). `[AK]`
- ~~Repo skill, plans, stale memory directories~~: answered 2026-11-09
  (Briefing §1.11): the repo skill is a stand-alone subset, both it and
  `PC1500` are updated; plans keep their wording; stale directories are
  merged and deleted. `[AK]`
- ~~Do the `MEMORY.md` indexes exceed the load limit?~~ No: Claude Code
  loads the first 200 lines or 25 KB (https://code.claude.com/docs/en/memory);
  the largest index, Calc-U-1600's, has 64 lines and 15 260 bytes (2026-11-09).

### Raised by this matrix
- ~~1600 background behaviour (M17), preset error policy (M21, C7), 59
  `traceMaxFileSizeMB` default (M51), 1600 empty slot (M35)~~: checked in
  the code 2026-11-09, see the rows. `[matrix]`
- ~~Inconsistencies inside single inventories~~: recounted 2026-11-09 and
  corrected: the Calc-U-1600 memory directory has 59 files; calc-u-site hosts
  49 program files (43 in `59/software-files/`, 6 in `59/tutorial-files/`)
  plus 2 `.md`. `[matrix]`
- Other drift between the products' own docs and ReleaseIt examples
  (`examples/Calc-U-59.yaml`, `examples/Calc-U-1600.yaml` older than the
  real `release.yaml` files) is recorded in `RT`; they are rewritten when
  ReleaseIt gets per-product subdirectories (answered 2026-11-09). `[RT]`
