# Debugger — internals

How the DAP debugger is built, and the rules its code relies on. The user
manual is [help/07-debugger.md](../../help/07-debugger.md); the DAP protocol as
the server speaks it is [DAP.md](DAP.md).

## Behaviour not in the manual

- **Transport:** the server listens on 127.0.0.1 only and takes one client
  at a time; a second one is told the debugger is busy. `--dap <port>`
  overrides Settings for one run; a port outside 1024 to 65535 is said
  (standard error and a notice; the harness ends with exit status 2) and
  the server stays off for that run.
- **Hosts:** the server is the app layer's `DapPart`; the Qt6 frontend
  and the harness (`calcu --product c1600 serve --dap <port>`) host it, the
  Web frontend does not. Under the harness emulated time passes, unpaced,
  while the DAP session has the machine running (no wall clock): a
  scenario that pauses by a breakpoint repeats exactly, one that pauses by
  hand does not.
- **Run control:** the library's (LIB-10, `library/debugger/`): the
  `DebuggerPart` among `SharpAppPart`'s parts is the machine session's gate
  whether or not a client is attached. Pause, step and resume come from
  the DAP, the harness (`calcu --product c1600`), Machine ▸ Resume and the
  session commands; the window's title says "Paused (debugger)" whoever
  paused it. A DAP `stopped` comes from the run control's state after the
  host call the pause landed in, or after the request that paused it
  (`DapSession::poll()`); the pauses of the set-up are not told.
- **The set-up waits for `configurationDone`:** `attach` leaves the machine
  paused; a program started with `after: call` (or `stopOnEntry`) in the
  set-up runs only from `configurationDone`, so the client's breakpoints
  are set first (ledger 1600-20).
- **Traces:** the library's trace session (`trace` command, the
  INSPECTOR's TRACE button as `trace {button: true}`, a preset's
  `trace:` step through LIB-6's preset engine) writes the core's
  records (`c1600.trace-records`, `app/SharpTraceRecorder`) into the trace
  folder, limits every trace to `[trace] max-file-size-mb` MiB and says so
  in a notice. The Web has no trace folder: a trace fails there.
- **`launch` is `attach`:** `cu1600::dap::DapSession` treats a `launch` request as
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
  - `ProgramLoader`: Build & Load's rules (the address, the entry, the range a listing
    binds to); it places the bytes with C1600-2's machine-code loader (`core/Loaders`).
- **`app/`:** `SharpDebuggerSetup` (the `DebuggerPart`'s set-up: `cpu` only, the
  trace session, no run-control steps on the Sharp models), `SharpTraceRecorder`,
  `SharpSymbolReaders` (sdas, zasm, `.SYMBOLS:` for `symbols`).
- **`app/dap/`** (C1600-3, namespace `cu1600::dap`; no Qt, every native tree, not
  Emscripten):
  - `DapPart`: the server, a session part after the `DebuggerPart`: listening by
    `[debug] dap-enabled`/`dap-port` or the run's port (`c1600.dap-port`), the status
    topic `c1600.dap`, the command `c1600.dap-disconnect`, the held messages, one
    `DapSession` per client;
  - `DapSession`: the requests and events; `DapSetup.cpp`: `attach`/`launch`,
    `restart`, `c1600/load`, `c1600/reset` as operations of the session;
  - `DapConfig` (a configuration over a project's `debug:` block, checked by
    `core/Debug/AttachKeys`), `DapJson` (JSON and `calcu::sessions::Value`), `DapWire`
    (the `Content-Length` framing), `DapTransport` (what a host gives);
  - `DapTargets` (threads, registers by name, banks, memory), `DapBreakpoints` (the
    client's breakpoints as the library's with the setter `dap` and a test:
    condition, hit condition, log message), `DapStepping` (`next`, `stepIn`,
    `stepOut`; a source-line step is one library step with a test).
- **Hosts:**
  - `frontends/qt6/app/debug/DapSocket`: a `QTcpServer` on 127.0.0.1, the Qt6
    frontend's `DapTransport`; `MachineController` makes it and hands it to the part;
    `main.cpp` sends `--dap` as `c1600.dap-port`; Settings ▸ Debugger shows the topic
    and sends `c1600.dap-disconnect` (`MainWindow::refreshSerialStatus`);
  - `app/HarnessProduct.cpp`: the same part among the harness's parts and the serve
    add-on `--dap` (`library/harness/ServeAddOn.hpp`) on attach's one-client listener
    `library/harness/attach/LocalStream` (built where attach is, `CALCU_ATTACH`).
- **Hooks elsewhere:**
  - `SharpAppPart`: the `DebuggerPart` and the `DapPart` among its parts;
  - the library's typing engine (`typeText` with ENTER) types a configuration's `command`
    and the program's starting command;
  - `PresetsPart::loadWithin`: a preset loaded inside the set-up's operation;
  - `main.cpp`: `macDisableWindowRestoration()`.
- **`vscode/calcu1600-debug/`:** the extension. Install with `tools/install_vscode_extension.sh`.

## Design rules

- **Arming:** the library's rule G1: breakpoints and watches are checked
  only while no operation runs (`Checks`). A boot, a preset or a load is an
  operation and never parks on a breakpoint; those loops would hang.
- **Queueing:** the server's own rule (AC-8): a message that arrives while
  an operation of the session runs (any: the startup preset, a menu's
  load, the DAP's own set-up) waits, and the held ones are answered in
  their order when it ends (`DapPart::operationEnded`). No nested event
  loop: a request that loads a preset is answered at the end of its
  operation. An attach handled during the startup preset would rebuild the
  machine under the running loader.
- **Typing:** the GUI paste never presses ENTER; that's deliberate. The DAP
  types with ENTER (the engine's `typeText`, on every model).
- **Build & Load order:** clean start (the configuration's `preset`, else
  the model's default preset, else All Reset), then a direct load (no
  dialog or popup), then auto-start. `cleanStart: false` skips the clean
  start.
- **Memory references** must stay numeric (VS Code can't parse `1:0000`);
  see `cu1600::dap::memoryReference()`.
- **Scripted launches:** never SIGTERM the app; quit via `c1600/quit`
  and launch with `-ApplePersistenceIgnoreState YES`. The scenarios run
  against the harness without an app (`tools/dap_smoke.py --harness`,
  CTest `CalcU1600DapSmoke`); `--app` is for the Qt check
  (`CalcU1600DapSmokeQt`).
