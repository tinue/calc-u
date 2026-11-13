# C59-6: PC-100C and module slot as functional connectors

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: C59-1 (the TI machines behind the machine interface, golden
  runs), LIB-2 (attach and detach, pin-outs, controls).
- Contracts and decisions: `docs/contracts/peripherals.md` (P-1 to P-6,
  fit table), `docs/contracts/machine.md` (M-3, M-8),
  `docs/contracts/presets.md` (PR-9: a machine without a module),
  `docs/contracts/roms.md` (R-3: a peripheral whose ROM is missing);
  ADR 0010, 0011, 0012; `Briefing.md` §1.9;
  `docs/behaviour-changes.md` row 59-11.
- Facts: Calc-U-59 at `v2.3.0`; every `path:line` below is at that tag.

In this spec "the calculator" is the real device; the running thing is
"the machine" (glossary).

## User Stories
- As a user, I want the PC-100C to print, feed and trace exactly as
  today, and my Solid State ROM modules to run as today.
- As a user, I want to run a TI-59 with an empty module slot, as the
  real calculator allows.
- As the developer, I want the printer and the module to be peripherals
  plugged into the machine, each with its connector written down, so
  that either can later be modelled at contact level without touching
  anything else (`Briefing.md` §1.9).

## Out of Scope
- The paper as the library and the user see it: the paper output kind,
  marks, the text layer, the paper panel, Copy and Cut (owner: LIB-8,
  whose Calc-U-59 column takes the PC-100C's paper). This feature checks
  what the printer prints inside the core (AC-3).
- The picker for the module in the control bar and the control that
  connects the printer (owners: LIB-4, LIB-7, LIB-14, C59-8; behaviour
  change 59-9).
- The attach and detach commands, the power cycle that follows, and that
  changing a module is one step with one power cycle (owner: LIB-18;
  behaviour change 59-11). What a power cycle does to the machine's
  memory: C59-1.
- The machine the product starts with, and what the start does when a
  ROM file is missing (owners: LIB-18, LIB-12; behaviour change 59-10).
- The descriptors of the PC-100C and the modules (owner: LIB-1,
  Calc-U-59 column, AC-31 and AC-32 there), and finding their ROM files
  (LIB-12).
- The cue card of the running module program, a Calc-U-59 product kind,
  and `fit` (owner: C59-7).
- Modelling either connector at contact level, and which of the 12
  springs of the PC-100C is unconnected (`Briefing.md` §6).
- The `switch` step for TRACE in presets (owner: LIB-6).

## Acceptance Criteria

The PC-100C
- [ ] **AC-1** — Given a TI machine, then a PC-100C can be plugged into
  the place `printer` and removed again; a machine without one has no
  paper output and no controls of a printer. The paper the PC-100C
  printed is kept by the app session and is back when a PC-100C is
  attached again (LIB-8 AC-8, AC-12).
- [ ] **AC-2** — Given a PC-100C is plugged in, then the machine's ROM
  finds a printer as on the real device (today: the printer line at D0
  on the TI-59 and TI-58, at D10 on the TI-58C,
  `Core/TMC0501.cpp:1284-1295`); given none is plugged in, then it finds
  none.
- [ ] **AC-3** — Given a golden run of C59-1 that prints, when it is
  played on a machine with a PC-100C, then the lines the printer prints,
  their characters and their dots are the recorded ones, read by the
  test at the printer inside the core
  (`Core/TMC0501.cpp:27-99, 758-843`).
- [ ] **AC-4** — Given the printer prints or feeds, then it is busy for
  as many instructions of the machine as today: 4165 for a line, 1370
  for a feed, with rows and columns timed as today
  (`Core/TMC0501.hpp:246-247, 554-556`,
  `reference/CoreArchitecture.md:722-830`); a line cut short by the
  printer interrupt has the same partial row as today
  (`tests/core_tests.cpp`: printer-interrupt partial rows). The timings
  of today's code come from measurements on a PC-100A, not on a
  PC-100C; no difference between the two is expected (user, 2026-11-11).
- [ ] **AC-5** — Given a PC-100C is plugged in, then it has the controls
  `PRINT` and `ADV`, which are keys, and `TRACE`, which is a switch
  (`Core/TI59Machine.cpp:198-212`; today's headless tool calls ADV
  "PAPER", `tools/headless/ti59.py:57`):
  `PRINT` held down prints the display, as today
  (`tests/core_tests.cpp`: the PRINT button emits a line);
  `ADV` held down feeds paper for as long as it is held;
  `TRACE` on makes the machine print as it calculates, until it is set
  off.
- [ ] **AC-6** — Given `TRACE` is on, when the machine goes through a
  reset, a power cycle or a cold boot, then `TRACE` is still on; when
  the PC-100C is detached and attached again, then `TRACE` is off,
  unlike the real switch, which keeps its position; the help says so
  (`Core/TI59Machine.cpp:44, 214-221`; OQ-3).
- [ ] **AC-7** — Given a machine without a PC-100C and a golden run in
  which a program executes print instructions, then the display output
  and the tick counts are the recorded ones: a program takes as long
  without a printer as it does in 2.3.0 (OQ-2).

The module slot
- [ ] **AC-8** — Given a TI machine, then one of these 14 Solid State
  ROM modules can be plugged into the place `slot-1`, named by its ID,
  the `module-name` of its module file (`docs/formats/modules.md` §2;
  the files come from the ROM repository, LIB-12 AC-14). A preset names
  it by `slot-1: { name: <ID> }`; `{ file: … }` is refused with a notice
  that this release takes a module by its ID only:

  | ID | Module | ROM |
  |---|---|---|
  | `ML` | Master Library (1) | TMC0541 |
  | `ST` | Applied Statistics (2) | TMC0542 |
  | `RE` | Real Estate / Investment (3) | TMC0543 |
  | `SY` | Surveying (4) | TMC0544 |
  | `NG` | Marine Navigation (5) | TMC0545 |
  | `AV` | Aviation (6) | TMC0546 |
  | `LE` | Leisure Library (7) | TMC0547 |
  | `SA` | Securities Analysis (8) | TMC0548 |
  | `BD` | Business Decisions (9) | TMC0549 |
  | `MU` | Math / Utilities (10) | TMC0550 |
  | `EE` | Electrical Engineering (11) | TMC0551 |
  | `SE` | Structural Engineering | TMC0553 |
  | `AG` | Agriculture (12) | TMC0554 |
  | `RP` | RPN Simulator (13) | TMC0555 |

- [ ] **AC-9** — Given a module is plugged in and a golden run that
  calls one of its programs by keys, then the display output, the memory
  and the printed lines are the recorded ones; there is such a run for
  the Master Library and for at least one other module.
- [ ] **AC-10** — Given the place `slot-1` is empty, then the machine
  runs, and its ROM finds no module: a library program that is asked
  for, as with `2nd Pgm 01`, makes the display blink as for an error,
  as `2nd Pgm 30` does with the Master Library module, which has no
  program 30 (user, 2026-11-11, from the real machine; `presets.md` PR-9; OQ-1).
  Today a
  missing module leaves an image of zeros and a line in the log
  (`Shared/SessionController.cpp:129-130`).
- [ ] **AC-11** — Given a module was changed, or the PC-100C attached or
  detached, and the power cycle that follows is over (LIB-2 AC-24), then
  the ROM finds what is plugged in now. Today the module is swapped
  under the running machine (`Shared/SessionController.cpp:220-241`).

## Edge Cases
- **EC-1** — Given a module whose ROM file is not handed in, when it is
  attached, then the attach is refused and the file is named (LIB-2
  EC-4; `roms.md` R-3: "a peripheral whose ROM is missing cannot be
  attached"). Today the machine starts without the module's data. With
  the full ROM set required at the start (LIB-12 AC-5), a running
  product does not meet this.
- **EC-2** — Given the PC-100C is detached while it prints a line, then
  the rows already printed stay on the paper, which is kept for the run
  of the product (LIB-8 AC-8), the rest of the line is not printed, and
  the power cycle that follows a detach runs.
- **EC-3** — Given the TI-58C, then the ROM's key handling differs with
  and without a printer as today (`Shared/MachineModelInfo.hpp:36-38`);
  key readiness takes this into account (LIB-5).

## Technical Requirements (optional)
- The CPU component holds no printer state and no module image after
  this feature; the PC-100C and a module see only the signals of their
  connector (`peripherals.md` I-1, I-2). The descriptors say so (LIB-1
  AC-31, AC-32).
- The printer connector's pin-out is functional and names the signals
  of the fit table in `peripherals.md`: Busy (KR), KP, KN, Φ1, D0, D15,
  IRG, IDLE, EXT, Φ2, D12, I/A. The module slot's pin-out is functional
  and names: VGG, VDD, instruction words, calculating status, external
  access, Φ2, Φ1, common (OQ-5).
- The printer's busy times are counted in instructions of the machine,
  and an instruction in the ROM's idle state counts once although it
  takes four times the ticks (AC-4; today `Core/TMC0501.cpp:472-478`).
  This reads `peripherals.md` P-2 so: a peripheral is passed the host's
  ticks, and a functional pin-out may also carry the signal that marks
  an instruction.
- The golden runs for AC-3, AC-7, AC-9 and AC-10 (an empty slot and
  `2nd Pgm 01`) are recorded with those of
  C59-1, from the code as imported, and also hold the printed lines and
  dots.

## Open Questions
- [x] **OQ-1** — *Decided (user, 2026-11-11): the real machine error-blinks, the same as for `2nd Pgm 30` on the Master Library module, which does not have 30 programs. A golden run records it.* What the real ROM does with an empty module slot
  (AC-10): for example on `2nd Pgm 01`. Today's code has no test for it
  and the research notes do not say. *Recommendation:* whatever the
  original ROM does when it reads no module is right by definition; try
  it on a real TI-59 once, record the result in `ti59-research`, and
  add a golden run.
- [x] **OQ-2** — *Decided (user, 2026-11-11): this is the ROM's job, and the emulator must not interfere. AC-7 holds as written, checked by a golden run against 2.3.0.* Print instructions without a printer (AC-7). Today the
  printer's logic and its busy time sit in the CPU and do not ask
  whether a printer is connected; whether the ROM ever executes a print
  instruction without one is not verified. If it does, the busy time
  must still pass after the printer has moved out, or programs would run
  faster than in 2.3.0. *Recommendation:* verify on 2.3.0 with a program
  that prints; AC-7 holds either way, and the design follows the result.
- [x] **OQ-3** — *Decided as recommended (user, 2026-11-11).* `TRACE` after a detach (AC-6): the real switch keeps
  its position when the calculator is lifted off the cradle. Today's
  code releases it. *Recommendation:* keep today's behaviour, because in
  the emulator a detached printer does not exist; say so in the help.
- [x] **OQ-4** — *Decided as recommended (user, 2026-11-11).* The name of a module in a preset and in the harness:
  `presets.md` PR-9 says "a module by name or ID". *Recommendation:* the
  two-letter ID of AC-8 is the name; the title is for the picker.
- [x] **OQ-5** — *Decided as recommended (user, 2026-11-11).* The fit table names 12 signals on the printer
  connector, and `Briefing.md` §1.9 says the calculator has 11 pads.
  Which of the 12 is not on the connector is the open point of
  `Briefing.md` §6. *Recommendation:* leave the list as it is until the
  connector is modelled at contact level; a functional pin-out is not
  harmed by one name too many.

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| The PC-100C printer connector and the module slot become functional connectors in the migration | ADR 0011 | 2026-11-09 |
| Peripherals are keyed by connector and see only its signals | ADR 0010; `peripherals.md` I-1 | 2026-11-09 |
| The keys of the PC-100C belong to the printer, not to the machine | `machine.md` M-3 | 2026-11-10 |
| A module change and connecting or disconnecting the printer are followed by a power cycle | `peripherals.md` P-1 (user); behaviour change 59-11 | 2026-11-10 |
| A TI machine without a module is allowed | `presets.md` PR-9 | 2026-11-10 |
| A peripheral whose ROM is missing cannot be attached | `roms.md` R-3 | 2026-11-10 |
| It is an internal restructuring: what the user sees stays, except the listed changes | `Briefing.md` §1.9 | 2026-11-09 |
| Proposed in this spec and accepted (user, 2026-11-11): the control IDs `PRINT`, `ADV`, `TRACE`; what a detach in the middle of a line leaves (EC-2); the reading of P-2 for the printer's busy time | `presets.md` PR-7 writes `TRACE`; no source gives EC-2; P-2 speaks of ticks only | 2026-11-10 |
| With an empty module slot a library program that is asked for makes the display blink as for an error (AC-10) | user, 2026-11-11, from the real machine | 2026-11-11 |
| What a print instruction does without a printer is the ROM's; the emulator adds nothing of its own (AC-7) | user, 2026-11-11 | 2026-11-11 |
| A preset and the harness name a module by its two-letter ID only; `TRACE` is released by a detach, as today; the fit table keeps its 12 signals until the connector is modelled at contact level | user, 2026-11-11 | 2026-11-11 |
| Every measurement of the printer's timing was made on a PC-100A, not on a PC-100C; no difference is expected, and the criteria keep today's numbers (AC-4) | user, 2026-11-11 | 2026-11-11 |
| The modules come from `.rom.yaml` files of the ROM repository; a preset names one by `name:` only, `file:` is refused in this release (AC-8) | user, 2026-11-13 (question F-9 of `docs/walkthroughs/presets.md` §5.4); `docs/formats/modules.md` | 2026-11-13 |
