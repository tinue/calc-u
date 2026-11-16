# Frontend Parity (Swift app ↔ Qt6 app ↔ web client)

> **Provenance notice.** Maintained by hand (and by AI agents) — see
> CLAUDE.md § "Frontend Parity". Every user-visible feature has a row; a
> change to one frontend updates its row on the same branch.

All three GUIs run the same `core/` and `app/` code; this table tracks
the GUI on top. The web client (`frontends/web/`, `WebClient.md`) has two layouts:
wide (as the macOS/Qt window) and portrait (as the iPhone, no debugger).
Web paths are relative to `frontends/web/src/`; `TIWebPart` is
`app/TIWebPart` (what the page draws, as the session's `c59.web.*` topics,
since M-34), `engine.js` translates for the components.

**Status:** `All` — in all three (web: in both layouts unless the Notes say
"wide only"); `Both` — in both apps, deliberately not in the web client; `Pending (Qt)` / `Pending (Swift)` / `Pending (Web)` —
the named one still to do (the Notes say why); `Swift only` / `Qt only` /
`Web only` — deliberately platform-specific.

## Calculator

| Feature | Swift | Qt | Web | Shared | Status | Notes |
|---|---|---|---|---|---|---|
| Model picker TI-59 / TI-58 / TI-58C | `CalculatorView` | `ControlBar` | `ControlBar` | LIB-18's `model {name}` (`TIProductMachines`), `MachineModelInfo` | All | |
| Reset; clean reset (Cmd/Ctrl held, red "Clean") | `CalculatorView` | `ControlBar` | `ControlBar` | LIB-18's `reset` / `cold-boot`, `Machine::reset()`/`coldBoot()` (C59-1) | All | iOS and web touch: 1 s long press |
| Calculator artwork, key hit test (widened rects) | `KeyboardView` | `KeyboardWidget` | `CalculatorFace`, `TIWebPart` (hit test) | `KeyboardLayout` | All | Web: transparent key buttons for assistive technology |
| Pressed-key effect (blacked out, shifted 3 px) | `KeyboardView` | `KeyboardWidget` | `CalculatorFace` | — | All | |
| One key at a time; release on focus loss / teardown | `KeyboardView` | `KeyboardWidget` | `CalculatorFace`, `TIWebPart` | — | All | |
| Hold display for full speed | `KeyboardView` | `KeyboardWidget` | `CalculatorFace`, `TIWebPart` | `Pacing`, `KeyboardLayout` (`PointerTracker`) | All | |
| Physical keyboard (26 keys, Shift = 2nd, min. hold) | `KeyboardView` | `KeyboardWidget` | `PlayCalculator` (`keyboard` prop), `TIWebPart` | `TITyping` (`TIKeyboardMap`), the library's typing (LIB-5) | All | macOS only in the Swift app; web: only while the calculator has focus |
| LED display, Modernized / Classic styles | `LEDDisplayView` | `LedRenderer` | `painters.js` | `SevenSegment`, `LedGeometry` | All | Glow: SwiftUI blur / box-blur layers / Canvas 2D blur; web: Classic only |
| Digit and decimal-point brightness (persistence of vision), "C" indicator brightness | `LEDDisplayView` | `LedRenderer` | `painters.js`, `TIWebPart` | `FrameSampler`, `DisplayPov` | All | |
| Cue card: 3 templates, `\blank` spans, shrink-to-fit, inks | `CueCardView` | `CueCardRenderer` | `painters.js`, `TIWebPart` (draw list) | `CueCard` (layout, geometry, inks) | All | |
| Module program cards (SCOM[9]), user cards | `EmulatorViewModel` | `EmulatorController` | `TIWebPart` | `TIAppPart`, `CueCard` resolveCueCard | All | |
| Solid State module selection | `SettingsView` | `SettingsDialog` | `Dialogs` (Settings) | `ModuleCatalog`, LIB-18's `attach`/`detach slot-1` | All | power cycle on change (row 59-11) |
| Startup calculator (model / last used) | `SettingsView` | `SettingsDialog` | `Dialogs` (Settings) | `Settings` | All | `Calc-U-59.ini` everywhere; web: the file in the persistent file system (IndexedDB, since M-34); the `model` prop overrides it |

## Files

| Feature | Swift | Qt | Web | Shared | Status | Notes |
|---|---|---|---|---|---|---|
| Open preset `.ti59` / `.ti58` / `.ti58c` | `CalculatorView` | `MainWindow` | `Dialogs` (preset picker: bundled, upload) | LIB-6's `load-preset` (`TIPresetRules`, `TIPresetKeys`) | All | Qt6 by its path, Swift and Web uploads by their content; Web: the bundled examples of `examples/bundle.yaml` (`preset-list`); Qt6 and Swift bundle the same files (their Examples menu: M-57, M-58) |
| A preset's `keys:` steps (wait, full-speed, trace, elapsed-time, expect, screenshot) | — | — | — | LIB-6's loader, LIB-5's typing engine | All | Web: a `trace:` is handed over, downloadable from the LOG tab |
| A preset's `debug-panel:` selects the tab (the `c59.show-debug-panel` topic) | `EmulatorViewModel` (`showDebugPanel`) | `EmulatorController` → `DebugPanel` (`debugPanelRequested`) | `engine.js` `debugPanel`, `Debugger.jsx` | `TIAppPart` showDebugPanelNamed (LIB-10's step) | All | Paged layouts keep their page; web portrait has no debugger and remembers the tab |
| Sections after a `keys:` section (2.3.0's reloads); magnetic cards in presets | — | — | — | LIB-6's loader, `TIPresetKeys` | All | Cards in presets (`magnetic-cards:`, `card:`, `arm:`) come with C59-7 (M-55) |
| Drag-and-drop of presets and cards | `CalculatorView` (macOS) | `MainWindow` | `PlayCalculator`, `engine.js`, worker | `TIAppPart` dropAction (a preset: `load-preset {content, name}`; a card: fitted) | All | Not in the iOS build. A card is fitted (TI-59; ignored on the TI-58/58C). Web: a write goes to the session's copy |
| Open-with of presets | — | `main.cpp` | — | — | Qt only | |
| Magnetic cards: fit, fresh card, feed by corner, lit corner, eject, auto-eject, replace confirmation | `CalculatorView`, `KeyboardView`, `CueCardView`, `CardPickerView` | `ControlBar`, `KeyboardWidget`, `CueCardRenderer`, `CardPickerDialog` | `ControlBar`, `CalculatorFace`, `painters.js`, `Dialogs` (card picker), `TIWebPart` | `TIAppPart` (card reader), `CardFile` (formats, listed names), `CueCard` cardCornerGeometry, `KeyboardLayout` cardCornerAt | All | TI-59 only; web: upload a card to fit it, download one to keep it |
| Card and `ti58c.memory.yaml` location | iCloud Documents (`CardStorage`) | `~/Calc-U-59` (`AppPaths`) | the save folder in the persistent file system (IndexedDB `/persist-calc-u-59`, `TIWebPart`; since M-34) | — | Swift only / Qt only / Web only | Qt: card folder in Settings; web: cards and `ti58c.memory.yaml` are kept between visits; a second tab keeps nothing (LIB-15 AC-8, AC-21) |
| Open Preset start folder | system file importer | last used, or fixed in Settings | — | — | Qt only | Calc-U-1600 layout |
| TI-58C constant memory (`ti58c.memory.yaml`: restored at a build without a preset, written on change, suspend, quit) | the bridge's `FileIO` | `TIFileAccess` (LIB-9's save folder) | `TIFileAccess` (the persistent file system) | `ConstantMemoryKeeper`, `ConstantMemoryFile`, the kind `c59.constant-memory` | All | The same file in all three frontends (C59-3 AC-13) |

## Printer

| Feature | Swift | Qt | Web | Shared | Status | Notes |
|---|---|---|---|---|---|---|
| PC-100C connect toggle (resets) | `PrinterView` | toggle above the paper panel (`MainWindow`) | toggle above `CalcuPaperPanel` | `TIAppPart` (`c59.printer` connected; LIB-8 P-E1 until M-57) | All | |
| Dot view at PC-100C proportions; text view | `PrinterView` (from the `paper PC-100C` marks, S-P1 until C59-8) | library `PaperPanel` | library `CalcuPaperPanel` | `core/PC100CPaper` (marks), LIB-8 `PaperPart` | All | |
| Printer interrupt: cut-short lines, the interrupted row's fired columns, faint dots | `PrinterView` (the marks' darkness) | library `PaperPanel` | library `CalcuPaperPanel` | `core/PC100CPaper` (`lineDots`: `drawnRows`, `inkedColumns`, `faintColumns`; greys of `inkAlpha`) | All | |
| PRINT / ADV (hold), T (TRACE switch) | `PrinterView` (`set-control`) | library `PaperPanel` (controls) | library `CalcuPaperPanel` (controls) | `TIAppPart` answers `set-control` (LIB-8 P-E2 until M-54) | All | |
| Copy (text + 600 dpi image), cut | `PrinterView` (`paper.copy`, `paper.cut`; the bridge answers `clipboard-image`) | library `PaperPanel` (`ClipboardService`) | library `CalcuPaperPanel` (`hostServices.js`) | LIB-8 `PaperPart` (picture, cut) | All | web: image where the browser's clipboard takes one, else text |

## Debugger

| Feature | Swift | Qt | Web | Shared | Status | Notes |
|---|---|---|---|---|---|---|
| CALCULATOR tab: registers, flags, HIR, T, return stack, partition, source | `LiveDebugView` | `LiveDebugWidget` | `Debugger` (`LiveDebug`) | `DebugModel`, `DebugSnapshots`, `DebugText`, `ModuleImage` | All | web: wide only |
| Program steps ±5 running / full listing frozen | `LiveDebugView` | `LiveDebugWidget` | `Debugger` (`LiveDebug`) | `DebugModel` (listing, SolidStateReturn advance), `DebugSnapshots` | All | web: wide only |
| FREEZE, FREEZE ON START (next step), ARMED, RESUME, STEP (hold to repeat) | `LiveDebugView` (the bridge's `ButtonRule`) | `LiveDebugWidget` (library `RunControlButtons`, level `program`) | `Debugger` (`CalcuRunControlButtons`, level `program`) | LIB-10 run control (`pause`, `step`, `resume`, `pause-on-start`), `TILevelRules` | All | web: wide only |
| CPU tab: history, registers, control registers, display state | `CPUInspectorView` | `CpuInspectorWidget` | `Debugger` (`CpuInspector`) | `DebugModel` (cpuHistory), `DebugSnapshots`, `DebugText` | All | web: wide only |
| CPU FREEZE ON START (scan-loop exit), STEP | `CPUInspectorView` (the bridge's `ButtonRule`) | `CpuInspectorWidget` (library `RunControlButtons`, level `cpu`) | `Debugger` (`CalcuRunControlButtons`, level `cpu`) | LIB-10 run control, `TILevelRules` | All | web: wide only |
| ROM heatmap with hover address, CLR | `CPUInspectorView` | `RomHeatmapWidget` | `Debugger` (`Heatmap`) | `DebugModel`, `RomHeatmap` | All | web: wide only |
| ASM overlay: select, run, clear | `DebugView` | `AsmOverlayBox` | `Debugger` (`AsmOverlay`, upload) | `TIAppPart` loadAsm/runAsm/clearAsm, `AsmOverlay` | All | web: wide only |
| LOG tab: dumps, clear, log level | `DebugView` | `LogWidget` | `Debugger` (`LogTab`) | `TIAppPart` (dumps, level), `DebugModel` (RingLog), `DebugLog` | All | web: wide only |
| LOG kept across model switch | `DebugView` | `LogWidget` | `Debugger` (`LogTab`) | `DebugModel` (the LOG survives `attach`) | All | web: wide only |
| Binary trace (TRACE toggle), size limit | `DebugView` (`trace {button}`) | `LogWidget` (`trace {button}`) | `Debugger` (`LogTab`; the list of the traces handed over) | LIB-10 trace session (one at a time), `TITraceRecorder`, `TraceWriter` | All | web: wide only; handed over (downloaded) when switched off |
| Trace location | Local / iCloud / custom | card folder, or trace folder in Settings | none: `keep-file` downloads | — | Swift only / Qt only / Web only | iCloud: Swift only; web: no location to choose |
| Breakpoints | — | — | — | LIB-10 run control (the harness's `break`) | — | no UI in any GUI (`debugger.md` DB-3); an app session pauses at one set through the harness (LIB-10 AC-31) |

## App

| Feature | Swift | Qt | Web | Shared | Status | Notes |
|---|---|---|---|---|---|---|
| About (version, links) | `AppInfo` | `AboutDialog` | `Dialogs` (About) | — | All | |
| Settings | `SettingsView` | `SettingsDialog` | `Dialogs` (Settings) | `Settings` | All | Qt: preset, card and trace folders instead of the trace location; Swift: Keyboard Feedback and Debug Page in Portrait on iOS only; web: no folders, no segment style, Keyboard Feedback in portrait and the PWA only, Maximum Trace File Size whenever the pages sit side by side (the wide layout, the PWA in landscape) |
| Help (user guide link) | — | Help menu | `Dialogs` (Settings, About) | — | Pending (Swift) | opens `https://calc-u.org/59/`; the Swift app has no Help command yet |
| iOS layouts (portrait pages, iPhone; side by side, iPad and iPhone Duo), haptics, click sound | `CalculatorView`, `KeyboardView` | — | `PlayCalculator` (portrait layout; the PWA's `auto` layout, keyboard feedback) | `DebugText` (`mobileLayout`) | Swift only / Web only | Swift: iOS only: pages when tall or folded; side by side when wide: calculator and printer, plus the debugger on iPad and iPhone Duo; web: the PWA picks the same layouts by the window's size (no fold), click via WebAudio only (no haptics in a browser) |
| UI test automation hooks | `CalculatorView` | — | — | — | Swift only | |
| `--grab <png>` offscreen screenshot | — | `main.cpp` | — | — | Qt only | developer check |
| Installable PWA, offline | — | — | `app/` | — | Web only | layout follows the window as on iOS (rotates; pages, or side by side); all three models, every module and preset offline |
| Pause while hidden / in the background | `Calc-U-59App` (`scenePhase`), `EmulatorViewModel` | — | the library's `shell.js` (`visibilitychange`: `suspend` / `continue`) | the session's `suspend` / `continue` commands; `TIAppPart::suspending` persists | Swift only / Web only | Qt keeps running, as a desktop app does |
