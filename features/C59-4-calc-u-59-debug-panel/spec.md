# C59-4: Calc-U-59 debug panel

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: C59-1 (the TI machines), LIB-10 (the debug target, the run
  control, its run state, the three run-control buttons, the trace),
  LIB-13 (opening a file, for the ASM overlay); also named here: LIB-3
  (the log, product topics), LIB-4 (product entries), LIB-9 (the trace
  settings), LIB-14 (the area for panels).
- Contracts and decisions: `docs/contracts/debugger.md` (the minimal
  agreement, DB-6), `docs/contracts/frontend.md` (FE-6, SP-3),
  `docs/contracts/capabilities.md` (product kinds of Calc-U-59); ADR
  0023 as amended, 0037; `docs/behaviour-changes.md` row 59-1;
  `docs/glossary-mapping.md` ("FREEZE", "RESUME" stay).
- Facts: Calc-U-59 at `v2.3.0`; every `path:line` below is at that tag.
  `reference/DebugAPI.md` there describes the panel's data.

Glossary terms used: Debug panel, Pause, Trace, ASM overlay, Level,
Program step. On the panel, pause is called FREEZE and pause-on-start
FREEZE ON START, as today. Words of this spec: the **heatmap** shows
how often each part of the ROM was carried out; a **dump** is a text
that lists a part of the calculator's memory into the log. Words of
LIB-10: run state, history, breakpoint.

## User Stories
- As a user who wants to see how the calculator works, I want to freeze
  it, step through a keystroke program or through the ROM, and watch the
  registers, as today.
- As a user, I want one place for the log, with dumps of the memory and
  a trace I can switch on.
- As a tinkerer, I want to load a few machine instructions of my own
  into the calculator and run them.

## Out of Scope
- Pausing, stepping, pause-on-start, breakpoints and the trace as such
  (owner: LIB-10); this feature shows them and adds what only Calc-U-59
  has.
- The debug target of the TI machines and what its registers mean
  (C59-1; LIB-10 AC-28).
- The Swift views of this panel are specified here like the others;
  where the panel sits in each window is the layout of LIB-14 AC-33 and
  C59-8 AC-15.
- A CPU tab that shows the commented disassembly of the ROM, and
  breakpoints in the panel (backlog, `debugger.md`).
- The harness commands for coverage and timings (owner: C59-9).

## Acceptance Criteria

The panel
- [ ] **AC-1** — Given a frontend of Calc-U-59 that shows the debug
  panel, then the panel has the three tabs CALCULATOR, CPU and LOG, as
  today, and is fed by product topics of Calc-U-59 only (`debugger.md`
  DB-6; `App/Views/DebugView.swift:11-63`). It is shown in Qt6 and in
  the Swift frontend where the layout has room for it, and in the Web
  frontend in the wide layout (LIB-14 AC-33; C59-8 AC-15).
- [ ] **AC-2** — Given the step `debug-panel:` with `calculator`, `cpu`
  or `log` (LIB-10 AC-32), then that tab is shown; while an ASM overlay
  runs, `calculator` is passed over (`Shared/SessionController.cpp:678-679`).

CALCULATOR tab
- [ ] **AC-3** — Given the CALCULATOR tab, then it shows what it shows
  today, read through the debug target (LIB-10 AC-28) between two
  program steps: the data registers that are not zero, on the TI-58C
  also H00 to H03; the partition; the program steps around the current
  one, which is marked as the next to run or as running; the registers
  of pending operations, T and X; the flags, the display format and the
  angle mode; where the program that runs comes from; the calculator's
  own 16 registers; and the return addresses
  (`reference/DebugAPI.md:165-199`).
- [ ] **AC-4** — Given the machine is paused, then the tab also shows
  the whole listing of the program that runs, from the calculator's
  memory, from the ROM's own programs or from the module, with the
  current step marked (`reference/DebugAPI.md:124-127, 201-211`).
- [ ] **AC-5** — Given the tab's buttons, then FREEZE, STEP and RESUME
  are the run-control buttons of LIB-10 AC-18 for the level `program`;
  FREEZE ON START arms pause-on-start for `program` and then reads
  ARMED, and pressing ARMED disarms it; FREEZE and FREEZE ON START can
  be used only while the machine runs and this tab's pause-on-start is
  not armed (`App/Views/LiveDebugView.swift:45-87`).

CPU tab
- [ ] **AC-6** — Given the CPU tab, then it shows the last instructions
  from the history of the debug target, each with its address, its text
  and the registers before it: the newest 32 while the machine runs, up
  to 1024 and the next instruction while it is paused
  (`reference/DebugAPI.md:251-292`).
- [ ] **AC-7** — Given the tab's buttons, then they are those of AC-5
  for the level `cpu` (`App/Views/CPUInspectorView.swift:295-337`).
- [ ] **AC-8** — Given a TI machine, then it lists a product kind for
  the heatmap, which counts for each address of the ROM how often it was
  carried out since the count was last set to zero. The CPU tab shows
  the counts as a picture of the ROM; they are set to zero by the tab's
  button, by a Reset, by a model switch and when the panel comes into
  view (`capabilities.md`, product kinds;
  `Shared/SessionController.cpp:188, 203, 756-767`,
  `tests/shared_debug_tests.cpp:220`).

LOG tab
- [ ] **AC-9** — Given the LOG tab, then it shows the log (LIB-3 AC-11),
  the last 2000 lines, and its text can be copied; a button clears the
  view (`Shared/DebugLog.hpp:13-29`).
- [ ] **AC-10** — Given the buttons SCOM, Memory, Prog and Vars, then
  each writes its dump into the log: the calculator's own registers, the
  memory, the program, the data registers, in today's layout
  (`reference/DebugAPI.md:330-336`, `tests/shared_debug_tests.cpp:287`).
- [ ] **AC-11** — Given the button LOG, then it steps the level of the
  machine's own log lines through off, info and debug and shows which
  one holds; at debug the machine logs each write to its own registers
  and to memory, as today; lines of the app itself are in the log at
  every level (`Shared/SessionController.cpp:795-804`,
  `reference/DebugAPI.md:343-365`).
- [ ] **AC-12** — Given the button TRACE, then it starts and ends the
  trace of LIB-10 AC-23 under the file name of LIB-10 AC-33, shows
  whether a trace runs, whoever started it (LIB-10 AC-25), and cannot be
  used where the trace folder cannot be written; in the Web frontend the
  tab lists the traces that have ended, to keep (LIB-10 AC-26)
  (`Shared/SessionController.cpp:832-855`, `Web/src/Debugger.jsx:261-272`).

The ASM overlay
- [ ] **AC-13** — Given a TI machine, then it lists a product kind for
  the ASM overlay: up to 2048 instructions of 13 bits can be put over
  the addresses 1800 to 1FFF of the ROM, taken away again, and entered
  at 1800 (`capabilities.md`, product kinds; `Core/ROM.hpp:11-36`,
  `Core/TI59Machine.cpp:250-264`).
- [ ] **AC-14** — Given an ASM overlay file, then it is a text file in
  the ASM overlay format, version 2.0: the first line is `Calc-U-59 ASM
  2.0`; everything up to a line `HEX:` is for the reader and is not
  read; after it come the instructions as groups of four hexadecimal
  digits, optionally after `0x`, parted by spaces or line ends; a `;`
  starts a comment that runs to the end of its line. A file without the
  first line is outdated (ADR 0037; behaviour change 59-1; today
  `Shared/AsmOverlay.cpp:10-49`; OQ-1).
- [ ] **AC-15** — Given an ASM overlay file is read, then it is refused,
  with a notice that says which, when: it is outdated; it has no `HEX:`
  line; nothing follows that line; a group is not four hexadecimal
  digits or a multiple of four; or there are more than 2048
  instructions (`Shared/AsmOverlay.cpp:16-47`,
  `tests/shared_debug_tests.cpp:238-259`).
- [ ] **AC-16** — Given the box "ASM OVERLAY" on the CPU tab, then it
  shows the file's name, the number of instructions and a status line,
  and has three buttons: Select File, which reads a file and puts it
  over the ROM at once; Run, which can be used when instructions are
  loaded and enters them at 1800; and Clear, which takes them away, and
  resets the machine when they were running
  (`Shared/SessionController.cpp:859-933`,
  `Qt6/app/debug/AsmOverlayBox.cpp:19-70`).
- [ ] **AC-17** — Given Run, then the machine is brought to the next
  program step boundary and made to go on at 1800; given it has not got
  there after 8192 instructions, then the status says "ASM entry failed
  after n step(s)." and a notice says so. Given pause-on-start for `cpu`
  is armed, then the machine is paused at 1800; otherwise the overlay
  runs (`Shared/AsmOverlay.cpp:51-59`, `Core/TMC0501.cpp:424-463`).
- [ ] **AC-18** — Given an overlay runs, then the CALCULATOR tab cannot
  be chosen and the panel shows the CPU tab; given the machine is
  paused, reset or cold booted, then the overlay no longer runs, its
  instructions stay over the ROM, and Run enters them again; given the
  model is switched, then they are put over the new machine's ROM
  (`Shared/SessionController.cpp:141-163, 769-775`,
  `tests/shared_controller_tests.cpp:221-243`).
- [ ] **AC-19** — Given a file whose name ends in `.asm` is opened
  (LIB-13 AC-1, AC-13), then it is an ASM overlay and is read as by
  Select File; this is the file kind Calc-U-59 adds to "open file".
  Today such a file can only be chosen in the box
  (`Shared/SessionController.cpp:620-624`).
- [ ] **AC-20** — Given the four overlays under `examples/assembly/`,
  then each is in version 2.0, and each that loads in 2.3.0 loads and
  gives the same instructions (`tests/shared_debug_tests.cpp:238-259`).

Every frontend, and the checks
- [ ] **AC-21** — Given the Swift, Qt6 and Web frontends, then AC-1 to
  AC-19 hold in each, with the labels of today and their short forms
  where the panel is narrow ("F.START", "DEBUG")
  (`App/Views/LiveDebugView.swift:71-76`).
- [ ] **AC-22** — Given the harness with Calc-U-59, then `ti59 asm load
  <file>`, `ti59 asm run` and `ti59 asm clear` do what the three buttons
  do, `ti59 dump scom|memory|prog|vars` gives a dump as its reply, and
  `ti59 log-level off|info|debug` sets the level of AC-11.
- [ ] **AC-23** — Given what the tests of Calc-U-59 check today about
  the panel, then it holds: the step shown is the next one; while the
  machine runs only whole program steps are shown; the cursor at rest
  is where LRN after SST shows it; the snapshot and the listing; the
  heatmap counting new instructions once; the overlay's reading and
  running; the pages of the log; the dumps
  (`tests/shared_debug_tests.cpp:78-287`).
- [ ] **AC-24** — Given the GUI test of each frontend, then it has two
  scenarios: under the ID C59-4 AC-5, FREEZE, STEP and RESUME on the
  CALCULATOR tab; under the ID C59-4 AC-16, an overlay is chosen and run
  and the CPU tab shows it (today
  `Web/tests/play-calculator.spec.js:389`,
  `Calc-U-59UITests/ScreenshotTests.swift:115-145`; ADR 0032).

## Edge Cases
- **EC-1** — Given Run without instructions loaded, then the status
  says "No ASM overlay loaded." and nothing else happens.
- **EC-2** — Given Select File with a file that is refused, then the
  instructions that were loaded before stay.
- **EC-3** — Given STEP on a tab whose level does not own the pause,
  then it cannot be used (LIB-10 AC-18); a preset's `step:` of that
  level is carried out all the same (LIB-10 AC-45).
- **EC-4** — Given a frontend that does not show the panel (the Web
  frontend in its portrait layout), then a `debug-panel:` step is kept
  and has its effect when the panel is shown
  (`Web/src/Debugger.jsx:290-292`).
- **EC-5** — Given an ASM overlay is opened before a machine runs, then
  it is refused with a notice.

## Technical Requirements (optional)
- The panel is not part of the shared component sets; each frontend of
  Calc-U-59 has its own, and only the three run-control buttons are the
  sets' (`debugger.md` DB-6; ADR 0023 as amended).
- What the panel shows is computed below the frontends and sent as
  product topics; a frontend draws it (`frontend.md` FE-6).
- The ASM overlay format is an own format (ADR 0037), described in the
  reference chapter of the help (LIB-16 AC-10).

## Open Questions
- [ ] **OQ-1** — The new version of the ASM overlay format (AC-14). The
  index asks for a version; today's files have none, and after `HEX:`
  any run of four or more hexadecimal digits is taken as instructions,
  also inside what looks like a comment. *Recommendation:* as in AC-14:
  a first line `Calc-U-59 ASM 2.0`, and `;` for comments.
- [ ] **OQ-2** — The level button of the log (AC-11). The machine has
  log lines only at the level debug today, so "info" shows nothing more
  than "off". *Recommendation:* keep the three levels as they are.
- [ ] **OQ-3** — The commands of AC-22. *Recommendation:* as written;
  they belong here by rule 4 of the index.
- [ ] **OQ-4** — "Heatmap" and "dump" for the glossary, as defined
  above.

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| The debug panel is per product; Calc-U-59 keeps the CALCULATOR, CPU and LOG tabs, FREEZE and FREEZE ON START, the ASM overlay and the heatmap, unchanged; no DAP | `debugger.md`, minimal agreement, DB-6 | 2026-11-10 |
| The component sets share only the pause, step and resume buttons | ADR 0023 as amended; LIB-10 AC-18 | 2026-11-10 |
| The ASM overlay and the heatmap are Calc-U-59 product kinds | `capabilities.md`, product kinds | 2026-11-10 |
| ASM overlays written for 2.3.0 are refused with the hint | ADR 0037; behaviour change 59-1 | 2026-11-10 |
| The ASM overlay is a file kind of "open file" | `frontend.md` SP-3; the index row | 2026-11-10 |
| "FREEZE" and "RESUME" stay as the panel's words | `docs/glossary-mapping.md` | 2026-11-10 |
| Proposed in this spec, for the gate: an `.asm` file is told by its name (AC-19) | LIB-13 AC-1 asks each file kind for its rule | 2026-11-10 |
