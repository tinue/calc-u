# Inventory: Calc-U-1600

Pinned: `v0.8.0` @ `d976d3f` (= `dev-0.8.0` @ `70c3258`). Read on 2026-11-08.
Read with `git archive v0.8.0` into a scratch directory (identical to
`git show v0.8.0:<path>`); the working tree was not touched. All `path:line`
references are at that tag. Research facts from `Sharp1500-1600-Ref` @
`43743ed` (cited as `Ref/<path>`). CI timings from GitHub Actions run
`42369839986` (2026-11-07, head SHA `70c3258`, the pinned tree).

## Summary

- One product, three emulated models: PC-1500, PC-1500A, PC-1600
  (`README.md:3-4`). Each runs its original ROM (`README.md:26-28`).
- Three layers: `Core/` (hardware), `Shared/` (app logic without GUI),
  frontends `Qt6/app/` (desktop) and `Web/` (WASM + React)
  (`CLAUDE.md:16`, `docs/developer/Shared-Architecture.md:7-25`). C++20,
  standard library only, single-threaded; long machine work is C++20
  coroutine jobs (`Core/Job.hpp`).
- There is no common machine base class: `shared::MachineSession` owns a
  `std::unique_ptr<PC1500Machine>` and a `std::unique_ptr<PC1600Machine>`
  side by side (`Shared/MachineSession.hpp:379-380`) and branches per
  machine (89 references in `Shared/MachineSession.cpp`).
- Peripherals are modelled as cards on explicit, pin-numbered connectors:
  40-pin memory slot (`PinState`), 60-pin system bus (`SystemBusPins`)
  (`TODO.md:185-192`, `docs/background/Decisions.md:497-522`).
- Own formats (preset, card, floppy) all carry a required
  `format-version: 1`; the trace file carries binary version 3. Settings,
  `bundled.yaml` and screenshot scenarios are unversioned.
- No machine state file (save state) exists; persistence is per medium
  (card, floppy, tape WAV) plus settings.
- Tests: one `CoreTests` binary (Core + Shared, hand-rolled runner, ~788
  test functions), Playwright tests for the web client, a WASM smoke test,
  a DAP smoke script, scripted Qt screenshot scenarios, two dev-only
  matrices. CI runs CoreTests, WASM smoke and Playwright; not the Qt GUI.

## Architecture overview

| Layer | Directory | Language | Built by | Notes |
|---|---|---|---|---|
| Core | `Core/` (CPU, PC1500, PC1600, Connector, Debug, Display, Preset, Tape, Serial, Audio, Basic, Resources) | C++20 | `cmake/CalcU.cmake` → static lib `calcu_core` | No Qt/Cocoa/Emscripten headers (`docs/developer/Shared-Architecture.md:36-38`) |
| Shared | `Shared/` (+ `Shared/KeyLayout/`) | C++20 | `cmake/CalcU.cmake` → `calcu_shared` | "everything above Core that is not drawing" (`Shared-Architecture.md:20-23`) |
| Qt6 frontend | `Qt6/app/` (+ `debug/`, `screenshots/`), resources `Qt6/resources/` | C++20, Qt6 Widgets/Multimedia/Network; 3 Objective-C++ files on macOS | `Qt6/CMakeLists.txt` (release entry point, `Qt6/CMakeLists.txt:31`) | DAP server lives here (`Qt6/app/debug/`) |
| Web frontend | `Web/` (`bindings.cpp`, worker, `src/*.jsx`, `app/` PWA) | C++ (embind) + JS/JSX, vendored React 18 + Babel | `Web/build.sh` (emcmake target `calcu_web`) + `Web/build.js` | `docs/developer/Web-Client.md:29-60` |
| Dev/IDE build | root `CMakeLists.txt` | CMake ≥ 3.21 | CLion, CI `core-tests` | Adds CLIs, probes, `CoreTests` (`docs/developer/Building.md:156-194`) |
| Tools | `tools/` | C++, sh, Python | `tools/build_*.sh` (list Core sources by hand, `TODO.md:398-399`) | Headless CLIs, probes, fetch scripts |
| VS Code extension | `vscode/calcu1600-debug/` | JavaScript | `tools/install_vscode_extension.sh` | version 0.2.0 (`vscode/calcu1600-debug/package.json:5`) |
| Own firmware | `firmware/pc1600-hostdrive/` | Z-80 asm | `tools/build_hostdrive_rom.sh` | Host-drive ROM, bank 7 (`Core/Resources/BundledRomCatalog.hpp:207-212`) |

- C++ standard: C++20 (`CMakeLists.txt:33`, `Qt6/CMakeLists.txt:16`).
  macOS deployment target 15.8 (`CMakeLists.txt:25`, `Qt6/CMakeLists.txt:6`).
- Qt in CI: Ubuntu 24.04 apt packages (`qt6-base-dev`, `qt6-multimedia-dev`)
  for Linux (`.github/workflows/build.yml:54-60,151-157`), Homebrew `qt` on
  macOS (`build.yml:404`), Qt 6.8.2 via install-qt-action on Windows
  (`build.yml:545,622`). Docs require Qt ≥ 6.3 (`Building.md:19-22`).
- Emscripten 6.0.13 in CI (`build.yml:104-106`); must match the committed
  `libsharpdx-wasm.a` (`Core/Basic/vendor/sharpdx/README.md:52-54`).
- Layer rules: non-GUI logic goes in Shared; frontends only translate types
  (`Shared-Architecture.md:33-35`); Shared never decides file locations,
  the frontend passes `shared::HostDirs` and a `shared::SettingsStore`
  (`Shared-Architecture.md:44-47`, `Shared/AppPaths.hpp:17`); user messages
  are English sentences, no translations (`Shared-Architecture.md:48-49`).
- How a frontend drives it: implement `shared::AppSession::Platform`, build
  `Settings`/`AppPaths`/`AppSession`, `attachAudio()`, call `tick()` at
  ~60 Hz, start operations with `start(op, done)` or `runNow(op, pump)`
  (`Shared-Architecture.md:70-101`).
- Not in Shared yet: the DAP debugger (written against QJson), and the
  CLIs' preset environment (`Shared-Architecture.md:133-140`).

### Both machines and how they share code

| Shared piece | Used by | Source |
|---|---|---|
| `LH5801` CPU core | PC-1500 CPU; base class of `LH5803` | `Core/CPU/LH5803/LH5803.hpp:20` |
| Connector/card layer (`CardBase`, `ExpansionCard`, `SystemBusCard`, `SystemBus`, `SoftwareDefinedCard`, `MemoryCardDefinition`) | both | `Core/Connector/`, `docs/developer/PC1600-Core-Limitations.md:8-12` |
| `Ce150Card`, `Ce158Card` (incl. on the PC-1600 LH5803 side, MODE 1) | both | `Core/PC1600/PC1600Machine.hpp:545-546` |
| `AlpsPlotterMechanism` (CE-150 and CE-1600P) | both | `Core/Connector/Ce150Card.hpp:8`, `CE1600PCard.hpp:8` |
| `TapeDeck`, `PiezoSampler`, `StatusLine`, `LcdText` | both | `Core/PC1500/PC1500Machine.hpp:307`, `Core/PC1600/PC1600Machine.hpp:540`, `PC1600-Core-Limitations.md:281-285` |
| `PC1500TraceFile` (one trace format, both CPUs' frames) | both | `Core/PC1500/PC1500TraceFile.hpp:7-27`, `PC1600Machine.hpp:550` |
| Debugger: `DebugTarget`, `MachineDebugTarget<Machine>` template | both | `Core/Debug/MachineDebugTargets.hpp:21-48` |
| Preset parser `PresetFile`; per-machine loaders `PC1500PresetLoader`, `PC1600PresetLoader` | parser shared, loaders per machine | `Core/Preset/`, `Core/PC1500/`, `Core/PC1600/` |
| `MachineSession` | both, via two typed pointers, no interface | `Shared/MachineSession.hpp:379-380` |

## Capabilities

### Keyboard
- How it works today: frontend turns its key event into `shared::HostKey`
  + modifiers + typed character; `shared::resolveHostKey` maps it to a
  calculator key name (`Shared/HostKeyMap.hpp:1-30`). How a key is typed is
  `MachineSession::hostKeyDown()`'s decision: PC-1600 accents through the
  live queue (KBII sequences), shifted keys as Shift sequences, physical
  keys as real presses; PC-1500 cursor keys real presses, the rest through
  the keystroke queue (`Decisions.md:571-576`). Every key held at least 2
  emulated frames (`Decisions.md:577-587`). Faceplate hit regions per model
  in `Shared/KeyLayout/` (`PC1500KeyLayout`, `PC1600KeyLayout`,
  `FaceplateLayout`). Paste Text types the first clipboard line, never
  ENTER (`Decisions.md:597-599`, `docs/User-Guide.md:155-158`). Host
  characters outside ASCII type nothing except PC-1600 KBII accents
  (`Decisions.md:748-749`).
- Where it lives: `Shared/HostKeyMap.*`, `Shared/KeyLayout/`,
  `Core/KeyPaste.*`, `Core/SharpShiftedSymbols.hpp`, `Core/PC1500/PC1500TypedInput.hpp`,
  `Core/PC1600/PC1600TypedInput.hpp`; Qt adapter `Qt6/app/PC1500KeyboardMap.*`
  (name is historic: serves all models, `Qt6/app/PC1500KeyboardMap.hpp:8-10`).
- Frontends: Qt6 (key events, IME for dead keys, `Decisions.md:617-621`);
  Web (`KeyboardEvent.code/.key`, bare Shift tap within 400 ms = SHIFT,
  touch takes nearest key within 24 px; `Web-Client.md:117-124`,
  `Decisions.md:593-596`). The web keeps no hold of its own
  (`Decisions.md:808-812`).
- Maturity: complete for the documented keys; PC-1600 keys `ctrl`, `kbii`,
  `bs` cannot be pressed from a preset (`User-Guide.md:851-852`). Full table
  `docs/Keyboard-Mapping.md` (137 lines).
- Product-specific names: `HostKey`, `hostKeyDown`, `KeyPasteFeeder`, "Paste Text", KBII.

### Presets
- How it works today: a preset is a script that builds a machine from a
  cold boot (model, ROM, modules, peripherals) then runs `keys:` and
  `program:` blocks (`User-Guide.md:625-645`). "YAML-shaped, not YAML": a
  hand-written parser; only `debug:` and `bus-rom:` are YAML proper
  (`Decisions.md:381-386`). Required `format-version: 1` and `model:`
  (`User-Guide.md:659-661`, `Core/Preset/PresetFile.hpp:265`). Steps:
  `key:`, `type:`, `wait:`, `expect:`, `saveas:`, `screenshot:`,
  `syncclock:`, `trace:` (`User-Guide.md:841`). Extension matches model
  (`.pc1500`, `.pc1500a`, `.pc1600`) but parser goes by `model:`
  (`Decisions.md:409-411`). Bundled presets listed in
  `examples/bundled.yaml` in two lists, `defaults` (Settings ▸ Default
  presets) and `examples` (File ▸ Examples) (`examples/bundled.yaml:1-10`,
  `Decisions.md:425-427`). Settings stores a bundled preset as
  `examples:<file>` (`Decisions.md:428-430`). Default preset per model
  applies on app start and model switch, not on reset or ROM change
  (`User-Guide.md:788-802`).
- Where it lives: `Core/Preset/` (`PresetFile`, `PresetRunner`,
  `PresetMedia`, `PresetBusRomLoader`, `PresetDebugBlock`),
  `Core/PC1500/PC1500PresetLoader.*`, `Core/PC1600/PC1600PresetLoader.*`,
  `Shared/PresetCatalog.*`.
- Frontends: Qt6 (File ▸ Load Preset…, File ▸ Examples submenu grouped,
  `Qt6/app/MainWindow.cpp:843-857`; double-click/Open With on macOS,
  `User-Guide.md:185-188`); Web (File ▸ Load Preset submenu with groups +
  "Upload Preset…", `Web/src/CalcU1600.jsx:240-251,263`; `?preset=` URL
  parameter, `README.md:46-48`); CLIs (`--preset`, `--check-preset`,
  `tools/pc1600_cli.cpp:7-10`). CLIs build their own preset environment,
  not `AppSession` (`Shared-Architecture.md:137-138`).
- Maturity: complete.
- Product-specific names: preset ("builds a machine from a cold boot" — a
  script, not a stored state), `preset-name:`, `defaults` / `examples`,
  `saveas: live|template`, project preset (debugger).

### Debugger (including DAP)
- How it works today: DAP server on 127.0.0.1 (default port 32168, one
  client at a time), enabled in Settings ▸ Debugger or `--dap <port>`
  (`docs/developer/Debugger-Internals.md:10-12`, `Shared/Settings.hpp`
  `debug/dapEnabled`, `debug/dapPort`). One thread per CPU (LH5801; Z80
  (SC7852) and LH5803 on the PC-1600); frames 1–20 are history; breakpoints
  with conditions/hit counts/log messages, data breakpoints, listings
  (sdas `.lst`/`.rst`, zasm `.lst`, `.SYMBOLS:`), bank qualifiers
  (`docs/Debugger.md:316-358`). Snapshots are pre-execution everywhere
  (`Decisions.md:858-863`). Custom requests `calcu1600/screen`,
  `calcu1600/quit` (`tools/dap_smoke.py:1-13`, `Debugger-Internals.md:70-71`).
  VS Code extension carries configurations, builds (zasm / sdas) and
  default presets (`Decisions.md:828-831`). Project settings live in the
  preset's `debug:` block (`Decisions.md:832-836`). Separate from the DAP
  debugger: the INSPECTOR panel (views built in Core,
  `Decisions.md:628-634`; `User-Guide.md:889-941`).
- Where it lives: `Core/CPU/` (history, breakpoints, watches),
  `Core/Debug/` (`DebugTarget`, `CpuViews`, `RunControl`, `SourceMap`,
  `Listing/`, `Disasm/`, `Inspect/`, `ProgramLoader`), `Qt6/app/debug/`
  (`DapServer`, `DapSession` 1113 lines, `DebugController`),
  `vscode/calcu1600-debug/` (`Debugger-Internals.md:24-50`).
- Frontends: Qt6 only. Not in the web client (`Web-Client.md:109`).
  CLion recipe documented but untried (`docs/Debugger.md:1`, `TODO.md:456`).
- Maturity: complete for VS Code (automated side passes; manual checks
  open, `TODO.md:446-456`). No BASIC-level debugger (`TODO.md:457-461`).
  Limitations: no backward stepping, no TASM listings, vertical banks not a
  qualifier (`Debugger.md:390-397`).
- Product-specific names: `DebugTarget`, `DebugController`, `runSlice()`,
  INSPECTOR, project preset, `boot: debug`, Build & Load.

### Settings
- How it works today: every key with its default in `shared::Settings`,
  storage behind `shared::SettingsStore` (`Shared/Settings.hpp:10-17`).
  Keys: `storage/instanceDirOverride`, `storage/tapeDirOverride`,
  `<group>openDir`, `<group>lastOpenDir`, `preset/default/<model>`,
  `trace/dirOverride`, `trace/maxFileSizeMB`, `debug/dapEnabled`,
  `debug/dapPort`, `serial/linkDirectory`, `window/width|height`,
  `startup/modelPreference`, `startup/lastUsedModel`
  (`Shared/Settings.hpp:34-121`). Values stored as strings
  (`Decisions.md:767-768`). Hardware picks (model, modules) are on the
  control bar and not saved; Settings holds app preferences only
  (`Decisions.md:642-644`). Dialog has only Close; changes apply
  immediately (`User-Guide.md:864-886`).
- Where it lives: `Shared/Settings.*`, `Shared/AppPaths.*`; Qt store
  `Qt6/app/AppSettings.hpp:25-46` (QSettings, org and app "Calc-U-1600";
  an isolated INI for screenshot runs); dialog `Qt6/app/SettingsDialog.cpp`.
- Frontends: Qt6 (Calc-U-1600 ▸ Settings… on macOS, Edit ▸ Settings…
  elsewhere, `User-Guide.md:864-865`); Web (`localStorage`, keys prefixed
  `calcu1600/`, `Web-Client.md:93-94`; Edit ▸ Settings…,
  `Web/src/CalcU1600.jsx:276`; no folder settings, `Web-Client.md:113`).
- Maturity: complete.
- Product-specific names: "Battery-card saves" (save folder), "Startup device".

### Media and files
Overview: removable media are handled by bays in Shared — `ModuleBay`,
`FloppyBay`, `TapeBay` — with catalogues, attach, Name & Save, preset
`saveas:` and debounced autosave (`Shared-Architecture.md:57`). Bundled
items are templates, never written; Name & Save makes an instance that
autosaves about 0.5 s after a change (`User-Guide.md:442-471`). Cards and
floppies resolve bundled-first, then the save folder (`Decisions.md:680-681`).
Media pickers cache by file stamps (`Shared/DirectoryScanCache.hpp`,
`Decisions.md:786-789`). `shared::MediaLibrary` lists/exports/imports/deletes
the save folder's cards, disks, tapes (web "Saved Media…",
`Shared-Architecture.md:64`).

#### Memory modules (cards)
- How it works today: every module is a `.card.yaml` (software-defined
  card: pins that enable it, memory regions, banking, optional
  `initial-content`) (`User-Guide.md:943-950`,
  `docs/Memory-Card-Definition-Spec.md`, `docs/Memory-Card-Definition-Format.md`).
  Bundled: CE-151, CE-155, CE-161, CE-1638, CE-163F, CE-502B, CE-1600M,
  CE-1601M, superRAM (`User-Guide.md:392-402`, `Qt6/resources/cards/`).
  PC-1500/1500A one slot, PC-1600 two slots (`User-Guide.md:378-379`).
  Choosing a module restarts the machine (`User-Guide.md:389-390`). A card
  never points at another file (`Decisions.md:841-846`). v1 scope:
  Regular/Flash/ROM content, unbanked or trigger-banked latches;
  `line-based` latches rejected (`PC1600-Core-Limitations.md:381-384`).
- Where it lives: `Core/Connector/MemoryCardDefinition.hpp` (1449 lines),
  `SoftwareDefinedCard.hpp`, `BatteryCardInstance.hpp`, `MemoryCardCatalog.hpp`,
  `MediaSave.hpp`, `NamedFileCatalog.hpp`; `Shared/ModuleBay.*`,
  `Shared/CardYamlDump.*`.
- Frontends: Qt6 control-bar picker per slot (`Qt6/app/ControlBar.hpp:12,75`,
  `Qt6/app/MemoryModuleManager.*`, `NamedComboBox`); Web control bar
  (`Web/src/ControlBar.jsx`, own `NamedCombo`, `Decisions.md:431-434`).
- Maturity: complete within v1 scope.
- Product-specific names: card, module, slot, template, instance, "Name &
  Save", "Battery-card saves", `module-name:`.

#### Floppy (CE-1600F)
- How it works today: attaches together with the CE-1600P (union, no own
  attach) (`User-Guide.md:406-407`, `PC1600-Core-Limitations.md:425-427`).
  Disk picker: –empty–, Blank, Formatted, saved disks; A/B flips; lamp
  (`User-Guide.md:411-416`). Image format `.floppy.yaml` only
  (`Decisions.md:678-679`), documented in `docs/developer/Floppy-Image-Format.md`.
  No GCR, no per-track format state, no write-protect
  (`PC1600-Core-Limitations.md:428-435`). Outside edits while inserted
  are overwritten ("eject first", `TODO.md:470-474`).
- Where it lives: `Core/Connector/CE1600FCard.hpp`, `FloppyImageFile.hpp`;
  `Shared/FloppyBay.*`; `Qt6/app/FloppyDiskManager.*`.
- Frontends: Qt6 and Web control bar.
- Maturity: complete.
- Product-specific names: disk, side A/B, "Blank", "Formatted".

#### Host drive (S3:, PC-1600)
- How it works today: File ▸ Mount Directory… makes a host folder drive
  `S3:` (also `Y:` without a CE-1600F) through an own ROM in page-1 bank 7
  and an own protocol over ports 90H/91H (`User-Guide.md:418-440`,
  `Decisions.md:542-569`). Only 8.3 names, bytes unchanged
  (`Decisions.md:687-688`); `INIT "S3:"` refused (`Decisions.md:682-683`).
  Not saved across launches (`Decisions.md:650-651`). Preset key
  `host-drive:` (`User-Guide.md:832`).
- Where it lives: `Core/Connector/HostDirectoryDrive.hpp` (697 lines),
  `PC1600HostDriveCard.hpp`, `firmware/pc1600-hostdrive/`;
  `docs/PC1600-Host-Drive.md`, `docs/developer/PC1600-Host-Drive-Internals.md`.
- Frontends: Qt6 only (`Web-Client.md:111`).
- Maturity: complete; a superset of the MEP interface (`Decisions.md:555-557`).
- Product-specific names: host drive, `S3:`, Mount Directory.

#### Cassette tape
- How it works today: two separate paths. (1) Cassette bay: the ROM's
  `CLOAD`/`CSAVE` read/write a WAV in emulated time through the CE-150
  (PC-1500/1500A) or CE-1600P (PC-1600 MODE 0); tape moves only while the
  remote relay runs it (`User-Guide.md:313-370`, `Decisions.md:91-124`).
  Tapes are fixed-role (play or record), written at each motor stop
  (`Decisions.md:91-100`). Tape folder default `~/Calc-U-1600/Tapes`
  (`User-Guide.md:317`). (2) Fast-load: a WAV is decoded by libsharpdx and
  its file written into memory like a `.bbin`/`.bin`; several files → the
  GUI asks (`User-Guide.md:298-311`, `Decisions.md:463-469`). The two
  never share code (`Decisions.md:466-467`).
- Where it lives: `Core/Tape/TapeDeck.hpp`, `Core/CPU/LH5811Serial.hpp`,
  `Core/ProgramFile.*`, `Shared/TapeBay.*`, `Shared/MachineCodeLoad.*`,
  `Qt6/app/TapeManager.*`.
- Frontends: Qt6 and Web (web: import WAV via Import Card, Disk or Tape…,
  `Web/tests/play.spec.js:248`). CLIs `--tape-in`/`--tape-out`
  (`tools/pc1600_cli.cpp:49-54`).
- Maturity: partial. Not supported: PC-1600 MODE 1 reading PC-1500 tapes
  via CE-1600P, CE-150 on a PC-1600 (`User-Guide.md:367-370`,
  `TODO.md:321-340`).
- Product-specific names: cassette bay, tape counter, "–empty–".

#### Serial ports
- How it works today: PC-1600 RS-232C (TC8576F) and CE-158 RS-232C appear
  as host pseudo-terminals with stable symlinks `calcu1600-rs232c.serial`,
  `calcu1600-ce158.serial` in the serial link directory (default
  `~/Calc-U-1600`) (`User-Guide.md:534-559`, `Decisions.md:689-695`).
  macOS and Linux only; Windows not yet (`User-Guide.md:559`). Line timing
  per character, no modem lines on raw PTY
  (`PC1600-Core-Limitations.md:118-136`). RS-232C/SIO mux not modelled
  (`PC1600-Core-Limitations.md:115-117`). SharpDataExchange's
  `pc1600emul` device appends the RS-232C file name itself
  (`Decisions.md:694-695`).
- Where it lives: `Core/Serial/` (`SerialLink`, `PtySerialLink`,
  `LazySerialLink`), `Core/PC1600/TC8576F.*`, `Core/Connector/Ce158Card.hpp`,
  `Ce158Port.hpp`; doc `docs/developer/PC1600-Serial-Port.md`.
- Frontends: Qt6 only (`Web-Client.md:110`; PTY compiles to an inert stub
  in WASM, `Shared-Architecture.md:129-130`).
- Maturity: partial (Windows missing; SIO split, socket transport, SDE
  live round-trip open, `TODO.md:145-158`).

#### Program files (BASIC, machine code)
- How it works today: File ▸ Load BASIC Program… (`.bas`, `.bbin`, WAV)
  and Load Machine Code… (`.bin` with PC-1600 16-byte or CE-158 27-byte
  header, headerless with address dialog, WAV) write straight into memory;
  they follow MODE and `TITLE` and never change them
  (`User-Guide.md:237-296,475-531`, `Decisions.md:440-443`). File content
  picks the loader via libsharpdx `sde_file_info` (`Decisions.md:457-462`).
  Drag and drop routed by `Core/DropFile` classification
  (`Decisions.md:470-476`, `User-Guide.md:167-188`). No export of programs
  (`User-Guide.md:292-296`).
- Where it lives: `Core/ProgramFile.*`, `Core/MachineCodeFile.*`,
  `Core/DropFile.*`, `Core/Basic/`, `Core/PC1500/PC1500BasicLoader.*`,
  `Core/PC1600/PC1600BasicLoader.*`, `PC1600ProgramPlacement.*`,
  `PC1600MachineCodeLoader.*`; `Shared/MachineCodeLoad.*`;
  `Qt6/app/MachineCodeLoadDialog.*`.
- Frontends: Qt6 and Web (web drop up to 1 MB, `Web-Client.md:127-128`).
- Maturity: complete.

### SDE integration (libsharpdx)
- How it works today: a prebuilt Rust static library from SharpDataExchange
  (crate `sharpdx` 0.3.5) is a required dependency of every program loader;
  used calls: `sde_file_info`, `sde_tokenize`, `sde_wav_count`,
  `sde_wav_decode`, `sde_last_error`, `sde_buf_free`
  (`Core/Basic/vendor/sharpdx/README.md:1-17`). Built without `serial`
  and `audio` features. macOS arm64 `libsharpdx.a` and
  `libsharpdx-wasm.a` are committed (`README.md:21-22,46-51` there);
  Linux/Windows download it at build time from a SharpDataExchange
  **GitHub release** (`tools/fetch_sharpdx.sh`, default tag `v0.3.5`, URL
  `https://github.com/tinue/SharpDataExchange/releases/download/...`, via
  `curl`), not committed. CI runs `tools/fetch_sharpdx.sh` in the
  `core-tests`, Linux and Windows jobs (`build.yml:69-71,163-169,314-316,566-568,646-650`).
  `tools/refresh_sharpdx.sh` rebuilds from a sibling checkout with cargo.
  Linked through target `calcu_sharpdx` (`cmake/CalcU.cmake`).
- Interface contracts with SDE beyond the library: `.bbin` = what
  `sde convert` writes (`User-Guide.md:240-241`); serial file name
  appended by SDE's `pc1600emul` device (`Decisions.md:694-695`);
  `.floppy.yaml` read/written by `sde` (`TODO.md:498-499`,
  `Floppy-Image-Format.md` §8). libsharpdx has `sde_disk_list`/`sde_disk_get`,
  unused (`TODO.md:442-444`).
- Maturity: complete.

### Printers and plotters
- CE-150 (PC-1500/1500A; also PC-1600 MODE 1): ROM runs on host LH5801;
  card supplies ROM window and an LH5810 register model; the ALPS
  mechanism (stepper phases, pen, colour turret) in
  `AlpsPlotterMechanism` (`Core/Connector/Ce150Card.hpp:12-30`).
- CE-1600P (PC-1600): ROM banks 4/5, motor-phase ports 81H-83H, cassette
  relay (`Core/Connector/CE1600PCard.hpp:12-25`). New/Old ROM selectable
  independently of the calculator ROM (`roms/README.md`).
- One plotter at a time; CE-1600P + CE-158 not supported (blocked on IO7N
  question) (`User-Guide.md:198-201`, `TODO.md:430-435`). Attach/detach
  runs a power cycle (`User-Guide.md:205-220`).
- Paper panel with Copy (high-res, physical size) and Cut
  (`User-Guide.md:222-233`; `Qt6/app/PlotterPaperWidget.*`,
  `Shared/PlotterPaper.*`; macOS clipboard image `Qt6/app/MacClipboardImage.mm`).
- CE-158 printer port output panel: Save… writes the bytes exactly as sent,
  Clear starts a new sheet (`User-Guide.md:602-604`,
  `Qt6/app/Ce158PrinterWidget.cpp:53`).
- Frontends: Qt6 and Web (web has paper and printer panels,
  `Web-Client.md:12-17`, `Web/tests/play.spec.js:208`).
- Known accepted limitation: pen colour drift after OFF/ON
  (`Decisions.md:297-312`).

### Audio
- How it works today: `PiezoSampler` follows the buzzer drive line the ROM
  toggles through each machine's gate; no synthesized tones
  (`Decisions.md:82-89`); 48 kHz default (`Core/Audio/PiezoSampler.hpp:38`).
  `shared::AudioPump` delivers PCM to a frontend `AudioSink`; turbo and
  operation audio are thrown away (`Decisions.md:759-761`). PC-1600
  sub-CPU F-pin tones (key click, alarm) not generated
  (`PC1600-Core-Limitations.md:164-166`).
- Frontends: Qt6 `QAudioSink` (`Qt6/app/AudioOutput.*`); Web AudioWorklet
  ring buffer, starts on first key/click (`Web/audio-worklet.js`,
  `Web-Client.md:129-130`). CLIs `--wav` record (`tools/pc1600_cli.cpp:44-47`).
- Maturity: partial (see limitations).

### Pacing
- How it works today: `shared::Pacer` credits each ~16 ms tick against an
  absolute deadline; stall > 0.1 s dropped; turbo while the LCD is held runs
  as many frames as fit in 14 ms (`Shared/Pacer.hpp:9-29`,
  `Decisions.md:212-217`). Clock rates: PC-1500 1.3 MHz
  (`Core/PC1500/PC1500Clocks.hpp:9`), PC-1600 3.58 MHz T-states
  (`Core/PC1600/PC1600Clocks.hpp:8`). RTC advances at emulated rate, seeded
  once from host clock (`Decisions.md:314-320`).
- Frontends: Qt6 `EmulationPacer` (QTimer), turbo via `LcdWidget`
  (`Qt6/app/MainWindow.cpp:230-231`); Web worker 60 Hz `tick()` and "holding
  the LCD runs turbo" (`Web-Client.md:117-118`).
- Maturity: complete. Open: PC-1600 BASIC ~0.4-0.65 % fast (`TODO.md:8`).

### Threading (coroutine jobs)
- How it works today: Core and Shared single-threaded, never blocking; long
  machine work (boot, preset, typing, power cycle) is a resumable
  `job::Task<T>` coroutine (`co_await job::run(machine, n)`), run sliced
  (`step(cycles)`) or to completion; a run request is never split
  (`Shared-Architecture.md:103-120`, `Core/Job.hpp:16-30`,
  `Decisions.md:726-743`). No threaded mode, unlike Calc-U-59
  (`Decisions.md:726-730`). Core's machine mutexes stay, uncontended
  (`Decisions.md:731`). Busy state is `AppSession::busy()` alone
  (`Decisions.md:752-758`).
- Frontends: Qt `SyncOperations` uses `runNow()` (10 ms slices, pumps Qt
  event loop, "Loading…" popup); Web worker uses `start(op, done)`
  (`Shared-Architecture.md:91-101`). CLIs/tests use the blocking names that
  run jobs to completion; the yield hook stays for the CLIs
  (`Decisions.md:740-743`).
- Maturity: complete; `job_tests.cpp` checks sliced vs straight equivalence
  on five presets (`Shared-Architecture.md:112-115`).
- Product-specific names: job, `Task`, `runNow`, `tick`.

### Display
- How it works today: PC-1500 156×7 dot matrix decoded from display RAM
  (`Core/PC1500/PC1500Display.hpp:9-14`); PC-1600 HD61203 + 2× HD61102
  model with datasheet behaviour and a fitted busy time
  (`Decisions.md:224-236`); status line decoded in one place for both
  (`Core/Display/StatusLine.hpp`). `shared::DisplayFrame` is a
  model-agnostic frame (`Shared-Architecture.md:67`); `LcdLayout` gives
  geometry/colours/indicators. LCD text readout (`Core/Display/LcdText`,
  `docs/developer/LCD-Text.md`); Copy Screen gives image + plain Unicode,
  `--lcd-text` an escaped form (`Decisions.md:655-661`). Faceplates
  `Qt6/resources/faceplates/{pc1500,pc1500A,pc1600}.png`.
- Frontends: Qt6 `LcdWidget`, `FaceplateWidget`; Web canvas painters in
  the page (`Web/src/painters.js`, `Decisions.md:813-815`). Web keeps the
  faceplate full-width and shows panels only if ≥ 220 px remain
  (`Web-Client.md:12-17`, `Decisions.md:818-822`).
- Maturity: complete; kana not parsed in LCD text (`TODO.md:403-411`).

### Web shell
- How it works today: one `Web/dist/` serves embedding (React component
  `CalcU1600`, props `width`, `height`, `keyboard`, `model`, `preset`,
  `workerUrl`) and an installable offline PWA (`dist/app/`)
  (`Web-Client.md:1-90`). Engine in one Web Worker, no threads; IDBFS with
  `autoPersist`; Web Lock `calcu1600-storage` lets one tab write
  (`Decisions.md:792-803`, `Web/calc-engine-worker.js:36,116`). ROMs ship
  in `dist/` (`Decisions.md:804-805`). React vendored, JSX compiled at
  build time with vendored Babel, "as in Calc-U-59" (`Decisions.md:816-817`).
  Release attaches `Calc-U-1600-web.zip` (`Web-Client.md:58-60`).
- Left out on the web: DAP, serial ports, host drive, TRACE/LOG, Quit,
  window size, folder settings (`Web-Client.md:105-113`).
- Published: as a preview at `https://tinue.github.io/1600`
  (`README.md:5-6,34-50`); calc-u.org/1600 play page is open work
  (`TODO.md:384-388`).
- Maturity: complete as a preview; real-iPad check open (`TODO.md:389-390`).

### Tests and headless
See section "Tests" below.

### Help-site material (`docs/`)
- User docs: `docs/User-Guide.md` (967 lines, screenshots generated),
  `docs/Keyboard-Mapping.md`, `docs/PC1600-Host-Drive.md`,
  `docs/Debugger.md` (397), `docs/PC1500-Keyword-Modules.md`,
  `docs/Memory-Card-Definition-Spec.md` (624), `docs/Memory-Card-Definition-Format.md`
  (441), `examples/README.md`, `vscode/calcu1600-debug/README.md`
  (`README.md:52-80`). Images in `docs/images/guide/` (generated,
  `docs/developer/screenshots/README.md:1-9`).
- Developer docs: `docs/developer/` (11 files: README, Building,
  Shared-Architecture, Web-Client, PC1600-Core-Limitations, PC1600-Serial-Port,
  PC1600-Host-Drive-Internals, Debugger-Internals, LCD-Text,
  Floppy-Image-Format, Cloud-Sessions) (`docs/developer/README.md:9-85`).
- Background: `docs/background/Decisions.md` (883 lines), 24 plans in
  `docs/background/plans/`.
- `CHANGELOG.md` per release; README "Web version (preview)" section.
- Content is Markdown with Mermaid diagrams (`User-Guide.md:41-57` and others).
- No Calc-U-1600 page on calc-u.org yet (`TODO.md:384-388`).

### Machine control: models, ROMs, reset, power
- Models PC-1500 (ROM A01/A03/A04, default A04), PC-1500A (A04 only),
  PC-1600 (ROM New/Old); CE-1600P ROM New/Old (`User-Guide.md:78-93`,
  `roms/README.md`). A model switch gives a fresh machine without
  peripherals (`User-Guide.md:73-76`). Reset and Reset All
  (`User-Guide.md:95-106`). A CE-1600P ROM switch swaps the box without a
  reset (`Decisions.md:533-540`). Startup boots straight into BASIC, no
  power-on step (`User-Guide.md:63-64`). No APO control or OFF/ON UI
  (`PC1600-Core-Limitations.md:200-202`).
- Rules about which control a model offers live in `shared::ControlState`
  (`Decisions.md:770-781`).

### ROMs
- 21 ROM files fetched by `tools/fetch_roms.sh` and md5-verified; not in
  the repository (`roms/README.md:1-8`). Without them the build succeeds but
  nothing boots (`Building.md:64-66`, `build.yml:3-7`). Released packages
  bundle them (`Web` `dist/` too, `Decisions.md:804-805`). Plus the own
  host-drive ROM (`firmware/pc1600-hostdrive/PC1600-P1-B7-HOSTDRIVE.bin`).

### Other capabilities
- Trace: CPU instruction trace `TRACE.bin` from INSPECTOR TRACE or preset
  `trace:`; reader `tools/read_trace.py` (`Core/PC1500/PC1500TraceFile.hpp:7-20`).
  Rolls over at `trace/maxFileSizeMB` (default 50).
- Clipboard: Copy Screen image + text (`User-Guide.md:136-153`).
- Drag and drop, macOS Dock / Open With (`User-Guide.md:167-188`).
- Cloud sessions: `.claude/settings.json` SessionStart hook runs
  `tools/cloud_session_start.sh` (`docs/developer/Cloud-Sessions.md`).
- 3D-printed memory-slot cover `3d-print/memory-slot-cover.scad`.
- Source artwork `source-material/` (faceplate SVG/TIF, icon script).
- `examples/` shipped as `Calc-U-1600-examples.zip`; user-facing only
  (`Decisions.md:698-703`, `README.md:10-13`).

## Modelling depth (Briefing §1.9)

Real chip complements (research): PC-1600 = SC-7852 (Z-80A core + clock,
memory select, interrupt control, LH-5810-style I/O, LH-5803 interface),
LH-5803, LU-57813P sub-CPU, LR38041 gate array, TC8576F UART/Centronics,
HD61203 + 2× HD61102 LCD, BX7269W level shifter, 3 ROMs, 2 RAMs
(`Ref/PC-1600/PC-1600-Machine-Overview.md:22-38`). PC-1500 = LH5801, LH5811
(LH5810 on early units) I/O, uPD1990AC RTC, TC40H139F/TC40H138F decoders,
SC882G display chips, RAMs, system ROM
(`Ref/PC-1500/Memory-Architecture/PC-1500-Address-Decoding.md:15-57`).

### PC-1500 / PC-1500A

| Part | Modelled as | Source |
|---|---|---|
| LH5801 CPU | Separate class `LH5801` behind `LH5801Bus`; cycle counts from opcode tables; `CDV` no-op; undocumented opcodes no-ops | `Core/CPU/LH5801/LH5801.hpp:18,49`, `PC1600-Core-Limitations.md:23-43` |
| Address decoding, RAM, ROM, display RAM | Folded into `PC1500Memory` (variant-specific RAM sizes and aliasing) | `Core/PC1500/PC1500Memory.hpp:224-233` |
| LH5811 I/O chip | Folded into `PC1500Memory` (DDA/OPA/DDB/OPB/OPC/IF registers); serial block as separate `LH5811Serial`; MSK masking not modelled; serial receive out of scope | `PC1500Memory.hpp:238-290`, `PC1600-Core-Limitations.md:324-333` |
| uPD1990AC RTC | Separate `Upd1990ac`, bit-banged via OPC; test mode not modelled | `Core/PC1500/Upd1990ac.hpp`, `PC1600-Core-Limitations.md:341-343` |
| Keyboard matrix | Separate `PC1500Keyboard` inside memory | `PC1500Memory.hpp:237` |
| LCD | `PC1500Display`: read-only decode of display RAM (functional; no display controller model) | `Core/PC1500/PC1500Display.hpp:9-14` |
| Buzzer | `PiezoSampler` on the drive line (NAND of SD0, PC6·CMT IN) | `Decisions.md:82-89` |
| 40-pin memory slot | Explicit `ExpansionConnector`, cards see `PinState` (PU pin 2, PV pin 3); DME0 (pin 6) not driven; ME1 never offered to it | `Core/Connector/ExpansionConnector.hpp:21`, `TODO.md:186-188,199-224` |
| 60-pin system bus | Explicit `SystemBus` with `SystemBusPins` contacts; WEX/W1/WAIT, INT/IRQ, BFO/φOS, RSTE not driven | `PC1500Memory.hpp:294`, `PC1600-Core-Limitations.md:355-359` |
| CE-150 | `Ce150Card` on the 60-pin bus: ROM window + LH5810 register model; mechanics in `AlpsPlotterMechanism`; tape through `TapeDeck` | `Core/Connector/Ce150Card.hpp:12-30` |
| CE-158 | `Ce158Card`: ROM window, LH5811 PIO, CDP1854A UART, on 60-pin bus; serial via `SerialLink` | `Core/Connector/Ce158Card.hpp:12-30` |
| Memory modules | Software-defined cards from `.card.yaml` (functional description of decode and memory, not chip-level) | `Core/Connector/SoftwareDefinedCard.hpp` |

### PC-1600

| Part | Modelled as | Source |
|---|---|---|
| SC7852 Z-80 core | Separate `SC7852` behind `SC7852Bus`; one wait per M1; MEMPTR and some undocumented effects not modelled | `Core/CPU/SC7852/SC7852.hpp:21,70`, `PC1600-Core-Limitations.md:45-63` |
| SC7852 on-chip blocks (memory select, interrupt control, LH-5810-style I/O 10H-1FH, LH-5803 interface) | Folded into `PC1600Memory` (ports, INT level, keyboard strobes, buzzer modulator) and `PC1600Bank` (Port 31H/28H/3DH/3CH) | `Core/PC1600/PC1600Memory.hpp:441-561`, `Core/PC1600/PC1600Bank.hpp` |
| LH5803 | Separate `LH5803` (subclass of `LH5801`) with `LH5803SharedMemory` (shared RAM, ME0/ME1 routing, display mirror) | `Core/CPU/LH5803/`, `PC1600-Core-Limitations.md:65-104` |
| CPU handover | Separate `PC1600BusArbiter`; only one CPU executes at a time | `Core/PC1600/PC1600Machine.hpp:538`, `PC1500TraceFile.hpp:22-27` |
| LR38041 gate array | Not a separate model; its functions are spread over `PC1600Memory`, `PC1600Bank`, `MemorySlotConnector`, `PC1600BusDrive`; the display mirror of LH5803 writes is attributed to it | `PC1600-Core-Limitations.md:295-317` |
| LU-57813P sub-CPU | Separate `PC1600SubCpu`, high-level (functional) model of the command set rebuilt from the Z-80 ROM; no datasheet, ROM undumped; one fitted response time | `Core/PC1600/PC1600SubCpu.hpp:9-25`, `PC1600-Core-Limitations.md:143-173` |
| TC8576F | Separate `TC8576F` register model per datasheet; per-character line timing | `Core/PC1600/TC8576F.*`, `PC1600-Core-Limitations.md:108-139` |
| HD61203 + 2× HD61102 | One `PC1600Display` following the datasheet (stale first read, busy) with fitted busy clocks | `Decisions.md:224-236` |
| Keyboard | Separate `PC1600Keyboard` | `PC1600Memory.hpp:546` |
| ROM/RAM | Arrays in `PC1600Memory`; CS24 8 KB sub-banking not separate | `PC1600Memory.hpp:551-556`, `PC1600-Core-Limitations.md:237-239` |
| 40-pin memory slots ×2 | Explicit `MemorySlotConnector` each; S1-S3 order remapped per Port 3CH b6; K0-K2 and PVIN (pin 2) not asserted | `PC1600Memory.hpp:434-437`, `Decisions.md:238-249`, `PC1600-Core-Limitations.md:385-391` |
| 60-pin system bus | One explicit `SystemBus`, driven by both CPUs through `PC1600BusDrive`; cards decode their own CPU family's contacts | `Decisions.md:505-522` |
| CE-1600P / CE-1600F | `CE1600PCard` and `CE1600FCard` on the system bus; CE-1600F register protocol, sectors stored decoded (functional for GCR) | `PC1600-Core-Limitations.md:417-435` |
| Host drive | `PC1600HostDriveCard` + `HostDirectoryDrive`, own protocol (functional, not the MEP's) | `Decisions.md:546-549` |
| Buzzer | `PiezoSampler` with a PC-1600 transducer model; F-register modulator idle case | `PC1600-Core-Limitations.md:409-415` |
| BX7269W, RS-232C/SIO mux | Not modelled; one `SerialLink` for both connectors | `PC1600-Core-Limitations.md:115-117` |

Functional emulation in use (both machines): plotter output via mechanism
model (`AlpsPlotterMechanism`); memory modules as software-defined cards;
sub-CPU as a command-level model; CE-163F flash program/erase completes
instantly (`PC1600-Core-Limitations.md:374-377`); floppy without GCR; host
drive own protocol; serial bytes over a PTY instead of a line model;
program fast loaders and WAV fast-load bypass the ROM.

Interfaces explicit vs implicit:
- Explicit: 40-pin memory slot connector(s), 60-pin system bus (both
  pin-numbered), `SerialLink` (byte transport), `TapeDeck` (cassette
  jack, audio samples), `AudioSink`, CPU bus interfaces `LH5801Bus`,
  `SC7852Bus`. `InhibitSource` exists for INH (`Core/Connector/ExpansionCard.hpp:87`).
- Implicit: CE-1600P/CE-1600F union attach; CE-158 printer port (Centronics
  output goes straight to a panel); CE-1600P rear connector for the CE-150
  (not modelled; one plotter at a time); interrupt request lines, WAIT,
  RSTE, BFO/φOS (`TODO.md:254-297`).

## Formats (Briefing §1.2)

| Format | Own / external | Read | Written | Version | Source |
|---|---|---|---|---|---|
| Preset `.pc1500` / `.pc1500a` / `.pc1600` | own | app, CLIs, web | not by the app; scaffolded by the VS Code extension templates; `examples/` maintained by hand | `format-version: 1` required, exact match (`kPresetFormatVersion = 1`) | `Core/Preset/PresetFile.hpp:265`, `PresetFile.cpp:629-655`, `Decisions.md:672-673` |
| Memory card `.card.yaml` | own | app, CLIs, web | Name & Save, autosave, `saveas:` | `format-version: 1` required (`kCardFormatVersion`) | `Core/Connector/MemoryCardDefinition.hpp:24-25,1361` |
| Floppy `.floppy.yaml` | own (also read/written by SDE) | app, web, SDE | autosave, Name & Save, `saveas:`, SDE | `format-version: 1` required (`kFloppyFormatVersion`), plus a `format:` key | `Core/Connector/FloppyImageFile.hpp:45,77-78,115`, `docs/developer/Floppy-Image-Format.md` |
| `.floppy.img` | own (legacy) | no longer read | — | — | `Decisions.md:678-679` |
| `TRACE.bin` | own | `tools/read_trace.py` | app (INSPECTOR TRACE), preset `trace:` | binary magic `PC15`, version 3 | `Core/PC1500/PC1500TraceFile.cpp:9-11` |
| Settings | own | app | app | none (QSettings native store, org/app "Calc-U-1600"; web `localStorage` `calcu1600/…`); values as strings | `Qt6/app/AppSettings.hpp:25-27`, `Web-Client.md:93-94`, `Decisions.md:767-768` |
| `examples/bundled.yaml` | own (internal) | `Shared/PresetCatalog` | by hand | none | `examples/bundled.yaml:1-10` |
| Screenshot scenario `*.shots.yaml` | own (dev) | Qt app `--shots` | by hand | none found | `docs/developer/screenshots/README.md:47-60` |
| Debug `debug:` block / VS Code `launch.json` configuration | own | app (DAP), extension | extension scaffold | none | `docs/Debugger.md:361-388` |
| Card YAML dump (INSPECTOR) | own (fragment of card format) | — | INSPECTOR | n/a | `Shared/CardYamlDump.*`, `User-Guide.md:931-932` |
| LCD text form (`--lcd-text`, `expect:`, `calcu1600/screen`) | own | tests, DAP clients | CLIs, DAP | none | `Decisions.md:655-661`, `docs/developer/LCD-Text.md` |
| Web `dist/` contract, `files.json`, `MANIFEST.json` | own | embedding pages | `Web/build.js` | none | `Web-Client.md:54-90` |
| Host-drive ROM + I/O protocol (ports 90H/91H) | own | — | — | none | `Decisions.md:546-554` |
| Serial port file names `calcu1600-*.serial` | own (contract with SDE) | — | — | n/a | `Decisions.md:689-695` |
| ROM images (`roms/*`) | external | app, CLIs, tests | — | — (md5 list in `tools/fetch_roms.sh`) | `roms/README.md` |
| Cassette WAV | external | any PCM WAV 8-32 bit, mono/stereo (bay and fast-load) | 48 kHz 16-bit (bay) | — | `User-Guide.md:355-358` |
| BASIC listing `.bas` | external | yes (tokenized by libsharpdx / typed) | no | — | `User-Guide.md:239-243` |
| Tokenized BASIC `.bbin` (CE-158 or PC-1600 header) | external (SDE exchange) | yes | no | — | `User-Guide.md:240-241,279-281` |
| Machine code `.bin` with PC-1600 16-byte or CE-158 27-byte header; headerless `.bin`/`.rom` | external | yes | no (machine `SAVE`/`CSAVE` only) | — | `User-Guide.md:483-497` |
| Assembler listings sdas `.lst`/`.rst`, zasm `.lst`, `.SYMBOLS:` tables | external | debugger | no | — | `docs/Debugger.md:348-353` |
| Host-drive files | external (PC-1600 file bytes, 8.3 names) | yes | yes, byte for byte | — | `Decisions.md:687-688` |
| CE-158 printer output | external (raw bytes as sent) | — | Save… | — | `User-Guide.md:602-604` |
| PNG (Copy Screen, `screenshot:`, `--lcd-png`) | external | — | yes | — | `User-Guide.md:773`, `Core/Display/PngWriter.hpp` |
| DAP (JSON over TCP) | external protocol | yes | yes | — | `Debugger-Internals.md:10-17` |

Version policy at the pin: every file format the app reads carries a
required `format-version`; a missing or unknown version is refused; before
1.0 old forms just fail; from 1.0 the previous version keeps working and
app-written files are upgraded at their next save
(`Decisions.md:663-677`). Bundled files are updated in the repository with
each format change (`Decisions.md:674-677`). Pending pre-1.0 change:
optional `dirty` key in `.floppy.yaml`, `format-version` stays 1
(`TODO.md:470-483`).

## GUI concepts (Briefing §1.3, §1.5)

Window layout (Qt6 and Web): calculator (faceplate + LCD) on top, control
bar below, panels at the bottom (INSPECTOR, plotter paper or CE-158
printer) (`User-Guide.md:33-37`, `Qt6/app/MainWindow.cpp:89-93`). The web
mirrors the Qt window's faceplate, menus, control bar and panels
(`README.md:34-36`).

| Concept | Qt6 desktop (macOS, Windows, Linux) | Web (browser, PWA) |
|---|---|---|
| Calculator | Faceplate + LCD, click keys, hold LCD for turbo | Same; touch nearest key within 24 px, multi-pointer |
| Printer / plotter | Paper panel (Copy, Cut); CE-158 printer panel (Save…, Clear); attach via control-bar buttons (model's own peripherals) or Machine ▸ Peripherals (all) (`Decisions.md:645-649`) | Same panels; Machine ▸ Peripherals (`Web/src/CalcU1600.jsx:284`) |
| Debugger | DAP via Settings ▸ Debugger (VS Code); INSPECTOR panel with Pointers, Memory ▾, Dump ▾, Card YAML, Clear, TRACE, LOG (placeholder) | INSPECTOR without TRACE/LOG; no DAP |
| Preset loader | File ▸ Load Preset…, File ▸ Examples ▸ <group>; Settings ▸ Default presets (+ Choose…) | File ▸ Load Preset ▸ <group> items + Upload Preset…; Settings; `?preset=` |
| Removable-media loader | Control bar: slot picker(s) with save icon; disk row (picker, A/B, save, lamp) while CE-1600P attached; cassette bay (tape picker, save, counter) while CE-150/CE-1600P attached; File ▸ Mount Directory… / Unmount Directory | Control bar same; File ▸ Import Card, Disk or Tape…; File ▸ Saved Media… (download/delete) |
| File load / save | File ▸ Load BASIC Program…, Load Machine Code…; drag and drop; save only of media (cards, disks, tapes) and CE-158 printout | Load BASIC/Machine Code via file picker; drop (1 MB); Saved Media downloads |
| Machine | Machine ▸ Model, ROM Revision / ROM Version / CE-1600P ROM, Peripherals, Reset, Reset All; model combo on control bar duplicates Machine ▸ Model (`TODO.md:522-523`) | Same menu structure (`Web/src/CalcU1600.jsx:279-288`) |
| Bundled material | Presets (`examples/bundled.yaml`), bundled cards and disks (`Qt6/resources/cards/`), examples zip separate | Bundled presets copied into `dist/presets/` by `build.js`; ROMs in `dist/` |
| Settings | Settings dialog (Close only) | Settings dialog (no folders) |
| Help | Help ▸ About Calc-U-1600… | Help ▸ About |

Qt menu sources: `Qt6/app/MainWindow.cpp:842-997`. Drift already within
the product: hardware pickers on the control bar, not Settings; ROM
versions only in the Machine menu (`Decisions.md:642-649`); Mount Directory
in File menu (`Decisions.md:650-651`). Long names in lists, short name once
chosen, in both frontends (`Decisions.md:431-434`).

## Tests (Briefing §1.10, §8)

| Harness / suite | Kind | Platform | Where it runs | Elapsed | Capabilities | Source |
|---|---|---|---|---|---|---|
| `CoreTests` (Core + Shared, 50 Core and 9 Shared test `.cpp` files, ~788 test functions, one `main()` in `lh5801_tests.cpp`) | unit + integration (real ROMs, presets, loaders, job equivalence) | macOS, Linux (CI) | locally (`tools/run_tests.sh`, `-O1`), CI `core-tests` (ubuntu-24.04, RelWithDebInfo, CTest) | CI "Run tests" step 1 min 47 s (job 6 min 40 s incl. build) | headless run, scripted input via presets, LCD text/PNG checks, memory checks | `CMakeLists.txt:153-169`, `Core/tests/lh5801_tests.cpp:1558`, `tools/run_tests.sh`, `build.yml:46-83` |
| WASM smoke `tools/wasm_smoke.cpp` via `tools/build_wasm_check.sh` | smoke | node (Emscripten) | locally, CI `web` | 44 s step incl. build | both models boot through `AppSession`, three presets sliced from `tick()`, typed input | `Shared-Architecture.md:122-131`, `build.yml:116-118` |
| Playwright `Web/tests/*.spec.js` (play, offline, prerender; 21 `test(` calls) | GUI/e2e | Chromium (root, sub-path), WebKit (offline) | locally (`Web/run-tests.sh`), CI `web` | 31 s step | page boot, keyboard, touch, panels, Name & Save persistence, multi-tab, offline PWA, SSR prerender | `Web/playwright.config.js:13-19`, `build.yml:130-132` |
| `tools/dap_smoke.py` | protocol smoke | macOS (app required) | locally only | unknown | drives VS Code's DAP requests, `calcu1600/screen`, quit | `tools/dap_smoke.py:1-13` |
| Screenshot scenarios `--shots` (`docs/developer/screenshots/guide/*.shots.yaml`, 9 files) | GUI smoke + checkpoint (image generation) | Qt app; images taken on macOS | locally only (`tools/make_screenshots.sh`) | unknown | frozen emulation, isolated settings, byte-identical Qt images, exit codes 0/1/2 | `docs/developer/screenshots/README.md:1-46`, `User-Guide.md:24-25` |
| `tools/check_presets.sh` (`pc1600_cli --check-preset`) | parse check | local | locally only | unknown | parses every preset in the repo | `tools/check_presets.sh` |
| Headless CLIs `pc1500_cli`, `pc1600_cli` | harness | local | locally | n/a | `--preset`, `--dump-basic`, `--dump-mem`, `--inspect <view>`, `--lcd-png`, `--lcd-text`, `--wav`, `--tape-in/-out`, `--ce158-pty/-rx/-rx-hold/-tx`, `--run-after`, `--rom`, `--ce1600p-rom`, `--save-dir`, `--modules-dir` | `tools/pc1600_cli.cpp:1-57`, `tools/Ce158CliPeer.hpp` |
| Probes `pc1600_plotter_probe`, `pc1600_power_probe`, `pc1600_uart_probe` | hardware diagnostics | local | locally | n/a | e.g. `PC1600_SERIAL=1` PTY | `Building.md:170-176`, `tools/pc1600_uart_probe.cpp:75-85` |
| `dev/loader-matrix/` | one-time verification (ROM load vs fast loader, 80 + 250 cells) | local | manual | unknown | memory comparison | `dev/loader-matrix/README.md:1-12` |
| `dev/tape-matrix/` | one-time check vs Pocket Tools | local | manual | ~1 min (PC-1600), ~15 s (PC-1500) per its README | `--tape-in/-out` per cell | `dev/tape-matrix/README.md:1-12` |
| `dev/hardware-checks/` | programs to run on real hardware | real units | manual | n/a | — | `Decisions.md:701-703` |

Facts:
- CI is dispatched manually only (`workflow_dispatch`, `build.yml:33-36`);
  the Qt app is built but not tested in CI; no CI job runs the screenshot
  scenarios or `dap_smoke.py` (no match in `build.yml`).
- No golden image files in `Core/tests/fixtures/` (only `.bas`, `.bbin`,
  listings, presets, tape fixtures); assertions are in code. `expect:`
  steps in presets act as in-preset assertions (`User-Guide.md:772`).
- Tests use fixture copies, never `examples/` (`Decisions.md:704-707`);
  `LegacyExpression.hpp` is a test oracle (`Decisions.md:720-723`).
- iOS, Windows, Linux GUI tests: none.

## Deliberate oddities

Source: `docs/background/Decisions.md` (883 lines; read before any cleanup,
`CLAUDE.md:3`) and `docs/developer/PC1600-Core-Limitations.md` (444 lines).
Grouped list; the file is the authority and must survive the merge as a
whole.

Emulation fidelity, keep as is (`Decisions.md:10-249`):
- CE-150 carriage constants in half-step units (`:12-22`).
- `colorMagnet()` computed, not latched (`:24-27`).
- CE-150 ROM window gated on PV = 0 (`:29-35`).
- LH5803 PIO latch narrow mask at ME1 F000-F00F; IF software-only (`:37-47`).
- Unclaimed ME1 is not a mirror of ME0 (`:49-65`).
- SC7852 one wait state per M1, measured (`:67-70`).
- PC-1500 power-up RAM fill pattern (&7600-&7BFF = FF) (`:72-76`).
- uPD1990AC TP free-running (`:78-80`).
- Buzzer follows the drive line, no synthesized BEEP (`:82-89`).
- Tapes fixed-role, saved at each motor stop (`:91-100`); tape moves only
  with the relay (`:102-110`); 12 Hz AC coupling with spikes (`:112-124`).
- LH5811 transmitter timing (`:126-143`); PB7 high while ON pressed
  (`:145-149`); CE-1600P port 82H readback (`:151-154`).
- HLE sub-CPU answers IOCS 25H (`:156-159`); power-off is a real cut,
  power-on a reset (`:161-172`); SRIRQ clear on read (`:174-179`); ACK
  timing (`:181-184`).
- TC8576F receiver, PSR bit 0, transmit ignores CS (`:186-201`).
- PC-1600 INT is a level (`:203-210`).
- Absolute-deadline pacing (`:212-217`); RTC runs while LH5803 owns bus
  (`:219-222`).
- PC-1600 LCD stale first read, fitted `kBusyClocks = 4` (`:224-236`).
- Slot 1 S1-S3 reversed order per Port 3CH b6 (`:238-249`).

Authentic ROM behaviour, not bugs (`Decisions.md:251-293`): CE-1600P Y clip
~1000 units; `NEW "S1:"` ERROR 24; virgin CE-1601M INIT; RAM-disk cap 256 KB;
F89B = 160 after boot; INIT can wipe S0; short `bin2wav` leaders fail;
`CLOAD -1` in MODE 0; F127H/F88EH values; `MEM` ignores `TITLE`; CE-158
TERMINAL drops a pre-arrived byte.

Accepted limitations (`Decisions.md:295-320`): plotter pen colour drift
after OFF/ON; TIME advances at emulated rate.

Design choices (`Decisions.md:322-863`), among them: fix the loader not the
preset; preset owns `NEW0` and mode; hardcoded PC-1600 idle PC; idle
detection by share of 32 samples; loaders write modules through slot pins;
no line straddles module banks; `saveas:` needs `template`/`live`; preset is
YAML-shaped; `type:` typed verbatim; one naming rule (Sharp names with
hyphen, `-file` keys, `&`/`0x`/`$` hex); `debug:` keeps camelCase; name
lives in the file; two preset lists; loaders follow MODE/`TITLE`; listing
tokenized by MODE; libsharpdx says what a file is; WAV is a program file;
cards know only the bus; one 60-pin connector per machine; generic open-bus
path for peripheral ROMs; host-drive placement, protocol and MEP interface;
typing rules (2-frame hold, KBII sequences, Paste never ENTER); INSPECTOR
views in Core; `ApplePersistenceIgnoreState`; Copy Screen text vs
`--lcd-text`; `format-version` policy; repository layout rules; Shared
single-threaded; coroutine jobs; busy state in AppSession; UI rules in
Shared (`ControlState`); web: no threads, IDBFS, one writing tab, no own key
hold, hit-testing in the page; debugger: nothing copied into projects,
`debug:` block in preset, ROM extensions via preset keys, bus ROM shadows,
`boot: debug`, pre-execution snapshots; wording: original sources only,
`Ref/<path>` citations, docs describe what is; no FILEX; "original work,
not a fork".

PC1600-Core-Limitations.md adds the catalogue of "not modelled" scope
notes per CPU and device; each entry is backed by a code comment
(`PC1600-Core-Limitations.md:1-17`).

## Release material

- Release assets built by CI: macOS arm64 DMG (signed, notarized in CI),
  Linux x86_64 / aarch64 AppImage, Windows x86_64 / arm64 Inno Setup
  installers (unsigned), web zip (`build.yml` jobs; `Building.md:259-267`).
  Details belong to `release-tooling.md`.
- `Calc-U-1600-examples.zip` from `examples/` (`README.md:10-13`).
- No App Store material; screenshots exist only for the User Guide
  (`docs/images/guide/`, generated).
- Icons: `Qt6/resources/macos/AppIcon.icon/`, `Qt6/resources/windows/`,
  `Qt6/resources/linux/`, source `source-material/Calc-U-1600_icon.*`.

## Open questions

- Briefing and prompt give `TODO.md` as 728 lines; at `v0.8.0` it has 732
  lines (`wc -l`). Which count is meant (possibly a different revision)?
- VS Code extension tests: `tools/check_presets.sh` says templates "are
  checked by the extension's own tests", but `vscode/calcu1600-debug/package.json`
  has no test script; whether such tests exist is unverified.
- Local elapsed time of `tools/run_tests.sh`, the screenshot scenarios and
  `dap_smoke.py`: unknown (not built or run here).
- Qt version actually used by the Linux CI jobs (Ubuntu 24.04 apt package)
  and the macOS job (Homebrew, unpinned): not recorded in the workflow;
  *unverified*.
- Whether the screenshot scenarios run on Linux/Windows: the doc names
  macOS screenshots and three macOS-only `.mm` helpers exist; other
  platforms *unverified*.
- Whether `.floppy.img` files still exist anywhere users might have them
  (bundled or hosted): not checked outside the repository.
