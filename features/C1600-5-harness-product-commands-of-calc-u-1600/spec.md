# C1600-5: Harness product commands of Calc-U-1600

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: LIB-11 (the harness, product commands under the product's
  name), LIB-7 (cassette tapes), LIB-10 (reading memory); also named
  here: C1600-1 (the product kinds for the INSPECTOR views and for the
  CE-158's printer port), LIB-2 (the serial port kind and its host
  link), LIB-8 (the paper).
- Contracts and decisions: `docs/contracts/harness.md` (H-1 to H-3);
  LIB-11 AC-13, AC-18 and its OQ-4 and OQ-5 as decided.
- Facts: Calc-U-1600 at `b4ea8c8`; every `path:line` below is at that
  commit. No test there runs these functions of the two command-line
  tools; what they do is read from their code.

Words of this spec: a **view** is one of the texts the INSPECTOR shows
(C1600-1 AC-19); the **peer** of a serial port is what stands at its
other end: a file of bytes to receive, a file that takes what is sent,
or a pseudo-terminal of the host.

## User Stories
- As a developer who checks a BASIC loader, I want the pointers of BASIC
  and the bytes of the program area as text.
- As a developer or an agent, I want any INSPECTOR view as text, without
  a window.
- As a developer who checks cassette loading and saving, I want to play
  a WAV file into the machine and to record one from it.
- As a developer who checks the CE-158, I want to feed its serial port
  from a file and to see what it sends and prints.

## Out of Scope
- Loading a preset, checking presets, waiting, the display as text and
  as a picture, the buzzer as a WAV (LIB-6, LIB-11); memory as a hex
  dump, registers, the last instructions and where time was spent
  (LIB-10); the model's variants (LIB-18); the paper of the plotters
  (LIB-8); the loaders (C1600-2).
- The three probes, which stay scripts (LIB-11 OQ-5 as decided).
- The PC-1600's own RS-232C port beyond AC-9 (OQ-1).

## Acceptance Criteria

- [ ] **AC-1** — Given the harness with Calc-U-1600, then the commands
  of this spec exist under `pc1600`, for the PC-1500, the PC-1500A and
  the PC-1600 (LIB-11 AC-13, AC-20).

BASIC
- [ ] **AC-2** — Given `pc1600 basic-dump`, then the reply gives the
  pointers of BASIC, each with its name, its address and its value, and
  then the program area from its start to 16 bytes past its end, in
  rows of 16 bytes with the address before them and the same bytes as
  characters after them. On a PC-1500 and PC-1500A the pointers are the
  start, the end (with the length) and the edit pointer of the program,
  the start of the variables, the data pointer, the end of the RAM and
  the warm-start mark; on a PC-1600 the start, the end (with the
  length) and the edit pointer, the base of the RAM, the pointer at
  F89D and the pointer of the variables, each with the Z-80's address
  for it (`tools/pc1500_cli.cpp:342-373`, `tools/pc1600_cli.cpp:200-237`).
- [ ] **AC-3** — Given the lines of that reply, then their form is
  today's, so that what reads them today still can:
  `BASPRG_END $7867 = $40D2   (len = 13)` for a pointer and
  `40C5: 00 0A … |…|` for a row (`dev/tape-matrix/run.py:99-105`).

INSPECTOR views
- [ ] **AC-4** — Given `pc1600 inspect` with the name of a view, then
  the reply is that view as the INSPECTOR shows it, as text: `pointers`,
  `inventory`, `z80`, `lh`, `basic`, `programs`, `disks`, `physical`,
  `dump-basic`, `dump-s1`, `dump-s2`, `dump-disk-s1`, `dump-disk-s2`,
  `dump-ml`. A name that is none of these is refused; a view the model
  does not have gives one line that says so
  (`Core/Debug/Inspect/Inspector.cpp:35-53`,
  `Core/Debug/Inspect/PC1500Inspector.cpp:365-388`,
  `Core/tests/inspector_tests.cpp:97-250`).

Cassette files
- [ ] **AC-5** — Given `pc1600 tape play` with the path of a WAV file,
  then that recording is in the cassette recorder as a tape to play,
  from its start; given `pc1600 tape record` with a path, then a blank
  tape is in the recorder, and what the machine saves is written to
  that file as LIB-7 AC-42 says, each time the motor stops and when the
  tape is taken out; given `pc1600 tape eject`, then the recorder is
  empty. One tape is in the recorder at a time; putting one in takes
  the other out. The machine needs the peripheral that has the cassette
  (`tools/CliCommon.hpp:56-95`, `Core/Tape/TapeDeck.hpp:55-133`).
- [ ] **AC-6** — Given `pc1600 tape status`, then the reply gives where
  the tape is and how long it is, in seconds, and whether the motor
  runs; and it gives the error of a save that failed, which today is
  not told (`tools/CliCommon.hpp:84-95`).

The CE-158
- [ ] **AC-7** — Given a CE-158 and `pc1600 ce158 feed` with the path of
  a file, optionally with `hold=` and a number, then the file's bytes
  are the ones its serial port receives, one for each character time
  and only while the port has taken the one before; with `hold` the
  first so many times the port asks, nothing comes. Given `pc1600 ce158
  capture` with a path, then the bytes the port sends are written to
  that file at the end of the run. Given `pc1600 ce158 pty`, then the
  port is linked to a pseudo-terminal of the host, and the reply gives
  its path (`tools/Ce158CliPeer.hpp:18-94`,
  `Core/Connector/Ce158Card.hpp:145-170`; LIB-2, the host link).
- [ ] **AC-8** — Given `pc1600 ce158 report`, then the reply gives the
  speed the port is set to and its state, the bytes sent to the
  printer's port as text, how many of the bytes to receive were taken,
  and the bytes the serial port sent as text; in such a text a line end
  and a character of ASCII that can be printed stand as they are, a
  carriage return is left out, and any other byte is written as `<XX>`
  (`tools/Ce158CliPeer.hpp:77-94`, `Core/Connector/Ce158Card.hpp:363-382`).
- [ ] **AC-9** — Given a PC-1600, then the commands of AC-7 and AC-8
  exist for its own RS-232C port too, as `pc1600 rs232c feed`,
  `capture`, `pty` and `report` without the printer's port (OQ-1).

Checks
- [ ] **AC-10** — Given each command of AC-2 to AC-9, then the test
  suite has a test for it; today no test runs these functions. The
  check `dev/tape-matrix/run.py` runs against the harness and passes.

## Edge Cases
- **EC-1** — Given `pc1600 tape play` with a file that is no WAV that
  can be read, then it is refused with the reason.
- **EC-2** — Given a `tape` command on a machine without a cassette (a
  PC-1600 with a CE-150), then it is refused.
- **EC-3** — Given a `ce158` command on a machine without a CE-158, then
  it is refused.
- **EC-4** — Given `pc1600 basic-dump` on a machine whose BASIC is not
  set up, then the pointers are given as they are, and the program area
  is left out when its start and end make no sense.
- **EC-5** — Given `pc1600 ce158 feed` twice, then the second file takes
  the place of what was left of the first.

## Technical Requirements (optional)
- The commands reach the machine through the product kinds of C1600-1
  and the generic kinds (the serial port, media); they name no machine
  class.
- The commands are product commands: they are reached only with
  Calc-U-1600 (`harness.md` H-2).

## Open Questions
- [ ] **OQ-1** — The PC-1600's own RS-232C port (AC-9). Neither tool
  reaches it today; only a probe does. The serial port is one generic
  kind (LIB-2), so the same commands cost little. *Recommendation:* add
  them, as AC-9 says.
- [ ] **OQ-2** — The names of the commands of this spec.
  *Recommendation:* as written.
- [ ] **OQ-3** — "View" (of the INSPECTOR) and "peer" (of a serial
  port) for the glossary, as defined above.

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| What only one product's tools have becomes product commands under the product's name | `harness.md` H-2; LIB-11 AC-13, OQ-4 (user, batch 3 gate) | 2026-11-10 |
| The three probes stay scripts | LIB-11 OQ-5 (user, batch 3 gate) | 2026-11-10 |
| Cassette tapes keep their own rules | `media.md` Q-1; LIB-7 AC-42 | 2026-11-10 |
| Proposed in this spec, for the gate: `tape status` also tells a failed save (AC-6) | Today the error of a save at a motor stop is never told | 2026-11-10 |
| Proposed in this spec, for the gate: the tape and the CE-158's peer are set by commands at any time, not only before a preset runs | A harness run is a sequence of commands (LIB-11 AC-2) | 2026-11-10 |
