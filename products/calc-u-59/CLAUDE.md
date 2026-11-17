# Project Context

## Skills

Invoke the right skill before starting work in each area:

| Skill | Invoke for |
|-------|-----------|
| `calc-u-59-core` | C++ emulation core (`core/`, `tools/`, ROM disassembly, trace infrastructure) |
| `calc-u-59-app` | The app layer (`app/`, `tests/`, the product's `CMakeLists.txt`, `tools/goldens/`) — non-GUI logic all three frontends use |
| `calc-u-59-swift` | Swift/SwiftUI app (`frontends/swift/App/`, `frontends/swift/App/Views/`, `frontends/swift/Bridge/`) |
| `calc-u-59-qt` | Qt6 desktop app (`frontends/qt6/`, the repository's `.github/workflows/calc-u-59.yml`, Qt packaging) |
| `calc-u-59-web` | Web client (`frontends/web/`: React + WASM engine, PWA, `frontends/web/build.sh`, `frontends/web/dist/`, the `web` CI job) |
| `calc-u-59-docs` | Documentation: the user help (`help/`, LIB-16), `reference/`, root markdown files |
| `calc-u-59-presets` | Writing `.ti59`/`.ti58`/`.ti58c` presets of format 2, keystroke program files, `examples/bundle.yaml` — the TI sections, key IDs, checking a preset with `calcu` |
| `calc-u-59-uitesting` | XCUITest UI tests (`frontends/swift/Calc-U-59UITests/`) — file picker navigation, accessibility identifiers, orientation |

## Branches

Calc-U-59 lives in `products/calc-u-59/` of the source repository
tinue/calc-u (ADR 0002). Work happens on short branches merged into `main`
(ADR 0029); a release is tagged `calc-u-59-vX.Y.Z` (ADR 0028). The
repository's `CLAUDE.md` holds the current phase and its rules; they come
first.

User help is `help/` (the product parts, one file or more per chapter,
`help/help.yaml`) and `library/help/` at the repository root (the shared
parts), checked by `tools/help check calc-u-59` in Frequent
(`CalcU59HelpCheck`; LIB-16) and built into the release's
`Calc-U-59-help.zip`; a user-visible change updates its chapter in the
same step. Developer documents stay in `reference/`.

## Architecture Reference

Architecture facts (register model, SCOM layout, PC encoding, machine variants, trace API) live in:

- `reference/CoreArchitecture.md` — C++ emulation core
- `reference/AppLayer.md` — the app layer `app/` and its one thread
- `reference/AppArchitecture.md` — Swift app and bridge layer
- `reference/DebugAPI.md` — debug panel and bridge API
- `reference/FrontendParity.md` — which feature lives where in the three GUIs
- `reference/WebClient.md` — the web client and its `dist/` contract
- `reference/Harness.md` — the harness `calcu --product 59`: ROM traces, timing and profiling, the `ti59` commands, the scripts that stay
- `docs/formats/preset.md`, `docs/formats/cue-card.md` (repository root) — the preset format 2 (`.ti59`/`.ti58`/`.ti58c`) and the cue card block
- `rom/TI59-ROM-Guide.md`, `rom/TI58C-ROM-Guide.md` — how the ROMs work

Do not restate these facts in CLAUDE.md, skills, or memory files — link to the reference doc instead.

## Harness

`calcu` is the family's harness (LIB-11, `features/LIB-11-harness/` at the
repository root): the product without a window, on the library's app
session with the product's app layer `app/TIAppPart` as the frontends host
it (built in `prepare()` on the run's settings and folders,
`app/HarnessProduct.cpp`), in emulated time only, so two runs give the same
replies. It replaced `tools/headless/` and `tools/agentic/` in M-59. Read
`reference/Harness.md` before tracing the ROM or measuring or comparing
program speed (the time base, the recipes, the pitfalls). Build
the target `calcu` of a root tree (`cmake --build ../../build/tests --target
calcu`; the program is `build/tests/library/harness/cli/calcu`). It finds the
ROMs in `roms/` (`roms/fetch_roms.sh`), the bundled presets in `examples/`
(`examples/bundle.yaml`), and starts the startup model (the TI-59 under the
default settings) with its startup preset or its built-in default: the
Master Library module, no printer (LIB-18 AC-18), at key readiness.

- One-shot: `calcu --product 59 wait 2s then key-down 7 then wait 100ms then key-up 7 then wait 500ms then lcd`
  (commands parted by `then`; exit status 0, 1 at the first refusal, 2 when
  it cannot start).
- `calcu --product 59 serve`: one command per line on standard input, one
  reply per line (`done`, `done <n>` and n lines, `refused: <reason>`);
  `--json` gives one object `{"done","result","reason"}` per reply. From
  Python: `library/harness/python/calcu_serve.py`.
- First commands: `wait <2s|500ms>`, `key-down`/`key-up <key ID>` (raw),
  `key <name> …` (LIB-5: each pressed in turn as the app types it, 100 ms
  and until the ROM read it; the names of `docs/formats/preset.md` §8.1,
  `2nd Pgm`, `x^2`, `RTN`; replies when the last key is up; no `type` on a
  TI model), `lcd`,
  `display`, `state <topic>`, `capabilities`, `ticks`, `quit`
  (`calcu --help` lists them). Machine control (LIB-18): `model <TI-59|TI-58|TI-58C>`,
  `attach <place> <peripheral>` (`attach slot-1 ML`), `detach <place>`, `reset`,
  `cold-boot`, `power-cycle`; each replies when the machine is ready for keys
  again. No settings or save folder of yours is
  touched unless `--settings`, `--save-folder`, `--output-folder` name them.
- Presets (LIB-6, C59-2): `preset <file>|examples:<path>` (format 2 only;
  a 2.3.0 file is refused with the hint), `check-preset <file>…`,
  `preset-list`, `wait` without a duration (until idle), `expect <number>` /
  `not-expect <number>` (X in 13 digits), `screenshot <name>` (the classic
  segment picture, into `--output-folder`), and the steps as commands
  (`printer-trace on|off`, `elapsed-time start|report`, `full-speed`, …).
  The product's sections and rules: `app/TIPresetKeys`, `app/TIPresetRules`.
- Program memory (C59-2 AC-9): `ti59 partition [<value>]`, `ti59 program
  [<step>] <keycode>…` (write) and `ti59 program <step> count=<n>` (read),
  `ti59 registers <NN>=<value>…` (write; `H00`–`H03` on the TI-58C) and
  `ti59 registers <NN>…` (read).
- Paper (LIB-8): `paper text|marks|png <path>`, `cut-paper`; debugger
  (LIB-10): `pause`, `step`, `resume`, `registers`, `break`, `trace <file>|off`, ….
- Debug panel (C59-4): `ti59 asm load <file>|run|clear` (the ASM overlay,
  format 2.0, `examples/assembly/README.md`), `ti59 dump
  scom|memory|prog|vars`, `ti59 log-level off|info|debug`, `ti59 heatmap
  reset`; `state calculator-debug|program-listing|cpu-debug|heatmap|asm-overlay|debug-view`
  reads what the panel shows (`app/TIDebugPart`).
- Opening a file (LIB-13): `open <path>` opens it as a drop does: a preset
  by its extension or its `model:` line, an `.asm` overlay, a
  `.keystrokes.yaml` program written into the running machine from step 000;
  `.rom.yaml` and `ti58c.memory.yaml` are refused (`app/TIFileKinds`).
- Product commands (C59-9, `app/TIHarnessCommands`, the run facts in
  `app/TIRunProbe`, the assembly in `app/TIMnemonics`): `ti59 run-time
  <key>…`, `ti59 line-times [limit=<d>]`, `ti59 hot-steps [limit=<d>]`,
  `ti59 step-trace <from> <to>`, `ti59 step-cost <keycode>…`, `ti59 coverage
  [all]` (after `profile on`), `ti59 keycodes list|disassemble [<first>
  [<last>]]` and `ti59 keycodes assemble <file>`. A ROM trace of keys:
  `trace <file>`, the keys, `trace off`, then `tools/read_trace.py
  --labels rom/TI59-labels.asm --changes <file>`.
- Machine state (LIB-11 AC-24, C59-9 AC-15): `state-save <file>`,
  `state-restore <file>` (`core/TIMachineState.*`; the fields of each part
  in its `stateIo` template, the records in `core/TIStateRecords.hpp`).
- Scripts on the harness (`tools/calcu59.py`): `tools/coverage.py`,
  `tools/fastmode_matrix.py`, `tools/calendar/`; output in `tools/build/`.

## Frontend Parity (Swift app ↔ Qt6 app ↔ web client)

Three GUIs share `core/` (hardware) and `app/` (everything else that isn't
drawing): the Swift app (macOS, iOS — the Mac product, with iCloud), the
Qt6 app (Windows and Linux; its macOS build is for development only, never
shipped and never built in CI) and the web client (`frontends/web/`, embedded by the
website; no iCloud, debugger only side by side (its wide layout, the PWA in
landscape on a large screen) — its column in
`reference/FrontendParity.md` has the scope). Keep them in step:

- **Non-GUI logic goes in `app/` only** — parsing, file formats, timing,
  key sequencing, snapshot building, layout rules and numbers the renderers
  use. Never write such logic in `frontends/swift/App/*.swift`, `frontends/qt6/app/` or `frontends/web/`; put it
  in `app/` and call it (frontends/swift/Bridge/ wrapper for Swift, directly from Qt,
  `frontends/web/bindings.cpp` for the web).
- **`frontends/swift/Bridge/`, the Qt adapters and `frontends/web/bindings.cpp` translate types only.**
- **The Qt6 and Web windows are put together from LIB-14's shell** (M-57;
  the Swift frontend natively since M-58, C59-8: `Bridge/TIShell` computes
  the same models, SwiftUI draws them): the menus, the control bar (and the
  More button where there is no menu bar), the settings dialog, About,
  Help, notices and "Loading…" are drawn from the library's models over
  the control state, with the product's data in `app/TIShellProduct`. A
  new menu item or setting row goes into that model (`shellProduct()`,
  `settingsSpec()`), never into one frontend; the model, the module, the
  printer and the card controls are drawn from `control-state` only (CTest
  `CalcU59FrontendsDrawControlState`).
- **`app/` never resolves locations.** It takes paths, and ROM files only
  through the library's ROM lookup (a `calcu::roms::Bundle` the frontend
  makes; LIB-12, `app/ProductRoms.hpp`);
  iCloud lives only in `frontends/swift/App/CardStorage.swift`; the frontend
  passes that folder in, and every other place (the settings file, traces)
  comes from the library's `calcu::settings::Paths` (LIB-9).
- **A user-visible change in one GUI needs the matching change in the others
  on the same branch** — or a `Pending (Qt)` / `Pending (Swift)` /
  `Pending (Web)` row with a reason in `reference/FrontendParity.md`. Features
  outside the web's scope need no web change (its column says so). Update
  that table either way. `tools/check_parity.sh` flags a branch that touches
  one GUI only.
- **Every change to `core/` or `app/` must pass all builds before committing:**
  `cmake -S ../.. -B ../../build/tests -G Ninja -DCALCU_FRONTEND_QT=OFF && cmake --build ../../build/tests && ctest --test-dir ../../build/tests/products/calc-u-59 --output-on-failure`
  (the repository's root CMake project, ADR 0003; that tree holds the
  Frequent group only, ADR 0031 and `docs/test-groups.md` at the root list
  every test and its group; `CalcU59Tests <substring>` runs the matching
  `TEST`s of the one runner, `library/testing/`, ADR 0038),
  `xcodebuild -project frontends/swift/Calc-U-59.xcodeproj -scheme Calc-U-59 -destination platform=macOS build`,
  and the web client, which compiles both layers too:
  `frontends/web/build.sh && frontends/web/run-tests.sh` (`frontends/web/dist/` is gitignored; CI builds it).
  `app/` is C++20 (ADR 0005) and standard-library-only, and runs on one
  thread: the app session's (Qt6's GUI thread, the Swift frontend's
  `SessionThread`, the Web's Worker since M-34).
- **Settings** are declared once, in `app/Settings.hpp` (`settingsDecl()`:
  section, key, default, accepted values; LIB-9), read and written
  through the library's `calcu::settings::SettingsFile` (its `Paths`,
  `settings::startupModel`, `settings/OpenFolders`, LIB-14's
  `SettingsPart`), and live in `Calc-U-59.ini`. `Settings` keeps typed
  calls only for what a frontend reads itself (the Qt6 window state,
  `frontends/qt6/app/AppSettings.hpp`; the Swift trace location). The Swift
  frontend's file is the session's (LIB-14's `SettingsPart`, read through
  `Bridge/TIShell`; C59-8 D-3); no frontend keeps a setting in UserDefaults, QSettings or
  `localStorage` (CTest `CalcU59NoPlatformSettings`, allow-list
  `tools/platform_stores_allowed.txt`).
- **Assets have one source:** ROMs in `roms/`, artwork in `frontends/swift/App/Assets.xcassets`,
  the icon in `frontends/swift/App/AppIcon.icon` (`tools/gen_qt_icons.sh` derives the Qt icons;
  `frontends/web/build.sh` copies the Linux one), the bundled presets in `examples/`, exactly the files `examples/bundle.yaml` lists (all three frontends bundle them).
- **Version:** `MARKETING_VERSION` in the Xcode project is the only version;
  the Qt build and the web build read it.

