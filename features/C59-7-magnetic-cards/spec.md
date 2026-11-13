# C59-7: Magnetic cards

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: C59-1 (the TI-59 behind the machine interface, which says
  when it needs a card), LIB-7 (the media model: instances in the save
  folder, names, the steps `insert` and `arm`, `medium:` sections, the
  saved media of the Web frontend); also named here: LIB-6 (the place
  for the step `fit`), LIB-14 (the control bar), C59-2 (the `cue-card`
  topic and the content of a cue card), C59-6 (the Solid State ROM
  modules).
- Contracts and decisions: `docs/contracts/media.md` (MD-1 to MD-5, Q-4),
  `docs/contracts/presets.md` (PR-8), `docs/contracts/capabilities.md`
  (the media kind; product kinds of Calc-U-59); ADR 0019, 0020, 0037;
  `docs/behaviour-changes.md` rows 59-1, 59-6, 59-7, 59-8, 59-17.
- Facts: Calc-U-59 at `v2.3.0`; every `path:line` below is at that tag.
  What the ROM does with a card is from `rom/TI59-ROM-Guide.md:487-517`
  there.

Glossary terms used: Magnetic card, Cue card, Fit, Insert, Arm, Medium,
Instance, Media place, Partition. Words of this spec: a magnetic card
has two **sides**, 1 and 2; a side holds one **bank**, a quarter of the
TI-59's memory, numbered 1 to 4: 240 program steps or 30 data registers;
a **pass** is one run of a side through the card reader, in which the
calculator reads or writes that side's bank; a card is **fresh** while
neither side has been written; a **library program** is one of the
numbered programs of a Solid State ROM module, which the user selects
with `Pgm`. Words of LIB-7: kind, saved media.

## User Stories
- As a user of the TI-59, I want to keep programs and data on magnetic
  cards, read them back and write them, as on the real calculator.
- As a user, I want my cards to be in one place under names I gave
  them, in every frontend, the web included.
- As a user, I want to see the card I am working with above the keys,
  with what I wrote on it, and to feed a side with one tap.
- As the author of a preset, I want to define cards in the preset, feed
  them at the right moment and collect what the program writes.
- As a user of a library program, I want to see its cue card while it
  runs.

## Out of Scope
- The media model, names, the save folder, outdated files in general,
  the saved media of the Web frontend (owner: LIB-7); opening a card
  file from outside (LIB-13).
- The content and the look of a cue card and the section `cue-card:`
  (owner: C59-2); keeping the fitted cue card of a TI-58C (C59-3).
- The TI-59's ROM and what it does with a card; this spec states only
  what a user sees of it.
- Reading cards of an earlier format (ADR 0037: there is no converter).
- The sound of a card (an idea, `ideas/CardSound.md`).

## Acceptance Criteria

The card and its file
- [ ] **AC-1** — Given a magnetic card, then it has a name, a cue card
  and two sides, each empty or holding one bank; a bank is what the
  TI-59 writes: the partition, whether it is program or data, its
  number, whether it is protected, 30 registers and a checksum. Either
  side can hold any bank (`Shared/CardFile.hpp:53-65`,
  `tests/shared_cooperative_tests.cpp:661`).
- [ ] **AC-2** — Given a card file, then it is a text file in UTF-8 in
  the card format, version 3.0, which is the layout of version 2.0 with
  a new first line and the name after it (`media.md` MD-2; ADR 0020;
  today's layout `Shared/CardFile.cpp:52-92, 293-311`). Its lines, in
  this order, empty lines allowed between them:

  | Line | Content |
  |---|---|
  | `Calc-U-59 Card 3.0` | exactly this; the first line |
  | `Name: <name>` | required; the card's name as LIB-7 AC-3 leaves it |
  | `CUECARD:` and the lines of a cue card | optional; the keys of C59-2 AC-5 |
  | `SIDE: 1` or `SIDE: 2` | once for each side that holds a bank; an empty side is not written |
  | `HEADER:` | follows `SIDE:` |
  | `Partition: HH` | two hexadecimal digits, the first byte of the bank |
  | `DataType: program` or `data` | |
  | `Bank: n` | 1 to 4 |
  | `Protection: yes` or `no` | |
  | `Checksum: HH` | two hexadecimal digits |
  | `DATA:` | follows the five header lines |
  | `Rnnn: HH HH HH HH HH HH HH HH` | exactly 30 such rows, each the eight bytes of one register in hexadecimal; the number after `R` is for the reader's eye and is not read |
  | `END-CARD` | the last line; required |

  A card file has no comments: a ` #` in a value, such as a title, is
  part of the value. The file ends with a line end (OQ-4).

  For example, a card with a program on side 1 and nothing on side 2:

  ```
  Calc-U-59 Card 3.0
  Name: Diagnostic

  CUECARD:
  Template: MagnetCard
  Title: Diagnostic

  SIDE: 1
  HEADER:
  Partition: 1C
  DataType: program
  Bank: 1
  Protection: no
  Checksum: 02

  DATA:
  R000: 00 00 00 00 00 63 20 17
  (29 more rows)

  END-CARD
  ```
- [ ] **AC-3** — Given a card file is read, then it is refused when a
  key of a header is missing or unknown, a side has not exactly 30 rows,
  a side comes twice, `END-CARD` is missing, or a line is none the
  format knows; inside `CUECARD:` an unknown key is refused too (C59-2
  AC-5). Keys are read in any case. Today an unknown key of the cue card
  is passed over (`Shared/CardFile.cpp:147-203`,
  `tests/shared_formats_tests.cpp:306-331`).
- [ ] **AC-4** — Given a `.u59` file that starts with `Calc-U-59 Card `
  and another version, then it is outdated (LIB-7 AC-9): it is not read,
  and the hint names the card format, its version 3.0 and where it is
  described (ADR 0037; behaviour change 59-1). Any other file that is
  not AC-2, the two earlier binary layouts among them, is a card that
  cannot be read (LIB-7 EC-3). A card file ends in `.u59` (OQ-2).
- [ ] **AC-29** — Given the media model of LIB-7 (AC-46 there), then a
  magnetic card gives it: its name; as its description the title of its
  cue card; no template mark, because there are no card templates; and
  no write-protect switch, because a bank's protection is the
  calculator's matter. `saveas:` is not there for the card reader and
  refuses a preset when it is read.
- [ ] **AC-5** — Given a card is written, then its file holds the card
  as the product has it, whole: the sides that hold a bank, its cue card
  and its name, in version 3.0, and it reads back as the same card. A
  card that was opened from outside the save folder is written back to
  its own file; in a Web frontend it was imported into the save folder
  when it was opened (LIB-13 AC-4).
  Nothing of the file as it was is kept; today the side that was not
  passed is read from the file again first
  (`tests/shared_formats_tests.cpp:272-296`,
  `Shared/SessionController.cpp:554-582`; LIB-7 EC-2; OQ-6).

The card reader
- [ ] **AC-6** — Given a TI-59, then its card reader is a media place of
  the kind magnetic card, pass-through, with the sides 1 and 2 (LIB-7
  AC-6, AC-32); a TI-58 or TI-58C has none.
- [ ] **AC-7** — Given a side of a card is inserted, then one pass
  follows: the calculator reads the side's bank or writes one, as its
  ROM decides, and the card is given back by itself; given the ROM wrote
  a whole bank, then that side of the card holds it afterwards, the
  other side is as the product has it, and the card's instance is
  written (LIB-7 AC-12). Given the ROM does not take the card at once,
  as while a program runs that asks for none, then the card stays at the
  reader with the card switch closed, and the ROM takes it as soon as it
  can, as the real calculator pulls a card in; nothing gives it back by
  a time of its own (user, 2026-11-10, batch 7 gate)
  (`Core/TMC0501.cpp:704-757, 1190-1229`,
  `Shared/SessionController.cpp:554-582`).
- [ ] **AC-8** — Given a side with a bank is inserted while the
  calculator waits for a key, then the ROM reads it without a key press,
  as on the real calculator: the bank is stored when the card's
  partition is the calculator's and its checksum is right, and the
  display blinks otherwise (`reference/CoreArchitecture.md:603`,
  `rom/TI59-ROM-Guide.md:487-517`).
- [ ] **AC-9** — Given a keystroke program asks for a card (`n Write`,
  `n INV Write`), then the media place says that the machine waits for a
  medium (LIB-7 AC-6) until a side is inserted or R/S ends the wait;
  while the calculator only waits for a key it does not say so. Today
  this is told from the place in the ROM
  (`Shared/MachineModelInfo.hpp:70-102`, `Shared/EmulatorSession.cpp:309-325`).
- [ ] **AC-10** — Given a card in the reader, then Eject Card takes it
  out at once; a bank is kept as in AC-7 only when the ROM had written
  all of it (`Shared/SessionController.cpp:472-487`).

Fitting a card
- [ ] **AC-11** — Given a card is fitted, then it lies in the window
  above the top row of keys: its cue card shows, each top corner shows
  the number of the bank on that side, or nothing for an empty side, and
  the machine sees nothing of it (`presets.md` PR-8;
  `Shared/SessionController.cpp:454-463`).
- [ ] **AC-12** — Given a fitted card, when the user presses its top
  left or top right corner, without dragging, then side 1 or side 2 is
  inserted (AC-7), and the corner is lit while that side is in the
  reader; the card stays fitted afterwards. Each corner is a fifth of
  the card's width and 27.7 % of its height
  (`Shared/KeyboardLayout.hpp:27-49`,
  `tests/shared_cooperative_tests.cpp:548-567, 781`).
- [ ] **AC-13** — Given the control bar on a TI-59 (LIB-14 AC-27), then
  while no card is in the reader it has two buttons that both read "Crd"
  and differ by their icon and their name for a screen reader, "Fit
  Card" and "Fit Fresh Card", and while a card is in the reader it has
  "Eject Card" in their place; where the frontend has a File menu, it
  has "Fit Card…" and "Fit Fresh Card…" (`Qt6/app/ControlBar.cpp:55-99`,
  `Qt6/app/MainWindow.cpp:57-58`; the Swift frontend on iOS has the
  buttons only, C59-8 AC-18).
- [ ] **AC-14** — Given "Fit Card", then a dialog lists the saved
  magnetic cards by name, an outdated one as an entry that cannot be
  chosen (LIB-7 AC-9), and the chosen one is fitted; given none is
  saved, then it says "No cards found" (`Qt6/app/CardPickerDialog.cpp:54-66`,
  `App/Views/CardPickerView.swift:47-68`).
- [ ] **AC-15** — Given "Fit Fresh Card", then a dialog asks for a name,
  shows the saved cards, and fits a fresh card of that name, whose cue
  card has the name as its title; given a card of that name is saved,
  then it asks "Replace card `<name>`?" with "Its first write replaces
  both sides and the cue card." first; nothing is written until the
  fresh card's first write (LIB-7 AC-20;
  `Shared/CardFile.cpp:330-337`, `tests/shared_cooperative_tests.cpp:727`).
- [ ] **AC-16** — Given a card is fitted, by a button, a preset or a
  drop, while another is fitted, then it takes its place; while a card
  is in the reader, nothing can be fitted
  (`tests/shared_cooperative_tests.cpp:605`).

In presets and the harness
- [ ] **AC-17** — Given a preset for the TI-59, then it can have:
  `- fit:` with a magnetic card (LIB-7 AC-21), which fits it; a cue card
  of another kind is fitted by a `cue-card:` section (C59-2), on every
  model;
  `- insert: card-reader` with a side alone, which inserts that side of
  the fitted card and fails when none is fitted, or with a card and a
  side, which inserts it without fitting it; a side that is left out is
  side 1; the step ends when the card is back. A preset in which the
  ROM does not take the card has a defect: given the card is still at
  the reader and the machine has put out nothing new, on the display or
  on the paper, for 30 minutes of emulated time, then the preset ends
  there with a line in the log; a program that keeps giving output is
  never stopped by this, however long it runs (user, 2026-11-11; AC-7; LIB-7 AC-26);
  `- arm: card-reader` with a card and a side, which is inserted when
  the program asks for a card (AC-9)
  (`presets.md` PR-8; today `Shared/StateFile.cpp:168-201`,
  `Shared/EmulatorSession.cpp:296-325`). The harness has commands of the
  same names. On a TI-58 or TI-58C these steps refuse the preset when it
  is read.
- [ ] **AC-18** — Given a `medium:` section with `kind: card` (LIB-7
  AC-22), then it describes one bank, which is on whichever side of that
  card is inserted. It has `name:`, `bank:` (1 to 4, required),
  optionally `protected:` (`yes` or `no`) and `partition:` (as C59-2
  AC-2), and the content of the bank
  as C59-2 writes it: `steps: |`, whose keycodes start at the bank's
  first step, or `registers: |`, and `cue-card: |`. A step or a register
  outside the bank refuses the preset when it is read
  (`Shared/StateFile.cpp:210-237, 397-448`,
  `tests/shared_preset_stages_tests.cpp:70-138`).
- [ ] **AC-19** — Given such a card is inserted, then the bank it holds
  is the one a program wrote to it earlier in the same preset, else the
  one its section describes, built with its `partition:` or with the
  calculator's partition at that moment; a section without `steps:` and
  `registers:` gives an empty side; the bank built is,
  byte for byte, the one the ROM writes for the same content
  (`Shared/SessionController.cpp:494-511`,
  `tests/shared_preset_stages_tests.cpp:119-138`).
- [ ] **AC-20** — Given a card named in a step by a name that no card
  has, then it is a fresh card of that name, and a line of the log says
  so; its first write makes the instance (LIB-7 AC-25, AC-35). A name
  stands for one card throughout a preset: a later step that names it
  gets the card as the earlier steps left it, so that two steps can fill
  the two sides of one card (AC-27)
  (`Shared/SessionController.cpp:513-536`,
  `tests/shared_cooperative_tests.cpp:791-812`).

The cue card that shows
- [ ] **AC-21** — Given a TI machine, then it lists a product kind that
  gives the number of the library program that is selected now, or
  none (`capabilities.md`, product kinds; today read from the
  calculator's own registers, `Shared/CueCard.hpp:153-154`).
- [ ] **AC-22** — Given a machine, then at most one cue card is fitted
  at a time: fitting a magnetic card, a `cue-card:` section and the
  cue card `ti58c.mem` brings back (C59-3 AC-7) each take the place of
  the one before. The topic `cue-card` (C59-2 AC-6) names the cue card
  to show: while a library program is selected, the module cue card of
  that program as the module file defines it (`modules.md` §2.3), and,
  where it defines none, no module cue card, so that the fitted cue
  card shows, or none; otherwise the fitted cue card, if there is
  one. No card is made up at run time. It follows a change at the
  latest half a second later (`Shared/CueCard.cpp:402-418`,
  `Shared/SessionController.cpp:315-343`).
- [ ] **AC-23** — Given the module cue cards of the bundled modules,
  then the pre-printed ones are those of today: 25 for the Master
  Library and 21 for the Leisure Library; every other library program
  of every bundled module has a card in its module file that looks like
  today's stand-in, with the module's title and ID (`modules.md` §2.3)
  (`roms/solid-state/cuecards.txt`, `tests/shared_formats_tests.cpp:433-461`,
  `Shared/CueCard.cpp:402-418`).
- [ ] **AC-24** — Given a library program's cue card covers a fitted
  magnetic card, then the corners do nothing, and the fitted card shows
  again when no library program is selected
  (`tests/shared_cooperative_tests.cpp:767-779`).

What lasts
- [ ] **AC-25** — Given a Reset, on any model, then what is fitted stays
  fitted, a cue card of any kind; a fitted magnetic card can be fed
  again afterwards by its corners (AC-12); a card that is in the reader
  at that moment is taken out as by Eject Card (user, batch 5 and batch
  6 gates; behaviour change 59-17). The same holds for a power cycle
  and a Cold boot (user, 2026-11-10, batch 7 gate). Given a new machine is built, by a model
  switch or a preset, then nothing is fitted until the preset fits
  something, or, on a TI-58C, `ti58c.mem` brings its cue card back
  (C59-3 AC-7).
- [ ] **AC-30** — Given the four examples of Calc-U-59 with magnetic
  cards (C59-2 AC-10), then they are upgraded by these rules and
  compared as C59-2 AC-11 says: a `CARD: mem:X` block becomes a
  `medium:` section with `kind: card` and `name: X`; `ArmCard:` and
  `InsertCard:` become `- arm: card-reader` and `- insert: card-reader`
  with the card, written `mem:X` or by its name, and the side as a word
  of its own (`PDCM-1:2` becomes `PDCM-1 2`).

Every frontend
- [ ] **AC-26** — Given the Swift, Qt6 and Web frontends, then AC-11 to
  AC-16 hold in each, with the cards of that frontend's save folder; in
  the Web frontend they are kept from one visit to the next, where today
  they last for one page only (`Web/bindings.cpp:225-257`; LIB-7 AC-33,
  AC-36; behaviour change 59-6).
- [ ] **AC-27** — Given what the tests of Calc-U-59 check today about
  cards, then it holds, without the earlier formats: a blank card
  written and read
  back; a card fed only when a corner is pressed; a read that writes
  nothing; both sides in one file; the other side kept on a write; a
  fresh card replacing a file only at its first write; `arm` not
  waiting and `insert` waiting; a card defined in a preset read by a
  program; a card written and then inserted; a card that was armed and
  never used logged (`tests/shared_cooperative_tests.cpp:345-791`,
  `tests/core_tests.cpp:257`).
- [ ] **AC-28** — Given the GUI test of each frontend, then it has two
  scenarios under these IDs: for AC-15, a fresh card is fitted, written
  by a program and listed by "Fit Card"; for AC-12, a card is fitted and
  read when a corner is pressed (today
  `Web/tests/play-calculator.spec.js:266, 298-326`; ADR 0032).

## Edge Cases
- **EC-1** — Given a card whose partition is not the calculator's is
  read while the calculator waits for a key, then nothing is stored and
  the display blinks (AC-8).
- **EC-2** — Given an empty side is inserted while the calculator waits
  for a key, then the card reader gives the ROM zeroes for that side,
  and what follows is the ROM's: it ends the read with an error.
  Nothing is written to the card (user, 2026-11-10, after the batch 7
  gate; OQ-3). Today's test ejects such a card by hand
  (`tests/shared_cooperative_tests.cpp:738-750`).
- **EC-3** — Given a card's instance was changed by another device
  since it was fitted, then a write replaces it with the card as this
  product has it (AC-5; LIB-7 EC-2); today the other side is kept as
  that device left it (`tests/shared_cooperative_tests.cpp:697`; OQ-6).
- **EC-4** — Given a write of a card fails, then a notice says so, the
  fitted card keeps the bank that was written, and the next write tries
  again (LIB-7 AC-14; `media.md` I-2).
- **EC-5** — Given a card file with a mark for the byte order at its
  start, or one that is not UTF-8, then it is refused as a card that
  cannot be read; today the message speaks of its size
  (`Shared/CardFile.cpp:383-408`).
- **EC-6** — Given `fit:` in a preset for a card that is outdated or
  cannot be read, then the preset is refused when it is read (LIB-7
  AC-23).
- **EC-7** — Given a card with a protected bank is read, then the
  calculator behaves as the real one does with a protected program; this
  spec adds nothing to it.

## Technical Requirements (optional)
- The card format is an own format (ADR 0037) and is described in the
  reference chapter of the help (LIB-16); cards are always written in
  the current version (ADR 0020).
- The machine sees a card only through its media place; fitting is the
  frontend's and the session's matter (`presets.md` PR-8).
- The readers of the earlier card formats, their fixtures and the `CARD`
  records of `tests/golden/formats.txt` are not carried over.

## Open Questions
- [x] **OQ-1** — *Decided as recommended (user, 2026-11-10, batch 7 gate).* A power cycle and a Cold boot (AC-25). Decided for a
  Reset (user, 2026-11-10, batch 6 gate): a fitted cue card stays on every model, and a fitted
  magnetic card can still be fed. Today a module change, the printer's
  switch and Clean take the card away as Reset does. *Recommendation:*
  the same for a power cycle and a Cold boot, since the card lies
  outside the machine; only a new machine starts with nothing
  fitted.
- [x] **OQ-2** — *Decided as recommended (user, 2026-11-10, batch 7 gate).* `.bin` (AC-4). Today a card file can end in `.u59` or
  `.bin`; a `.bin` is one of the two binary layouts, which are no longer
  read. *Recommendation:* `.bin` is no longer a card; a `.bin` that is
  opened is refused like any file the product cannot open.
- [x] **OQ-3** — *Decided (user, 2026-11-10, after the batch 7 gate): an empty side feeds the calculator zeroes, and the ROM errors out; there is no fallback by time (OQ-8). The first task records on 2.3.0 how that error shows.* An empty side inserted while the calculator waits for
  a key (EC-2): whether the pass ends by itself is not verified; today's
  test ejects the card by hand after five seconds. *Recommendation:*
  verify on 2.3.0 in the first task; if the ROM never gives it back, the
  reader does after the time a pass takes.
- [x] **OQ-4** — *Decided as recommended (user, 2026-11-10, batch 7 gate).* Small changes in the card file (AC-2, AC-3): an
  unknown key of the cue card refuses the file; a ` #` in a title is no
  longer cut off, because comments in a card file go; the file ends with
  a line end. *Recommendation:* all three, with the new version.
- [x] **OQ-5** — *Decided as recommended (user, 2026-11-10, batch 7 gate).* "Side", "bank", "pass", "fresh" (of a magnetic card)
  and "library program" for the glossary, as defined above.
- [x] **OQ-6** — *Decided as recommended (user, 2026-11-10, batch 7 gate).* Writing a card back (AC-5, EC-3). Today the file is
  read again before a write, so that a side another device wrote in the
  meantime is kept. LIB-7 writes what the product has and merges
  nothing. *Recommendation:* the family's rule, also for cards; with
  two devices on one iCloud folder the last write then wins for the
  whole card.
- [x] **OQ-7** — *Decided as recommended (user, 2026-11-10, batch 7 gate).* `fit:` (AC-17). `presets.md` PR-8 says it "places a
  magnetic card or cue card". *Recommendation:* `fit:` takes a magnetic
  card, on the TI-59; the other cue cards are fitted by `cue-card:`,
  which C59-2 already has for every model.
- [x] **OQ-8** — *Decided (user, 2026-11-10, batch 7 gate): no time. The card is armed with the card switch closed, and it is for the ROM to decide what to do with it; real hardware pulls the card as soon as the calculator can.* The time after which a card the ROM does not take is
  given back (AC-7). Nothing states one today; a pass takes about half a
  second. *Recommendation:* 2 seconds of emulated time.

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| The card format gets a name field and a new version; saved cards of 2.3.0 are refused with the hint | `media.md` MD-2, Q-4 (user); ADR 0037; behaviour change 59-1 | 2026-11-10 |
| Cards are always written in the current version | ADR 0020 | 2026-11-10 |
| Magnetic cards the product makes live in the save folder, also on the web; a card file opened from elsewhere is used and written back where it lies, and only a Web frontend imports it (LIB-13 AC-4) | ADR 0019 as amended; behaviour changes 59-6, 59-8 | 2026-11-10 |
| `fit` places a card above the keys and affects only what the user sees; `insert` and `arm` go through the card reader, a pass-through media place | `presets.md` PR-8 | 2026-11-10 |
| The cue card of the running module program is a Calc-U-59 product kind | `capabilities.md`, product kinds | 2026-11-10 |
| Reset keeps the cue card that is fitted, on all models; a fitted magnetic card can then still be fed | user, 2026-11-10, batch 5 and batch 6 gates; behaviour change 59-17 | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 7 gate): the version is 3.0 and the name is the line `Name:` after the first line (AC-2) | `media.md` MD-2 asks for a name field and names no layout | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 7 gate): a `medium:` section for a card has the fields of AC-18 | Today's `CARD: mem:` block, in the notation of version 2 | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 7 gate): the machine itself says when it waits for a card (AC-9) | LIB-7 AC-6; the pacing already needs "needs a card now" from the machine | 2026-11-10 |
| An empty side gives the ROM zeroes, and the ROM ends the read with an error (EC-2) | user, 2026-11-10, after the batch 7 gate | 2026-11-10 |
| A card name stands for one card throughout a preset (AC-20) | Phase 5, preset pass (`docs/walkthroughs/presets.md`, A-12); AC-27 keeps today's test of both sides in one file | 2026-11-11 |
| A preset whose `insert:` step leaves the card at the reader while the machine gives no output for 30 minutes of emulated time ends there; a program that keeps giving output is never stopped. "Hanging" is measured by output, and the preset ends instead of going on; the other waits of a preset stay at one hour (reading confirmed by the user, 2026-11-11) | user, 2026-11-11: "If possible I'd stop a hanging preset after 30 minutes emulated time, but again: don't break a working program" | 2026-11-11 |
| Module cue cards come only from the module file; the bundled modules get a card like today's stand-in for every program without a pre-printed one; a program without a card takes off the module cue card (AC-22, AC-23) | user, 2026-11-13 (§5.4 of `docs/walkthroughs/presets.md`): every program has a cue card in reality; the fallback card was a stand-in for missing ones; the modules get one card per program, generated like the stand-in where none is defined; without one, any module cue card is taken off | 2026-11-13 |
