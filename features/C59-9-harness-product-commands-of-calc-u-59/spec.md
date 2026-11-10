# C59-9: Harness product commands of Calc-U-59

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: LIB-11 (the harness, product commands under the product's
  name, emulated time), LIB-6 (loading a preset, `elapsed`), LIB-10
  (the debug target, `profile`, the history); also named here: LIB-5
  (key names), LIB-8 (the paper), C59-2 (reading and writing the
  program), C59-4 (the names of keycodes in the debug panel).
- Contracts and decisions: `docs/contracts/harness.md` (H-1 to H-3);
  LIB-11 AC-13, AC-14 and its OQ-4 and OQ-5 as decided.
- Facts: Calc-U-59 at `v2.3.0`; every `path:line` below is at that tag.
  No test there runs these tools; what they do is read from their code
  and from `tools/agentic/README.md`.

Words of this spec: a **keycode** is the two-digit code of one program
step (C59-2); a **mnemonic** is the short name of a keycode (`RCL`,
`Lbl`); a **listing** is a keystroke program written as one line per
instruction, with step numbers and mnemonics. Times are emulated time
as LIB-11 AC-17 counts it.

## User Stories
- As a developer or an agent who tunes a keystroke program, I want to
  know how long each printed line and each key takes, which program
  steps the time goes to, and what a step costs.
- As a developer who studies the ROM, I want to know which parts of it a
  run reaches.
- As an agent, I want to read a program as a listing and to write one
  back.

## Out of Scope
- Running a preset, the emulated time of a run and the printout (LIB-6,
  LIB-8); pressing keys by name (LIB-5); registers, memory and the
  record of instructions (LIB-10); writing and reading program steps and
  registers (C59-2); the ASM overlay and the dumps (C59-4).
- Tools that were written for one investigation and stay scripts in the
  source repository, driving the harness: the scenarios of
  `tools/headless/coverage.py` and its views of their results,
  `tools/agentic/fastmode_matrix.py`, and the calendar tools (LIB-11
  OQ-5 as decided).
- An assembler with labels and computed addresses (today
  `tools/agentic/asm59.py`, a library of the calendar tools; OQ-2).

## Acceptance Criteria

- [ ] **AC-1** — Given the harness with Calc-U-59, then the commands of
  this spec exist under `ti59`, work on the TI-59, the TI-58 and the
  TI-58C, and build with the harness on macOS, Linux and Windows
  (LIB-11 AC-13, AC-15). Today the tools work for the TI-59 only and on
  macOS only (`tools/agentic/ti59file.py:20, 106`,
  `tools/headless/build.sh:10-12`).

ROM coverage
- [ ] **AC-2** — Given `ti59 coverage on`, then from then on the ROM
  addresses that are carried out are noted; given `ti59 coverage
  report`, then the reply lists them in order, as hexadecimal addresses,
  and their number out of the ROM's size, without the addresses of the
  ROM's idle state unless `all` is added; given `ti59 coverage off`,
  then noting ends and the notes are dropped
  (`tools/headless/coverage.py:217-261`).

Timings
- [ ] **AC-3** — Given `ti59 line-times`, optionally with `limit=` and a
  duration (4000 s without it), then the machine runs until 15 seconds
  after the last printed line with the calculator waiting for a key, or
  until the limit; the reply gives, for each line printed meanwhile, the
  time it appeared, the time since the line before and its text, and for
  the blank lines and the others each: how many, the mean, the least and
  the greatest gap and their sum; and the least time the printer itself
  needs for that printout (AC-4) (`tools/agentic/linetimes.py:16-46`).
- [ ] **AC-4** — Given a printout, then the least time the printer needs
  for it is 4165 instructions for each printed line and 1370 for each
  blank one (`tools/agentic/ti59file.py:192-195`,
  `tools/agentic/README.md:69-70`).
- [ ] **AC-5** — Given `ti59 run-time` with key names (LIB-5 AC-26),
  then each key is pressed in turn, and the reply gives for each the
  time from its press until the display is lit again and no program
  runs, 0 for a key that starts none, and their sum; a key whose program
  has not ended after 4000 seconds is reported as not finished, and the
  command ends there (`tools/agentic/runtime.py:18-32`,
  `tools/agentic/ti59file.py:116-145`).
- [ ] **AC-6** — Given `ti59 hot-steps`, optionally with `until=` and a
  duration (200 s without it), then the machine runs for that long and
  the program step it is at is noted every 97 instructions; the reply
  gives the share of the notes in which the printer was busy, the share
  in which the ROM waited for the printer, where the program came from
  (the calculator's memory, the ROM, the module), and the 25 program
  steps noted most often, each with its share
  (`tools/agentic/hotsteps.py:14-45`).
- [ ] **AC-7** — Given `ti59 step-trace` with two times, then the
  machine runs to the second, and the reply gives the program steps it
  was at between the two, noted every 20 instructions, each with how
  often in a row (`tools/agentic/hotsteps.py:48-62`).
- [ ] **AC-8** — Given `ti59 step-cost` with a sequence of keycodes,
  then the reply gives what the sequence costs, in instructions and in
  milliseconds, in normal mode and in fast mode: the sequence is run 30
  times as a program from step 008, and the cost of the same program
  without it is taken off. The machine is built anew for the command
  and is not the one that ran before
  (`tools/agentic/stepcost.py:18-66`).

Listings
- [ ] **AC-9** — Given `ti59 list`, optionally with a first and a last
  step, then the reply is the program in the calculator's memory as a
  listing: one line per instruction with its step number and its
  mnemonic, an instruction that takes a register, a flag, a label or an
  address shown with it on its line, an indirect one marked
  (`tools/agentic/kdis.py:20-58`).
- [ ] **AC-10** — Given `ti59 assemble` with a file that holds a
  listing, then the keycodes it stands for are written into the program
  memory from the step each line names, or one after the other where a
  line names none; a mnemonic that is unknown or an operand that is
  missing refuses the command, and nothing is written. What `ti59 list`
  gives, assembled again, gives the same program (OQ-2).
- [ ] **AC-11** — Given the mnemonics of AC-9 and AC-10, then they are
  one table for the product, the one the debug panel's listing uses
  (C59-4 AC-4). Today the two tools each have a table of their own, and
  the product a third with other spellings
  (`tools/agentic/kdis.py:10-19`, `tools/agentic/asm59.py:7-12`,
  `Shared/KeyNames.cpp:8-29`; OQ-1).

Checks
- [ ] **AC-12** — Given each command of AC-2 to AC-10, then the test
  suite has a test for it with a short program whose answer is known;
  today no test runs these tools.
- [ ] **AC-13** — Given the scripts that stay (Out of Scope), then each
  runs against the harness and gives what it gives today for one case
  that its documentation states (`tools/agentic/README.md:63-64, 89-171`).

## Edge Cases
- **EC-1** — Given `ti59 coverage report` without `on` before it, then
  it is refused.
- **EC-2** — Given `ti59 line-times` and nothing is printed, then the
  reply says so, and the command ends at once when the calculator waits
  for a key.
- **EC-3** — Given `ti59 step-cost` with a sequence that never ends,
  then the command fails after 300 seconds of emulated time
  (`tools/agentic/README.md:239-243`).
- **EC-4** — Given `ti59 list` for an empty program, then the reply has
  no lines.
- **EC-5** — Given a TI-58 or TI-58C, then the commands count with that
  model's idle state and its number of steps (LIB-11 AC-17).

## Technical Requirements (optional)
- The coverage of AC-2 and the notes of AC-6 are taken through the
  debug target and the `profile` of LIB-10; the commands add what only
  the TI knows (the idle state, the program step, the printer).
- The commands are product commands: they are reached only with
  Calc-U-59 (`harness.md` H-2).

## Open Questions
- [ ] **OQ-1** — The spelling of the mnemonics (AC-11): the tools write
  `Lbl`, `StFlg`, `IfFlg`, the product's table `LBL`, `STF`, `IFF`.
  *Recommendation:* the product's table, so that the panel, the listing
  and the assembler agree; the scripts that stay are changed with it.
- [ ] **OQ-2** — What `ti59 assemble` reads (AC-10). Today's assembler
  is a library for scripts, with labels and computed addresses, and has
  no text form. *Recommendation:* the command reads what `ti59 list`
  writes, no more; the library with labels stays with the calendar
  scripts that use it.
- [ ] **OQ-3** — The names of the commands of this spec.
  *Recommendation:* as written.
- [ ] **OQ-4** — "Mnemonic" and "listing" for the glossary, as defined
  above. The glossary lists "listing" under Avoid for a program; here it
  is the written form of one.

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| What only one product's tools have becomes product commands under the product's name | `harness.md` H-2; LIB-11 AC-13, OQ-4 (user, batch 3 gate) | 2026-11-10 |
| Tools written for one investigation stay scripts | LIB-11 OQ-5 (user, batch 3 gate) | 2026-11-10 |
| Proposed in this spec, for the gate: the scenarios of `coverage.py` and `fastmode_matrix.py` are such scripts; the command gives only the addresses reached (AC-2) | Their scenarios and tables are data of one investigation | 2026-11-10 |
| Proposed in this spec, for the gate: `step-cost` builds a machine of its own (AC-8) | It needs an empty program and fixed registers, as today | 2026-11-10 |
