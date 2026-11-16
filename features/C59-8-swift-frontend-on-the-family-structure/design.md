# C59-8 Swift frontend on the family structure: design
Status: Draft for the Part D/E boundary (2026-11-16)

## 1. Scope

C59-8 is a product row: one column, one step.

| Column | Step | Tests from (plan §3) | Done in (plan §5.1) |
|---|---|---|---|
| Calc-U-59 | M-58 | AC-1–13, AC-15–21 (AC-14 withdrawn), EC-1–6 | M-58 |

The same step takes:
- LIB-4 AC-10–15, Swift parts (LIB-4 design §10): the Swift frontend
  draws from `control-state`; `App/MachineModel.swift` goes. LIB-4 59 is
  Done in M-58.
- The Swift parts of LIB-8 59 and LIB-10 59 that M-53 leaves to it
  (plan M-53: "Swift parts in M-58"): the paper panel on LIB-8's `paper`
  topic and commands (C59-8 AC-12, AC-13), and FREEZE, STEP and RESUME as
  the run-control buttons for `program` and `cpu` (LIB-10 AC-34). LIB-10
  59 is Done here if M-53 left AC-34's Swift part (plan M-53).
- C59-6 AC-8's picker in Swift (C59-6 qa-report, "Open (Done in M-58)"),
  so C59-6 is Done in M-58; LIB-3 59 is Done in M-58 (plan §5.1).

Paths are at `7bdd51c` under `products/calc-u-59/frontends/swift/`
unless named otherwise; the spec's are at `v2.3.0` (`App/…` there is
`frontends/swift/App/…` here, `Shared/…` is `app/…`).

The steps this design uses:
- M-33 (C59-5): the session thread, the bridge `TIAppSession`, attach in
  Debug builds (`Bridge/TIAppSession.h:137-176`); LIB-14 Part D D.6 widened
  the attached registry to the serve form's commands and the bare `wait`.
- M-53 to M-56: the library's paper, run control, presets, machine
  control, media and opening for Calc-U-59, and C59-4's and C59-7's Swift
  views as their designs say. If one of them leaves a Swift piece undone,
  M-58 takes it over and the qa-report says so (as LIB-14 D-25).
- M-57 (LIB-14 59, LIB-4 59): the More model, the bar without a menu bar,
  labelled choices, `cu59::shellProduct()` and `cu59::settingsSpec()`
  (`app/TIShellProduct.*`), `cu59::faceplateHitMap()`, the faceplate's
  geometry (LIB-14 design §10.3, §10.6).

Not here: what the menus and the bar hold (LIB-14), the behaviour behind
each control (LIB-6, LIB-7, LIB-8, LIB-10, LIB-13, C59-3), the debug
panel and the card controls (C59-4, C59-7: their Swift views), hosting
(C59-5).

## 2. Modules and files

### Bridge (`Bridge/`)

| File | Change |
|---|---|
| `TIShell.h/.mm` (new) | the shell's models for Swift (§3.1): keeps the last Value of each input topic from the bridge's messages and computes `calcu::shell::menus`, `moreItems`, `bar`, `dialog`, `LoadingRule`, `NoticeQueue` and the pointer rule (`PointerTracker` over `cu59::faceplateHitMap`) for the traits it is given; returns ObjC values (`TIMenu`, `TIMenuItem`, `TIBarControl`, `TIDialogSection`, `TIDialogRow`); `choose:` sends an item's command through `TIAppSession` |
| `TIAppSession.h/.mm` | `send:args:` for a library command by name with a Value from `TIShell` (C59-5 A-2's one factory per command stays for the product's own); the model, module, printer and reset methods (`TIAppSession.h:170-176`) and `TISessionStart.moduleID` (`:54`) go; `SettingsPart` among the parts the session thread builds, on the settings file named in `TISessionStart` (D-3) |
| `TISettings.h/.mm` | deleted (D-3) |
| `TISharedTables.h/.mm` | `keyIndexAtX:y:` (`TISharedTables.h:31-32`) goes: the pointer rule is the library's (D-9); the drawing tables stay |
| `TIViewRules.h/.mm` | stay: the layout rules of AC-15 (`mobileLayoutWidth`, `mobileBarLabels`, …) are product material |

### App (`App/`)

| File | Change |
|---|---|
| `MachineModel.swift` | deleted (LIB-4 AC-15): the model is the control state's ID; the art (`ti59_base`, `ti58_base`) is chosen by it in the view |
| `AppSettings.swift` | `SettingsModel` over `TISettings` (`AppSettings.swift:47-90`) goes; the views read the `settings` topic through `TIShell`; the macOS trace-folder bookmark stays in the platform store (`:92-94`; LIB-9 AC-34) |
| `Calc-U-59App.swift` | the family's menus as `.commands` (§3.2) on macOS and iPadOS; the Settings scene (`:35-37`) draws `SettingsView` from `TIShell`; About stays the system panel (`:39-43`; AC-11); `RomsMissingView` (`:49-63`) gets the Help item (AC-19) |
| `EmulatorViewModel.swift` | holds `TIShell`; `model`, `printerConnected`, `start(model:)`, `allModules`, `changeModule`, `resetMachine`, `cleanResetMachine`, `togglePrinterConnection` (`:71, 90, 227, 299-321`) go; `errorTitle`/`errorMessage` (`:72-74, 521-522`) give way to the notice queue; `isKeystrokesPlaying` (`:83, 486`) and `--module` (`:197`) go (AC-21) |
| `Views/CalculatorView.swift` | the test aids (`:84-147`) go; the toolbar's model picker (`:169-173`, `:406-418`) goes; the alert (`:174-181`) shows the notice queue; `cardReaderBar` (`:302-402`) gives way to `ControlBarView`; the Command-key poll (`:214-222`) and the long press (`:326-335`) go; the layout (`:226-277`, `:429-477`) stays (AC-15); "Loading…" over the faceplate (§3.5) |
| `Views/ControlBarView.swift` (new) | the bar from `TIShell.bar` (§3.3): model picker, slot picker, the "PC-100C" button, C59-7's card controls, and on iOS Reset, Preset, Settings and More |
| `Views/ShellCommands.swift` (new) | the `Commands` of §3.2 from `TIShell.menus` |
| `Views/SettingsView.swift` | draws `TIShell.dialog` (§3.4); the module row (`:64`) goes |
| `Views/KeyboardView.swift` | the hit test and the display hold (`:130-165`) give way to `TIShell`'s pointer rule; `UIKitTouchTracking` stays the source of touches |
| `Views/PrinterView.swift` | the paper from LIB-8's `paper` topic and commands, as far as M-53 left it; Copy keeps its two forms (`:227-239`; AC-13) |
| `Views/DebugView.swift`, `LiveDebugView.swift`, `CPUInspectorView.swift` | FREEZE, STEP, RESUME send LIB-10's `pause`, `step`, `resume` for `program` and `cpu`, enabled from `run-state` (LIB-10 AC-18, AC-34), as far as M-53 and M-56 left them |
| `AppInfo.swift` | stays (the macOS About panel) |

### Tests

| File | Holds |
|---|---|
| `products/calc-u-59/tests/swift_shell_tests.cpp` (new, `CalcU59Tests`) | the C++ side of what Swift draws: the models with Swift's traits on the real session (§6) |
| `Calc-U-59Tests/ShellBridgeTests.swift` (new) | `TIShell` through the real bridge (`CalcU59SwiftUnitTests`, Longer) |
| `Calc-U-59UITests/FamilyStructureTests.swift` (new) | the scenarios of AC-16, in `UIRegressionTests` (iOS, Longer) and on macOS (`CalcU59UITestsMac`, Checkpoint) |
| `Calc-U-59UITests/AppHelpers.swift`, `HarnessClient.swift` | launch with `--attach-port` and `--settings-file`, no `--UITesting`; `waitIdle()` (the bare `wait`) |
| `Calc-U-59UITests/LayoutTests.swift`, `FilePickerTests.swift`, `PresetLoadTests.swift`, `ScreenshotTests.swift`, `AttachTests.swift` | re-pointed (AC-17, AC-21) |
| `frontends/swift/check_no_test_aids.sh` (new; CTest `CalcU59NoTestAids`, Frequent) | the app's sources hold no `--UITesting`, `isUITesting`, `btn-key-rs`, `keystroke-playback-status`, `MachineModel` (AC-21, LIB-4 AC-15) |

## 3. Seams and interfaces

### 3.1 One model for every frontend (Technical Requirements; D-1, D-2)

The Swift frontend draws the library's shell models, computed in the
bridge from the same topics the Qt6 and React sets read; no Swift view
decides what is offered.

```objc
@interface TIShell : NSObject
- (instancetype)initWithSession:(TIAppSession *)session;     // takes the messages the bridge passes on (C59-5 §3.3)
@property (nonatomic, readonly) BOOL started;                 // a control state came (AC-19)
- (NSArray<TIMenu *> *)menusWithTraits:(TIShellTraits *)t;    // calcu::shell::menus
- (NSArray<TIMenuItem *> *)moreItemsWithTraits:(TIShellTraits *)t;   // calcu::shell::moreItems
- (NSArray<TIBarGroup *> *)barWithTraits:(TIShellTraits *)t;  // calcu::shell::bar (B1 to B9)
- (NSArray<TIDialogSection *> *)settingsDialog;               // calcu::shell::dialog over cu59::settingsSpec(swift, system)
- (nullable NSString *)loadingAt:(double)now;                 // LoadingRule (AC-9)
- (nullable NSString *)currentNotice;  - (void)closeNotice;   // NoticeQueue (AC-10)
- (void)pointerDown:(int64_t)p x:(double)x y:(double)y;       // PointerTracker over cu59::faceplateHitMap(model)
- (void)pointerMove:(int64_t)p x:(double)x y:(double)y;  - (void)pointerUp:(int64_t)p;
- (void)choose:(TIMenuItem *)item;                            // its command and args, or a shell action by id
@property (nonatomic, copy, nullable) void (^changed)(void);  // on the main thread, after an input changed
@end
```

- **Inputs.** `TIShell` keeps, on the main thread, the last `toValue()`
  of the inputs of LIB-14 §3.1 (`control-state`, `busy`, `run-state`,
  `roms-missing`, `settings`, `media`, the reply of `preset-list`) from
  the bridge's message batches; it holds nothing else of the machine and
  decides nothing of its own (C59-5 Technical Requirements). The models
  are computed when an input changes (EC-1).
- **Traits.** macOS `{hasQuit: true, shortcuts: true, savedMedia: false,
  menuBar: true, settings: true, about: true}`; iOS and iPadOS the same
  with `menuBar: false`, `about: false`, `hasQuit: false` (AC-5, AC-11).
- **Commands** go out through `TIAppSession` as the library's names
  (`model`, `attach`, `detach`, `reset`, `cold-boot`, `resume`,
  `load-preset`, `set-setting`, `screen-key-down`, `screen-key-up`,
  `speed`); their refusals reach the notice queue (D-7 of LIB-14).

### 3.2 Menus (AC-1, AC-2, AC-18, EC-6)

`ShellCommands` builds SwiftUI `Commands` from `menusWithTraits:`:
File's items through `CommandGroup(replacing: .newItem)` (Load Preset…
⌘O, Examples ▸, C59-7's card items, which M17 lists on the TI-59 only);
`CommandMenu("Machine")` (Model ▸, Peripherals ▸, Reset ⌘R, Cold boot
⇧⌘R, Resume); Help through `CommandGroup(replacing: .help)` ("Calc-U-59
Help" opens `https://calc-u.org/59/` with `openURL`, AC-2). Settings…
is the Settings scene's item and About the system's, both in the
application menu; Quit and the menus Edit, View, Window are macOS's own
and stay (AC-1). An item that cannot be used is disabled, not left out
(AC-18; LIB-14 M9). The shortcuts are menu key equivalents, so they are
taken before `KeyboardView` sees the key (AC-1). The same `Commands`
appear in the iPad's menu bar (EC-6); the system's own settings item of
the app menu is left to the system (OQ-1).

### 3.3 The control bar and the More button (AC-3, AC-4, AC-5, AC-6, EC-3, EC-4)

`ControlBarView` draws `barWithTraits:` group by group:
- the model picker (`Picker`, `.menu` style), first on both systems
  (AC-3; the toolbar item goes);
- the picker of `slot-1`: the Swift counterpart of LIB-7's module picker
  (AC-12): the entries of the `media` topic's listing for `slot-1` with
  the module's description as label and the empty slot (LIB-7 AC-8,
  AC-10);
- the button "PC-100C" (B3), pressed while plugged in;
- C59-7's card controls on the TI-59 (EC-3: none on the TI-58 and TI-58C,
  because the control state lists no card reader);
- on iOS and iPadOS (B8): Reset, Preset and Settings as today's buttons
  (`CalculatorView.swift:317-393`, their identifiers `btn-reset`,
  `btn-preset`, `btn-settings` kept), and More as a SwiftUI `Menu` with
  the `ellipsis.circle` image and the identifier `bar-more`, holding
  `moreItemsWithTraits:` (Examples ▸, Cold boot, Resume, "Calc-U-59
  Help"). Cold boot has `role: .destructive` and asks through
  `confirmationDialog` before it sends `cold-boot` (M15).

A control drawn from an entry that is not available now is disabled; on
macOS its reason is `.help(reason)`, which XCUITest reads as the
element's accessibility help; on iOS no reason is shown (AC-6). Where
the bar is too narrow, the controls show their icons only, by
`TIViewRules.mobileBarLabels` as today (EC-4).

### 3.4 Settings (AC-7, AC-8)

`SettingsView` draws `settingsDialog` (LIB-14 §3.5 with S8's labels):
macOS the Settings scene's window, iOS a sheet whose one button is Done
(`SettingsView.swift:39-40`). A change sends `set-setting` at once
(AC-8). Rows by widget: a choice → `Picker`; a choice whose values are
`true` and `false` → `Toggle` (D-6); a folder → the path and Change… /
Reset (macOS); an integer → a number field with "MB"; a note → text.
`cu59::settingsSpec(swift, system)` gives:

| Section | Row | macOS | iOS |
|---|---|---|---|
| General | Startup model ("Last used", TI-59, TI-58, TI-58C) | yes | yes |
| General | the version (a note) | yes | yes |
| Startup presets | TI-59, TI-58, TI-58C: "None" and the startup presets (no "Choose…", OQ-3) | yes | yes |
| Display | Segment Style: `modernized` "Modernized", `classic` "Classic" | yes | yes |
| Keyboard | Keyboard Feedback: `off` "Off", `haptic` "Haptic", `click` "Click Sound" | no | yes |
| Layout | Debug Page in Portrait (`[window] portrait-debug-page`) | no | yes |
| Tracing | where traces go: `[trace] location`, `app-support` "Local (App Library)", `icloud` "iCloud", and on macOS `folder` "Custom Folder" with the row `[trace] folder` (today's labels, `AppSettings.swift:103-108`; LIB-9 AC-24, AC-34) | yes | yes |
| Tracing | Maximum file size (MB) | yes | yes |

The macOS folder row asks the host's folder picker, whose answer keeps
the security-scoped bookmark as today (C59-5 §3.4).

### 3.5 "Loading…", notices, About, a failed start (AC-9, AC-10, AC-11, AC-19, EC-2)

- "Loading…": a small `ProgressView` with the text, placed over the
  faceplate below the display area of the hit map, with
  `.allowsHitTesting(false)`, shown while `loadingAt:` says so (AC-9).
- Notices: one `alert` whose only text is `currentNotice` and an OK
  button that calls `closeNotice`; the next one follows (AC-10). It is
  presented from the topmost view, so it shows over the settings sheet
  (EC-2).
- About: the system's About panel with `AppInfo.aboutPanelOptions` on
  macOS (AC-11); none on iOS, the version is in the settings.
- A failed start: `RomsMissingView` with the lookup's text, no bar, no
  Machine menu, and the Help item (AC-19).

### 3.6 The native counterparts (AC-12, AC-13)

| Counterpart | Drawn from | Criteria |
|---|---|---|
| module picker of `slot-1` | `media`'s listing, `control-state`'s `slot-1` | LIB-7 AC-8, AC-10 |
| paper panel with Copy and Cut | LIB-8's `paper` topic; Copy as today (`PrinterView.swift:227-239`: macOS two entries, picture first; iOS one item with both) | LIB-8 AC-12–20, AC-28; AC-13 |
| run-control buttons | `run-state`; `pause`, `step`, `resume` for `program` and `cpu`; step repeats four times a second while held (`HoldRepeatButton.swift`) | LIB-10 AC-18, AC-34 |
| Examples menu, startup preset rows | `preset-list` | LIB-6 AC-32; LIB-14 AC-12 |
| on-screen keys | `TIShell`'s pointer rule on `cu59::faceplateHitMap`, the card tap first (C59-7) | LIB-14 AC-24 |
| holding the display | the same rule's H5 (`speed {full}`) | LIB-14 AC-43 |

### 3.7 The cue card (AC-20)

`CueCardView` draws the cue card the product topic names (C59-2 AC-6,
C59-7) above the top row of keys, in its template, as today; only its
input moves to the topic M-55 leaves.

### 3.8 Layout (AC-15)

Unchanged (`CalculatorView.swift:226-277, 429-477`): macOS three columns;
iOS side by side from 820 × 560 points, faceplate and paper alone when
wider than high and smaller, else pages with the corner arrows; the
debug page among them while `[window] portrait-debug-page` is true,
read from the `settings` topic. The paper panel is always there (LIB-8
AC-12, EC-5).

### 3.9 The UI tests through attach (AC-16, AC-17, AC-21)

- The app has no `--UITesting`, no hidden R/S button and no
  `keystroke-playback-status` (AC-21). A Debug build takes
  `--settings-file <path>`: the settings file of that launch, given to
  the session at its start; Release has none (`#if DEBUG`). The
  `--setting` and `--module` launch arguments go: a test writes its
  settings file, and sets the module through attach (`attach slot-1 <id>`).
- A test launches with `--attach-port N` and connects
  `HarnessClient` to 127.0.0.1:N from the runner (C59-5 S3). A load
  started with a tap is awaited by the reply to the bare `wait` (LIB-14
  Part D D-27), read from the message stream. A key into a running
  program (an overlay among them) is pressed with `key-down` and
  `key-up`.
- `FamilyStructureTests` carries AC-16's rows under their IDs
  (`test_lib14_ac1_the_menus`, `test_lib14_ac4_command_r_resets`,
  `test_lib14_ac6_application_menu`, `test_lib14_ac8_the_bar`,
  `test_lib14_ac11_the_settings`, `test_lib14_ac15_loading`,
  `test_lib14_ac17_a_notice`, `test_lib14_ac20_help`,
  `test_lib14_ac24_a_key_under_the_pointer`,
  `test_c59_8_ac15_one_picture_per_layout`, `test_lib6_ac42_an_example`,
  `test_lib8_ac31_the_printer`); on iOS a row that is "not there" is a
  test that finds no such element.

## 4. Product side: what is replaced

| Today | After M-58 |
|---|---|
| `App/MachineModel.swift:9-34` (the model facts) | the control state (LIB-4 AC-15) |
| the model picker in the window toolbar (`CalculatorView.swift:169-173`) | the bar's first control (AC-3; row 59-14) |
| Reset with Command for "Clean", Preset on the bar (macOS, `CalculatorView.swift:305-315, 368-378`) | Machine ▸ Reset, Cold boot; File ▸ Load Preset… (AC-4) |
| the long press on Reset for "Clean" (iOS, `:326-335`) | More ▸ Cold boot, asking first (AC-5) |
| the module row of the settings (`SettingsView.swift:64`) | the picker of `slot-1` (row 59-9) |
| a titled alert, one at a time, a second replacing the first (`CalculatorView.swift:174-181`) | the notice queue (AC-10) |
| `TISettings` on the main thread (C59-5 D-3) | `SettingsPart` on the session thread (D-3) |
| `cu59::PointerTracker` behind `TISharedTables.keyIndex` (`app/KeyboardLayout.hpp:103`) | the library's `PointerTracker` (D-9); `cu59::PointerTracker` is deleted |
| the interim topic `c59.model` and the commands of LIB-4 design §10.3 | gone (LIB-4 §10.7) |
| the test aids (`CalculatorView.swift:84-147`, `AppSettings.swift:62-65`, `EmulatorViewModel.swift:197`) | attach and `--settings-file` (AC-21) |

## 5. Adapters

Removed in M-58:

| Adapter | Where | Named by |
|---|---|---|
| interim topic `c59.model`, and the model, module, printer and reset commands | `app/TIAppPart.*` | LIB-3 §5, LIB-4 §5 and §10.7 |
| the test aids | `App/Views/CalculatorView.swift:84-147` | C59-5 §5 |
| `cu59::PointerTracker` | `app/KeyboardLayout.hpp:103-…` | LIB-14 §4 |
| the Swift readers of the interim product topics that M-53 to M-56 replaced (`c59.printer`, `c59.run-state`, `c59.card-reader`, …) | `Bridge/TIAppSession.*`, `App/EmulatorViewModel.swift` | C59-5 §5 (each with its owner's step; the Swift half here) |

Plan §1: no adapter is left at the end; M-59 checks it.

## 6. Tasks

Tests before the code. C++ cases in `CalcU59Tests`
(`tests/swift_shell_tests.cpp`, Frequent), names `c59_8_acN_…`; bridge
cases in `CalcU59SwiftUnitTests` (Longer); XCUITests in
`UIRegressionTests` (`CalcU59UIRegression`, Longer, iPhone simulator)
and `CalcU59UITestsMac` (Checkpoint, run at the end of Part E).

| Task | What | ACs | Test |
|---|---|---|---|
| T1 | Menus with Swift's traits | AC-1, AC-2, AC-18, EC-6 | `c59_8_ac18_menus_file_machine_help_in_order` (the card items on the TI-59 only; disabled, not left out), `c59_8_ac1_shortcuts_and_roles`; `ShellBridgeTests`: `test_c59_8_ac18_menus_through_the_bridge`; XCUITest `test_lib14_ac1_the_menus`, `test_lib14_ac4_command_r_resets`, `test_lib14_ac6_application_menu`, `test_lib14_ac20_help` (macOS); `test_c59_8_ec6_ipad_menu_bar` (iPad simulator, Checkpoint) |
| T2 | The bar | AC-3, AC-4, EC-3, EC-4 | `c59_8_ac3_bar_per_model` (model picker first; slot picker with 14 modules and None; "PC-100C"; card controls on the TI-59 only); XCUITest `test_lib14_ac8_the_bar` (both systems), `test_c59_8_ec3_no_card_controls_on_the_ti58`, `test_c59_8_ec4_iphone_portrait_reaches_every_control` |
| T3 | Without a menu bar | AC-5, EC-1 | `c59_8_ac5_more_holds_examples_cold_boot_resume_help`, `c59_8_ac5_cold_boot_asks_first`, `c59_8_ec1_more_follows_the_control_state` (an operation starts: the examples are disabled, Cold boot stays available, LIB-4 AC-3); XCUITest `test_c59_8_ac5_more_menu` (iOS) |
| T4 | Availability and reasons | AC-6 | `ShellBridgeTests`: `test_c59_8_ac6_disabled_with_reason`; XCUITest `test_c59_8_ac6_help_tag_on_macos` (macOS: the element's help is the reason) |
| T5 | Settings | AC-7, AC-8, EC-2 | `c59_8_ac7_settings_spec_macos_and_ios`; XCUITest `test_lib14_ac11_the_settings` (window on macOS, sheet with Done on iOS; a change written: read back through attach `state settings`), `test_c59_8_ec2_notice_over_the_settings_sheet` |
| T6 | "Loading…", notices, About, ROMs missing | AC-9, AC-10, AC-11, AC-19 | XCUITest `test_lib14_ac15_loading` (a preset at full speed; the keys and the bar still take taps), `test_lib14_ac17_a_notice` (two notices, one after the other), `test_c59_8_ac11_about_panel` (macOS); `c59_8_ac19_no_bar_no_machine_menu_without_control_state` |
| T7 | Counterparts | AC-12, AC-13 | `test_lib14_ac24_a_key_under_the_pointer`, `test_lib6_ac42_an_example`, `test_lib8_ac31_the_printer` (XCUITest); `ShellBridgeTests`: `test_c59_8_ac12_hit_rule_matches_the_library` (the lines of `library/shell/tests/hit_cases.txt`), `test_c59_8_ac13_copy_forms` (macOS and iOS pasteboards); `test_lib10_ac34_run_control_buttons` (XCUITest: FREEZE, STEP, RESUME on both tabs) |
| T8 | Cue card | AC-20 | XCUITest `test_c59_8_ac20_cue_card_above_the_keys` (an example with a cue card; the element shows its template) |
| T9 | Layout | AC-15 | XCUITest `test_c59_8_ac15_one_picture_per_layout` (macOS one; iPhone portrait pages; iPad landscape side by side), `LayoutTests.swift` re-pointed |
| T10 | Today's checks | AC-17 | `LayoutTests`, `FilePickerTests`, `PresetLoadTests`, `ScreenshotTests` pass with Reset and Load Preset… found in the menus on macOS |
| T11 | Test aids gone | AC-21 | `CalcU59NoTestAids` (Frequent); `AttachTests` R/S through `key-down`/`key-up`; every test that waits for a load uses the reply to `wait` |
| T12 | LIB-4 59's Swift half | LIB-4 AC-15 | `CalcU59FrontendsDrawControlState` (LIB-4 §10.6) with `App/MachineModel.swift` gone |

Coverage: AC-1–13, AC-15–21 and EC-1–6 (26 of 26).

## 7. Tools and agent knowledge

**Tools.** `calcu attach` drives the app as before; the UI tests use no
test aid of the app. The Swift app is built with `xcodebuild` from the
main checkout: from a `.claude/worktrees/` worktree its planning fails
("no XCFramework found"; handover, "Findings not raised yet"), so M-58's
Swift builds and XCUITests run there.

**Agent knowledge changed in the same step.**

| File | Change |
|---|---|
| `products/calc-u-59/.claude/skills/calc-u-59-swift/SKILL.md` | `TIShell` and the shell's models; the menus as `Commands`, `ControlBarView`, the More button; settings through `SettingsPart`; `MachineModel.swift` and `TISettings` gone |
| `products/calc-u-59/.claude/skills/calc-u-59-uitesting/SKILL.md` | no `--UITesting`, `--setting`, `--module` (`SKILL.md:31, 46, 138`); `--settings-file`, attach, `waitIdle`; the identifier table (`:148-165`): `btn-key-rs` and `keystroke-playback-status` gone (`:151, 161, 221-226, 258`), `bar-more`, `more-item-<id>` new; `FamilyStructureTests` |
| `products/calc-u-59/reference/AppArchitecture.md`, `FrontendParity.md`, `NewGUIGuide.md` | the Swift frontend on the family structure |
| `products/calc-u-59/CLAUDE.md` | the Swift frontend draws the library's shell models through `TIShell` |
| memory `calc-u-59/screenshot_convention.md` | the screenshot tests launch without `--UITesting` |
| `docs/glossary-mapping.md` | the visible renames for the Swift frontend done ("Clean" → Cold boot, "Startup Calculator" → Startup model) |
| `docs/test-groups.md` | `CalcU59NoTestAids`, the new cases |

## 8. Golden runs, reference runs and risks

- Golden runs and the 57 reference runs drive no frontend; M-58 changes
  no core. A difference is a finding.
- **The settings file on the session thread.** Since M-54 the library's
  `StartupPart` writes `[startup] last-model` on the session's thread,
  while `TISettings` writes on the main thread (C59-5 D-3): two writers
  of one file. M-58 ends it (D-3); if M-54 already moved the file to the
  session, M-58 only removes `TISettings`.
- **`--settings-file` in the simulator.** That an XCUITest runner can
  write a file the app under test reads by path is not verified; the
  first test of M-58 checks it. If it cannot, the argument carries the
  file's text instead, a departure the qa-report records.
- **SwiftUI `Commands` from a model that changes.** Menus rebuilt from
  `TIShell` on every change may flicker or close an open menu; the
  models change at most on control-state changes, not at 60 Hz.
- **Simulator UI tests** need more work later (spec OQ-5); a flaky run is
  rerun once and recorded, as at the end of Part D.
- **CI.** No workflow or packaging change; the Swift app is not built
  in CI. The local checks are the Xcode builds for macOS and the iOS
  simulator and the Longer XCUITests.

## 9. Decided (best guess)

- **D-1 (Technical Requirements; LIB-14 D-34).** The Swift frontend
  draws the library's shell models, computed by the C++ of
  `library/shell/` through the bridge, so the menus, the bar, the More
  button and the settings rows are one rule in all three frontends.
- **D-2.** `TIShell` keeps the last Value of the shell's input topics on
  the main thread; this is the shell's input, not machine state, and
  the bridge still decides nothing.
- **D-3 (supersedes C59-5 D-3).** The Swift settings file is the
  session's: `SettingsPart` on the session thread, `TISettings` gone,
  the views read the `settings` topic. One owner of the file (LIB-9),
  as in Qt6 and the Web.
- **D-4 (AC-1, EC-6).** The family's menus are SwiftUI `Commands`:
  File's items replace the `.newItem` group, Machine is a
  `CommandMenu`, Help replaces the `.help` group; the same `Commands`
  serve the iPad's menu bar.
- **D-5 (AC-5).** More is a SwiftUI `Menu` with the `ellipsis.circle`
  image; Cold boot in it is `.destructive` and asks with a
  `confirmationDialog`.
- **D-6 (AC-7).** A choice row whose values are `true` and `false` is
  drawn as a `Toggle` (Debug Page in Portrait), the platform's form.
- **D-7 (AC-7).** Tracing keeps today's choices and labels for where
  traces go (`AppSettings.swift:98-108`) as labelled choices of `[trace]
  location`, with the folder row on macOS.
- **D-8 (AC-11).** The macOS About stays the system panel from
  `AppInfo`; its summary and links are the same as `TIShellProduct`'s,
  which a unit test compares.
- **D-9 (AC-12).** The keys under a touch or a click are found by the
  library's `PointerTracker` on `cu59::faceplateHitMap()`, the map Qt6
  and the Web use; `cu59::PointerTracker` goes.
- **D-10 (AC-21).** `--settings-file` is the one test argument of a
  Debug build; the module is set through attach, not by an argument.
- **D-11.** Swift pieces of LIB-8, LIB-10, LIB-13, C59-4 and C59-7 that
  M-53 to M-56 leave are taken over here, as LIB-14 D-25.

## 10. Plan notes (proposals; the plan is not edited)

- M-58 "Completes" gains LIB-8 59's Swift part (plan M-53 names it
  C59-8's), C59-6 (§5.1, Done in M-58) and LIB-3 59 (§5.1).
- M-58 "Waits for" names M-57 for the shell's models of LIB-14 §10.3,
  which it has through "M-53 to M-57".
- M-58 "Knowledge, ledger" gains the memory file of §7 and
  `CalcU59NoTestAids` in `docs/test-groups.md`; it is run from the main
  checkout for the Swift builds.
- **Ledger.** Rows 59-9, 59-14 (Swift), 59-44 and 59-45 become visible
  in Swift. A row is proposed (number at the merge): **59-new-E
  (Swift).** A notice is an alert with only its sentence and OK, and
  several are shown one after the other (today a titled alert whose
  second replaces the first); "Loading…" is shown during a long load;
  the Settings show the rows of the family (Startup model with "Last
  used" first, a Startup presets section) and no Solid State Module.
  Sources: C59-8 AC-7, AC-9, AC-10.

## 11. Questions for the user

None that split the design.
