# Project Context

## Skills

Invoke the right skill before starting work in each area:

| Skill | Invoke for |
|-------|-----------|
| `calc-u-59-core` | C++ emulation core (`core/`, `tools/`, ROM disassembly, trace infrastructure) |
| `calc-u-59-shared` | Shared C++ layer (`app/`, `tests/`, the product's `CMakeLists.txt`, `tools/goldens/`) — non-GUI logic all three frontends use |
| `calc-u-59-swift` | Swift/SwiftUI app (`frontends/swift/App/`, `frontends/swift/App/Views/`, `frontends/swift/Bridge/`) |
| `calc-u-59-qt` | Qt6 desktop app (`frontends/qt6/`, the repository's `.github/workflows/calc-u-59.yml`, Qt packaging) |
| `calc-u-59-web` | Web client (`frontends/web/`: React + WASM engine, PWA, `frontends/web/build.sh`, `frontends/web/dist/`, the `web` CI job) |
| `calc-u-59-docs` | Documentation (`reference/`, root markdown files); the user guide lives in `../calc-u-site` and is updated with them |
| `calc-u-59-statefiles` | Writing `.ti59`/`.ti58`/`.ti58c` state files — format, matrix codes, KEYSTROKES sequences |
| `calc-u-59-uitesting` | XCUITest UI tests (`frontends/swift/Calc-U-59UITests/`) — file picker navigation, accessibility identifiers, orientation |

## Branches

Calc-U-59 lives in `products/calc-u-59/` of the source repository
tinue/calc-u (ADR 0002). Work happens on short branches merged into `main`
(ADR 0029); a release is tagged `calc-u-59-vX.Y.Z` (ADR 0028). The
repository's `CLAUDE.md` holds the current phase and its rules; they come
first.

## Architecture Reference

Architecture facts (register model, SCOM layout, PC encoding, machine variants, trace API) live in:

- `reference/CoreArchitecture.md` — C++ emulation core
- `reference/SharedArchitecture.md` — the shared C++ layer and its runtime threads
- `reference/AppArchitecture.md` — Swift app and bridge layer
- `reference/DebugAPI.md` — debug panel and bridge API
- `reference/FrontendParity.md` — which feature lives where in the three GUIs
- `reference/WebClient.md` — the web client and its `dist/` contract
- `reference/StateFileFormat.md` — the `.ti59`/`.ti58`/`.ti58c` file format
- `rom/TI59-ROM-Guide.md`, `rom/TI58C-ROM-Guide.md` — how the ROMs work

Do not restate these facts in CLAUDE.md, skills, or memory files — link to the reference doc instead.

## Agentic tools

`tools/agentic/` holds command-line tools for running, timing and profiling
`.ti59` programs headless (run a file, per-line print times, run time per
key, hot steps, step costs, keycode disassembler/assembler). Read
`tools/agentic/README.md` before measuring or comparing program speed.
ROM-level traces of key sequences come from `tools/headless/` (its README).

## Harness

`calcu` is the family's harness (LIB-11, `features/LIB-11-harness/` at the
repository root): the product without a window, on the library's app
session, in emulated time only, so two runs give the same replies. Build
the target `calcu` of a root tree (`cmake --build ../../build/tests --target
calcu`; the program is `build/tests/library/harness/cli/calcu`). It finds the
ROMs in `roms/` (`roms/fetch_roms.sh`) and starts the TI-59 with the PC-100C.

- One-shot: `calcu --product 59 wait 2s then key-down 7 then wait 100ms then key-up 7 then wait 500ms then lcd`
  (commands parted by `then`; exit status 0, 1 at the first refusal, 2 when
  it cannot start).
- `calcu --product 59 serve`: one command per line on standard input, one
  reply per line (`done`, `done <n>` and n lines, `refused: <reason>`);
  `--json` gives one object `{"done","result","reason"}` per reply. From
  Python: `library/harness/python/calcu_serve.py`.
- First commands: `wait <2s|500ms>`, `key-down`/`key-up <key ID>`, `lcd`,
  `display`, `state <topic>`, `capabilities`, `ticks`, `quit`
  (`calcu --help` lists them). No settings or save folder of yours is
  touched unless `--settings`, `--save-folder`, `--output-folder` name them.
- Not yet: `ti59 …` commands, typing, presets, traces, memory (later
  features). Until M-59, `tools/headless/` and `tools/agentic/` stay the
  tools for those.

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
- **`app/` never resolves locations.** It takes paths, and ROM files only
  through the library's ROM lookup (a `calcu::roms::Bundle` the frontend
  makes; LIB-12, `app/ProductRoms.hpp`);
  iCloud lives only in `frontends/swift/App/CardStorage.swift` (and `AppSettings.traceDirectory()`).
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
  `app/` is C++20 (ADR 0005) and standard-library-only, and must work without threads
  (`Threading::Cooperative`, used by the web build).
- **Settings both apps store** (key names, defaults, resolution) live in
  `app/SettingsKeys.hpp`; `frontends/swift/App/AppSettings.swift` and `frontends/qt6/app/AppSettings.hpp` use them.
- **Assets have one source:** ROMs in `roms/`, artwork in `frontends/swift/App/Assets.xcassets`,
  the icon in `frontends/swift/App/AppIcon.icon` (`tools/gen_qt_icons.sh` derives the Qt icons;
  `frontends/web/build.sh` copies the Linux one), presets the web offers in `examples/`.
- **Version:** `MARKETING_VERSION` in the Xcode project is the only version;
  the Qt build and the web build read it.

