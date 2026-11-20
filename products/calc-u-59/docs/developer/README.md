# Developer documentation

For people who build Calc-U-59 from source or work on the emulator
itself. Using the emulator (including its debugger) is covered by the
help (below, "The help").

## Build and test

- [README.md](../../README.md) § "Building" — the Swift app, the root
  CMake project with the tests and the Qt6 app, the web client.
- `tools/run_tests.sh calc-u-59 [filter]` at the repository root — every
  Frequent CTest entry of the product, `-O1`, in the repository's
  `build/tests` tree; a filter runs the `TEST`s whose name contains it.
- [roms/README.md](../../roms/README.md) — where each ROM image comes from;
  `roms/fetch_roms.sh` downloads them.
- [CLAUDE.md](../../CLAUDE.md) — the project instructions for Claude Code.
- [TODO.md](../../TODO.md) — known issues and open work;
  [CHANGELOG.md](../../CHANGELOG.md) — what changed in each release.

## How the emulator works

| Document | Abstract |
|---|---|
| [Core-Architecture.md](Core-Architecture.md) | The C++ emulation core (`core/`): TMC0501 CPU, BCD register model, SCOM memory map, instruction set, display/keyboard/card/printer subsystems, machine-variant differences. |
| [App-Layer.md](App-Layer.md) | The app layer (`app/`) all three GUIs use: modules, the app session's part `TIAppPart`, threads, the real-time emulation loop, key input and the physical keyboard map, card reader, presets (the library's loader with the TI rules), the TI-58C constant memory, ROM loading, drawing rules; the Swift app and its ObjC++ `frontends/swift/Bridge/` layer; tests and goldens. |
| [Frontend-Parity.md](Frontend-Parity.md) | Feature-by-feature table of the Swift app, the Qt6 app and the web client — where each feature lives and what is platform-specific. |
| [Web-Client.md](Web-Client.md) | The web client (`frontends/web/`): layout, build, and the contract of the `frontends/web/dist/` folder the website embeds — mounting `CalcU59`, location independence, no side effects in render, styling, the PWA, `MANIFEST.json`. |
| [Debug-API.md](Debug-API.md) | The debugger across the core (trace, breakpoints, CpuFrame), the app layer (`DebugModel`, snapshots, LOG, trace files, ASM overlay) and the debug panels of all three GUIs. |
| [Harness.md](Harness.md) | The harness `calcu --product c59` without a window: ROM traces of key sequences, timing and profiling presets, the `c59` commands, the scripts that stay (`tools/coverage.py`, `tools/fastmode_matrix.py`, `tools/calendar/`). |
| [Idle.md](Idle.md) | When the machine takes a key, waits, has settled or has finished: the signals and how each question is answered from them. |
| [New-GUI-Guide.md](New-GUI-Guide.md) | Porting guide for adding another GUI frontend: building on `app/` (the Qt6 app as the worked example), or compiling `app/` for a platform without threads (the web build as the example). |

How the ROMs work: [rom/TI59-ROM-Guide.md](../../rom/TI59-ROM-Guide.md),
[rom/TI58C-ROM-Guide.md](../../rom/TI58C-ROM-Guide.md); the CPU traces they
cite are in [rom/traces/](../../rom/traces/README.md).

## File formats

The formats are specified at the repository root, in
[docs/formats/](../../../../docs/formats/README.md): the preset format 2
(`.ti59`, `.ti58`, `.ti58c`) and the keystroke program file
([preset.md](../../../../docs/formats/preset.md)), the magnetic card and the
TI-58C's constant memory file ([media.md](../../../../docs/formats/media.md)),
the Solid State ROM module ([modules.md](../../../../docs/formats/modules.md)),
the cue card block ([cue-card.md](../../../../docs/formats/cue-card.md)) and
the binary trace file ([trace.md](../../../../docs/formats/trace.md)). The
`c59` harness commands are in [Harness.md](Harness.md).

## The help

The user guide is the help (on the web at <https://calc-u.org/59/>),
Markdown in the source repository: [help/](../../help/) holds Calc-U-59's
product parts (`help/help.yaml` lists them), the family's shared parts are
in [library/help/](../../../../library/help/). `tools/help check calc-u-59`
(at the repository root) checks it (Frequent, `CalcU59HelpCheck`), and
`tools/help build calc-u-59` writes `Calc-U-59-help.zip`. Developer
documents stay here in `docs/developer/` and are not help. A user-visible
change updates its chapter in the same step.
