# C1600-2: Sharp program files

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: C1600-1 (the Sharp machines; the product kind for the
  loaders' memory access, which this feature defines), LIB-11 (the
  harness, for the product commands), LIB-14 (the File menu, notices),
  LIB-5 (typing, for a program that is typed in), LIB-9 (the folders a
  file dialog starts in).
- Contracts and decisions: `docs/contracts/frontend.md` (SP-3, FE-6),
  `docs/contracts/capabilities.md` (product kinds), `docs/contracts/harness.md`
  (H-2); ADR 0037 (external formats are not touched); the index as
  re-cut at the batch 5 gate (the loaders alone; the file kinds of "open
  file" are product data in LIB-13).
- Facts: Calc-U-1600 at `b4ea8c8`; every `path:line` below is at that
  commit. The header layouts are from the research (`PC1500` skill,
  `Shared/Data-Formats/Binary-Exchange-Formats.md`).

Glossary terms used: Program (BASIC program, machine-code program),
Memory module, Program module, Cassette tape. Words of this spec: a
**BASIC listing** is a BASIC program as text, and **tokenized BASIC**
the same as the machine stores it, in a file with a header; a
**header** is the first bytes of a program file that say what it is,
how long it is and where it goes; **headerless** code is a file of
machine code without one; the **program area** is the part of memory
BASIC keeps the program in, on the PC-1600 the one that is selected
with the BASIC command TITLE: S0, or S1 or S2, which lie in the memory
module of slot 1 or 2; **program mode** is the mode in which typed lines
are stored (PRO); **MODE 0** and **MODE 1** are the two modes of the
PC-1600, its own and the one in which it behaves as a PC-1500; a
**plan** is the loader's answer to where a machine-code file would go,
made before anything is written; the **starting command** is the BASIC
command that starts loaded code (`CALL` and its forms); the **work
area** is the memory the system keeps its own variables in.

## User Stories
- As a user, I want to put a BASIC program from a file into the machine
  at once, without typing it and without a cassette.
- As a user, I want to put a machine-code program into memory and be
  told how to protect it from BASIC and how to start it.
- As a user with a cassette recording, I want to load a program from the
  WAV file at once instead of playing the tape.
- As the author of a preset or a debugger, I want the same loading with
  the same checks, whatever starts it.

## Out of Scope
- The `program:` section of a preset, which calls these loaders (owner:
  C1600-4); the debugger's Build & Load (C1600-3).
- Dropping a program file, Open With, and telling a program file from a
  preset or a medium (owner: LIB-13); this feature supplies the kinds
  and what is done with each.
- Telling what a file is and decoding a cassette WAV: SharpDataExchange
  does both, and it is used as it is, as a built library that the build
  on GitHub fetches, as today (OQ-1).
- Playing and recording a cassette in the cassette recorder (owner: LIB-7).
- How a key is pressed (owner: LIB-5).
- A menu item that types a program in (today only a preset can; a new
  function).

## Acceptance Criteria

What a file is
- [ ] **AC-1** — Given the bytes of a file, then the loaders tell it to
  be one of: empty; a BASIC listing; tokenized BASIC for the PC-1500
  (CE-158 header) or for the PC-1600; machine code with a CE-158 header
  or with a PC-1600 header; headerless; or something else, with a few
  words that say what (a reserve area, variables, plain text). For
  headerless bytes it also says whether they look like code of the
  LH5801 or of the Z-80. The content decides, never the file name, and
  the rules are SharpDataExchange's, unchanged
  (`Core/ProgramFile.cpp:17-105`).
- [ ] **AC-2** — Given a file with a header, then the loaders read from
  it: from a CE-158 header (27 bytes) the name, the load address, the
  length and the address to start at; from a PC-1600 header (16 bytes)
  the length, the load address with its bank and the address to start
  at; an address to start at of zero or FFFF means none
  (`Core/ProgramFile.cpp:88-94`, `Core/tests/program_file_tests.cpp:107, 154`).
- [ ] **AC-3** — Given a cassette WAV, then it is decoded at once, and
  every file on it whose checksums are right is offered, in the order on
  the tape, with its name; each is a file like any other (AC-1). Given
  no file on it can be read, then the WAV is "something else", and the
  words say that it is a cassette recording without a file that can be
  read (`Core/ProgramFile.cpp:36-78`, `Core/tests/program_file_tests.cpp:272-300`).

BASIC
- [ ] **AC-4** — Given a BASIC listing or tokenized BASIC and the
  command to load it, then the program is in the program area as the
  ROM's own loading would leave it: the program that was there is gone,
  the pointers of BASIC say where the new one ends, and nothing else
  changes: no reset, no NEW, the mode and the selected program area as
  before. The memory is then the same, byte for byte, as when the same
  program is typed in (`Core/PC1500/PC1500BasicLoader.hpp:19-26`,
  `Core/tests/basic_fastloader_tests.cpp:131`,
  `Core/tests/pc1600_basicloader_tests.cpp:421`).
- [ ] **AC-5** — Given a listing, then it is turned into tokens for the
  machine it is loaded into: on a PC-1500 and PC-1500A with the PC-1500's
  words, on a PC-1600 in MODE 0 with the PC-1600's and in MODE 1 with
  the PC-1500's, where a character that is not ASCII is refused. Given
  tokenized BASIC, then a PC-1500 refuses a PC-1600 file, and a PC-1600
  takes a PC-1600 file in both modes and a CE-158 file in MODE 1 only
  (`Core/PC1600/PC1600BasicLoader.cpp:56-73`,
  `Core/PC1500/PC1500BasicLoader.cpp:39-40`).
- [ ] **AC-6** — Given a PC-1500 or PC-1500A, then the program goes to
  where BASIC's program starts; given a PC-1600, then into the program
  area that is selected: S0, which can reach over memory modules and the
  built-in RAM, filled in the order BASIC's own table of that area gives
  its parts, a line that does not fit into the rest of one part starting
  the next; or the program area S1 or S2 in a memory module. Given the pointers of BASIC are not those
  of a machine that was set up (after NEW), the program does not fit,
  or the selected slot holds no program module, then the load is refused
  with a reason that names the pointer or the room, and nothing is
  written (`Core/PC1500/PC1500BasicLoader.cpp:62-115`,
  `Core/PC1600/PC1600BasicLoader.cpp:158-194`,
  `Core/PC1600/PC1600ProgramPlacement.cpp:270-286`).
- [ ] **AC-7** — Given the command to type a listing in, then its lines
  are typed through the ROM's editor, each followed by ENTER, by the
  typing engine (LIB-5 AC-16, AC-28); an empty line is left out, and a
  character without a key refuses the command before anything is typed.
  After each line the loader waits until the machine reports prompt
  readiness, for at most 12 seconds of emulated time, and then until the
  end of the program has not moved for 30 frames, for at most 6
  seconds. A line is stored when the end of the
  program or that line's own record changed; on the PC-1600 a line of
  more than 79 characters is not typed and counts as not stored. Given
  no line was stored, or some were not, then the command fails and names
  them. The loader does not put the machine into program mode
  (`Core/PC1500/PC1500BasicTyper.cpp:43-56, 159-231`,
  `Core/PC1600/PC1600BasicTyper.hpp:24-59`,
  `Core/PC1600/PC1600BasicTyper.cpp:332-339`).

Machine code
- [ ] **AC-8** — Given machine code and the command to load it, with or
  without an address and a length given by hand, then one plan is made,
  the same for the menu, a preset and the debugger, and it is refused,
  before anything is written, in these cases, checked in this order
  (`Core/MachineCodeFile.cpp:240-297`):

  | Case | Refused with |
  |---|---|
  | the file is not machine code (a BASIC file: with the hint to load it as BASIC), or its header is damaged | what the file is |
  | the file holds fewer or more bytes than its header says, and no length was given by hand | both numbers |
  | a CE-158 file on a PC-1600 that is not in MODE 1; a PC-1600 file on a PC-1500 | which machine or mode takes it |
  | no byte follows the header, or the length given by hand is 0 | that there is no code |
  | headerless, and no address was given | the address is needed; on a PC-1600 the start of the selected program area is proposed |
  | the length given by hand is more than the file holds | both numbers |
  | a PC-1600 header names a bank other than 0 to 3, or the code does not lie in &8000 to &BFFF there, or there is no RAM in that bank | the bank and the range |
  | the code would run past &FFFF | the range |
  | code for the LH5803 that does not lie in &0000 to &7FFF | the range |
  | on a PC-1600 the address is ROM, or lies in a module that is not part of the selected program area | where machine code can go instead |

- [ ] **AC-23** — Given a plan that holds, then the code goes: to the
  address given by hand, else to the header's load address, and
  headerless code needs the one given by hand; with the length given by
  hand, else the number of bytes after the header. On a PC-1600 the bank
  is the one in the header's address, when no address was given by hand,
  and otherwise the one that follows from the selected program area and
  the address; headerless code cannot name a bank
  (`Core/MachineCodeFile.cpp:259-294`).
- [ ] **AC-9** — Given a PC-1600, then the processor the code is for is:
  the Z-80 for a file with a PC-1600 header, the LH5803 for one with a
  CE-158 header, and for headerless code the one the caller names, or
  else the Z-80 in MODE 0 and the LH5803 in MODE 1; addresses in every
  reason are in that processor's address space
  (`Core/MachineCodeFile.cpp:252-257`).
- [ ] **AC-10** — Given a plan that is not refused, then the bytes are
  written to the RAM the plan names, whichever bank is switched in at
  that moment, and nothing is written when a byte of the range is not
  RAM. Today the PC-1500 writes the other bytes and then reports the
  failure (`Core/PC1500/PC1500MachineCodeLoader.cpp:9-29`,
  `Core/PC1600/PC1600MachineCodeLoader.cpp:8-54`; OQ-2).
- [ ] **AC-11** — Given code was loaded, then the reply has these parts:
  the range it went to, and on the PC-1600 the program area and the
  bank; a NEW command with a note, or only a note where no NEW helps;
  the starting command with a note; and warnings, if any (AC-12). The
  starting command calls the header's address to start at, else the
  first byte, and the note says which: `CALL &X` on a PC-1500 and for
  Z-80 code in bank 0, `CALL #b,&X` for Z-80 code in another bank,
  `XCALL &X` for LH5801 code on a PC-1600. The NEW command is, as today:

  | Where the code lies | NEW |
  |---|---|
  | PC-1500, and PC-1600 in MODE 1 with S0: inside BASIC's memory | `NEW &e`, e the first address after the code |
  | PC-1500: outside BASIC's memory | none; the note says that none is needed |
  | PC-1600: in the program area S1 or S2 that is selected | `NEW "Sn:",&size` |
  | PC-1600, MODE 0: in the first part of S0 | `NEW "S0:",&size`, with a note on the bytes below the code that BASIC loses |
  | PC-1600: in a later part of S0, or above the program area in the built-in RAM | none; a warning that BASIC or its variables can overwrite it |
  | PC-1600: elsewhere | none; the note says that BASIC does not use this memory |

  (`Core/MachineCodeFile.cpp:350-505`,
  `Core/tests/machine_code_file_tests.cpp:349, 367`.)
- [ ] **AC-12** — Given code that lies in the system's work area of a
  PC-1600 (&F000 to &FFFF), then it is loaded and the reply carries a
  warning that names what is there (`Core/MachineCodeFile.cpp:166-185`).
- [ ] **AC-13** — Given the command to load code and start it, and a
  header with an address to start at, then after the load the starting
  command is typed with ENTER and the command waits until the machine is
  idle; after an hour of emulated time it is done all the same, and a
  line of the log says so. Within a preset the wait is the preset's
  implicit wait instead: code that has not returned within `timeout:`
  fails the preset (`preset.md` §6.2, §6.3; C1600-4 EC-3). Without such
  an address nothing is started
  (`Core/Preset/PresetRunner.cpp:243-257`).
- [ ] **AC-24** — Given the commands of this feature, then loading BASIC
  and loading machine code are carried out at once and are no
  operations; typing a program in, and loading with a start, are
  operations (LIB-3 AC-14). Each is refused while another operation
  runs, unless that operation itself calls it: a preset's `program:`
  section is part of the preset's operation (LIB-3 AC-17).

The product kind
- [ ] **AC-14** — Given a Sharp machine, then it lists the product kind
  "loader memory access" (C1600-1 AC-19), owned by the machine itself,
  which this feature defines: reading memory as a named processor sees
  it, without an effect on the machine; on a PC-1600 the mode (MODE 0 or
  1) and the selected program area; writing bytes into the built-in RAM and into the RAM of a
  module in a slot, whichever bank is switched in; whether a range of a
  slot is RAM; the size and the banks of the module in a slot; a write
  that is not stored says so (today
  `Core/PC1500/PC1500Memory.hpp:94-95`, `Core/PC1600/PC1600Machine.hpp:383-474`).

In the frontends
- [ ] **AC-15** — Given the Qt6 and the Web frontend, then the File menu
  has "Load BASIC Program…" and "Load Machine Code…" (LIB-14 AC-34 for
  Qt6, AC-35 for Web); the
  file dialog offers `.bas`, `.bbin` and `.wav`, or `.bin` and `.wav`,
  and all files; in Qt6 it starts in the folder set for BASIC or
  for assembly, or the one last used (LIB-9 AC-29)
  (`Qt6/app/MainWindow.cpp:178-180, 410-412, 877-878`,
  `Web/src/CalcU1600.jsx:268-269`).
- [ ] **AC-16** — Given a cassette WAV with more than one file that can
  be read, then the user is asked "`<file>` holds `<n>` files. Load
  which one?" with the files by number and name, and Cancel loads
  nothing (`Shared/MachineCodeLoad.cpp:12-26`,
  `Qt6/app/MainWindow.cpp:428-440`).
- [ ] **AC-17** — Given Load BASIC Program… with a file, then the
  program is loaded by AC-4; a refusal is a notice (LIB-14 AC-17).
- [ ] **AC-18** — Given Load Machine Code… with headerless code, then a
  dialog asks for the start address, shows the length and the processor
  that is assumed (AC-9), takes the address as `C000`, `&C000`, `$C000`
  or `0xC000`, says while the user types whether the plan of AC-8 holds
  and where the code would go, and its Load button can be used only
  while it holds (`Qt6/app/MachineCodeLoadDialog.cpp:12-63`,
  `Shared/MachineCodeLoad.cpp:52-110`).
- [ ] **AC-19** — Given machine code was loaded from the menu, then a
  dialog "Machine Code Loaded" shows the reply of AC-11 under "Keep
  BASIC from overwriting it:" and "Start it:", and the starting command
  is then put into the machine's input line without ENTER, so that the
  user starts the code (`Qt6/app/MainWindow.cpp:468-487`,
  `Web/src/Dialogs.jsx:134-150`). The menu never starts code by itself.
- [ ] **AC-20** — Given the GUI test of the Qt6 and of the Web frontend,
  then each has two scenarios: under the ID C1600-2 AC-18, headerless
  code is loaded, the address is asked, and NEW and CALL are shown;
  under the ID C1600-2 AC-16, a WAV with several programs asks which one
  (today
  `Web/tests/play.spec.js:278, 298`; ADR 0032).

Commands
- [ ] **AC-21** — Given the harness with Calc-U-1600, then these
  commands exist under the product's name (LIB-11 AC-13):

  | Command | Reply |
  |---|---|
  | `file-kind <file>` | what the file is (AC-1), and for a WAV its files (AC-3) |
  | `load-basic <file> [<n>]` | done, or refused with the reason; `<n>` chooses a file of a WAV, the first without it |
  | `type-basic <file>` | AC-7; done, or failed with the lines that were not stored |
  | `load-code <file> [<n>] [address=<a>] [length=<l>] [cpu=<name>] [run]` | the reply of AC-11 and AC-12, or refused with the reason; `run` is AC-13 |

- [ ] **AC-22** — Given what the tests of Calc-U-1600 check today about
  the loaders, then it holds
  (`Core/tests/program_file_tests.cpp`,
  `Core/tests/basic_fastloader_tests.cpp`,
  `Core/tests/pc1600_basicloader_tests.cpp`,
  `Core/tests/machine_code_file_tests.cpp`,
  `Core/tests/presetloader_trace_tests.cpp:326-360`,
  `Shared/tests/shared_app_session_tests.cpp:236-252`).
- [ ] **AC-25** — Given the same case is refused by the machine-code
  loader, by the BASIC loader or by a preset, then the reason is the
  same sentence, the menu's of today; a preset puts its line number
  before it (OQ-4; behaviour change 1600-15).

## Edge Cases
- **EC-1** — Given an empty file, then it is refused as empty by every
  loader; today the machine-code loader takes it as headerless and
  refuses it later (`Core/MachineCodeFile.cpp:62-69, 316`).
- **EC-2** — Given a file that starts like a header and has no complete
  header of a known kind, then it is refused, and the reason says just
  that (OQ-4).
- **EC-3** — Given a load while no program area is set up (the machine
  shows `NEW0? :CHECK`), then a BASIC load is refused with the hint to
  run NEW first (AC-6).
- **EC-4** — Given a preset names a WAV, then the first file on it is
  taken, as today (`Core/ProgramFile.cpp:69-74`).
- **EC-5** — Given machine code is given to the BASIC loader, then it is
  refused with the hint to load it as machine code (the other way round:
  AC-8).
- **EC-6** — Given a load from the menu or the harness while an
  operation runs, then it is refused (AC-24).

## Technical Requirements (optional)
- One classifier and one plan for the menu, the drop, the preset and the
  debugger (`Core/MachineCodeFile.hpp:131-185`).
- The loaders reach memory only through the product kind of AC-14; they
  name no machine class (`capabilities.md`, product kinds).
- Cassette WAV, `.bas`, `.bbin` and `.bin` files are external formats:
  what loads today loads afterwards (ADR 0037; Briefing §1.2).

## Open Questions
- [x] **OQ-1** — *Decided (user, 2026-11-10, batch 7 gate): as today, the build on GitHub fetches the library.* SharpDataExchange. Telling what a file is, turning a
  listing into tokens and decoding a WAV are done by a library that
  Calc-U-1600 carries as a built file for arm64 and for the web only
  (`Core/Basic/vendor/sharpdx/README.md:1-54`). The harness and the Qt6
  frontend of the source repository are to build on macOS, Linux and
  Windows. *Recommendation:* this spec takes the library as it is; how
  it is got for every platform is a point for the migration plan (phase
  6), and I need to know where its source lives. (Its source is
  `~/Development/sharp/SharpDataExchange`, Briefing §5.3; the migration
  plan fetches it as today, M-7.)
- [x] **OQ-2** — *Decided as recommended (user, 2026-11-10, batch 7 gate); row 1600-15, without `.rom`.* AC-10: a PC-1500 load that meets a byte that is not RAM
  writes the rest today and then fails. *Recommendation:* nothing is
  written, as on the PC-1600. This, OQ-3 and EC-1 are small visible
  changes; one row in `docs/behaviour-changes.md` names the three.
- [x] **OQ-3** — *Decided (user, 2026-11-10, batch 7 gate): no. `.rom` is not added; the extension is not used anywhere. Afterwards the user dropped it from a drop and from Open With too (LIB-13 AC-17, AC-21).* `.rom` in the file dialog of Load Machine Code…
  (AC-15). A drop and the macOS document types take it; the dialog's
  filter does not. *Recommendation:* add it.
- [x] **OQ-4** — *Decided as recommended (user, 2026-11-10, batch 7 gate).* The words of a refusal. The same case is worded three
  ways today, by the machine-code loader, the BASIC loader and the
  preset. *Recommendation:* one sentence for each case, the menu's of
  today; a preset puts its line number before it.
- [x] **OQ-5** — *Decided as recommended (user, 2026-11-10, batch 7 gate).* "BASIC listing", "tokenized BASIC", "header",
  "headerless", "program area", "program mode", "plan" and "starting
  command" for the glossary, as defined above (C1600-4 uses some too).
  The glossary's Program module, "cannot be written", does not fit the
  PC-1600's S1 and S2, which this spec calls program areas in a memory
  module.

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| This feature is the loaders alone; a preset's `program:` and the file kinds of "open file" build on it | user, 2026-11-10, batch 5 gate | 2026-11-10 |
| The loaders' memory access is a Calc-U-1600 product kind | `capabilities.md`, product kinds; C1600-1 AC-19 | 2026-11-10 |
| One "open file" command; a product adds its file kinds | `frontend.md` SP-3 | 2026-11-10 |
| External formats are not touched | ADR 0037; Briefing §1.2 | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 7 gate): the commands of AC-21 and their names | Rule 4 of the index; LIB-11 AC-18 | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 7 gate): an empty file is refused as empty by every loader (EC-1) | One wording, see OQ-4 | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 7 gate): the menu items and dialogs of the loaders are this feature's (AC-15 to AC-19) | Rule 5 of the index: they show this feature's behaviour | 2026-11-10 |
| OQ-1's last point answered: SDE's source is `sharp/SharpDataExchange`; the migration plan gets the library as today (M-7) | user, 2026-11-14 (question 9 of `docs/migration-plan.md`) | 2026-11-14 |
| The plan request is `c1600.code-plan`, the name of its payload (was `c1600.plan-code`; internal, no AC names it) | `docs/reviews/release-simplification.md` V-12 | 2026-11-18 |
