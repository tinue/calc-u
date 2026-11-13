# C1600-3: DAP in the app layer

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: LIB-10 (debug targets, the run control, breakpoints and
  watches with their tests, pausing at the next start), C1600-2 (the
  loaders, for Build & Load), C1600-4 (the `debug:` block of a preset),
  LIB-6 (loading a preset), LIB-9 (the `[debug]` settings), LIB-11 (the
  harness as a host), LIB-14 (the Debugger section of the settings
  dialog), LIB-18 (Reset, Cold boot).
- Contracts and decisions: `docs/contracts/debugger.md` (DB-4, the
  minimal agreement, I-1 to I-4); Briefing §2 (no DAP for Calc-U-59);
  ADR 0006, 0009.
- Facts: Calc-U-1600 at `b4ea8c8`; every `path:line` below is at that
  commit. `docs/Debugger.md` there is the user's description of the
  debugger.

Words used here: **DAP** is the Debug Adapter Protocol, which debuggers
such as VS Code speak; the **server** is the part of Calc-U-1600 that
answers it; a **client** is the debugger that connects; a **DAP
session** lasts from a client's `initialize` to its disconnect; a
**configuration** is what the client sends with `attach`; a **thread**
is DAP's word for a debug target, numbered in the order the machine
lists its targets (LIB-1 AC-36), from 1. **Build & Load** puts a freshly
built program into the machine (AC-7), after a **clean start** when the
machine is first built anew. A **listing** is
the file an assembler writes with addresses, bytes and source lines; a
**binding** ties a listing or a table of symbols to a debug target, so
that addresses become source lines and names. Words of LIB-10:
breakpoint, watch, hit, qualifier, run state.

## User Stories
- As a developer of a program, a ROM or a driver for a Sharp machine, I
  want to debug it from VS Code as today: source lines, breakpoints,
  stepping, registers and memory.
- As a developer, I want Build & Load to put my freshly built program
  into a clean machine and stop at its first line.
- As a developer of Calc-U-1600, I want the server to run without a
  window, so that its scenarios can be checked automatically.

## Out of Scope
- The debug targets, pausing, stepping, breakpoints, watches and the
  trace as such (owner: LIB-10).
- Placing a program in memory and the commands that start it (owner:
  C1600-2); the keys of a `debug:` block and their check when a preset
  is read (C1600-4 AC-11).
- The VS Code extension, which stays as it is, except that its
  description of `program.after` gives `none` as the default (OQ-2);
  its presets and templates are upgraded with C1600-4.
- A debugger for BASIC, a debugger in the Web frontend, and DAP for
  Calc-U-59 (`debugger.md`, backlog and Briefing §2).
- The INSPECTOR.

## Acceptance Criteria

The server and its hosts
- [ ] **AC-1** — Given Calc-U-1600, then the server is part of its app
  layer and needs from its host only a socket on this computer, the
  settings and the app session; it runs in the Qt6 frontend and under
  the harness (AC-22), and not in the Web frontend, as today
  (`debugger.md` DB-4; today every part of it is in the Qt6 frontend,
  `Qt6/app/debug/DapServer.cpp:3-17`, `Qt6/app/debug/DapSession.hpp:1-4`).
- [ ] **AC-2** — Given `[debug] dap-enabled` is on, or the option
  `--dap <port>` for one run, which changes no setting, then the server
  listens on 127.0.0.1 only, on `[debug] dap-port` or that port (LIB-9
  AC-26, AC-28). Given the port cannot be had, then the status says so
  (AC-21) (`Qt6/app/debug/DapServer.cpp:17`, `Qt6/app/main.cpp:84-92`).
- [ ] **AC-3** — Given a client is connected, when a second one
  connects, then the second is told "Calc-U-1600 already has a debugger
  attached…" and is closed (`Qt6/app/debug/DapServer.cpp:31-46`).

A session
- [ ] **AC-4** — Given `initialize`, then the machine is paused (LIB-10
  AC-6), and it is paused again after every step of the set-up of AC-6
  that built it anew or let it run; given `configurationDone`, then it
  resumes, or with `stopOnEntry` it stays paused and the client is told
  `stopped` with the reason entry
  (`Qt6/app/debug/DapSession.cpp:146-176, 542-546`).
- [ ] **AC-5** — Given `attach`, or `launch`, which means the same, then
  its configuration has the keys of a `debug:` block (C1600-4 AC-11)
  and, in a launch configuration only, `project` and `preset`. With
  `project`, that preset's `debug:` block is the base: the keys of the
  configuration win, `program` is merged key by key, and `listings` and
  `symbols` are added to the block's. `project` is the path of that
  preset. A key the server does not know is passed over in a
  configuration, except `slot` inside `program`, which was removed and
  is refused with a hint. A number is a JSON number or a text by
  the preset's rule (LIB-6 AC-1); a text of hexadecimal digits without a
  mark is refused (`Core/Debug/AttachKeys.hpp:9-88`,
  `Qt6/app/debug/DapSession.cpp:212-297`).
- [ ] **AC-6** — Given a configuration, then the machine is set up in
  this order (`Qt6/app/debug/DapSession.cpp:416-478`):
  `boot: debug` together with `program` or `command` is refused;
  with `preset` and no `program`, the preset is loaded (LIB-6), which
  builds a machine that runs until the load is over; with `boot: debug`
  the preset is loaded to build only (LIB-6 AC-55): its machine is built
  and is paused before its first instruction, and its sections are not
  carried out (LIB-10 AC-46);
  with `reset: reset` or `allReset`, a Reset or a Cold boot follows that
  is paused before its first instruction, at the reset vector;
  the `listings` and `symbols` are bound (AC-13), each to the thread its
  `cpu` names, the first without it, with its `bank`, `me`, `pu` and
  `pv` as the qualifier;
  with `program`, Build & Load follows (AC-7); without it, a `command`
  is handed to the typing engine with ENTER and is typed when the
  machine runs.
- [ ] **AC-7** — Given Build & Load of a `program`, then: with
  `cleanStart`, which is on unless it is switched off, the machine is
  first built anew, from the configuration's `preset`, else from the
  model's startup preset, else by a Cold boot, and a `preset` is used
  for nothing else when there is a `program`; the file `bin` is placed
  by the loader of C1600-2 (AC-8, AC-23 there), with the length of the
  file given by hand, so that a header that says another length does not
  refuse it, and with the address given by hand that is `address`, else
  for headerless code the lowest address of a line of the listing that
  holds bytes, so that an equate does not count; `cpu` names the
  processor; the entry is
  `entry` (a number, or a symbol of the listing), else the header's
  address to start at, else the listing's entry when it lies in the
  code, else the load address; the `listing`, with `source` as its main
  source file where that cannot be told, and the `symbols` are bound
  over exactly the bytes loaded, with `bank`, `me`, `pu` and `pv` as the
  qualifier; and then, by `after`: with `none`, the default, nothing
  more; with `call` the starting command (C1600-2 AC-11), or `command`
  in its place, is typed and the machine resumes, and in the set-up of a
  session (AC-6) it resumes only at `configurationDone` (AC-4), so the
  client's breakpoints are set first; with `stopOnEntry` the
  same, after the server has set a breakpoint of its own at the entry
  that holds once, and the client is told `stopped` with the reason
  entry when it is hit
  (`Qt6/app/debug/DapSession.cpp:343-414`,
  `Core/Debug/ProgramLoader.cpp:17-133`).
- [ ] **AC-24** — Given a debug session, then there are two ways to
  use it, and the rules of operations (LIB-18 AC-13, LIB-10 AC-13, AC-8
  here) never stand in the way of either: breakpoints that work
  normally on a running machine; or the calculator's reset routine
  watched from its reset vector, where the machine stops (AC-6,
  `reset:` or `boot: debug`), after which stepping and breakpoints work
  normally. A reset that stops at the reset vector is over when it
  stops there. How the configuration's keys combine beyond this is not
  specified; today's behaviour holds (`Qt6/app/debug/DapSession.cpp:430-444`).
- [ ] **AC-25** — Given Build & Load in a session whose configuration has
  no `program` (a ROM project), then the set-up of AC-6 is done again,
  and the machine then goes on as at `configurationDone` (AC-4)
  (`Qt6/app/debug/DapSession.cpp:496-514`).
- [ ] **AC-8** — Given a message of the client arrives while an
  operation runs, then the server holds it back and answers it when the
  operation is over, in the order of arrival; this is the server's own
  rule, as today (`Qt6/app/debug/DebugController.cpp:25-50`).
- [ ] **AC-9** — Given the machine is replaced under a DAP session (a preset,
  a model switch), then the DAP session goes on with the new machine: the
  breakpoints are set again (LIB-10 AC-15), the client gets `invalidated`
  and the line "The machine was rebuilt; breakpoints re-applied."
  (`Qt6/app/debug/DapSession.cpp:552-556`).
- [ ] **AC-10** — Given `disconnect`, a closed socket or Disconnect in
  the settings dialog, then the DAP session ends, its breakpoints and
  watches are gone (LIB-10 AC-47), and the machine runs on, resumed if
  it was paused
  (`Qt6/app/debug/DebugController.cpp:89-154`).

What it answers
- [ ] **AC-11** — Given a DAP session, then the server answers these
  requests as today, each through LIB-10 (`Qt6/app/debug/DapSession.cpp:91-120`):

  | Requests | Through |
  |---|---|
  | `threads` | the debug targets as threads (LIB-10 AC-1, AC-36) |
  | `stackTrace`, `scopes`, `variables`, `setVariable` | frame 0 of a thread is its state now, frames 1 to 20 its last instructions from the history, the newest first, which are read-only; the scopes are Registers, Flags and Banks, the last being the state of the qualifier's fields now (LIB-10 AC-1, AC-2, AC-4) |
  | `readMemory`, `writeMemory`, `disassemble`, `evaluate` | reading and writing memory, decoding, and the expressions of AC-15, for the frame the request names, else frame 0 of the current target; a memory reference is a number: the address in its low 16 bits, the thread above them, and one more bit for ME1, as today |
  | `continue`, `pause` | resume and pause (LIB-10 AC-6) |
  | `next`, `stepIn`, `stepOut` | AC-12 |
  | `setBreakpoints`, `setFunctionBreakpoints`, `setInstructionBreakpoints`, `dataBreakpointInfo`, `setDataBreakpoints`, `setExceptionBreakpoints` | AC-14; the last answers with nothing |
  | `restart` | AC-6 again with the configuration it brings, `reset: none` taken as `reset` |
  | `calcu1600/load`, `calcu1600/reset`, `calcu1600/screen`, `calcu1600/quit` | AC-17 |

  Any other request is answered "Unsupported request".
- [ ] **AC-12** — Given `next`, `stepIn` or `stepOut`, then the named
  thread steps over, in or out (LIB-10 AC-7, AC-8); where the
  instruction it stands on has a source line and the client did not ask
  for single instructions, a step goes on until the source line changes
  or code without source is reached
  (`Core/Debug/RunControl.cpp:34-49, 101-109`,
  `Core/tests/run_control_tests.cpp:122-159`).
- [ ] **AC-13** — Given a listing of sdas (`.lst`, `.rst`) or zasm
  (`.lst`), or a table of symbols after a `.SYMBOLS:` line, then it is
  bound to a thread, with a qualifier where one is given (LIB-10 AC-1):
  a listing of a loaded program over the bytes loaded, any other over
  the thread's whole address space; a binding whose bytes are not those
  in memory takes no part and the client is told how many lines differ.
  An address then shows the source line whose bytes cover it, and a
  breakpoint on a line goes to the first line with code at or after it
  (`Core/Debug/Listing/Listing.cpp:154-219`,
  `Core/Debug/SourceMap.cpp:60-170`, `Core/tests/listing_tests.cpp:62-302`).
- [ ] **AC-14** — Given breakpoints of the client, by source line, by
  address, by a symbol's name or on memory, then each is a breakpoint or
  a watch of LIB-10 (AC-10, AC-11) that belongs to the DAP session (AC-47),
  with the qualifier of its binding; a condition, a hit condition (a
  number `N` alone or after `==`, `!=`, `>=`, `<=`, `>`, `<` or `%`,
  compared with the count of hits; one that cannot be read counts as
  met) and a log message with `{expression}` parts are its test (LIB-10 AC-12): a breakpoint with a log message
  writes the line and does not pause; a condition that cannot be worked
  out pauses and says why. A breakpoint in a bank that is not switched
  in is passed, and its status says "Only while" that bank
  (`Core/Debug/BreakpointTable.cpp:86-227, 328-358`,
  `Core/tests/run_control_tests.cpp:178-271`).
- [ ] **AC-15** — Given an expression, in `evaluate`, a condition, a log
  message or a new value for a register, then it is built from: numbers,
  decimal or hexadecimal after `0x`, `$` or `&`; names of registers and
  flags, then of symbols; memory as `[a]` for a byte, `w[a]` for a word
  in the processor's byte order and `#[a]` for a byte of ME1; and the
  operators of C with their order. An error names what is wrong
  (`Core/Debug/DebugExpression.cpp:18-279`,
  `Core/tests/debug_target_tests.cpp:84, 637-701`).
- [ ] **AC-16** — Given a request of the client ends with the machine
  paused, or the machine is paused from elsewhere (a preset, the
  harness), then the client gets `stopped` once, with the thread, the
  IDs of its own breakpoints that were hit, the word that every thread
  stopped, and the reason: "pause", "step", "breakpoint", "data
  breakpoint" for a watch, or "entry" (LIB-10 AC-17). The pauses in
  between of a step by source line or of the set-up are not told.
  Further events are, as today: `initialized`, `output`, `breakpoint`
  when one is resolved anew, `invalidated`, `memory` after a write, and
  `terminated` after Disconnect in the settings dialog
  (`Qt6/app/debug/DapSession.cpp:79-82, 552-582, 1088-1092`).
- [ ] **AC-17** — Given the server's own requests, then:
  `calcu1600/load` does Build & Load, of the `program` it brings or of
  the configuration's, and answers with the range, the entry and the
  starting command;
  `calcu1600/reset` does a Reset, or with `kind: allReset` a Cold boot,
  paused before the first instruction unless `stop` is false;
  `calcu1600/screen` answers with the display as text (LIB-11 AC-19),
  also without a DAP session;
  `calcu1600/quit` ends the DAP session and the program
  (`Qt6/app/debug/DapSession.cpp:125-131, 496-540`).

In the frontends
- [ ] **AC-18** — Given the Qt6 frontend, then the Debugger section of
  the settings dialog has, next to the two settings (LIB-14 AC-37), a
  status line and the button Disconnect, which can be used while a
  client is connected (`Qt6/app/SettingsDialog.cpp:336-375`).
- [ ] **AC-21** — Given the server, then its status is one of "Off",
  "Listening on 127.0.0.1:`<port>`", "Client connected on
  127.0.0.1:`<port>`" and "Port `<port>` not available: `<why>`", and a
  state topic of Calc-U-1600 carries it
  (`Qt6/app/debug/DebugController.cpp:79-85`).
- [ ] **AC-19** — Given the VS Code extension of `b4ea8c8`, its code
  unchanged except the description of `program.after` (OQ-2), and its
  presets and templates as C1600-4 upgrades them, then
  it works against the server: attach, Build & Load, Reset and Stop, All
  Reset and Stop, and its four project templates; the scenarios of AC-20
  that use a project and a template check it
  (`vscode/calcu1600-debug/package.json:26-603`,
  `vscode/calcu1600-debug/extension.js:215-262`).

Checks
- [ ] **AC-22** — Given the harness in its form `serve` with
  Calc-U-1600 and the option `--dap <port>`, then a client can attach to
  a machine without a window; while a DAP session wants the machine to
  run, the harness lets emulated time pass as fast as it can, without a
  wall clock (`debugger.md` DB-4; LIB-11 AC-3, AC-6; OQ-1).
- [ ] **AC-20** — Given the ten scenarios of `tools/dap_smoke.py`, then
  each passes against the server hosted by the harness, as a test of the
  test suite; today they are run by hand against a running app
  (`tools/dap_smoke.py:112-631`).
- [ ] **AC-23** — Given what the tests of Calc-U-1600 check today about
  listings, bindings, expressions, conditions, hit conditions and log
  messages, then it holds (`Core/tests/listing_tests.cpp`,
  `Core/tests/debug_target_tests.cpp:84, 637-732`,
  `Core/tests/run_control_tests.cpp:122-271`).

## Edge Cases
- **EC-1** — Given `--dap` with a port outside 1024 to 65535, then the
  program says so and the server does not listen; today the option is
  passed over without a word (`Qt6/app/debug/DebugController.cpp:60-66`).
- **EC-2** — Given a `program` without `bin`, an unknown value of
  `after`, `cpu` or `reset`, or the removed key `slot`, then the request
  is refused with `Bad "<key>": <what>`.
- **EC-3** — Given Build & Load of headerless code without `address`
  and without a listing, then it is refused as C1600-2 refuses headerless
  code without an address (AC-8 there), with the hint that a listing
  would do too.
- **EC-4** — Given the clean start would take the model's startup preset
  and that preset is for another model, then Build & Load is refused.
- **EC-5** — Given a DAP session is ended while a request of it is carried
  out, then the request is finished first.
- **EC-6** — Given `setVariable` for a register of an earlier
  instruction, then it is refused: only the registers of now can be
  changed.
- **EC-7** — Given a listing whose lines do not match their source
  text, then it is bound, and the client is told how many lines did not
  match.

## Technical Requirements (optional)
- The server names no type of a frontend; what it needs from a host is a
  local socket, a way to be called again later on the app session's thread,
  and the settings (`debugger.md` DB-4, I-4).
- The server reaches the machine only through LIB-10, C1600-2 and the
  app session; it names no machine class.
- The names of the server's own requests keep the prefix `calcu1600/`,
  because the extension uses them.

## Open Questions
- [x] **OQ-1** — *Decided as recommended (user, 2026-11-10, batch 7 gate).* How the harness hosts the server (AC-22). A harness run
  has no wall clock and two forms (`harness.md` I-2; LIB-11 AC-6), but a
  debugger expects a machine that runs by itself after `continue`.
  *Recommendation:* no third form and no real time: `serve --dap
  <port>`, in which the machine runs unpaced whenever a DAP session has
  it running. A scenario that pauses by a breakpoint repeats exactly;
  one that pauses by hand does not, and that is said in the help of the
  option.
- [x] **OQ-2** — *Decided as recommended (user, 2026-11-10, batch 7 gate).* `program.after` without a value means `none` in the
  server, while the extension's description of the key gives
  `stopOnEntry` as its default (`Qt6/app/debug/DapSession.cpp:379-382`,
  `vscode/calcu1600-debug/package.json:228-234`). *Recommendation:* the
  server keeps `none`; the extension's description is corrected.
- [x] **OQ-3** — *Decided as recommended (user, 2026-11-10, batch 7 gate).* The server never sends `continued` when it resumes the
  machine by itself (`configurationDone`, `restart`, the two own
  requests). *Recommendation:* keep that; no client is known to need it.
- [x] **OQ-4** — *Decided as recommended (user, 2026-11-10, batch 7 gate).* A log message on a breakpoint on memory: the code
  takes it, the documentation says it does not
  (`Core/Debug/BreakpointTable.cpp:339-342`, `docs/Debugger.md:344`).
  *Recommendation:* take it, as the code does.
- [x] **OQ-5** — *Decided as recommended (user, 2026-11-10, batch 7 gate).* "DAP", "server", "client", "DAP session",
  "configuration", "thread", "Build & Load", "clean start", "listing"
  and "binding" for the glossary, as defined above.

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| DAP stays Calc-U-1600 product code, moved out of the Qt6 frontend into the app layer, on the library's debug target and run control, without Qt types; the harness can host it | `debugger.md` DB-4 | 2026-11-10 |
| Listings, symbols, the `debug:` block, `boot: debug` and the VS Code extension stay Calc-U-1600 material, unchanged | `debugger.md`, minimal agreement | 2026-11-10 |
| `stopOnEntry` and pause-on-start stay separate things | `debugger.md`, minimal agreement | 2026-11-10 |
| No DAP for Calc-U-59 | Briefing §2 | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 7 gate): the scenarios of `tools/dap_smoke.py` become tests of the test suite (AC-20) | The server no longer needs a window; today nothing checks it automatically | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 7 gate): a wrong `--dap` port is an error at the start (EC-1) | Today it is ignored without a word | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 7 gate): the server's status is a state topic of the product (AC-21) | The settings dialog of any frontend can then show it | 2026-11-10 |
| "The lowest address of the listing" (AC-7) means the lowest address of a line with bytes, as today's listing readers keep only such lines | Phase 5, preset pass (`docs/walkthroughs/presets.md`, A-11); AC-23; `Core/Debug/Listing/ZasmListing.cpp:90-91`, `SdasListing.cpp:174-175` | 2026-11-11 |
| Two uses of a debug session: breakpoints that work normally, or the reset routine watched from the reset vector; a reset that stops there is over when it stops; nothing more is specified about combining `reset:` and `boot: debug` (AC-6, AC-24) | user, 2026-11-13 (B-19 (a), (b) of `docs/walkthroughs/presets.md` §5.2): "may be overspecified" | 2026-11-13 |
| In the set-up of a session the program started by `after: call` waits for `configurationDone` (AC-7); Build & Load without a `program` does the set-up again (AC-25) | user, 2026-11-13 (B-19 (c), (e) of `docs/walkthroughs/presets.md` §5.2); today the program runs during `attach`, before the client's breakpoints arrive | 2026-11-13 |
