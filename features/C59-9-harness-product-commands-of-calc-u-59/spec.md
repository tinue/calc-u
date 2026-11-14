# C59-9: Harness product commands of Calc-U-59

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: LIB-11 (the harness, product commands under the product's
  name, emulated time), LIB-6 (loading a preset, idle), LIB-10 (the
  debug target and `profile`), LIB-8 and C59-6 (the paper of the
  PC-100C, for the timings of printed lines), LIB-5 (key names), C59-2
  (reading and writing the program).
- Contracts and decisions: `docs/contracts/harness.md` (H-1 to H-3, the
  fit table); LIB-11 AC-13, AC-14 and its OQ-4 and OQ-5 as decided.
- Facts: Calc-U-59 at `v2.3.0`; every `path:line` below is at that tag.
  No test there runs these tools; what they do is read from their code
  and from `tools/agentic/README.md`.

Words of this spec: a **keycode** is the two-digit code of one program
step (C59-2); a **mnemonic** is the name of a keycode, which is the name
of its key (LIB-5 AC-26; C59-1 AC-9); a **listing** is a keystroke
program written as one line per step, as the PC-100C's `List` prints
it (AC-9; `preset.md` §4.1); an **assembly** is a keystroke program
written as one line per instruction, mnemonics with their operands and
no keycodes, in the form of AC-14;
**normal mode** is the calculator when it is not in fast mode
(glossary). A time in **instructions** is emulated time in units of 32
ticks, the time of one instruction outside the ROM's idle state (LIB-11
AC-17). The machine is idle as LIB-6 AC-36 says.

## User Stories
- As a developer or an agent who tunes a keystroke program, I want to
  know how long each printed line and each key takes, which program
  steps the time goes to, and what a step costs.
- As a developer who studies the ROM, I want to know which parts of it a
  run reaches.
- As an agent, I want to read a program as a listing and paste it into
  a preset, and to read and write it as an assembly.

## Out of Scope
- Running a preset, the emulated time of a run and the printed lines
  (LIB-6; LIB-8 `paper text`); pressing keys by name (LIB-5); registers,
  memory, the record of instructions and the count of `profile`
  (LIB-10); writing and reading program steps and registers as keycodes
  (C59-2 AC-9); the ASM overlay and the dumps (C59-4).
- A dump of a magnetic card: a card file is its own dump, a text file
  in magnetic card format 3 (`docs/formats/media.md` §1; C59-7 AC-2).
- An assembler with labels and computed addresses (today
  `tools/agentic/asm59.py`, a library of the calendar scripts; OQ-2).
- The calendar scripts, which stay scripts (LIB-11 OQ-5 as decided).

## Acceptance Criteria

- [ ] **AC-1** — Given the harness with Calc-U-59, then the commands of
  this spec exist under `ti59`, next to those of C59-2 and C59-4, work
  on the TI-59, the TI-58 and the TI-58C, and build with the harness on
  macOS, Linux and Windows (LIB-11 AC-13, AC-15). Today the tools work
  for the TI-59 only and on macOS only
  (`tools/agentic/ti59file.py:20, 106`, `tools/headless/build.sh:10-12`).

ROM coverage
- [ ] **AC-2** — Given `profile on` (LIB-10 AC-20) and then `ti59
  coverage`, then the reply lists, in order and as hexadecimal numbers,
  the addresses of the calculator's own ROM, 0000 to 17FF, that were
  carried out since, and their number out of 6144; the addresses the
  ROM is at while the machine reports key readiness (C59-1 AC-18) are
  left out unless the word `all` follows. Without `profile on` it is
  refused (`tools/headless/coverage.py:217-261`).

Timings
- [ ] **AC-3** — Given `ti59 line-times`, optionally with `limit=` and a
  duration (4000 s without it), then the machine runs until it has been
  idle for 15 seconds in which nothing was printed, or until the limit.
  The reply gives, for each line printed meanwhile, the time from the
  start of the command, the time since the line before, and its text;
  then for the feeds and for the other lines each: how many, the mean,
  the least and the greatest of those gaps and their sum; then the least
  time the printer itself needs for that printout (AC-4); and whether
  the limit ended it (`tools/agentic/linetimes.py:16-46`).
- [ ] **AC-4** — Given a printout, then the least time the printer needs
  for it is 4165 instructions for each printed line and 1370 for each
  feed (`tools/agentic/ti59file.py:192-195`,
  `tools/agentic/README.md:69-70`). Both
  numbers come from measurements on a PC-100A (C59-6 AC-4).
- [ ] **AC-5** — Given `ti59 run-time` with key names, then each key is
  pressed in turn as the command `key` presses it (LIB-5 AC-15), and the
  reply gives for each the time from its press until the display is lit
  again and no program runs, 0 for a key that starts none, and their
  sum. A key whose program has not ended after 4000 seconds is marked
  as not finished in the reply, and the command ends there, done
  (`tools/agentic/runtime.py:18-32`, `tools/agentic/ti59file.py:116-145`).
- [ ] **AC-6** — Given `ti59 hot-steps`, optionally with `limit=` and a
  duration (200 s without it), then the machine runs for that long and
  the program step it is at is noted every 97 instructions. The reply
  gives the share of the notes in which the printer was printing, the
  share in which the ROM was waiting for the printer to finish, how
  many notes fell into a program of the calculator's memory, of the ROM
  and of the module, and the 25 program steps noted most often, each
  with its share (`tools/agentic/hotsteps.py:14-45`).
- [ ] **AC-7** — Given `ti59 step-trace` with two times, counted from
  the start of the command, the second after the first, then the machine
  runs to the second, and the reply gives the program steps it was at
  between the two, noted every 20 instructions, each with how often in a
  row (`tools/agentic/hotsteps.py:48-62`).
- [ ] **AC-8** — Given `ti59 step-cost` with keycodes (as C59-2 AC-9
  writes them), then the reply gives what one run of that sequence
  costs, in instructions and in milliseconds, in normal mode and in fast
  mode. The command uses the session's machine and leaves it changed: it
  writes a program of its own that runs the sequence 30 times from step
  008, with the registers 2.3.0's tool sets, runs it once in each mode,
  entering fast mode by the keys that tool uses, takes off the cost of
  the same program without the sequence, and divides by 30
  (`tools/agentic/stepcost.py:18-66`).

Listings
- [ ] **AC-9** — Given `ti59 keycodes list`, optionally with a first and
  a last step, then the reply is the program in the calculator's memory
  as a listing, as `2nd List` prints it on the PC-100C: one line per
  step, the step number in three digits, the keycode in two, then the
  mnemonic (`010 43 RCL`); it is the listing notation of
  `docs/formats/preset.md` §4.1 and can be pasted into a preset's
  `keycodes:` or a keystroke program file as it is; no printer is
  needed.
- [ ] **AC-14** — Given `ti59 keycodes disassemble`, optionally with a
  first and a last step, then the reply is the program in the
  calculator's memory as an assembly, one line per instruction: the step number in three
  digits, two spaces, the mnemonic, and after one space what the
  instruction takes: a register, a flag or a program number in two
  digits, a step address in three, a label as its mnemonic, each after
  `Ind` when it is indirect; an instruction that takes two of these has
  both on its line:

  ```
  000  Lbl A
  002  STO 05
  004  GTO 123
  007  Dsz Ind 03 A
  ```

  (`tools/agentic/kdis.py:20-58`).
- [ ] **AC-10** — Given `ti59 keycodes assemble` with a file that holds
  an assembly, then the keycodes it stands for are written into the
  program memory, each line from the step it names; a mnemonic that is
  unknown, an operand that is missing, or a step beyond the model's last
  one refuses the command, and nothing is written. What `ti59 keycodes
  disassemble` gives, assembled again, gives the same program; `ti59 program`
  (C59-2 AC-9) reads and writes the same memory as bare keycodes (OQ-2).
- [ ] **AC-11** — Given an assembly, then every mnemonic in it is a name
  that the command `key` takes for that key (LIB-5 AC-26; OQ-1): the
  names of `preset.md` §8.1, such as `Lbl`, `STF`, `IFF`.

Scripts that stay
- [ ] **AC-13** — Given `tools/headless/coverage.py`, with its scenarios
  and its views of what they reach, and
  `tools/agentic/fastmode_matrix.py`, then each stays a script in the
  source repository, drives the harness, and gives for the one case its
  documentation states what it gives today
  (`tools/headless/README.md:29-30`, `tools/agentic/README.md:158-171`;
  OQ-5).
- [ ] **AC-15** — Given a TI-58, TI-58C or TI-59, then it offers the
  machine state of LIB-11 AC-24: the processor, RAM, SCOM, the constant
  memory, the plugged PC-100C and Solid State ROM module, and a magnetic
  card in the reader with its position.

## Edge Cases
- **EC-2** — Given `ti59 line-times` and nothing is printed, then the
  reply says so, and the command ends after the first 15 seconds of
  idle.
- **EC-3** — Given `ti59 step-cost` with a sequence that never ends,
  then the command fails after 300 seconds of emulated time
  (`tools/agentic/README.md:239-243`).
- **EC-4** — Given `ti59 keycodes list` for an empty program, then the
  reply has no lines.
- **EC-5** — Given a TI-58C, whose instructions all take 32 ticks, then
  the times in instructions are ticks divided by 32 like on the other
  models.
- **EC-6** — Given `ti59 line-times` or `ti59 hot-steps` on a machine
  without a PC-100C, then `line-times` is refused, and `hot-steps`
  gives zero for the two shares of the printer.

## Technical Requirements (optional)
- The mnemonics of an assembly and the names of keys are one table of
  the product (AC-11).
- Each command of AC-2 to AC-10 and AC-14 has a test in the test suite, with a
  program small enough that its expected reply is written out in the
  test; today no test runs these tools.
- The commands are product commands: they are reached only with
  Calc-U-59 (`harness.md` H-2).

## Open Questions
- [x] **OQ-1** — *Decided as recommended (user, 2026-11-10, batch 8 gate).* The spelling of the mnemonics (AC-11). Three exist
  today: the key names of the headless tool, which LIB-5 AC-26 took
  (`Lbl`, `StFlg`, `IfFlg`); the table of the debug panel's listing
  (`LBL`, `STF`, `IFF`; `Shared/KeyNames.cpp:8-29`); and those of
  `kdis.py`. *Recommendation:* a listing of the harness uses the key
  names of LIB-5, so that one spelling holds in every command and
  preset; the debug panel keeps what it shows today. *Since 2026-11-13
  (user) those names are `preset.md` §8.1's, so `StFlg` and `IfFlg`
  become `STF` and `IFF`.*
- [x] **OQ-2** — *Decided as recommended (user, 2026-11-10, batch 8 gate).* What `ti59 keycodes assemble` reads (AC-10). Today's
  assembler is a library for scripts, with labels and computed
  addresses, and has no text form. *Recommendation:* the command reads
  what `ti59 keycodes list` writes, no more; the library with labels
  stays with the calendar scripts that use it.
- [x] **OQ-3** — *Decided as recommended (user, 2026-11-10, batch 8 gate).* The names of the commands. `harness.md` H-2 names `ti59
  keycodes` and `ti59 coverage`. *Recommendation:* as written here.
- [x] **OQ-4** — *Decided as recommended (user, 2026-11-10, batch 8 gate).* "Mnemonic" and "listing" for the glossary, as defined
  above. The glossary lists "listing" under Avoid for a program; here it
  is the written form of one.
- [x] **OQ-5** — *Decided as recommended (user, 2026-11-10, batch 8 gate).* LIB-11 AC-14 names the functions of `coverage.py` and
  `fastmode_matrix.py` as product commands. Their scenarios and tables
  are the data of one investigation each. *Recommendation:* only the
  addresses reached become a command (AC-2); the two scripts stay
  scripts (AC-13), and LIB-11 AC-14 is worded so.

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| What only one product's tools have becomes product commands under the product's name | `harness.md` H-2; LIB-11 AC-13, OQ-4 (user, batch 3 gate) | 2026-11-10 |
| Tools written for one investigation stay scripts | LIB-11 OQ-5 (user, batch 3 gate) | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 8 gate): coverage is read from the `profile` of LIB-10; there is no second counter (AC-2) | LIB-10 AC-20 already counts every address | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 8 gate): `step-cost` uses and changes the session's machine (AC-8) | A harness run has one machine (LIB-11 AC-2) | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 8 gate): every timing counts from the start of its command, and the limit is written `limit=` in each | One rule for the replies | 2026-11-10 |
| The mnemonics of a listing are the names of `preset.md` §8.1 (AC-11) | user, 2026-11-13 (question F-5 of `docs/walkthroughs/presets.md` §5.4); `docs/formats/preset.md` §8.1 | 2026-11-13 |
| Aligned with `docs/formats/` after the Q&A round of 2026-11-13: a card file is magnetic card format 3 (`media.md` §1) (Out of Scope) | user rulings of 2026-11-13 (`docs/walkthroughs/presets.md` §5.2, §5.4); `docs/formats/` | 2026-11-13 |
| A **listing** is the PC-100C's `List` form (step, keycode, mnemonic), which `ti59 keycodes list` gives and a preset takes; the instruction form is an **assembly**, given by `ti59 keycodes disassemble` and read by `assemble` (AC-9, AC-10, AC-11, AC-14) | user, 2026-11-13 (consistency pass): "this thing that the printer produces, and that I can copy / paste into the program section of a preset is a listing" | 2026-11-13 |
| The TI machines offer the machine state of LIB-11 AC-24 (AC-15) | user, 2026-11-14 (debugging and testing through the harness) | 2026-11-14 |
