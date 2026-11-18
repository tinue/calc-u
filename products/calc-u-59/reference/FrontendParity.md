# Frontend Parity (Swift app ↔ Qt6 app ↔ web client)

> **Provenance notice.** Maintained by hand (and by AI agents) — see
> CLAUDE.md § "Frontend Parity". Every user-visible feature has a row; a
> change to one frontend updates its row on the same branch.

All three GUIs run the same `core/` and `app/` code; this table tracks
the GUI on top. The web client (`frontends/web/`, `WebClient.md`) has two layouts:
wide (as the macOS/Qt window) and portrait (as the iPhone, no debugger).
Web paths are relative to `frontends/web/src/`; `TIWebPart` is
`app/TIWebPart` (what the page draws, as the session's `c59.web.*` topics
and product requests). The Qt6 and Web windows are
put together from the library's shell (the family structure: the menus, the
control bar, the More button where there is no menu bar, the settings
dialog, About, Help, notices, "Loading…"), drawn from the control state
with the product's data `app/TIShellProduct`; "shell" in the table means
that (Qt6 `calcu::qt6::Shell`, Web the React set's shell components).
The Swift frontend draws the same models natively:
"Swift shell" is `Bridge/TIShell` (the library's models over the same
topics) drawn by `ShellCommands` (the menus), `ControlBarView` (the bar and
More on iOS), `SettingsView`, the notice alert and "Loading…".

**Status:** `All` — in all three (web: in both layouts unless the Notes say
"wide only"); `Both` — in both apps, deliberately not in the web client; `Pending (Qt)` / `Pending (Swift)` / `Pending (Web)` —
the named one still to do (the Notes say why); `Swift only` / `Qt only` /
`Web only` — deliberately platform-specific.

## Calculator

| Feature | Swift | Qt | Web | App layer | Status | Notes |
|---|---|---|---|---|---|---|
| Model picker TI-59 / TI-58 / TI-58C | Swift shell (the bar's model picker, first; Machine ▸ Model on macOS and the iPad's menu bar) | shell (bar's model picker; Machine ▸ Model) | shell (bar's model picker; Machine ▸ Model in the wide layout) | the library's `model {name}` (`TIProductMachines`); the library's `control-state` | All | |
| Reset; Cold boot | Swift shell (macOS: Machine ▸ Reset ⌘R, Cold boot ⇧⌘R; iOS: the bar's Reset and More ▸ Cold boot, which asks first) | shell (Machine ▸ Reset Ctrl+R, Cold boot Ctrl+Shift+R; no bar button) | shell (wide: Machine menu; portrait: the bar's Reset and More ▸ Cold boot, which asks first) | the library's `reset` / `cold-boot`, `Machine::reset()`/`coldBoot()` | All | |
| Calculator artwork, key hit test (widened rects) | `KeyboardView` | `KeyboardWidget` | `CalculatorFace`, `TIWebPart` (hit test) | `KeyboardLayout` | All | Web: transparent key buttons for assistive technology |
| Pressed-key effect (blacked out, shifted 3 px) | `KeyboardView` | `KeyboardWidget` | `CalculatorFace` | — | All | |
| One key at a time; release on focus loss / teardown | `KeyboardView` | `KeyboardWidget` | `CalculatorFace`, `TIWebPart` | — | All | |
| Hold display for full speed | `KeyboardView` | `KeyboardWidget` | `CalculatorFace`, `TIWebPart` | `Pacing`, `KeyboardLayout` (`PointerTracker`) | All | |
| Physical keyboard (26 keys, Shift = 2nd, min. hold) | `KeyboardView` | `KeyboardWidget` | `CalcU59` (`keyboard` prop), `TIWebPart` | `TITyping` (`TIKeyboardMap`), the library's typing | All | macOS only in the Swift app; web: only while the calculator has focus |
| LED display, Modernized / Classic styles | `LEDDisplayView` | `LedRenderer` | `painters.js` | `SevenSegment`, `LedGeometry` | All | Glow: SwiftUI blur / box-blur layers / Canvas 2D blur; web: Classic only |
| Digit and decimal-point brightness (persistence of vision), "C" indicator brightness | `LEDDisplayView` | `LedRenderer` | `painters.js`, `TIWebPart` | `FrameSampler`, `DisplayPov` | All | |
| Cue card: 3 templates, `\blank` spans, shrink-to-fit, inks | `CueCardView` | `CueCardRenderer` | `painters.js`, `TIWebPart` (draw list) | `CueCard` (layout, geometry, inks) | All | |
| Module program cards (the product kind `c59.library-program`), fitted cards | `EmulatorViewModel` | `EmulatorController` | `TIWebPart` | `TIAppPart` refreshCueCard (`c59.cue-card`), `ModuleCueCards` (the module files) | All | |
| Solid State module selection | Swift shell (bar's picker of `slot-1`; the empty slot) | shell (bar's picker of `slot-1`, the library's `MediaPicker`; the empty slot) | shell (bar's picker of `slot-1`, the library's `CalcuMediaPicker`; the empty slot) | `ModuleCatalog`, the library's `media.choose {place: slot-1}` | All | a change power-cycles the machine (the TI-58C keeps its constant memory); the choice is not remembered across starts, a startup preset sets it |
| Startup model (model / last used) | `SettingsView` | shell (Settings ▸ General, "Last used" first) | shell (Settings ▸ General) | `Settings`, the library's `SettingsPart` | All | `Calc-U-59.ini` everywhere; web: the file in the persistent file system (IndexedDB); the `model` prop overrides it |

## Files

| Feature | Swift | Qt | Web | App layer | Status | Notes |
|---|---|---|---|---|---|---|
| Load Preset… `.ti59` / `.ti58` / `.ti58c`; Examples ▸ | Swift shell (macOS: File ▸ Load Preset… ⌘O, File ▸ Examples ▸; iOS: the bar's Preset and More ▸ Examples ▸; the dialog takes the preset types and any file) | shell (File ▸ Load Preset… Ctrl+O, `Presets (*.ti59 *.ti58 *.ti58c)`, sends `load-preset {ref}`; File ▸ Examples ▸) | shell (wide: File ▸ Load Preset…, Examples ▸; portrait: the bar's Preset and More ▸ Examples ▸; an upload goes to the library's `open {way: dialog}`) | the library's `open` (`TIFileKinds`), the library's `load-preset`, `preset-list` (`TIPresetRules`, `TIPresetKeys`) | All | Qt6 by its path, Swift and Web uploads by their content; Web: several files are one folder; the bundled examples of `examples/bundle.yaml` |
| A preset's `keys:` steps (wait, full-speed, trace, elapsed-time, expect, screenshot) | — | — | — | the library's preset loader and typing engine | All | Web: a `trace:` is handed over, downloadable from the LOG tab; a `screenshot:` is a download |
| A preset's `debug-panel:` selects the tab (the product topic `c59.debug-view`, its `requests` count) | `EmulatorViewModel` (`showDebugPanel`) | `EmulatorController` → `DebugPanel` (`debugPanelRequested`) | `engine.js` `debugPanel`, `Debugger.jsx` | `TIDebugPart::showView` (the library debugger's step; `calculator` passed over while an overlay runs) | All | Paged layouts keep their page; web portrait has no debugger and remembers the tab |
| Sections after a `keys:` section; magnetic cards in presets | — | — | — | the library's preset loader, `TIPresetKeys`, `TIMagneticCardsKey`, the library's `card:`, `arm:`, `saveas: { card: }` | All | |
| Drag-and-drop of any one file | `CalculatorView` (macOS; `open-would-take`, then `open {way: drop}`) | `MainWindow` (`open-would-take`, then `open {way: drop}`) | `CalcU59` (`open {way: drop, upload: true}`) | the library's `open` (a `.magnetcard.yaml`: the library's MediaOpener, fitted on the TI-59 (refused on the TI-58/58C), imported first on the Web; `TIFileKinds`: presets, `.asm`, `.keystrokes.yaml`; `.rom.yaml` and `ti58c.memory.yaml` refused) | All | Not in the iOS build. One file per drop (more: ignored); Qt6 raises the window and opens after the drop. A refusal is a notice. A card is fitted where it lies (TI-59; refused with a notice on the TI-58/58C). Web: an upload, a card goes into the save folder |
| Open-with of presets; a file named at the start | — | `main.cpp` (`QFileOpenEvent`: `open {way: open-with}`; the start file by `opening::startFile` after the program's own options, `open {way: start}`) | — | `programArguments` (`TIFileKinds`) | Qt only | The three preset extensions on macOS, Windows, Linux (`CalcU59OpenWithTypes`); opened as what it is |
| Magnetic cards: Arm Card, Arm Fresh Card, feed by corner, lit corner, Eject Card, given back after a pass, replace question | `CalculatorView`, `KeyboardView`, `CueCardView`, `CardPickerView` | `CardControls` on the shell's bar, File ▸ Arm Card… / Arm Fresh Card…, `KeyboardWidget` (the corner tap the shell lets through), `CueCardRenderer`, `CardPickerDialog` | the card controls on the shell's bar, File ▸ Arm Card… / Arm Fresh Card…, `CalculatorFace`, `painters.js`, `Dialogs` (card picker), `TIWebPart` | the library's media part and topic `media` (place `card-reader`), `TIMagneticCardKind`, `core/TICardReaderPlace`, `CueCard` cardCornerGeometry, `KeyboardLayout` cardCornerAt (`c59.tap-card`) | All | TI-59 only (the control state's media place); web: Import Media… and Saved Media… in File (wide) or More (portrait) |
| Card and `ti58c.memory.yaml` location | iCloud Documents (`CardStorage`) | `~/Calc-U-59` (`AppPaths`) | the save folder in the persistent file system (IndexedDB `/persist-calc-u-59`, `TIWebPart`) | — | Swift only / Qt only / Web only | Qt: the save folder in Settings; web: cards and `ti58c.memory.yaml` are kept between visits; a second tab keeps nothing |
| Load Preset… start folder | system file importer | last used, or fixed in Settings ▸ Folders ▸ Preset folder (`remember-folder`) | — | the library's `SettingsPart` | Qt only | Calc-U-1600 layout |
| TI-58C constant memory (`ti58c.memory.yaml`: restored at a build without a preset, written on change, suspend, quit) | `TIFileAccess` through the bridge's `CoordinatedFiles` (`media::FileAccess`) | `TIFileAccess` (the library's save folder) | `TIFileAccess` (the persistent file system) | `ConstantMemoryKeeper`, `ConstantMemoryFile`, the kind `c59.constant-memory` | All | The same file in all three frontends |

## Printer

| Feature | Swift | Qt | Web | App layer | Status | Notes |
|---|---|---|---|---|---|---|
| PC-100C plugged in or out (resets) | Swift shell (bar's "PC-100C" button; Machine ▸ Peripherals ▸ on macOS) | shell (bar's "PC-100C" button; Machine ▸ Peripherals ▸) | shell (bar's "PC-100C" button) | the library's `attach`/`detach printer` and `control-state` | All | a change power-cycles the machine |
| Dot view at PC-100C proportions; text view | `PrinterView` (the paper panel from the `paper PC-100C` marks; its owner's name, an empty paper under none) | library `PaperPanel` | library `CalcuPaperPanel` | `core/PC100CPaper` (marks), the library's `PaperPart` | All | |
| Printer interrupt: cut-short lines, the interrupted row's fired columns, faint dots | `PrinterView` (the marks' darkness) | library `PaperPanel` | library `CalcuPaperPanel` | `core/PC100CPaper` (`lineDots`: `drawnRows`, `inkedColumns`, `faintColumns`; greys of `inkAlpha`) | All | |
| PRINT / ADV (hold), T (TRACE switch) | `PrinterView` (`set-control`) | library `PaperPanel` (controls) | library `CalcuPaperPanel` (controls) | the library's `PeripheralsPart` answers `set-control`; the positions in the library's `control-state` (`controls`); TRACE's position kept by `TIAppPart` while detached | All | |
| Copy (text + 600 dpi image), cut | `PrinterView` (`paper.copy`, `paper.cut`; the bridge answers `clipboard-image`) | library `PaperPanel` (`ClipboardService`) | library `CalcuPaperPanel` (`hostServices.js`) | the library's `PaperPart` (picture, cut) | All | web: image where the browser's clipboard takes one, else text |

## Debugger

| Feature | Swift | Qt | Web | App layer | Status | Notes |
|---|---|---|---|---|---|---|
| CALCULATOR tab: registers, flags, HIR, T, return stack, partition, source | `LiveDebugView` | `LiveDebugWidget` | `Debugger` (`LiveDebug`) | `TIDebugPart` (`c59.calculator-debug`), `DebugModel`, `DebugSnapshots`, `DebugText`, `ModuleImage` | All | web: wide only; cannot be chosen while an ASM overlay runs (`c59.calculator-tab`) |
| Program steps ±5 running / full listing frozen | `LiveDebugView` | `LiveDebugWidget` | `Debugger` (`LiveDebug`) | `TIDebugPart` (`c59.program-listing`), `DebugModel` (listing, SolidStateReturn advance), `DebugSnapshots` | All | web: wide only |
| FREEZE, FREEZE ON START (next step), ARMED, RESUME, STEP (hold to repeat) | `LiveDebugView` (the bridge's `ButtonRule`, `freezeButtons`) | `LiveDebugWidget` (library `RunControlButtons`, level `program`; `freezeButtons`) | `Debugger` (`CalcuRunControlButtons`, level `program`) | the library's run control (`pause`, `step`, `resume`, `pause-on-start`), `TILevelRules`, `freezeButtons` | All | web: wide only |
| CPU tab: history, registers, control registers, display state | `CPUInspectorView` | `CpuInspectorWidget` | `Debugger` (`CpuInspector`) | `TIDebugPart` (`c59.cpu-debug`), `DebugModel` (cpuHistory), `DebugSnapshots`, `DebugText` | All | web: wide only |
| CPU FREEZE ON START (scan-loop exit), STEP | `CPUInspectorView` (the bridge's `ButtonRule`) | `CpuInspectorWidget` (library `RunControlButtons`, level `cpu`) | `Debugger` (`CalcuRunControlButtons`, level `cpu`) | the library's run control, `TILevelRules` | All | web: wide only |
| ROM heatmap with hover address, CLR | `CPUInspectorView` | `RomHeatmapWidget` | `Debugger` (`Heatmap`) | `TIDebugPart` (`heatmap`, `c59.heatmap-reset`), the core's `c59.rom-heatmap`, `RomHeatmap` (steps, colours) | All | web: wide only |
| ASM overlay: select, run, clear; `.asm` opened by any way | `DebugView` | `AsmOverlayBox` | `Debugger` (`AsmOverlay`, upload) | `TIDebugPart` (`c59.asm-overlay`, `c59.asm-load`/`-run`/`-clear`, the entry `c59.asm-run`), `AsmOverlay` (format 2.0), the core's `c59.asm-overlay` | All | web: wide only |
| LOG tab: dumps, clear, log level | `DebugView` | `LogWidget` | `Debugger` (`LogTab`) | `TIDebugPart` (`c59.dump`, `c59.log-level`, `c59.debug-view.logLevel`), the library's `log`, `DebugLog` (`RingLog`, the frontends' view) | All | web: wide only; clear empties the view only |
| LOG kept across model switch | `DebugView` | `LogWidget` | `Debugger` (`LogTab`) | the frontends' view of the library's `log` | All | web: wide only |
| Binary trace (TRACE toggle), size limit | `DebugView` (`trace {button}`) | `LogWidget` (`trace {button}`) | `Debugger` (`LogTab`; the list of the traces handed over) | the library's trace session (one at a time), `TITraceRecorder`, `TraceWriter` | All | web: wide only; handed over (downloaded) when switched off; the LOG tab lists the traces only; where the trace folder cannot be written the start is refused |
| Trace location | Local / iCloud / custom | save folder, or trace folder in Settings | none: `keep-file` downloads | — | Swift only / Qt only / Web only | iCloud: Swift only; web: no location to choose |
| Notices, "Loading…" | Swift shell (an alert with the notice and OK, one after the other; "Loading…" below the display) | shell | shell | the library's `notices`, `NoticeQueue`, `LoadingRule` | All | several notices are shown one after the other; "Loading…" while a preset or another operation runs at full speed for more than half a second |
| Breakpoints | — | — | — | the library's run control (the harness's `break`) | — | no UI in any GUI; an app session pauses at one set through the harness |

## App

| Feature | Swift | Qt | Web | App layer | Status | Notes |
|---|---|---|---|---|---|---|
| About (icon, version, summary, links) | `AppInfo` (macOS: the system's About panel with `TIShellProduct`'s texts; none on iOS) | shell (Help ▸ About Calc-U-59; the window icon, "Built with Qt") | shell (wide: Help ▸ About; portrait: More ▸ About; no icon) | `TIShellProduct` | All | |
| Settings | Swift shell (`settingsSpec(SwiftMac/SwiftIOS)`: General, Startup presets, Display, Keyboard and Layout (iOS), Tracing; macOS the settings window, iOS a sheet with Done) | shell (`cu59::settingsSpec(Qt6)`: General, Startup presets, Storage, Folders, Tracing, Display) | shell (`settingsSpec(WebWide/WebPortrait)`: General, Startup presets, Tracing (wide), Keyboard (portrait); no Settings with the prop `settings` off) | `Settings`, the library's `SettingsPart`, `TIShellProduct` | All | every change applied at once; Swift: Keyboard Feedback and Debug Page in Portrait on iOS only |
| Help (user guide link) | Swift shell (macOS and the iPad's menu bar: Help ▸ Calc-U-59 Help; iOS: More ▸ Calc-U-59 Help; also without the ROMs) | shell (Help ▸ Calc-U-59 Help) | shell (wide: Help menu; portrait: More ▸ Calc-U-59 Help) | `TIShellProduct` | All | opens `https://calc-u.org/59/` |
| iOS layouts (portrait pages, iPhone; side by side, iPad and iPhone Duo), haptics, click sound | `CalculatorView`, `KeyboardView` | — | `CalcU59` (portrait layout; the PWA's `auto` layout, keyboard feedback) | `DebugText` (`mobileLayout`) | Swift only / Web only | Swift: iOS only: pages when tall or folded; side by side when wide: calculator and printer, plus the debugger on iPad and iPhone Duo; web: the PWA picks the same layouts by the window's size (no fold), click via WebAudio only (no haptics in a browser) |
| UI test launch argument (`--settings-file`, Debug only) | `AppSettings` | — | — | — | Swift only | no other test aids in the app (CTest `CalcU59NoTestAids`); the tests drive the session through harness attach |
| `--grab <png>` offscreen screenshot | — | `main.cpp` | — | — | Qt only | developer check |
| Installable PWA, offline | — | — | `app/` | — | Web only | layout follows the window as on iOS (rotates; pages, or side by side); all three models, every module and preset offline |
| Pause while hidden / in the background | `Calc-U-59App` (`scenePhase`), `EmulatorViewModel` | — | the library's `shell.js` (`visibilitychange`: `suspend` / `continue`) | the session's `suspend` / `continue` commands; `TIAppPart::suspending` persists | Swift only / Web only | Qt keeps running, as a desktop app does |
