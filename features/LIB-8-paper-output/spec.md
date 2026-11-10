# LIB-8: Paper output

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: LIB-3 (topics, host services such as the clipboard), LIB-6
  (the place for the step `cut-paper`), LIB-11 (the harness); also named
  here: LIB-2 (the controls of a printer), LIB-4 (the paper panel in the
  control state), LIB-14 (the area for panels), LIB-18 (what builds a
  machine).
- Contracts and decisions: `docs/contracts/capabilities.md` (C-2, the
  generic kinds, I-1 to I-3), `docs/contracts/presets.md` (PR-7,
  `cut-paper:`); ADR 0023; LIB-18 OQ-6 as decided.
- Facts: Calc-U-59 at `v2.3.0`, Calc-U-1600 at `b4ea8c8`; every
  `path:line` below is at those pins.

Glossary terms used: Printer, Paper panel, Mark, text layer. "The
paper" is all marks a printer has put down since its paper was last
empty. Words of other specs: "topic" shapes (LIB-3 AC-5 to AC-8),
"notice" (LIB-3 AC-10), "controls" (LIB-2 AC-14).

What the three status columns mean for this feature:
- **Library:** the paper output kind, the paper topic, the paper panel
  of the Qt6 and the React component set, Copy, Cut and the commands
  exist and are shown to work with a printer built for the tests.
- **Calc-U-59:** the PC-100C gives its paper only through this kind, and
  the Qt6 and Web frontends show it in the shared panel (Swift: C59-8).
- **Calc-U-1600:** the CE-150 and the CE-1600P give their paper only
  through this kind, and both frontends show it in the shared panel.

## User Stories
- As a user, I want to see what my printer or plotter puts on paper
  while it happens, and to scroll back.
- As a user, I want to copy the paper as a picture, and as text where
  the printer prints characters, and to tear it off.
- As the author of a preset, I want to start a demo with empty paper.
- As a developer or an agent, I want to read the printout through the
  harness and compare it with an earlier one.

## Out of Scope
- What a printer prints and when: the PC-100C's printing, its PRINT, ADV
  and TRACE (owner: C59-6); the plotters' mechanics (C1600-1). This
  feature takes the marks they give.
- The controls kind (owner: LIB-2); whether a printer is plugged in, and
  the control for it on the bar (LIB-4, LIB-14).
- The CE-158's printer port and its panel: a product kind of Calc-U-1600
  that stays as it is (`capabilities.md` C-2).
- A text layer for the plotters (a backlog idea, `capabilities.md` C-2).
- Saving the paper to a file from a frontend, vector formats, and a
  limit on the length of the paper (new functions; none today).
- The Swift frontend's paper panel (owner: C59-8).

## Acceptance Criteria

### Library

The kind
- [ ] **AC-1** — Given a printer, then it has one paper output
  capability, which states: the width of its paper and the unit of its
  places, both in millimetres; whether its marks are dots or pen
  strokes; and whether it has a text layer (`capabilities.md` C-2).
- [ ] **AC-2** — Given a printer prints, then its marks are appended to
  the paper in the order in which they were made and are never changed
  afterwards: a dot with its place and its darkness, or a pen stroke
  with its colour and the places it passes through.
- [ ] **AC-3** — Given a printer with a text layer, then each line it
  prints is appended to the text layer with its characters, as Unicode
  text, and its place on the paper; a line the printer feeds without
  printing is an empty line of the text layer.
- [ ] **AC-4** — Given a paper, then its length is the farthest place a
  mark reaches or the paper was fed to, whichever is more; it has no
  limit (both products today).
- [ ] **AC-5** — Given the paper is cut through the capability, then it
  is empty: no marks, no text, length zero; what the printer prints next
  starts a new paper.

The topic
- [ ] **AC-6** — Given a machine with a paper output capability, then
  the app session has a paper topic for it, a stream topic (LIB-3 AC-7):
  a message carries the marks and text lines appended since the last
  one, or says that the paper was emptied. A frontend that starts to
  listen is sent the whole paper first (LIB-3 OQ-5).
- [ ] **AC-7** — Given the same commands at the same tick counts in two
  runs, then the paper holds the same marks in the same order (LIB-11
  AC-6).

What the paper lives through
- [ ] **AC-8** — Given a Reset, a Cold boot or a power cycle, then the
  paper stays (both products today: Calc-U-59
  `Shared/SessionController.cpp:93-207`; Calc-U-1600
  `Core/tests/ce150_tests.cpp:204-222`).
- [ ] **AC-9** — Given a machine is built by a model switch or a variant
  switch (LIB-18 AC-6, AC-7), and the new machine has a printer of the
  same type in the same place as the one before, then the paper is
  carried over to it; otherwise it is gone (LIB-18 OQ-6 as decided).
- [ ] **AC-10** — Given a machine is built by a preset, then every paper
  is empty at the start (LIB-6 AC-41; both products today).
- [ ] **AC-11** — Given a printer is detached, or replaced by a printer
  of another type, then its paper is gone; given it is replaced by
  another variant of itself, then the paper stays (OQ-1).

The paper panel
- [ ] **AC-12** — Given the control state lists a paper panel (LIB-4
  AC-13), then the panel shows, under the name of its owner, the whole
  width of the paper scaled to the width of the panel, with every mark
  at its place: dots in black, pen strokes in their colour.
- [ ] **AC-13** — Given the paper grows while the view is at its end or
  no more than 64 points from it, then the view follows the newest
  marks; given the user has scrolled further back, then it stays where
  it is. The paper is scrolled with the wheel or by touch; no scroll bar
  is drawn (Calc-U-1600 `Qt6/app/PlotterPaperWidget.cpp:24, 192-218`,
  `Web/src/Panels.jsx:106-113`).
- [ ] **AC-14** — Given a paper with a text layer, then the panel has a
  control that switches between the marks and the text, and shows which
  of the two is on; the text view shows the lines of the text layer in a
  fixed-width font, and its text can be selected (Calc-U-59
  `Qt6/app/PrinterWidget.cpp:131-162`).
- [ ] **AC-15** — Given the owner of the paper has controls (LIB-2
  AC-14), then the panel shows them: a key as a button that is down
  while it is held, a switch as a button that shows its position
  (Calc-U-59 `Qt6/app/PrinterWidget.cpp:177-184`).

Copy and Cut
- [ ] **AC-16** — Given Copy, then the clipboard gets a picture of the
  whole paper, black and colours on white, at its physical size and at
  the resolution the printer states, and with it the text of the text
  layer, its lines joined by line ends, when there is one
  (`capabilities.md` C-2).
- [ ] **AC-17** — Given a paper so long that the picture would have a
  side of more than 16000 pixels or more than 64 million pixels in all,
  then the resolution is lowered until it fits; the paper is never
  cropped (Calc-U-1600 `Shared/PlotterPaper.cpp:8-50`; Calc-U-59
  `Shared/PrinterFont.hpp:101-107`).
- [ ] **AC-18** — Given Cut, then the paper is copied as by Copy and
  then cut (AC-5), also when the copy did not succeed (all frontends
  today).
- [ ] **AC-19** — Given the paper is empty, then Copy and Cut cannot be
  used.
- [ ] **AC-20** — Given a Copy or a Cut, then a short message "Copied
  to clipboard" is shown for about two seconds, or "Copy failed" when
  the clipboard took nothing (Calc-U-59 `Qt6/app/PrinterWidget.cpp:262-267`,
  `Web/src/Printer.jsx:120-121`).
- [ ] **AC-21** — Given a Web frontend whose browser refuses a picture
  on the clipboard, then with a text layer the text alone is copied, and
  without one the picture is offered as a download named after the
  printer (Calc-U-59 `Web/src/Printer.jsx:68-87`; Calc-U-1600
  `Web/src/painters.js:158-185`).

Step and commands
- [ ] **AC-22** — Given the step `- cut-paper:`, which takes no value,
  then every paper of the machine is cut (AC-5), without a copy; the
  step needs no emulated time (LIB-6 AC-19) (`presets.md` PR-7).
- [ ] **AC-23** — Given the harness, then these commands exist at the
  top level; where a machine has more than one paper, each takes the
  owner's name first:

  | Command | Reply |
  |---|---|
  | `cut-paper` | done |
  | `paper text` | the lines of the text layer of the whole paper; refused without a text layer |
  | `paper marks` | every mark of the whole paper as one line of text each, in the order made: for a dot its place and darkness, for a pen stroke its colour and its places |
  | `paper png <path>` | the picture of AC-16 and AC-17 written to that file |

  The paper topic itself is read with `state` (LIB-11 AC-9), which gives
  what was appended since it was last read.

### Calc-U-59

- [ ] **AC-24** — Given a PC-100C, then its paper output capability
  states: dots; a text layer; and the paper of today, with the dot pitch
  of 0.430 mm as the unit and a width of 129.07 units
  (`Shared/PrinterFont.hpp:12-28, 95-99`).
- [ ] **AC-25** — Given the PC-100C prints a line, then the text layer
  gets its 20 positions, spaces included, each as the character of today
  for its code (`Core/TMC0501.cpp:33-42, 810-821`), and the paper gets
  the dots of those characters in the 5 by 7 font of today
  (`Shared/PrinterFont.hpp:6-8, 110`); a complete line takes 10.18 units
  of paper, and a feed takes 7.09 units and gives an empty line, so that
  one ADV gives two (`Core/TMC0501.cpp:827-842`).
- [ ] **AC-26** — Given a line that is cut short while it prints (a
  printer-interrupt picture), then the paper gets only the rows and
  columns that were printed, the last ones with their darkness, and the
  line takes only the paper of its rows, as today
  (`Shared/PrinterFont.hpp:33-92`, `Core/TMC0501.hpp:15-27`).
- [ ] **AC-27** — Given Copy on the PC-100C's paper, then the picture is
  at 600 dots per inch, which is about 1311 pixels wide
  (`Shared/PrinterFont.hpp:101-107`).
- [ ] **AC-28** — Given the Qt6 and Web frontends, then the paper panel
  shows the PC-100C's PRINT and ADV as held buttons and TRACE as a
  switch (AC-15; C59-6), and the control for the marks and the text
  (AC-14). In the Web frontend that control shows today the view it
  would switch to (`Web/src/Printer.jsx:132-133`).
- [ ] **AC-29** — Given the PC-100C is detached, then its paper is gone
  (AC-11). Today the paper stays through a disconnect and through every
  model switch (`Shared/SessionController.cpp:368-371`,
  `tests/shared_controller_tests.cpp:119-132`; OQ-1).
- [ ] **AC-30** — Given what the tests of Calc-U-59 check today about
  the paper, then it holds: the shape of the font, the heights of a
  feed, a complete and a cut-short line, the rows of a printer-interrupt
  line against the scan of a real printout, and that PRINT gives a line
  (`tests/shared_tables_tests.cpp:86, 136`, `tests/core_tests.cpp:268, 312`).
- [ ] **AC-31** — Given the GUI test of the Qt6 and of the Web frontend,
  then each has this scenario under this ID: the PC-100C prints a
  number, the text view shows it, Cut empties the paper and can no
  longer be used (today `Web/tests/play-calculator.spec.js:342`; ADR
  0032).

### Calc-U-1600

- [ ] **AC-32** — Given a CE-150 or a CE-1600P, then its paper output
  capability states: pen strokes in black, blue, green and red; no text
  layer; a unit of 190/1920 mm; and paper 56 mm wide of which 42.75 mm
  can be drawn on (CE-150), or 210 mm wide of which 190 mm can be drawn
  on (CE-1600P), the rest split evenly as margins
  (`Shared/PlotterPaper.hpp:28-53`,
  `Core/Connector/AlpsPlotterMechanism.hpp:31-42`).
- [ ] **AC-33** — Given the pen is put down, moved and lifted, then the
  paper gets one pen stroke in the pen's colour through the places it
  passed; a move with the pen up, a feed included, gives no mark
  (`Core/tests/ce1600p_tests.cpp:89-118`, `Core/tests/ce150_tests.cpp:142`).
- [ ] **AC-34** — Given the paper panel, then a pen stroke is drawn one
  point wide with round ends, in black, blue (0, 0, 255), dark green
  (0, 128, 0) or red (255, 0, 0) (`Shared/PlotterPaper.hpp:89-92`).
- [ ] **AC-35** — Given Copy on a plotter's paper, then the picture is
  at 1200 dots per inch unless AC-17 lowers it; an empty CE-1600P paper
  gives 9921 pixels of width (`Shared/tests/shared_geometry_tests.cpp:56-68`).
- [ ] **AC-36** — Given the ROM of the model, or of the CE-1600P, is
  switched, then the paper stays (AC-9, AC-11). Today it is lost, as it
  is when a module is chosen (`Shared/MachineSession.cpp:86-142, 732-739`);
  a module change is a power cycle now (LIB-18 AC-29), so the paper
  stays there too (OQ-2).
- [ ] **AC-37** — Given what the tests of Calc-U-1600 check today about
  the paper, then it holds: how steps become places, a stroke per pen
  down, the colour changes of both plotters, the paper kept through a
  reset, how places map to the panel, strokes split by colour, and the
  size of the copied picture (`Core/tests/ce1600p_tests.cpp:47-130`,
  `Core/tests/ce150_tests.cpp:112-222`,
  `Shared/tests/shared_geometry_tests.cpp:25-68`).
- [ ] **AC-38** — Given the GUI test of the Qt6 and of the Web frontend,
  then each has this scenario under this ID: a plotter example is
  loaded, and the paper panel appears under the plotter's name with
  marks on it (today `Web/tests/play.spec.js:208-214`; ADR 0032).

## Edge Cases
- **EC-1** — Given a cut while the printer is printing, then the marks
  made after the cut are on the new paper.
- **EC-2** — Given a machine without a printer, then there is no paper
  topic, `cut-paper` is done and changes nothing, and the `paper`
  commands are refused.
- **EC-3** — Given a frontend starts to listen while the paper is long,
  then it is sent the whole paper once and from then on only what is
  appended (AC-6).
- **EC-4** — Given marks are appended faster than the panel draws
  (full speed), then no mark is lost; the panel shows them at the end of
  the host call in which they came (LIB-3 AC-15).
- **EC-5** — Given Copy while the paper grows, then the picture holds
  the marks up to the moment of the copy.
- **EC-6** — Given `paper png` with a path that cannot be written, then
  the command is refused with the reason.

## Technical Requirements (optional)
- The component sets have one paper panel, which draws any paper from
  its marks; it names no printer (`capabilities.md` C-2, I-1; ADR 0023).
- The picture of Copy and of `paper png` is made below the frontend, so
  that every frontend and the harness get the same pixels for the same
  paper; what the platform adds (the resolution written into the
  picture, the form of the clipboard entry) is the frontend's.
- Paper output crosses the machine as marks; the machine draws nothing.

## Open Questions
- [ ] **OQ-1** — What becomes of the paper when its printer is detached
  (AC-11, AC-29). Today Calc-U-59 keeps it through a disconnect and
  shows it again on reconnect; Calc-U-1600 loses it. LIB-18 OQ-6 decided
  for the model switch that the paper goes when the new machine has no
  PC-100C. *Recommendation:* the same rule for a detach: the paper
  belongs to the printer and goes with it. This is a visible change for
  Calc-U-59 and gets a row in `docs/behaviour-changes.md`.
- [ ] **OQ-2** — AC-36 keeps a plotter's paper where Calc-U-1600 loses
  it today (a ROM switch, a module change). *Recommendation:* keep it,
  as AC-9 and AC-11 say; it follows from one rule for both products, and
  the row of OQ-1 names it.
- [ ] **OQ-3** — The message of AC-20 is new in Calc-U-1600, and
  following the paper only near its end (AC-13) is new in Calc-U-59's
  Qt6 frontend, which jumps to the end on every line today
  (`Qt6/app/PrinterWidget.cpp:228-232`). *Recommendation:* both, as
  written; too small for a row.
- [ ] **OQ-4** — The form of a line of `paper marks` (AC-23).
  *Recommendation:* fixed in the design and then kept stable, because
  C1600-4 AC-16 and the golden runs compare printouts with it; places in
  the unit of AC-1 as whole numbers where the printer gives whole
  numbers.

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| Paper output is one generic kind: a stream of marks (dots, or pen strokes with colour) and an optional text layer | `capabilities.md` C-2 (user) | 2026-11-10 |
| One paper panel per component set; Copy gives the picture always and the text when there is a text layer; Cut and `cut-paper` exist once | `capabilities.md` C-2; ADR 0023 | 2026-11-10 |
| The CE-158's printer port stays a product kind; a text layer for the plotters is a backlog idea | `capabilities.md` C-2 | 2026-11-10 |
| `cut-paper:` is a step in every product | `presets.md` PR-7 | 2026-11-10 |
| At a model switch the paper stays while the new machine has the same printer, and goes otherwise | LIB-18 OQ-6 (user, batch 4 gate) | 2026-11-10 |
| A preset starts with empty paper | LIB-6 AC-41; both products today | 2026-11-10 |
| Proposed in this spec, for the gate: the caps of AC-17 for both products (Calc-U-59 has only the 16000 pixels today) | One rule; no PC-100C picture reaches 64 million pixels | 2026-11-10 |
| Proposed in this spec, for the gate: Cut cuts also when the copy failed (AC-18) | All frontends today | 2026-11-10 |
| Proposed in this spec, for the gate: the control of AC-14 shows the view that is on (Qt6 and Swift today; the Web frontend shows the other) | One rule for the family | 2026-11-10 |
| Proposed in this spec, for the gate: the commands of AC-23 and their names; `cut-paper` without a copy | They make the criteria checkable; LIB-11 AC-14 | 2026-11-10 |
| Proposed in this spec, for the gate: a dot has a darkness (AC-2) | The PC-100C's cut-short lines print lighter dots today | 2026-11-10 |
