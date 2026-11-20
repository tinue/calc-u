# Calc-U-59

The product rules on top of the repository's `CLAUDE.md`.

## Skills

Invoke the right skill before starting work in each area:

| Skill | Invoke for |
|-------|-----------|
| `calc-u-59-core` | C++ emulation core (`core/`, `tools/`, ROM disassembly, trace infrastructure) |
| `calc-u-59-app` | The app layer (`app/`, `tests/`, the product's `CMakeLists.txt`) — non-GUI logic all three frontends use |
| `calc-u-59-swift` | Swift/SwiftUI app (`frontends/swift/App/`, `frontends/swift/App/Views/`, `frontends/swift/Bridge/`) |
| `calc-u-59-qt` | Qt6 desktop app (`frontends/qt6/`, the repository's `.github/workflows/calc-u-59.yml`, Qt packaging) |
| `calc-u-59-web` | Web client (`frontends/web/`: React + WASM engine, PWA, `frontends/web/build.sh`, `frontends/web/dist/`, the `web` CI job) |
| `calc-u-59-docs` | Documentation: the user help (`help/`), `docs/developer/`, root markdown files |
| `calc-u-59-presets` | Writing `.ti59`/`.ti58`/`.ti58c` presets of format 2, keystroke program files, `examples/bundle.yaml` — the TI sections, key IDs, checking a preset with `calcu` |
| `calc-u-59-uitesting` | XCUITest UI tests (`frontends/swift/Calc-U-59UITests/`) — file picker navigation, accessibility identifiers, orientation |

## Help

User help is `help/` (the product parts, `help/help.yaml`) and
`library/help/` at the repository root (the shared parts), checked by
`tools/help check calc-u-59` (`CalcU59HelpCheck`, Frequent) and built into
the release's `Calc-U-59-help.zip`. Developer documents stay in
`docs/developer/`.

## Architecture Reference

Architecture facts (register model, SCOM layout, PC encoding, machine variants, trace API) live in:

- `docs/developer/Core-Architecture.md` — C++ emulation core
- `docs/developer/App-Layer.md` — the app layer `app/`, its one thread, and the Swift app and bridge layer
- `docs/developer/Debug-API.md` — debug panel and bridge API
- `docs/developer/Frontend-Parity.md` — which feature lives where in the three GUIs
- `docs/developer/Web-Client.md` — the web client and its `dist/` contract
- `docs/developer/Harness.md` — the harness `calcu --product c59`: ROM traces, timing and profiling, the `c59` commands, the scripts that stay
- `docs/formats/preset.md`, `docs/formats/cue-card.md` (repository root) — the preset format 2 (`.ti59`/`.ti58`/`.ti58c`) and the cue card block
- `rom/TI59-ROM-Guide.md`, `rom/TI58C-ROM-Guide.md` — how the ROMs work

Do not restate these facts in CLAUDE.md, skills, or memory files — link to the reference doc instead.

## Harness

`calcu --product c59` is the product without a window, in emulated time
only, so two runs give the same replies. Build the target `calcu` of a
root tree (`cmake --build ../../build/tests --target calcu`; the program is
`build/tests/library/harness/cli/calcu`). The common commands, replies and
options are in `docs/formats/harness.md` (repository root); the `c59`
commands, the ROM trace recipes and the time base are in
`docs/developer/Harness.md`: read it before tracing the ROM or measuring
program speed. Scripts on the harness (`tools/calcu59.py`):
`tools/coverage.py`, `tools/fastmode_matrix.py`, `tools/calendar/`;
output in `tools/build/`.

## Frontend Parity (Swift app ↔ Qt6 app ↔ web client)

Three GUIs share `core/` (hardware) and `app/` (everything else that isn't
drawing): the Swift app (macOS, iOS — the Mac product, with iCloud), the
Qt6 app (Windows and Linux; its macOS build is for development only, never
shipped and never built in CI) and the web client (`frontends/web/`, embedded by the
website; no iCloud, debugger only side by side (its wide layout, the PWA in
landscape on a large screen) — its column in
`docs/developer/Frontend-Parity.md` has the scope). Keep them in step:

- **Non-GUI logic goes in `app/` only** — parsing, file formats, timing,
  key sequencing, snapshot building, layout rules and numbers the renderers
  use. Never write such logic in `frontends/swift/App/*.swift`, `frontends/qt6/app/` or `frontends/web/`; put it
  in `app/` and call it (frontends/swift/Bridge/ wrapper for Swift, directly from Qt,
  `frontends/web/bindings.cpp` for the web).
- **`frontends/swift/Bridge/`, the Qt adapters and `frontends/web/bindings.cpp` translate types only.**
- **The Qt6 and Web windows are put together from the library's shell**
  (the Swift frontend natively: `Bridge/TIShell` computes the same models,
  SwiftUI draws them): the menus, the control bar (and the
  More button where there is no menu bar), the settings dialog, About,
  Help, notices and "Loading…" are drawn from the library's models over
  the control state, with the product's data in `app/TIShellProduct`. A
  new menu item or setting row goes into that model (`shellProduct()`,
  `settingsSpec()`), never into one frontend; the model, the module, the
  printer and the card controls are drawn from `control-state` only (CTest
  `CalcU59FrontendsDrawControlState`).
- **`app/` never resolves locations.** It takes paths, and ROM files only
  through the library's ROM lookup (a `calcu::roms::Bundle` the frontend
  makes; `app/ProductRoms.hpp`);
  iCloud lives only in `frontends/swift/App/CardStorage.swift`; the frontend
  passes that folder in, and every other place (the settings file, traces)
  comes from the library's `calcu::settings::Paths`.
- **A user-visible change in one GUI needs the matching change in the others
  on the same branch** — or a `Pending (Qt)` / `Pending (Swift)` /
  `Pending (Web)` row with a reason in `docs/developer/Frontend-Parity.md`. Features
  outside the web's scope need no web change (its column says so). Update
  that table either way. `tools/check_parity.sh` flags a branch that touches
  one GUI only.
- **Every change to `core/` or `app/` must pass all builds before committing:**
  `../../tools/run_tests.sh calc-u-59` (the repository's one test script:
  every Frequent CTest entry of the product, `-O1`, in the root CMake
  project's `build/tests` tree; `docs/test-groups.md` at the root lists
  every test and its group; `../../tools/run_tests.sh calc-u-59 <substring>`
  runs the matching `TEST`s of the one runner `CalcU59Tests`,
  `library/testing/`),
  `xcodebuild -project frontends/swift/Calc-U-59.xcodeproj -scheme Calc-U-59 -destination platform=macOS build`,
  and the web client, which compiles both layers too:
  `frontends/web/build.sh && frontends/web/run-tests.sh` (`frontends/web/dist/` is gitignored; CI builds it).
  `app/` is C++20 and standard-library-only, and runs on one
  thread: the app session's (Qt6's GUI thread, the library's
  `shell/SessionThread` for the Swift frontend, the Web's Worker).
- **Settings** are declared once, in `app/Settings.hpp` (`settingsDecl()`:
  section, key, default, accepted values), read and written
  through the library's `calcu::settings::SettingsFile` (its `Paths`,
  `settings::startupModel`, `settings/OpenFolders`, the library's
  `SettingsPart`), and live in `Calc-U-59.ini`. `Settings` keeps typed
  calls only for what a frontend reads itself (the Qt6 window state,
  `frontends/qt6/app/AppSettings.hpp`). The Swift
  frontend's file is the session's (the library's `SettingsPart`, read
  through `Bridge/TIShell`); no frontend keeps a setting in UserDefaults, QSettings or
  `localStorage` (CTest `CalcU59NoPlatformSettings`, allow-list
  `tools/platform_stores_allowed.txt`).
- **Assets have one source:** ROMs in `roms/`, artwork in `frontends/swift/App/Assets.xcassets`,
  the icon in `frontends/swift/App/AppIcon.icon` (`tools/gen_qt_icons.sh` derives the Qt icons;
  `frontends/web/build.sh` copies the Linux one), the bundled presets in `examples/`, exactly the files `examples/bundle.yaml` lists (all three frontends bundle them).
- **Version:** `MARKETING_VERSION` in the Xcode project is the only version;
  the Qt build and the web build read it.

