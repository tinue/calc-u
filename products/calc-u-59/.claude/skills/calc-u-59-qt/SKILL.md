---
name: calc-u-59-qt
description: Use this skill when working on the Qt6 desktop app of Calc-U-59 (frontends/qt6/, frontends/qt6/app/, frontends/qt6/resources/, `.github/workflows/calc-u-59.yml` (repository root), the repository's tools/run_qt.sh, tools/gen_qt_icons.sh). Covers the class map against the Swift views, threading (the library's app session on the GUI thread), building, packaging for Windows and Linux, and the parity rule with the Swift app. Trigger on any task that touches frontends/qt6/ or the Qt CI workflow.
user-invocable: true
---

# Calc-U-59 Qt6 App

**Ship scope first:** the Qt app ships on **Windows and Linux** only, built
by the repository's `.github/workflows/calc-u-59.yml` and attached to releases by the root
`release.yaml`. The **macOS Qt build is local and for development only** —
never shipped, never built in CI, never signed. On the Mac the Swift app is
the product. The Qt app has no iCloud: cards, `ti58c.memory.yaml` and traces live in
the save folder `~/Calc-U-59` (`AppPaths::saveDir()`, LIB-9), each folder (and where Load Preset… starts) set in Settings, as
in Calc-U-1600.

**The window is put together from LIB-14's shell** (M-57, design §10.4):
`MainWindow` makes a `calcu::qt6::Shell` over the `calcu::qt6::SessionHost`
that `EmulatorController` makes, with Calc-U-59's data from
`app/TIShellProduct` (`cu59::shellProduct`, `cu59::settingsSpec(ShellFrontend::Qt6, …)`,
`cu59::faceplateHitMap`). The shell draws the menus (File, Machine, Help; no
Edit), the control bar (the model picker, LIB-7's `MediaPicker` for `slot-1`,
the "PC-100C" button, C59-7's `CardControls` by `registerMediaControls("magnetic-card", …)`),
the settings dialog, About, Help, notices, "Loading…" and "ROMs missing",
all from the control state and the library's commands. **A new menu item
goes into the menu model (`cu59::shellProduct().items` or the library's
`shell/MenuModel`), never into a frontend.** The pointer on the faceplate is
the shell's (`Shell::setFaceplate` with `FaceplateGeometry`: the canvas's
scale and corner, and `KeyboardWidget::cardTakes` for the card tap).

**Magnetic cards** (C59-7, LIB-7 59, M-55) are the library's media part: the
`media` topic's place `card-reader` (what is fitted, `passing`, the picker)
and the commands `media.choose` (Arm Card), `media.start-new` with the
template `implicit:Fresh card` (Arm Fresh Card; a refusal `asks: …` is the
replace question, sent again with `confirmed`: `calcu::qt6::sendAsking`, `library/qt6/AskedSend`), `media.eject`, through
`EmulatorController` (`cardReader`, `savedCards`, `armCard`, `armFreshCard`,
`ejectCard`). `CardControls` (on the bar while the control state lists the
card reader, the TI-59) shows the two "Crd" buttons (names "Arm Card",
"Arm Fresh Card") or "Eject Card"; File ▸ Arm Card… and Arm Fresh Card… are
the product items `arm-card`, `arm-fresh-card` (`mediaPlace: card-reader`);
`CardPickerDialog` draws the saved cards it is handed and reads no folder.
A corner tap is `c59.tap-card` (the shell lets a press on a card on show
through to `KeyboardWidget`); the lit corner comes from `c59.cue-card`.

Structure follows the sibling project Calc-U-1600
(`~/Development/sharp/Calc-U-1600/Qt6`): Widgets, AUTOMOC/AUTORCC, no `.ui`
files, QPainter rendering.

Everything that isn't drawing: `reference/AppLayer.md`.

## Rules

1. **Parity with the Swift app and the web client** (CLAUDE.md § "Frontend
   Parity"): a user-visible change here needs the matching Swift change, and
   the web change if the feature is in the web's scope, on the same branch —
   or a `Pending (Swift)` / `Pending (Web)` row in `reference/FrontendParity.md`.
   Update the table either way.
2. **No logic in `frontends/qt6/app/`.** Anything that isn't Qt widgets, painting or
   file dialogs belongs in `app/` (calc-u-59-app skill) so the Swift app
   gets it too. `EmulatorController` is the product's glue (as Calc-U-1600's
   `MachineController`): it makes the app session (`cu59::makeAppSession`
   on the library's `calcu::shell::makeHostSession`, which puts LIB-14's
   `SettingsPart` in front; without the full ROM set a session with no
   machine and the topic `roms-missing`) and the `SessionHost` over it
   (the library's `calcu::qt6::ProductHost`: the 60 Hz timer, the listener, the clipboard and open-url services,
   harness attach), reads the host's topic mirror and turns topic changes
   into Qt signals; it keeps the LOG tab's view of `log`. Notices, the log
   to the platform, "Loading…" are the shell's. The paper is the library's
   paper panel in the shell's panel area (owner from the control state;
   `MainWindow::followPrinter` sets the PC-100C's PRINT, ADV and TRACE with
   their positions from the control state's `controls`,
   `EmulatorController::printerControls`); FREEZE, STEP and RESUME are the
   library's run-control buttons of the levels `program` (CALCULATOR) and
   `cpu` (CPU), FREEZE ON START sends `pause-on-start`, TRACE sends
   `trace {button: true}` (LIB-10). The model, the module, the printer and
   the card controls are drawn from the control state only (LIB-4 AC-15;
   CTest `CalcU59FrontendsDrawControlState`).
3. **One thread.** The session lives on the GUI thread; there is no
   emulation thread and no queue to hop. Qt6 sends neither suspend nor
   continue (LIB-3 AC-31; CTest `CalcU59NoOwnSession`).
4. **Assets have one source:** ROMs from `roms/` and artwork from
   `frontends/swift/App/Assets.xcassets` are compiled in as Qt resources by `frontends/qt6/CMakeLists.txt`;
   icons come from `frontends/swift/App/AppIcon.icon` via `tools/gen_qt_icons.sh`. Never copy them.
5. **Settings** are declared in `app/Settings.hpp` and live in
   `Calc-U-59.ini` (LIB-9); the settings dialog is the shell's
   (`ShellSettingsDialog` over the `settings` topic, rows from
   `cu59::settingsSpec`), the library's `calcu::qt6::HostFolders`
   (`AppPaths::folders()`, `file()`, `places()`) opens the file and gives
   the places (`calcu::settings::Paths`); `AppPaths.cpp` keeps the bundled
   examples' folder;
   `AppSettings.hpp` wraps the rest in QString calls. No QSettings. The
   window is `[window]` numbers (`cu59::settings::WindowState`).
6. **Compile, don't run.** Build with the commands below. To check rendering
   without showing a window: `QT_QPA_PLATFORM=offscreen ../../build/app-59/products/calc-u-59/frontends/qt6/Calc-U-59.app/Contents/MacOS/Calc-U-59 [preset] --grab out.png --after 3000`
   (an offscreen screen may clamp the window size). Ask the user to try the app itself.
   A development or test build (`CALCU_ATTACH`, POSIX) offers harness
   attach (`--attach-port N`, else 59059; `calcu attach`) and
   `--pacing-probe <s>` (LIB-17 AC-19, CTest `CalcU59QtPacingProbe`), an
   attach client in the app's own process (the library's
   `calcu::qt6::runPacingProbe` over `AttachLineClient`; the lines and the bound are `main.cpp`'s): `ticks` over `<s>` seconds against wall
   time, exit 0 or 1; run it offscreen with a temporary `HOME`, so no
   settings of the user's are touched. Release builds and Windows have neither.
7. **Tests:** `CalcU59QtTests` (`frontends/qt6/tests/`, Frequent in a
   tree with `CALCU_FRONTEND_QT=ON`; offscreen, a `HOME` of the build
   tree's): the real window (`tests/QtFixture`, the bundled examples from
   `examples/`, the file dialogs and the browser replaced by recorders) on
   the real clock; the scenarios of LIB-14 AC-25 under their IDs
   (`lib14_acN_…`, the Web's of the same IDs in `frontends/web/tests/shell.spec.js`).
   `QtResourceBundle::hideForTests` makes the ROMs-missing case.
8. **Commit often, never push.**

## Class map

| Qt (`frontends/qt6/app/`) | Swift counterpart |
|---|---|
| `main.cpp` | `Calc-U-59App.swift` |
| `MainWindow` | `CalculatorView` (layout, menus, presets, drag and drop: every file to LIB-13's `open` through the library's `library/qt6/FileOpening` (`calcu::qt6::openFile(window, …)`, which raises the window but for the start, `dragEnter`, `dragMove`, `drop` (accepted, opened after the drop: LIB-13 AC-24), `startFile`, the macOS `FileOpenFilter` in `main.cpp`); Arm Fresh Card's replace question through `calcu::qt6::sendAsking`) |
| `EmulatorController` | `EmulatorViewModel` |
| `CalculatorPanel` (the faceplate, the shell's control bar and "ROMs missing") | `CalculatorView` calculator column |
| the shell's control bar (`calcu::qt6::Shell::controlBar()`), `CardControls` | `CalculatorView` bar and card buttons |
| `KeyboardWidget` (the faceplate: art, LED, cue card, pressed key from `held-keys`, host keys through `calcu::qt6::HostKeys`) | `KeyboardView` |
| `LedRenderer` | `LEDDisplayView` |
| `CueCardRenderer` | `CueCardView` |
| the library's `calcu::qt6::PaperPanel` in the shell's panel area (LIB-8, LIB-14 AC-23) | `PrinterView` |
| `CardPickerDialog` | `CardPickerView` |
| `debug/DebugPanel`, `debug/AsmOverlayBox`, `debug/LogWidget` | `DebugView` (`ASMDebugContent`, `StaticDebugContent`); they draw `TIDebugPart`'s product topics through `EmulatorController`, which keeps the LOG tab's view (`cu59::RingLog`) of the `log` topic; Run, TRACE and the CALCULATOR tab follow the product entries (`entryAvailable`) |
| `debug/LiveDebugWidget` | `LiveDebugView` |
| `debug/CpuInspectorWidget`, `debug/RomHeatmapWidget` | `CPUInspectorView` |
| the library's `calcu::qt6::RunControlButtons` (FREEZE, STEP, RESUME of each debug tab, LIB-10 since M-53) | `HoldRepeatButton.swift` |
| `ChromeStyle.hpp` | `ChromeControlStyles.swift` |
| the shell's `ShellSettingsDialog` (`cu59::settingsSpec`), `AppSettings.hpp`, `AppPaths` | `SettingsView`, `AppSettings.swift`, `CardStorage.swift` |
| the shell's `ShellAbout` (`cu59::shellProduct`'s texts and links, the window icon, "Built with Qt") | `AppInfo.swift` |

## Build

```bash
../../tools/run_qt.sh 59 --build      # Release build into ../../build/app-59 (configures once)
../../tools/run_qt.sh 59 [file.ti59]  # build and start (user's call — see rule 6)
cmake -S ../.. -B ../../build/all -G Ninja && cmake --build ../../build/all   # root project: both products, tests + Qt apps
```

All C++ builds go through the repository's root `CMakeLists.txt` (ADR 0003;
options `CALCU_PRODUCT_59`, `CALCU_PRODUCT_1600`, `CALCU_FRONTEND_QT`); the
product directory and `frontends/qt6/` do not configure on their own. The
packaging jobs in CI configure the root into `build/release`
(`-DCALCU_PRODUCT_1600=OFF -DCALCU_TESTS=OFF`) and build `CalcU59Qt`; the
`.iss` includes `Version.iss` from that tree.
Qt comes from Homebrew on macOS (`/opt/homebrew/opt/qt`, found automatically).
The version is read from the Xcode project's `MARKETING_VERSION`.

## Packaging (CI only)

- **Linux** x86_64 / aarch64: linuxdeploy + Qt plugin → AppImage, with the
  Wayland and platform-theme plugins and a GTK-theme `AppRun` wrapper.
- **Windows** x86_64 / arm64: windeployqt → Inno Setup
  (`frontends/qt6/resources/windows/calc-u-59.iss`, preset associations).
- `LICENSE`, `NOTICE`, `THIRD-PARTY-NOTICES.md` and `licenses/` (Qt LGPLv3)
  ship in both packages: next to the Windows executable, and in
  `usr/share/doc/calc-u-59/` inside the AppImage.
