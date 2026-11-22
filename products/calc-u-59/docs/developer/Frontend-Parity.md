# Frontend Parity (Swift app ↔ Qt6 app ↔ web client)

All three GUIs run the same `core/` and `app/` code; this table tracks
the GUI on top. Every user-visible feature has a row; a change to one
frontend updates its row on the same branch. The web client (`frontends/web/`, `Web-Client.md`) has two layouts:
wide (as the desktop window, the Qt `MainWindow`) and portrait (as the iPhone, no debugger).
The Qt window and the Web's `auto` layout pick their view with the
library's view rule (`shell::view`): the printer (and the debugger, where
there is room) beside the faceplate where the room beside takes it, else
the pages; the iPhone app stays upright in portrait.
The platform class is apart from the layout: Qt is desktop (a menu bar),
the Swift frontend mobile (none), the Web desktop in a browser tab on a
computer and mobile when installed or when the browser asks for the
mobile site (the library's `calcuShellMobile`; `?mobile=` in a test).
Web paths are relative to `frontends/web/src/`; `TIWebPart` is
`app/TIWebPart` (what the page draws, as the session's `c59.web.*` topics
and product requests). The Qt6 and Web windows are
put together from the library's shell (the family structure: the menus, the
control bar with the More button at its right edge, the settings
dialog, About, Help, notices, "Loading…"), drawn from the control state
with the product's data `app/TIShellProduct`; "shell" in the table means
that (Qt6 `calcu::qt6::Shell`, Web the React set's shell components).
The Swift frontend is the iPhone and iPad app (iOS and iPadOS only; the
Mac app is the Qt6 app) and draws the same models natively:
"Swift shell" is `Bridge/TIShell` (the library's models over the same
topics) drawn by `ControlBarView` (the bar and More), `SettingsView`, the notice alert and "Loading…".

**Status:** `All` — in all three (web: in both layouts unless the Notes say
"wide only"); `Both` — in both apps, deliberately not in the web client; `Pending (Qt)` / `Pending (Swift)` / `Pending (Web)` —
the named one still to do (the Notes say why); `Swift only` / `Qt only` /
`Web only` — deliberately platform-specific; `Qt and Web` — deliberately
not in the Swift app (desktop and browser tools).

## Calculator

| Feature | Swift | Qt | Web | App layer | Status | Notes |
|---|---|---|---|---|---|---|
| Model picker TI-59 / TI-58 / TI-58C | Swift shell (the bar's model picker, first; More ▸ Model ▸) | shell (bar's model picker; Machine ▸ Model) | shell (bar's model picker; Machine ▸ Model in the wide layout) | the library's `model {name}` (`TIProductMachines`); the library's `control-state`; the bar's names 59 / 58 / 58C from `TIShellProduct`'s `barModelLabels` | All | the menus keep TI-59 / TI-58 / TI-58C |
| Reset; Cold boot | Swift shell (More ▸ Reset and Cold boot, which asks first) | shell (Machine ▸ Reset Ctrl+R, Cold boot Ctrl+Shift+R; More ▸ Reset and Cold boot, which asks first) | shell (desktop: Machine menu; More ▸ Reset and Cold boot, which asks first, on every bar) | the library's `reset` / `cold-boot`, `Machine::reset()`/`coldBoot()` | All | |
| More (⋯) at the bar's right edge: the menus' items without the model, Quit and (where the bar or the paper panel shows them) the places | Swift shell (`ControlBarView`'s More from `TIShell.bar`; Cold boot's question on the button) | shell (`ShellControlBar`'s More button; Cold boot asks in a window-modal question) | shell (`ShellMoreMenu`) | the library's bar model (`BarControl::Kind::More`, `moreItems`), `MenuModel` | All | on every bar, desktop included; Load Preset… and Settings… are in it, no bar has their buttons |
| Calculator artwork, key hit test (widened rects) | `KeyboardView` | `KeyboardWidget` | `CalculatorFace`, `TIWebPart` (hit test) | `KeyboardLayout` | All | Web: transparent key buttons for assistive technology |
| Pressed-key effect (blacked out, shifted 3 px) | `KeyboardView` | `KeyboardWidget` | `CalculatorFace` | — | All | |
| One key at a time; release on focus loss / teardown | `KeyboardView` | `KeyboardWidget` | `CalculatorFace`, `TIWebPart` | — | All | |
| Hold display for full speed | `KeyboardView` | `KeyboardWidget` | `CalculatorFace`, `TIWebPart` | `Pacing`, `KeyboardLayout` (`PointerTracker`) | All | |
| Physical keyboard (26 keys, Shift = 2nd, min. hold) | — | `KeyboardWidget` | `CalcU59` (`keyboard` prop), `TIWebPart` | `TITyping` (`TIKeyboardMap`), the library's typing | Qt and Web | the Swift app is played by touch; web: only while the calculator has focus |
| LED display, Modernized / Classic styles | `LEDDisplayView` | `LedRenderer` | `painters.js` | `SevenSegment`, `LedGeometry` | All | Glow: SwiftUI blur / box-blur layers / Canvas 2D blur; web: Classic only |
| Digit and decimal-point brightness (persistence of vision), "C" indicator brightness | `LEDDisplayView` | `LedRenderer` | `painters.js`, `TIWebPart` | `FrameSampler`, `DisplayPov` | All | |
| Cue card: 3 templates, `\blank` spans, shrink-to-fit, inks | `CueCardView` | `CueCardRenderer` | `painters.js`, `TIWebPart` (draw list) | `CueCard` (layout, geometry, inks) | All | |
| Module program cards (the product kind `c59.library-program`), fitted cards | `EmulatorViewModel` | `EmulatorController` | `TIWebPart` | `TIAppPart` refreshCueCard (`c59.cue-card`), `ModuleCueCards` (the module files) | All | |
| Solid State module selection | Swift shell (bar's picker of `slot-1` from the `media` entries, `TIAppSession.pickerEntries`; the empty slot) | shell (bar's picker of `slot-1`, the library's `MediaPicker`; the empty slot) | shell (bar's picker of `slot-1`, the library's `CalcuMediaPicker`; the empty slot) | `ModuleCatalog`, the library's `media.choose {place: slot-1}`; the `media` topic's entries in the product's listing order, the `chosen` one, each one's `short` | All | a change power-cycles the machine (the TI-58C keeps its constant memory); the choice is not remembered across starts, a startup preset sets it; closed, the picker shows the module's ID (ML) |
| The control bar's look and width | Swift shell (`ControlBarView` from `TIShell.bar`: `groupPushed`, `shown`/`shortShown`; `CalculatorView` measures the bar with and without labels, `TIBarFit`) | shell (`ShellControlBar`: pushed groups, short pickers; `CalculatorPanel::fitBar`, `calcu::shell::barFit`) | shell (`ShellControlBar`: pushed groups, short pickers; `CalcU59`'s `useBarFit`, `calcuShellBarFit`) | `TIShellProduct`'s `barSpread`, `barShortPickers`; the library's bar model (`BarGroupModel::pushed`, `BarControl::shown`) and `barFit` | All | model and module left, the cards in the middle, More right (pushed on every platform); labels ("Crd") only where the whole bar fits the faceplate with them; the bar as wide as the faceplate, wider where the icons alone need more; Qt: the bar's and the paper's text grows with the faceplate (`TIChrome`'s `textScale`) |
| The chrome: colours, panel header and button-bar heights, pills, the inset, the printer column's width range | Swift (the bridge's `TIChrome`, `ChromeControlStyles.swift`'s `Chrome`) | `ChromeStyle.hpp` over `TIChrome`, the component set's `qt6/ChromeStyle` (`PaperPanel::setChrome`) | `calc.css` custom properties from `c59.shell-product`'s `chrome` (`calcuChromeStyle`) | `TIChrome` (the library's `shell::Chrome`) | All | panels side by side line up: the printer header and the debugger's tab row 36 high, the printer's, the debugger's and the control bar's button bars 50; no colour literal in a frontend (CTest `CalcU59ChromeColours`); every GUI takes the chrome's printer width, 240–400 |
| Startup model (model / last used) | `SettingsView` | shell (Settings ▸ General, "Last used" first) | shell (Settings ▸ General) | `Settings`, the library's `SettingsPart` | All | `Calc-U-59.ini` everywhere; web: the file in the persistent file system (IndexedDB); the `model` prop overrides it |

## Files

| Feature | Swift | Qt | Web | App layer | Status | Notes |
|---|---|---|---|---|---|---|
| Load Preset… `.ti59` / `.ti58` / `.ti58c`; Examples ▸ | Swift shell (More ▸ Load Preset… and Examples ▸; the file importer takes the preset types only) | shell (File ▸ Load Preset… Ctrl+O, `Presets (*.ti59 *.ti58 *.ti58c)`, sends `load-preset {ref}`; File ▸ Examples ▸; the same in More) | shell (desktop: File ▸ Load Preset…, Examples ▸; More ▸ Load Preset…, Examples ▸ on every bar; the dialog takes `presetUploads`, the presets and the files a preset names, chosen together; they go to the library's `open {way: dialog}`) | the library's `open` (`TIFileKinds`), the library's `load-preset`, `preset-list` (`TIPresetRules`, `TIPresetKeys`) | All | Qt6 by its path, Swift and Web uploads by their content; Web: the files chosen are one folder, a file the preset names is found there by its name; the bundled examples of `examples/bundle.yaml` |
| A preset's `keys:` steps (wait, full-speed, trace, elapsed-time, expect, screenshot) | — | — | — | the library's preset loader and typing engine | All | Web: a `trace:` is handed over, downloadable from the LOG tab; a `screenshot:` is a download |
| A preset's `debug-panel:` selects the tab (the product topic `c59.debug-view`, its `requests` count) | `EmulatorViewModel` (`showDebugPanel`) | `EmulatorController` → `DebugPanel` (`debugPanelRequested`) | `Debugger.jsx` (`c59.debug-view`) | `TIDebugPart::showView` (the library debugger's step; `calculator` passed over while an overlay runs) | All | Paged layouts keep their page; web portrait has no debugger and remembers the tab |
| Sections after a `keys:` section; magnetic cards in presets | — | — | — | the library's preset loader, `TIPresetKeys`, `TIMagneticCardsKey`, the library's `card:`, `arm:`, `saveas: { card: }` | All | |
| Drag-and-drop of any one file | — | `MainWindow` (`open-would-take`, then `open {way: drop}`) | `CalcU59` (`open {way: drop, upload: true}`) | the library's `open` (a `.magnetcard.yaml`: the library's media part, fitted on the TI-59 (refused on the TI-58/58C), imported first on the Web; `TIFileKinds`: presets, `.asm`, `.keystrokes.yaml`; `.rom.yaml` and `ti58c.memory.yaml` refused) | Qt and Web | One file per drop (more: ignored); Qt6 raises the window and opens after the drop. A refusal is a notice. A card is fitted where it lies (TI-59; refused with a notice on the TI-58/58C). Web: an upload, a card goes into the save folder |
| Open-with of presets; a file named at the start | — | `main.cpp` (`QFileOpenEvent`: `open {way: open-with}`; the start file by `opening::startFile` after the program's own options, `open {way: start}`) | — | `programArguments` (`TIFileKinds`) | Qt only | The three preset extensions on macOS, Windows, Linux (`CalcU59OpenWithTypes`); opened as what it is. The Mac app has one window; a file never opens another |
| Magnetic cards: Arm Card, Arm Fresh Card, feed by corner, lit corner, Eject Card, given back after a pass, replace question | `CalculatorView`, `KeyboardView`, `CueCardView`, `CardPickerView` | `CardControls` on the shell's bar, File ▸ Arm Card… / Arm Fresh Card…, `KeyboardWidget` (the corner tap the shell lets through), `CueCardRenderer`, `CardPickerDialog` | the card controls on the shell's bar, File ▸ Arm Card… / Arm Fresh Card…, `CalculatorFace`, `painters.js`, `Dialogs` (card picker), `TIWebPart` | the library's media part and topic `media` (place `card-reader`), `TIMagneticCardKind`, `core/TICardReaderPlace`, `CueCard` cardCornerGeometry, `KeyboardLayout` cardCornerAt (`c59.tap-card`) | All | TI-59 only (the control state's media place); web: Import Media… and Saved Media… in File (desktop) and More |
| Card and `ti58c.memory.yaml` location | iCloud Documents (`CardStorage`) | `~/Calc-U-59` (`AppPaths`) | the save folder in the persistent file system (IndexedDB `/persist-calc-u-59`, `TIWebPart`) | — | Swift only / Qt only / Web only | Qt: the save folder in Settings; web: cards and `ti58c.memory.yaml` are kept between visits; a second tab keeps nothing |
| Start folders of Load Preset… and the ASM overlay's Select File | — (the system file importer, which takes no start folder) | last used, or fixed in Settings ▸ Folders ▸ Presets folder, ASM overlay folder (`remember-folder`) | — | the library's `SettingsPart`, `settingsSpec` (`TIShellProduct`) | Qt only | Calc-U-1600's labels and order; the Web has no folders |
| TI-58C constant memory (`ti58c.memory.yaml`: restored at a build without a preset, written on change, suspend, quit) | `TIFileAccess` through the bridge's `CoordinatedFiles` (`media::FileAccess`) | `TIFileAccess` (the library's save folder) | `TIFileAccess` (the persistent file system) | `ConstantMemoryKeeper`, `ConstantMemoryFile`, the kind `c59.constant-memory` | All | The same file in all three frontends |

## Printer

| Feature | Swift | Qt | Web | App layer | Status | Notes |
|---|---|---|---|---|---|---|
| PC-100C plugged in or out (resets) | Swift shell (the paper panel's "PC-100C" button, `TIShell.paperPlaces`; More ▸ Peripherals ▸) | shell (the paper panel's "PC-100C" button; Machine ▸ Peripherals ▸) | shell (the paper panel's "PC-100C" button) | the library's `attach`/`detach printer` and `control-state`; `TIShellProduct`'s `placesOnPaper` puts its button on the paper panel (the library's `Panel::places`) | All | a change power-cycles the machine; the button at the left of the paper panel's header, not on the bar |
| Dot view at PC-100C proportions; text view | `PrinterView` (the paper panel from the `paper PC-100C` marks; its owner's name, an empty paper under none) | library `PaperPanel` | library `CalcuPaperPanel` | `core/PC100CPaper` (marks), the library's `PaperPart` | All | |
| Printer interrupt: cut-short lines, the interrupted row's fired columns, faint dots | `PrinterView` (the marks' darkness) | library `PaperPanel` | library `CalcuPaperPanel` | `core/PC100CPaper` (`lineDots`: `drawnRows`, `inkedColumns`, `faintColumns`; greys of `inkAlpha`) | All | |
| PRINT / ADV (hold), T (TRACE switch) | `PrinterView` (`set-control`) | library `PaperPanel` (controls) | library `CalcuPaperPanel` (controls) | the library's `PeripheralsPart` answers `set-control`; the positions in the library's `control-state` (`controls`); TRACE's position kept by `TIAppPart` while detached | All | |
| Copy (text + 600 dpi image), cut | `PrinterView` (`paper.copy`, `paper.cut`; the bridge answers `clipboard-image`) | library `PaperPanel` (`ClipboardService`) | library `CalcuPaperPanel` (`hostServices.js`) | the library's `PaperPart` (picture, cut) | All | web: image where the browser's clipboard takes one, else text |

## Debugger

| Feature | Swift | Qt | Web | App layer | Status | Notes |
|---|---|---|---|---|---|---|
| CALCULATOR tab: registers, flags, HIR, T, return stack, partition, source | `LiveDebugView` | `LiveDebugWidget` | `Debugger` (`LiveDebug`) | `TIDebugPart` (`c59.calculator-debug`), `DebugModel`, `DebugSnapshots`, `DebugText`, `ModuleImage` | All | web: wide only; cannot be chosen while an ASM overlay runs (`c59.calculator-tab`) |
| Program steps ±5 running / full listing frozen | `LiveDebugView` | `LiveDebugWidget` | `Debugger` (`LiveDebug`) | `TIDebugPart` (`c59.program-listing`), `DebugModel` (listing, SolidStateReturn advance), `DebugSnapshots` | All | web: wide only |
| FREEZE, FREEZE ON START (next step), ARMED, RESUME, STEP (hold to repeat) | `DebugHeaderView` in `LiveDebugView` and `CPUInspectorView` (the bridge's `ButtonRule`, `freezeButtons`, `TIViewRules.debugHeaderButtons`) | `debug/DebugHeaderRow` in both tabs (library `RunControlButtons`; `freezeButtons`) | `Debugger`'s `FreezeHeader` (`CalcuRunControlButtons`; `c59.debug-headers`) | the library's run control (`pause`, `step`, `resume`, `pause-on-start`), `TILevelRules`, `freezeButtons`; `DebugText`'s `debugHeader` (title, buttons left to right, wide and narrow labels, colours) and `debugHeaderNarrow` | All | web: wide only; RESUME, FREEZE, FREEZE ON START, ARMED, STEP at the panel's base font size in every GUI |
| CPU tab: history, registers, control registers, display state | `CPUInspectorView` | `CpuInspectorWidget` | `Debugger` (`CpuInspector`) | `TIDebugPart` (`c59.cpu-debug`), `DebugModel` (cpuHistory), `DebugSnapshots`, `DebugText` | All | web: wide only |
| CPU FREEZE ON START (scan-loop exit), STEP | `CPUInspectorView` (the bridge's `ButtonRule`) | `CpuInspectorWidget` (library `RunControlButtons`, level `cpu`) | `Debugger` (`CalcuRunControlButtons`, level `cpu`) | the library's run control, `TILevelRules` | All | web: wide only |
| ROM heatmap with hover address, CLR | `CPUInspectorView` | `RomHeatmapWidget` | `Debugger` (`Heatmap`) | `TIDebugPart` (`c59.heatmap`, `c59.heatmap-reset`), the core's `c59.rom-heatmap`, `RomHeatmap` (steps, colours) | All | web: wide only |
| ASM overlay: select, run, clear; `.asm` opened by any way | `DebugView` | `AsmOverlayBox` | `Debugger` (`AsmOverlay`, upload) | `TIDebugPart` (`c59.asm-overlay`, `c59.asm-load`/`-run`/`-clear`, the entry `c59.asm-run`), `AsmOverlay` (format 2.0), the core's `c59.asm-overlay` | All | web: wide only |
| LOG tab: dumps, clear, log level | `DebugView` | `LogWidget` | `Debugger` (`LogTab`) | `TIDebugPart` (`c59.dump`), the library's `log` and `log-level`, `DebugLog` (`RingLog`, the frontends' view) | All | web: wide only; clear empties the view only |
| LOG kept across model switch | `DebugView` | `LogWidget` | `Debugger` (`LogTab`) | the frontends' view of the library's `log` | All | web: wide only |
| Binary trace (TRACE toggle), size limit | `DebugView` (`trace {button}`) | `LogWidget` (`trace {button}`) | `Debugger` (`LogTab`; the list of the traces handed over) | the library's trace session (one at a time), `TITraceRecorder`, `TraceWriter` | All | web: wide only; handed over (downloaded) when switched off; the LOG tab lists the traces only; where the trace folder cannot be written the start is refused |
| Trace location | Local / iCloud (Settings ▸ Tracing ▸ Save Location) | save folder, or trace folder in Settings | none: `keep-file` downloads | — | Swift only / Qt only / Web only | iCloud: Swift only; web: no location to choose |
| Notices, "Loading…" | Swift shell (an alert with the notice and OK, one after the other; "Loading…" below the display) | shell | shell | the library's `notices`, `NoticeQueue`, `LoadingRule` | All | several notices are shown one after the other; "Loading…" while a preset or another operation runs at full speed for more than half a second |
| Breakpoints | — | — | — | the library's run control (the harness's `break`) | — | no UI in any GUI; an app session pauses at one set through the harness |

## App

| Feature | Swift | Qt | Web | App layer | Status | Notes |
|---|---|---|---|---|---|---|
| About (icon, version, summary, links) | — (no About in the iPhone and iPad app) | shell (Help ▸ About Calc-U-59; the window icon, "Built with Qt") | shell (desktop: Help ▸ About; More ▸ About; no icon) | `TIShellProduct` | Qt and Web | |
| Settings | Swift shell (`settingsSpec(SwiftIOS)`: General, Startup presets, Display, Keyboard, Layout, Tracing; a sheet with Done) | shell (`cu59::settingsSpec(Qt6)`: General, Startup presets, Storage, Folders, Tracing, Display) | shell (`settingsSpec(WebWide/WebPortrait)`: General, Startup presets, Tracing (wide), Keyboard (portrait); no Settings with the prop `settings` off) | `Settings`, the library's `SettingsPart`, `TIShellProduct` | All | every change applied at once; More ▸ Settings… on every bar |
| Help (user guide link) | Swift shell (More ▸ Calc-U-59 Help; without the ROMs, the ROM message's Help) | shell (Help ▸ Calc-U-59 Help) | shell (desktop: Help menu; More ▸ Calc-U-59 Help) | `TIShellProduct` | All | opens `https://calc-u.org/59/` |
| The view by the window's size (side by side or pages) | `CalculatorView` (`TIViewRules.view`; the pages over an active fold) | `MainWindow::applyView` (the pages' chevron at the window's top corner) | `CalcU59` (the PWA's `auto` layout) | the library's `shell::view` (`debuggerFits`) on `cu59::viewInputs`, `TIChrome` (`panelsMinWidth`, `debuggerMinWidth`, `debuggerMinHeight`) | All | the view rule everywhere: side by side when wide, the debugger where the room and the height take it, else the pages |
| Haptics, click sound | `KeyboardView` | — | `CalcU59` (portrait layout, the PWA; keyboard feedback) | — | Swift only / Web only | click via WebAudio only (no haptics in a browser) |
| UI test launch argument (`--settings-file`, Debug only) | `AppSettings` | — | — | — | Swift only | no other test aids in the app (CTest `CalcU59NoTestAids`); the tests drive the session through harness attach |
| `--grab <png>` offscreen screenshot | — | `main.cpp` | — | — | Qt only | developer check |
| Installable PWA, offline | — | — | `app/` | — | Web only | layout follows the window as on iOS (rotates; pages, or side by side); all three models, every module and preset offline |
| Pause while hidden / in the background | `Calc-U-59App` (`scenePhase`), `EmulatorViewModel` | — | the library's `shell.js` (`visibilitychange`: `suspend` / `continue`) | the session's `suspend` / `continue` commands; `TIAppPart::suspending` persists | Swift only / Web only | Qt keeps running, as a desktop app does |
