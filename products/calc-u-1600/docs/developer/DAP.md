# The DAP server

Calc-U-1600's debug server for VS Code (and other clients of the Debug
Adapter Protocol): the hosting, the set-up of a session, the requests and
events, and the syntax of conditions, hit conditions, log messages and
expressions. How to use it from VS Code, the launch configuration and the
project preset's `debug:` block: [help chapter 7](../../help/07-debugger.md).
How the code is built: [Debugger-Internals.md](Debugger-Internals.md). The
common harness (replies, JSON, options, exit codes, transports):
[docs/formats/harness.md](../../../../docs/formats/harness.md).

## Hosting

The server is the app layer's `DapPart` (`app/dap/`, namespace
`cu1600::dap`), in the app session of every native host; the Web frontend
has none. It listens on 127.0.0.1 only and takes one client at a time.

| Host | How it listens |
|---|---|
| Qt6 app | `[debug] dap-enabled` (default `false`) on `[debug] dap-port` (1024 to 65535, default 32168), or `--dap <port>` for one run (the settings unchanged) |
| Harness | `calcu --product c1600 serve --dap <port>` |

| Case | Result |
|---|---|
| `--dap` with a port outside 1024 to 65535, or not a number | "The DAP port must be from 1024 to 65535."; the server stays off for the run. Qt6: on standard error as `--dap <port>: <reason>` and as a notice, the app runs on. Harness: on standard error, exit status 2 |
| The port cannot be opened | Status "Port `<port>` not available: `<why>`"; the harness ends with that text and exit status 2 |
| `--dap` in a harness build without attach (`CALCU_ATTACH`: built only in the macOS and Linux trees with the tests) | "The DAP server is not in this build.", exit status 2 |
| `--dap` without `serve` | "--dap is an option of serve.", exit status 2 |
| A second client | gets one `output` event (category `important`): "Calc-U-1600 already has a debugger attached. Stop that debug session (Shift+F5), or use Settings > Debugger > Disconnect in the app." and is closed |
| A rebind (settings or run port changed) | drops the connected client; its session ends |

Under the harness, emulated time passes, unpaced, in pieces of 1/60 s
while the DAP session has the machine running; standard input and the
client's messages are taken in the order they arrive. A scenario that
pauses by a breakpoint repeats exactly; one that pauses by hand does not.
`c1600/quit` ends the run with exit status 0.

### Session names

| Name | Kind | Content |
|---|---|---|
| `c1600.dap` | state topic | `{status, port, client}`; `status` is "Off", "Listening on 127.0.0.1:`<port>`", "Client connected on 127.0.0.1:`<port>`" or "Port `<port>` not available: `<why>`" |
| `c1600.dap-disconnect` | command | ends the DAP session (Settings ▸ Debugger ▸ Disconnect); refused "No debugger is attached." without a client |
| `c1600.dap-port` | command | `{port}`: the port for this run, no setting changed; refused "The DAP port must be from 1024 to 65535." |

### Wire

Messages are JSON with `Content-Length: <n>\r\n\r\n` headers, both ways.
A message that arrives while an operation of the session runs (a preset
load, a menu's load, the server's own set-up) is held; the held messages
are answered in their order when the operation ends.

## Session set-up

`attach` and `launch` are the same request. Each is answered when its
set-up is done; the set-up is one operation of the session, so no
breakpoint or watch pauses it and nothing in it is told as `stopped`.

| Step | Request | What happens |
|---|---|---|
| 1 | `initialize` | The machine is paused between two instructions; the reply carries the capabilities (below); `initialized` follows |
| 2 | `attach` / `launch` | The configuration: the client's keys over the `debug:` block of the preset `project` names (the client's keys win; `program` merged key by key; `listings` and `symbols` added to the block's; `project` is the `preset` unless one is given). Keys the server does not know are passed over |
| 3 | | `boot: debug` with `program` or `command`: refused |
| 4 | | `preset` without `program`: the preset is loaded to its end; with `boot: debug` it only builds the machine, which stands before its first instruction |
| 5 | | `reset: reset` / `allReset`: a Reset / Cold boot without the run to key readiness; the machine stands at the reset vector |
| 6 | | `listings` and `symbols` are bound to the CPU `cpu` names (default the first thread) with the qualifiers `bank`, `me`, `pu`, `pv`; a listing whose bytes do not match memory is reported and not used for source |
| 7 | | With `program`: Build & Load (below). Else a `command` is typed with ENTER once the machine runs |
| 8 | | The machine is paused again; the reply is sent |
| 9 | `configurationDone` | The machine resumes, unless `stopOnEntry`: then it stays paused and `stopped` (reason `entry`) is sent. A program started with `after: call` or `stopOnEntry` runs only from here |

### Build & Load

| Step | Rule |
|---|---|
| Clean start | With `cleanStart` (default `true`): the configuration's `preset`, else the model's startup preset (refused when it is for another model), else a Cold boot to key readiness. Told as `output` "Clean start: preset `<path>`" / "Clean start: startup preset `<path>`" / "Clean start: Cold boot (no startup preset for this model)" |
| Placing | `bin` by the machine-code loader, at `address`, else for code without a header at the lowest address of a listing line with bytes |
| Entry | `entry` (a number or a listing symbol), else the header's start address, else the listing's entry inside the code, else the load address |
| Binding | `listing` (with `source` as its main source file) and `symbols` are bound over the bytes loaded |
| `after` | `none` (default): nothing. `call`: the starting command (or the configuration's `command`) is typed with ENTER. `stopOnEntry`: the same, after a breakpoint at the entry that is removed when hit and told as `stopped` with reason `entry`. Without a starting command: warning "no BASIC command starts this code; load it, then call it from your own code" |
| Report | `output` "Loaded `<file>` at `<lo>`-`<hi>`" and, with a command, "; start with `<command>`" |

### Set-up refusals

| Case | Reply message |
|---|---|
| A key with a wrong value | `Bad "<key>": <what>` (`<what>`: "a number from 0 to 7 (&, 0x or $ for hex)", "a number up to &FFFF (&, 0x or $ for hex)", "one of <the key's choices>" (e.g. "one of none, reset, allReset"; `core/Debug/AttachKeys.hpp`), "true or false", "a block of settings", "a list of paths or {path, ...} objects", "a list of paths", "text", "a path") |
| `slot` in `program` | `Bad "slot": not a key of "program": the loader places code by the machine's MODE, the program area TITLE selects, and the address. Set MODE / TITLE on the calculator first.` |
| No `bin` in `program` | `Bad "bin": it is missing` |
| An `entry` that is neither | `Bad "entry": an address from 0 to &FFFF, or a symbol of the listing` |
| `boot: debug` with `program` or `command` | `"boot": "debug" runs the boot under the debugger; it takes no "program" or "command"` |
| `project` unreadable | `Project <path>: <why>` |
| A preset that fails | `Preset failed: <why>` |
| The clean start fails | `Clean start failed: <why>` |
| The load fails | `Load failed: <why>` |
| Another operation running | `Another operation is running.` |
| The operation stopped before its end (a model switch, a Reset from the menu) | `The operation was stopped.` |
| No machine | `No machine is running` |

## Requests

| Request | Arguments | Reply / effect |
|---|---|---|
| `initialize` | | capabilities; then the event `initialized` |
| `attach`, `launch` | the configuration | set-up (above) |
| `configurationDone` | | resume, or stay paused with `stopOnEntry` |
| `disconnect` | | the session ends: its breakpoints and watches go, a paused machine resumes, the client is closed |
| `restart` | `arguments`: the (edited) configuration, else the last one | the project is read again; set-up steps 2 to 8 with `reset: none` taken as `reset`; then resume, or stay paused with `stopOnEntry` (told as `entry`) |
| `threads` | | one thread per debug target, from 1: `LH5801` (PC-1500, PC-1500A); `Z80 (SC7852)` and `LH5803` (PC-1600); the bus owner's name gets ` [bus]` when there are two |
| `stackTrace` | `threadId`, `startFrame`, `levels` | frame 0 is the live state, frames 1 to 20 the last instructions that CPU ran, newest first (`presentationHint: subtle`; an interrupt as `interrupt at HHHH`). Frame id = thread × 1000 + index; `name` is `HHHH  <instruction>`; `source`/`line` where a listing covers the address |
| `scopes` | `frameId` | one scope `Registers` (`presentationHint: registers`) |
| `variables` | `variablesReference` | the registers (16-bit ones with a `memoryReference`), `Flags` (expandable: one variable per flag bit) and, for frame 0, `Banks` (expandable: the bank state) |
| `setVariable` | `variablesReference`, `name`, `value` (an expression) | live frame only: "Only the live frame's registers can be changed"; an unknown name: "Can't set `<name>`" |
| `readMemory` | `memoryReference`, `offset`, `count` (at most 65536) | `address`, `data` (base64); leading unreadable bytes are counted in `unreadableBytes`, not sent |
| `writeMemory` | `memoryReference`, `offset`, `data` (base64) | `bytesWritten`; none written: "Memory at HHHH isn't writable"; then the event `memory` |
| `disassemble` | `memoryReference`, `offset`, `instructionOffset`, `instructionCount` | `instructions`: `address`, `instructionBytes`, `instruction`, `symbol`, `location`, `line` |
| `evaluate` | `expression`, `frameId` (default the bus owner's live frame) | `result`: a value 0 to FFFF as `HH (n)` / `HHHH (n)` with a `memoryReference`, any other as decimal; an error gives the expression's error text |
| `continue` | | resume; `allThreadsContinued: true` |
| `pause` | | pause the CPU; `stopped` with reason `pause` |
| `next`, `stepIn`, `stepOut` | `threadId`, `granularity` | step over / into / out of the named thread; with a source line and no `granularity: instruction`, until the line changes or code without source is reached |
| `setBreakpoints` | `source.path`, `breakpoints[]` (`line`, `condition`, `hitCondition`, `logMessage`) | line breakpoints; a line goes to the first line with code at or after it |
| `setFunctionBreakpoints` | `breakpoints[]` (`name`, …) | a listing or symbol-table symbol; on the CPU and bank qualifier of its binding |
| `setInstructionBreakpoints` | `breakpoints[]` (`instructionReference`, `offset`, …) | at an address |
| `dataBreakpointInfo` | `name` (an expression), `variablesReference`, `frameId`, `bytes` | `dataId`, `description` (`HHHH (n bytes)`, `#HHHH` for ME1), `accessTypes` `read`/`write`/`readWrite`, `canPersist: true`; a register: "Registers can't be watched; use a conditional breakpoint"; not an address: "Not a memory address" |
| `setDataBreakpoints` | `breakpoints[]` (`dataId`, `accessType`, default `write`, `condition`, `hitCondition`) | memory watches; instruction fetches do not trigger them; the machine stops after the instruction that made the access |
| `setExceptionBreakpoints` | | `breakpoints: []` |
| `c1600/load` | a `program` (when it has `bin`), else the configuration's | Build & Load; the project is read again. Reply: `start`, `end`, `entry` (memory references), `call` (the starting command). Without a program in the configuration (a ROM project): set-up steps 2 to 8 again, then as at `configurationDone` |
| `c1600/reset` | `kind` (`allReset`: Cold boot; else Reset), `stop` (default `true`) | Reset or Cold boot without the run to key readiness; stands before the first instruction (`stopped`, reason `entry`), or resumes with `stop: false` |
| `c1600/display-text` | | the display as text, the object the harness's `display-text` gives with `--json`; needs a machine, not a DAP session |
| `c1600/quit` | | ends the session and the program (the harness's run with exit status 0) |
| any other | | "Unsupported request '`<name>`'" |

`initialize` and `c1600/display-text` need a machine; every other request
but `disconnect` and `c1600/quit` needs an initialized session on a machine
with debug targets. Else: "No machine is running".

Capabilities in the `initialize` reply: `supportsConfigurationDoneRequest`,
`supportsConditionalBreakpoints`, `supportsHitConditionalBreakpoints`,
`supportsLogPoints`, `supportsFunctionBreakpoints`,
`supportsInstructionBreakpoints`, `supportsDataBreakpoints`,
`supportsDisassembleRequest`, `supportsSteppingGranularity`,
`supportsReadMemoryRequest`, `supportsWriteMemoryRequest`,
`supportsSetVariable`, `supportsEvaluateForHovers`,
`supportsRestartRequest` (all `true`); `supportsValueFormattingOptions`
`false`.

### Breakpoint replies

Each breakpoint is answered with `id`, `verified`, `line`,
`instructionReference` and, where it applies, a `message`:

| Message | When |
|---|---|
| "No code at or after this line in a loaded listing" | a line breakpoint that binds nowhere |
| "Only while `<qualifier>`" (`bank 3`, `pv=1`, …) | a breakpoint tied to a bank qualifier; it is passed while that bank is not switched in |
| "Unknown symbol '`<name>`'" | a function breakpoint on no known symbol |
| "Unknown instruction reference" | an unreadable instruction reference |

### Memory references

A memory reference is a hexadecimal number `0xNNNN…`: the address in its
low 16 bits, the thread above them when it is not 1, and `0x100000` added
for the LH580x's ME1. Anything else is refused "Bad memory reference".

## Events

| Event | Body | When |
|---|---|---|
| `initialized` | | after the `initialize` reply |
| `stopped` | `reason`, `threadId` (the CPU that caused it), `allThreadsStopped: true`, `hitBreakpointIds` (the client's breakpoints hit) | the machine paused outside the set-up and outside a line step |
| `output` | `category` (`console`, `important`), `output` | load reports, warnings, log messages, condition errors, listing mismatches |
| `breakpoint` | `reason: changed`, `breakpoint` | a breakpoint resolved anew (a listing bound or rebound) |
| `invalidated` | `areas: ["all"]` | the machine was rebuilt (a model switch, a preset): with the `output` "The machine was rebuilt; breakpoints re-applied." |
| `memory` | `memoryReference`, `offset`, `count` | after `writeMemory` |
| `terminated` | | after Disconnect in Settings ▸ Debugger, with the `output` (`important`) "Disconnected from the app (Settings > Debugger)." |

There is no `continued` event.

### Stop reasons

| `reason` | Cause |
|---|---|
| `breakpoint` | a line, function or instruction breakpoint |
| `data breakpoint` | a memory watch |
| `step` | a step ended |
| `entry` | the entry breakpoint of `after: stopOnEntry`, `stopOnEntry` at `configurationDone` or `restart`, `c1600/reset` with `stop` |
| `pause` | `pause`, or a pause from elsewhere (Machine ▸ Pause, the harness) |

## Conditions, hit conditions, log messages

| Item | Syntax |
|---|---|
| `condition` | an expression; the breakpoint pauses when it is not 0. One that cannot be worked out pauses and writes "Breakpoint condition '`<text>`': `<error>`" |
| `hitCondition` | `N`, or `==`, `=`, `!=`, `>=`, `<=`, `>`, `<`, `%` followed by `N` (`N` a C integer: decimal, `0x` hexadecimal); compared with the hits counted after the condition passed; `% N` meets every Nth hit. One that cannot be read counts as met |
| `logMessage` | text with `{expression}` parts; each part is written as `0xHH` (0 to FF), `0xHHHH` (above), or decimal when negative; a part that fails is written `{<error>}`. A log message writes its line as `output` and does not pause, on memory breakpoints too |

## Expressions

Used by conditions, log message parts, Watch, hovers, the Debug Console,
`setVariable` and `dataBreakpointInfo`. Integers, 64 bits signed;
comparisons and logic give 0 or 1.

| Element | Syntax |
|---|---|
| Numbers | `42` decimal; `0x7A00`, `$7A00`, `&7A00` hexadecimal |
| Registers | the CPU's names, any case. LH5801/LH5803: `a`, `x`, `y`, `u`, `s`, `p`, `t`, `pu`, `pv`, `tm`, the halves `xl`, `xh`, `yl`, `yh`, `ul`, `uh`. Z80: `af`, `bc`, `de`, `hl`, `ix`, `iy`, `sp`, `pc`, `af'` (or `af2`), `bc'`, `de'`, `hl'`, `i`, `r`, `im`, `iff1`, `iff2`, the halves `a`, `f`, `b`, `c`, `d`, `e`, `h`, `l`, `ixh`, `ixl`, `iyh`, `iyl` |
| Flags | the flag name with `f`: LH580x `cf`, `ief`, `zf`, `vf`, `hf`; Z80 `cf`, `nf`, `pvf`, `hf`, `zf`, `sf` |
| Symbols | the names of the bound listings and symbol tables (after registers and flags) |
| Memory | `[a]` a byte; `w[a]` a word in the CPU's byte order (LH580x high byte first, Z80 low byte first); `#[a]` a byte of the LH580x's ME1 |
| Operators | unary `!` `~` `-` `+`; `*` `/` `%`; `+` `-`; `<<` `>>`; `<` `<=` `>` `>=`; `==` `!=`; `&`; `^`; `\|`; `&&`; `\|\|`; parentheses; C's precedence |

Errors: "empty expression", "unexpected '`<c>`'", "missing ']'", "missing
')'", "missing hex digits", "unexpected end of expression", "unknown name
'`<name>`'", "memory not available", "memory at `<n>` is not readable",
"division by zero".

## Launch configuration

The keys of a launch configuration and of a project preset's `debug:`
block, the number rule and the keys the VS Code extension adds:
[help chapter 7, "The `debug:` block and launch configurations"](../../help/07-debugger.md#the-debug-block-and-launch-configurations).
