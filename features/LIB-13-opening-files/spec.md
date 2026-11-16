# LIB-13: Opening files

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: LIB-6 (loading a preset), LIB-7 (media, importing one,
  putting one into its place); also named here: LIB-3 (commands,
  notices, operations), LIB-11 (the harness), LIB-14 (the File menu),
  LIB-15 (what a Web frontend can hand over). The product columns need
  the features that own each product's file kinds: C59-7 (magnetic
  cards) and C59-4 (the ASM overlay); C1600-2 (BASIC and machine-code
  programs, cassette recordings).
- Contracts and decisions: `docs/contracts/frontend.md` (SP-3, Q-4),
  `docs/contracts/media.md` (I-1 to I-3); ADR 0019; behaviour change
  59-6.
- Facts: Calc-U-59 at `v2.3.0`, Calc-U-1600 at `b4ea8c8`; every
  `path:line` below is at those pins.

Words used here: to **open** a file is to hand it to the app session
with the command of this feature, which finds out what the file is and
does what belongs to it. A **file kind** is one sort of file a product
opens besides presets and media (a BASIC program, an ASM overlay); the
product names its file kinds and tells one from another. Words of other
specs: "kind" of medium, "import" (LIB-7), "notice" (LIB-3 AC-10).

What the three status columns mean for this feature:
- **Library:** the command, the telling apart of presets, media and
  product file kinds, and what follows for presets and media, shown with
  a product and file kinds built for the tests.
- **Calc-U-59, Calc-U-1600:** every way a file reaches the product goes
  through this command, in every frontend, and the product's file kinds
  are those listed here.

## User Stories
- As a user, I want to drop a file on the calculator, or open it from
  the file manager, and have the product do the right thing with it,
  whatever kind of file it is.
- As a user, I want to be told when a file is nothing the product can
  open.
- As a user, I want a file I open while the machine is busy to be opened
  when it is free.
- As a developer or an agent, I want to open a file through the harness
  as a user would.

## Out of Scope
- Loading a preset (owner: LIB-6); importing a medium and putting it
  into its place (LIB-7; magnetic cards: C59-7).
- What is done with a file of a product's kind: BASIC and machine-code
  programs and cassette recordings (owner: C1600-2), the ASM overlay
  (C59-4). This feature brings the file there.
- The pickers of media places and the import menu item (owner: LIB-7).
- The `preset` and `model` props and the address parameters of a Web
  frontend (owner: LIB-15).
- Drop on iOS, and Open With in the Swift frontend: there is none today
  and none is added (`frontend.md` SP-3).
- Opening a file by double click on Windows and Linux for Calc-U-1600 (a
  new function; none today).

## Acceptance Criteria

### Library

The command
- [ ] **AC-1** — Given the command to open a file, with its bytes, its
  name and, where the host has it, its path, then the session goes
  through these tests in this order, and the first that applies decides;
  names are compared without regard to upper and lower case
  (`frontend.md` SP-3):

  | # | Test | Then |
  |---|---|---|
  | 1 | the file cannot be read, is larger than the product allows for this way in (AC-10), or its name starts with a dot | refused |
  | 2 | its name ends in a preset extension of the product | a preset: it is loaded (LIB-6), from its folder where the path is known, else as its content (LIB-6 EC-5) |
  | 3 | its name ends in the suffix of a kind of medium of the product | a medium: AC-4 |
  | 4 | a file kind of the product takes it, the kinds asked in the order the product gives, each by its own rule | the product's handling of that kind |
  | 5 | its text has a top-level `model:` line (AC-2) | a preset, as in 2 |
  | 6 | nothing of these | refused |

  A refusal is a notice that names the file and says why.

- [ ] **AC-2** — Given a file without a preset extension, then it is a
  preset when it is text in UTF-8 without a zero byte and has a line
  that starts with `model:` in the first column (Calc-U-1600
  `Core/DropFile.cpp:20-37`, `Core/tests/program_file_tests.cpp:199-242`).
- [ ] **AC-3** — Given the command, then its reply says what the file
  was opened as, that it is kept for later (AC-7), or why it was
  refused.
- [ ] **AC-4** — Given a medium, then it is used where it lies: nothing
  is copied, the file is the medium's instance from then on, and what
  the machine writes to it is saved back to that file (LIB-7 AC-1,
  AC-11, AC-12), unless the file says it is a template, which is never
  written. Everything is checked first: an outdated file, or one that
  cannot be read, is refused with the hint of LIB-7 AC-9. Then a medium
  for a holding media place is inserted (LIB-7 AC-26), with its first
  side, the medium that was there being ejected first; a medium for a
  pass-through place gets what the product says follows. A module is
  plugged in, as by choosing it in the slot's picker, when it is clear
  from the context where it goes: the machine has one module slot (a
  PC-1500), or the module can go into only one of the machine's slots
  (user, 2026-11-10, after the batch 7 gate). Any other module, and a medium whose place is not there now, is
  refused with a notice that says where such a medium is chosen.
  In a Web frontend, where a file that is opened is an upload and
  cannot be written back, the medium is imported into the save folder
  first (LIB-7 AC-30); given a medium of that kind already has the name,
  then the rule of Name & Save holds (LIB-7 AC-17, first column)
  (user, 2026-11-10, batch 7 gate; ADR 0019 as amended;
  `media.md` I-1; `frontend.md` Q-4; OQ-1).

The ways in
- [ ] **AC-5** — Given a drop on the window, Open With of the platform,
  a file named when the program is started, and the file dialog of File
  ▸ Load Preset…, which offers the preset extensions and all files, then
  each sends this command and nothing else decides what the file is. A
  menu item of a product for one kind of file (Calc-U-1600's Load BASIC
  Program…) and the import item of LIB-7 do not: they go straight to
  their feature (`frontend.md` SP-3).
- [ ] **AC-6** — Given a frontend where a drag can be told what it
  carries before the drop (Qt6, Swift on macOS), then the frontend asks
  the session whether it would take a file of that name, which goes by
  the tests 1 to 3 of AC-1 and by the names the product's file kinds go
  by; a drag of more than one file, or of a name it would not take, is
  not taken, so that the pointer shows it. The drop itself goes through
  all of AC-1
  (Calc-U-1600 `Qt6/app/MainWindow.cpp:781-812`; Calc-U-59
  `App/EmulatorViewModel.swift:362-374`).
- [ ] **AC-7** — Given a file is to be opened while an operation runs or
  before the product has finished its start, then the session keeps it,
  the reply says so, and it is opened when the session is free; given a
  second one comes meanwhile, then it takes the place of the first (Calc-U-1600 `Qt6/app/MainWindow.cpp:814-834`,
  `Web/src/CalcU1600.jsx:173-186`).
- [ ] **AC-8** — Given a Web frontend whose file dialog hands over
  several files at once, then they are taken as one folder, and the
  preset among them is opened, the first by name when there are several,
  so that the files it names are found; given none is a preset, then the
  first file is opened alone
  (Calc-U-1600 `Web/src/CalcU1600.jsx:137-147`).
- [ ] **AC-9** — Given the harness, then `open <path>` is this command,
  with the reply of AC-3.
- [ ] **AC-10** — Given a product, then it can set the largest file each
  way in takes; a larger one is refused (AC-1).

### Calc-U-59

- [ ] **AC-11** — Given Calc-U-59, then its preset extensions are
  `.ti59`, `.ti58` and `.ti58c`; its one kind of medium is the magnetic
  card, `.magnetcard.yaml`; its file kinds are the keystroke program
  file (AC-23) and the ASM overlay, which C59-4 adds;
  it sets no limit on the size of a file
  (`Shared/SessionController.cpp:620-638`, `Shared/StateFile.cpp:554-557`).
- [ ] **AC-12** — Given a magnetic card is opened on a TI-59, then it is
  fitted from where it lies and written back there, as today, unless
  its file is a template (`template: true`), which is never written
  (LIB-7 AC-25) (AC-4;
  C59-7 AC-11; `Shared/SessionController.hpp:192-196`); in the Web
  frontend it is imported and fitted. Given a TI-58 or TI-58C, which has no card
  reader, then it is refused with a notice; today nothing happens
  (`tests/shared_cooperative_tests.cpp:593`).
- [ ] **AC-23** — Given a keystroke program file (`.keystrokes.yaml`,
  `docs/formats/preset.md` §4.1.1) is opened on a TI machine, then its
  program is written into the running machine's program memory from
  step 000, as a preset's `program:` writes it, without a cold boot:
  the partition, the data registers and the steps it does not list
  stay; a file that cannot be read, or a step beyond the model's last
  step (C59-2 AC-3), is refused with a notice. Given a Solid State ROM module file
  (`.rom.yaml`), then it is refused with a notice that this release
  takes a module by its ID only (C59-6 AC-8). Given the constant memory
  file (`ti58c.memory.yaml`), then it is refused with a notice that it
  is the product's own file and is not opened.
- [ ] **AC-13** — Given a file kind that a later feature adds (the ASM
  overlay, C59-4), then this feature's column is complete without it;
  until then an `.asm` file is refused, as today
  (`Shared/SessionController.cpp:620-624`).
- [ ] **AC-14** — Given the ways in of Calc-U-59, then they are those of
  today: a drop in Qt6, in the Web frontend and in the Swift frontend on
  macOS; File ▸ Load Preset… in every frontend; in Qt6 a file named at
  the start of the program, of which the last one counts, and Open With
  on macOS, Windows and Linux for the three preset extensions
  (`Qt6/app/main.cpp:13-31, 68-81`,
  `Qt6/resources/macos/Info.plist.in:30-44`,
  `Qt6/resources/windows/calc-u-59.iss:46-52`,
  `Qt6/resources/linux/calc-u-59.desktop:5-8`). A file named at the
  start, and one opened with Open With, is opened as what it is; today
  it is always read as a preset.
- [ ] **AC-15** — Given a file that is refused in Qt6 or in the Swift
  frontend after it was handed over, then the notice of AC-1 is shown,
  as the Web frontend shows a line today (`Web/src/engine.js:140`).
- [ ] **AC-16** — Given what the tests of Calc-U-59 check today about
  dropped files, then it holds, with AC-12: a preset by its extension in
  any case, a card on the TI-59, a hidden file refused, a dropped preset
  loaded like one from the dialog
  (`tests/shared_cooperative_tests.cpp:529-546, 838-847`,
  `Web/tests/play-calculator.spec.js:266-292`).

### Calc-U-1600

- [ ] **AC-17** — Given Calc-U-1600, then its preset extensions are
  `.pc1500`, `.pc1500a` and `.pc1600`; its kinds of media are the
  battery-backed module (`.card.yaml`), the floppy disk
  (`.floppy.yaml`) and the cassette tape (`.tape.yaml`, AC-19); its file kinds, asked in this order, are the BASIC
  program and the machine-code program, told by what C1600-2 says the
  file is (C1600-2 AC-1): a BASIC listing or tokenized BASIC is a BASIC
  program; machine code with a header is a machine-code program;
  headerless bytes only when C1600-2 says they look like code and the
  name ends in `.bin` (today also `.rom`, which goes: user, 2026-11-10, after the batch 7 gate); a cassette
  WAV goes to the loader of
  its first readable file, and to the BASIC loader when none can be
  read, so that the loader says why; an empty file and "something else"
  are taken by neither (`Core/DropFile.cpp:39-67`).
- [ ] **AC-18** — Given a BASIC program or a machine-code program is
  opened, then what follows is what File ▸ Load BASIC Program… and Load
  Machine Code… do with that file (C1600-2 AC-16 to AC-19): a WAV with
  several files asks which one, and the file chosen is then loaded by
  the loader of its own kind. In the harness, which has no dialogs,
  `open` loads the first file of a WAV and refuses headerless code,
  which needs an address (C1600-2 AC-21).
- [ ] **AC-19** — Given a disk file is opened, then AC-4 holds: in Qt6
  it is inserted from where it lies, when the drive is there; in the Web
  frontend it is imported, and inserted when the drive is there. A
  module file is plugged in from where it lies in Qt6 when its slot is
  clear from the context, and refused otherwise; in the Web frontend it
  is imported, and plugged in under the same rule (AC-4).
  Today such a file can only be imported from the menu of the Web
  frontend (`Shared/MediaLibrary.cpp:59-107`). A tape definition
  (`.tape.yaml`) is a medium, inserted into the cassette when the model
  has one (LIB-7 AC-42). A WAV is never taken as a tape here: it is a
  program file and is loaded (`docs/formats/media.md` §3; C1600-2).
- [ ] **AC-20** — Given a drop or Open With, then a file of more than
  1 MiB is refused; today it is passed over without a word (OQ-4) (`Qt6/app/MainWindow.cpp:768-779`,
  `Web/src/CalcU1600.jsx:16, 175`); the file dialogs of the menu have no
  limit.
- [ ] **AC-21** — Given the ways in of Calc-U-1600, then they are those
  of today: a drop in Qt6 and in the Web frontend; the items of the File
  menu; in Qt6 on macOS Open With, for the three preset extensions as
  the product's own and for `.bas`, `.bbin` and `.bin`; `.rom`, which
  is in that list today, goes
  (`Qt6/app/main.cpp:23-49`, `Qt6/resources/macos/Info.plist.in:49-99`).
  A file named at the start of the Qt6 program is opened too; today it
  is passed over (`Qt6/app/main.cpp:72-101`; OQ-2).
- [ ] **AC-22** — Given what the tests of Calc-U-1600 check today about
  telling a dropped file, then it holds
  (`Core/tests/program_file_tests.cpp:199-300`).

## Edge Cases
- **EC-1** — Given a file with a preset extension that is outdated or
  has a mistake, then it is a preset and LIB-6 refuses it, with its
  hint; it is not tried as anything else.
- **EC-2** — Given a floppy disk file is opened while no drive is there,
  then it is refused with a notice in Qt6, and imported only in the Web
  frontend (AC-4).
- **EC-3** — Given a preset opened in a Web frontend alone, which names
  other files, then it is refused when it is read (LIB-6 EC-5).
- **EC-4** — Given a medium is opened while its place holds one that is
  being passed through or written, then it is refused with a notice,
  and nothing is imported; today Calc-U-59 does nothing and says
  nothing
  (`Shared/SessionController.cpp:399`).
- **EC-5** — Given a file that cannot be read, then the command is
  refused and the notice names the file.
- **EC-7** — Given a file is dropped, opened with Open With or named
  at the start, then the folder in which the next file dialog starts
  does not change; only a file chosen in a file dialog sets it (LIB-9
  AC-29; OQ-3). Today a drop in Calc-U-1600 changes it.
- **EC-6** — Given the user of a Web frontend says no to replacing a
  medium (AC-4), then nothing is imported and nothing is put into a
  place.

## Technical Requirements (optional)
- No frontend decides what a file is; it hands the file over
  (`frontend.md` SP-3, FE-2).
- Which kinds of file a product claims in the file manager of each
  platform is product data of its frontends (AC-14, AC-21).
- The library tells presets and media; a product's file kinds and their
  rules are supplied by the product (`frontend.md` SP-3, I-4).

## Open Questions
- [x] **OQ-1** — *Decided (user, 2026-11-10, batch 7 gate): no copy, except in a Web frontend. A medium that is dropped or opened stays where it is and, if necessary, is written back where it is. In a Web frontend a drop is an upload, so there it is copied into the save folder.* A medium that lies outside the save folder
  (`frontend.md` Q-4). ADR 0019 lets such a file be used only when a
  preset names it or the user puts it into the save folder; Calc-U-59
  fits a dropped card and writes it back where it lies today.
  *Recommendation:* AC-4: opening it puts it into the save folder for
  the user (an import) and then into its place; a medium of the same
  name is replaced only after the user said yes.
- [x] **OQ-2** — *Decided as recommended (user, 2026-11-10, batch 7 gate).* A file named at the start of the program (AC-14,
  AC-21): Calc-U-59's Qt6 frontend opens it, Calc-U-1600's passes it
  over. *Recommendation:* both open it, through this command.
- [x] **OQ-3** — *Decided as recommended (user, 2026-11-10, batch 7 gate).* A file dialog and the folder it starts in (LIB-9
  AC-29). Today a drop in Calc-U-1600 changes the folder the next file
  dialog starts in. *Recommendation:* only a file dialog does.
- [x] **OQ-4** — *Decided as recommended (user, 2026-11-10, batch 7 gate).* Refusals that are silent today get a notice: a file
  that is nothing the product opens, after Open With or at the start; a
  file over 1 MiB in Calc-U-1600; a magnetic card on a model without a
  reader; a card while one is in the reader (AC-1, AC-12, AC-20, EC-4). *Recommendation:* all three; too small
  for a row each, one line in row 59-14 and its Calc-U-1600 twin.
  (Since then the file changes have rows of their own: 59-20 and
  1600-15.)
- [x] **OQ-5** — *Decided as recommended (user, 2026-11-10, batch 7 gate).* "Open" (a file) and "file kind" for the glossary, as
  defined above.

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| One "open file" command on the app session for a drop, Open With and a file picker that takes any file; the library tells presets and media, a product adds its file kinds | `frontend.md` SP-3 (user) | 2026-11-10 |
| Today's accepted files stay; where a platform has no drop today, none is added | `frontend.md` SP-3 (user) | 2026-11-10 |
| A medium opened from elsewhere is used and written back where it lies; only a Web frontend copies it | user, 2026-11-10, batch 7 gate; ADR 0019 as amended; behaviour change 59-6 as reworded | 2026-11-10 |
| Each product's file kinds are product data of this spec; Calc-U-1600's rest on the loaders of C1600-2 | user, 2026-11-10, batch 5 gate | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 7 gate): a file with a preset extension is a preset whatever it holds (AC-1, EC-1) | An outdated preset then gets its hint, not "cannot open" | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 7 gate): the order of the tests of AC-1, a file's name before its content | A medium or a program file that happens to hold a `model:` line is then not taken for a preset | 2026-11-10 |
| In a Web frontend an opened medium is imported, and a name that is taken follows the rule of Name & Save (AC-4) | user, 2026-11-10, batch 7 gate; LIB-7 AC-17 | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 7 gate): a module file or a disk file can be dropped in Calc-U-1600 (AC-19) | It follows from one command for every kind | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 7 gate): one file is kept while the session is busy (AC-7), as in Calc-U-1600 | Today's behaviour of one product, taken for the family | 2026-11-10 |
| An opened module file is plugged in when it is clear from the context where it goes (a machine with one slot; a module that fits only one slot); otherwise it is refused with a notice. It is plugged in as by the slot's picker, from where it lies | user, 2026-11-10, after the batch 7 gate; confirmed 2026-11-11 | 2026-11-10 |
| `.rom` is no name of a machine-code file: not in a dialog, not on a drop, not in the file types of Open With | user, 2026-11-10, after the batch 7 gate: the extension is not used anywhere | 2026-11-10 |
| Only a file dialog changes the folder in which the next file dialog starts (EC-7) | user, 2026-11-10, batch 7 gate; OQ-3; behaviour change 1600-15 | 2026-11-10 |
| An opened `.tape.yaml` is a medium; a WAV is a program file and is loaded (AC-19) | user, 2026-11-13 (question F-7 of `docs/walkthroughs/presets.md` §5.4); `docs/formats/media.md` §3 | 2026-11-13 |
| Aligned with `docs/formats/` after the Q&A round: `.magnetcard.yaml` (AC-11) | user rulings of 2026-11-13 (`docs/walkthroughs/presets.md` §5.2, §5.4); `docs/formats/` | 2026-11-13 |
| Aligned with `docs/formats/` after the Q&A round: a template card is never written (AC-12); the cassette tape as a media kind (AC-17) | user rulings of 2026-11-13 (`docs/walkthroughs/presets.md` §5.2, §5.4); `docs/formats/` | 2026-11-13 |
| Opening a `.keystrokes.yaml` writes its program into the running machine; `.rom.yaml` and `ti58c.memory.yaml` are refused with a notice (AC-11, AC-23) | user, 2026-11-13 (consistency pass after the Q&A round) | 2026-11-13 |
| OQ-4's "row 59-14 and its twin" are now rows 59-20 and 1600-15; a note says so | user, 2026-11-14 (question 9 of `docs/migration-plan.md`) | 2026-11-14 |
| Calc-U-59 takes the library's rule for the file named at the start (AC-14): the last argument that does not start with `-` and names an existing file (`opening::startFile`, Calc-U-1600's AC-21 rule); today it takes the last argument that is not an option value, without checking it. Done in M-56; ledger row 59-47 | user, 2026-11-16 (Q-1 of `docs/reviews/d-simplification.md`) | 2026-11-16 |
