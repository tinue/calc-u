# LIB-7: Media and save folder

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: LIB-4 (media places and module slots in the control state),
  LIB-6 (the place for keys, sections and steps of a preset), LIB-9 (the
  save folder and the tape folder), LIB-11 (the harness); also named
  here: LIB-2 (attach, the write-protect switch as a control), LIB-3
  (topics, notices, suspend), LIB-14 (where pickers and menu items go),
  LIB-15 (the persistent file system of a Web frontend), LIB-18 (what
  follows a module change).
- Contracts and decisions: `docs/contracts/media.md` (MD-1 to MD-5, I-1
  to I-4, Q-1, Q-5), `docs/contracts/presets.md` (PR-8, PR-11),
  `docs/contracts/capabilities.md` (the media kind, Q-2),
  `docs/contracts/machine.md` (M-8, media places); ADR 0019, 0020, 0025,
  0037; `docs/behaviour-changes.md` rows 59-6, 59-7, 59-9, 59-11,
  1600-1, 1600-4 to 1600-6, 1600-18, 1600-19.
- Facts: Calc-U-59 at `v2.3.0`, Calc-U-1600 at `b4ea8c8`; every
  `path:line` below is at those pins.

Glossary terms used: Medium, Template, Instance, Name & Save, Insert,
Fit, Arm, Save folder, Media place (pass-through, holding), Outdated,
Module, Memory module. A **kind** of medium is what one sort of media
place takes (a magnetic card, a floppy disk, a battery-backed module, a
cassette tape); each kind has its file suffix and its format, which its
product supplies (AC-46). A **user template** is a template the user
made, a file in the save folder that says it is one; the glossary's
Template covers both. The **saved media** are
the media in the save folder and the tape folder, and the list of them
that a Web frontend shows.

What the three status columns mean for this feature:
- **Library:** the media model, the save rules, autosave, the media
  topic, the pickers of the two component sets, the preset keys, steps
  and commands exist and are shown to work with a machine and a kind of
  medium built for the tests.
- **Calc-U-59:** magnetic cards are media of this model in every
  frontend; the card reader, the card format and fitting are C59-7, and
  this column is complete only together with it.
- **Calc-U-1600:** memory modules, floppy disks and cassette tapes are
  media of this model in both frontends.

## User Stories
- As a user, I want what my machine writes onto a medium to be kept
  without my doing anything, and never to lose a write.
- As a user, I want the media I am given with the product to stay as
  they are, so that I can always start again from them.
- As a user, I want to save a medium under a name of my own and to find
  it again in the picker of its place.
- As the author of a preset, I want to name the media a preset needs, to
  insert and eject them from its steps, and to save results.
- As a user of a Web frontend, I want to get my media out of the browser
  and into it.

## Out of Scope
- Where the save folder and the tape folder are (owner: LIB-9); the
  persistent file system of a Web frontend and two tabs (LIB-15).
- What a medium's content means and the format of each kind: the
  magnetic card and its new version, the card reader, fitting, the
  corner taps and the cue cards (owner: C59-7); `.card.yaml`, which
  stays as it is, `.floppy.yaml`, which goes to version 2 with the
  sides `0` and `1` (AC-37), and the cassette tape
  definition `.tape.yaml` with its WAV (Calc-U-1600; OQ-6;
  `docs/formats/media.md` §3).
- Which places a model has and what fits them (LIB-1, LIB-2, LIB-4);
  what a module change does to the machine (LIB-18 AC-29).
- Opening a medium by drop, Open With or a file picker (owner: LIB-13;
  `frontend.md` Q-4).
- The host drive, the constant memory file `ti58c.memory.yaml`, traces and screenshots: they are in or
  near the save folder and are not media (`media.md` Q-2).
- Renaming a medium, and merging the instances of two devices (new
  functions; none today).

## Acceptance Criteria

### Library

The model
- [ ] **AC-1** — Given a medium, then it is a template or an instance. A
  template is bundled with the product, or is a file in the save folder
  that says it is one (a user template); an instance is a file in the
  save folder, or a medium file the user opened from elsewhere, which
  stays where it lies and is saved there (LIB-13 AC-4); a Web frontend
  instead imports such a file into its save folder (AC-30), because a
  browser cannot write it back. A bundled file
  is never written, and a user template only
  by a save as a template (AC-17) (`media.md` I-1; ADR 0019).
- [ ] **AC-2** — Given a medium file, then it carries the medium's name
  inside; the file name counts only for a file that carries none
  (`media.md` MD-2, MD-5).
- [ ] **AC-3** — Given a name for a medium, then spaces around it are
  dropped and runs of spaces become one, and it is refused when it is
  empty or holds a character a file name cannot hold: `/ \ : * ? " < > |`
  or a control character. Two names that differ only in upper and lower
  case are the same name (`media.md` MD-3; behaviour change 1600-5;
  OQ-2).
- [ ] **AC-4** — Given a medium is written under a name, then its file
  is the name with the suffix of its kind, in the save folder; given a
  file of that name exists and holds a medium with another name, then
  the new file gets the first free numbered name (`foo-1`) (`media.md`
  MD-5).
- [ ] **AC-5** — Given a medium is looked up by its name and kind, then
  the bundled ones are searched first and the save folder second
  (Calc-U-1600 `Core/Connector/NamedFileCatalog.hpp:124-170`). An
  instance never carries the name of a bundled template
  (`docs/formats/preset.md` §5.2.1). Two files of the save folder with one name are both listed;
  a save by that name replaces, and a preset finds, the one whose file
  name is the one the name gives (AC-4), and if neither is, the first
  by file name (`media.md` MD-5; OQ-1). In a module slot the modules that are not
  media (LIB-4 AC-1) and the media of the slot's kind are one list of
  names.
- [ ] **AC-6** — Given a media place of a machine, then its capability
  states its kind, whether it is pass-through or holding, and its sides,
  and gives: inserting and ejecting a medium, which medium is in and
  which side, the content for saving with a counter that changes with
  it, whether the machine waits for a medium now, and, where the medium
  has one, the write-protect switch (`capabilities.md`, the media kind;
  `presets.md` PR-8).
- [ ] **AC-7** — Given an app session, then a state topic lists the
  media of the product: for each kind the templates and the instances,
  each with its name, its description, whether it is outdated, and the
  place it is in now, if any; and for each media place whether it
  holds a medium and whether the machine waits for one. The folders are
  read again whenever the product wrote to them and whenever a picker or
  the saved media list is opened; they are not watched for changes from
  outside (LIB-4 OQ-3 as decided).
- [ ] **AC-46** — Given a kind of medium, then its product supplies the
  reading and the writing of its files, and this is all the library
  knows of a file: its name, its description, whether it is a template,
  its format version and whether that is outdated, with the words of the
  hint (AC-9), whether it is write-protected, and its content, which the
  library hands to the media place and takes back from it without
  looking into it (`capabilities.md` Q-2).

Pickers
- [ ] **AC-8** — Given the picker of a module slot or of a holding media
  place, then it lists, in this order and each group sorted by name: the
  empty place; the templates, the bundled ones before the user's; the
  instances; and, in a module slot, the modules that are not media (LIB-4
  AC-1). An entry shows the name, and its description as its tooltip
  (ADR 0025; Calc-U-1600 `Qt6/app/ControlBar.cpp:41-62`,
  `Web/src/ControlBar.jsx:24-43`). The exception is a Solid State ROM
  module: it shows its `description` as its label, without a tooltip,
  and the modules are in the order of their `menu-sort`, equal values in
  the loader's order, as today (`docs/formats/modules.md` §2.1; C59-6
  AC-8).
- [ ] **AC-9** — Given an outdated file of that kind in the save folder,
  then the picker shows it as an entry that cannot be chosen, marked
  outdated, with a hint that names the format, its current version and
  where it is described; the log gets one line per outdated file at each
  start (`media.md` MD-4; ADR 0037; behaviour change 1600-6).
- [ ] **AC-10** — Given an entry is chosen, then what the old medium
  still has to save is written first (AC-13); in a holding media place
  the new medium is in at once, without a power cycle; in a module slot
  it is a replace of the module (LIB-2 AC-35; what follows: LIB-18 AC-21
  for the TI models, AC-29 for the Sharp ones). Given the
  medium cannot be read, then the place is empty and a notice says why.

Autosave
- [ ] **AC-11** — Given an instance in a holding place whose content
  changes, then its file holds the new content at the latest half a
  second after the change, counted in the wall time the host passes
  (LIB-3 AC-2), and is not written while nothing changes; in the
  harness, which has no wall clock, it is written at the end of each
  command (Calc-U-1600
  `Shared/ModuleBay.cpp:233-274`, `Shared/FloppyBay.cpp:168-188`).
- [ ] **AC-12** — Given an instance in a pass-through place, then its
  file is written when the medium is given back, if the machine wrote to
  it (Calc-U-59 `Shared/SessionController.cpp:275, 472-487, 554-582`).
- [ ] **AC-13** — Given an instance with a change not yet written, then
  it is written at once: before it leaves its place, before an operation
  starts, before the machine is replaced, when the session is suspended
  (LIB-3 AC-24), and when the product quits (Calc-U-1600
  `Shared/AppSession.cpp:59-84`, `Qt6/app/MainWindow.cpp:323-329`).
- [ ] **AC-14** — Given a write of an instance fails, then a notice
  says so, once for that medium until a write of it succeeds, and the
  next change tries again; today both products fail without a word (Calc-U-1600
  `Shared/ModuleBay.cpp:245-247`; OQ-5).
- [ ] **AC-15** — Given a template the user chose in a picker, when the
  machine writes to it, then the template is not written, and what was
  written lives in the place until Name & Save makes it an instance or
  the medium leaves the place, as today (Calc-U-1600
  `Shared/ModuleBay.cpp:425`); the same holds for a template a preset
  names, by name or by file (AC-25).
  Given an in-file card a preset defines itself (`mem:`), then nothing
  is written for it unless `saveas:` saves it; saved `as: live`, it is
  that instance from then on and later writes go into its file
  (`preset.md` §3.1, §5.2.1).

Name & Save
- [ ] **AC-16** — Given a place that holds a medium which is not an
  instance and whose content can change, then Name & Save can be used
  for it; it asks for the name only, and for the write-protect switch
  where the medium has one that is still off (`media.md` MD-5;
  Calc-U-1600 `Shared/ModuleBay.hpp:81-86`, `Qt6/app/MainWindow.cpp:131-157`).
- [ ] **AC-17** — Given a save under a name, by Name & Save, by `saveas`
  or by the harness, then these rules hold in this order (`media.md`
  MD-3, I-3, I-4; Calc-U-1600 `Core/Connector/MediaSave.hpp:73-95`):

  | Case, always among media of the same kind | Name & Save | `saveas` with `as: live` | `saveas` with `as: template` |
  |---|---|---|---|
  | the name is refused by AC-3 | refused | refused | refused |
  | the name is that of a bundled medium | refused | refused | refused |
  | the name is that of a user template | refused | refused | replaces it |
  | the name is that of an instance that is in another place now | refused | refused | refused |
  | the name is that of an instance | asks before replacing | replaces | refused |
  | the file the name gives exists and cannot be read or is outdated | a numbered file (AC-4) | a numbered file | a numbered file |

  The harness command `saveas` follows the two `saveas` columns. The
  `file:` form writes exactly the file it names, with the file name
  without its suffix as the medium's name; it is refused for a bundled
  file, and for a user template unless it saves a template.

- [ ] **AC-18** — Given a save succeeded, then the place holds the saved
  medium under its new name without a power cycle; saved as an instance
  it is autosaved from then on, saved as a template it is not
  (Calc-U-1600 `Shared/ModuleBay.cpp:222-229`).
- [ ] **AC-19** — Given the write-protect switch is asked for with a
  save, then the file says so, the medium in the machine is protected
  at once, and the machine goes through a power cycle, as if a module
  had been plugged in (LIB-2 AC-28, AC-32; user, 2026-11-11; behaviour change
  1600-17); no control switches it off again, as today (Calc-U-1600
  `Shared/ModuleBay.cpp:208-220`).
- [ ] **AC-20** — Given a place where a new, empty medium is started
  under a name before anything is written (a fresh magnetic card, a tape
  to record), then the name is asked first, and when an instance of that
  name exists the user is asked before it is replaced; the file is
  replaced only at the first write; this is Name & Save done up front,
  and the template itself is never written (`media.md` §1.3) (Calc-U-59
  `tests/shared_cooperative_tests.cpp:727`; Calc-U-1600
  `Qt6/app/MainWindow.cpp:381-393`).

Media in presets
- [ ] **AC-21** — Given a preset, then the module slot keys
  (`slot-1:`, `slot-2:`) and the media steps (`card:`, `arm:` on the
  TI-59; `floppy:`, `tape:` on the Sharp models) take a reference as
  `docs/formats/README.md` §3 defines it: a mapping with exactly one of
  `name:` or `file:`, on the TI-59 also `mem:` for an in-file card of
  `magnetic-cards:`, plus `side: 0|1` where the place has sides
  (`card:`, `floppy:`; default 0), as in `slot-1: { name: My programs }`
  and `- floppy: { file: disks/work.floppy.yaml, side: 1 }`; a bare
  value is an error. `saveas:` is a mapping of its own (`preset.md`
  §5.2.1). The keys `slot-1-file:` and `floppy-file:` of today are
  gone; a preset that uses them is refused (LIB-6 AC-30).
- [ ] **AC-22** — Given media defined inside a preset, then they are
  the in-file magnetic cards of the TI-59's header key
  `magnetic-cards:` (`preset.md` §3.1; owner: C59-7), read before the
  machine is built and saved only by `saveas:`; no other kind of medium
  is defined inside a preset (today Calc-U-59
  `Shared/StateFile.cpp:397-448`).
- [ ] **AC-23** — Given a preset names a medium by `file:` or `name:`
  that does not exist, is outdated or cannot be read, then the preset is
  refused when it is read, for an outdated file with the hint of AC-9
  (LIB-6 AC-7; `media.md` MD-4; `preset.md` §5.2). A `mem:` name the
  preset does not define is a fresh in-file card (`preset.md` §3.1).
- [ ] **AC-24** — Given a preset names an instance by its name, then a
  module in a slot key is fitted with that instance before the machine
  starts, and a medium in a step is inserted as that instance; either is
  autosaved as always.
- [ ] **AC-25** — Given a medium whose file says `template: true`, then
  it is a template wherever it lies and however it was named (by a
  preset by name or by file, by the user, in a picker), and it is never
  written: what the machine writes lives in the place, and Name & Save
  is offered (AC-15, AC-16). Given a medium whose file does not say so,
  then it is an instance wherever it lies, and every write is saved into
  that file in place, also when a preset named it by file; a preset
  that is run again then starts from what the last run left. Calc-U-1600
  does so today (`docs/formats/media.md`, Templates and instances;
  `modules.md` §1.8; ADR 0019 as amended 2026-11-13). The log names a
  medium that was written by its kind and its name tag, never by its
  path.

Steps and commands
- [ ] **AC-26** — Given a preset, then these steps exist, and the
  harness has commands of the same names and values (`presets.md` PR-8;
  a mapping written as LIB-11 AC-22 says).
  Removable media are inserted and ejected by steps, never by header
  keys: on the Sharp models `floppy:` and `tape:` insert, replacing what
  is in (EC-5), and `eject: floppy` or `eject: tape` empties the place,
  which its selector then shows as empty (EC-6); modules and other
  installed peripherals are header keys only and are fitted before the
  machine starts (`preset.md` §3, §5.2; user, 2026-11-13). The table
  below is aligned with `preset.md` §5.2 after the Q&A round:

  | Step | Value | What it does | Emulated time (LIB-6 AC-19) |
  |---|---|---|---|
  | `card:` [TI-59] | a reference (AC-21) by `file:`, `name:` or `mem:`, with `side: 0\|1` (default 0) | pushes that side through the card reader; the step has finished when the card has passed and is fitted; a card the ROM does not take is waited for within `timeout:` (`preset.md` §6.3; C59-7 AC-17) | needs it |
  | `arm:` [TI-59] | a reference by `file:`, `name:` or `mem:`, no side | fits the card for the user to feed by a corner; finished when it is fitted (C59-7) | none |
  | `floppy:` [PC-1600 with CE-1600P] | a reference by `file:` or `name:`, with `side: 0\|1` (default 0) | the disk is in at once and stays; one in the drive is ejected first (EC-5) | none |
  | `tape:` [with CE-150 or CE-1600P] | a reference by `file:` or `name:` | the tape is in the cassette interface; one that is in is ejected first (EC-5) | none |
  | `eject:` [Sharp] | `floppy` or `tape` | takes the medium out (AC-13 first); the place shows as empty (EC-6) | none |
  | `saveas:` | a mapping: `place:` (or `card:` on the TI-59), `as: live\|template`, optionally `protected: true`, and `name:` or `file:` (`preset.md` §5.2.1) | AC-17 to AC-19; a failure fails the preset | none |

  (Calc-U-1600 `Core/Preset/PresetFile.cpp:295-367`; Calc-U-59
  `Shared/StateFile.cpp:168-201`, `Shared/KeySequencer.cpp:214-219`.)
- [ ] **AC-27** — Given a medium was armed and never passed through when
  the next machine is built, then a line of the log says so (Calc-U-59
  `Shared/SessionController.cpp:539-546`).
- [ ] **AC-28** — Given the harness, then it also has: `media` (the
  topic of AC-7), `media import <file>` (AC-30), `media export <kind>
  <name> <path>` and `media delete <kind> <name>` (AC-31). A run of the
  harness has an empty save folder of its own unless it is given one
  (LIB-11 AC-8); the bundled media are always there.

Getting media in and out
- [ ] **AC-29** — Given a Web frontend, then its File menu has the items
  for importing media and for the saved media (LIB-14 AC-35), and its
  media are in the persistent file system (LIB-15 AC-5).
- [ ] **AC-30** — Given a file is imported, then it must be a medium of
  a kind the product has, in the current version; it is written into the
  save folder under the file name its inner name gives (AC-4), as the
  template or instance it says it is. An import is not a save under
  AC-17: it is refused when a medium of that kind, bundled or saved,
  already has the name, and when the file is outdated or cannot be read
  (Calc-U-1600 `Shared/MediaLibrary.cpp:59-107`).
- [ ] **AC-31** — Given the saved media, then the list shows the media
  of the save folder by kind and name, marks a template and a medium
  that is in a place now, hands a medium's file to the user (LIB-15
  AC-9), and deletes one that is in no place after the user confirmed
  (Calc-U-1600 `Web/src/Dialogs.jsx:248-298`,
  `Shared/MediaLibrary.cpp:24-116`; OQ-4).

### Calc-U-59

- [ ] **AC-32** — Given Calc-U-59, then its one kind of medium is the
  magnetic card, with the suffix `.magnetcard.yaml` (format 3,
  `media.md` §1), and its one media place is the card reader of the
  TI-59, which is pass-through with the sides 0 and 1 (C59-7; LIB-4
  AC-12). There are no bundled card files; its one template is the fresh
  card, which has no file (`media.md` §1.2).
- [ ] **AC-47** — Given any TI model, then the picker of `slot-1` is
  that of AC-8 with one group: the empty slot and the 14 Solid State ROM
  modules, which are not media (LIB-4 AC-11; C59-6); choosing one
  replaces the module, and a power cycle follows (LIB-18 AC-21;
  behaviour changes 59-9, 59-11).
- [ ] **AC-33** — Given the Swift, Qt6 and Web frontends, then AC-1 to
  AC-5 hold for magnetic cards in the save folder of each (LIB-9 AC-13
  to AC-15); what that changes for the user, and the card file itself,
  is C59-7's (behaviour change 59-6).
- [ ] **AC-34** — Given a card that is read and written through the
  reader, then its instance is written when it is given back (AC-12);
  what that write keeps of the other side is C59-7's.
- [ ] **AC-35** — Given a preset of Calc-U-59, then `card:` (with
  `side: 0|1`) and `arm:` (no side) take a card by `name:`, `file:` or
  `mem:` (AC-21); a `name:` or `file:` card that does not exist refuses
  the preset (AC-23); a `mem:` name the preset does not define is a
  fresh in-file card (`preset.md` §3.1); today a card name that no card
  has stands for a new, empty card (`Shared/SessionController.cpp:513-536`);
  a card file is written in place unless it is a template (AC-25).
- [ ] **AC-36** — Given the Web frontend, then it has the import and the
  saved media of AC-29 to AC-31 for cards; today it has a download per
  card and an upload in the card picker (`Web/src/Dialogs.jsx:93-103`).

### Calc-U-1600

- [ ] **AC-37** — Given Calc-U-1600, then its kinds of media are the
  battery-backed module (`.card.yaml`), the floppy disk
  (`.floppy.yaml`) and the cassette tape (a tape definition
  `.tape.yaml` with its `.wav`, `media.md` §3, version 1, new); the
  module definition stays at `format-version: 1`, the floppy disk goes
  to version 2, whose sides are `0:` and `1:` in place of `a:` and `b:`
  (`media.md` §2; `preset.md` §9.4), and both are read and written as
  today otherwise, the name being `module-name:`, `disk-name:` or
  `tape-name:` and a template saying `template: true` (`Core/Connector/MemoryCardDefinition.hpp:24-25, 168-172`,
  `Core/Connector/FloppyImageFile.hpp:23-54`; behaviour change 1600-1).
- [ ] **AC-38** — Given the picker of a module slot, then it is that of
  AC-8, with the modules that fit the slot (LIB-2 AC-2); choosing one
  replaces the module with a power cycle that keeps the memory (LIB-18
  AC-29), and the other slot and the disk in the drive are not touched.
  Today both slots and the disk are read again from their files, and
  what a template held is lost (`Shared/AppSession.cpp:308-312`,
  `Shared/ModuleBay.cpp:59-118`; behaviour change 1600-4).
- [ ] **AC-39** — Given the floppy disk controls, then they are: the
  picker of AC-8; a button "0" or "1" that turns the disk over, which is
  an eject and an insert of the other side; a Name & Save button; and a
  lamp that is green while the drive's motor runs
  (`Qt6/app/ControlBar.cpp:174-215, 355-370`, `Shared/FloppyBay.cpp:200-210`).
- [ ] **AC-40** — Given Name & Save for a module or a disk, then the
  dialog is today's ("Instance name:" or "Disk name:", and for a module
  with the switch "Write-protect (switch to read-only, can't be
  undone)"); an instance of that name is replaced after the user was
  asked, where today the name is refused; a name with `/` or `:` is
  refused, where today the character is replaced
  (`Qt6/app/MainWindow.cpp:131-212`, `Shared/ModuleBay.cpp:137-193`;
  `media.md` MD-3; behaviour change 1600-5).
- [ ] **AC-41** — Given a module is saved, then the file it came from is
  kept as it is in everything but the content, the name and the template
  mark, as today; a disk is written whole and keeps its description
  (`Core/Connector/BatteryCardInstance.hpp:233-249, 329`,
  `Shared/FloppyBay.cpp:154`).
- [ ] **AC-42** — Given cassette tapes, then a tape is a medium like the
  others: a tape definition (`media.md` §3) with its name tag
  `tape-name`, its `description`, the sound file it names by `file:`
  and its `position`, which is reserved for future use and is 0, a
  template when it says `template: true` and an
  instance otherwise (AC-25). The product bundles one tape template, the
  empty tape, which the tape picker offers; Name & Save of a tape copies
  the definition and its sound file into an instance, written side by
  side in the tape folder (LIB-9 AC-16), and recording onto an instance
  writes into its sound file; a definition the user writes around an
  existing WAV may name it by any relative path. A preset inserts a
  tape with the step `tape:` and takes it out with `eject: tape`
  (`preset.md` §5.2), and the picker lists the tape definitions
  (the topic of AC-7). A `.wav` on its own is not a tape but a program
  file (C1600-2; LIB-13 AC-19). What stays of today: a recording is
  written as mono 16-bit WAV at 48000 Hz, each time the motor stops and
  when the tape is ejected; a failed write is a notice; the counter
  shows the position while playing and the length while recording, and
  is green while the motor runs (`Shared/TapeBay.cpp:28-80`,
  `Core/Tape/TapeDeck.hpp:30-163`, `Qt6/app/ControlBar.cpp:386-411`;
  OQ-9 as decided).
- [ ] **AC-43** — Given the Web frontend, then importing takes module
  files, disk files and tape definitions with their WAV, and the saved
  media list is today's
  (`Shared/MediaLibrary.cpp:59-116`); deleting now asks first (OQ-4).
- [ ] **AC-44** — Given what the tests of Calc-U-1600 check today about
  media, then it holds, with the changes of AC-38 and AC-40: the lists
  and the filter of the pickers, Name & Save and what the place holds
  afterwards, the half second of autosave and the saves that do not
  wait, the write-protect switch, the rules of a save by name, the tape
  rules, import, export and delete, and a second tab
  (`Shared/tests/shared_bay_tests.cpp:106-210`,
  `Shared/tests/shared_app_session_tests.cpp:146-380`,
  `Core/tests/battery_card_instance_tests.cpp:232-513`,
  `Core/tests/tape_deck_tests.cpp:91-342`, `Web/tests/play.spec.js:216-329`).
- [ ] **AC-45** — Given the GUI test of the Qt6 and of the Web frontend,
  then each has two scenarios: under the ID LIB-7 AC-16, a module
  template is saved under a name and is an instance afterwards; under
  the ID LIB-7 AC-8, the picker of that slot then lists it among the
  instances (today `Web/tests/play.spec.js:236-266`; ADR 0032).

## Edge Cases
- **EC-1** — Given the save folder does not exist when a medium is first
  written, then it is made (LIB-9 EC-8); given it cannot be written,
  then AC-14 holds.
- **EC-2** — Given an instance's file is changed or removed outside the
  product while the medium is in its place, then the next save writes
  the product's content; nothing is merged (Calc-U-1600 today,
  `media.md`, facts).
- **EC-3** — Given a file in the save folder that is of a kind's suffix
  and cannot be read, then it is not listed, and the log gets one line
  for it at each start (today Calc-U-1600 leaves it out without a word,
  `Shared/ModuleBay.cpp:35-41`).
- **EC-4** — Given a medium is deleted from the saved media while the
  list is open elsewhere, then a second delete says that it is no longer
  there.
- **EC-5** — Given `floppy:` or `tape:` while the place holds a medium,
  then that one is ejected first (AC-13; `preset.md` §5.2).
- **EC-6** — Given `eject:` for an empty place, then nothing happens.
- **EC-7** — Given `saveas:` for a place whose medium has no
  write-protect switch, with `protected: true`, then it fails with that reason
  (Calc-U-1600 `Shared/tests/shared_app_session_tests.cpp:342-380`).
- **EC-8** — Given a session is suspended, then every change not yet
  written is written (AC-13); today the Web frontend of Calc-U-1600 can
  lose the last half second when its tab is hidden
  (`Web/src/engine.js:180-181`).
- **EC-9** — Given Name & Save for a medium that is already an instance,
  then it cannot be used (AC-16).

## Technical Requirements (optional)
- One media model and one set of save rules for every product, frontend
  and the harness (`media.md` I-4).
- The product never loses a write and never writes a template
  (`media.md` I-1, I-2); it replaces an instance only when the user or a
  preset asks (`media.md` I-3).
- The library reads and writes no medium format itself: each kind's
  reader and writer is supplied by its product, behind one seam (AC-46;
  OQ-6).
- A file that is written holds either the old or the new content, never
  a part.

## Open Questions
- [x] **OQ-1** — *Decided as recommended (user, 2026-11-10, batch 6 gate).* Two instances with one name (`media.md` Q-5): which one
  a save by name replaces, and which one a preset finds.
  *Recommendation:* the one whose file name is the one the name gives
  (AC-4); if neither is, the first by file name.
- [x] **OQ-2** — *Decided as recommended (user, 2026-11-10, batch 6 gate).* The characters a name must not hold (AC-3). `media.md`
  MD-3 says "a character a file name cannot hold" and lists none.
  *Recommendation:* as in AC-3, what Windows, macOS and Linux refuse
  between them, the same on every platform, so that a save folder can be
  carried from one to another.
- [x] **OQ-3** — *Superseded 2026-11-13: references are mappings (`docs/formats/README.md` §3) and `saveas:` is a mapping (`preset.md` §5.2.1); AC-21.* *Decided as recommended (user, 2026-11-10, batch 6 gate).* How a preset writes a medium (AC-21, AC-26). PR-8 gives
  examples, not a notation. *Recommendation:* as in AC-21: a name, in
  quotes when it holds a space, `file:<path>` or `mem:<name>`, then the
  side; `slot-1-file:` and `floppy-file:` go, and `saveas:` keeps
  today's form.
- [x] **OQ-4** — *Decided as recommended (user, 2026-11-10, batch 6 gate).* Import, export and delete of saved media (AC-29 to
  AC-31) exist only in Calc-U-1600's Web frontend today, and delete does
  not ask. *Recommendation:* in the Web frontend of both products,
  because a browser has no folder the user can reach; a delete asks
  first; the Qt6 and Swift frontends leave it to the file manager.
- [x] **OQ-5** — *Decided as recommended (user, 2026-11-10, batch 6 gate).* Changes this spec makes to what a user sees, which
  have no row in `docs/behaviour-changes.md` yet: a notice for a failed
  autosave (AC-14) and a line of the log for a file that cannot be read
  (EC-3), both silent today; every change written when the session is
  suspended (EC-8); a module change that leaves the other slot and the
  disk alone (AC-38); Name & Save asking before it replaces an instance,
  where Calc-U-1600 refuses the name today (AC-40; `media.md` MD-3);
  deleting saved media asking first (AC-31). *Recommendation:* all of
  them, with one row for each product.
- [x] **OQ-6** — *Decided as recommended (user, 2026-11-10, batch 6 gate).* Where the readers and writers of the media formats sit
  (`capabilities.md` Q-2; the open point "Sharp media formats" of
  `features/INDEX.md`). *Recommendation:* product code behind one seam
  of this feature: the magnetic card with C59-7; `.card.yaml`,
  `.floppy.yaml` and the tape's WAV with the Calc-U-1600 column of this
  feature, moved under the seam unchanged. No product row is added for
  them. *Since 2026-11-13 (user) the tape definition `.tape.yaml` is
  new there too.*
- [x] **OQ-7** — *Decided as recommended (user, 2026-11-10, batch 6 gate).* "Kind" (of medium), "user template" and "saved media"
  for the glossary, as defined above; the glossary's Template then says
  "bundled with a product or made by the user".
- [x] **OQ-8** — *Decided as recommended (user, 2026-11-10, batch 6 gate).* A write to a template (AC-15, AC-5). `media.md` I-2
  says "a write to a template or to a preset-named file goes to an
  instance". For a preset that is settled (AC-25). For a template the
  user chose in a picker, Calc-U-1600 keeps the written content in the
  place and loses it when the medium leaves, unless the user used Name &
  Save; an instance made by itself would need a name nobody gave, and
  the save folder would fill with copies. *Recommendation:* keep today's
  way and word I-2 for preset-named media; the Name & Save button shows
  that a medium is not saved. Second part: an instance that AC-25 makes
  from a bundled template carries the template's name, which no other
  instance may. *Recommendation:* allow it for AC-25 only, as AC-5
  says, so that a preset finds the template again and replaces its own
  results.

- [x] **OQ-9** — *Decided (user, 2026-11-13, §5.4 of
  `docs/walkthroughs/presets.md`): a tape saved by the product has its
  WAV beside its definition; a definition written around an existing
  WAV names it by a relative path of the user's choice; `position` is
  reserved for future use and must be 0; an old WAV is not a tape until
  it is wrapped (behaviour change 1600-18); `tape:` with a CE-150 on a
  PC-1600 is not restricted by the specification (LIB-6 OQ-10, C1600-1
  AC-16) (AC-42).* Tapes (AC-42), what `media.md` §3 leaves open: where
  tape definitions and their sound files lie (the tape folder of LIB-9
  AC-16, or the save folder like the other media); when `position` is
  written; what a WAV of today's tape folder is after the update, given
  that the user wraps the few there are in a definition by hand (read
  into the ruling of 2026-11-13); and whether `tape:` works with a
  CE-150 on a PC-1600, which `preset.md` §3 allows and C1600-1 AC-16
  ("as today") does not (LIB-6 OQ-10).

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| Media live in the save folder; a medium a preset names by path is never written, its first write makes an instance | ADR 0019 | 2026-11-10 |
| That instance takes the medium's name and replaces an instance of that name | `media.md` MD-1 (user); behaviour changes 59-7, 1600-7 | 2026-11-10 |
| Every medium carries its name inside its file; the file is named after it; the file name is only a fallback | `media.md` MD-2, MD-5; behaviour change 59-6 | 2026-11-10 |
| One set of save rules for Name & Save, `saveas` and the harness; Name & Save asks before replacing an instance; a name a file name cannot hold is refused | `media.md` MD-3; behaviour change 1600-5 | 2026-11-10 |
| An outdated file shows in its picker as a disabled entry with the hint; a preset naming one is refused; logged once per start | `media.md` MD-4; ADR 0037; behaviour change 1600-6 | 2026-11-10 |
| *Superseded 2026-11-13 (row at the end).* `insert`, `eject`, `arm` over media places that are pass-through or holding; `medium:` sections; `saveas` stays | `presets.md` PR-8 | 2026-11-10 |
| Modules are chosen with a picker on the control bar | ADR 0025 | 2026-11-10 |
| *Superseded 2026-11-13 (F-7: a tape is a medium, AC-42).* Cassette tapes keep their own rules; WAV is an external format | `media.md` Q-1 | 2026-11-10 |
| The names and states of media are a topic of this feature; the picker joins it with the control state | LIB-4 OQ-3 (user, batch 3 gate) | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 6 gate): every change is written when the session is suspended (AC-13, EC-8) | LIB-3 AC-24; a hidden tab loses a change today | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 6 gate): a module change leaves the other slot and the disk alone (AC-38) | It follows from the power cycle of behaviour change 1600-4 | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 6 gate): the half second of AC-11 for every kind, taken from Calc-U-1600 | It makes the criterion checkable | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 6 gate): the harness commands of AC-28 | Rule 4 of the index; LIB-11 AC-14, AC-18 | 2026-11-10 |
| A medium the user opens from outside the save folder is used where it lies and written back there; only a Web frontend copies it in, because a browser cannot write back | user, 2026-11-10, batch 7 gate; LIB-13 AC-4; ADR 0019 as amended | 2026-11-10 |
| A save that sets the write-protect switch is followed by a power cycle, as if a module had been plugged in (AC-19) | user, 2026-11-11; LIB-2 OQ-4 | 2026-11-11 |
| *Superseded 2026-11-13 (row at the end).* The notation of AC-21 holds for header keys, `insert` and `arm`; `saveas` keeps today's form, as OQ-3 decided, so a name or a path with spaces is written without quotes there | Phase 5, preset pass (`docs/walkthroughs/presets.md`, A-7); OQ-3 | 2026-11-11 |
| A medium's own file decides: `template: true` is never written, wherever it lies and however it was named; without it the file is an instance, written in place, also when a preset names it by file. No copy is made at a first write, and no instance takes a bundled template's name (AC-5, AC-15, AC-25, AC-35). Replaces MD-1 and the second part of OQ-8; the earlier rule was an error of the spec, Calc-U-1600 works so today | user, 2026-11-13 (question F-1 of `docs/walkthroughs/presets.md` §5.4); `docs/formats/media.md` | 2026-11-13 |
| Cassette tapes are media by `media.md` §3: a `.tape.yaml` definition with its WAV, templates and instances, Name & Save, `position`, the preset key `tape:`; one bundled template, the empty tape; a `.wav` alone is a program file (AC-37, AC-42, AC-43). Replaces "tapes keep their own rules" (`media.md` Q-1) | user, 2026-11-13 (question F-7 of `docs/walkthroughs/presets.md` §5.4); `docs/formats/media.md` §3 | 2026-11-13 |
| A Solid State ROM module shows its `description` in the picker, ordered by `menu-sort`, as today (AC-8) | user, 2026-11-13 (question F-14 of `docs/walkthroughs/presets.md` §5.4); `docs/formats/modules.md` §2.1, §2.4 | 2026-11-13 |
| The log names a written medium by its kind and name tag, never by its path, so that logs are equal in every frontend and the harness (LIB-6 AC-11, AC-13) | user, 2026-11-13 (B-8 (d) of `docs/walkthroughs/presets.md` §5.2) | 2026-11-13 |
| Sides are numbered 0 and 1 for magnetic cards and floppy disks, in presets, files and the GUI; the floppy disk format goes to version 2 (AC-37), so disks saved with 0.8.x are refused with the hint until upgraded by hand | user, 2026-11-13 (B-8 (a) of `docs/walkthroughs/presets.md` §5.2); `docs/formats/media.md` §1, §2 | 2026-11-13 |
| A saved tape's WAV lies beside its definition; a hand-written definition names its WAV by any relative path; `position` is reserved and 0 (AC-42, OQ-9) | user, 2026-11-13 (LIB-7 OQ-9, §5.4 of `docs/walkthroughs/presets.md`) | 2026-11-13 |
| Removable media (floppy disk, tape) are inserted and ejected by steps `floppy:`, `tape:`, `eject:`; modules are header keys only (AC-26, AC-42, EC-5, EC-6) | user, 2026-11-13 (LIB-7 EC-5, EC-6, §5.4 of `docs/walkthroughs/presets.md`): removable media are changed at runtime and belong into the chain of steps, like the TI-59's magnetic cards; installed peripherals, memory modules included, is fitted before the machine boots | 2026-11-13 |
| Aligned with `docs/formats/` after the Q&A round: references are mappings `{ name: }`, `{ file: }`, `{ mem: }`; steps `card:`, `arm:`, `floppy:`, `tape:`, `eject:` and a `saveas:` mapping, no `insert:` step and no `medium:` section; in-file media are the TI-59's `magnetic-cards:`; `.magnetcard.yaml` with sides 0 and 1; the floppy button shows 0 or 1 (AC-17, AC-21 to AC-24, AC-26, AC-32, AC-35, AC-39, AC-42, EC-5, EC-7) | user rulings of 2026-11-12/13 (`docs/formats/README.md` §3, `preset.md` §3.1, §5.2; `docs/walkthroughs/presets.md` §5.2, §5.4) | 2026-11-13 |
| A medium started under a name is Name & Save done up front (AC-20; `media.md` §1.3) | user, 2026-11-13 (consistency pass after the Q&A round): today's flow, read as Name & Save done up front | 2026-11-13 |
