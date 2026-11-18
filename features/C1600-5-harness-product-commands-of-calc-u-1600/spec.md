# C1600-5: Harness product commands of Calc-U-1600

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: LIB-11 (the harness, product commands under the product's
  name), LIB-7 (cassettes, `cassette` and `eject`), LIB-10 (reading
  memory); also named here: C1600-1 (the product kinds for the INSPECTOR
  views, for the CE-158's printer port and for the host drive's folder),
  LIB-2 (the serial port kind, its host link and serial port file),
  LIB-18 (attach).
- Contracts and decisions: `docs/contracts/harness.md` (H-1 to H-3, I-2,
  the fit table); LIB-11 AC-13, AC-18 and its OQ-4 and OQ-5 as decided.
- Facts: Calc-U-1600 at `b4ea8c8`; every `path:line` below is at that
  commit. No test there runs these functions of the two command-line
  tools; what they do is read from their code.

Words of this spec: the **INSPECTOR** is Calc-U-1600's debug panel, and
a **view** is one of the texts it shows (C1600-1 AC-19); a **character
time** is the time one character takes on a serial port at the speed
the port is set to: ten bits. Words of LIB-2: serial port, host link,
serial port file.

## User Stories
- As a developer who checks a BASIC loader, I want the pointers of BASIC
  and the bytes of the program area as text.
- As a developer or an agent, I want any INSPECTOR view as text, without
  a window.
- As a developer who checks cassette saving, I want to record what the
  machine saves into a WAV file of my choice.
- As a developer who checks a serial port, I want to feed it from a
  file and to see what it sends, and for the CE-158 what it prints.

## Out of Scope
- Loading a preset, checking presets, waiting, the display as text and
  as a picture, the buzzer as a WAV (LIB-6, LIB-11); memory as a hex
  dump, registers, the last instructions and where time was spent
  (LIB-10); the model's variants and attaching a peripheral (LIB-18);
  the paper of the plotters (LIB-8); the loaders (C1600-2).
- Playing a cassette recording and taking a cassette out: `cassette` and
  `eject` of LIB-7 for the cassette (LIB-7 AC-26, AC-42).
- The three probes and `dev/tape-matrix/run.py`, which are not taken over
  (LIB-11 OQ-5 as amended, 2026-11-13).

## Acceptance Criteria

- [ ] **AC-1** — Given the harness with Calc-U-1600, then the commands
  of this spec exist under `c1600`, next to those of C1600-2, for the
  PC-1500, the PC-1500A and the PC-1600 (LIB-11 AC-13, AC-20).

BASIC
- [ ] **AC-2** — Given `c1600 dump-basic`, then the reply gives the
  pointers of BASIC, one line each in this order, and then the program
  area from its start to 16 bytes past its end, in rows of 16 bytes with
  the address before them and the same bytes as characters after them:

  | Model | Pointers, as named in the reply |
  |---|---|
  | PC-1500, PC-1500A | `BASPRG_ST $7865`, `BASPRG_END $7867` with the length, `BASPRG_EDT $7869`, `VAR_START $7899`, `DATA_PTR $78BE`, `RAM_END $7864` with `WARM_START $7A20` |
  | PC-1600 | `BASPRG_ST $F865` and `BASPRG_END $F867`, each with the Z-80's address for it, the second with the length, `BASPRG_EDT $F869`, `F5CF` with `F89D`, `VARIABLE_PTR $F899` |

  It is not the view `dump-basic` of AC-4, which is the INSPECTOR's own
  dump (`tools/pc1500_cli.cpp:342-373`, `tools/pc1600_cli.cpp:200-237`;
  OQ-2).
- [ ] **AC-3** — Given the lines of that reply, then their form is
  today's: `BASPRG_END $7867 = $40D2   (len = 13)` for a pointer and
  `40C5: 00 0A … |…|` for a row (`tools/pc1600_cli.cpp:200-237`).

INSPECTOR views
- [ ] **AC-4** — Given `c1600 inspect` with the name of a view, then
  the reply is that view as the INSPECTOR shows it, as lines of text. A
  name that is no view is refused; a view the model does not have gives
  the one line `<title>: not on the PC-1500`, or `… PC-1600`:

  | Views | PC-1500, PC-1500A | PC-1600 |
  |---|---|---|
  | `pointers`, `inventory`, `lh`, `basic`, `physical`, `dump-basic` | yes | yes |
  | `dump-ml` | yes | no |
  | `z80`, `programs`, `disks`, `dump-s1`, `dump-s2`, `dump-disk-s1`, `dump-disk-s2` | no | yes |

  (`Core/Debug/Inspect/Inspector.cpp:35-53`,
  `Core/Debug/Inspect/PC1500Inspector.cpp:365-388`,
  `Core/Debug/Inspect/PC1600Inspector.cpp:865-888`,
  `Core/tests/inspector_tests.cpp:97-250`.)

Cassette
- [ ] **AC-5** — Given `c1600 cassette record` with a path, then a new,
  empty cassette is in the cassette recorder, and what the machine saves is written
  to that file as LIB-7 AC-42 says, each time the motor stops and when
  the cassette is ejected; the name LIB-7 AC-20 asks a user for is not
  asked. A cassette that was in the recorder is ejected first
  (`tools/CliCommon.hpp:56-80`, `Core/Tape/TapeDeck.hpp:55-133`).
- [ ] **AC-6** — Given `c1600 cassette status`, then the reply (`cassette: playing|recording|empty …`, `motor: on|off`) gives the
  position of a tape that plays, or the length of one that records, in
  seconds, the length of the whole tape, and whether the motor runs, or
  that the cassette recorder is empty (`tools/CliCommon.hpp:84-95`; LIB-7 AC-42).

Serial ports
- [ ] **AC-7** — Given a serial port of the machine, `ce158` or, on a
  PC-1600, `rs232c` (OQ-1), then these commands exist for it
  (`tools/Ce158CliPeer.hpp:18-94`, `Core/Connector/Ce158Card.hpp:145-170`):

  | Command | What it does |
  |---|---|
  | `c1600 serial <port> feed <file> [hold=<n>]` | the file's bytes are what the port receives, one in each character time in which the port's receiver is empty; with `hold`, nothing comes in the first n character times. A second `feed` takes the place of what was left |
  | `c1600 serial <port> capture <file>` | the bytes the port sends are written to that file; the file is complete at the end of the run, at `quit`, and when another `capture` names another file |
  | `c1600 serial <port> link` | the port gets the host link of LIB-2 (AC-18 there), with its serial port file in the folder made for the run; the reply gives the path. `feed` and `capture` are then refused for that port |
  | `c1600 serial <port> report` | the speed the port is set to, the state of its receiver and sender, how many of the bytes to receive were taken, and all bytes the port sent since the machine was built, as text; for `ce158` also the bytes sent to its printer's port, as text |

- [ ] **AC-8** — Given such a text, then a line end and a character of
  ASCII that can be printed stand as they are, a carriage return is
  left out, and any other byte is written as `<XX>`
  (`Core/Connector/Ce158Card.hpp:363-382`).
- [ ] **AC-9** — Given a run of the harness, then no serial port has a
  host link unless `link` asked for one, and a run with a host link is
  not one that repeats exactly (LIB-11 AC-6, AC-8; `harness.md` I-2).

Host drive
- [ ] **AC-11** — Given a PC-1600 and `c1600 host-drive` with the path
  of a folder, then the host drive is attached in the place `host-drive`
  showing that folder, or shows that folder from then on when it is
  attached already; given `c1600 host-drive off`, then it is detached;
  what follows is what LIB-18 AC-30 says (`harness.md`, fit table;
  C1600-1 AC-19).

Checks
- **AC-10** — *Withdrawn (user, 2026-11-13, B-25 of
  `docs/walkthroughs/presets.md`): `dev/tape-matrix/run.py`, a finished
  one-time check, is not taken over (LIB-11 OQ-5 as amended).* It read
  the replies of AC-2 and of `peek`.

Machine-code routines
- [ ] **AC-12** — Given a paused machine, when `c1600 call <target>
  <address> [timeout=<duration>]` is given, then the target runs the
  routine at that address as a call from the current state, by the call
  and return of that processor, and pauses when it returns to the
  caller; the reply gives the registers then and the emulated time the
  routine took; when it has not returned within the timeout (default
  10 s of emulated time), the machine is paused and the reply says so.

Machine state
- [ ] **AC-13** — Given a PC-1500, PC-1500A or PC-1600, then it offers the
  machine state of LIB-11 AC-24: every processor, all memory with its
  banks, the clocks and timers, and every plugged peripheral with a
  medium in it (a cassette with its position, a floppy disk).

## Edge Cases
- **EC-2** — Given a `cassette` command on a machine without a cassette
  recorder (neither a CE-150 nor a CE-1600P), then it is refused.
- **EC-3** — Given `serial ce158` on a machine without a CE-158, or
  `serial rs232c` on a PC-1500 or PC-1500A, then it is refused; it is
  not held for a machine built later. A script that needs the port from
  the program's first byte builds the machine with commands (`model`,
  `attach`; LIB-18 AC-15), or with a preset that does not start the
  program, then connects the port, then starts the program; today
  `pc1500_cli` connects the port before its preset builds the machine
  (`tools/pc1500_cli.cpp:185-187`).
- **EC-4** — Given `c1600 dump-basic` on a machine whose BASIC is not
  set up, then the pointers are given as they are, and the program area
  is left out when its start does not lie before its end.
- **EC-5** — Given `c1600 host-drive` on a PC-1500 or PC-1500A, or with
  a path that is no folder, then it is refused.
- **EC-6** — Given `c1600 cassette record` with a path that cannot be
  written, then the save fails when the motor stops, and LIB-7's notice
  says so.

## Technical Requirements (optional)
- The commands reach the machine through the product kinds of C1600-1
  and the generic kinds (the serial port, media); they name no machine
  class.
- Each command of AC-2 to AC-9 and AC-11 has a test in the test suite whose
  expected reply is written out in the test; today no test runs these
  functions.
- The commands are product commands: they are reached only with
  Calc-U-1600 (`harness.md` H-2).

## Open Questions
- [x] **OQ-1** — *Decided as recommended (user, 2026-11-10, batch 8 gate).* The PC-1600's own RS-232C port (AC-7). Neither tool
  reaches it today; only a probe does. The serial port is one generic
  kind (LIB-2), so the same commands cost little. *Recommendation:* add
  it, as AC-7 says.
- [x] **OQ-2** — *Decided as recommended (user, 2026-11-10, batch 8 gate).* The names of the commands. `harness.md` H-2 names
  `c1600 dump-basic`, `pc1600 tape` and `c1600 serial`; the INSPECTOR
  has a view that is also called `dump-basic`. *Recommendation:* as
  written here: the contract's names, with the view reached as `c1600
  inspect dump-basic`.
- [x] **OQ-3** — *Decided as recommended (user, 2026-11-10, batch 8 gate).* "View" (of the INSPECTOR) and "character time" for the
  glossary, as defined above.

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| What only one product's tools have becomes product commands under the product's name | `harness.md` H-2; LIB-11 AC-13, OQ-4 (user, batch 3 gate) | 2026-11-10 |
| *Superseded 2026-11-13 (B-25).* The three probes stay scripts | LIB-11 OQ-5 (user, batch 3 gate) | 2026-11-10 |
| *Superseded 2026-11-13 (F-7: a tape is a medium, LIB-7 AC-42).* Cassette tapes keep their own rules | `media.md` Q-1; LIB-7 AC-42 | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 8 gate): playing a recording and ejecting are LIB-7's `insert` and `eject`; only recording to a path and the status are commands of this spec | One vocabulary (`harness.md` H-1) | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 8 gate): a serial port has no host link in a harness run unless `link` asks (AC-9) | A run must repeat exactly and touch no file of the user (LIB-11 AC-6, AC-8) | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 8 gate): `pc1600 host-drive` (AC-11) | `harness.md`'s fit table lists the host drive among the product commands; today no tool has an option for it | 2026-11-10 |
| The probes and `dev/tape-matrix/run.py` are not taken over; AC-10 withdrawn | user, 2026-11-13 (B-25 of `docs/walkthroughs/presets.md` §5.2): "old, no longer relevant scripts and probes are not taken over into the new project. calendar yes, upgrade it" | 2026-11-13 |
| A `serial` command on a machine without the port is refused, never held for a later machine; the order is the script's: build, connect, start (EC-3) | user, 2026-11-13 (B-25 (c) of `docs/walkthroughs/presets.md` §5.2), the revised recommendation: every preset change has a harness command (LIB-18 AC-15, LIB-7 AC-26, LIB-6 AC-34), so no command needs to wait for a future machine | 2026-11-13 |
| `pc1600 call` runs a machine-code routine until it returns, for testing routines (AC-12) | user, 2026-11-14 (debugging and testing through the harness) | 2026-11-14 |
| The Sharp machines offer the machine state of LIB-11 AC-24 (AC-13) | user, 2026-11-14 (debugging and testing through the harness) | 2026-11-14 |
| M-51: the machine state of AC-13 is split off (design §8): it needs a save and a restore on every part of the core that holds state; the column stays In progress until it passes, the rest of M-51 does not wait for it | design §8, as it foresaw; qa-report | 2026-11-16 |
| M-51: `feed`'s `hold` counts the character times from the `feed` command; the bytes go to the port one in each character time, at the speed the port is set to, or 300 baud while it has none | the spec leaves the start open; a script sends `feed` when the port is ready (EC-3); the CLI counted from the port's first use, which the command can wait for with `wait` | 2026-11-16 |
| M-51: `c1600.routine-call` is one capability of the main CPU that calls on any processor of the model by its debug target name, not one capability per processor (design §3.6) | the simpler structure; the targets are the debugger's names | 2026-11-16 |
| M-51: the CE-158 gives its serial port the speed its ROM set (LIB-2's line settings), so `report` and `feed` know it | today the card kept the speed to itself; the PC-1600's port already did (LIB-2 AC-17) | 2026-11-16 |
| M-51 (AC-13): the machine state holds the machine and its peripherals with their media; what the session keeps is not in it: LIB-7's record of what a media place holds and where `tape record` writes, the paper drawn, the bytes in a serial port's queues, the debugger's breakpoints, watches, history and trace. A state saved while a change of the peripherals waits for a power cycle is refused at the restore | the session's parts are not the machine (LIB-11 AC-24: files of the host are not part of the state); a waiting change is not a part of the core; qa-report | 2026-11-16 |
| M-51 (AC-13, T8): `c1600_5_ac13_full_pc1600_…` runs two PC-1600s, the CE-1600P with its disk and tape on one, the CE-150 with the CE-158 on the other | the CE-1600P and the CE-158 cannot be used together (LIB-2 D-6, ledger 1600-22) | 2026-11-16 |
| The cassette commands are `pc1600 cassette record <file.wav>` and `pc1600 cassette status`, the reply `cassette: …` (AC-5, AC-6, EC-2, EC-6 edited in place; was `pc1600 tape …`, `tape: …`); the word `pc1600` stays until release prep step 1 | user, 2026-11-17 (Part E review Q-6); `docs/reviews/release-prep-plan.md` §2 (2o); behaviour change 1600-51 | 2026-11-17 |
| Release prep step 1: the commands are under the product word `c1600` (was `pc1600`), `c1600 cassette record|status` among them. ACs edited in place, names only | user, 2026-11-17 (`docs/reviews/e-simplification.md` §10 Q-6): one word per product everywhere; `docs/reviews/release-prep-plan.md` §1; ledger row 1600-50 | 2026-11-17 |
| The INSPECTOR's module dump is "Module YAML" (was "Card YAML"), its header lines "Module dump: …" and "Module dump: no module plugged in, …" (were "Module card dump: …", "no card attached"); the Web page's request `c1600.module-yaml` (was `c1600.card-yaml`), the app layer's `ModuleYamlDump` (was `CardYamlDump`) | the glossary avoids "card" for a module; `docs/reviews/release-simplification.md` L-8, V-2; behaviour change 1600-54 | 2026-11-18 |
