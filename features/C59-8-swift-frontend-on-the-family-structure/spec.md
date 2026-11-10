# C59-8: Swift frontend on the family structure

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: C59-5 (the Swift frontend hosts the library's app session),
  LIB-4 (the control state), LIB-14 (the family menus, control bar,
  settings dialog, "Loading…", notices, Help and About, and the GUI
  scenarios), and, each for the native counterpart of what it
  specifies, LIB-6 (examples, startup presets), LIB-7 (module picker),
  LIB-8 (paper panel), LIB-10 (run-control buttons).
- Contracts and decisions: ADR 0023 as amended, 0024, 0025, 0032, 0033;
  `docs/contracts/frontend.md` (FE-2, I-4); `docs/glossary-mapping.md`
  (visible renames); `docs/behaviour-changes.md` rows 59-5, 59-9, 59-14.
- Facts: Calc-U-59 at `v2.3.0`; every `path:line` below is at that tag.

Words of other specs used here: "shell", "faceplate", "preset list"
(glossary), "notice" (LIB-3 AC-10), "product entry" (LIB-4 AC-7). The
platform's own words are used for its own things: a **sheet** is a
dialog that slides over the window on iOS, an **alert** is the system's
small dialog with a text and buttons, the **help tag** is the text
macOS shows when the pointer rests on a control, and the **window
toolbar** is the strip in a macOS window's title area, which is not the
control bar. A **page** is one of the full-screen views the iOS layout
switches between.

## User Stories
- As a user of Calc-U-59 on a Mac, I want the same menus and the same
  control bar as in the Qt6 and Web frontends.
- As a user on an iPhone or iPad, I want to reach the same functions
  without a menu bar.
- As a user, I want the app to feel native: the settings where macOS
  and iOS put them, sheets, alerts and the system file pickers.
- As a developer, I want the Swift frontend to be checked by the same
  scenarios as the other frontends, so that a missing function shows.

## Out of Scope
- What the menus, the bar, the settings dialog, "Loading…" and a notice
  are and hold (owner: LIB-14); this feature gives their native form.
- The behaviour behind each control: presets (LIB-6), media and pickers
  (LIB-7), paper (LIB-8), run control (LIB-10), opening and dropping
  files (LIB-13), the constant memory (C59-3).
- The debug panel (owner: C59-4) and the magnetic card controls, the
  card pickers and the corner taps (owner: C59-7): each of those specs
  covers its Swift views itself.
- Hosting the session, the threads and going to the background (owner:
  C59-5).
- A Swift frontend for Calc-U-1600 (not planned; ADR 0023).

## Acceptance Criteria

Menus
- [ ] **AC-1** — Given the Swift frontend on macOS, then its menu bar
  has the family's menus File, Machine and Help with the items of AC-18,
  next to the menus macOS itself gives every app (the application menu,
  Edit, View, Window), which stay. Load Preset… is Command+O, Reset is
  Command+R and Cold boot is Shift+Command+R, and a shortcut is taken
  before the keyboard map sees the key (LIB-14 AC-4). Settings…, About
  and Quit are in the application menu. Today the app adds only About
  (`App/Calc-U-59App.swift:31-37`; behaviour change 59-14).
- [ ] **AC-18** — Given the family's menus in the Swift frontend, on
  macOS in the menu bar and on iOS behind the menu button of AC-5, then
  they hold, in this order:

  | Menu | Items |
  |---|---|
  | File | Load Preset…; Examples ▸ with one submenu per group, when the preset list has examples; the magnetic card items (C59-7), on the TI-59; on iOS Settings… |
  | Machine | Model ▸; Peripherals ▸; Reset; Cold boot |
  | Help | "Calc-U-59 Help" |

  (LIB-14 AC-1, AC-26.)
- [ ] **AC-2** — Given Help ▸ "Calc-U-59 Help", then `https://calc-u.org/59/`
  opens in the browser (LIB-14 AC-32); today the Swift frontend has no
  help item (`reference/FrontendParity.md:84`).

Control bar
- [ ] **AC-3** — Given the Swift frontend, then the control bar below
  the faceplate holds what LIB-14 AC-27 says, drawn from the control
  state (LIB-4 AC-15): the model picker, the picker of `slot-1` with the
  14 Solid State ROM modules and the empty slot, the button "PC-100C",
  and on the TI-59 the magnetic card controls (C59-7). On macOS the
  model picker moves there from the window toolbar, where it is today
  (ADR 0024: "the model first"; `App/Views/CalculatorView.swift:169-173`).
- [ ] **AC-4** — Given macOS, then the buttons Reset and Preset are no
  longer on the bar, and holding Command to turn Reset into "Clean" is
  gone: both are the menu items of AC-18 (LIB-14 AC-28;
  `App/Views/CalculatorView.swift:214-222, 304-315`).
- [ ] **AC-5** — Given iOS and iPadOS, then a menu button is the first
  control of the bar, before the model picker; it opens a menu with the
  submenus File, Machine and Help of AC-18. The buttons Reset, Preset
  and Settings are no longer on the bar, and the long press on Reset for
  "Clean" is gone, because Cold boot is a menu item (ADR 0024: "on iOS
  the same items appear without a menu bar";
  `App/Views/CalculatorView.swift:316-393`; OQ-1).
- [ ] **AC-6** — Given any control or menu item that is drawn from the
  control state, then it can be used exactly when its entry is available
  now (LIB-14 AC-5). On macOS the reason is its help tag, which a test
  reads as the control's accessibility help; on iOS the reason is not
  shown, and neither is the description of a module (LIB-7 AC-8).

Settings
- [ ] **AC-7** — Given Settings…, then it opens as the settings window
  of the app on macOS and as a sheet on iOS, whose one button is Done,
  the native form of Close (LIB-14 AC-11), and holds:

  | Section | Setting | Key (LIB-9 AC-19) | Where |
  |---|---|---|---|
  | General | Startup model: "Last used", TI-59, TI-58, TI-58C | `[startup] model` | both |
  | General | the version | — | both |
  | Startup presets | one row for each model: "None", the startup presets of the preset list, and the file that is set now when it is none of them (LIB-14 AC-12) | `[preset]` | both |
  | Display | Segment Style: "Modernized", "Classic" | `[display] led-style` | both |
  | Keyboard | Keyboard Feedback: "Off", "Haptic", "Click Sound" | `[keyboard] feedback` | iOS |
  | Layout | Debug Page in Portrait | LIB-9 OQ-5 | iOS |
  | Tracing | where traces go | LIB-9 AC-24, AC-34 and OQ-7 | as LIB-9 says for each system |
  | Tracing | Maximum file size, in MB | `[trace] max-file-size-mb` | both |

  The row Solid State Module is gone (behaviour change 59-9)
  (`App/Views/SettingsView.swift:31-150`).
- [ ] **AC-8** — Given a setting is changed, then it is applied and
  written at once (LIB-14 AC-11).

"Loading…", notices, About, a failed start
- [ ] **AC-9** — Given LIB-14 AC-15 and AC-16, then "Loading…" is shown
  as a small sign over the faceplate that leaves the display uncovered
  and takes no touches and no clicks, so that the keys, the control bar
  and the menus under and around it can be used.
- [ ] **AC-10** — Given a notice, then it is shown as an alert whose
  only text is the notice, with an OK button; given several, then each
  is shown in turn (LIB-14 AC-17). Today a second notice replaces the
  first before it was read (`App/Views/CalculatorView.swift:174-181`).
- [ ] **AC-11** — Given About on macOS, then it is the system's About
  panel with what it shows today (`App/AppInfo.swift:25-90`); on iOS
  there is no About, and the version is in the settings, as today.
- [ ] **AC-19** — Given the product could not start because a ROM file
  is missing (LIB-12 AC-5), then the window shows the message LIB-12
  gives in place of the faceplate, no control bar and no Machine menu;
  Help still works (LIB-14 AC-19).

The native counterparts
- [ ] **AC-12** — Given a picker, a panel or a button group that the
  Qt6 and React component sets have, then the Swift frontend has its own
  of the same kind, which meets the same criteria, read for the Swift
  frontend where they name Qt6 and Web:

  | Counterpart | Criteria |
  |---|---|
  | module picker of `slot-1` | LIB-7 AC-8, AC-10 |
  | paper panel with Copy and Cut | LIB-8 AC-12 to AC-20, AC-28 |
  | run-control buttons | LIB-10 AC-18 |
  | Examples menu, startup preset rows | LIB-6 AC-32; LIB-14 AC-12 |
  | on-screen keys on the faceplate | LIB-14 AC-24 |
  | holding the display for full speed | LIB-14 AC-43 |

- [ ] **AC-13** — Given Copy on the paper panel, then on macOS the
  clipboard gets the picture and the text as two entries, the picture
  first, and on iOS one entry with both, as today
  (`App/Views/PrinterView.swift:227-239`).
- [ ] **AC-20** — Given the product topic that says which cue card is to
  be shown (C59-2 AC-6; C59-7), then the Swift frontend draws that cue
  card in the window above the top row of keys, in the template it
  names, as today (`reference/SharedArchitecture.md:451-465`).

Layout
- [ ] **AC-15** — Given the windows of the Swift frontend, then their
  layouts are today's, with the paper panel there only while the control
  state lists it (LIB-4 AC-13; LIB-14 AC-23): on macOS the faceplate,
  the paper panel and the debug panel side by side; on iOS and iPadOS
  side by side when the window is wider than high and at least 820 by
  560 points, the faceplate and the paper panel alone when it is wider
  than high and smaller, and otherwise pages that are turned with the
  arrows at the corners, the debug panel as a page among them when
  Debug Page in Portrait is on. Without a paper panel its room goes to
  the debug panel, or its page is not there. The iPhone is used in
  portrait only (`App/Views/CalculatorView.swift:228-276, 429-477`,
  `Shared/DebugText.cpp:16-30`).

Scenarios
- [ ] **AC-16** — Given the XCUITest suite, then it carries out the
  family's scenarios under the IDs of the criteria that own them (ADR
  0032), on macOS and on iOS:

  | ID | On macOS | On iOS |
  |---|---|---|
  | LIB-14 AC-1 | the three menus and their items are in the menu bar | they are behind the menu button |
  | LIB-14 AC-4 | Command+R resets | not there |
  | LIB-14 AC-6 | Settings…, About and Quit are in the application menu | not there |
  | LIB-14 AC-8 | the bar holds the controls of AC-3 | the same, after the menu button |
  | LIB-14 AC-11 | the settings window with the rows of AC-7 | the sheet with the rows of AC-7 |
  | LIB-14 AC-15 | "Loading…" shows during a long load | the same |
  | LIB-14 AC-17 | a notice shows as an alert and goes with OK | the same |
  | LIB-14 AC-20 | the help item is there and can be chosen | the same, in the menu |
  | LIB-14 AC-24 | a click on a key presses it | a touch on a key presses it |
  | C59-8 AC-15 | one picture of the window | one picture per layout |
  | LIB-6 AC-42 | a bundled example is loaded and the display shows its result | the same |
  | LIB-8 AC-31 | the PC-100C prints, the text view shows it, Cut empties the paper | the same |

- [ ] **AC-17** — Given what the XCUITest suite checks today, then it
  holds, with Reset, Preset and Settings found in the menus: the
  controls can be reached in portrait and, on the iPad, in landscape; a
  wide window shows the debug panel and its tabs take taps; the file
  picker opens and opens again after Cancel; a preset loaded through it
  runs; and the screenshot tests still make their pictures
  (`Calc-U-59UITests/LayoutTests.swift:35-159`,
  `Calc-U-59UITests/FilePickerTests.swift:29-49`,
  `Calc-U-59UITests/PresetLoadTests.swift:35`,
  `Calc-U-59UITests/ScreenshotTests.swift:103-315`).
- [ ] **AC-21** — Given the app, then it no longer has the aids that
  exist only for its tests today: the launch argument `--UITesting`, the
  hidden R/S button and the element that tells the state of a preset;
  the tests reach the session through the attached harness instead
  (C59-5 OQ-4 as decided; ADR 0033;
  `App/Views/CalculatorView.swift:84-147`).

## Edge Cases
- **EC-1** — Given the menu of AC-5 is open when an operation starts,
  then its items follow the control state as soon as it changes.
- **EC-2** — Given a notice arrives while the settings sheet is open on
  iOS, then it is shown over the sheet.
- **EC-3** — Given a TI-58 or TI-58C, then the bar and the menus show
  no magnetic card controls (LIB-4 AC-12).
- **EC-4** — Given an iPhone in portrait, then every control of the bar
  can be reached; where the bar is too narrow for labels, the controls
  show their icons only, as today (`Shared/DebugText.cpp:16-30`).
- **EC-5** — Given the PC-100C is attached or detached, then the paper
  panel appears or goes (AC-15), and the other panels keep what they
  show (LIB-14 EC-5).
- **EC-6** — Given an iPad on which the system shows a menu bar for the
  app, then the menus of AC-18 are there too, with the shortcuts of
  AC-1; the menu button of AC-5 stays (OQ-1).

## Technical Requirements (optional)
- A view decides no rule about what the machine offers; it draws the
  control state (`frontend.md` FE-2). The copy of the model facts that
  the Swift frontend holds today goes (LIB-4 AC-15).
- The Swift components of AC-12 and the shell parts of AC-1 to AC-10
  name no model; they read the generic topics, the settings, the preset
  list and, for what is special to Calc-U-59, its product entries, which
  are supplied to them as in the other component sets (ADR 0023: the
  Swift frontend "is the component set a later Calc-U-1600 Swift
  frontend would use").
- The accessibility identifiers the tests use today stay on the controls
  that stay, and move with a control that moves into a menu.

## Open Questions
- [ ] **OQ-1** — iOS without a menu bar (AC-5, AC-18, EC-6; LIB-14 OQ-3
  left it to this spec). *Recommendation:* one menu button at the start
  of the bar with the family's menus as submenus, as the Web frontend
  has in its portrait layout (LIB-14 AC-29); Reset, Preset and Settings
  leave the bar, and the long press for "Clean" goes; where iPadOS shows
  a menu bar, the same menus are there as well.
- [ ] **OQ-2** — Row 59-14 names the Web frontend's changes and not this
  frontend's. *Recommendation:* extend it: in the Swift frontend, Reset,
  Preset and Settings are menu items on iOS too and the long press for
  "Clean" goes; on macOS "Clean" with Command held goes, the model
  picker moves from the window toolbar to the control bar, and Help is
  new; the paper panel is there only while the PC-100C is plugged in, in
  every frontend.
- [ ] **OQ-3** — A startup preset from a file of the user's (LIB-14
  AC-12 gives "Choose…" to Qt6 only). *Recommendation:* not in the Swift
  frontend now: the bundled ones and "None", as on the web.
- [ ] **OQ-4** — "Debug Page in Portrait" (AC-7, AC-15) is a setting
  only this frontend has (LIB-9 OQ-5). *Recommendation:* keep it, on
  iOS.

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| The Swift frontend follows the same concepts natively; it is not built from the Qt6 or React component set | ADR 0023 | 2026-11-10 |
| One menu and control-bar structure; on iOS the same items appear without a menu bar; the bar holds the model first | ADR 0024; behaviour change 59-14 | 2026-11-10 |
| The module is chosen with a picker on the bar; the setting goes | ADR 0025; behaviour change 59-9 | 2026-11-10 |
| Where there is a menu bar, Reset, Preset and Settings leave the control bar; a notice is a dialog and none is lost; "Loading…" is new | LIB-14 OQ-3, OQ-4 (user, batch 5 gate); behaviour change 59-5 | 2026-11-10 |
| GUI tests carry the same scenarios under the same IDs in every frontend | ADR 0032 | 2026-11-10 |
| The test-only aids of the app go once the harness attaches | C59-5 OQ-4 (user, batch 3 gate); ADR 0033 | 2026-11-10 |
| Proposed in this spec, for the gate: the menus macOS gives every app stay next to the family's (AC-1) | "Where the platform expects it" (ADR 0024) | 2026-11-10 |
| Proposed in this spec, for the gate: Done on the iOS settings sheet is the native form of Close (AC-7) | The platform's custom | 2026-11-10 |
| Proposed in this spec, for the gate: "Loading…" is a small sign that takes no touches (AC-9); a notice alert has the notice as its only text (AC-10) | LIB-14 AC-16, AC-17 | 2026-11-10 |
| Proposed in this spec, for the gate: the reason of a control that cannot be used is its help tag on macOS and is not shown on iOS (AC-6) | iOS has no such text | 2026-11-10 |
| Proposed in this spec, for the gate: Copy keeps its two forms of today on macOS and iOS (AC-13) | Today's behaviour | 2026-11-10 |
