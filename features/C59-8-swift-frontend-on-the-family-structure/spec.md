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
  scenarios); also named here, each for the native counterpart of what
  it specifies: LIB-6 (examples, startup presets), LIB-7 (module
  picker), LIB-8 (paper panel), LIB-9 (settings), LIB-10 (run-control
  buttons), LIB-13 (opening files), C59-4 (debug panel), C59-7 (magnetic
  card controls).
- Contracts and decisions: ADR 0023 as amended, 0024, 0025, 0032;
  `docs/contracts/frontend.md` (FE-2, I-4); `docs/glossary-mapping.md`
  (visible renames); `docs/behaviour-changes.md` rows 59-5, 59-9, 59-14.
- Facts: Calc-U-59 at `v2.3.0`; every `path:line` below is at that tag.

Words of other specs used here: "shell", "faceplate" (glossary), "notice"
(LIB-3 AC-10), "preset list" (glossary), "product entry" (LIB-4 AC-7).

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
  files (LIB-13), the debug panel (C59-4), magnetic cards (C59-7), the
  constant memory (C59-3).
- Hosting the session, the threads and going to the background (owner:
  C59-5).
- A Swift frontend for Calc-U-1600 (not planned; ADR 0023).

## Acceptance Criteria

Menus on macOS
- [ ] **AC-1** — Given the Swift frontend on macOS, then its menu bar
  has the family's menus with the items of LIB-14 AC-26: File with Load
  Preset… (Command+O), Examples ▸ when the preset list has examples, and
  the magnetic card items (C59-7); Machine with Model ▸, Peripherals ▸,
  Reset (Command+R) and Cold boot (Shift+Command+R); Help with
  "Calc-U-59 Help". Settings…, About and Quit are in the application
  menu. Today the app adds only About (`App/Calc-U-59App.swift:31-37`;
  behaviour change 59-14).
- [ ] **AC-2** — Given Help ▸ "Calc-U-59 Help", then `https://calc-u.org/59/`
  opens in the browser (LIB-14 AC-32); today the Swift frontend has no
  help item (`reference/FrontendParity.md:84`).

Control bar
- [ ] **AC-3** — Given the Swift frontend, then the control bar below
  the faceplate holds what LIB-14 AC-27 says, drawn from the control
  state (LIB-4 AC-15): the model picker, the picker of `slot-1` with the
  14 Solid State ROM modules and the empty slot, the button "PC-100C",
  and on the TI-59 the magnetic card controls (C59-7). On macOS the
  model picker is in the window's toolbar today
  (`App/Views/CalculatorView.swift:169-173`; OQ-2).
- [ ] **AC-4** — Given macOS, then the buttons Reset and Preset are no
  longer on the bar, and holding Command to turn Reset into "Clean" is
  gone: both are the menu items of AC-1 (LIB-14 AC-28;
  `App/Views/CalculatorView.swift:214-222, 304-315`).
- [ ] **AC-5** — Given iOS, which has no menu bar, then one menu button
  on the control bar opens the family's menus with the same items in the
  same order, Settings… among them; the buttons Reset, Preset and
  Settings are no longer on the bar, and the long press on Reset for
  "Clean" is gone, because Cold boot is a menu item (ADR 0024: "on iOS
  the same items appear without a menu bar";
  `App/Views/CalculatorView.swift:316-393`; OQ-1).
- [ ] **AC-6** — Given any control or menu item that is drawn from the
  control state, then it can be used exactly when its entry is available
  now (LIB-14 AC-5); on macOS its help text gives the reason, on iOS the
  reason is not shown.

Settings
- [ ] **AC-7** — Given Settings…, then it opens as the settings window
  of the app on macOS and as a sheet with a Done button on iOS, as
  today, and holds what LIB-14 AC-30 gives the Swift frontend: General
  with Startup model and the version; Startup presets with the rows
  TI-59, TI-58 and TI-58C, each offering "None" and the startup presets
  of the preset list; Display with Segment Style; on iOS Keyboard
  Feedback ("Off", "Haptic", "Click Sound") and Debug Page in Portrait;
  Tracing with Maximum file size and, on macOS, the place of the traces
  (LIB-9 AC-24, AC-34). The row Solid State Module is gone (behaviour
  change 59-9) (`App/Views/SettingsView.swift:31-150`).
- [ ] **AC-8** — Given a setting is changed, then it is applied and
  written at once (LIB-14 AC-11).

"Loading…", notices, About
- [ ] **AC-9** — Given LIB-14 AC-15 and AC-16, then "Loading…" is shown
  over the faceplate in both systems, without blocking the control bar
  and the menus.
- [ ] **AC-10** — Given a notice, then it is shown as an alert with OK;
  given several, then each is shown in turn (LIB-14 AC-17). Today a
  second notice replaces the first before it was read
  (`App/Views/CalculatorView.swift:174-181`).
- [ ] **AC-11** — Given About on macOS, then it is the system's About
  panel with what it shows today (`App/AppInfo.swift:25-90`); on iOS
  there is no About, and the version is in the settings, as today.

The native counterparts
- [ ] **AC-12** — Given a picker, a panel or a button group that the
  Qt6 and React component sets have, then the Swift frontend has its own
  of the same kind, which meets the criteria of the feature that owns
  it:

  | Counterpart | Criteria |
  |---|---|
  | module picker of `slot-1` | LIB-7 AC-8, AC-10 |
  | paper panel with Copy and Cut | LIB-8 AC-12 to AC-21, AC-28 |
  | run-control buttons | LIB-10 AC-18, AC-34 |
  | Examples menu, startup preset rows | LIB-6 AC-32; LIB-14 AC-12 |
  | on-screen keys on the faceplate | LIB-14 AC-24 |

- [ ] **AC-13** — Given Copy on the paper panel, then on macOS the
  clipboard gets the picture and the text as two entries, the picture
  first, and on iOS one entry with both, as today
  (`App/Views/PrinterView.swift:227-239`).
- [ ] **AC-14** — Given the Swift components of AC-12 and the shell
  parts of AC-1 to AC-10, then they name no model and read the control
  state, the settings and the preset list only; what is special to
  Calc-U-59 is supplied to them, as in the other component sets (ADR
  0023: the Swift frontend "is the component set a later Calc-U-1600
  Swift frontend would use").

Layout
- [ ] **AC-15** — Given the windows of the Swift frontend, then their
  layouts are today's: on macOS the faceplate, the paper panel and the
  debug panel side by side; on iOS and iPadOS side by side when the
  window is wide and at least 820 by 560 points, the faceplate and the
  paper panel alone when it is wide and smaller, and otherwise pages
  that are turned with the arrows at the corners, the debug page among
  them when Debug Page in Portrait is on; the iPhone in portrait only
  (`App/Views/CalculatorView.swift:228-276, 429-477`,
  `Shared/DebugText.cpp:16-30`).

Scenarios
- [ ] **AC-16** — Given the XCUITest suite, then it carries out the
  scenarios of LIB-14 AC-25 under the same IDs on macOS and on iOS, and
  those of LIB-6 AC-42 and LIB-8 AC-31 (ADR 0032). Where a scenario
  needs a control that iOS shows in the menu of AC-5, it opens that
  menu.
- [ ] **AC-17** — Given what the XCUITest suite checks today, then it
  holds, with Reset, Preset and Settings found in the menus: the
  controls can be reached in portrait and landscape, a wide window shows
  the debug panel and its tabs take taps, the file picker opens and
  opens again after Cancel, a preset loaded through it runs, and the
  screenshot tests still make their pictures
  (`Calc-U-59UITests/LayoutTests.swift:35-159`,
  `Calc-U-59UITests/FilePickerTests.swift:29-49`,
  `Calc-U-59UITests/PresetLoadTests.swift:35`,
  `Calc-U-59UITests/ScreenshotTests.swift:103-315`).

## Edge Cases
- **EC-1** — Given the menu of AC-5 is open when an operation starts,
  then its items follow the control state as soon as it changes.
- **EC-2** — Given a notice arrives while the settings sheet is open on
  iOS, then it is shown when the sheet is closed, or over it; it is not
  lost.
- **EC-3** — Given a TI-58 or TI-58C, then the bar and the menus show
  no magnetic card controls (LIB-4 AC-12).
- **EC-4** — Given an iPhone in portrait, then every control of the bar
  can be reached; where the bar is too narrow for labels, the controls
  show their signs only, as today
  (`Shared/DebugText.cpp:16-30`, bar labels from 1300 points on).

## Technical Requirements (optional)
- A view decides no rule about what the machine offers; it draws the
  control state (`frontend.md` FE-2). The copy of the model facts that
  the Swift frontend holds today goes (LIB-4 AC-15).
- The accessibility identifiers the tests use today stay on the controls
  that stay, and move with a control that moves into a menu.

## Open Questions
- [ ] **OQ-1** — iOS without a menu bar (AC-5; LIB-14 OQ-3 left it to
  this spec). *Recommendation:* one menu button on the bar with the
  family's menus, as the Web frontend has in its portrait layout (LIB-14
  AC-29); Reset, Preset and Settings leave the bar, and the long press
  for "Clean" goes.
- [ ] **OQ-2** — The model picker on macOS (AC-3): on the control bar,
  as ADR 0024 says and as iOS has it, or in the window's toolbar, as
  today. *Recommendation:* on the bar, the same in every frontend.
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
| One menu and control-bar structure; on iOS the same items appear without a menu bar | ADR 0024; behaviour change 59-14 | 2026-11-10 |
| The module is chosen with a picker on the bar; the setting goes | ADR 0025; behaviour change 59-9 | 2026-11-10 |
| Where there is a menu bar, Reset, Preset and Settings leave the control bar; a notice is a dialog and none is lost; "Loading…" is new | LIB-14 OQ-3, OQ-4 (user, batch 5 gate); behaviour change 59-5 | 2026-11-10 |
| GUI tests carry the same scenarios under the same IDs in every frontend | ADR 0032 | 2026-11-10 |
| Proposed in this spec, for the gate: the reason of a control that cannot be used is its help text on macOS and is not shown on iOS (AC-6) | iOS has no tooltips | 2026-11-10 |
| Proposed in this spec, for the gate: Copy keeps its two forms of today on macOS and iOS (AC-13) | Today's behaviour | 2026-11-10 |
