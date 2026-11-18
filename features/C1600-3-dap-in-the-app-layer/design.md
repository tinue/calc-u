# C1600-3 DAP in the app layer: design
Status: approved by the user at the Part C2/D boundary (2026-11-16)

## 1. Scope

| Row | Step | Tests from (plan §3) | Done in (plan §5.1) |
|---|---|---|---|
| C1600-3 | M-50 (pieces in M-45, §1.1) | AC-1–25, EC-1–7 | M-50 |

- M-50 waits for M-45 (LIB-10 1600: the Sharp debug targets, the
  library's run control in the product, and the DAP's layer in `app/dap/`),
  M-46 (C1600-4's `debug:` block, the presets on LIB-6, LIB-18's Reset and
  Cold boot in the product), M-44 (C1600-2's loaders, for Build & Load) and
  M-49 (LIB-14 1600: the Debugger section of the settings dialog for AC-18;
  attach in the Qt6 frontend). The plan names only M-45 and M-46 (§10).
- The spec has AC-1 to AC-25 (AC-24 and AC-25 stand after AC-7) and EC-1
  to EC-7. All pass in M-50.
- Clauses that name other features: AC-17's `c1600/display-text` (was `c1600/screen`, R-4) gives the
  display as text by LIB-11 AC-19's rule. The product's reader
  (`core/Display/LcdText`) exists today and the harness's `display-text` uses it;
  the LIB-11 1600 column (M-51) only adds its tests. AC-19's `check-preset`
  is LIB-6 AC-51, which Calc-U-1600 has from M-46.
- Not here: the debug targets, the run control, breakpoints and watches
  as such, traces (LIB-10); placing a program in memory and the starting
  command (C1600-2); the keys of a `debug:` block and their check when a
  preset is read (C1600-4 AC-11); the INSPECTOR; a debugger in the Web
  frontend.

### 1.1 What M-45 builds of this row

LIB-10 §10 D-15: when `debug::RunControl` and `debug::BreakpointTable`
go in M-45, the DAP's own layer is written in `app/dap/`, without Qt,
with this design: `DapBreakpoints`, `DapStepping`, `DapTargets` (§3.6,
§3.7). M-45 ports their tests (AC-23's cases) and runs
`tools/dap_smoke.py` against the Qt app. M-50 adds the server, the
session, the set-up, the hosts and the remaining tests, and reports every
AC in the qa-report.

## 2. What the code is today (main, `60e5b09`)

- Everything of the server is in the Qt frontend:
  `frontends/qt6/app/debug/DapServer.*` (`QTcpServer`, one client, the
  second told and closed, `DapServer.cpp:17-46`); `DapSession.*` (the
  requests, `DapSession.cpp:84-131`; set-up `:146-176, 212-297, 343-478`;
  events `:552-582`; memory references `:608-643`); `DebugController.*`
  (the queue while busy, `:23-50`; the status, `:66-85`; Disconnect,
  `:87-120`; the session's target, run control and source map,
  `:122-187`; presets and clean starts through `Operations::runAndWait`,
  a nested event loop, `:189-235`; Build & Load, `:237-263`; the reset
  without the boot run, `:269-286`). The `--dap` option is read in
  `main.cpp:133-147`; a port outside 1024 to 65535 is dropped without a
  word (`DebugController.cpp:64`). The settings dialog's Debugger section:
  `SettingsDialog.cpp:336-375`.
- In the core: `core/Debug/AttachKeys.hpp` (the keys a configuration and
  a `debug:` block share), `ProgramLoader.*` (Build & Load),
  `SourceMap.*`, `Listing/` (sdas, zasm, `.SYMBOLS:`), `DebugExpression.*`,
  `Disasm/`.
- After M-45 (LIB-10 §10): `DebugTarget`, `RunControl` and
  `BreakpointTable` of the core are gone; the Qt `DapSession` reaches the
  machine through `app/dap/` and the `DebuggerPart` (adapter L-A1 of LIB-10
  §10.9).
- Checks: `tools/dap_smoke.py` (ten scenarios, `:112-631`) runs by hand,
  or as the Longer CTest `CalcU1600DapSmoke` against the Qt app
  (`CMakeLists.txt:175-180`). It skips the RENUM template scenario without
  `sdaslh5801` (`:367-373`).
- The VS Code extension `vscode/calcu1600-debug/` (`package.json`,
  `extension.js`, `scaffold.js`, four templates). The description of
  `program.after` gives `stopOnEntry` as the default
  (`package.json:228-234`); the server takes `none` (OQ-2).

## 3. Modules and seams

### 3.1 Files

`products/calc-u-1600/app/dap/`, namespace `shared::dap`, compiled into
the product's app library in every tree but Emscripten (AC-1: not in the
Web frontend). No file includes Qt (TR).

| File | Holds | Step |
|---|---|---|
| `DapTargets.hpp/.cpp` | threads (the debug targets in the machine's order, from 1), frames (0 now, 1 to 20 from the history), the scopes Registers, Flags, Banks, registers by name with the CPU's 8-bit halves and flags (`core/Debug/CpuRegisters`), numeric memory references, the `ExpressionContext` of `core/Debug/DebugExpression` over `machine::DebugTarget` | M-45 |
| `DapBreakpoints.hpp/.cpp` | the client's breakpoints by line, address, symbol and on memory as LIB-10 breakpoints and watches with the setter `dap` and a `HitTest`: condition, hit condition, log message (from `core/Debug/BreakpointTable.*`) | M-45 |
| `DapStepping.hpp/.cpp` | `next`, `stepIn`, `stepOut` with and without source lines, on the library's `step` with its test (LIB-10 §10 D-16) | M-45 |
| `DapJson.hpp/.cpp` | JSON text to and from `calcu::sessions::Value` (D-4) | M-50 |
| `DapWire.hpp/.cpp` | the DAP's framing: `Content-Length` headers over a byte stream, both ways | M-50 |
| `DapTransport.hpp` | what a host gives the server (§3.2) | M-50 |
| `DapConfig.hpp/.cpp` | the configuration of AC-5: the client's keys over a project's `debug:` block (C1600-4), checked by `core/Debug/AttachKeys.hpp`; EC-2's refusals | M-50 |
| `DapSetup.hpp/.cpp` | the set-up of AC-6, Build & Load of AC-7, AC-24, AC-25, as one operation of the session (§3.4) | M-50 |
| `DapSession.hpp/.cpp` | one DAP session: the requests of AC-11 and AC-17, the events of AC-16; from `frontends/qt6/app/debug/DapSession.cpp`, with `Value` for `QJsonObject` | M-50 |
| `DapPart.hpp/.cpp` | `shared::dap::DapPart : calcu::sessions::SessionPart`: the server, its status topic and command, the held messages (§3.3) | M-50 |

Elsewhere:

| File | Change | Step |
|---|---|---|
| `frontends/qt6/app/debug/DapServer.*`, `DapSession.*`, `DebugController.*` | deleted (LIB-10 §10, L-A1) | M-50 |
| `frontends/qt6/app/debug/DapSocket.hpp/.cpp` (new) | a `QTcpServer` on 127.0.0.1 as the `DapTransport` of the Qt6 frontend; posts what arrives to the session's thread (the GUI thread) | M-50 |
| `frontends/qt6/app/main.cpp` | `--dap <port>` is handed to the `DapPart` for the run; a port outside 1024 to 65535 is said on standard error and in a notice, and the server does not listen in that run (EC-1) | M-50 |
| `frontends/qt6/app/MachineController.*`, `MainWindow.cpp` | `debugController()` goes; the `DapPart` is among `SharpAppPart`'s parts; `c1600/quit` quits the application (AC-17) | M-50 |
| `frontends/qt6/app/SettingsDialog.cpp` (the Debugger section, a product section of the shell's dialog after M-49) | the status line from the `c1600.dap` topic and the button Disconnect, which sends `c1600.dap-disconnect` and can be used while a client is connected (AC-18, AC-21) | M-50 |
| `app/SharpAppPart.hpp/.cpp` | a `DapPart` among `sessionParts()`, with the references of §3.3; not in an Emscripten tree, so the Web session has none | M-50 |
| `app/HarnessProduct.cpp` | the same `DapPart` among the harness's parts; the serve add-on `--dap` (§3.5) | M-50 |
| `library/harness/ServeAddOn.hpp` (new), `Product.hpp`, `Run.cpp`, `cli/Cli.cpp` | a product's option of `serve`, its events in the serve loop, emulated time while it wants the machine to run (§3.5) | M-50 |
| `library/harness/attach/LocalStream.hpp/.cpp` (new), `AttachEndpoint.cpp` | the one-client listener on 127.0.0.1 of attach, for bytes instead of lines; `AttachEndpoint` keeps its lines on it. With `CALCU_ATTACH`, as attach (§3.5) | M-50 |
| `core/Debug/ProgramLoader.*`, `core/Machine/LegacyCalls.hpp/.cpp` | Build & Load places the bytes with C1600-2's machine-code loader; `LegacyCalls::loadProgram` goes; `ProgramLoader` keeps the entry and the range rules of AC-7 | M-50 |
| `vscode/calcu1600-debug/package.json` | the description and default of `program.after`: `none` (OQ-2). Nothing else in the extension changes | M-50 |
| `tools/dap_smoke.py` | `--harness <calcu>` starts `calcu --product c1600 serve --dap <port>` and quits it with `c1600/quit`; `--app` stays; the RENUM scenario uses the fixture ROM and no longer skips (AC-19) | M-50 |
| `tests/fixtures/dap/` (new) | the RENUM ROM built once from its template with `sdaslh5801` (research skill `pc1500-build`), with its listing; the four templates filled as `scaffold.js:49-58` fills them | M-50 |
| `tests/dap_layer_tests.cpp` (M-45), `tests/dap_tests.cpp` (new, M-50), `CMakeLists.txt` | §6 | M-45, M-50 |

### 3.2 What a host gives the server (AC-1, TR)

```cpp
namespace shared::dap {
// The host's side: a socket on this computer, and the way back onto the
// session's thread. The server calls these on the session's thread.
class DapTransport {
public:
    virtual ~DapTransport() = default;
    virtual std::optional<std::string> listen(std::uint16_t port) = 0;   // the reason it cannot
    virtual void close() = 0;                                             // stops listening, drops a client
    virtual void send(std::string bytes) = 0;                             // to the client
    virtual void dropClient(std::string lastWords) = 0;                   // AC-3, AC-10
    virtual void quitProgram() = 0;                                       // AC-17 c1600/quit
};
// The host calls these on the session's thread, in the order they came.
struct DapInput {
    enum class Kind { Connected, Bytes, Disconnected, SecondClient } kind;
    std::string bytes;
};
}
```

- The Qt6 host is `DapSocket` (`QTcpServer`, on the GUI thread, which is
  the session's thread). It closes a second connection itself after
  sending the sentence it is given (AC-3).
- The harness host is `LocalStream` with its I/O thread; what it reads is
  posted into the serve loop's inbox and handed over there (§3.5).
- The settings (`[debug] dap-enabled`, `dap-port`, LIB-9 AC-26, AC-28) are
  read from the product's settings in the session, by the `DapPart`.

### 3.3 The part (AC-2, AC-3, AC-8, AC-9, AC-10, AC-18, AC-21; EC-1, EC-5)

`DapPart` is constructed with references to the parts it uses: the
`DebuggerPart` (LIB-10), the `TypingPart` (LIB-5), the `PresetsPart`
(LIB-6), the startup part (LIB-18), the product's settings and C1600-2's
loader. It names no machine class (TR).

| Name | Shape | Content |
|---|---|---|
| `c1600.dap` | state topic | `{status, port, client}`: the status of AC-21, "Off", "Listening on 127.0.0.1:`<port>`", "Client connected on 127.0.0.1:`<port>`", "Port `<port>` not available: `<why>`" (today's texts, `DebugController.cpp:77-83`) |
| `c1600.dap-disconnect` | command | ends the DAP session as AC-10 says; refused "No debugger is attached." without a client |
| `c1600.dap-port` | command | `{port}` for one run (`--dap`), no setting changed; a port outside 1024 to 65535 refused with "The DAP port must be from 1024 to 65535." (EC-1) |

- **Listening.** At `attach` and whenever the settings change, the part
  listens when `dap-enabled` is on or a run port is set, on that port or
  `dap-port`; otherwise it closes (AC-2).
- **A second client** is told "Calc-U-1600 already has a debugger
  attached…" and closed (AC-3; today's sentence, `DapServer.cpp:31-46`).
- **Held messages (AC-8).** A whole message that arrives while an
  operation runs (`ctx.busy()`) waits in a queue. At `operationEnded`,
  the queue is answered in the order of arrival, until a message starts an
  operation again. The server's own operations (§3.4) count too.
- **Ending (AC-10, EC-5).** `disconnect`, a closed socket or
  `c1600.dap-disconnect`: a request being carried out is finished first;
  then the setter `dap`'s breakpoints and watches go (LIB-10 AC-47) and a
  paused machine is resumed. After Disconnect in the dialog the client
  gets `terminated` first (AC-16).
- **A new machine (AC-9).** On `machineChanged`, the library has set the
  breakpoints again on the targets of the same name (LIB-10 AC-15). The
  part checks the bindings against the new memory, sends `invalidated`
  and the line "The machine was rebuilt; breakpoints re-applied.".

### 3.4 The set-up (AC-4 to AC-7, AC-24, AC-25; EC-2 to EC-4)

`attach` (and `launch`, the same) is answered when the set-up is done.
The set-up is one operation of the session (`DapSetup`, a
`calcu::sessions::Task`), so the messages that come meanwhile wait
(AC-8), and the rules of operations hold inside it: no breakpoint or watch
pauses it (LIB-10 AC-13), and a reset it makes is over when the machine
stands at the reset vector (AC-24). Today the same is done in a nested
event loop of the Qt frontend (`DebugController.cpp:193-235`).

| # | Step | AC |
|---|---|---|
| S0 | `initialize`: `pause cpu`, the machine stands between two instructions; `initialized` follows the reply | AC-4 |
| S1 | The configuration: the client's keys over the `debug:` block of `project` (C1600-4 AC-11); `program` merged key by key, `listings` and `symbols` added; `project` is the `preset` unless the launch configuration gives one. Unknown keys are passed over; `slot` in `program` is refused with a hint; numbers by LIB-6 AC-1; hexadecimal without a mark refused; `Bad "<key>": <what>` (EC-2) | AC-5 |
| S2 | `boot: debug` with `program` or `command`: refused | AC-6 |
| S3 | `preset` without `program`: the preset is loaded (LIB-6), and its load runs to its end; with `boot: debug` it is loaded to build only (LIB-6 AC-55) after `pause-at-start` (LIB-10 AC-46), so the new machine stands before its first instruction | AC-6 |
| S4 | `reset: reset` or `allReset`: `pause-at-start`, then the machine's Reset or Cold boot without the run to key readiness, told to the parts (`tellRestarted`); the machine stands at the reset vector | AC-6, AC-24 |
| S5 | `listings`, `symbols`: bound to the thread `cpu` names (the first without it) with `bank`, `me`, `pu`, `pv` as the qualifier; a binding whose bytes differ takes no part and the client is told how many lines differ (AC-13, EC-7) | AC-6 |
| S6 | `program`: Build & Load (S7). Else a `command` is given to the typing engine with ENTER (LIB-5, not a user key) and typed when the machine runs | AC-6 |
| S7 | Build & Load: with `cleanStart` (on unless switched off) the machine is built anew: from `preset`, else the model's startup preset (refused when it is for another model, EC-4), else a Cold boot to key readiness. `bin` is placed by C1600-2's machine-code loader with the file's length and the address given by hand (C1600-2 AC-8, AC-23): `address`, else for headerless code the lowest address of a listing line with bytes; without either refused as C1600-2 refuses it, with the hint that a listing would do too (EC-3). The entry: `entry` (a number or a listing symbol), else the header's start address, else the listing's entry inside the code, else the load address. The listing (with `source` as its main file where it cannot be told) and the symbols are bound over the bytes loaded. Then `after`: `none` (default) nothing; `call` types the starting command (C1600-2 AC-11) or `command`; `stopOnEntry` the same after a breakpoint of the setter `dap` at the entry, which the part takes away when it is hit and tells as `stopped` with the reason entry | AC-7 |
| S8 | After each of S3 to S7 that built the machine anew or let it run: `pause cpu` again. During the set-up nothing is told as `stopped` (AC-16) | AC-4 |
| S9 | `configurationDone`: `resume`, unless `stopOnEntry` (attach's) is set: then the machine stays paused and the client is told `stopped` with the reason entry. A program started with `after: call` in the set-up runs only from here (ledger 1600-20) | AC-4, AC-7 |

- **`restart`** is S1 to S8 with the configuration it brings,
  `reset: none` taken as `reset`, then S9's resume (AC-11).
- **`c1600/load`** is S7, of the `program` it brings or of the
  configuration's, answered with the range, the entry and the starting
  command (AC-17). Without a `program` in the configuration (a ROM
  project) it is S1 to S8 again, then S9 (AC-25).
- **`c1600/reset`** is S4 (`kind: allReset` a Cold boot), standing
  before the first instruction unless `stop` is false, in which case the
  machine resumes (AC-17).

### 3.5 Hosting under the harness (AC-1, AC-22)

```cpp
namespace calcu::harness {
// A product's option of `serve` (C1600-3 AC-22): it takes its value at the
// start, then events from its own I/O thread through the loop's inbox.
class ServeAddOn {
public:
    virtual ~ServeAddOn() = default;
    virtual std::string_view option() const = 0;                      // "--dap"
    virtual std::string_view help() const = 0;
    virtual std::optional<std::string> start(std::string_view value, sessions::AppSession&,
                                             std::function<void(std::string)> post) = 0;   // the reason it cannot
    virtual void take(std::string event, sessions::AppSession&) = 0;  // on the loop's thread
    virtual bool wantsTime() const = 0;                               // the machine should run now
    virtual void stop() = 0;
};
// HarnessProduct: virtual std::vector<ServeAddOn*> serveAddOns() { return {}; }
}
```

- `calcu --product c1600 serve --dap <port>`: `Cli.cpp` hands the value
  to the product's add-on. An add-on that cannot start ends the run with
  exit status 2 and the reason (EC-1; LIB-11's "cannot start").
- **The loop.** With an add-on started, standard input is read by a
  thread of its own; its lines and the add-on's events go into one inbox,
  taken in order on the loop's thread. While the inbox is empty and the
  add-on wants time (a DAP session has the machine running), the loop
  advances the session unpaced, in pieces of 1/60 s of emulated time, and
  looks at the inbox between pieces. There is no wall clock (LIB-11 AC-6).
  A scenario that pauses by a breakpoint repeats exactly; one that pauses
  by hand does not, and `--dap`'s help says so (OQ-1).
- **The socket.** `LocalStream` is attach's one-client listener on
  127.0.0.1 (`AttachEndpoint`), for bytes. It is compiled where attach is
  (`CALCU_ATTACH`: macOS and Linux trees with the tests). Elsewhere `--dap`
  is refused "The DAP server is not in this build." (D-3).
- `c1600/quit` ends the DAP session and the run (exit status 0).

### 3.6 Requests and events (AC-11, AC-12, AC-16)

| Requests | Through |
|---|---|
| `threads` | `machine::Debugging`: one thread per target, `LH5801`; `SC7852`, `LH5803` |
| `stackTrace`, `scopes`, `variables`, `setVariable` | frame 0 is now; frames 1 to 20 the history, read-only (EC-6: `setVariable` on them refused). Registers: `RegisterInfo` and `readRegister`/`writeRegister`. Flags: the bits of the status register by `flagNames` (`core/Debug/CpuRegisters`). Banks: `qualifierNow()`; on the SC7852 the bank of each of the four pages, found by `qualifierMatches` at each page's start (LIB-10 §10 D-20; today `PC1600DebugTarget::bankState`) |
| `readMemory`, `writeMemory`, `disassemble`, `evaluate` | `peek`, `poke` (refused cells as LIB-10 AC-2), `core/Debug/Disasm` over `peek` with the bound symbols, AC-15's expressions. A memory reference is a number: the address in its low 16 bits, the thread above them when it is not 1, and `0x100000` for ME1, as today (`DapSession.cpp:608-610`) |
| `continue`, `pause` | `resume`; `pause cpu` (LIB-10 AC-6) |
| `next`, `stepIn`, `stepOut` | `DapStepping`: LIB-10 `step` over, in, out of the named thread; with a source line under it and no `granularity: instruction`, the step goes on until the line changes or code without source is reached (LIB-10 §10 D-16) |
| `setBreakpoints`, `setFunctionBreakpoints`, `setInstructionBreakpoints`, `dataBreakpointInfo`, `setDataBreakpoints` | `DapBreakpoints` (§3.7) |
| `setExceptionBreakpoints` | answered with nothing |
| `restart`, `c1600/load`, `c1600/reset` | §3.4 |
| `c1600/display-text` | the display as text as the harness's `display-text` reads it (`displayTextNow`, `app/SharpDisplayLines`), in its JSON form (`displayTextValue`), also without a DAP session |
| `c1600/quit` | §3.5, `DapTransport::quitProgram` |

Any other request: "Unsupported request '<name>'" (today's text).

Events (AC-16): the part compares the `run-state` topic after each host
call. A change to paused, outside the set-up and inside no line step,
gives one `stopped`: `threadId` of the current target, `allThreadsStopped:
true`, `hitBreakpointIds` the DAP IDs of the hits whose setter is `dap`,
and the reason: `pause` (Pause and PauseOnStart), `step`, `breakpoint`,
`data breakpoint` (Watch), `entry`. Further events as today:
`initialized`, `output`, `breakpoint` when a breakpoint is resolved anew,
`invalidated`, `memory` after a write, `terminated` after Disconnect in
the dialog. No `continued` (OQ-3).

### 3.7 Breakpoints, listings, expressions (AC-13 to AC-15; built in M-45)

- **Kinds.** A line, an address or a symbol's name gives a LIB-10
  breakpoint with the setter `dap` and the binding's qualifier; a memory
  breakpoint gives a watch (`dataBreakpointInfo` names the range and the
  access). IDs: the DAP's own, mapped to the library's IDs.
- **Tests** (LIB-10 AC-12): the condition, the hit condition (`N`, or
  `==`, `!=`, `>=`, `<=`, `>`, `<`, `%` with `N`, compared with the hits;
  one that cannot be read counts as met) and the log message with
  `{expression}` parts. A log message writes its line as `output` and does
  not pause, also on a memory breakpoint (OQ-4). A condition that cannot be
  worked out pauses and says why.
- **Banks.** A breakpoint in a bank that is not switched in is passed by
  `qualifierMatches`; its status says "Only while bank 3" (or the PU, PV,
  ME it needs).
- **Listings** (`core/Debug/Listing/`, `SourceMap`, unchanged): sdas
  `.lst`/`.rst`, zasm `.lst`, a `.SYMBOLS:` table; a loaded program's
  listing over the bytes loaded, any other over the thread's whole space;
  a line breakpoint goes to the first line with code at or after it.
- **Expressions** (`core/Debug/DebugExpression`, unchanged): numbers in
  decimal or after `0x`, `$`, `&`; registers and flags, then symbols;
  `[a]`, `w[a]` in the processor's byte order, `#[a]` for ME1; C's
  operators and their order.

## 4. Product side and frontends

- **Qt6** (AC-1, AC-2, AC-18, AC-21; EC-1). The `DapPart` is one of
  `SharpAppPart`'s parts; `DapSocket` is its transport. The settings
  dialog shows the status line and Disconnect beside the two settings.
  `main.cpp` keeps the option `--dap <port>` and its help text.
- **Harness** (AC-22). §3.5.
- **Web.** No `DapPart`: `SharpWebPart`'s session has none (AC-1).
- **VS Code extension** (AC-19). Unchanged but for `program.after`'s
  description. Its presets and templates are C1600-4's (M-46). The test
  suite fills each of the four templates as `scaffold.js:49-58` does and
  checks the result with `check-preset`; the RENUM scenario uses the ROM
  of `tests/fixtures/dap/`.

## 5. Adapters

Removed in M-50:

| Adapter | Where | Since |
|---|---|---|
| L-A1 Qt DAP: `DapServer`, `DapSession` in `QJsonObject`, `DebugController`'s queue | `frontends/qt6/app/debug/` | LIB-10 §10.9, M-45 |
| `LegacyCalls::loadProgram` | `core/Machine/LegacyCalls.*` | C1600-1 §5 |

M-50 adds no adapter. `tools/dap_smoke.py` stays a script (it is a test
client, not one of today's tools that LIB-11 replaces).

## 6. Tasks (AC → task → test)

Cases in `products/calc-u-1600/tests/` are in `CoreTests` (Frequent).
`dap_tests.cpp` drives a `DapPart` through a test transport (bytes in and
out, on the test's thread) in a session of `app/tests/SessionFixture.hpp`
in virtual time. Names are `c1600_3_acN_…`, `c1600_3_ecN_…`. Tests come
before the code of each task.

| Task | What | AC/EC | Test (file: cases; group) |
|---|---|---|---|
| T1 (M-45) | Today's tests on the new layer | AC-23 | `dap_layer_tests.cpp`: `c1600_3_ac23_line_breakpoint_and_line_steps`, `_conditions_hit_counts_and_logpoints`, `_bank_qualified_listing`, `_hit_condition_parsing` (`run_control_tests.cpp:115-150, 171-211, 236-290`), `_expressions`, `_compiled_expressions_match_legacy`, `_hit_conditions_and_log_templates_match_legacy`, `_function_breakpoint_on_lh5803_symbol` (`debug_target_tests.cpp:84, 637-732`); `listing_tests.cpp` unchanged (`c1600_3_ac23_` prefix added); Frequent |
| T2 (M-45) | Line steps, breakpoints | AC-12, AC-14 | `dap_layer_tests.cpp`: `c1600_3_ac12_step_goes_on_until_the_line_changes`, `c1600_3_ac12_instruction_granularity_one_instruction`, `c1600_3_ac14_breakpoints_belong_to_dap`, `c1600_3_ac14_log_message_writes_and_does_not_pause`, `c1600_3_ac14_unworkable_condition_pauses_and_says_why`, `c1600_3_ac14_other_bank_passed_status_only_while`; Frequent |
| T3 | JSON, framing | (support) | `dap_tests.cpp`: `dap_json_round_trip`, `dap_wire_frames_across_reads`; Frequent |
| T4 | Listening, second client, status | AC-2, AC-3, AC-21, EC-1 | `c1600_3_ac2_listens_on_127_0_0_1_by_setting_or_run_port`, `c1600_3_ac2_port_taken_status_says_so`, `c1600_3_ac3_second_client_told_and_closed`, `c1600_3_ac21_status_texts_on_the_topic`, `c1600_3_ec1_port_out_of_range_not_listening`; Frequent |
| T5 | Session start | AC-4, AC-5 | `c1600_3_ac4_initialize_pauses_configuration_done_resumes`, `c1600_3_ac4_stop_on_entry_stays_paused_entry`, `c1600_3_ac5_project_block_is_the_base_keys_win`, `c1600_3_ac5_program_merged_listings_added`, `c1600_3_ac5_unknown_key_passed_over_slot_refused`, `c1600_3_ac5_numbers_by_preset_rule_bare_hex_refused`, `c1600_3_ec2_bad_key_refused_with_reason`; Frequent |
| T6 | Set-up | AC-6, AC-24 | `c1600_3_ac6_boot_debug_with_program_refused`, `c1600_3_ac6_preset_loaded_then_paused`, `c1600_3_ac6_boot_debug_builds_only_paused_before_first_instruction`, `c1600_3_ac6_reset_stops_at_the_reset_vector`, `c1600_3_ac6_listings_bound_to_their_cpu_with_qualifier`, `c1600_3_ac6_command_typed_when_running`, `c1600_3_ac24_breakpoints_work_after_the_setup`, `c1600_3_ac24_reset_over_when_it_stops_at_the_vector`; Frequent |
| T7 | Build & Load | AC-7, AC-25, EC-3, EC-4 | `c1600_3_ac7_clean_start_from_preset_startup_preset_or_cold_boot`, `c1600_3_ac7_length_and_address_by_hand`, `c1600_3_ac7_headerless_lowest_listing_address_not_an_equate`, `c1600_3_ac7_entry_rules`, `c1600_3_ac7_after_none_call_stop_on_entry`, `c1600_3_ac7_call_in_setup_waits_for_configuration_done` (ledger 1600-20), `c1600_3_ac25_rom_project_load_redoes_the_setup`, `c1600_3_ec3_headerless_without_address_refused_with_hint`, `c1600_3_ec4_startup_preset_for_another_model_refused`; Frequent |
| T8 | Held messages, ending | AC-8, AC-10, EC-5 | `c1600_3_ac8_messages_held_during_an_operation_answered_in_order`, `c1600_3_ac10_disconnect_closed_socket_and_dialog_end_the_session`, `c1600_3_ac10_breakpoints_gone_machine_resumed`, `c1600_3_ec5_request_finished_before_the_end`; Frequent |
| T9 | A new machine | AC-9 | `c1600_3_ac9_model_switch_keeps_the_session_invalidated_and_line`; Frequent |
| T10 | Requests | AC-11, AC-13, AC-15, EC-6, EC-7 | `c1600_3_ac11_threads_frames_scopes_variables`, `c1600_3_ac11_banks_scope_per_page_on_the_sc7852`, `c1600_3_ac11_memory_references_numeric`, `c1600_3_ac11_read_write_memory_disassemble_evaluate`, `c1600_3_ac11_unsupported_request`, `c1600_3_ac13_binding_bytes_differ_told_how_many`, `c1600_3_ac15_expression_error_names_what`, `c1600_3_ec6_set_variable_of_history_refused`, `c1600_3_ec7_listing_lines_not_matching_bound_and_told`; Frequent |
| T11 | Events | AC-16 | `c1600_3_ac16_stopped_once_with_reason_ids_all_threads`, `c1600_3_ac16_pause_from_harness_told`, `c1600_3_ac16_line_step_and_setup_pauses_not_told`, `c1600_3_ac16_memory_breakpoint_invalidated_terminated_events`; Frequent |
| T12 | Own requests | AC-17 | `c1600_3_ac17_load_answers_range_entry_command`, `c1600_3_ac17_reset_and_all_reset_stop_unless_stop_false`, `c1600_3_ac17_screen_without_a_session`, `c1600_3_ac17_quit_ends_session_and_program`; Frequent |
| T13 | The harness | AC-1, AC-22 | `library/harness/tests/serve_addon_tests.cpp`: `serve_addon_events_and_time_while_wanted`, `serve_addon_cannot_start_exit_2` (`calcu_library_tests`, Frequent); `harness_tests.cpp`: `c1600_3_ac22_client_attaches_without_a_window`, `c1600_3_ac22_running_machine_passes_emulated_time`; Frequent |
| T14 | The ten scenarios | AC-20, AC-19 | CTest `CalcU1600DapSmoke`: `tools/dap_smoke.py --harness $<TARGET_FILE:calcu>`, the ten scenarios (`main`, `program_run`, `pc1600_run`, `project_run`, `bus_rom_run`, `renum_template_run`, `rom_run`, `reset_run`, `lh5803_run`, `display_text_run`), each named `c1600_3_ac20_<scenario>` in its output; group by the budget (D-5). `c1600_3_ac19_templates_filled_and_checked` (the four templates as `scaffold.js` fills them, `check-preset`); Frequent |
| T15 | Qt6 | AC-1, AC-18 | CTest `CalcU1600DapSmokeQt` (`--app`, every scenario against the Qt app; Longer); `CalcU1600Shots.c1600-3-ac18` (the Debugger section: status line, Disconnect usable only with a client; Longer) |

Coverage: AC-1 to AC-25, 25 of 25; EC-1 to EC-7, 7 of 7.

## 7. Tools and agent knowledge (M-50)

- `tools/dap_smoke.py` gains `--harness`; `--app` stays for the Qt check.
  `CalcU1600DapSmoke` moves to the harness, `CalcU1600DapSmokeQt` is new.
- `products/calc-u-1600/CLAUDE.md`, section Harness: `serve --dap <port>`
  (a client attaches without a window; emulated time passes while the
  client has the machine running); section "Core, Shared, frontends": the
  DAP server is `app/dap/` on every native host.
- `products/calc-u-1600/docs/developer/Debugger-Internals.md`: "Where the
  code is" (`app/dap/`, `frontends/qt6/app/debug/DapSocket`), "Hooks
  elsewhere", "Queueing" (the server's own rule, by the session's busy
  state, no nested event loop), the harness host; `docs/Debugger.md`
  (`program.after` defaults to `none`; `calcu … serve --dap`);
  `docs/developer/Building.md:266` (`dap_smoke.py --harness`);
  `docs/developer/Shared-Architecture.md` (the `app/dap/` row).
  `docs/background/plans/DAP-Debugger-Plan.md` stays history.
- `docs/glossary-mapping.md`: rows `DapServer`, `DapSession`,
  `DebugController` → server, DAP session (`shared::dap::DapPart`,
  `DapSession`).
- `docs/test-groups.md`: `dap_tests.cpp`, `serve_addon_tests.cpp`,
  `CalcU1600DapSmoke` (harness), `CalcU1600DapSmokeQt`,
  `CalcU1600Shots.c1600-3-ac18`.
- Memory (`calc-u-1600/`, with their lines in `calc-u-1600-memories.md`):
  `project_dap_debugger_plan.md` (the server is in `app/dap/`, hosted by
  the Qt6 frontend and the harness; `SyncOperations`/`Operations`' nested
  event loop is gone); `feedback_gui_test_launch_quit.md` (the scripted
  checks run against `calcu serve --dap`, no app to launch; the rule
  stays for `--app`); `project_shared_layer_wasm.md` ("DAP move open" is
  done). `feedback_lcd_text_before_images.md` keeps
  `c1600/screen` (`c1600/display-text` since R-4).
- No skill: Calc-U-1600 has none.

## 8. Golden runs, reference runs and risks

- **Golden and reference runs.** M-50 touches no core but
  `ProgramLoader`'s placing, which C1600-2's loader takes over. The golden
  runs replay unchanged; the 32 examples (M-4) are not run through the
  DAP. `dap_smoke.py` at the pin is the reference of the scenarios: each
  passed in the Part B run (`docs/test-groups.md`).
- **The open `stepIn` finding.** `docs/test-groups.md` records that
  `CalcU1600DapSmoke` failed three times on 2026-11-14 (no `stopped` after
  the first `stepIn`), possibly by timing. Under the harness the run is in
  emulated time; if the scenario still fails there it is a finding of the
  server, found at last.
- **Operations instead of a nested event loop.** Today a DAP request that
  loads a preset waits in a nested event loop while the frame timer goes
  on. In M-50 it is the session's operation, and the request is answered
  at its end. A request that relied on the machine running meanwhile (a
  typed command) still gets it, since an operation runs the machine.
- **The serve loop's threads.** The harness's serve loop gets a reader
  thread for standard input only when an add-on is started; without
  `--dap` it reads as today, so LIB-11's tests are untouched.
- **Determinism.** A scenario that pauses by hand (`continue` and a
  `pause` after 0.3 s, `dap_smoke.py`) lands at a place that depends on the
  host's speed. The checks only ask that it pauses (OQ-1).

## 9. Decided (best guess)

- **D-1 (the set-up is an operation).** The set-up, Build & Load,
  `restart` and the own `load` and `reset` requests are each one operation
  of the session. So AC-8's hold, EC-5 and AC-24 follow from the session's
  rules, and no host needs a nested event loop.
- **D-2 (the DAP's reset is bare).** S4 resets or cold boots the machine
  without LIB-18's run to key readiness, as today's `resetMachine`
  (`DebugController.cpp:269-286`). LIB-18's Reset waits for key readiness,
  which a machine standing at its reset vector never reaches (LIB-10's
  qa-report: an operation on a paused machine does not finish); AC-24 says
  such a reset is over when it stops.
- **D-3 (the harness's socket).** The harness listens with attach's
  POSIX listener, built where attach is. A Windows harness has no `--dap`.
  The Qt6 app keeps `QTcpServer` on every platform.
- **D-4 (JSON).** A small JSON reader and writer over
  `calcu::sessions::Value` in `app/dap/`; the DAP is its only user. If a
  second one appears, it moves to `library/formats/`.
- **D-5 (the group of the ten scenarios).** Frequent if
  `CalcU1600DapSmoke` stays under 10 s and Calc-U-1600's Frequent under
  60 s; Longer otherwise. The qa-report gives the time.
- **D-6 (EC-1 in each host).** Qt6: a sentence on standard error and a
  notice, the server off for the run. Harness: exit status 2 with the
  reason.
- **D-7 (reasons of `stopped`).** A pause-on-start is told as `pause`
  (DAP has no own reason for it); entry is `entry`.

## 10. Plan notes

- Plan M-50, "Waits for": add M-44 (Build & Load on C1600-2's loader) and
  M-49 (the settings dialog's Debugger section, AC-18). The order of steps
  already holds both.
- Plan M-50, "Does": add "`calcu serve --dap` in the harness (a library
  serve add-on)". "Knowledge, ledger": add the product's `CLAUDE.md`,
  `docs/Debugger.md`, `Building.md`, `Shared-Architecture.md` and the three
  memory files of §7.
- Plan M-45 (with LIB-10 §10.16): "Tests from" adds C1600-3 AC-12, AC-14,
  AC-23 (their cases in `dap_layer_tests.cpp`, T1, T2).
- Plan M-51: `LegacyCalls::loadProgram` already goes in M-50.

## 11. Ledger rows

- 1600-20 becomes visible in M-50.
- Proposed addition to 1600-20: "`--dap` with a port outside 1024 to
  65535 is reported, and the server does not listen in that run (today it
  is passed over without a word). The harness accepts a debugger too
  (`calcu --product c1600 serve --dap <port>`)."

## 12. Questions for the user

None that split this design. The one question of LIB-10 §10.17 (Q-1)
decides where §1.1's pieces are built.
