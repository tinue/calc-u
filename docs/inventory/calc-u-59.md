# Inventory: Calc-U-59

Pinned: `v2.3.0` @ `231792c` (= `dev-2.3.0` @ `915a2b5`). Read on 2026-11-08.
Other repositories read: `calc-u-site` @ `a383fbc` (branch `59/2.3.0`; now
pinned at `acd629c`, which removes `59/beta/`, see `calc-u-site.md`),
`ti59-research` @ `bf423f9`, Calc-U-59 branch `appstore` @ `3e23fd2` (local;
`origin/appstore` @ `ffd1cd2`).

All `path:line` references without a repository prefix are Calc-U-59 at
`v2.3.0`. Facts that come from the product's own `reference/*.md` carry that
document's provenance caveat ("best-effort, the code wins").

## Summary

- One emulation core (`Core/`, C++17, ~3,300 lines), one shared non-GUI layer
  (`Shared/`, C++17 std-only, ~8,500 lines, namespace `cu59`) and three GUIs:
  Swift/SwiftUI (macOS, iOS, iPadOS) via an ObjC++ `Bridge/`, Qt6 Widgets
  (Windows, Linux; macOS dev-only), and a React + WebAssembly web client.
- Every GUI owns one `cu59::SessionController` and implements its `Platform`
  interface; the GUIs only draw. A parity table (`reference/FrontendParity.md`)
  tracks every user-visible feature per GUI.
- The core folds the TMC0501 CPU, SCOM, display/keyboard scanning, the Solid
  State module (CROM), the card reader and the PC-100C printer into one class
  (`TMC0501`); only ROM and RAM are separate classes. Neither the printer
  connector nor the module slot is an explicit interface.
- `Decoder.asm` fails because the core has a single digit counter (no
  separate SCOM/display counter synchronised at `SET IDLE`), and also because
  the shared 7-segment decoder maps hex digits A–F to 0–5.
- Own formats: only card files (`Calc-U-59 Card 1.0`/`2.0`) and the binary
  trace (v1/v2) carry a version. The preset format (`.ti59`/`.ti58`/`.ti58c`),
  `ti58c.mem`, the ASM overlay file and settings carry none. Every bundled
  and hosted preset is therefore in an unversioned format (see Open questions).
- Tests: 150 C++ test cases (CTest, ~7 s in CI), 59 Playwright runs in CI,
  21 XCUITest cases run locally only.

## Architecture overview

| Layer | Directory | Language | Build | Notes |
|---|---|---|---|---|
| Core | `Core/` | C++17 | root `CMakeLists.txt` (`calcu59_core`, `CMakeLists.txt:32`); Xcode synchronized folders; `Web/build.sh` (em++) | `TI59Machine` facade over `ROM`, `RAM`, `TMC0501` (`reference/CoreArchitecture.md:19-42`) |
| Shared | `Shared/` | C++17, standard library only | `calcu59_shared` (`CMakeLists.txt:44-51`), Xcode (gnu++17, `Calc-U-59.xcodeproj/project.pbxproj:346`), em++ | "No locations", thread-safe where needed, must run without threads (`reference/SharedArchitecture.md:19-27`, `CLAUDE.md:79-80`) |
| Swift bridge | `Bridge/` | ObjC++ | Xcode | "translate types only" (`CLAUDE.md:65`); `TISessionController`, `TIViewRules`, `TISharedTables`, `TIFileFormats`, `TISettings` |
| Swift app | `App/`, `App/Views/` | Swift 5 (`project.pbxproj:516`), SwiftUI | Xcode project, scheme `Calc-U-59`; iOS 18.0, macOS 15.6 (`project.pbxproj:499,502`) | Ships macOS (DMG) and iOS/iPadOS (App Store) |
| Qt6 app | `Qt6/app/` | C++17, Qt Widgets | `Qt6/CMakeLists.txt` (release entry, CI configures `-S Qt6`, `CMakeLists.txt:88-89`); macOS deployment 15.6 (`Qt6/CMakeLists.txt:10`) | Ships Windows and Linux; macOS build "for development only, never shipped and never built in CI" (`CLAUDE.md:53-55`) |
| Web client | `Web/` | JSX (React 18, vendored), C++ (`Web/bindings.cpp`, embind) | `Web/build.sh` + `Web/build.js` (vendored Babel in Node, no npm deps) → `Web/dist/` (`reference/WebClient.md:78-111`) | `Threading::Cooperative`, runs in a Web Worker (`reference/WebClient.md:8-12`) |
| Tests | `tests/` | C++17, own minimal harness (`tests/TestHarness.hpp:8-10`) | CTest targets `CalcU59Tests`, `CalcU59TraceReader` (`CMakeLists.txt:63-83`) | |
| Tools | `tools/` | Python (via `uv run`), shell, Swift (goldens) | `ti59headless` shared library (`CMakeLists.txt:57-58`) or `tools/headless/build.sh` | Headless harness, agentic tools, trace reader, disassembler |

Diagram: `reference/SharedArchitecture.md:13-17`. Line counts at the tag:
Core 3,306; Shared 8,519; Bridge 1,424; App 4,667; Qt6/app 3,212; Web (JS +
`bindings.cpp`) 3,318; tests 3,538.

Single version source: `MARKETING_VERSION` in the Xcode project
(`CLAUDE.md:86-87`, `project.pbxproj:503`); the Qt build reads it with a regex
(`Qt6/CMakeLists.txt:36-44`) and the web build stamps it into `MANIFEST.json`
(`reference/WebClient.md:260-277`).

Shared rules the code base enforces by convention: non-GUI logic only in
`Shared/` (`CLAUDE.md:60-64`); settings both apps store live in
`Shared/SettingsKeys.hpp` (`CLAUDE.md:81-82`); assets have one source
(`CLAUDE.md:83-85`); `tools/check_parity.sh` flags a branch that touches one
GUI only (`tools/check_parity.sh:1-11`).

## Capabilities

### Calculator models
- How it works today: three models, TI-59, TI-58, TI-58C (`Core/MachineVariant.hpp`); a switch starts a new machine (`SessionController::startMachine`), RAM is lost except the TI-58C's constant memory (`TODO.md:5`). Per-model facts (instruction rate, RAM, card reader, ROM files, scan-loop ranges) in `Shared/MachineModelInfo.hpp` (`reference/SharedArchitecture.md:65`).
- Where it lives: `Core/MachineVariant.hpp`, `Shared/MachineModelInfo.hpp`, `Shared/SessionController.*`.
- Frontends: model picker in all three (`reference/FrontendParity.md:22`).
- Maturity: complete.
- Product-specific names: `Model`, `MachineVariant`, `ModelInfo`.

### Keyboard (on-screen and physical)
- How it works today: on-screen key hit test with widened rects (`Shared/KeyboardLayout`); physical keyboard maps 26 of the 45 keys, by produced character, Shift + letter = 2nd + key (`reference/SharedArchitecture.md:204-250`). All key injection goes through `KeySequencer`: 100 ms minimum hold, tap waits for the steady scan loop, one key at a time, OS auto-repeat ignored, focus loss releases (`reference/SharedArchitecture.md:252-268`). Coordinate systems: `reference/SharedArchitecture.md:191-202`.
- Where it lives: `Shared/KeyboardMap.*`, `Shared/KeySequencer.*`, `Shared/KeyMatrix.hpp`, `Shared/KeyboardLayout.hpp`; GUI: `App/Views/KeyboardView.swift`, `Qt6/app/KeyboardWidget.*`, `Web/src/CalculatorFace.jsx`, `Web/src/PlayCalculator.jsx`.
- Frontends: physical keyboard in the macOS Swift app only (`#if os(macOS)`, `App/Views/KeyboardView.swift:178`), the Qt app, and the web client while the calculator has focus (`reference/FrontendParity.md:28`). iOS/iPadOS: no physical keyboard (`reference/SharedArchitecture.md:207-208`, `TODO.md:16`). Debug and printer panels take no calculator keys (`reference/SharedArchitecture.md:208-210`).
- Maturity: complete for the calculator; debug panels and iPadOS hardware keyboard missing (`TODO.md:15-16`).
- Product-specific names: `KeySequencer`, `KeyboardMap`, `PointerTracker`, matrix code.

### Presets (state files)
- How it works today: UTF-8 text `.ti59`/`.ti58`/`.ti58c` with sections `MODEL:`, `PARTITION:`, `PROGRAM:`, `REGISTERS:`, `KEYSTROKES:`, `CUECARD:`, `CARD:`…`END-CARD`, `SOLID-STATE-MODULE:`, `PRINTER:` (`reference/StateFileFormat.md:17-45`). Applied to a running calculator: reset, 300,000 power-on steps, RAM written directly, then KEYSTROKES played live (`reference/StateFileFormat.md:585-599`, `reference/SharedArchitecture.md:332-345`). KEYSTROKES directives include `Wait:`, `FullSpeed:`/`RegularSpeed:`, `Trace:`, `ZeroElapseTime:`/`ReportElapseTime:`, `ArmCard:`/`InsertCard:`, and GUI controls `PrinterTrace:`, `CutPaper:`, `DebugPanel:`, `Freeze:`, `FreezeOnStart:`, `Step:`, `Resume:` (`reference/StateFileFormat.md:273-350`). Reloads: `PROGRAM:`/`REGISTERS:`/`CUECARD:`/`PARTITION:` after `KEYSTROKES:` apply only listed steps/registers (`reference/StateFileFormat.md:354-392`). Bad lines are reported and the load continues (`reference/StateFileFormat.md:43-45`). The GUI calls a state file a "preset" (`reference/StateFileFormat.md:9`). Presets are read only; no GUI writes one (no writer in `Shared/`; `Platform` writes only cards and `ti58c.mem`, `Shared/SessionController.hpp:65-67`).
- Where it lives: `Shared/StateFile.*` (parser), `Shared/StateApplier.*`, `Shared/SessionController.cpp` (`loadStateFile`, `applyReloadStage`, `applyControl`), `Shared/KeySequencer.*`, `Shared/EmulatorSession.*`.
- Frontends: all three (`reference/FrontendParity.md:40-44`). Swift macOS: `NSOpenPanel` from the "Preset" button (`App/Views/CalculatorView.swift:370-379`); iOS: `fileImporter` (`App/Views/CalculatorView.swift:381-387`, `:182-205`); Qt: File › Open Preset… and the Preset button (`Qt6/app/MainWindow.cpp:56`, `Qt6/app/ControlBar.cpp:71-75`); web: preset picker with bundled presets and upload (`Web/src/Dialogs.jsx:113-140`). Open-with of state files: Qt only (`reference/FrontendParity.md:45`, `Qt6/app/main.cpp:13-25`). Drag and drop: all except iOS (`reference/FrontendParity.md:44`).
- Maturity: complete.
- Product-specific names: `StateFile`, `StateFileResult`, "state file", "preset", `KEYSTROKES`, reload, `CardDefinition`.

### Settings
- How it works today: shared keys and resolution rules for `startupModel`, `lastUsedModel`, `solidStateModuleID`, `traceMaxFileSizeMB`, `ledFontStyle`, `keyboardFeedback` (`Shared/SettingsKeys.hpp:13-42`). Stored in UserDefaults (Swift), QSettings `Calc-U-59/Calc-U-59` (Qt, `Qt6/app/AppSettings.hpp:16`), `localStorage` with prefix `calcu59/` (web, `Web/src/engine.js:20`). App-only keys: Swift `traceLocation`, `traceCustomPath`, `traceCustomPathBookmark`, `portraitDebugPage` (`App/AppSettings.swift:48-51`); Qt `storage/cardDirOverride`, `trace/dirOverride`, `windowGeometry`, `splitterState`, `preset/openDir`, `asm/openDir` and `lastOpenDir` (`Qt6/app/AppSettings.hpp:32-66`).
- Where it lives: `Shared/SettingsKeys.hpp`, `App/AppSettings.swift`, `App/Views/SettingsView.swift`, `Qt6/app/AppSettings.hpp`, `Qt6/app/SettingsDialog.*`, `Web/src/Dialogs.jsx`.
- Frontends: all; per-GUI differences listed in `reference/FrontendParity.md:83`. Swift macOS: standard Settings scene (`App/Calc-U-59App.swift:28-30`); iOS: gear button opens a sheet (`App/Views/CalculatorView.swift:164-167`, `:388-394`); Qt: File › Settings… (`Qt6/app/MainWindow.cpp:60`); web: gear button (`Web/src/ControlBar.jsx:116`).
- Maturity: complete. No setting to disable persistency (`TODO.md:11`).
- Product-specific names: `SettingsKeys`, `kStartupLastUsed`.

### Solid State Software modules
- How it works today: 14 module ROMs in `roms/solid-state/TMC05xx.txt`, catalogued with titles and per-program cue cards in `roms/solid-state/cuecards.txt` (`MODULE-ID`, `MODULE-TITLE`, `MODULE-MENU-TITLE`, `MODULE-MENU-SORT`, `MODULE-ROM`, `CUECARD: n` blocks; `roms/solid-state/cuecards.txt:1-17`). One module is always inserted; the default is `ML` (`Shared/SettingsKeys.hpp:24`, `:40-42`); there is no "no module" option in any GUI. A change triggers a soft reset (`reference/FrontendParity.md:33`). Presets can set one with `SOLID-STATE-MODULE:` (`reference/StateFileFormat.md:28`).
- Where it lives: `Shared/ModuleCatalog.*`, `Shared/ModuleImage.*` (program table, `libraryStep`), `Shared/RomLoader.*`; core: `TMC0501` `m_libData[5000]` (`Core/TMC0501.hpp:372-390`).
- Frontends: selected in Settings in all three: Swift `Picker("Solid State Module")` (`App/Views/SettingsView.swift:74`), Qt combo box (`Qt6/app/SettingsDialog.cpp:37-41`), web Settings dialog, applied on close (`Web/src/Dialogs.jsx:152-172`).
- Maturity: complete.
- Product-specific names: `ModuleCatalog`, `ModuleImage`, "Solid State module", `MODULE-ROM`, `libExecPC`.

### Magnetic cards (TI-59 only)
- How it works today: a card file holds a whole card (two sides) in text version 2.0; it is "fitted" into the cue-card cutout and fed by pressing a corner; the core auto-ejects at `CRD.OFF`; a written side is merged back into the file (`reference/SharedArchitecture.md:282-319`). Presets queue cards (`ArmCard:`, `InsertCard:`, `mem:` cards from `CARD:` blocks) (`reference/StateFileFormat.md:396-539`). The card pickers list `.u59` and `.bin` files, no hidden files (`Shared/CardFile.hpp:95-97`). New cards get `.U59` (`Shared/CardFile.hpp:99-100`).
- Where it lives: `Shared/CardFile.*`, `Shared/SessionController.*` (card reader flow), `Shared/CueCard.*` (corner geometry); core `TMC0501` (`m_cardFullData[984]`, `Core/TMC0501.hpp:395-407`).
- Frontends: all, TI-59 only (`reference/FrontendParity.md:46`). Card storage: Swift iCloud Documents with local Documents fallback (`App/CardStorage.swift:9-13`); Qt `~/Calc-U-59` or the folder set in Settings (`Qt6/app/AppPaths.cpp:19-21`); web an in-memory folder for the page session, upload to fit, download to keep (`reference/WebClient.md:21-24`).
- Maturity: complete; "card stacking in the GUI" by hand is missing (`TODO.md:29`). No card pass timing: a pass costs ROM time only, 0.43 s vs about 1.16 s on hardware (`ideas/CardSound.md:82-93`).
- Product-specific names: `CardDocument`, fit / fresh card / feed side, `mem:` card, `ArmCard`, `InsertCard`, `QueuedCard`.

### Cue cards
- How it works today: three templates (`CueCard`, `MagnetCard`, `SolidStateCard`), `Key: value` grammar with math tokens (`reference/StateFileFormat.md:542-581`); which card shows: running library program, else module card, else user card (`reference/SharedArchitecture.md:447-469`). Layout, shrink-to-fit and draw list computed in `Shared/CueCard.cpp`.
- Frontends: all; the web gets a draw list from `Web/bindings.cpp` (`reference/SharedArchitecture.md:467-469`).
- Maturity: complete.
- Product-specific names: `CueCardContent`, `resolveCueCard`, `\blank` span.

### TI-58C constant memory
- How it works today: `ti58c.mem` text file (optional `CUECARD:`, then `── Registers (raw values) ──` and `Rnnn:` lines) saved 0.5 s after a batch if changed, and on switch/load/suspend (`reference/SharedArchitecture.md:394-406`, `reference/DebugAPI.md:497-520`).
- Where it lives: `Shared/ConstantMemory.*`, `SessionController`; location chosen by the platform: Swift next to the cards (`App/EmulatorViewModel.swift:193`), Qt in the card folder (`Qt6/app/AppPaths.cpp:23`), web `localStorage` key `calcu59/ti58c.mem` (`reference/WebClient.md:24-26`).
- Maturity: complete.

### Printer (PC-100C)
- How it works today: functional emulation inside the CPU class: the `PRT.*` opcodes fill a 20-character buffer and hold BUSY for 4165 (line) / 1370 (feed) instruction cycles; printer interrupt cuts a line short, with column-by-column dot timing measured on a real PC-100A (`reference/CoreArchitecture.md:722-830`, `README.md:38`). Printer presence, PRINT, ADV, TRACE are K-line signals in the key matrix (`reference/CoreArchitecture.md:599-611`). Font and paper geometry in `Shared/PrinterFont.hpp` (`reference/SharedArchitecture.md:471-480`).
- Where it lives: `Core/TMC0501.*` (`Core/TMC0501.hpp:410-422`), `Core/TI59Machine.hpp:87-94`, `Shared/PrinterFont.hpp`, `Shared/SessionController.*` (strip, cut).
- Frontends: all: connect toggle (resets), dot and text view, PRINT/ADV/TRACE, copy text + 600 dpi image, cut (`reference/FrontendParity.md:51-59`). Starts connected (`reference/WebClient.md:28`).
- Maturity: complete for output; paper mechanics not modelled (`README.md:38`).
- Product-specific names: `PrinterCodeLine`, `PrinterGeometry`, `lineDots`, printer interrupt.

### Debugger
- How it works today: CALCULATOR tab (registers, flags, HIR, T, return stack, partition, program listing, PRG SOURCE), CPU tab (history, registers, control registers, display state, ROM heatmap), LOG tab (dumps, level, binary trace toggle), ASM overlay (run hand-assembled TMC0501 code at `0x1800–0x1FFF`). FREEZE, FREEZE ON START, STEP (keycode or instruction), RESUME (`reference/FrontendParity.md:61-76`, `reference/DebugAPI.md:7-19`, `:369-414`). Core PC breakpoints exist (`TI59Machine::addBreakpoint`, `Core/TI59Machine.hpp:103-105`) but only the headless tools use them; no GUI sets them and the session ignores them (`reference/DebugAPI.md:309-313`, `reference/FrontendParity.md:76`). No DAP.
- Where it lives: `Shared/DebugModel.*`, `DebugSnapshots.*`, `DebugText.*`, `DebugLog.*`, `RomHeatmap.*`, `AsmOverlay.*`, `TraceWriter.*`, `FreezeController.*`, `EmulatorSession.*`; GUI: `App/Views/DebugView.swift`, `LiveDebugView.swift`, `CPUInspectorView.swift`; `Qt6/app/debug/*`; `Web/src/Debugger.jsx`.
- Frontends: all three; web only in the wide layout (`reference/FrontendParity.md:65-74`). iPhone: debug page disabled by default (`README.md:9`, `App/AppSettings.swift:51`).
- Maturity: complete for inspection and stepping; breakpoints UI, conditional freezes, keypress latch missing (`README.md:40`, `TODO.md:19-25`).
- Product-specific names: CALCULATOR / CPU / LOG tab, `DebugState`, `FreezeOwner`, `CpuFrame`, ASM overlay, `SolidStateReturn`, PRG SOURCE.

### Traces
- How it works today: binary trace format v2 (magic `TI59`, version u16, model u16), records SESSION_START, TRACE_EVENT (125-byte `CpuFrame`), SESSION_END, USER_EVENT, TRACE_GAP (`reference/DebugAPI.md:417-456`, `Shared/TraceWriter.cpp:20-22`). Manual trace (LOG tab toggle) and scripted `Trace:` files; size limit from Settings.
- Location: Swift Local / iCloud / custom folder; Qt card or trace folder; web MEMFS, offered as download (`reference/FrontendParity.md:74-75`).
- Reader: `tools/read_trace.py` accepts v1 and v2 (`tools/read_trace.py:53-95`); no GUI reads traces.
- Maturity: complete.

### Audio and haptics
- How it works today: "Keyboard Feedback" setting (0 off, 1 haptic, 2 click; `Shared/SettingsKeys.hpp:18`). Swift iOS: `UIImpactFeedbackGenerator` and system sound 1104 (`App/Views/KeyboardView.swift:51-73`); web: synthesised WebAudio click, portrait and PWA only, no haptics (`Web/src/PlayCalculator.jsx:45-53`, `reference/FrontendParity.md:83-85`). Qt: none. No emulated machine sound (card sound is an idea, `ideas/CardSound.md`).
- Maturity: complete for key feedback.

### Pacing
- How it works today: 20 ms batches of cycle-weighted steps (284 on the TI-59, 240 TI-58, 246 TI-58C), absolute deadlines, random batch-end jitter (`BatchJitter`), lag over 100 ms restarts the schedule; full speed skips the sleep except in the scan loop or after a hardware poll (`reference/SharedArchitecture.md:160-189`, `Shared/Pacing.hpp:9-33`). Full speed by holding the display (`Shared/KeyboardLayout.hpp:99`, `reference/FrontendParity.md:27`) or `FullSpeed:` in presets. No speed multiplier (`TODO.md:6`).
- Where it lives: `Shared/Pacing.*`, `Shared/EmulationRunner.*`.
- Maturity: complete.

### Threading
- How it works today: `Threading::Threads` (apps) or `Threading::Cooperative` (web, virtual-time tests), selected by the last constructor argument of `SessionController`, `EmulatorSession`, `EmulationRunner`, `KeySequencer` (`Shared/Threading.hpp:4-14`, `reference/SharedArchitecture.md:119-158`). Threads: UI thread, one emulation thread per session (serial job queue), two key-sequencer lanes, a sequencer timer. Core locking: `m_keyMutex`, `m_displayMutex`, `m_prnMutex`, `m_traceMutex`, atomics (`reference/CoreArchitecture.md:834-850`). Pause in background: Swift (`scenePhase`) and web (`visibilitychange`), Qt keeps running (`reference/FrontendParity.md:89`).
- Product-specific names: `EmulationRunner`, `Threading::Cooperative`, `pump`, `post()`/`sync()`.

### Display
- How it works today: the core latches a glyph per strobe in IDLE and reports per-position duty; `Shared/DisplayPov` turns duty into brightness (persistence of vision), `Shared/SevenSegment.hpp` maps digit + control nibble to segments, `Shared/LedGeometry` gives shapes for "Modernized" and "Classic" styles (`reference/CoreArchitecture.md:491-531`, `reference/SharedArchitecture.md:430-445`).
- Frontends: all; web Classic only (`reference/FrontendParity.md:29`, `reference/WebClient.md:21`). Glow drawn per frontend.
- Maturity: complete within the stated limitations (see Modelling depth).

### Layouts per device
- How it works today: macOS and Qt: calculator | printer | debugger side by side (`App/Views/CalculatorView.swift:226-239`, `Qt6/app/MainWindow.cpp:27-34`). iOS: `TIViewRules.mobileLayoutWidth` (Shared `mobileLayout`, `Shared/DebugText.hpp:30`) chooses pages (portrait, chevrons) or side by side (calculator + printer, plus debugger on iPad and iPhone Duo) (`App/Views/CalculatorView.swift:241-273`). Web: wide or portrait; the PWA picks by window size (`reference/WebClient.md:14-20`, `:226-235`).

### Web shell
- How it works today: `Web/dist/` is self-contained and location-independent; a host loads `calc.js`, `calc.css` and mounts the `PlayCalculator` component; `MANIFEST.json` gives `{name, version, commit, files}` (`reference/WebClient.md:113-131`, `:260-277`). The installable PWA (`app/`) precaches all ROMs, modules and presets for offline use (`reference/WebClient.md:226-258`). calc-u-site embeds the zip as `59/calc/` (`reference/WebClient.md:30-33`); the site also carries `59/beta/` (calc-u-site @ `a383fbc`).
- Maturity: complete.

### Headless and agent tools
- `tools/headless/`: flat C API over `TI59Machine` (`tools/headless/capi.cpp:1-7`, functions `h_new`, `h_load_model_roms`, `h_load_module`, `h_press`, `h_stepn`, `h_record`, `h_snapshot`, `h_ram_read/write`, `h_card_insert/eject`, `h_printer_lines`, `h_disasm`, …) and `ti59.py` (ctypes) for ROM traces of key sequences (`tools/headless/README.md:1-27`). It drives `Core/` directly, not `SessionController`. `build.sh` builds a `.dylib`, macOS only (`tools/headless/README.md:5-6`); CMake also builds `ti59headless` (`CMakeLists.txt:57-58`). `coverage.py` maps keystroke scenarios to ROM words.
- `tools/agentic/`: run, time and profile `.ti59` files headless, keycode disassembler/assembler (`tools/agentic/README.md:1-60`). `ti59file.py` contains its own Python parser for `PROGRAM`, `REGISTERS`, `PARTITION`, `KEYSTROKES` (`tools/agentic/ti59file.py:1-10`, `:43`), separate from `Shared/StateFile.cpp`.
- Others: `tools/read_trace.py`, `tools/compare_trace.py`, `tools/compare_logs.py`, `tools/disasm.py`, `tools/annotate_rom.py`, `tools/goldens/`, `tools/check_parity.sh`, `tools/gen_qt_icons.sh`, `tools/build_qt.sh`.

### Help-site material
- The user guide lives only in `calc-u-site` (`59/`), not in this repository (`README.md:15-21`, `.claude/skills/calc-u-59-docs/SKILL.md:14-33`). The repository supplies: the web client zip, the example files (mirrored byte-identical into `59/software-files/` and `59/calc/presets/`, see Compatibility surface), the format reference the site links to (`reference/StateFileFormat.md`).
- Hand-maintained duplicates: the site's keyboard legend `TI59_KEYBOARD_LEGEND` in `calc-u-site` `59/Pages.jsx` lists the physical-key bindings by hand (`reference/SharedArchitecture.md:212-214`); `prompt/ti59-agent-prompt.md` duplicates the parts of `StateFileFormat.md` it needs (`README.md:186`).

### Further capabilities
- Reset and clean reset (Cmd/Ctrl held; 1 s long press on touch) (`reference/FrontendParity.md:23`).
- About and Help: About in all three; Help link to `https://calc-u.org/59/` in Qt and web, pending in Swift (`reference/FrontendParity.md:82-84`).
- Qt developer screenshot `--grab <png> [--after ms]` (`Qt6/app/main.cpp:68-77`).
- UI test automation hooks in the Swift app (`reference/FrontendParity.md:86`).
- LLM system prompt `prompt/ti59-agent-prompt.md` producing `.ti59` files (`README.md:60-72`).
- ROM listings and guides in `rom/` (annotated disassemblies of both ROM sets, fast mode, printer interrupt) (`README.md:166-172`).

## Modelling depth (Briefing §1.9)

### Blocks modelled

| Block | Hardware part (product's naming) | Modelled as | Source |
|---|---|---|---|
| Instruction ROM | TMC0582 / TMC0583 / TMC0571B (TI-59, TI-58); CD2400 / CD2401 / TMC0573 (TI-58C) | Separate class `ROM`: three chips concatenated into 6144 words, plus the debug overlay region | `reference/CoreArchitecture.md:34-37`, `reference/SharedArchitecture.md:408-423` |
| Constant ROM | the two "SCOM" ROM chips' constant sections (`*-CONST-K.txt`) | Loaded into the core (`TI59Machine::loadConstants`) | `Core/TI59Machine.hpp:16`, `roms/calculator/files.txt` |
| User RAM | TMC0598 | Separate class `RAM` (120 × 16 nibbles) | `reference/CoreArchitecture.md:36` |
| CPU | TMC0501 | Class `TMC0501` | `reference/CoreArchitecture.md:37` |
| SCOM registers, digit counter, display strobe, keyboard scan | SCOM chips | Folded into `TMC0501` | `Core/TMC0501.hpp:335`, `reference/CoreArchitecture.md:541-545` |
| Solid State module (CROM) | TMC0540 family, own address counter | Folded into `TMC0501` (`m_libData[5000]`, `m_libAddr`, `m_libExecPC`) | `reference/CoreArchitecture.md:638-693`, `Core/TMC0501.hpp:372-390` |
| Card reader | TMC0594 (research name) | Folded into `TMC0501` (`m_cardFullData`, `m_cardBankBuffer`, card switch at a digit column) | `Core/TMC0501.hpp:395-407`, `reference/CoreArchitecture.md:615-634` |
| Printer PC-100C | external cradle | Folded into `TMC0501` (`m_prnBuf`, BUSY countdown) | `Core/TMC0501.hpp:410-422`, `reference/CoreArchitecture.md:722-735` |
| 7-segment decoder, zero suppression, C indicator | (hardware: segment decode in the CPU chip per `ti59-research/docs/architecture.md:411-416`) | Zero suppression in `TMC0501::postOperation`; glyph decoding in `Shared/SevenSegment.hpp`; brightness in `Shared/DisplayPov` | `reference/CoreArchitecture.md:493-519`, `Shared/SevenSegment.hpp:33-51` |
| Clock | TP0240 clock chip / resonator (TI-58C: none) | Constant `resonatorHz()`; instruction rates in `MachineModelInfo`; IDLE weight 4 (TI-59/58) or 1 (TI-58C) | `reference/CoreArchitecture.md:55-78`, `:876-890` |

`TI59Machine` is the facade; `Shared/` never touches `TMC0501` directly
(`reference/CoreArchitecture.md:29-32`). The core's own table: "`TMC0501.hpp/.cpp`
| TMC0501 + SCOM + CROM, card reader, printer | CPU and every peripheral"
(`reference/CoreArchitecture.md:37`).

### Interfaces: explicit or implicit
- Printer connector: implicit. The printer is selected by `TI59Machine::setPrinterConnected` (`Core/TI59Machine.hpp:93`); the core decodes the `PRT.*` opcodes itself and asserts printer signals in the key matrix (KP at D0 = present, PRINT/ADV at row 12, TRACE at row 15; TI-58C present at D10) (`reference/CoreArchitecture.md:599-611`). No IRG/EXT/IDLE bus is modelled. The PC-100A service manual lists 12 signal contacts: Busy (KR), KP, KN, Φ1, D0, D15, IRG, IDLE, EXT, Φ2, D12, I/A (`ti59-research/PC100C/service_manual.txt:33-45`), and calls the connector "14-contact", with two large contacts for power (`:2-5`).
- Module slot: implicit. The module image is loaded with `TI59Machine::loadLibrary` (`Core/TI59Machine.hpp:15`); the `LIB.*` commands act directly on the array (`reference/CoreArchitecture.md:644-655`). The slot's pin-out was not found in `ti59-research` (see Open questions).
- Card reader: implicit; `insertCard`/`cardEject` on the facade (`Core/TI59Machine.hpp:77-83`), card switch modelled as a K-line at a digit column (`Core/TMC0501.hpp:180-186`).
- Keyboard: the key matrix `key[digit]` is the interface; GUIs press via `KeyMatrix` → `TI59Machine::pressKey` (`reference/SharedArchitecture.md:191-202`).
- ROM/RAM: explicit classes, but accessed directly by `TMC0501` (references, `reference/CoreArchitecture.md:23-27`).

### Functional emulation
- Printer: output functional; BUSY timing in instruction cycles; dot timing for cut lines measured (`README.md:38`, `reference/CoreArchitecture.md:736-830`).
- Card reader: no transport timing; `CRD.RD`/`CRD.IN` never wait (`ideas/CardSound.md:91-93`).
- `WAIT.BUSY` is a no-op (`reference/CoreArchitecture.md:555`).
- Display: one glyph per position at one brightness; on hardware the C and the other leftmost segments come from different chips (`reference/CoreArchitecture.md:526-531`).
- TI-58C clock: a model of one unit's clock (`reference/CoreArchitecture.md:78`, appendix `:894-950`).

### Why `examples/assembly/Decoder.asm` does not run
The program issues `WAIT D12` instead of `WAIT D1` before `SET IDL`
(`examples/assembly/Decoder.asm:54-55`), to shift the display multiplex out of
phase so B.DPT shows as a digit and all 16 decoder combinations appear
(`:7-15`, `:34-40`). It depends on:

1. **A second digit counter, synchronised at the RUN→IDLE transition.** On
   hardware the SCOM samples the CPU's digit counter at `SET IDLE`; a wrong
   phase leaves the two counters offset (`ti59-research/docs/CPU_SCOM_Interconnect.md:27-39`, `:113-145`).
   In the core, `SET IDL` only sets `FLG_IDLE` (`Core/TMC0501.cpp:855-858`) and
   the strobe uses the CPU's own `digit` (`Core/TMC0501.cpp:1414-1420`): "the
   CPU's digit and the display strobe share one counter, so they are always in
   phase" (`reference/CoreArchitecture.md:521-524`).
2. **Keyboard rows on the same offset counter.** The program's keys misbehave
   on hardware (RCL resets the CPU, LRN row dead, BST row odd;
   `examples/assembly/Decoder.asm:38-40`). The core scans `key[digit]` from
   the same single counter (`reference/CoreArchitecture.md:541-545`), so keys
   are read correctly (`examples/assembly/Decoder.asm:22-26`).
3. **Hex digit glyphs.** Even with a phase shift, digit values 0xA–0xF would
   not show as on hardware: `segmentsFor` maps `digit % 10`
   (`Shared/SevenSegment.hpp:43`), while the research decode table shows A as
   "A" and B as "b" (`ti59-research/docs/architecture.md:582-599`).

The research document describing the emulator's display as "a phase-independent
snapshot taken at digit == 0" (`ti59-research/docs/CPU_SCOM_Interconnect.md:166-188`,
`:282`) describes an older core; at the pin the capture is per strobe
(`Core/TMC0501.cpp:1399-1420`). A stale comment at `Core/TMC0501.cpp:856-857`
still mentions the digit-0 snapshot.

## Compatibility surface (Briefing §1.2)

### Formats read and written by the release

| Format | Own / external | Read | Written | Explicit version | Versions read | Version written | Source |
|---|---|---|---|---|---|---|---|
| Preset / state file `.ti59`, `.ti58`, `.ti58c` | Own | yes (all GUIs, drag and drop, Qt open-with) | no | **none** (no version line; parser checks only extension and content) | — | — | `reference/StateFileFormat.md:17-45`, `Shared/StateFile.hpp:117-119` |
| Card file text `Calc-U-59 Card 2.0` (`.u59`/`.bin`) | Own | yes | yes (new cards) | yes, magic line | 2.0 | 2.0 | `Shared/CardFile.hpp:11-24`, `Shared/CardFile.cpp:19-21`, `:384-392` |
| Card file text `Calc-U-59 Card 1.0` | Own | yes | yes: a version 1 card is written back as 1.0 | yes, magic line | 1.0 | 1.0 (for cards read as v1) | `Shared/CardFile.hpp:25-41`, `:73`, `Shared/CardFile.cpp:293-294`, `reference/StateFileFormat.md:491-495` |
| Card file binary: `Calc-U-59-CRD` header + 246 bytes | Own | yes | rewritten as text 1.0 | no (magic only) | — | 1.0 text | `Shared/CardFile.cpp:22`, `:393-402`, `reference/StateFileFormat.md:493-495` |
| Card file binary headerless, 246 or 984 bytes | Own | yes | rewritten as text 1.0 (984 reduced to one bank) | no | — | 1.0 text | `Shared/CardFile.cpp:403-408`, `reference/StateFileFormat.md:493-495` |
| `ti58c.mem` (TI-58C constant memory) | Own | yes | yes | **none** | — | — | `Shared/ConstantMemory.hpp:9-13`, `reference/DebugAPI.md:497-520` |
| ASM overlay `.asm` (`PROGRAM:` listing, `HEX:` words) | Own (derived from Sladký's guide listings) | yes | no | none | — | — | `reference/DebugAPI.md:380-397`, `examples/assembly/README.md:8-22` |
| Binary trace `CALCU59_TRACE.bin` etc. | Own | no (only `tools/read_trace.py`) | yes | yes, header u16 | (tool: v1, v2) | v2 | `reference/DebugAPI.md:417-440`, `Shared/TraceWriter.cpp:20-22`, `tools/read_trace.py:53-95` |
| Settings (UserDefaults, QSettings, `localStorage`) | Own | yes | yes | none | — | — | `Shared/SettingsKeys.hpp:6-24` |
| Printer copy (text, 600 dpi image to clipboard) | Own (output only) | no | clipboard | — | — | — | `reference/FrontendParity.md:59` |
| ROM text dumps `roms/calculator/*.txt`, `roms/solid-state/TMC05xx.txt` (Datamath "TMS0500 … ROM Recorder" text) | External | yes (bundled) | no | n/a | — | — | `roms/calculator/TMC0582.txt:1-8`, `roms/solid-state/TMC0541.txt:1-8`, `reference/SharedArchitecture.md:408-423` |
| Module catalog `roms/solid-state/cuecards.txt` | Own, bundled only | yes | no | none | — | — | `roms/solid-state/cuecards.txt:1-17` |
| Web `presets/manifest.json`, `MANIFEST.json` | Own, build artefacts | web only | build | none (MANIFEST carries the app version) | — | — | `reference/WebClient.md:124-129`, `:260-277` |

Constructs that were removed and are now rejected as errors (not silently
accepted): `SKIP-RESET:` (`Shared/StateFile.cpp:383`), KEYSTROKES key 99
(`Shared/StateFile.cpp:91-92`), `WaitFullSpeed:` (`Shared/StateFile.cpp:519`).
Still accepted and ignored: `PARTITION` `.xx` suffix
(`reference/StateFileFormat.md:54-55`), cue-card `IDAlign`
(`reference/StateFileFormat.md:566`), unknown cue-card keys (`:544-546`).
Card text 1.0 exists since 1.1.0 (`CHANGELOG.md:143`); `ti58c.mem` changed
from binary to text in 0.4.0 or earlier (`CHANGELOG.md:204`); trace v1 since
1.0.0 (`reference/DebugAPI.md:424-425`).

### Unversioned legacy formats still accepted (*dropped* per Briefing §1.2)
- Binary card files with `Calc-U-59-CRD` header (246 bytes).
- Headerless binary card files of 246 or 984 bytes.
- `ti58c.mem` (no version; user data on every device that ran a TI-58C).
- Preset files `.ti59`/`.ti58`/`.ti58c` (no version; see Open questions).
- ASM overlay files `.asm` (no version).
- Settings stores (no version).

### Bundled files in an unversioned format
- Web client: 10 presets copied from `examples/` by `Web/build.js:75-79` per
  `Web/presets.json:1-12`: `diag.ti59`, `diag.ti58`, `ram_test.ti59`,
  `sum.ti59`, `characters.ti59`, `printer-interrupt.ti59`, `picard.ti59`,
  `globe-print.ti59`, `switzerland.ti59`, `fastmode-sst.ti59`. All unversioned
  preset format.
- Swift and Qt apps bundle no presets, cards or `.asm` files: Qt resources
  are the ROM text files and artwork (`Qt6/CMakeLists.txt:100-116`,
  `Qt6/resources/calc-u-59.qrc:1-5`); the Xcode project references only
  `roms/` text files (`Calc-U-59.xcodeproj/project.pbxproj:48-76`).
- Test fixtures (not shipped): `tests/fixtures/statefiles/*.ti5x` (preset
  format), `tests/fixtures/cards/*.u59` (all text 1.0, versioned),
  `tests/fixtures/cards/*.hex` (test-only).

### Files hosted on calc-u.org (`calc-u-site` @ `a383fbc`, `59/`)
All files below are in an unversioned format. Every one except the tutorial
files is byte-identical to the file of the same name in `examples/` at the tag
(compared with `cmp`).

- `59/software-files/` (preset format): `FastMode_Zero4.ti59`,
  `Frequency-Transformation.ti59`, `Integer-Base-Conversion.ti59`,
  `calendar-01-Weinberger.ti59` … `calendar-09-dispatch.ti59` (9 files),
  `characters-fastmode.ti59`, `characters.ti59`, `diag.ti58`, `diag.ti59`,
  `erdkundetest.ti59`, `erdkundetest-2.ti59`, `fast-grafik-3d-plot.ti59`,
  `fastmode-sst.ti59`, `fastmode.ti58`, `fastmode.ti59`, `firmware.ti59`,
  `globe-print.ti59`, `keycodes.ti59`, `life.ti59`, `pdcm-card-tricks.ti59`,
  `pdcm-make-cards.ti59`, `picard.ti59`, `printer-interrupt.ti59`,
  `printer-quirks.ti59`, `ram_test.ti59`, `ram_test_full_fast.ti59`,
  `repartition.ti58c`, `stars-and-stripes.ti59`, `sum.ti59`,
  `switzerland.ti59`, `texas-print.ti59`, `trace-quirk.ti59` (39 files).
- `59/software-files/assembly/` (ASM overlay format): `DPT.asm`,
  `Decoder.asm`, `simple_count.asm`, `stopwatch.asm`.
- `59/tutorial-files/` (preset format, not in `examples/`):
  `tutorial-1-setup.ti59` … `tutorial-5-registers.ti59`, `tutorial.ti59`.
- `59/calc/presets/` and `59/beta/presets/`: the same 10 presets as the web
  bundle (`59/beta/` is removed at `acd629c`).
- No card files (`.u59`, `.bin`) and no `ti58c.mem` are hosted.
- None of these files (repository or site) uses `SKIP-RESET:`,
  `WaitFullSpeed:` (grep over `examples/`, `tests/fixtures/`, `work/`,
  `59/software-files/`, `59/tutorial-files/`), and no `KEYSTROKES:` section in
  them contains the key token `99` (scan of the 44 files with a `KEYSTROKES:`
  section, comments stripped).

## GUI concepts (Briefing §1.3, §1.5)

| Concept | Swift macOS | Swift iOS / iPadOS | Qt6 (Windows, Linux) | Web |
|---|---|---|---|---|
| Calculator | Left column of one window (`CalculatorView.swift:230-232`) | Page 1 of paged layout, or left column side by side (`CalculatorView.swift:241-273`) | Left pane of a `QSplitter` (`MainWindow.cpp:27-34`) | Left column (wide) or page (portrait) (`reference/WebClient.md:14-18`) |
| Model picker | Window toolbar (`CalculatorView.swift:169-172`) | In the bar below the keys (`CalculatorView.swift:337-340`) | Combo box in the control bar (`ControlBar.cpp:46-51`) | Control bar select (`Web/src/ControlBar.jsx:92`) |
| Printer | Middle column, always visible (`CalculatorView.swift:234-235`) | Page 2, or middle column (`CalculatorView.swift:254-256`) | Middle splitter pane | Middle column / page 2 |
| Debugger | Right column, always visible (`CalculatorView.swift:237-238`) | Page 3 only if "Debug Page in Portrait" (default off); side by side on iPad and iPhone Duo (`CalculatorView.swift:257-264`, `App/AppSettings.swift:51`) | Right splitter pane (`MainWindow.cpp:34`) | Right column, wide layout only; none in portrait (`reference/FrontendParity.md:65-74`) |
| Preset loader | "Preset" button in the bar → `NSOpenPanel` (`CalculatorView.swift:370-379`); drag and drop | "Preset" button → `fileImporter` (`CalculatorView.swift:381-387`) | File › Open Preset… (Ctrl+O), "Preset" button, drag and drop, open-with (`MainWindow.cpp:56`, `ControlBar.cpp:71-75`) | "Preset" button → preset picker: bundled list and upload (`Web/src/ControlBar.jsx:113`, `Web/src/Dialogs.jsx:113-140`); drag and drop |
| Card loader (TI-59) | "Crd" (fit) and "+ Crd" (fresh) buttons in the bar, "Eject Card" while fitted (`CalculatorView.swift:347-367`), `CardPickerView` sheet; drag and drop | Same buttons, `CardPickerView` | File › Fit Card…, Fit Fresh Card…, control-bar buttons, `CardPickerDialog` (`MainWindow.cpp:57-58`, `ControlBar.cpp:54-68`) | Control-bar buttons → card picker dialog with upload/download (`Web/src/ControlBar.jsx:105-110`, `Web/src/Dialogs.jsx:101`) |
| Card feed | Press on the fitted card's corner on the calculator canvas (`reference/SharedArchitecture.md:294-297`) | same | same | same |
| Module loader | Settings scene (app menu › Settings) (`App/Calc-U-59App.swift:28-30`, `SettingsView.swift:74`) | Settings sheet via gear button (`CalculatorView.swift:388-394`) | File › Settings… (`MainWindow.cpp:60`, `SettingsDialog.cpp:37-41`) | Settings dialog via gear (`Web/src/Dialogs.jsx:152-172`) |
| ASM overlay load | CPU tab "Select File" (`DebugView.swift:232`) | same (when the debugger shows) | `AsmOverlayBox` file dialog (`MainWindow.cpp:105`) | Debugger ASM overlay upload (`reference/FrontendParity.md:71`) |
| File save | Cards only, written back on eject into iCloud/Documents | same | Cards only, in the card folder | Cards in session memory; user downloads them |
| Trace files | LOG tab TRACE toggle; location from Settings | same | LOG tab; trace folder in Settings | LOG tab; download when switched off |
| Bundled material | ROMs, modules, artwork only | same | same | plus 10 presets |
| Help | none (`reference/FrontendParity.md:84`) | none | Help › Calc-U-59 Help (`MainWindow.cpp:69-70`) | Settings and About dialogs |

Concepts named in the briefing that differ already between GUIs of this
product: modules are reached only through Settings, presets through a
bar button or menu, cards through bar buttons and a canvas press.

## Tests (Briefing §1.10, §8)

| Suite / harness | Kind | Platform | Where it runs | Elapsed | Capabilities |
|---|---|---|---|---|---|
| `CalcU59Tests` (`tests/*.cpp`, 150 `TEST` cases: core 17, controller 14, cooperative 42, debug 14, formats 21, preset stages 7, runtime 17, state applier 8, tables 8, trace 2) | Unit and ROM-driven integration; golden comparison (`tests/golden/formats.txt`, `keynames.txt`) | Any CMake host (Linux in CI) | Locally (`ctest`), GitHub Actions job `core-tests` (`.github/workflows/build.yml:18-32`) | 6.55 s (CI run 42388503215 at `915a2b5`, 2026-11-07); `ctest` total 7.14 s; local: M4 Pro, macOS 27.2, 2026-11-09: cold build 3.7 s, ctest 6.75 s (150 tests, 8335 checks, all passed) | Headless, real ROMs, virtual time (cooperative), fake platform (`tests/FakePlatform.hpp`), state-file and card fixtures, writes a trace |
| `CalcU59TraceReader` | Cross-check: `tools/read_trace.py --json` on the trace the tests wrote | Needs Python 3 | Locally, CI | 0.30 s (same run) | Keeps reader and `TraceWriter` in step (`CMakeLists.txt:76-83`) |
| Goldens generator `tools/goldens/` | Golden data from the old Swift code at commit `66537d3` | macOS (Swift compiler) | Locally, on demand | local: M4 Pro, macOS 27.2, 2026-11-09: 3.8–4.5 s (`gen.sh`, all). `keynames.txt` regenerates byte-identical; `formats.txt` does not (48 410 vs 31 778 lines, more example files, other order): the committed golden predates today's `examples/` | Regenerated only when a driver changes (`tools/goldens/README.md:15-25`) |
| Web Playwright (`Web/tests/play-calculator.spec.js` 28, `offline.spec.js` 1, `prerender.spec.js` 1) | Browser GUI tests against `Web/dist/` | Chromium (projects `root`, `subpath`), offline in Chromium and WebKit (`Web/playwright.config.js:12-18`) | Locally (`Web/run-tests.sh`), GitHub Actions job `web` | 59 passed in 1.5 m (CI run 42388503215); step 15:54:47–15:56:21 | Two HTTP servers to prove location independence (`Web/run-tests.sh:1-5`) |
| XCUITests `Calc-U-59UITests/` (21 tests: Layout 9, Screenshot 8, FilePicker 2, PresetLoad 1, LaunchTests 1) | GUI tests and screenshot capture | iOS simulator (devices in `bin/simulators.yaml`), macOS | Locally only (no macOS CI job) | local: M4 Pro, macOS 27.2, 2026-11-09, `UIRegressionTests` on iPhone 18 Pro Max (iOS 27.2): build-for-testing 8.7 s (cold), tests 140 s wall (XCTest 116.5 s), 14 run: 13 passed, 1 skipped (`testWideWindowShowsDebugger`, portrait device). macOS destination: not measured (runner started, no test began in 6 min; unsigned runner / missing Accessibility permission) | Test plans `UIRegressionTests.xctestplan` (skips `ScreenshotTests`) and `Screenshots.xctestplan` (only `ScreenshotTests`) |
| `bin/run-screenshots`, `bin/simulators` | Checkpoint: App Store screenshots into `appstore/screenshots` | iOS simulators | Locally | local: M4 Pro, macOS 27.2, 2026-11-09: iPhone role (6 shots on iPhone 18 Pro Max) 272 s wall, 147 s in tests, one `xcodebuild test` per shot; full set 10 shots on 3 devices (iPhone Duo must be opened flat by hand), not timed | Filters by test name; simulator set from `bin/simulators.yaml` (`bin/run-screenshots:1-26`) |
| Qt `--grab` | Offscreen screenshot of the Qt window | Qt platforms | Locally, by hand | n/a | `Qt6/app/main.cpp:68-77` |
| `tools/headless/` | Headless emulator (Core only) | macOS (`build.sh` dylib); CMake target on any host | Locally | n/a | Headless run, scripted keys, per-instruction records, state dump (registers, SCOM, RAM), printer lines, cards, disassembly, ROM coverage |
| `tools/agentic/` | Headless program runner | as above | Locally | full calendar year 1–2 s, slow programs up to ~30 s (`tools/agentic/README.md:30-32`) | Run `.ti59` files, timing in emulated seconds, printout, hot steps, step cost, keycode (dis)assembler |
| `tools/check_parity.sh` | Branch check for one-sided GUI changes | shell + git | Locally | n/a | `tools/check_parity.sh:1-20` |

Whole CI run (all jobs in parallel) for the pinned tree: 15:52:48–15:56:41,
about 3 m 53 s. No GUI test of the Swift or Qt apps runs in CI; no Windows or
Linux installation or smoke test exists. Qt in CI: Windows 6.8.2
(`.github/workflows/build.yml:162-168`); Linux uses the distribution packages
of Ubuntu 24.04, which installed Qt 6.4.2 (`build.yml:83-87`; CI log of run
42388503215: `qt6-base-dev (6.4.2+dfsg-21.1build5)`).

## Deliberate oddities

Behaviour that looks wrong or arbitrary but is intended, with the stated reason.

| ID | Behaviour | Reason given | Source |
|---|---|---|---|
| ODD59-1 | Only 26 of 45 keys bound on the physical keyboard; "Do not 'complete' the table without asking" | Deliberate scope | `reference/SharedArchitecture.md:216-222` |
| ODD59-2 | Shift is a 2nd prefix only for letters; other keys resolve on the produced character | Non-US layouts work | `reference/SharedArchitecture.md:228-233` |
| ODD59-3 | Debug and printer panels take no calculator keys; no focus indicator, so clicking the debugger silently stops keys | Space and Cmd/Ctrl+C would have no single owner; stepping stone to a unified input surface | `reference/SharedArchitecture.md:206-210`, `:270-280`, `TODO.md:14` |
| ODD59-4 | Key press held at least 100 ms; tap waits for the scan loop; auto-repeat ignored; two-code bindings ignore key-up | ROM debounce needs two scans one sweep apart | `reference/SharedArchitecture.md:252-268` |
| ODD59-5 | Random 0 to half-batch jitter on batch ends | Fixed batches locked onto ROM loops (TI-58 error blink froze the CPU panel) | `reference/SharedArchitecture.md:168-174` |
| ODD59-6 | Full speed still sleeps in the scan loop and after hardware polls; printer BUSY counted in emulated cycles | Full speed should speed up printing but not idle | `reference/SharedArchitecture.md:180-187` |
| ODD59-7 | Preset parse errors are reported and the load continues without the bad line | Design choice (vs. Calc-U-1600's all-or-nothing) | `reference/StateFileFormat.md:43-45`, `ideas/PresetConvergence.md:34`, `:38-40` |
| ODD59-8 | A preset keeps the current module and printer when its lines are absent | A preset is applied to a running calculator, often as a demo | `reference/StateFileFormat.md:28-29`, `ideas/PresetConvergence.md:35-40` |
| ODD59-9 | `#` starts a comment anywhere, so no value can contain `#`; fresh-card titles replace `#` by a space | Comment syntax | `reference/StateFileFormat.md:41-42`, `Shared/CardFile.cpp:335` |
| ODD59-10 | Registers written after the program, as given, even into the program area | Some historic listings do this deliberately | `reference/StateFileFormat.md:109-112` |
| ODD59-11 | A damaged card file is refused whole; v2 ends with `END-CARD` so a v1 reader fails | No silent blanking; an older app cannot downgrade a card | `reference/StateFileFormat.md:487-490`, `Shared/CardFile.hpp:22-23` |
| ODD59-12 | Version 1 cards stay version 1 when written | Keeps their single side | `reference/StateFileFormat.md:491-495` |
| ODD59-13 | Adv in KEYSTROKES needs `RegularSpeed:` | Adv repeat is gated by CPU steps, not real time | `reference/StateFileFormat.md:299-301` |
| ODD59-14 | A load runs 300,000 power-on steps before writing RAM | Skipping power-on leaves stale SCOM flags | `reference/StateFileFormat.md:591`, `reference/SharedArchitecture.md:391-392` |
| ODD59-15 | TI-58C extra registers are `H00`–`H03`, not R60–R63 | They are not registers; RAM clear keeps them (ROM needs H00) | `reference/StateFileFormat.md:101-105`, `:593` |
| ODD59-16 | Leftmost display position shows the brighter of C and glyph, not both | One glyph per position; real hardware drives them from two chips | `reference/CoreArchitecture.md:526-531` |
| ODD59-17 | C indicator in RUN counts set `fA` bits | Matches a film of a real TI-59 | `reference/CoreArchitecture.md:516-519` |
| ODD59-18 | Zero suppression darkens ctrl 2 and 4 and turns 5 into a minus | Pictures of a real TI-59 | `reference/CoreArchitecture.md:497-501`, `Shared/SevenSegment.hpp:33-40` |
| ODD59-19 | Printer line BUSY 4165, not the measured 4168 | Printer-interrupt line darkness order fits only 4165 | `reference/CoreArchitecture.md:766-769`, `:808-821` |
| ODD59-20 | `WAIT.BUSY` is a no-op | Undocumented instruction | `reference/CoreArchitecture.md:555` |
| ODD59-21 | TRACE latch survives reset | Hardware latch | `reference/CoreArchitecture.md:611` |
| ODD59-22 | TI-58 card-veto poll (`0459`–`045B`) counted as scan loop | FREEZE ON START, key sequencing, full-speed throttle | `reference/CoreArchitecture.md:604` |
| ODD59-23 | Web: LED Classic only; cards last for the page session | Deliberately different from the apps | `reference/WebClient.md:21-27` |
| ODD59-24 | Qt macOS build never shipped; Swift is the Mac product | Release scope | `CLAUDE.md:53-55`, `.github/workflows/build.yml:4-7` |
| ODD59-25 | Qt keeps running in the background; Swift and web pause | Desktop app behaviour | `reference/FrontendParity.md:89` |
| ODD59-26 | iPhone: debug page off by default | App presents as a classic calculator | `README.md:9` |
| ODD59-27 | The session ignores core breakpoints; the loop just stops on bit 31 | Breakpoints are for headless tools only | `reference/SharedArchitecture.md:187-189`, `reference/FrontendParity.md:76` |
| ODD59-28 | On the Mac a dropped card file's security scope stays open while fitted | A write goes back to it | `reference/SharedArchitecture.md:327-328` |
| ODD59-29 | A fresh card with an existing name asks for confirmation; its first write replaces the file | Only the GUI knows its card folder | `reference/SharedArchitecture.md:291-293` |

## Release material

- `appstore` is an orphan branch checked out as a worktree at `appstore/`,
  which `.gitignore` excludes ("One-time setup per clone: git worktree add
  appstore appstore") (`.gitignore:42-44`).
  ReleaseIt uses it via `appstore.worktree: appstore` (`release.yaml:51-57`);
  `release` writes `appstore/whatsnew.txt` from the CHANGELOG (`release.yaml:4-5`).
- Content (local `appstore` @ `3e23fd2`): `metadata/description.txt`,
  `keywords.txt`, `subtitle.txt`, `marketing_url.txt` (`https://calc-u.org/59/`),
  `privacy_url.txt` (GitHub `PRIVACY.md`), `support_url.txt` (GitHub issues);
  `whatsnew.txt` (v2.3.0 notes); `screenshots/`: `iphone-01-plain-calculator`,
  `iphone-02-cpu-inspector`, `iphone-03-calc-debugger`, `iphone-04-diagnostics`,
  `iphone-05-settings`, `iphone-06-display-glyphs`, `ipad-01-assembly`,
  `ipad-02-calc-debugger`, `ipad-03-texas`, `iphoneduo-03-calc-debugger`
  (PNG). Naming and pixel sizes: `bin/run-screenshots:4-7`.
- Local `appstore` (`3e23fd2`, 2026-11-07 16:46, "QUIRKS section in the
  description; screenshots retaken, with iphone-06 display glyphs") is one
  commit ahead of `origin/appstore` (`ffd1cd2`, 2026-11-04, "whatsnew: v2.3.0").
- Screenshot sources: `examples/screentest/*` presets drive the
  `ScreenshotTests` (`reference/StateFileFormat.md:349-350`).
- Other release inputs: `bin/ExportOptions.plist` (Developer ID DMG),
  `bin/AppStoreExportOptions.plist`, `bin/install-release.sh` (installs the
  release build into /Applications, `release.yaml:27-29`), Inno Setup script
  `Qt6/resources/windows/calc-u-59.iss`, Linux desktop file and icons
  (`Qt6/resources/`), `PRIVACY.md`.

## Open questions

- **Preset format is unversioned.** Briefing §1.2 defines "old format" as
  "every explicitly versioned format the latest released Calc-U-59 reads" and
  drops unversioned legacy formats. The preset format (`.ti59`/`.ti58`/`.ti58c`)
  has no version marker at all, so every bundled and hosted preset (10 web
  presets; 45 files in `59/software-files/` and `59/tutorial-files/`, plus the
  two 10-file copies in `59/calc/presets/` and `59/beta/presets/`) falls under
  "unversioned". Is the v2.3.0
  preset syntax meant to count as the versioned "old format" (for example as
  an implicit version), or is a version to be introduced? `ideas/PresetConvergence.md:55-58`
  proposes an optional `FORMAT-VERSION: 2`. *Answered 2026-11-08:* the
  format as 2.3.0 reads it is version 1 (Briefing §1.2).
- **`ti58c.mem` is unversioned** and lives on users' devices (iCloud, `~/Calc-U-59`,
  `localStorage`). Dropping it loses TI-58C memory contents on upgrade. Intended?
  *Answered 2026-11-08:* version 1, not dropped (Briefing §1.2).
- **ASM overlay files** (`.asm`, 4 hosted on calc-u.org) are unversioned. Own or external
  (the listings come from Sladký's guide, the `PROGRAM:`/`HEX:` wrapper is Calc-U's)?
  *Answered 2026-11-09:* own, version 1 as 2.3.0 reads them (Briefing §1.2).
- **Card version 1 write-back.** Today a v1 card (text 1.0 or binary) is written
  back as text 1.0 (`Shared/CardFile.cpp:293-294`); Briefing §1.2 says anything
  written uses the new format. Recorded as fact; decision belongs to phase 3.
- **Settings** keys carry no version; whether they count as an "own format" for §1.2.
  *Answered 2026-11-09:* settings become a documented settings file, converted
  from the platform store on first start (Briefing §1.2).
- **Module slot pin-out**: not found in `ti59-research` or the product. Source needed for an explicit slot interface. *Answered 2026-11-09:*
  https://www.datamath.org/Chips/TMC0540.htm (Briefing §1.9).
- **Printer connector contact count**: answered 2026-11-09. 12 springs, one
  unconnected, and 11 pads were counted by hand on the devices; the service
  manual's "14-contact" (`ti59-research/PC100C/service_manual.txt:2`) is a
  typo (Briefing §1.9). Still open: which spring is unconnected; the
  manual lists 12 signal contacts including `I/A` (`:33-45`).
- **Research sources disagree** on the TI-59 chip set: `ti59-research/docs/architecture.md:47-58`
  names TMC0599 RAM, TMC0571 as "SCOM chip" and a 240 kHz clock; the product
  names TMC0598 RAM, TMC0571B as a ROM chip and 455 kHz (`reference/CoreArchitecture.md:34-37`, `:59-62`).
  The display decode table in research (`architecture.md:582-593`) also differs
  from the product's ctrl-nibble rules (`Shared/SevenSegment.hpp:33-51`, checked
  on a real TI-59 for 2, 4, 5). Which is authoritative for phase 4 fit tables?
- ~~**XCUITest elapsed time** and goldens-generation time~~: measured 2026-11-09 (see Tests). Open: the macOS XCUITest run, which needs a signed runner and the Accessibility permission.
- `ideas/KeypressLatch.md` refers to `Bridge/TI59MachineWrapper.h/.mm`
  (`ideas/KeypressLatch.md:141`), which does not exist at the tag (Bridge has
  `TISessionController`; the name survives only in a comment in
  `Shared/MachineModelInfo.hpp`); freeze now goes through `SessionController`
  (`App/EmulatorViewModel.swift:313-314`). The proposal predates the `Shared/` move.
