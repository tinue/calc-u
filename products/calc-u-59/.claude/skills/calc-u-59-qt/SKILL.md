---
name: calc-u-59-qt
description: Use this skill when working on the Qt6 desktop app of Calc-U-59 (frontends/qt6/, frontends/qt6/app/, frontends/qt6/resources/, `.github/workflows/calc-u-59.yml` (repository root), tools/build_qt.sh, build_and_run.sh, tools/gen_qt_icons.sh). Covers the class map against the Swift views, threading (the library's app session on the GUI thread), building, packaging for Windows and Linux, and the parity rule with the Swift app. Trigger on any task that touches frontends/qt6/ or the Qt CI workflow.
user-invocable: true
---

# Calc-U-59 Qt6 App

**Ship scope first:** the Qt app ships on **Windows and Linux** only, built
by the repository's `.github/workflows/calc-u-59.yml` and attached to releases by the root
`release.yaml`. The **macOS Qt build is local and for development only** —
never shipped, never built in CI, never signed. On the Mac the Swift app is
the product. The Qt app has no iCloud: cards, `ti58c.mem` and traces live in
`~/Calc-U-59`, each folder (and where Open Preset starts) set in Settings, as
in Calc-U-1600 (`AppPaths`, `AppSettings::OpenFolder`).

Structure follows the sibling project Calc-U-1600
(`~/Development/sharp/Calc-U-1600/Qt6`): Widgets, AUTOMOC/AUTORCC, no `.ui`
files, QPainter rendering.

Everything that isn't drawing: `reference/SharedArchitecture.md`.

## Rules

1. **Parity with the Swift app and the web client** (CLAUDE.md § "Frontend
   Parity"): a user-visible change here needs the matching Swift change, and
   the web change if the feature is in the web's scope, on the same branch —
   or a `Pending (Swift)` / `Pending (Web)` row in `reference/FrontendParity.md`.
   Update the table either way.
2. **No logic in `frontends/qt6/app/`.** Anything that isn't Qt widgets, painting or
   file dialogs belongs in `app/` (calc-u-59-shared skill) so the Swift app
   gets it too. `EmulatorController` is a thin QObject frontend of the
   library's app session (`cu59::makeAppSession`, LIB-3 AC-28): it calls the
   session at 60 Hz, sends `c59.*` commands, reads a `TopicMirror` and turns
   its topic changes into Qt signals; notices are its alerts. Since M-53 it
   states and answers `clipboard-image` and `clipboard-text` with the
   library's `ClipboardService` (LIB-8 adapter P-E6, until LIB-14's
   `SessionHost` in M-57), and hands the library's `papers`, `paper …`,
   `run-state` and `trace` topics to the widgets (`addMessageHandler`).
   The paper is the library's paper panel (`PaperPane`; owner and controls
   from `c59.printer`, adapter P-E1); FREEZE, STEP and RESUME are the
   library's run-control buttons of the levels `program` (CALCULATOR) and
   `cpu` (CPU), FREEZE ON START sends `pause-on-start`, TRACE sends
   `trace {button: true}` (LIB-10).
3. **One thread.** The session lives on the GUI thread; there is no
   emulation thread and no queue to hop. Qt6 sends neither suspend nor
   continue (LIB-3 AC-31; CTest `CalcU59NoOwnSession`).
4. **Assets have one source:** ROMs from `roms/` and artwork from
   `frontends/swift/App/Assets.xcassets` are compiled in as Qt resources by `frontends/qt6/CMakeLists.txt`;
   icons come from `frontends/swift/App/AppIcon.icon` via `tools/gen_qt_icons.sh`. Never copy them.
5. **Settings** are declared in `app/Settings.hpp` and live in
   `Calc-U-59.ini` (LIB-9); `frontends/qt6/app/AppSettings.hpp` only wraps
   them in QString calls, and `AppPaths.cpp` opens the file and takes every
   folder from `calcu::settings::Paths`. No QSettings. The window is
   `[window]` numbers (`cu59::settings::WindowState`).
6. **Compile, don't run.** Build with the commands below. To check rendering
   without showing a window: `QT_QPA_PLATFORM=offscreen ../../build/app-59/products/calc-u-59/frontends/qt6/Calc-U-59.app/Contents/MacOS/Calc-U-59 [state-file] --grab out.png --after 3000`
   (an offscreen screen may clamp the window size). Ask the user to try the app itself.
   A build with the tests also has `--pacing-probe <s>` (LIB-17 AC-19, CTest
   `CalcU59QtPacingProbe`): emulated against wall time over `<s>` seconds, exit 0 or 1;
   run it offscreen with a temporary `HOME`, so no settings of the user's are touched.
7. **Tests:** `CalcU59QtTests` (`frontends/qt6/tests/`, Frequent in a
   tree with `CALCU_FRONTEND_QT=ON`; offscreen, a `HOME` of the build
   tree's) builds the app's widgets on a real `EmulatorController`.
8. **Commit often, never push.**

## Class map

| Qt (`frontends/qt6/app/`) | Swift counterpart |
|---|---|
| `main.cpp` | `Calc-U-59App.swift` |
| `MainWindow` | `CalculatorView` (layout, menus, presets, drag and drop) |
| `EmulatorController` | `EmulatorViewModel` |
| `CalculatorPanel`, `ControlBar` | `CalculatorView` calculator column and card bar |
| `KeyboardWidget` | `KeyboardView` |
| `LedRenderer` | `LEDDisplayView` |
| `CueCardRenderer` | `CueCardView` |
| `PaperPane` (the library's `calcu::qt6::PaperPanel` with the PC-100C's connect toggle above it, LIB-8 since M-53) | `PrinterView` |
| `CardPickerDialog` | `CardPickerView` |
| `debug/DebugPanel`, `debug/AsmOverlayBox`, `debug/LogWidget` | `DebugView` (`ASMDebugContent`, `StaticDebugContent`) |
| `debug/LiveDebugWidget` | `LiveDebugView` |
| `debug/CpuInspectorWidget`, `debug/RomHeatmapWidget` | `CPUInspectorView` |
| the library's `calcu::qt6::RunControlButtons` (FREEZE, STEP, RESUME of each debug tab, LIB-10 since M-53) | `HoldRepeatButton.swift` |
| `ChromeStyle.hpp` | `ChromeControlStyles.swift` |
| `SettingsDialog`, `AppSettings.hpp`, `AppPaths` | `SettingsView`, `AppSettings.swift`, `CardStorage.swift` |
| `AboutDialog` | `AppInfo.swift` |

## Build

```bash
tools/build_qt.sh                 # Release build into ../../build/app-59 (configures once)
./build_and_run.sh [file.ti59]    # build and start (user's call — see rule 6)
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
  (`frontends/qt6/resources/windows/calc-u-59.iss`, state-file associations).
- `LICENSE`, `NOTICE`, `THIRD-PARTY-NOTICES.md` and `licenses/` (Qt LGPLv3)
  ship in both packages: next to the Windows executable, and in
  `usr/share/doc/calc-u-59/` inside the AppImage.
