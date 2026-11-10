# LIB-14: Family window structure and component-set shell

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: LIB-4 (the control state, from which every menu and control
  about the machine is drawn), LIB-9 (the settings the dialog shows),
  LIB-18 (model switch, attach and detach, Reset, Cold boot); through
  them LIB-3 (the busy topic, notices, the log, host services).
- Features that put their items, pickers and panels into this structure
  (rule 5 of `features/INDEX.md`); this spec names their place, not
  their behaviour: LIB-5 (Paste Text), LIB-6 (examples, startup
  presets), LIB-7 (module and media pickers, Name & Save, saved media),
  LIB-8 (paper panel), LIB-10 (run-control buttons), LIB-13 (opening
  files), C59-4, C59-7, C1600-2.
- Contracts and decisions: ADR 0023 as amended, 0024, 0025, 0021, 0032;
  `docs/contracts/frontend.md` (FE-2, FE-6, SP-2, I-4, I-5, Q-1 as
  amended), `docs/contracts/help.md` (HL-3), `docs/contracts/presets.md`
  (PR-4, "Loading…"); `docs/glossary-mapping.md` (visible renames);
  `docs/behaviour-changes.md` rows 59-5, 59-9, 59-14, 1600-9.
- Facts: Calc-U-59 at `v2.3.0`, Calc-U-1600 at `b4ea8c8`; every
  `path:line` below is at those pins.

Words used here that the glossary does not define yet (OQ-6):
- The **shell** is the part of a component set that every product's
  window is put together from: the menu bar, the control bar, the
  settings dialog, the About dialog, the notice, "Loading…", and the
  area in which panels are shown.
- The **faceplate** is the picture of the calculator with its display
  and its on-screen keys; the **hit map** says which area of it is which
  key (ADR 0023; `frontend.md`, non-responsibilities).

What the three status columns mean for this feature:
- **Library:** the shell exists in the Qt6 and in the React component
  set and is shown to work with a control state, settings and a
  catalogue built for the tests, without code of a product.
- **Calc-U-59:** its Qt6 and Web frontends are put together from the
  shell. Its Swift frontend follows natively and is C59-8.
- **Calc-U-1600:** its Qt6 and Web frontends are put together from the
  shell.

## User Stories
- As a user of one product, I want to find the same menus, the same
  control bar and the same settings dialog in the other, so that
  learning one teaches the other (Briefing §1.5).
- As a user, I want a control that cannot be used right now to say why.
- As a user, I want to see that a long operation is running, and to be
  able to stop it with Reset.
- As a user, I want to be told in plain words when something failed.
- As a developer of a product, I want to put my faceplate, my panels and
  my own menu items into a window that is otherwise there.

## Out of Scope
- What the control state holds and when something is available (owner:
  LIB-4); the settings, their keys and their file (LIB-9); what a model
  switch, Reset and Cold boot do (LIB-18).
- The pickers, dialogs and panels of other features: the module and
  media pickers, Name & Save and the saved media (LIB-7; magnetic cards:
  C59-7), the paper panel with Copy and Cut (LIB-8), the run-control
  buttons (LIB-10), the file pickers and the drop (LIB-13), the debug
  panel of Calc-U-59 (C59-4). The INSPECTOR and the CE-158 printer panel
  of Calc-U-1600 stay as they are and are product material.
- The Swift frontend (owner: C59-8); the embedding of a Web frontend,
  its props and its worker (LIB-15).
- The layout of a product's window, its faceplates and its hit maps
  (product material, ADR 0023); how a display is drawn (per product,
  OQ-5).
- The help's content and the site (LIB-16, LIB-19).
- A product that cannot start because a ROM file is missing (LIB-12).

## Acceptance Criteria

### Library

Menus
- [ ] **AC-1** — Given a frontend with a menu bar, then its menus are
  File, Edit, Machine and Help, in this order, with these items in this
  order (ADR 0024; OQ-1); an item for which the product has no function
  does not appear, and a menu without items does not appear:

  | Menu | Items | Comes from |
  |---|---|---|
  | File | Load Preset…; Examples ▸ (one submenu per group, when the catalogue lists examples) | LIB-13, LIB-6 |
  | File | the product's own kinds of files | LIB-13; C1600-2 |
  | File | the items for media | LIB-7; C59-7; the host drive of Calc-U-1600 |
  | File | Settings…; Quit | this feature (AC-11); the platform |
  | Edit | Copy Screen; Paste Text | the product (AC-41); LIB-5 |
  | Machine | Model ▸; one submenu for each thing that has variants; Peripherals ▸ | this feature (AC-2, AC-3) |
  | Machine | Reset; Cold boot | this feature (AC-4) |
  | Help | `<Product>` Help; About `<Product>` | this feature (AC-20, AC-21) |

- [ ] **AC-2** — Given the control state, then Machine ▸ Model lists the
  product's models with the running one marked, and choosing another
  sends the model switch (LIB-18 AC-6); for the running model, and for
  each plugged-in peripheral, that has variants, a submenu lists them
  with the running one marked, under a label the product gives
  (Calc-U-1600 `Qt6/app/MainWindow.cpp:858-1018`).
- [ ] **AC-3** — Given the control state, then Machine ▸ Peripherals
  lists every place that is not a module slot with the peripherals that
  fit it, the plugged-in one marked; choosing one attaches it, choosing
  the marked one detaches it (LIB-18 AC-10).
- [ ] **AC-4** — Given Machine ▸ Reset and Machine ▸ Cold boot, then
  they send those commands (LIB-18 AC-13); their shortcuts are Ctrl+R
  and Ctrl+Shift+R, with the Command key on macOS (Calc-U-1600
  `Qt6/app/MainWindow.cpp:1025-1050`; `docs/glossary-mapping.md`).
- [ ] **AC-5** — Given any menu item or control that is drawn from the
  control state, then it can be used exactly when its entry is available
  now, and otherwise it is shown disabled with the entry's reason as its
  tooltip (LIB-4 AC-2, AC-3).
- [ ] **AC-6** — Given the Qt6 frontend on macOS, then Settings…, About
  and Quit are in the application menu, as the platform expects; given a
  Web frontend, then there is no Quit.
- [ ] **AC-7** — Given a feature or a product that adds an item to a
  menu, then the item appears in the group AC-1 names for it, and the
  shell shows and disables it like its own (AC-5).

Control bar
- [ ] **AC-8** — Given the control state, then the control bar below the
  faceplate holds, from left to right and in the order of the
  descriptors: the model picker; for each module slot its picker
  (LIB-7), with the slot's number before it when the model has more than
  one; for each other place the control of AC-9; and for each media
  place the controls of its owner (LIB-7, C59-7) (ADR 0024, 0025;
  Calc-U-1600 `Qt6/app/ControlBar.cpp:76-272`).
- [ ] **AC-9** — Given a place that is not a module slot, then its
  control is a button that shows the peripheral's name and whether it is
  plugged in, when exactly one peripheral fits the place, and a picker
  with the empty place as a choice, when several fit (OQ-2). A place
  whose peripheral needs more than a choice to be attached (the host
  drive's folder) has no control on the bar.
- [ ] **AC-10** — Given a model without a place or a media place, then
  the bar has no control for it and no gap where it would be (LIB-4
  AC-5).

Settings dialog
- [ ] **AC-11** — Given Settings…, then a dialog titled "Settings" opens
  with these sections, each shown when the frontend has one of its
  settings (LIB-9 AC-25, AC-32); a change is applied and written at
  once (LIB-9 AC-6), and the only button is Close:

  | Section | Setting | Key (LIB-9) |
  |---|---|---|
  | General | Startup model: "Last used" and the product's models | `[startup] model` |
  | Startup presets | one row per model (AC-12) | `[preset]` |
  | Storage | Save folder; further folders of the product | `[storage]` |
  | Folders | the folder in which each file dialog starts | `[folders]` |
  | Tracing | Trace folder; Maximum file size, in MB | `[trace]` |
  | sections of the product | the product's own settings | the product's keys |

  (Calc-U-1600 `Qt6/app/SettingsDialog.cpp:29-383`, Calc-U-59
  `Qt6/app/SettingsDialog.cpp:19-94`.)
- [ ] **AC-12** — Given the row of a model under Startup presets, then
  it offers "None", the startup presets the catalogue lists for that
  model (LIB-6 AC-32), shown with name and description, and the file
  that is set now when it is not one of them; in a Qt6 frontend
  "Choose…" also sets a preset file from a folder (Calc-U-1600
  `Qt6/app/SettingsDialog.cpp:168-211`, `Web/src/Dialogs.jsx:189-230`).
- [ ] **AC-13** — Given a setting that is a folder, then its row shows
  the folder in use, with `~` for the home folder, a "Change…" button,
  and a "Reset" button that can be used only while the setting differs
  from its default (both products' Qt6 frontends today: Calc-U-1600
  `Qt6/app/SettingsDialog.cpp:97-131`).
- [ ] **AC-14** — Given the settings dialog is open, when an operation
  runs, then the settings can still be changed (`frontend.md` Q-1).

"Loading…"
- [ ] **AC-15** — Given the busy topic says that an operation runs at
  full speed, when it has run for 500 ms of wall time, then "Loading…"
  is shown, with a sign that work is going on and without a button to
  cancel, until the operation ends or goes on at regular speed
  (Calc-U-1600 `Qt6/app/SyncOperations.cpp:18-79`,
  `Web/src/Dialogs.jsx:298-310`; `presets.md` PR-4; LIB-6 AC-22).
- [ ] **AC-16** — Given "Loading…" is shown, then the display, the paper
  and the control bar go on showing what changes (`frontend.md` I-5),
  and the model picker, Reset and Cold boot can be used (LIB-4 AC-3;
  LIB-18 AC-16).

Notices and the log
- [ ] **AC-17** — Given a notice (LIB-3 AC-10), then it is shown as it
  is, in a dialog with an OK button, while the machine goes on running;
  given several notices, then each is shown, one after the other, in the
  order in which they came (OQ-4).
- [ ] **AC-18** — Given the log (LIB-3 AC-11), then the shell has no
  view of its own for it: a product that has a place for it shows it
  there (Calc-U-59: the LOG tab, C59-4); otherwise a Qt6 frontend writes
  the lines to the platform's log, as today (Calc-U-1600
  `Qt6/app/AppLogging.cpp:29-68`).

Help and About
- [ ] **AC-19** — Given the control state was not sent because the
  product could not start (LIB-4 AC-6), then the shell shows no control
  bar and no Machine menu; Help and About still work.
- [ ] **AC-20** — Given Help ▸ `<Product>` Help, then the product's help
  on calc-u.org opens in the user's browser, at the address the product
  gives (`help.md` HL-3).
- [ ] **AC-21** — Given About `<Product>`, then a dialog shows the
  product's name, its version and build, and the texts and links the
  product gives, with one button to close it.

The shell and what goes into it
- [ ] **AC-22** — Given a control state, settings and a catalogue built
  for the tests, then the shell of the Qt6 component set and the shell
  of the React component set each show AC-1 to AC-21 for them, with no
  code of a product (ADR 0023; `frontend.md` I-4).
- [ ] **AC-23** — Given a panel the control state lists (a paper panel,
  a panel of the product), then the shell shows it in the area for
  panels while it is listed and takes it away when it is not (LIB-4
  AC-13); where the area is and how panels share it is the product's
  layout (ADR 0023).
- [ ] **AC-24** — Given a faceplate and its hit map, then both component
  sets find the key for a pointer position by one rule, from what the
  hit map states for each key: its area, and how far outside it a hit
  still counts; a hit is sent as live input with the key's ID (LIB-5
  AC-4), several pointers can each hold a key, and the keys that live
  input holds are drawn pressed (LIB-5 AC-19) (OQ-5).
- [ ] **AC-25** — Given a frontend built from a component set, then its
  GUI test carries out these criteria as scenarios under their IDs, in
  every frontend the same: AC-1, AC-8, AC-11, AC-15, AC-17 (ADR 0032).
  The other criteria are checked on the components without a window.

### Calc-U-59

- [ ] **AC-26** — Given the Qt6 frontend, then its menus are: File with
  Load Preset… (Ctrl+O; today "Open Preset…"), Examples ▸ when the
  catalogue lists examples (C59-2), the card items of C59-7 (today "Fit
  Card…", "Fit Fresh Card…"), Settings… and Quit; Machine with Model ▸,
  Peripherals ▸, Reset and Cold boot; Help with "Calc-U-59 Help" and
  "About Calc-U-59". There is no Edit menu, because Calc-U-59 has
  neither Copy Screen nor Paste Text (`Qt6/app/MainWindow.cpp:54-77`;
  behaviour change 59-14).
- [ ] **AC-27** — Given any TI model, then the control bar holds the
  model picker, the picker of `slot-1` with the 14 Solid State ROM
  modules and the empty slot (LIB-4 AC-11; ADR 0025; behaviour change
  59-9), and the button "PC-100C" for the place `printer`; on the TI-59
  also the card controls (C59-7). Today the PC-100C is connected in the
  header of the printer panel
  (`Qt6/app/ControlBar.cpp`, `Web/src/ControlBar.jsx:90-100`; OQ-2).
- [ ] **AC-28** — Given a frontend of Calc-U-59 with a menu bar, then
  the buttons Reset, Preset and Settings are no longer on the control
  bar: they are the menu items of AC-26, and "Clean", today Reset with
  the Ctrl or Command key held, is Machine ▸ Cold boot (behaviour change
  59-14; `docs/glossary-mapping.md`; OQ-3).
- [ ] **AC-29** — Given the Web frontend of Calc-U-59, which has no menu
  bar today, then in its wide layout it has the menu bar of AC-1, and in
  its portrait layout one button on the control bar opens the same menus
  (OQ-3; `Web/src/PlayCalculator.jsx:127-196`).
- [ ] **AC-30** — Given the settings dialog of Calc-U-59, then it shows:
  General with Startup model (today "Startup Calculator") and the
  version; Startup presets with the rows TI-59, TI-58 and TI-58C;
  Storage with Save folder (Qt6; today "Card Folder"); Folders with the
  preset folder (Qt6); Tracing with Trace folder (Qt6) and Maximum file
  size; Display with Segment Style, "Modernized" or "Classic" (Qt6);
  Keyboard with Keyboard Feedback, "Off" or "Click Sound" (Web, not in
  the wide layout). The row Solid State Module is gone (behaviour change
  59-9; LIB-9 AC-19, AC-22; `Qt6/app/SettingsDialog.cpp:19-94`,
  `Web/src/Dialogs.jsx:148-198`).
- [ ] **AC-31** — Given an operation in Calc-U-59 that runs at full
  speed for more than 500 ms, then "Loading…" is shown (AC-15); today
  Calc-U-59 shows nothing of the kind (LIB-3 AC-32, OQ-7 there). Given a
  notice in the Web frontend, then it is shown as AC-17 says; today it
  is a line below the calculator (`Web/src/PlayCalculator.jsx:81-90`;
  OQ-4).
- [ ] **AC-32** — Given Help ▸ "Calc-U-59 Help", then
  `https://calc-u.org/59/` opens; given About, then it shows what it
  shows today: the icon, "Calc-U-59", the version and build, the
  summary, and the links to the source, to support and to the privacy
  statement (`Qt6/app/MainWindow.cpp:69-76`, `Qt6/app/AboutDialog.cpp:7-33`,
  `Web/src/Dialogs.jsx:190-209`). In the Web frontend Help and About are
  in the Help menu and no longer in the settings dialog.
- [ ] **AC-33** — Given the windows of Calc-U-59, then their layouts are
  those of today: in Qt6 the calculator, the printer and the debug panel
  side by side with dividers the user can move; in the Web frontend the
  layouts `portrait`, `wide` and `auto` (`Qt6/app/MainWindow.cpp:27-49`,
  `Web/src/PlayCalculator.jsx:127-196`).

### Calc-U-1600

- [ ] **AC-34** — Given the Qt6 frontend, then its menus are those of
  today in the order of AC-1: File with Load Preset…, Examples ▸, Load
  BASIC Program…, Load Machine Code…, Mount Directory… and Unmount,
  Settings… and Quit; Edit with Copy Screen (Copy Log Selection while
  text of the INSPECTOR is selected) and Paste Text; Machine with
  Model ▸, ROM Revision ▸ on the PC-1500, ROM Version ▸ and CE-1600P
  ROM ▸ on the PC-1600, Peripherals ▸, Reset and Cold boot; Help with
  "Calc-U-1600 Help" and "About Calc-U-1600…". Changed against today:
  Settings… moves from Edit to File (OQ-1), "Reset All" is named Cold
  boot (`docs/glossary-mapping.md`), and the Help item is new
  (`Qt6/app/MainWindow.cpp:858-1018`).
- [ ] **AC-35** — Given the Web frontend, then its menus are the same
  without Mount Directory…, Unmount and Quit, and with the media items
  of LIB-7 (today "Import Card, Disk or Tape…", "Saved Media…"). Load
  Preset… and Examples ▸ are two items as in Qt6; today the examples are
  inside "Load Preset ▸" (`Web/src/CalcU1600.jsx:243-298`).
- [ ] **AC-36** — Given a PC-1500 or PC-1500A, then the control bar
  holds: the model picker, the picker of `slot-1` with its Name & Save
  button (LIB-7), the buttons "CE-150" and "CE-158", and the tape
  controls while a peripheral with a cassette is plugged in. Given a
  PC-1600, then it holds: the model picker, the pickers of `slot-1` and
  `slot-2`, marked "1:" and "2:", a picker for `printer` with the
  CE-150, the CE-1600P and the empty place, the button "CE-158", the
  disk controls, and the tape controls. Today the PC-1600 has a button
  "CE-1600P" on the bar and the CE-150 and CE-158 in the menu only
  (`Qt6/app/ControlBar.cpp:76-272`, `Shared/ControlState.cpp:7-48`;
  OQ-2).
- [ ] **AC-37** — Given the settings dialog of Calc-U-1600, then it
  shows: General with Startup model (today "Startup device"); Startup
  presets (today "Default presets") with the rows PC-1500, PC-1500A and
  PC-1600; Storage with Save folder (today "Battery-card saves") and
  Tapes (Qt6), and in the Web frontend the sentence that says where
  saved media are kept; Folders with the samples, BASIC, assembly and
  host drive folders (Qt6); Tracing with Trace folder and Maximum file
  size (Qt6); Serial ports with the link folder and the two port lines
  (Qt6, not on Windows); Debugger with the debug server, its port, its
  status and Disconnect (Qt6) (`Qt6/app/SettingsDialog.cpp:217-383`,
  `Web/src/Dialogs.jsx:189-230`; LIB-9 AC-26).
- [ ] **AC-38** — Given an operation in the Qt6 frontend, then
  "Loading…" is shown as today after 500 ms, and it no longer keeps the
  user from Reset, Cold boot and the model picker (AC-16; today input is
  held back, `Qt6/app/SyncOperations.cpp:47-78`). Given a notice, then
  it is shown as today, in a dialog with OK, without the title of the
  operation above it (`Qt6/app/SyncOperations.cpp:34-41`,
  `Web/src/CalcU1600.jsx:126-132`; OQ-4).
- [ ] **AC-39** — Given Help ▸ "Calc-U-1600 Help", then
  `https://calc-u.org/1600/` opens (new; `help.md` HL-3); given About,
  then it shows what it shows today (`Qt6/app/AboutDialog.cpp:8-33`,
  `Web/src/Dialogs.jsx:232-246`).
- [ ] **AC-40** — Given the windows of Calc-U-1600, then their layout is
  that of today: the faceplate, below it the control bar, below it the
  row with the INSPECTOR and, while they are plugged in, the paper panel
  and the CE-158 printer panel; in the Web frontend the panels appear
  only when there is room for them, as today
  (`Qt6/app/MainWindow.cpp:89-124, 511-528`, `Web/src/CalcU1600.jsx:348-408`).
- [ ] **AC-41** — Given Edit ▸ Copy Screen, then the clipboard gets the
  picture of the display and its text, as today
  (`Qt6/app/MainWindow.cpp:680-699`, `Web/src/CalcU1600.jsx:229-232`).

## Edge Cases
- **EC-1** — Given an operation that ends before 500 ms have passed,
  then "Loading…" is not shown.
- **EC-2** — Given a notice arrives while "Loading…" is shown, then the
  notice is shown and "Loading…" goes on behind it; closing the notice
  does not end the operation.
- **EC-3** — Given the startup preset that is set for a model is a file
  that is no longer there, then its row shows that file as set (AC-12),
  and the start falls back as LIB-18 EC-1 says.
- **EC-4** — Given a model switch changes what the control state lists,
  then the menus and the bar show the new entries without the window
  being opened anew.
- **EC-5** — Given a peripheral is plugged in or removed, then a panel
  it owns appears or goes (AC-23) and the other panels keep their
  content.
- **EC-6** — Given the address of AC-20 cannot be opened (no browser, no
  network), then nothing else happens; the machine is not affected.

## Technical Requirements (optional)
- A component decides no rule about what the machine offers; it draws
  the control state (`frontend.md` FE-2).
- The component sets name no model and no product; faceplates, hit
  maps, layouts, labels of variant menus, About texts and help addresses
  are supplied by the product (ADR 0023; `frontend.md` I-4).
- The debug panel is not part of the component sets (ADR 0023 as
  amended); the sets share only the run-control buttons (LIB-10).
- The components are tested without a window; the GUI tests are thin
  (ADR 0032).

## Open Questions
- [ ] **OQ-1** — The menu table of AC-1, where the products differ
  today. *Recommendation:* "Load Preset…" in both (Calc-U-59's Qt6 says
  "Open Preset…"); Settings… at the end of File in both (Calc-U-1600 has
  it in Edit), so that Calc-U-59 needs no Edit menu for one item;
  Examples ▸ as an item of its own in every frontend; Ctrl+O for Load
  Preset…, Ctrl+R and Ctrl+Shift+R for Reset and Cold boot in both.
- [ ] **OQ-2** — The family rule for a place that is not a module slot
  (LIB-4 OQ-2). *Recommendation:* AC-9: a button when one peripheral
  fits, a picker when several do, computed from the descriptors. It
  keeps the PC-1500's buttons, moves the "PC-100C" button from the
  printer panel to the bar, and changes the PC-1600: a printer picker in
  place of the "CE-1600P" button, and the CE-158 on the bar. The product
  entry of LIB-4 AC-17 for this then goes.
- [ ] **OQ-3** — Calc-U-59 without a menu bar. Its Web frontend has
  buttons on the bar today (Reset, Preset, Settings), and so do Qt6 and
  Swift next to their small menus. *Recommendation:* where there is a
  menu bar, these buttons leave the control bar (AC-28), which then
  holds only hardware (ADR 0024); the Web frontend gets the menu bar in
  its wide layout and one menu button in its portrait layout (AC-29).
  The Swift frontend on iOS is settled in C59-8.
- [ ] **OQ-4** — How a notice is shown. Today Calc-U-1600 shows a dialog
  with a title and OK in both frontends; Calc-U-59 shows an alert in
  Swift and Qt6 and a line below the calculator on the web, and the
  Swift frontend loses a notice when a second one comes. A notice is a
  whole sentence (LIB-3 AC-10), so it needs no title. *Recommendation:*
  AC-17 for every frontend: a dialog with OK, no title, none lost.
- [ ] **OQ-5** — The calculator view (open point of
  `features/INDEX.md`). *Recommendation:* as the divergence matrix
  suggests (M16, M52): the component sets find the key under a pointer
  by one rule from the product's hit map (AC-24), and drawing the
  display stays with each product. Which of today's two rules becomes
  the one rule (Calc-U-59 widens each key's area; Calc-U-1600's Web
  frontend takes the nearest key within 24 px) is chosen in the design
  so that every faceplate behaves as today.
- [ ] **OQ-6** — "Shell", "faceplate" and "hit map" for the glossary, as
  defined above.

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| One menu and control-bar structure for the family; items a product lacks do not appear | ADR 0024; behaviour change 59-14 | 2026-11-10 |
| One Qt6 and one React component set; layouts, faceplates and hit maps stay per product; the debug panel is not shared | ADR 0023 as amended | 2026-11-10 |
| Modules are chosen with a picker on the control bar; the Solid State Module setting goes | ADR 0025, 0021; behaviour change 59-9 | 2026-11-10 |
| Controls are drawn from the control state, also Reset, Cold boot and loading a preset | `frontend.md` FE-2 as amended; LIB-4 AC-1 to AC-3 | 2026-11-10 |
| "Loading…" comes from the busy topic and is shown only while an operation runs at full speed | `frontend.md` SP-2; `presets.md` PR-4 | 2026-11-10 |
| Reset, Cold boot and a model switch can be used while an operation runs | `frontend.md` Q-1 as amended; LIB-18 AC-16 | 2026-11-10 |
| The in-app Help opens the product's help on calc-u.org | `help.md` HL-3 | 2026-11-10 |
| "Cold boot", "Startup presets" and "Save folder" replace "Clean", "Reset All", "Default presets", "Battery-card saves" and "Card Folder" | `docs/glossary-mapping.md`, visible renames | 2026-11-10 |
| GUI tests carry the same scenarios under the same criterion IDs in every frontend | ADR 0032 | 2026-11-10 |
| Proposed in this spec, for the gate: "Startup model" replaces "Startup Calculator" and "Startup device" | The glossary's term | 2026-11-10 |
| Proposed in this spec, for the gate: the 500 ms before "Loading…" and no cancel button, as in Calc-U-1600 today, for both products | Today's behaviour of one product, taken for the family | 2026-11-10 |
| Proposed in this spec, for the gate: a disabled control shows the reason of the control state as its tooltip (AC-5) | LIB-4 gives one reason per entry; this is where the user sees it | 2026-11-10 |
| Proposed in this spec, for the gate: the settings dialog applies every change at once and has only Close, in both products (Calc-U-59 applies some on close today) | Calc-U-1600's way; LIB-9 AC-6 | 2026-11-10 |
| Proposed in this spec, for the gate: the scenarios of AC-25 | ADR 0032 asks for named scenarios | 2026-11-10 |
