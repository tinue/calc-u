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
  0037; `docs/behaviour-changes.md` rows 59-6, 59-7, 1600-1, 1600-5,
  1600-6, 1600-7.
- Facts: Calc-U-59 at `v2.3.0`, Calc-U-1600 at `b4ea8c8`; every
  `path:line` below is at those pins.

Glossary terms used: Medium, Template, Instance, Name & Save, Insert,
Fit, Arm, Save folder, Media place (pass-through, holding), Outdated,
Module, Memory module. A **kind** of medium is what one sort of media
place takes (a magnetic card, a floppy disk, a memory module with a
battery, a cassette tape); each kind has its file suffix and its format,
which its product supplies (OQ-6).

What the three status columns mean for this feature:
- **Library:** the media model, the save rules, autosave, the media
  topic, the pickers of the two component sets, the preset keys, steps
  and commands exist and are shown to work with a machine and a kind of
  medium built for the tests.
- **Calc-U-59:** magnetic cards are media of this model in every
  frontend; the card reader, the card format and `fit` are C59-7.
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
  magnetic card and its new version, the card reader, `fit`, the corner
  taps and the cue cards (owner: C59-7); `.card.yaml` and
  `.floppy.yaml`, which stay as they are, and the cassette's WAV
  (Calc-U-1600; OQ-6).
- Which places a model has and what fits them (LIB-1, LIB-2, LIB-4);
  what a module change does to the machine (LIB-18 AC-29).
- Opening a medium by drop, Open With or a file picker (owner: LIB-13;
  `frontend.md` Q-4).
- The host drive, `ti58c.mem`, traces and screenshots: they are in or
  near the save folder and are not media (`media.md` Q-2).
- Renaming a medium, and merging the instances of two devices (new
  functions; none today).

## Acceptance Criteria

### Library

The model
- [ ] **AC-1** — Given a medium, then it is a template or an instance. A
  template is bundled with the product, or is a file in the save folder
  that says it is one; an instance is a file in the save folder. A
  template and a bundled file are never written (`media.md` I-1; ADR
  0019).
- [ ] **AC-2** — Given a medium file, then it carries the medium's name
  inside; the file name counts only for a file that carries none
  (`media.md` MD-2, MD-5).
- [ ] **AC-3** — Given a name for a medium, then spaces around it are
  dropped and runs of spaces become one, and it is refused when it is
  empty or holds a character a file name cannot hold: `/ \ : * ? " < > |`
  or a control character (`media.md` MD-3; behaviour change 1600-5;
  OQ-2).
- [ ] **AC-4** — Given a medium is written under a name, then its file
  is the name with the suffix of its kind, in the save folder; given a
  file of that name exists and holds a medium with another name, then
  the new file gets the first free numbered name (`foo-1`) (`media.md`
  MD-5).
- [ ] **AC-5** — Given a medium is looked up by its name and kind, then
  the bundled ones are searched first and the save folder second; a file
  in the save folder that carries the name of a bundled medium is not
  listed and never found (Calc-U-1600
  `Core/Connector/NamedFileCatalog.hpp:124-170`, `Shared/ModuleBay.cpp:54-55`).
  Two files of the save folder with one name are both listed (`media.md`
  MD-5; OQ-1).
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
  place it is in now, if any; it is sent again when the save folder
  changes (LIB-4 OQ-3 as decided).

Pickers
- [ ] **AC-8** — Given the picker of a module slot or of a holding media
  place, then it lists, in this order and each group sorted by name: the
  empty place; the templates, the bundled ones before the user's; the
  instances; and, in a module slot, the modules that are not media (LIB-4
  AC-1). An entry shows the name, and its description as its tooltip
  (ADR 0025; Calc-U-1600 `Qt6/app/ControlBar.cpp:41-62`,
  `Web/src/ControlBar.jsx:24-43`).
- [ ] **AC-9** — Given an outdated file of that kind in the save folder,
  then the picker shows it as an entry that cannot be chosen, marked
  outdated, with a hint that names the format, its current version and
  where it is described; the log gets one line per outdated file at each
  start (`media.md` MD-4; ADR 0037; behaviour change 1600-6).
- [ ] **AC-10** — Given an entry is chosen, then what the old medium
  still has to save is written first (AC-13); in a holding media place
  the new medium is in at once, without a power cycle; in a module slot
  it is a replace of the module (LIB-2 AC-35, LIB-18 AC-29). Given the
  medium cannot be read, then the place is empty and a notice says why.

Autosave
- [ ] **AC-11** — Given an instance in a holding place whose content
  changes, then its file holds the new content at the latest half a
  second after the change, counted in the wall time the host passes
  (LIB-3 AC-2), and is not written while nothing changes (Calc-U-1600
  `Shared/ModuleBay.cpp:233-274`, `Shared/FloppyBay.cpp:168-188`).
- [ ] **AC-12** — Given an instance in a pass-through place, then its
  file is written when the medium is given back, if the machine wrote to
  it (Calc-U-59 `Shared/SessionController.cpp:275, 472-487, 554-582`).
- [ ] **AC-13** — Given an instance with a change not yet written, then
  it is written at once: before it leaves its place, before an operation
  starts, before the machine is replaced, when the session is suspended
  (LIB-3 AC-24), and when the product quits (Calc-U-1600
  `Shared/AppSession.cpp:59-84`, `Qt6/app/MainWindow.cpp:323-329`).
- [ ] **AC-14** — Given a file is written, then it holds either the old
  or the new content, never a part. Given the write fails, then a notice
  says so once for that medium, and the next change tries again; today
  both products fail without a word (Calc-U-1600
  `Shared/ModuleBay.cpp:245-247`; OQ-5).
- [ ] **AC-15** — Given a medium in a place that is not an instance (a
  template, a medium a preset names by file or defines itself), then
  nothing is written for it until Name & Save, `saveas` or AC-25 makes
  it one; its content lives as long as it is in its place.

Name & Save
- [ ] **AC-16** — Given a place that holds a medium which is not an
  instance and whose content can change, then Name & Save can be used
  for it; it asks for the name only, and for the write-protect switch
  where the medium has one that is still off (`media.md` MD-5;
  Calc-U-1600 `Shared/ModuleBay.hpp:81-86`, `Qt6/app/MainWindow.cpp:131-157`).
- [ ] **AC-17** — Given a save under a name, by Name & Save, by `saveas`
  or by the harness, then these rules hold in this order (`media.md`
  MD-3, I-3, I-4; Calc-U-1600 `Core/Connector/MediaSave.hpp:73-95`):

  | Rule | Name & Save | `saveas: live` | `saveas: template` | the `file:` form |
  |---|---|---|---|---|
  | the name is refused by AC-3 | refused | refused | refused | the name is the file name without its suffix |
  | there is no save folder | refused | refused | refused | — |
  | the name is that of a bundled medium of the same kind | refused | refused | refused | — |
  | the name is that of a user's template of the same kind | refused | refused | replaces it | — |
  | the file it would write is a template | refused | refused | replaces a user's, never a bundled one | writes exactly that file |
  | an instance of that name exists | asks before replacing | replaces | — | — |

- [ ] **AC-18** — Given a save succeeded, then the place holds the saved
  medium under its new name without a power cycle; saved as an instance
  it is autosaved from then on, saved as a template it is not
  (Calc-U-1600 `Shared/ModuleBay.cpp:222-229`).
- [ ] **AC-19** — Given the write-protect switch is asked for with a
  save, then the file says so and the medium in the machine is protected
  at once; no control switches it off again, as today (Calc-U-1600
  `Shared/ModuleBay.cpp:208-220`).
- [ ] **AC-20** — Given a place where a new, empty medium is started
  under a name before anything is written (a fresh magnetic card, a tape
  to record), then the name is asked first, and when an instance of that
  name exists the user is asked before it is replaced; the file is
  replaced only at the first write (Calc-U-59
  `tests/shared_cooperative_tests.cpp:727`; Calc-U-1600
  `Qt6/app/MainWindow.cpp:381-393`).

Media in presets
- [ ] **AC-21** — Given a preset, then a medium is named in it by its
  name (in quotes when it holds a space), by `file:` and a path (LIB-6
  AC-1), or by `mem:` and the name of a `medium:` section of the same
  preset; the header key of a module slot and of a holding media place
  takes such a name, followed by a side where the place has sides
  (`slot-1: "My programs"`, `floppy: file:disks/work.floppy.yaml B`)
  (`presets.md` PR-8, PR-11; OQ-3).
- [ ] **AC-22** — Given a `medium:` section, then it defines a medium
  inside the preset, with `name:`, `kind:` and the content fields of
  that kind, which the kind's owner reads; it may repeat; such a medium
  is never saved by itself (`presets.md` PR-8; Calc-U-59
  `Shared/StateFile.cpp:397-448`).
- [ ] **AC-23** — Given a preset names a medium that does not exist, is
  outdated or cannot be read, or a `mem:` name without its section, then
  the preset is refused when it is read, for an outdated file with the
  hint of AC-9 (LIB-6 AC-7; `media.md` MD-4).
- [ ] **AC-24** — Given a preset names an instance by its name, then the
  machine is built with that instance, and it is autosaved as always.
- [ ] **AC-25** — Given a preset names a medium by file, or a template
  by name, when the machine first writes to it, then an instance with
  the medium's name is made in the save folder, replacing an instance of
  that name, the write goes there, and the log says which file was
  written; the file the preset named is never written (`media.md` MD-1,
  I-2; ADR 0019; behaviour changes 59-7, 1600-7).

Steps and commands
- [ ] **AC-26** — Given a preset, then these steps exist, and the
  harness has commands of the same names and values (`presets.md` PR-8):

  | Step | Value | What it does | Emulated time (LIB-6 AC-19) |
  |---|---|---|---|
  | `insert:` | a place, a medium (AC-21), a side where the place has sides | in a holding place the medium is in at once and stays; through a pass-through place it is passed, and the step ends when it is back | pass-through: needs it |
  | `eject:` | a place | takes the medium out (AC-13 first) | none |
  | `arm:` | a place, a medium, a side | lays the medium ready; it is passed through when the machine says it waits for one (AC-6); the step does not wait | none |
  | `saveas:` | `live` or `template`, optionally `protected`, then the place, a colon and a name, or the place, `:file:` and a path | AC-17 to AC-19; a failure fails the preset | none |

  (Calc-U-1600 `Core/Preset/PresetFile.cpp:295-367`; Calc-U-59
  `Shared/StateFile.cpp:168-201`, `Shared/KeySequencer.cpp:214-219`.)
- [ ] **AC-27** — Given a medium was armed and never passed through when
  the next machine is built, then a line of the log says so (Calc-U-59
  `Shared/SessionController.cpp:539-546`).
- [ ] **AC-28** — Given the harness, then it also has: `media` (the
  topic of AC-7), `media import <file>` (AC-30), `media export <kind>
  <name> <path>` and `media delete <kind> <name>` (AC-31). A run of the
  harness has an empty save folder of its own unless it is given one
  (LIB-11 AC-8).

Getting media in and out
- [ ] **AC-29** — Given a Web frontend, then its File menu has the items
  for importing media and for the saved media (LIB-14 AC-35), and its
  media are in the persistent file system (LIB-15 AC-5).
- [ ] **AC-30** — Given a file is imported, then it must be a medium of
  a kind the product has, in the current version; it is written into the
  save folder under the file name its inner name gives (AC-4), as the
  template or instance it says it is; given a medium of that kind
  already has the name, then the import is refused (Calc-U-1600
  `Shared/MediaLibrary.cpp:59-107`).
- [ ] **AC-31** — Given the saved media, then the list shows the media
  of the save folder by kind and name, marks a template and a medium
  that is in a place now, hands a medium's file to the user (LIB-15
  AC-9), and deletes one that is in no place after the user confirmed
  (Calc-U-1600 `Web/src/Dialogs.jsx:248-298`,
  `Shared/MediaLibrary.cpp:24-116`; OQ-4).

### Calc-U-59

- [ ] **AC-32** — Given Calc-U-59, then its one kind of medium is the
  magnetic card, with the suffix `.u59`, and its one media place is the
  card reader of the TI-59, which is pass-through with the sides 1 and 2
  (C59-7; LIB-4 AC-12). There are no bundled cards.
- [ ] **AC-33** — Given the Swift, Qt6 and Web frontends, then magnetic
  cards are instances in the save folder of that frontend (LIB-9 AC-13
  to AC-15), under the name inside the file. Today the Web frontend
  keeps cards for one page only, a card is known by its file name, and a
  card file from any folder is written back where it lies
  (`Web/bindings.cpp:225-257`, `Shared/SessionController.hpp:192-196`;
  behaviour change 59-6).
- [ ] **AC-34** — Given a card that is read and written through the
  reader, then its instance is written when it is given back (AC-12);
  what that write keeps of the other side is C59-7's.
- [ ] **AC-35** — Given a preset of Calc-U-59, then `insert:` and `arm:`
  name the card reader, a card by name, by file or by `mem:`, and the
  side, and a card name that no card has stands for a new, empty card of
  that name, with a line in the log, as today
  (`Shared/SessionController.cpp:513-536`); a card named by file is
  never written, and its first write makes the instance of AC-25
  (behaviour change 59-7).
- [ ] **AC-36** — Given the Web frontend, then it has the import and the
  saved media of AC-29 to AC-31 for cards; today it has a download per
  card and an upload in the card picker (`Web/src/Dialogs.jsx:93-103`).

### Calc-U-1600

- [ ] **AC-37** — Given Calc-U-1600, then its kinds of media are the
  memory module with a battery (`.card.yaml`), the floppy disk
  (`.floppy.yaml`) and the cassette tape (`.wav`); the two YAML formats
  stay at `format-version: 1` and are read and written as today, the
  name being `module-name:` or `disk-name:` and a template saying
  `template: true` (`Core/Connector/MemoryCardDefinition.hpp:24-25, 168-172`,
  `Core/Connector/FloppyImageFile.hpp:23-54`; behaviour change 1600-1).
- [ ] **AC-38** — Given the picker of a module slot, then it is that of
  AC-8, with the modules that fit the slot (LIB-2 AC-2); choosing one
  replaces the module with a power cycle that keeps the memory (LIB-18
  AC-29), and the other slot and the disk in the drive are not touched.
  Today both slots and the disk are read again from their files, and
  what a template held is lost (`Shared/AppSession.cpp:308-312`,
  `Shared/ModuleBay.cpp:59-118`; behaviour change 1600-4).
- [ ] **AC-39** — Given the floppy disk controls, then they are: the
  picker of AC-8; a button "A" or "B" that turns the disk over, which is
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
- [ ] **AC-42** — Given cassette tapes, then they keep their own rules
  (`media.md` Q-1): the tape picker lists the WAV files of the tape
  folder by name; a tape chosen there only plays, from its start; a new
  tape is named first (AC-20) and only records; a recording is written
  whole, as mono 16-bit WAV at 48000 Hz, each time the motor stops and
  when the tape is ejected; a failed write is a notice; the counter
  shows the position while playing and the length while recording, and
  is green while the motor runs (`Shared/TapeBay.cpp:28-80`,
  `Core/Tape/TapeDeck.hpp:30-163`, `Qt6/app/ControlBar.cpp:386-411`).
- [ ] **AC-43** — Given the Web frontend, then importing takes module
  files, disk files and WAV files as today, a WAV under its file name
  and only when it can be played, and the saved media list is today's
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
  then each has these scenarios under these IDs: for AC-16, a module
  template is saved under a name and is an instance afterwards; for
  AC-8, the picker of a slot lists it (today
  `Web/tests/play.spec.js:236-266`; ADR 0032).

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
- **EC-5** — Given `insert:` into a holding place that holds a medium,
  then that one is ejected first (AC-13).
- **EC-6** — Given `eject:` for an empty place, then nothing happens.
- **EC-7** — Given `saveas:` for a place whose medium has no
  write-protect switch, with `protected`, then it fails with that reason
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
  reader and writer is supplied by its product, behind one seam (OQ-6).

## Open Questions
- [ ] **OQ-1** — Two instances with one name (`media.md` Q-5): which one
  a save by name replaces, and which one a preset finds.
  *Recommendation:* the one whose file name is the one the name gives
  (AC-4); if neither is, the first by file name.
- [ ] **OQ-2** — The characters a name must not hold (AC-3). `media.md`
  MD-3 says "a character a file name cannot hold" and lists none.
  *Recommendation:* as in AC-3, what Windows, macOS and Linux refuse
  between them, the same on every platform, so that a save folder can be
  carried from one to another.
- [ ] **OQ-3** — How a preset writes a medium (AC-21, AC-26). PR-8 gives
  examples, not a notation. *Recommendation:* as in AC-21: a name, in
  quotes when it holds a space, `file:<path>` or `mem:<name>`, then the
  side; `slot-1-file:` and `floppy-file:` go, and `saveas:` keeps
  today's form.
- [ ] **OQ-4** — Import, export and delete of saved media (AC-29 to
  AC-31) exist only in Calc-U-1600's Web frontend today, and delete does
  not ask. *Recommendation:* in the Web frontend of both products,
  because a browser has no folder the user can reach; a delete asks
  first; the Qt6 and Swift frontends leave it to the file manager.
- [ ] **OQ-5** — A failed autosave is silent in both products today
  (AC-14); so is a file that cannot be read (EC-3). *Recommendation:* a
  notice for the first, a line of the log for the second, and one row in
  `docs/behaviour-changes.md` for each product.
- [ ] **OQ-6** — Where the readers and writers of the media formats sit
  (`capabilities.md` Q-2; the open point "Sharp media formats" of
  `features/INDEX.md`). *Recommendation:* product code behind one seam
  of this feature: the magnetic card with C59-7; `.card.yaml`,
  `.floppy.yaml` and the tape's WAV with the Calc-U-1600 column of this
  feature, moved under the seam unchanged. No product row is added for
  them.
- [ ] **OQ-7** — "Kind" (of medium) for the glossary, as defined above.

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| Media live in the save folder; a medium a preset names by path is never written, its first write makes an instance | ADR 0019 | 2026-11-10 |
| That instance takes the medium's name and replaces an instance of that name | `media.md` MD-1 (user); behaviour changes 59-7, 1600-7 | 2026-11-10 |
| Every medium carries its name inside its file; the file is named after it; the file name is only a fallback | `media.md` MD-2, MD-5; behaviour change 59-6 | 2026-11-10 |
| One set of save rules for Name & Save, `saveas` and the harness; Name & Save asks before replacing an instance; a name a file name cannot hold is refused | `media.md` MD-3; behaviour change 1600-5 | 2026-11-10 |
| An outdated file shows in its picker as a disabled entry with the hint; a preset naming one is refused; logged once per start | `media.md` MD-4; ADR 0037; behaviour change 1600-6 | 2026-11-10 |
| `insert`, `eject`, `arm` over media places that are pass-through or holding; `medium:` sections; `saveas` stays | `presets.md` PR-8 | 2026-11-10 |
| Modules are chosen with a picker on the control bar | ADR 0025 | 2026-11-10 |
| Cassette tapes keep their own rules; WAV is an external format | `media.md` Q-1 | 2026-11-10 |
| The names and states of media are a topic of this feature; the picker joins it with the control state | LIB-4 OQ-3 (user, batch 3 gate) | 2026-11-10 |
| Proposed in this spec, for the gate: every change is written when the session is suspended (AC-13, EC-8) | LIB-3 AC-24; a hidden tab loses a change today | 2026-11-10 |
| Proposed in this spec, for the gate: a module change leaves the other slot and the disk alone (AC-38) | It follows from the power cycle of behaviour change 1600-4 | 2026-11-10 |
| Proposed in this spec, for the gate: the half second of AC-11 for every kind, taken from Calc-U-1600 | It makes the criterion checkable | 2026-11-10 |
| Proposed in this spec, for the gate: the harness commands of AC-28 | Rule 4 of the index; LIB-11 AC-14, AC-18 | 2026-11-10 |
