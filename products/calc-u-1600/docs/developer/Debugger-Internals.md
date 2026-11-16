# Debugger — internals

How the DAP debugger is built, and the rules its code relies on. The user
manual is [../Debugger.md](../Debugger.md); the design history is in
[../background/plans/DAP-Debugger-Plan.md](../background/plans/DAP-Debugger-Plan.md)
and [../background/plans/Debugger-Use-Cases-Plan.md](../background/plans/Debugger-Use-Cases-Plan.md).

## Behaviour not in the manual

- **Transport:** the server listens on 127.0.0.1 only and takes one client
  at a time; a second one is told the debugger is busy. `--dap <port>`
  overrides Settings for one run.
- **Run control:** the library's (LIB-10, `library/debugger/`): the
  `DebuggerPart` among `SharpAppPart`'s parts is the machine session's gate
  whether or not a client is attached. Pause, step and resume come from
  the DAP, the harness (`calcu --product 1600`), Machine ▸ Resume and the
  session commands; the window's title says "Paused (debugger)" whoever
  paused it. A DAP `stopped` comes from the `run-state` topic after the
  host call the pause landed in (`DebugController::afterCall()`).
- **Traces:** the library's trace session (`trace` command, the
  INSPECTOR's TRACE button as `trace {button: true}`, a version 1
  preset's `trace:` through `PresetFile::traceHook`) writes the core's
  records (`c1600.trace-records`, `app/SharpTraceRecorder`) into the trace
  folder, limits every trace to `[trace] max-file-size-mb` MiB and says so
  in a notice. The Web has no trace folder: a trace fails there.
- **`launch` is `attach`:** `DapSession` treats a `launch` request as
  `attach`, which the CLion recipe relies on.
- **Installing the extension:** VS Code ignores extensions symlinked into
  `~/.vscode/extensions`. `tools/install_vscode_extension.sh` packages the
  `.vsix` into `headless/` and installs it.
- **"LH5801/PC-1500"** in VS Code's debugger list comes from the separate
  `pchambre.lh5801-asm` extension, not from ours.

## Where the code is

- **`core/CPU/`:**
  - `HistoryRing.hpp`: the per-CPU history, recorded in `step()`;
  - `WatchSet.hpp`: memory watches; `BreakpointSet.hpp`; `DebugStop.hpp`: the machines' stop latch;
  - breakpoint / skip-once / watch hooks in `LH5801` and `SC7852`.
- **`core/Debug/`:**
  - `Disasm/`;
  - `SharpDebugging` + `CpuViews`: the machine's `calcu::machine::Debugging`, one
    `SharpDebugTarget` per CPU (LH5801; SC7852 and LH5803) with the spaces `me0`,
    `me1`, `me1-written` / `memory`, `io`, `io-written`, the bank windows and the
    qualifier fields; `SharpMachine::advance` runs it while a check is on;
  - `CpuRegisters`: the register tables and the shared types (`Register`, `BankKey`, …);
  - `DebugExpression`;
  - `Listing/`: the `listingFormats()` table is the extension point for TASM or library listings later;
  - `SourceMap`;
  - `ProgramLoader`: Build & Load (planning in `core/MachineCodeFile`'s `planLoad()`).
- **`app/`:** `SharpDebuggerSetup` (the `DebuggerPart`'s set-up: `cpu` only, the
  trace session, no run-control steps on the Sharp models), `SharpTraceRecorder`,
  `SharpSymbolReaders` (sdas, zasm, `.SYMBOLS:` for `symbols`).
- **`app/dap/`** (C1600-3's layer, built in M-45; no Qt): `DapTargets` (threads,
  registers by name, banks, memory), `DapBreakpoints` (the client's breakpoints as
  the library's with the setter `dap` and a test: condition, hit condition, log
  message), `DapStepping` (`next`, `stepIn`, `stepOut`; a source-line step is one
  library step with a test).
- **`frontends/qt6/app/debug/`** (until M-50 moves them to `app/dap/`):
  - `DapServer`: transport;
  - `DapSession`: the requests;
  - `DebugController`: owned by `MachineController`; the session's source map and
    breakpoints, and the stops from the `run-state` topic.
- **Hooks elsewhere:**
  - `SharpAppPart`: the `DebuggerPart` among its parts; Reset and Cold boot tell the
    restart (`tellRestarted`, until LIB-18 1600);
  - `MachineController::typeCommand()` (the library's typing engine, `typeText` with ENTER);
  - `DebugController::runAndWait`: presets and resets from the debugger are the session's
    operations; a DAP request waits for the reply in a nested event loop, the host's timer
    (the shell's `SessionHost`) goes on; the busy topic drives the DAP queue (until M-50);
  - `main.cpp`: `--dap`, `macDisableWindowRestoration()`.
- **`vscode/calcu1600-debug/`:** the extension. Install with `tools/install_vscode_extension.sh`.

## Design rules

- **Arming:** the library's rule G1: breakpoints and watches are checked
  only while no operation runs (`Checks`). A boot, a preset or a load is an
  operation and never parks on a breakpoint; those loops would hang.
- **Queueing:** DAP messages wait while the app runs a synchronous
  operation (`MachineController::appBusyChanged`, from the app session's
  `busy` topic). An attach handled during the
  startup preset would rebuild the machine under the running loader.
- **Typing:** the GUI paste never presses ENTER; that's deliberate. Tools
  use `typeCommand()`, which does (on every model).
- **Build & Load order:** clean start (the configuration's `preset`, else
  the model's default preset, else All Reset), then a direct load (no
  dialog or popup), then auto-start. `cleanStart: false` skips the clean
  start.
- **Memory references** must stay numeric (VS Code can't parse `1:0000`);
  see `DapSession::memoryReference()`.
- **Scripted launches:** never SIGTERM the app; quit via `calcu1600/quit`
  and launch with `-ApplePersistenceIgnoreState YES`.
