# C59-6: PC-100C and module slot as functional connectors

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: C59-1 (the TI machines behind the machine interface), LIB-2
  (attach and detach, pin-outs, controls).
- Contracts and decisions: `docs/contracts/peripherals.md` (P-1 to P-6,
  fit table), `docs/contracts/machine.md` (M-3, M-8),
  `docs/contracts/presets.md` (PR-9: a machine without a module);
  ADR 0010, 0011, 0012; `Briefing.md` §1.9;
  `docs/behaviour-changes.md` row 59-11.
- Facts: Calc-U-59 at `v2.3.0`; every `path:line` below is at that tag.

## User Stories
- As a user, I want the PC-100C to print, feed and trace exactly as
  today, and my Solid State ROM modules to run as today.
- As a user, I want to run a TI-59 with an empty module slot, as the
  real calculator allows.
- As the developer, I want the printer and the module to be peripherals
  plugged into the calculator, each with its connector written down, so
  that either can later be modelled at contact level without touching
  anything else (`Briefing.md` §1.9).

## Out of Scope
- The paper: marks, the text layer, the paper panel, Copy and Cut
  (owner: LIB-8, whose Calc-U-59 column takes the PC-100C's paper
  output).
- The picker for the module in the control bar and the control that
  connects the printer (owners: LIB-4, LIB-7, LIB-14, C59-8; behaviour
  change 59-9).
- The attach and detach commands and the power cycle that follows
  (owner: LIB-18; behaviour change 59-11). What a power cycle does to
  the calculator's memory: C59-1.
- The machine the product starts with (owner: LIB-18; behaviour change
  59-10).
- The descriptors of the PC-100C and the modules (owner: LIB-1,
  Calc-U-59 column), and finding their ROM files (LIB-12).
- The cue card of a module's program and `cuecards.txt` (carried over;
  `fit`: C59-7).
- Modelling either connector at contact level, and which of the 12
  springs of the PC-100C is unconnected (`Briefing.md` §6).
- The `switch` step for TRACE in presets (owner: LIB-6).

## Acceptance Criteria

The PC-100C
- [ ] **AC-1** — Given a TI machine, then a PC-100C can be plugged into
  the place `printer` and removed again; nothing of the printer exists
  in a machine without one: no paper output, no controls.
- [ ] **AC-2** — Given a PC-100C is plugged in, then the calculator's
  ROM finds a printer as on the real device (today: the printer line at
  D0 on the TI-59 and TI-58, at D10 on the TI-58C,
  `Core/TMC0501.cpp:1284-1295`); given none is plugged in, then it finds
  none.
- [ ] **AC-3** — Given a program or a key that prints, when a PC-100C is
  plugged in, then the lines it prints, their characters and their dots
  are the same as in Calc-U-59 2.3.0 for the same keys and program
  (`Core/TMC0501.cpp:27-99, 758-843`).
- [ ] **AC-4** — Given the printer prints or feeds, then it is busy for
  as many instruction cycles of the calculator as today: 4165 for a
  line, 1370 for a feed, with rows and columns timed as today
  (`Core/TMC0501.hpp:246-247, 554-556`,
  `reference/CoreArchitecture.md:722-830`); a line cut short by the
  printer interrupt has the same partial row as today
  (`tests/core_tests.cpp`: printer-interrupt partial rows).
- [ ] **AC-5** — Given a PC-100C is plugged in, then it has the controls
  PRINT and ADV, which are keys, and TRACE, which is a switch
  (`Core/TI59Machine.cpp:198-212`):
  PRINT held down prints the display, as today
  (`tests/core_tests.cpp`: the PRINT button emits a line);
  ADV held down feeds paper for as long as it is held;
  TRACE on makes the calculator print as it calculates, until it is set
  off.
- [ ] **AC-6** — Given TRACE is on, when the calculator goes through a
  reset, a power cycle or a cold boot, then TRACE is still on; when the
  PC-100C is detached and attached again, then TRACE is off
  (`Core/TI59Machine.cpp:44, 214-221`).

The module slot
- [ ] **AC-7** — Given a TI machine, then one of these 14 Solid State
  ROM modules can be plugged into the place `slot-1`, named by its ID
  (`roms/solid-state/cuecards.txt`):

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

- [ ] **AC-8** — Given a module is plugged in, then the calculator runs
  its programs exactly as today: for every example of Calc-U-59 that
  calls a module program, the display output and the paper are the same
  as in 2.3.0.
- [ ] **AC-9** — Given the place `slot-1` is empty, then the machine
  runs, and its ROM finds no module (`presets.md` PR-9). Today a missing
  module leaves an image of zeros and a line in the log
  (`Shared/SessionController.cpp:129-130`).
- [ ] **AC-10** — Given a module is changed, or the PC-100C is attached
  or detached, then both pin-outs say that a power cycle follows
  (LIB-2 AC-19), and after it the ROM finds what is plugged in now.
  Today the module is swapped under the running calculator
  (`Shared/SessionController.cpp:220-241`).

The connectors
- [ ] **AC-11** — Given the printer connector, then its pin-out is
  functional and names these signals: Busy (KR), KP, KN, Φ1, D0, D15,
  IRG, IDLE, EXT, Φ2, D12, I/A; given the module slot, then its pin-out
  is functional and names: VGG, VDD, instruction words, calculating
  status, external access, Φ2, Φ1, common (`peripherals.md`, fit table).
- [ ] **AC-12** — Given the descriptors after this feature, then they
  list the PC-100C and each module as peripherals with components of
  their own, and the CPU component no longer lists the printer or the
  module as folded into it (ADR 0011, 0012).

## Edge Cases
- **EC-1** — Given a module of `slot-1` whose ROM file is not handed in,
  when it is attached, then the attach is refused and the file is named
  (LIB-2). Today the calculator starts without the module's data.
- **EC-2** — Given a printer is detached while it prints a line, then
  the line is lost and the calculator goes through the power cycle that
  follows a detach.
- **EC-3** — Given no PC-100C is plugged in, when a program executes a
  print instruction, then nothing is printed and the program goes on as
  on the real calculator without a printer (OQ-2).
- **EC-4** — Given the TI-58C, then the key scan differs with and
  without a printer as today (`Shared/MachineModelInfo.hpp:36-38`); key
  readiness takes this into account (LIB-5).

## Technical Requirements (optional)
- The CPU component holds no printer state and no module image after
  this feature; the PC-100C and a module see only the signals of their
  connector (`peripherals.md` I-1, I-2).
- The printer's busy times are counted in the calculator's instruction
  cycles, which the pin-out carries (Φ1, Φ2, IDLE); an instruction in
  IDLE counts once although it takes four times the ticks (AC-4; today
  `Core/TMC0501.cpp:472-478`).
- Before the core is touched, the paper of every shipped example that
  prints is recorded from 2.3.0, lines and dots, as golden files for
  AC-3, AC-4 and AC-8.

## Open Questions
- [ ] **OQ-1** — What the real ROM does with an empty module slot
  (AC-9): for example on `2nd Pgm 01`. Today's code has no test for it
  and the research notes do not say. *Recommendation:* whatever the
  original ROM does when it reads no module is right by definition; try
  it on a real TI-59 once, and record the result in `ti59-research`.
- [ ] **OQ-2** — Does the ROM execute print instructions when no printer
  is present (EC-3)? Today the core does not ask whether a printer is
  connected before it prints into its buffer, so the frontends could
  receive lines without a printer; whether the ROM ever gets there is
  not verified. *Recommendation:* verify on 2.3.0; the rule stays that
  without a PC-100C nothing is printed.
- [ ] **OQ-3** — TRACE after a detach (AC-6): the real switch keeps its
  position when the calculator is lifted off the cradle. Today's code
  releases it. *Recommendation:* keep today's behaviour, because in the
  emulator a detached printer does not exist; say so in the help.
- [ ] **OQ-4** — The name of a module in a preset and in the harness:
  `presets.md` PR-9 says "a module by name or ID". *Recommendation:* the
  two-letter ID of AC-7 is the name; the title is for the picker.

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| The PC-100C printer connector and the module slot become functional connectors in the migration | ADR 0011 | 2026-11-09 |
| Peripherals are keyed by connector and see only its signals | ADR 0010; `peripherals.md` I-1 | 2026-11-09 |
| The keys of the PC-100C belong to the printer, not to the calculator | `machine.md` M-3 | 2026-11-10 |
| A module change and connecting or disconnecting the printer are followed by a power cycle | `peripherals.md` P-1 (user); behaviour change 59-11 | 2026-11-10 |
| A TI machine without a module is allowed | `presets.md` PR-9 | 2026-11-10 |
| It is an internal restructuring: what the user sees stays, except the listed changes | `Briefing.md` §1.9 | 2026-11-09 |
