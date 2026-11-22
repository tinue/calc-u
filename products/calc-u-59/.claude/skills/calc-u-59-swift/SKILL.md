---
name: calc-u-59-swift
description: Use this skill when working on the Swift/SwiftUI code of Calc-U-59 (frontends/swift/App/, frontends/swift/App/Views/, frontends/swift/Bridge/). Covers EmulatorViewModel, the ObjC++ bridge, TIShell (the library's shell models: menus, control bar, More, settings, notices, "Loading…", the pointer rule), display, keyboard, debug panels, the paper panel, card reader, settings on the session, and opening files. Trigger on any task that touches frontends/swift/App/*.swift, frontends/swift/App/Views/*.swift, frontends/swift/Bridge/, or the Xcode project.
user-invocable: true
---

# Calc-U-59 Swift GUI

This skill covers all work inside `frontends/swift/App/`, `frontends/swift/App/Views/`, and `frontends/swift/Bridge/`.
The Swift app is the iPhone and iPad app (iOS and iPadOS only, played by
touch; its debugger for looking). The Mac app is the Qt6 app (calc-u-59-qt
skill).

- App structure, files, threading, bridge: `docs/developer/App-Layer.md`
  § "The Swift frontend".
- Everything that isn't drawing (run loop, keys, files, debugger, drawing
  rules): the rest of `docs/developer/App-Layer.md` and the calc-u-59-app skill.
- Freeze / step and the debug snapshots: `docs/developer/Debug-API.md`.
  FREEZE, STEP, RESUME and FREEZE ON START are the library's run
  control (`pause`, `step`, `resume`, `pause-on-start`; CALCULATOR is
  `program`, CPU `cpu`), and the buttons follow its rule through the bridge
  (`buttonUseFor:` with a `TIRunLevel`, the paused one `pausedLevel`;
  `calcu::debugger::buttonUse`; test
  `Calc-U-59Tests/RunControlTests.swift`); TRACE is the trace session's.
  The panel draws the session's product topics
  `c59.calculator-debug`, `c59.program-listing`, `c59.cpu-debug`, `c59.heatmap`,
  `c59.asm-overlay`, `c59.debug-view` (app/TIDebugPart.hpp), which the bridge
  mirrors; the LOG tab is the bridge's `cu59::RingLog` of the session's `log`
  (Clear empties the view only); the buttons send `c59.show-calculator-debug`,
  `c59.show-cpu-debug`, `c59.heatmap-reset`, `c59.dump`,
  `c59.asm-load`, `c59.asm-run`, `c59.asm-clear`, and the library's
  `log-level`. Run and the
  CALCULATOR tab follow the `control-state` entries `c59.asm-run`,
  `c59.calculator-tab`; TRACE can always be pressed (a trace folder that
  cannot be written refuses the start); FREEZE and FREEZE ON START
  `cu59::freezeButtons` (`freezeButtonsFor:`). The tab a `debug-panel:` step
  asks for arrives as `showDebugPanel:` when c59.debug-view's `requests` grows.
- Opening a file: the Preset button's importer (the preset types) sends
  the library's `open`
  (`openFile:path:fileName:way:completion:`, way `dialog`); the
  session finds out what the file is (a preset, a card, an ASM overlay, a
  keystroke program) and a refusal is its notice. `wouldOpenFile:` asks
  `open-would-take`. No Swift code tells a file's kind; test
  `Calc-U-59Tests/OpeningTests.swift`.
- The family structure: the window is drawn from the
  library's shell models, which `Bridge/TIShell` computes from the
  last Value of each input topic (`control-state`, `busy`, `run-state`,
  `roms-missing`, `settings`, `media`, the `preset-list` reply) with
  `app/TIShellProduct`: `menus` (SwiftUI `Commands` in
  `Views/ShellCommands.swift`, the iPad's menu bar; the items with the roles
  Settings…, About and Quit are left out), `bar` (`Views/ControlBarView.swift`: the model picker, the picker
  of `slot-1`, the card controls; Preset, Settings and More,
  `moreItems` with Reset, Cold boot asks first), `paperPlaces` (the
  PC-100C's button on the paper panel, `Views/PrinterView.swift`), `settingsDialog`
  (`Views/SettingsView.swift` over `cu59::settingsSpec(SwiftIOS)`, a sheet),
  "Loading…" (`LoadingRule`), the notices (`NoticeQueue`: one alert after
  the other, at the window's root and over the settings sheet) and the
  pointer on the faceplate (`PointerTracker` on `cu59::faceplateHitMap`,
  `pointerDown/Move/Up` in points with the points per base-image pixel).
  `choose:` sends an item's command (a refusal becomes a notice) or hands
  back a shell action (`TIShellAction…`, `arm-card`, `arm-fresh-card`).
  The model is the control state's `model` (`modelID`); there is no copy of
  the model facts in Swift (`CalcU59FrontendsDrawControlState`,
  `CalcU59NoTestAids`). A new menu item or settings row goes into
  `app/TIShellProduct`, never into a Swift view.
- Settings: the settings file is the session's. The bridge
  opens `Calc-U-59.ini` (or the Debug build's `--settings-file`; the
  library's `calcu::shell::HostSettings`) and
  hands the file to the session thread (the trace location, Local or
  iCloud, is read at each trace start; `HostDirs::iCloudAvailable` refuses
  the iCloud location while signed out), where the library's `SettingsPart` (made
  by `makeAppSession`'s `calcu::shell::HostSessionSetup`) (with `alsoShown` `window.portrait-debug-page`) and
  the library's start read and write it; views read the values through
  `TIShell.setting(_:_:)` and change one with `setSetting` (`set-setting`).
  Nothing is kept in UserDefaults.
- The paper panel: `PrinterView` draws the library's topic
  `paper PC-100C` (dots and text the bridge keeps) under its owner's name
  (`TIShell.paperOwner`), an empty paper under none; Copy and Cut are
  `paper.copy` / `paper.cut`, the picture the library's, the pasteboard
  entry the bridge's (`clipboard-image`: one entry, a PNG); PRINT, ADV, TRACE are `set-control`.
  `docs/developer/App-Layer.md` § "Printer".
- TI-59 background: the calc-u-59-core skill's Domain Primer. SwiftUI
  practice: the swiftui-pro skill.

## Rules

1. **No non-GUI logic in Swift.** Parsing, file formats, timing, key
   sequencing, snapshot building and the numbers the views draw with belong
   in `app/`, reached through `EmulatorViewModel` → `TIAppSession` (the app
   session) or a `frontends/swift/Bridge/` wrapper. Views are thin bindings
   over `EmulatorViewModel`.
   **The session runs on its own thread** (the library's `calcu::shell::SessionThread`):
   the bridge posts commands (the library's and `c59.*`) and hands each
   batch of messages to the main thread; `EmulatorViewModel.apply` copies
   what a batch changed (`TIChangeShell`: the shell's models). Nothing in
   Swift reads the session or runs the machine; a result comes back as a
   reply (a completion block) or a topic. A new value the views need is a
   topic (a library one, or a product topic of `TIAppPart`/`TIDebugPart`)
   plus a getter in `TIAppSession` or `TIShell` that reads the last message.
   The cards are the library's `media` topic (place `card-reader`) and
   `media.*` commands (`savedCards`, `armCard:`,
   `armFreshCard:confirmed:` with the `asks:` replace question, `ejectCard`);
   `c59.tap-card` feeds a corner.
   **Harness attach (Debug, simulator only):** the app opens its
   session on 127.0.0.1, port `--attach-port N`, else 59059; `calcu attach
   [--port N]` drives it (`library/harness/attach/`; one port per simulator).
   The attached registry has every command of the serve form (`preset`
   and `open` among them) and the product's `c59 …`.
   A Debug build also takes `--settings-file <path>` (a relative path in the
   app's temporary folder); the app has no other aid for its tests.
   Release and device builds have no attach (`CALCU_ATTACH` off; one
   entitlements file, `Calc-U-59.entitlements`, for every configuration).
   `check_no_attach.sh` checks it: `--sources` in Frequent, `--binary` as a
   build phase of every Release and device build, the full build check in
   Longer. The XCFramework phase must keep its declared output, or a Release
   build after a Debug one links the Debug XCFramework.
2. **Parity.** A user-visible change here needs the matching Qt change, and
   the web change if the feature is in the web's scope, on the same branch —
   or a `Pending (Qt)` / `Pending (Web)` row in `docs/developer/Frontend-Parity.md`.
   Update the table either way (CLAUDE.md § "Frontend Parity").
3. **Bridge wrappers translate types only.** Per-frame work that walks
   registers or program memory runs in C++ and crosses the bridge once as a
   finished result — never 100 bridge calls per 60 Hz frame.
4. **Guard `@Observable` writes** (`docs/developer/App-Layer.md` § "Messages, not a refresh timer").
5. **Keep the Dynamic Type cap.** `CalculatorView.body` limits Dynamic Type
   to `.small ... .large`. Wider sizes break the layout (the TI-59 card bar
   first).
6. **Compile, don't run.** Never launch the simulator;
   ask the user to, saying exactly what to look for.
7. **Commit often;** push as the root `CLAUDE.md` says.
8. **Cite sources.** A hardware spec or external document gets its https URL
   (or local reference file) in the comment or doc.

## Build Verification

Every change needs the iOS simulator build:

```bash
xcodebuild -project frontends/swift/Calc-U-59.xcodeproj -scheme Calc-U-59 \
  -destination 'generic/platform=iOS Simulator' build
```

A build for one simulator, when a task or test needs it, takes a simulator
from `bin/simulators.yaml`. `bin/simulators status` lists them with
their UDIDs; never hard-code a model name from memory.

```bash
bin/simulators status    # pick a device, e.g. the iPhone 18 Pro Max
xcodebuild -project frontends/swift/Calc-U-59.xcodeproj -scheme "Calc-U-59" \
  -destination "platform=iOS Simulator,id=<UDID>" build
```

The Xcode build first builds `CalcU59.xcframework` (the library, `core/`,
`app/`) through CMake: it needs CMake and Ninja, and Xcode compiles only
Swift and the bridge (`docs/developer/App-Layer.md` § "Build").

A change to `core/` or `app/` also needs the CMake build and tests
(calc-u-59-app skill).

Tests of the bridge and the session thread from Swift: the unit-test target
`Calc-U-59Tests` (plan `UnitTests`, calc-u-59-uitesting skill). After a build
that failed because the XCFramework was missing, Xcode may keep saying so
for one test plan: delete `Build/Intermediates.noindex/XCBuildData` in the
derived data.
