---
name: calc-u-59-uitesting
description: Use this skill when writing or debugging XCUITest UI tests for Calc-U-59 (frontends/swift/Calc-U-59UITests/). Covers test helpers, file picker navigation, accessibility identifiers, orientation setup, and how to verify state after a load. Trigger on any task that touches frontends/swift/Calc-U-59UITests/*.swift or *.xctestplan.
user-invocable: true
---

# Calc-U-59 UI Testing

This skill covers all work inside `frontends/swift/Calc-U-59UITests/`.

---

## Test Files

| File | Purpose |
|------|---------|
| `AppHelpers.swift` | Shared helpers: `launchApp(settings:arguments:)`, `launchApp(orientation:settings:arguments:)` (a settings file of the launch's own, `--settings-file`, and attach on port 59162), `writeSettingsFile(_:)`, `attachHarness()`, `waitIdle(_:)`, `waitForOperation(_:timeout:endsPaused:)` (the busy topic says one runs, then the reply to the bare `wait`), `navigateToOnMyIPhone(_:targetFile:)`, `loadPreset(_:file:idleTimeout:harness:endsPaused:)` (Preset button → picker → wait through attach; `endsPaused` for a preset that ends paused, which is never idle: the `run-state` topic is read instead of the bare `wait`, which would wait until resumed), `attachScreenshot()` |
| `FamilyStructureTests.swift` | The family's scenarios, shared with the Qt and Web tests (`test_the_menus`, `test_command_r_resets`, `test_application_menu`, `test_the_bar`, `test_the_settings`, `test_loading_sign_during_a_long_load`, `test_notices_one_after_the_other`, `test_help`, `test_a_key_under_the_pointer`, `test_an_example`, `test_the_printer`, `test_one_picture_per_layout`) and the Swift frontend's own (`test_more_cold_boot_asks_first`, `test_settings_file_by_path`, `test_notice_over_the_settings_sheet`, `test_no_card_controls_on_the_ti58`, `test_iphone_portrait_reaches_every_control`, `test_paper_cut_in_dot_mode`); in `UIRegressionTests` |
| `LayoutTests.swift` | Button visibility, label-wrapping, portrait/landscape screenshots |
| `FilePickerTests.swift` | File picker opens; reopens after cancel (regression for iPad popover dismiss bug) |
| `PresetLoadTests.swift` | End-to-end load of `test_diag.ti59` via the file picker, its end through attach |
| `CardScenarioTests.swift` | The magnetic card scenarios: `test_fresh_card_written_and_listed`, `test_card_read_by_its_corner` (the bar's card controls, the dialog's `card-name`, `card-arm`, `card-cancel`, the corner pressed on the faceplate at 0.9, 0.121) |
| `ScreenshotTests.swift` | App Store screenshot capture (driven by `bin/run-screenshots`); writes PNGs to the dir named in `/tmp/calc-u-59-screenshot-dir`. Its settings in a settings file, the module (LE) and R/S through attach. The runner names each test in full and fails a shot that writes no PNG. Its overlay shot is the debug panel's scenario `testAsmOverlay` (`test_stopwatch.asm`, ASM format 2.0, from `1-Testfiles`) |
| `DebugPanelTests.swift` | The debug panel's scenarios: `testCalculatorFreezeStepResume` (FREEZE, `btn-step`, RESUME on the CALCULATOR tab; R/S through attach), `testTabsAndLabels`; in `UIRegressionTests`, iOS |
| `HarnessClient.swift` | A line client for harness attach: `connect(port:)`, `command(_:)` (message lines skipped), `closedByPeer()` |
| `AttachTests.swift` | Harness attach in the simulator: launch with `--attach-port 59160`, drive keys, read `display-text`, `wait` on the app's clock, `quit`; Home and back, a command refused while suspended; a press on `led-display` for full speed while a program runs, then a touch on R/S of the faceplate. In `UIRegressionTests` |
| `PacingTests.swift` | A TI-59 keeps one minute, `ticks` over attach on port 59161. Checkpoint: only in the test plan `PacingCheckpoint`, skipped by `UIRegressionTests` |
| `UIRegressionTests.xctestplan` | Local regression test plan (no CI job runs XCUITests) |
| `Screenshots.xctestplan` | Test plan for the screenshot run (`bin/run-screenshots`) |
| `PacingCheckpoint.xctestplan` | `PacingTests` only (CTest `CalcU59PacingIOS`, Checkpoint) |

The Swift unit tests are a target of their own, `Calc-U-59Tests/` (plan
`UnitTests`, hosted by the app with `--settings-file unit-tests.ini`): a second session through
the real bridge (`TestSession.swift`), the bridge's Debug-only test hooks
(`rawKeyDown:completion:` and `rawKeyUp:completion:` (the session's raw key-down/key-up, no typing), `offeredHostServices:`, `emulatedTime:`,
`hostCallCount`); `ShellBridgeTests` checks `TIShell` (the menus, the
mobile bar with More, a disabled item's reason, the hit rule, the settings
rows and a change, Copy's pasteboard forms, "Loading…").
`xcodebuild test … -testPlan UnitTests` on a simulator.

Every UI test runs on an iOS simulator; the app has no macOS build.

**No test aids; attach and a settings file.** The app has no
`--UITesting`, no hidden R/S button and no `keystroke-playback-status`
(CTest `CalcU59NoTestAids`). A test launches through the helpers, which
write a settings file of the launch's own (`writeSettingsFile`, the
runner's temporary folder, given by its full path; the simulator's app
reads it there — checked by `test_settings_file_by_path`) and pass
`--settings-file` and `--attach-port`. A test that needs the session (a load's end, the
display as text, emulated time, the module) attaches: the runner, in the
same simulator, connects with `HarnessClient` to 127.0.0.1 and sends
`calcu serve` commands (`wait 1s`, `key-down R/S`, `display-text`, `ticks`,
`attach slot-1 LE`, `state settings`). A load started with a tap is awaited
with `waitForOperation` (the busy topic, then the bare `wait`); a key into
a running program (an overlay among them) is `key-down`/`key-up`. The
attached registry has every `calcu serve` command, `preset <path>` and
`open <path>` among them. Each
simulator needs a port of its own: the helpers use 59162, `AttachTests`
59160, `PacingTests` 59161. A notice is an alert: `notice(app)`,
`noticeText(app, contains:)` and `closeNotice(app)` in
`FamilyStructureTests` (tap OK by its label, the alert's button drops the
identifier `notice-ok`).

---

## Running a Single Test Standalone

Click the diamond icon next to the test function in Xcode, or from the
terminal. Use a simulator from `bin/simulators status` (the configured set in
`bin/simulators.yaml`); never hard-code a model name from memory.
Reset the simulator first, as the CTest entries and `bin/run-screenshots` do:
`bin/simulators reset` puts back the example presets and deletes what the app
saved (its cards and `ti58c.memory.yaml` in the app's Documents folder), so a
run never sees the cards of earlier runs (Arm Card's list would grow past
the screen and a card test would not find its card):

```bash
bin/simulators reset --role iphone
xcodebuild test \
  -project frontends/swift/Calc-U-59.xcodeproj \
  -scheme "Calc-U-59" \
  -destination "platform=iOS Simulator,id=<UDID from bin/simulators status>" \
  -only-testing "frontends/swift/Calc-U-59UITests/PresetLoadTests/testLoadDiagPreset"
```

---

## Screenshots and Rotation

### Use `XCUIScreen.main.screenshot()`, not `app.screenshot()`

`app.screenshot()` always captures in the device's **natural portrait coordinate space**, regardless of how the simulator is oriented. On a rotated iPad, this produces a portrait-framed PNG with landscape content rotated 90° — not what you want.

`XCUIScreen.main.screenshot()` captures the **physical screen pixels** and produces a correctly-oriented PNG — no post-processing or rotation needed. This works whether the orientation was set programmatically via `XCUIDevice.shared.orientation` or by manually rotating the simulator window:

```swift
let png = XCUIScreen.main.screenshot().pngRepresentation
let dest = URL(fileURLWithPath: "/Users/me/Desktop/my-screenshot.png")
try png.write(to: dest)
```

**Exception:** since Xcode 27 `XCUIScreen` hands back landscape shots as portrait pixels with a rotation tag, a pixel short on the open iPhone Duo. `ScreenshotTests` therefore asks `bin/run-screenshots` to take landscape shots with `simctl io … screenshot` (request file `/tmp/calc-u-59-screenshot-request`).

**Note:** Device bezels only appear in screenshots taken via the simulator's camera button. `XCUIScreen.main.screenshot()` captures app pixels only — bezels are never included.

### Writing screenshots to the Mac filesystem

In the XCUITest process, `HOME` and `NSHomeDirectory()` point to the **simulator sandbox**, not the Mac. Use `SIMULATOR_HOST_HOME` to get the real Mac home directory:

```swift
let home = ProcessInfo.processInfo.environment["SIMULATOR_HOST_HOME"] ?? "/tmp"
let dest = URL(fileURLWithPath: "\(home)/Desktop/shot.png")
```

To pass a target directory from a driving script into the test process, use the handoff-file pattern from `bin/run-screenshots`: the script writes the directory path to `/tmp/calc-u-59-screenshot-dir` before invoking `xcodebuild`, and `ScreenshotTests.swift` reads that file at runtime (env vars don't cross the `xcodebuild` → test-runner boundary without scheme changes).

---

## Device Detection

`UIDevice.current.userInterfaceIdiom` is available in the test process and reliably distinguishes iPhone from iPad at runtime:

```swift
let orientation: UIDeviceOrientation = UIDevice.current.userInterfaceIdiom == .pad
    ? .landscapeLeft : .portrait
let app = launchApp(orientation: orientation)
```

Use this whenever the app layout differs between form factors — for example, the iPad uses a landscape side-by-side layout while iPhone uses portrait.

---

## Orientation

Always set orientation **before** `app.launch()` to avoid simulator state leaking between runs. The shared helper does this:

```swift
let app = launchApp(orientation: .portrait)  // sets XCUIDevice.shared.orientation first
```

Reset in `tearDown`:

```swift
override class func tearDown() {
    XCUIDevice.shared.orientation = .portrait
    super.tearDown()
}
```

---

## Preset Prerequisites

Screenshot tests that load `.ti59` files require those files to be present on the simulator's local storage. Run `bin/simulators setup` once before running tests — it ensures the target simulators from `bin/simulators.yaml` exist and installs presets into each one: an explicit list of files from `examples/` goes to the storage root, and everything from `examples/screentest/` goes into a `1-Testfiles/` folder (sorted first by name) in the simulator's "On My iPhone/iPad" storage. It also boots each new simulator once and launches the Files app, which is what initialises the AppGroup storage in the first place. `bin/simulators status` reports whether a machine is fully provisioned; `bin/simulators` alone prints the full command reference. `setup` leaves the files of a simulator already provisioned as they are: after a change to `examples/screentest/` run `bin/simulators files` (or `bin/simulators reset`, which also deletes the app's saved cards; `status` shows how many there are), or a test loads the old file and gets the session's refusal instead of a load.

---

## Testing the debug panel (CALCULATOR/CPU/LOG tabs) — prefer iPad

FREEZE, STEP and RESUME are the library's run control:
a FREEZE lands at the next program step boundary (CALCULATOR) or within
50000 weighted instructions while the calculator waits for a key, inside
the same host call, so the button states follow the `run-state` topic
(`calcu::debugger::buttonUse`); TRACE starts the library's trace session
(one at a time; the file `CALCU59_TRACE.bin` and its siblings by model,
with no key or card records). The panel draws the
session's product topics; Run, TRACE and the CALCULATOR tab follow the
control state's entries (the tab is disabled while an overlay runs).
The CALCULATOR tab's STEP is `btn-step`.

On iPhone portrait the pages are calculator → printer → debug (`btn-page-right`), and `btn-page-left` on the calculator page goes straight to debug. The debug page (`DebugView`, holding `btn-tab-calculator`/`btn-tab-cpu`/`btn-tab-log`) exists only when Settings → "Debug Page in Portrait" is on (`[window] portrait-debug-page`, default `false`). A test puts it into the launch's settings file (`launchApp(orientation:settings: ["window.portrait-debug-page=true"])`). `ScreenshotTests` passes it and the other settings its shots show the same way; the module, which is no setting, goes in through attach (`attach slot-1 LE`), so the shots need no setup on a new simulator.

The iPad layout in landscape (and the iPhone Duo opened flat) shows the debug panel side-by-side with the calculator — no paging, no setting to flip. Which layout a window gets is the library's view rule (`shell::view` on `cu59::viewInputs`, by size, not idiom); `LayoutTests.testWideWindowShowsDebugger` checks it. When a test only needs to reach the debug tabs (e.g. checking the LOG tab's content after a preset runs), prefer running it on an iPad simulator destination to skip this whole prerequisite.

---

## Accessibility Identifiers in the App

| Identifier | Type | Value / Notes |
|------------|------|---------------|
| `control-bar` | Other | The control bar (`ControlBarView`) |
| `bar-model` | Button | The model picker; label the running model |
| `bar-slot-slot-1` | Button | The module picker (the `media` topic's entries in their order; closed: the module's ID); its entries `item-module:<id>` (`item-slot-1-none` for the empty slot) |
| `bar-place-printer` | Button | "PC-100C" at the left of the paper panel's header (iPhone portrait: on the paper page): value `On` / `Off` |
| `bar-media-card-reader` | Button | Arm Card (or Eject Card while a side passes), TI-59 only |
| `bar-more` | Button | More, at the right edge of the bar: items `more-item-<id>`, every menu item (`load-preset`, `examples`, `settings`, `model`, `peripherals`, `reset`, `cold-boot`, `resume`, `help`; headings `more-item-heading-<menu>`; helper `chooseFromMore(app, id)`); Cold boot's confirmation `confirm-cold-boot` |
| `settings-dialog` | Other | The settings form; rows `setting-<section>-<key>`; `settings-close` (Done) |
| `loading` | Other | "Loading…" below the display |
| `faceplate` | Other | The calculator's art: tap a key at a normalised offset (key centres from `app/KeyboardLayout.hpp`, e.g. 1 at 0.2812, 0.8839; R/S at 0.0783, 0.9704) |
| `cue-card` | Other | The cue card shown: label its title, value its template |
| `paper-owner`, `paper-view-mode`, `paper-text`, `paper-copy`, `paper-cut` | | The paper panel: the owner's name, dots/text, the text view, Copy, Cut |
| `btn-freeze-on-start` | Button | "FREEZE ON START" in CALCULATOR debug panel (`LiveDebugView`); label shows "F.START" in narrow layout |
| `btn-freeze-armed` | Button | "ARMED" in CALCULATOR debug tab; always present, enabled only while FREEZE ON START is pending; tap to disarm |
| `btn-page-left` | Button | Chevron-left page arrow (iPhone portrait only); destination depends on current page |
| `btn-page-right` | Button | Chevron-right page arrow (iPhone portrait only); destination depends on current page |
| `btn-tab-calculator` | Button | CALCULATOR tab in debug panel (`DebugView`) |
| `btn-tab-cpu` | Button | CPU tab in debug panel (`DebugView`) |
| `btn-tab-log` | Button | LOG tab in debug panel (`DebugView`) |
| `btn-cpu-freeze-on-start` | Button | "FREEZE ON START" in CPU tab (`CPUInspectorView`) |
| `btn-cpu-step` | Button | "STEP" in CPU tab (`CPUInspectorView`); tap to single-step, hold to auto-step. The CPU tab's ARMED has no identifier |
| `btn-step` | Button | "STEP" in CALCULATOR tab (`LiveDebugView`); tap to step one program step, hold to auto-step |
| `btn-trace` | Button | "TRACE" in LOG tab; always enabled (a folder that cannot be written refuses the start) |
| `asm-file`, `asm-words`, `asm-status` | StaticText | The ASM OVERLAY box's file name, count and status ("ASM entered at 0x1800 (n step(s)).") |
| `btn-asm-select` | Button | "Select File" in ASM overlay (`ASMDebugContent` in `DebugView`); opens `.asm` file picker |
| `btn-asm-run` | Button | "Run" in ASM overlay; disabled until a file is loaded (`vm.canRunASM`: the control state's `c59.asm-run`) |
| `btn-freeze` | Button | "FREEZE" in CALCULATOR debug tab (`LiveDebugView`) |
| `btn-resume` | Button | "RESUME" in CALCULATOR debug tab (`LiveDebugView`); left of FREEZE |
| `led-display` | Any | The LED display (`KeyboardView`); a press there is full speed while it lasts |

---

## Navigating the System Document Picker (fileImporter)

The iOS system document picker (`UIDocumentPickerViewController`) is driven by XCUITest as a sheet within the app's window. Several non-obvious rules apply:

### 1. Tap the *cell*, not the filename label

The picker shows files as collection view cells. Each cell contains inner labels (filename, date, size). Sending a tap to an inner `staticText` does **not** trigger file selection — only tapping the cell itself works.

```swift
// WRONG — taps the label, picker stays open
app.staticTexts["test_diag.ti59"].tap()

// CORRECT — taps the parent cell, picker loads the file
let cell = app.cells.containing(.staticText, identifier: "test_diag.ti59").firstMatch
XCTAssertTrue(cell.waitForExistence(timeout: 5))
cell.tap()
```

### 2. Use `tabBars.buttons` / `cells` to avoid ambiguous matches

Several picker labels appear in two places simultaneously:

- **Browse tab** ("Durchsuchen" / "Browse"): in the tab bar and as a back-button in the navigation bar. Use `app.tabBars.buttons[label]` to target the tab bar only.
- **"On My iPhone/iPad"**: in the sidebar cell list and as the navigation bar title once selected. Use `app.cells.containing(.staticText, identifier: label).firstMatch` — the nav bar title is not inside a cell.

Never use bare `app.buttons[label]` or `app.staticTexts[label]` for these — both will find multiple matches and crash.

### 3. Fast-path is safe because test filenames are unique

All files in `1-Testfiles/` (sourced from `examples/screentest/`) are prefixed with `test_` (e.g. `test_diag.ti59`). This prefix is never used for files in the root, so a fast-path check is unambiguous: if the file is already visible, we are already inside `1-Testfiles`.

`navigateToOnMyIPhone(_:targetFile:)` in `AppHelpers.swift` does this: return at once if the file is visible, else Browse tab → "On My iPhone/iPad" → `1-Testfiles`.

### 4. Locale-dependent labels

Navigation labels are locale-dependent. The helpers above try German first (device is German), then English. Any third locale will fail silently at the navigation step — the file cell check will then time out with a useful error.

### Debugging picker element tree

When the picker changes between iOS versions, dump the hierarchy after opening it:

```swift
presetButton.tap()
_ = app.buttons["Cancel"].waitForExistence(timeout: 3)  // wait for picker
print(app.debugDescription)
```

---

## Verifying a Successful Preset Load

A preset is the library's `load-preset` (one operation of the session). Wait for
its end through attach (`waitForOperation`: the busy topic, then the reply
to the bare `wait`), then check what it leaves: `display-text` through attach, or
the `cue-card` element's label (`FamilyStructureTests.test_an_example`).

---

## Brittleness Notes

The system document picker is the most fragile part of the test suite. Things that can change between iOS/Xcode versions:

- Tab bar label strings ("Durchsuchen", "Browse")
- Location list strings ("Auf meinem iPhone", "On My iPhone")
- Cell element type (currently `cell`; could become `otherElement`)
- Grid vs. list layout (affects which queries match)

Things that are stable:
- Filenames (e.g. `"test_diag.ti59"`) — always your primary locator
- `bar-*`, `more-item-*` — our own identifiers
