# QA report: C59-8 Swift frontend on the family structure

Results per acceptance criterion and edge case of `spec.md`.

## M-58 (2026-11-17, branch `m58-swift-59`, the main checkout)

Built and run on an M4 Pro (macOS 27.2, Xcode 27, Apple clang, Ninja,
RelWithDebInfo): `build/tests` (`-DCALCU_FRONTEND_QT=OFF`, Frequent); the
Swift app with `xcodebuild` for macOS and for the iOS simulator; the plan
`UnitTests` on macOS; the plan `UIRegressionTests` on the iPhone 18 Pro
Max simulator (iOS 27.2, `bin/simulators setup` first). The macOS
XCUITests (`CalcU59UITestsMac`, Checkpoint) and the iPad layout were not
run (below).

The code (design §2):
- **Bridge.** `Bridge/TIShell.{h,mm}` (new): the library's shell models for
  Swift from the last Value of each input topic (`control-state`, `busy`,
  `run-state`, `roms-missing`, `settings`, `media`, the `preset-list`
  reply), with the traits of a window with a menu bar (macOS) or without
  (iOS): `menus`, `moreItems`, `bar`, `settingsDialog`, `paperOwner`,
  "Loading…" (`LoadingRule`), the notice queue (`NoticeQueue`), the
  pointer rule (`PointerTracker` on `cu59::faceplateHitMap`), `choose:`;
  value objects `TIMenu`, `TIMenuItem`, `TIBarControl`, `TIDialogSection`,
  `TIDialogRow`. `Bridge/TIShell+Session.h` (ObjC++ only) is what the
  session and the shell tell each other. `TIAppSession` opens the settings
  file (or `--settings-file`), works out the trace folder, and hands the
  file to the session thread with LIB-14's `SettingsPart` (D-3); its
  model, module, printer, reset, screen-key, full-speed and
  keystrokes-playing members went; notices go to the shell's queue
  (`noticesChanged`). `TISettings` and `TIModuleMetadata` went;
  `TISharedTables.keyIndex` went (the pointer rule is the library's, D-9).
- **App.** `Views/ControlBarView.swift` and `Views/ShellCommands.swift`
  (new); `CalculatorView` without the test aids, the toolbar's model
  picker, the Command poll and the long press; `SettingsView` drawn from
  `settingsDialog`; `KeyboardView` on the shell's pointer rule, "Loading…"
  below the display, the cue card and the faceplate as accessibility
  elements; `PrinterView` as the paper panel (its owner's name, an empty
  paper under none; the "PC-100C" toggle went to the bar);
  `EmulatorViewModel` holds `TIShell`; `AppSettings` keeps only the places,
  `--settings-file` and the trace folder's bookmark; `AppInfo` takes its
  texts from `TIShellProduct` (D-8); `MachineModel.swift` went.
- **C++.** `cu59::settingsSpec` for `ShellFrontend::SwiftMac` and
  `SwiftIOS` (design §3.4); `SettingsPartSetup::alsoShown` in the library
  (the iOS row `[window] portrait-debug-page`, which SettingsPart left out;
  test `also_shown_key_in_topic_and_set`); the interim topic `c59.model`,
  `TIModelState`, `TIAppPart::allModules`, `cu59::PointerTracker`,
  `cu59::splitNotice` (the titled alert's split) and
  `cu59::settings::applyLaunchSetting` (`--setting`) went.
- **Tests.** `tests/swift_shell_tests.cpp` (8, Frequent),
  `Calc-U-59Tests/ShellBridgeTests.swift` (9),
  `Calc-U-59UITests/FamilyStructureTests.swift` (17),
  `Calc-U-59UITests/CardScenarioTests.swift` (2, C59-7 AC-28), the other UI
  tests re-pointed (`AppHelpers` launches with `--settings-file` and
  `--attach-port`; `waitIdle`, `waitForOperation`), CTest
  `CalcU59NoTestAids` (`frontends/swift/check_no_test_aids.sh`, Frequent),
  `CalcU59FrontendsDrawControlState` over the Swift sources too.

**The open check of design §8** (`--settings-file` in the simulator):
passes. The XCUITest runner writes the file into its own temporary folder
and the app under test in the same simulator reads it by that path
(`test_c59_8_ac21_settings_file_by_path`: `state settings` through attach
shows the file's `led-style = classic` and `max-file-size-mb = 7`). No
departure was needed. On macOS the app is sandboxed: the helper writes the
file into the app's container `tmp/` and passes its name, which a Debug
build takes in its temporary folder (written, not run: macOS UI tests are
Checkpoint).

| ID | Result | Test case |
|---|---|---|
| AC-1 | Pass (C++; macOS scenario written, not run) | `c59_8_ac1_shortcuts_and_roles` (⌘O, ⌘R, ⇧⌘R; Settings…, Quit, About carry the roles the application menu takes them by; none of About and Quit on iOS); `ShellCommands` draws File, Machine, Help next to macOS's own menus; XCUITest `test_lib14_ac1_the_menus`, `test_lib14_ac4_command_r_resets`, `test_lib14_ac6_application_menu` (macOS branches, Checkpoint) |
| AC-2 | Pass (iOS) | `test_lib14_ac20_help` (More ▸ Calc-U-59 Help is there, enabled, and chosen the app goes to the background for the browser); `test_c59_8_d8_about_texts_are_the_products` (`TIShell.helpURL` is `https://calc-u.org/59/`) |
| AC-3 | Pass | `c59_8_ac3_bar_per_model` (each model: the model picker first, the picker of `slot-1` with None and the 14 modules, "PC-100C", the card controls on the TI-59 only); `test_lib14_ac8_the_bar` (iOS: the model picker before the module picker, LE chosen from the picker reaches the control state) |
| AC-4 | Pass | `c59_8_ac3_bar_per_model` (with a menu bar no Action and no More on the bar); `check_no_test_aids`/the code: no Command poll, no "Clean"; `test_lib14_ac8_the_bar` macOS branch (written) |
| AC-5 | Pass | `c59_8_ac5_more_holds_examples_cold_boot_resume_help`, `c59_8_ac5_cold_boot_asks_first`; `ShellBridgeTests.test_c59_8_ac18_menus_through_the_bridge` (Reset, Preset, Settings on the bar, the rest behind More); XCUITest `test_c59_8_ac5_more_menu` (More ▸ Cold boot asks; confirmed, the display's 42 is gone), `test_lib14_ac1_the_menus` (iOS) |
| AC-6 | Pass (iOS; the macOS help tag by code) | `ShellBridgeTests.test_c59_8_ac6_disabled_with_reason` (Resume disabled with its reason, not carried out); `test_lib14_ac1_the_menus` (More ▸ Resume disabled); macOS draws the reason with `.help` (`reasonTag`), whose XCUITest (`test_c59_8_ac6_help_tag_on_macos`) was not written, see Deviations |
| AC-7 | Pass | `c59_8_ac7_settings_spec_macos_and_ios` (the sections and rows of each system, "Last used" first, None first among the startup presets, no Solid State Module); `ShellBridgeTests.test_c59_8_ac7_settings_rows_and_a_change`; XCUITest `test_lib14_ac11_the_settings` (the iOS sheet with its rows, Done closes it) |
| AC-8 | Pass | `c59_8_ac7_settings_spec_macos_and_ios` (`set-setting` written at once, the [window] row too); `test_c59_8_ac7_settings_rows_and_a_change` (the file holds `led-style = classic`); `test_lib14_ac11_the_settings` (the toggle written, read back through attach) |
| AC-9 | Pass | `ShellBridgeTests.test_c59_8_ac9_loading_during_a_long_load`; XCUITest `test_lib14_ac15_loading` ("Loading…" 1.1 s after an operation at full speed began; the More button still opens its menu under it; gone when it ends) |
| AC-10 | Pass | XCUITest `test_lib14_ac17_a_notice` (two notices, an alert each, the second after the first's OK); `test_c59_8_ec2_…`; `OpeningTests.testLib13AC15RefusalIsANotice` (through the queue) |
| AC-11 | Pass (iOS; macOS by code) | `c59_8_ac1_shortcuts_and_roles` (no About on iOS), `test_lib14_ac6_application_menu` (iOS: no About, no Quit); macOS keeps the system's About panel, its texts from `TIShellProduct` (`test_c59_8_d8_about_texts_are_the_products`) |
| AC-12 | Pass | module picker: `c59_8_ac3_bar_per_model`, `test_lib14_ac8_the_bar`; paper panel: `test_lib8_ac31_the_printer`; run-control buttons: `DebugPanelTests.testC59_4_AC5CalculatorFreezeStepResume`; Examples, startup preset rows: `test_lib6_ac42_an_example`, `c59_8_ac7_…`; keys: `ShellBridgeTests.test_c59_8_ac12_hit_rule_matches_the_library` (every key's centre, the display holds none), `test_lib14_ac24_a_key_under_the_pointer`; the display held: `AttachTests.testTapsReachSessionAtFullSpeed`, `FrameGapTests` |
| AC-13 | Pass | `ShellBridgeTests.test_c59_8_ac13_copy_forms` (macOS: the picture first; iOS's form by code, the iOS unit tests not run) |
| AC-15 | Pass (iPhone; iPad, macOS pictures not run) | XCUITest `test_c59_8_ac15_one_picture_per_layout` (the iPhone's three pages); `LayoutTests` |
| AC-16 | Pass (iOS); macOS written | `FamilyStructureTests`: every row of the table under its ID |
| AC-17 | Pass | `LayoutTests`, `FilePickerTests`, `PresetLoadTests.testLoadDiagPreset` (now run: the load's end through attach), `DebugPanelTests`; `ScreenshotTests` re-pointed, not run (Screenshots plan) |
| AC-18 | Pass | `c59_8_ac18_menus_file_machine_help_in_order` (both traits, each model, the card items on the TI-59 only, Resume disabled not left out); `ShellBridgeTests.test_c59_8_ac18_menus_through_the_bridge` |
| AC-19 | Pass | `c59_8_ac19_no_bar_no_machine_menu_without_control_state`; `LayoutTests.testRomMissingShowsMessage` (no bar, the Help item) |
| AC-20 | Pass | `test_lib6_ac42_an_example` (the cue card "Sum 1..N" in its template `CueCard`) |
| AC-21 | Pass | `CalcU59NoTestAids`; `test_c59_8_ac21_settings_file_by_path`; every UI test launches with `--settings-file` and attach; `AttachTests` S5 presses R/S on the faceplate; `DebugPanelTests` and `ScreenshotTests` press R/S with `key-down`/`key-up`; loads awaited by the busy topic and the bare `wait` |
| EC-1 | Pass | `c59_8_ec1_more_follows_the_control_state` (an operation: the examples disabled, Cold boot available, then enabled again); the bar and More are drawn from the models at each change |
| EC-2 | Pass | XCUITest `test_c59_8_ec2_notice_over_the_settings_sheet` |
| EC-3 | Pass | `c59_8_ac3_bar_per_model`, `c59_8_ac18_…`; XCUITest `test_c59_8_ec3_no_card_controls_on_the_ti58` |
| EC-4 | Pass | XCUITest `test_c59_8_ec4_iphone_portrait_reaches_every_control` |
| EC-5 | Pass | `test_lib8_ac31_the_printer` (the paper under "PC-100C" once attached); `PrinterView` shows an empty paper under none |
| EC-6 | Pass (by code) | `ShellCommands` are the scene's `.commands` on iOS too, so the iPad's menu bar holds them; `test_c59_8_ec6_ipad_menu_bar` (iPad, Checkpoint) was not written |

Test results (2026-11-17): Frequent 47 of 47 (`CalcU59Tests` 553 cases,
`calcu_library_tests` 959; `CalcU59NoTestAids`,
`CalcU59FrontendsDrawControlState` pass); `CalcU59QtTests` (build/groups,
offscreen) 27 of 27; Swift: the macOS and iOS simulator builds succeed;
`UnitTests` on macOS 19 of 19; `UIRegressionTests` on the iPhone 18 Pro
Max simulator 39 tests, 0 failures, 1 skipped
(`LayoutTests.testWideWindowShowsDebugger`, a plain iPhone stays in
portrait), 447 s. Not run: the iPad Pro 13-inch (the iPad's layout is
today's, design §3.8), `CalcU59UITestsMac`, the Screenshots plan.

Column status (INDEX): **Done**.

### Deviations from the design, chosen by the simpler structure

- **No `withTraits:` parameters.** `TIShell` computes with the traits of
  its platform (`withMenuBar`, which the bridge tests flip), not with
  traits passed per call (design §3.1); the iOS and macOS models differ
  only by the menu bar.
- **`TIShell` belongs to `TIAppSession`** (`session.shell`), made with it,
  instead of being made by the view model over the session; the inputs are
  kept by the session's drain (D-2 as designed, one owner fewer).
- **The settings file's rows of `[window]`.** LIB-14's `SettingsPart`
  showed no `[window]` key, so the iOS row Debug Page in Portrait could not
  be drawn from the `settings` topic; `SettingsPartSetup::alsoShown` names
  the keys a frontend shows besides (a library addition, LIB-14's area;
  `also_shown_key_in_topic_and_set`).
- **The trace folder is worked out by the bridge at the start** from the
  settings file it opens (LIB-9's `Paths`), as `AppSettings` did on the
  main thread; a change of `[trace] location` takes effect at the next
  start, as before.
- **More's Cold boot asks with the bar's `confirmationDialog`**, whose
  button carries `confirm-cold-boot`.
- **The notice alert sits at the window's root** (and in the iOS settings
  sheet while it is open) and keeps its own presented state, so a window
  that redraws at 60 Hz keeps it up and the next notice follows once the
  alert has gone; iOS drops the identifier `notice-ok` on an alert's
  button, so the tests tap the alert's OK.
- **The attached harness has no `preset`** (the serve form's registry):
  the tests open a preset with `open <path>`, as a drop does; a notice of
  the session is made with `ti59 asm load` of a bad overlay (C59-4 EC-2).
- **Not written:** `test_c59_8_ac6_help_tag_on_macos`,
  `test_c59_8_ac11_about_panel`, `test_c59_8_ec6_ipad_menu_bar` (macOS and
  iPad UI tests, Checkpoint) and `test_lib10_ac34_run_control_buttons`
  (LIB-10 59 passed in M-53 with `RunControlTests`; the CALCULATOR tab's
  buttons run in `DebugPanelTests`).

### Findings

- The harness `preset` command is refused through attach ("This build
  reads no presets of format 2 yet."): the attached registry has no preset
  vocabulary (as Qt6's attach test expects). A user of `calcu attach` gets
  that misleading sentence; `open <path>` works.
- `c59.full-speed`, `c59.run-state`'s `keystrokes-playing` and
  `c59.model`'s module list have no Swift reader any more; `c59.full-speed`
  has no frontend at all (the display's hold is the library's `speed`).
  `c59.led`, `c59.cue-card` and `c59.printer` (TRACE) stay as product
  topics that Qt6 and Swift read. For M-59's adapter check.
- macOS UI tests (`CalcU59UITestsMac`) and the iPad layout were not run in
  this step (Checkpoint; the user's Accessibility permission).
- `bin/simulators setup` leaves the files of a provisioned simulator as
  they are: the iPhone 18 Pro Max still had 2.3.0's `test_diag.ti59`
  (upgraded in M-54), so `PresetLoadTests` got the refusal instead of a
  load until `bin/simulators files` reinstalled them (noted in the
  `calc-u-59-uitesting` skill).
- The bare `wait` (until idle) is refused while an operation runs
  ("Another operation is running."), so a test cannot wait for a load's
  end with it alone; `waitForOperation` reads the busy topic's stream
  until it says none runs, then asks `wait` (design §3.9 assumed the reply
  to the bare `wait` was enough).
- The Swift card dialog named every element inside it "Arm Card" (the
  container's accessibility label): fixed (C59-7's qa-report).
- C59-7 AC-28's Qt6 scenarios were not in `CalcU59QtTests` after M-57;
  written here (`window_tests.cpp`), so C59-7 can be Done.

### Golden and reference runs

M-58 changes no core and no preset path: the golden runs
(`golden_runs_tests.cpp`) pass in Frequent; the reference runs were not
rerun (Longer; nothing they drive changed).
