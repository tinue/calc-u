# Developer documentation

For people who build Calc-U-1600 from source or work on the emulator
itself. Using the emulator (including writing PC-1500 / PC-1600 programs
with its debugger) is covered by the help (below, "The help").

## Build and test

- [Building.md](Building.md) — prerequisites, `libsharpdx`, getting the
  ROMs, per-platform builds, CLion, running the tests, packaging.
- [roms/README.md](../../roms/README.md) — where each ROM image comes from
  and its checksum; `roms/fetch_roms.sh` downloads them: the full set (21
  dumps and the CE-502B's module definition) that the one ROM lookup checks
  at every start (Building.md, "Get the ROMs").
- [Cloud-Sessions.md](Cloud-Sessions.md) — how the repository is set up for
  Claude Code cloud sessions; [CLAUDE.md](../../CLAUDE.md) holds the project
  instructions for Claude Code.
- [.github/macos-signing.md](../../.github/macos-signing.md) — how the CI
  signs and notarizes the macOS app.
- [TODO.md](../../TODO.md) — known issues, open work and the code-cleanup
  backlog.
- [CHANGELOG.md](../../CHANGELOG.md) — what changed in each release.

## Repository map

| Folder | What's in it |
|---|---|
| `core/` | The emulator, free of Qt: CPUs, machines, connectors and cards, loaders, the debugger's targets, views and listings (`core/Debug/`); `tests/` holds the test suite (presets are the library's; the resumable jobs the library's operations). The program loaders are `core/Loaders` (what a file is, BASIC, the one machine-code plan, its write and reply, one sentence per refusal) and `core/BasicTyping` (typing a listing in through the typing engine), on the loader memory access kind (`c1600.loader-memory`, `core/Machine/SharpLoaderMemory`); `core/ProgramFile` and `core/MachineCodeFile` hold the classifier and the plan |
| `app/` | The app logic every frontend shares, free of Qt: the live machine and its operations, the media kinds of the library's media part, pacing, settings, paths, layouts ([App-Layer.md](App-Layer.md)); the loaders' commands in `app/SharpLoaderPart` (`c1600.file-kind`, `load-basic`, `type-basic`, `code-plan`, `load-code`; the harness's `c1600 …`); `app/tests/` |
| `frontends/web/` | The web client: WASM bindings, worker, React UI, installable app, Playwright tests ([Web-Client.md](Web-Client.md)) |
| `frontends/qt6/` | The desktop app (`frontends/qt6/app/`) and its bundled resources: cards, faceplates, platform files (`frontends/qt6/resources/`) |
| `cmake/` | `CalcU.cmake`: the `calcu1600_core` / `calcu1600_shared` libraries for every CMake build |
| `firmware/` | Our own ROMs, e.g. the host-drive driver ROM (`firmware/pc1600-hostdrive/`) |
| `vscode/` | The VS Code debugger extension ([README](../../vscode/calcu1600-debug/README.md)); `tools/install_vscode_extension.sh` packages and installs it |
| `tools/` | Build scripts, `fetch_roms.sh`, `fetch_sharpdx.sh` / `refresh_sharpdx.sh`, `build_wasm_check.sh` / `build_sharpdx_wasm.sh`, `dap_smoke.py`, `read_trace.py`, `make_screenshots.sh` |
| `examples/` | User-facing samples, shipped as `Calc-U-1600-examples.zip` ([README](../../examples/README.md)); `examples/setup/` prepares memory modules ([README](../../examples/setup/README.md)) |
| `dev/` | Developer material that is not shipped: hardware checks and their presets ([README](../../dev/README.md)) |
| `headless/` | Gitignored scratch output of `calcu` runs and ad hoc checks ([README](../../headless/README.md)) |
| `release.yaml` | The release settings (assets, signing certificates) for the ReleaseIt tool |

## How the emulator works

- [App-Layer.md](App-Layer.md) — `core/`, the app layer `app/` and the
  frontends: the layering rules, the app session and `SharpAppPart`, resumable jobs, the
  WebAssembly check.
- [Web-Client.md](Web-Client.md) — the WebAssembly frontend: worker and
  bindings, the `dist/` contract for embedding, the installable app,
  storage, what it leaves out.
- [PC1600-Core-Limitations.md](PC1600-Core-Limitations.md) — the deliberate
  shortcuts, unmodelled parts and open assumptions in the PC-1600 core.
- [PC1600-Serial-Port.md](PC1600-Serial-Port.md) — the RS-232C port on a
  host pseudo-terminal: transport, line model, probe tooling.
- [PC1600-Host-Drive-Internals.md](PC1600-Host-Drive-Internals.md) — the
  host-drive ROM, its I/O protocol and FILE functions.
- [Debugger-Internals.md](Debugger-Internals.md) — how the DAP debugger is
  built: transport, run control, where the code is, design rules.
- [LCD-Text.md](LCD-Text.md) — reading the display as text: the ROM fonts,
  the cursor, the encoding, and where to use it (the harness's `display-text`,
  `expect:`, `c1600/display-text`).

## Sources

Comments and docs cite the original sources: the Technical Reference
Manual (TRM), the Service Manuals, data sheets and the ROMs (by bank and
address). Hardware facts collected from these sources are written up in
the research repository
[Sharp1500-1600-Ref](https://github.com/tinue/Sharp1500-1600-Ref). Comments
cite it as `Ref/<path>`, a path inside that repository, e.g.
`Ref/PC-1600/PC-1600-Keyboard.md §5`.

## File formats

- The `*.floppy.yaml` CE-1600F diskette format, for other tools that read
  or write it: [docs/formats/media.md §2](../../../../docs/formats/media.md#2-floppy-disk).
- The harness commands: [Harness.md](Harness.md); the DAP server:
  [DAP.md](DAP.md).
- Memory modules (`*.module.yaml`) are user-level:
  [09-module-definitions.md](../../help/09-module-definitions.md) and
  [10-module-definition-format.md](../../help/10-module-definition-format.md).
- Presets are described in the help,
  [6 Presets](../../help/06-presets.md) and its
  [Sharp sections](../../help/10-preset-sections.md).

## The help

The user guide is the help (on the web at
<https://calc-u.org/1600/>), Markdown in the source repository:

- [help/](../../help/) holds Calc-U-1600's product parts, chapter by
  chapter (`help/help.yaml` lists them), with their pictures in
  `help/images/`; the family's shared parts are in
  [library/help/](../../../../library/help/) at the repository root.
  The ten chapters are the family's; developer documents
  stay here in `docs/developer/` and are not help.
- `tools/help check calc-u-1600` (at the repository root) checks it: the
  chapters, every link and anchor, every picture used, no product word in
  a shared part, every settings key described in chapter 8
  (`help/settings-keys.txt`, written by `CalcU1600HelpTests` with
  `CALCU1600_HELP_KEYS=1`). It runs in Frequent (`CalcU1600HelpCheck`,
  `CalcU1600HelpBuild`); `tools/help build calc-u-1600` writes
  `Calc-U-1600-help.zip` (`build/help/`), which the release carries (the
  workflow's `help` job). The glossary's "Avoid" words are warnings
  (`tools/help check calc-u-1600` without `--quiet-warnings` lists them).
- A user-visible change updates its chapter in the same step.
- [screenshots/README.md](screenshots/README.md): the scripted
  scenarios, one per help chapter, behind `help/images/` (through harness
  attach), and how to take them again with `tools/make_screenshots.sh`
  (Checkpoint `CalcU1600HelpPictures`).
