# C59-7: Magnetic cards

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: C59-1 (the TI-59 behind the machine interface, which says
  when it needs a card), LIB-7 (the media model: instances in the save
  folder, names, the steps `card:` and `arm:`, the header key
  `magnetic-cards:`, `saveas:` for a card, the saved media of the Web
  frontend); also named here: LIB-6 (the preset loader), LIB-14 (the
  control bar), C59-2 (the `cue-card`
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
has two **sides**, 0 and 1 (`docs/formats/media.md` §1); a side holds one **bank**, a quarter of the
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
- [ ] **AC-2** — Given a card file, then it is a magnetic card file
  (`docs/formats/media.md` §1): format `ti59-magnetic-card`,
  `format-version: 3`, suffix `.magnetcard.yaml`, with the keys of
  `media.md` §1.1 (`card-name`, `description`, `template`, `saved`,
  `cue-card`, `sides` with `0` and/or `1`; each side `partition`,
  `data-type`, `bank`, `protection`, `checksum`, `encoding: register-hex`,
  `bytes` of exactly 30 rows), in the syntax of `docs/formats/README.md`
  §1. An example is in `media.md` §1.1 (ADR 0020; today's layout
  `Shared/CardFile.cpp:52-92, 293-311`).
- [ ] **AC-3** — Given a card file is read, then it is refused as a
  whole when it breaks `media.md` §1.1 or `README.md` §1: a missing side
  key, a side without exactly 30 rows, an unknown key at any level (the
  cue card included, `cue-card.md` §3). Keys are matched exactly; fixed
  values are read in any case (`README.md` §1, §4). Today an unknown key
  of the cue card is passed over (`Shared/CardFile.cpp:147-203`,
  `tests/shared_formats_tests.cpp:306-331`).
- [ ] **AC-4** — Given a card file of 2.3.0 (`.u59`, any of its
  layouts), then the product ignores it: it is not listed, not read and
  not changed, and the user converts it by hand (ADR 0037; behaviour
  change 59-1). Given a `.magnetcard.yaml` in an older format version,
  then it is outdated (LIB-7 AC-9). Any other `.magnetcard.yaml` that is
  not AC-2 is a card that cannot be read (LIB-7 EC-3). A card file ends in `.magnetcard.yaml`
  (`docs/formats/README.md`, formats at a glance; OQ-2).
- [ ] **AC-29** — Given the media model of LIB-7 (AC-46 there), then a
  magnetic card file gives it: its `card-name`; its `description`;
  whether it is a template (`template: true`; the fresh card is the
  implicit template and has no file, `media.md` §1, "Templates and
  instances", and §1.2); and no write-protect switch, because a bank's
  protection is the calculator's matter. An in-file card is saved by
  `saveas: { card: <card-name>, as: live|template, name: … }` or with
  `file:` in place of `name:` (`preset.md` §5.2.1); `protected:` is
  refused for a card.
- [ ] **AC-5** — Given a card is written, then its file is rewritten
  whole in the key order of `media.md` §1.1: only the side that was
  written is replaced, `card-name`, `description`, `template` and
  `cue-card` are kept as they were, and `saved` is set to the time of the
  write; comments are not kept (`media.md` §1.4). A template card is
  never written (`media.md` §1.4). A card that was opened from outside
  the save folder is written back to its own file; in a Web frontend it
  was imported into the save folder when it was opened (LIB-13 AC-4).
  As today, the side that was not written is kept as the file holds it;
  today the file is read again first for that
  (`tests/shared_formats_tests.cpp:272-296`,
  `Shared/SessionController.cpp:554-582`; LIB-7 EC-2; OQ-6).

The card reader
- [ ] **AC-6** — Given a TI-59, then its card reader is a media place of
  the kind magnetic card, pass-through, with the sides 0 and 1 (LIB-7
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
  left or top right corner, without dragging, then side 0 or side 1 is
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
  fresh card's first write. This is Name & Save done up front: the
  fresh card, the template, is never written (`media.md` §1.3) (LIB-7 AC-20;
  `Shared/CardFile.cpp:330-337`, `tests/shared_cooperative_tests.cpp:727`).
- [ ] **AC-16** — Given a card is fitted, by a button, a preset or a
  drop, while another is fitted, then it takes its place; while a card
  is in the reader, nothing can be fitted
  (`tests/shared_cooperative_tests.cpp:605`).

In presets and the harness
- [ ] **AC-17** — Given a preset for the TI-59, then it can have
  `- card: <reference>` (`{ file: }`, `{ name: }` or `{ mem: }`, with
  `side: 0|1`, default 0), which pushes that side through the reader and
  has finished when the card has passed and is fitted; and
  `- arm: <reference>` (no side), which fits the card for the user to
  feed by a corner (AC-12) and has finished when it is fitted
  (`preset.md` §5.2, §6.2). A card the ROM does not take is waited for
  at most the preset's `timeout:`, after which the step fails
  (`preset.md` §6.3; AC-7); as the last item it is not waited for. A cue
  card of another kind is fitted by a `cue-card:` section (C59-2)
  (today `Shared/StateFile.cpp:168-201`,
  `Shared/EmulatorSession.cpp:296-325`). The harness has commands of the
  same names. On a TI-58 or TI-58C these steps and `magnetic-cards:`
  refuse the preset (`preset.md` §7; `README.md` §5).
- [ ] **AC-18** — Given the header key `magnetic-cards:` (`preset.md`
  §3.1), then each of its items is an in-file card with `card-name`,
  optionally `description` and `cue-card` (`cue-card.md`), and `sides`
  with `0`, `1` or both; each side has `bank` (1 to 4, required),
  optionally `protected` (`true` or `false`, default `false`; `true` is
  written as `−n Write`) and `partition` (as C59-2 AC-2; default: the
  machine's when the card goes in), and the content of the bank:
  `program: |` in the keycode notation of `preset.md` §4.1, whose bare
  keycodes start at the bank's first step, and `data:` in the notation of
  `preset.md` §4.2. Contents outside the bank and two items with one
  `card-name` refuse the preset
  (`Shared/StateFile.cpp:210-237, 397-448`,
  `tests/shared_preset_stages_tests.cpp:70-138`).
- [ ] **AC-19** — Given an in-file card's side goes in, then the bank it
  holds is the one last written to that side since the preset was
  loaded, kept through a Reset, a power cycle or a Cold boot, until the
  next load or model switch (AC-25; `preset.md` §3.1); else the side as
  its item defines it, built with its `partition` or with the
  calculator's partition at that moment: steps and registers it does not
  list are read as 00, and a side that is not defined is fresh; the bank
  built is, byte for byte, the one the ROM writes for the same content,
  header and checksum included (`preset.md` §3.1;
  `Shared/SessionController.cpp:494-511`,
  `tests/shared_preset_stages_tests.cpp:119-138`).
- [ ] **AC-20** — Given a step with `{ mem: X }` and no item `X` in
  `magnetic-cards:`, then `X` is a fresh in-file card without a cue card,
  and a line of the log says so; it is saved to a file only by `saveas:`
  (`preset.md` §3.1, §5.2.1). Given a `{ name: }` or `{ file: }`
  reference to a card that does not exist, then it is an error
  (`preset.md` §5.2; `media.md` §1.3), and the preset is refused (EC-6).
  A name stands for one card throughout a preset: a later step that
  names it gets the card as the earlier steps left it, so that two steps
  can fill the two sides of one card (AC-27)
  (`Shared/SessionController.cpp:513-536`,
  `tests/shared_cooperative_tests.cpp:791-812`).

The cue card that shows
- [ ] **AC-21** — Given a TI machine, then it lists a product kind that
  gives the number of the library program that is selected now, or
  none (`capabilities.md`, product kinds; today read from the
  calculator's own registers, `Shared/CueCard.hpp:153-154`).
- [ ] **AC-22** — Given a machine, then at most one cue card is fitted
  at a time: fitting a magnetic card, a `cue-card:` section and the
  cue card the constant memory file (`ti58c.memory.yaml`) brings back
  (C59-3 AC-7) each take the place of
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
  today's stand-in, with the module's `description` as its title and its
  `module-name` as its ID (`modules.md` §2.3)
  (`roms/solid-state/cuecards.txt`, `tests/shared_formats_tests.cpp:433-461`,
  `Shared/CueCard.cpp:402-418`).
- [ ] **AC-24** — Given a library program's cue card covers a fitted
  magnetic card, then the corners do nothing, and the fitted card shows
  again when no library program is selected
  (`tests/shared_cooperative_tests.cpp:767-779`).

What lasts
- [ ] **AC-25** — Given a Reset, on any model, then what is fitted stays
  fitted, a cue card of any kind; a fitted magnetic card can be fed
  again afterwards by its corners (AC-12), and an in-file card then
  gives back what was written to it, as a real card would (`preset.md`
  §3.1); a card that is in the reader
  at that moment is taken out as by Eject Card (user, batch 5 and batch
  6 gates; behaviour change 59-17). The same holds for a power cycle
  and a Cold boot (user, 2026-11-10, batch 7 gate). Given a new machine is built, by a model
  switch or a preset, then nothing is fitted until the preset fits
  something, or, on a TI-58C, the constant memory file
  (`ti58c.memory.yaml`) brings its cue card back
  (C59-3 AC-7).
- [ ] **AC-30** — Given the four examples of Calc-U-59 with magnetic
  cards (C59-2 AC-10), then they are upgraded by the rows of `preset.md`
  §9.1 and compared as C59-2 AC-11 says: a `CARD: mem:X` … `END-CARD`
  block becomes an item of `magnetic-cards:`; `InsertCard: X:2` becomes
  `- card: { file: X.magnetcard.yaml, side: 1 }` (version 1's sides 1
  and 2 are 0 and 1); `InsertCard:` or `ArmCard:` on a card file that
  does not exist yet becomes an in-file card with that `cue-card`, fed
  with `mem:`, then `- saveas: { card: X, as: live, name: X }`;
  `ArmCard: mem:X` becomes `- card: { mem: X }` after the step that makes
  the machine wait for a card, or `- arm: { mem: X }` for the user to
  feed; and every card is fitted after it has passed, where today a
  `mem:` card is not.

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
  fresh card replacing a file only at its first write; `arm:` finished
  once the card is fitted, `card:` once it has passed; a card defined in a preset read by a
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
  since it was fitted, then a write replaces only the side that was
  written; the side not written is kept as the file holds it, and the
  other keys as AC-5 says (`media.md` §1.4; LIB-7 EC-2); so does today
  (`tests/shared_cooperative_tests.cpp:697`; OQ-6).
- **EC-4** — Given a write of a card fails, then a notice says so, the
  fitted card keeps the bank that was written, and the next write tries
  again (LIB-7 AC-14; `media.md` I-2).
- **EC-5** — Given a card file with a mark for the byte order at its
  start, or one that is not UTF-8, then it is refused as a card that
  cannot be read; today the message speaks of its size
  (`Shared/CardFile.cpp:383-408`).
- **EC-6** — Given a `card:` or `arm:` step whose `file:` or `name:`
  card does not exist, is outdated or cannot be read, then the preset is refused when it is read (LIB-7
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
- [x] **OQ-4** — *Superseded 2026-11-13 (`docs/formats/media.md` §1, `README.md` §1): the card file is magnetic card format 3 in simplified YAML (AC-2, AC-3); an unknown key of the cue card is still refused; comments are allowed and are not kept on a write; the last line may end without a line end.* *Decided as recommended (user, 2026-11-10, batch 7 gate).* Small changes in the card file (AC-2, AC-3): an
  unknown key of the cue card refuses the file; a ` #` in a title is no
  longer cut off, because comments in a card file go; the file ends with
  a line end. *Recommendation:* all three, with the new version.
- [x] **OQ-5** — *Decided as recommended (user, 2026-11-10, batch 7 gate).* "Side", "bank", "pass", "fresh" (of a magnetic card)
  and "library program" for the glossary, as defined above.
- [x] **OQ-6** — *Superseded 2026-11-13 (`docs/formats/media.md` §1.4): a write replaces only the side that was written and keeps the other side, `card-name`, `description`, `template` and `cue-card` as the file holds them (AC-5, EC-3).* *Decided as recommended (user, 2026-11-10, batch 7 gate).* Writing a card back (AC-5, EC-3). Today the file is
  read again before a write, so that a side another device wrote in the
  meantime is kept. LIB-7 writes what the product has and merges
  nothing. *Recommendation:* the family's rule, also for cards; with
  two devices on one iCloud folder the last write then wins for the
  whole card.
- [x] **OQ-7** — *Superseded 2026-11-13 (`docs/formats/preset.md` §5.2, `cue-card.md` §2): there is no `fit:` step; a magnetic card is fitted after `card:` has passed it, or by `arm:` (AC-17).* *Decided as recommended (user, 2026-11-10, batch 7 gate).* `fit:` (AC-17). `presets.md` PR-8 says it "places a
  magnetic card or cue card". *Recommendation:* `fit:` takes a magnetic
  card, on the TI-59; the other cue cards are fitted by `cue-card:`,
  which C59-2 already has for every model.
- [x] **OQ-8** — *Decided (user, 2026-11-10, batch 7 gate): no time. The card is armed with the card switch closed, and it is for the ROM to decide what to do with it; real hardware pulls the card as soon as the calculator can.* The time after which a card the ROM does not take is
  given back (AC-7). Nothing states one today; a pass takes about half a
  second. *Recommendation:* 2 seconds of emulated time.

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| *Superseded in part 2026-11-13: saved cards of 2.3.0 are ignored (AC-4).* The card format gets a name field and a new version; saved cards of 2.3.0 are refused with the hint | `media.md` MD-2, Q-4 (user); ADR 0037; behaviour change 59-1 | 2026-11-10 |
| Cards are always written in the current version | ADR 0020 | 2026-11-10 |
| Magnetic cards the product makes live in the save folder, also on the web; a card file opened from elsewhere is used and written back where it lies, and only a Web frontend imports it (LIB-13 AC-4) | ADR 0019 as amended; behaviour changes 59-6, 59-8 | 2026-11-10 |
| *Superseded 2026-11-13 (`docs/formats/preset.md` §5.2): the steps are `card:` and `arm:`; there is no `fit` step (AC-17).* `fit` places a card above the keys and affects only what the user sees; `insert` and `arm` go through the card reader, a pass-through media place | `presets.md` PR-8 | 2026-11-10 |
| The cue card of the running module program is a Calc-U-59 product kind | `capabilities.md`, product kinds | 2026-11-10 |
| Reset keeps the cue card that is fitted, on all models; a fitted magnetic card can then still be fed | user, 2026-11-10, batch 5 and batch 6 gates; behaviour change 59-17 | 2026-11-10 |
| *Superseded 2026-11-13 (`docs/formats/media.md` §1): magnetic card format 3, `.magnetcard.yaml`, name key `card-name` (AC-2).* Proposed in this spec and accepted (user, 2026-11-10, batch 7 gate): the version is 3.0 and the name is the line `Name:` after the first line (AC-2) | `media.md` MD-2 asks for a name field and names no layout | 2026-11-10 |
| *Superseded 2026-11-13 (`docs/formats/preset.md` §3.1): in-file cards are items of the header key `magnetic-cards:` (AC-18).* Proposed in this spec and accepted (user, 2026-11-10, batch 7 gate): a `medium:` section for a card has the fields of AC-18 | Today's `CARD: mem:` block, in the notation of version 2 | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 7 gate): the machine itself says when it waits for a card (AC-9) | LIB-7 AC-6; the pacing already needs "needs a card now" from the machine | 2026-11-10 |
| An empty side gives the ROM zeroes, and the ROM ends the read with an error (EC-2) | user, 2026-11-10, after the batch 7 gate | 2026-11-10 |
| A card name stands for one card throughout a preset (AC-20) | Phase 5, preset pass (`docs/walkthroughs/presets.md`, A-12); AC-27 keeps today's test of both sides in one file | 2026-11-11 |
| *Superseded 2026-11-13 (`docs/formats/preset.md` §6.3): a card the ROM does not take is waited for at most `timeout:`, then the step fails (AC-17).* A preset whose `insert:` step leaves the card at the reader while the machine gives no output for 30 minutes of emulated time ends there; a program that keeps giving output is never stopped. "Hanging" is measured by output, and the preset ends instead of going on; the other waits of a preset stay at one hour (reading confirmed by the user, 2026-11-11) | user, 2026-11-11: "If possible I'd stop a hanging preset after 30 minutes emulated time, but again: don't break a working program" | 2026-11-11 |
| Module cue cards come only from the module file; the bundled modules get a card like today's stand-in for every program without a pre-printed one; a program without a card takes off the module cue card (AC-22, AC-23) | user, 2026-11-13 (§5.4 of `docs/walkthroughs/presets.md`): every program has a cue card in reality; the fallback card was a stand-in for missing ones; the modules get one card per program, generated like the stand-in where none is defined; without one, any module cue card is taken off | 2026-11-13 |
| What is written to an in-file card lasts until the next preset load or model switch; a Reset, a power cycle or a Cold boot keeps it (AC-25; `preset.md` §3.1) | user, 2026-11-13 (§5.4 of `docs/walkthroughs/presets.md`); the format said "until the next load, reset or model switch" | 2026-11-13 |
| Aligned with `docs/formats/` after the Q&A round of 2026-11-13: sides 0 and 1 (AC-6, AC-12); card file is magnetic card format 3, `.magnetcard.yaml` (AC-2, AC-3, AC-4); what a card file gives the media model and `saveas:` for an in-file card (AC-29); a write replaces only the written side (AC-5, EC-3); steps `card:` and `arm:` with `timeout:` (AC-17, AC-27, EC-6); in-file cards under `magnetic-cards:` (AC-18, AC-19, AC-20); constant memory file name (AC-22, AC-25); stand-in module cue card (AC-23); upgrade rows of `preset.md` §9.1 (AC-30) | user rulings of 2026-11-13 (`docs/walkthroughs/presets.md` §5.2, §5.4); `docs/formats/` | 2026-11-13 |
| A 2.3.0 card file (`.u59`) is ignored: not listed, read or changed (AC-4) | user, 2026-11-13 (consistency pass after the Q&A round): "ignore them" | 2026-11-13 |
| "Fit Fresh Card" asks the name first and writes the instance at the first write: Name & Save done up front (AC-15; `media.md` §1.3) | user, 2026-11-13 (consistency pass after the Q&A round): today's flow, read as Name & Save done up front | 2026-11-13 |
