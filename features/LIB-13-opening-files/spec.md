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
- [ ] **AC-4** — Given a medium, then everything is checked first, and
  then it is imported into the save folder and put into its place:
  an outdated file, or one that cannot be read, is refused with the hint
  of LIB-7 AC-9, and nothing is imported;
  a file that already lies in the save folder is not imported again;
  given a medium of that kind already has the name, then the rule of
  Name & Save holds (LIB-7 AC-17, first column): the user is asked
  before an instance is replaced, and a bundled medium's name, a user
  template's and that of an instance in another place are refused;
  then a medium for a holding media place is inserted (LIB-7 AC-26),
  with its first side, the medium that was there being ejected first; a
  medium for a pass-through place gets what the product says follows; a
  module, and a medium whose place is not there now, is imported only,
  and a notice says where it can now be chosen.
  The file outside the save folder is never written (ADR 0019;
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
  with the reply of AC-3; where a user would be asked (AC-4), it is
  refused unless the word `replace` follows the path.
- [ ] **AC-10** — Given a product, then it can set the largest file each
  way in takes; a larger one is refused (AC-1).

### Calc-U-59

- [ ] **AC-11** — Given Calc-U-59, then its preset extensions are
  `.ti59`, `.ti58` and `.ti58c`; its one kind of medium is the magnetic
  card, `.u59`; its one file kind, the ASM overlay, is added by C59-4;
  it sets no limit on the size of a file
  (`Shared/SessionController.cpp:620-638`, `Shared/StateFile.cpp:554-557`).
- [ ] **AC-12** — Given a magnetic card is opened on a TI-59, then it is
  imported and fitted (AC-4; C59-7 AC-11); today it is fitted from where it
  lies and written back there (`Shared/SessionController.hpp:192-196`;
  behaviour change 59-6). Given a TI-58 or TI-58C, which has no card
  reader, then it is refused with a notice; today nothing happens
  (`tests/shared_cooperative_tests.cpp:593`).
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
  battery-backed module (`.card.yaml`) and the floppy disk
  (`.floppy.yaml`); its file kinds, asked in this order, are the BASIC
  program and the machine-code program, told by what C1600-2 says the
  file is (C1600-2 AC-1): a BASIC listing or tokenized BASIC is a BASIC
  program; machine code with a header is a machine-code program;
  headerless bytes only when C1600-2 says they look like code and the
  name ends in `.bin` or `.rom`; a cassette WAV goes to the loader of
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
- [ ] **AC-19** — Given a module file or a disk file is opened, then
  AC-4 holds: it is imported, and a disk is inserted when the drive is
  there; today such a file can only be imported from the menu of the Web
  frontend (`Shared/MediaLibrary.cpp:59-107`). A WAV is never taken as a
  tape here; a tape is imported from the menu (LIB-7 AC-43).
- [ ] **AC-20** — Given a drop or Open With, then a file of more than
  1 MiB is refused; today it is passed over without a word (OQ-4) (`Qt6/app/MainWindow.cpp:768-779`,
  `Web/src/CalcU1600.jsx:16, 175`); the file dialogs of the menu have no
  limit.
- [ ] **AC-21** — Given the ways in of Calc-U-1600, then they are those
  of today: a drop in Qt6 and in the Web frontend; the items of the File
  menu; in Qt6 on macOS Open With, for the three preset extensions as
  the product's own and for `.bas`, `.bbin`, `.bin` and `.rom`
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
  then it is imported only (AC-4).
- **EC-3** — Given a preset opened in a Web frontend alone, which names
  other files, then it is refused when it is read (LIB-6 EC-5).
- **EC-4** — Given a medium is opened while its place holds one that is
  being passed through or written, then it is refused with a notice,
  and nothing is imported; today Calc-U-59 does nothing and says
  nothing
  (`Shared/SessionController.cpp:399`).
- **EC-5** — Given a file that cannot be read, then the command is
  refused and the notice names the file.
- **EC-6** — Given the user says no to replacing a medium (AC-4), then
  nothing is imported and nothing is put into a place.

## Technical Requirements (optional)
- No frontend decides what a file is; it hands the file over
  (`frontend.md` SP-3, FE-2).
- Which kinds of file a product claims in the file manager of each
  platform is product data of its frontends (AC-14, AC-21).
- The library tells presets and media; a product's file kinds and their
  rules are supplied by the product (`frontend.md` SP-3, I-4).

## Open Questions
- [ ] **OQ-1** — A medium that lies outside the save folder
  (`frontend.md` Q-4). ADR 0019 lets such a file be used only when a
  preset names it or the user puts it into the save folder; Calc-U-59
  fits a dropped card and writes it back where it lies today.
  *Recommendation:* AC-4: opening it puts it into the save folder for
  the user (an import) and then into its place; a medium of the same
  name is replaced only after the user said yes.
- [ ] **OQ-2** — A file named at the start of the program (AC-14,
  AC-21): Calc-U-59's Qt6 frontend opens it, Calc-U-1600's passes it
  over. *Recommendation:* both open it, through this command.
- [ ] **OQ-3** — A file dialog and the folder it starts in (LIB-9
  AC-29). Today a drop in Calc-U-1600 changes the folder the next file
  dialog starts in. *Recommendation:* only a file dialog does.
- [ ] **OQ-4** — Refusals that are silent today get a notice: a file
  that is nothing the product opens, after Open With or at the start; a
  file over 1 MiB in Calc-U-1600; a magnetic card on a model without a
  reader; a card while one is in the reader (AC-1, AC-12, AC-20, EC-4). *Recommendation:* all three; too small
  for a row each, one line in row 59-14 and its Calc-U-1600 twin.
- [ ] **OQ-5** — "Open" (a file) and "file kind" for the glossary, as
  defined above.

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| One "open file" command on the app session for a drop, Open With and a file picker that takes any file; the library tells presets and media, a product adds its file kinds | `frontend.md` SP-3 (user) | 2026-11-10 |
| Today's accepted files stay; where a platform has no drop today, none is added | `frontend.md` SP-3 (user) | 2026-11-10 |
| A magnetic card elsewhere is no longer opened and written back in place | ADR 0019; behaviour change 59-6 | 2026-11-10 |
| Each product's file kinds are product data of this spec; Calc-U-1600's rest on the loaders of C1600-2 | user, 2026-11-10, batch 5 gate | 2026-11-10 |
| Proposed in this spec, for the gate: a file with a preset extension is a preset whatever it holds (AC-1, EC-1) | An outdated preset then gets its hint, not "cannot open" | 2026-11-10 |
| Proposed in this spec, for the gate: the order of the tests of AC-1, a file's name before its content | A medium or a program file that happens to hold a `model:` line is then not taken for a preset | 2026-11-10 |
| Proposed in this spec, for the gate: a name that is taken follows the rule of Name & Save (AC-4), and the harness needs the word `replace` (AC-9) | One rule for a name that is taken; LIB-7 AC-17 | 2026-11-10 |
| Proposed in this spec, for the gate: a module file or a disk file can be dropped in Calc-U-1600 (AC-19) | It follows from one command for every kind | 2026-11-10 |
| Proposed in this spec, for the gate: one file is kept while the session is busy (AC-7), as in Calc-U-1600 | Today's behaviour of one product, taken for the family | 2026-11-10 |
