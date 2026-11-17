# LIB-14: QA report

Results per acceptance criterion and edge case of `spec.md`, per column.

## Library (M-43, 2026-11-15, branch `m43-shell`)

Built and run on an M4 Pro, macOS 27.2, Apple clang, CMake, Ninja,
RelWithDebInfo, in two trees: `build/tests` (`-DCALCU_FRONTEND_QT=OFF`)
and `build/qt` (`-DCALCU_FRONTEND_QT=ON`, Homebrew Qt 6 with Qt Test). The
Web parts with Homebrew Emscripten in `build/web` and Playwright (Chromium,
WebKit).

The code:
- `library/shell/` (`calcu::shell`, a new area of `calcu_library`; LIB-1
  §2a updated): `Inputs.*` (the inputs in their Value form, the command
  names, Value reading), `ShellProduct.*` (with `about()`,
  `checkShellProduct()`), `MenuModel.*` (M1 to M12), `BarModel.*` (B1 to
  B7 and `panels()`), `SettingsPart.*` (the topic `settings`, the commands
  `set-setting` and `remember-folder`), `SettingsModel.*` (S1 to S7),
  `Loading.*` (L1), `Notices.*` (N1 to N4, D-7), `HitMap.*` (H1 to H6),
  `ObjectNames.hpp` (§3.10); tests in `library/shell/tests/` with
  `TestShellPart.hpp` (the control state, preset list and product built for
  the tests, from LIB-4's `ControlState` and LIB-6's `PresetListReply`),
  `hit_cases.txt` and `inputs.txt` (shared with the JS mirror);
- the Qt6 component set `library/qt6/` (`calcu_qt6`): `SessionHost.*` (the
  GUI-thread host with harness attach), `Shell.*`, `ShellMenus.*`,
  `ShellControlBar.*`, `ShellSettingsDialog.*`, `ShellLoading.*`,
  `ShellNotices.*`, `ShellAbout.*`, `PanelArea.*`, `FaceplateInput.*`,
  `RomsMissingView.*`; tests `tests/TestWindow.*`,
  `tests/shell_widgets_tests.cpp`, `tests/attach_host_tests.cpp` (in
  `calcu_qt6_tests`), `tests/shell_scenarios.cpp` (program
  `calcu_qt6_scenarios`), `tests/check_no_product_code.sh`
  (`LibraryShellNoProduct`);
- the React component set `library/web/components/`: `shellModel.js`,
  `useShell.jsx` (`useCalcuShell`), `ShellMenuBar.jsx`,
  `ShellControlBar.jsx`, `ShellModal.jsx` (with `ShellDialogs`),
  `ShellSettings.jsx`, `ShellAbout.jsx`, `ShellNotice.jsx`,
  `ShellLoading.jsx`, `ShellPanelArea.jsx`, `useShellFaceplate.jsx`,
  `ShellRomsMissing.jsx`, `shell.css`, appended to `COMPONENT_SOURCES` and
  `COMPONENT_CSS` of `library/web/build/dist.js`; tests
  `library/web/tests/shell_model.test.js` (in `LibraryWebNodeTests`) and
  `shell.spec.js` (projects `shell-chromium`, `shell-webkit` of
  `LibraryWebPlaywright`);
- the Web test product (`library/web/tests/product/`): TestShellPart and
  the library's SettingsPart in its session, [preset] keys per model and
  [trace] max-file-size-mb in its declaration, the component put together
  from the shell with the host page's `shell=1`.

No product code changed; no product links `calcu_qt6` or loads the shell
yet (design §1). No `library/sessions/` change.

### Checks run

| Check | Result |
|---|---|
| Frequent, `build/tests` (Qt OFF) | 31 of 31 entries pass (30 before; `LibraryShellNoProduct` new) |
| Frequent, `build/qt` (Qt ON) | 33 of 33 entries pass (31 before; `LibraryShellNoProduct`, `calcu_qt6_scenarios` new) |
| `calcu_library_tests` | 674 `TEST`s (629 before; 45 in `library/shell/tests/`), 0 failed |
| `calcu_qt6_tests` | 42 `TEST`s (10 before; 28 shell widgets, 4 attach host), 0 failed, 0 skipped |
| `calcu_qt6_scenarios` | 9 `TEST`s, 0 failed |
| `LibraryWebNodeTests` | 44 tests (4 before; 40 in `shell_model.test.js`), 0 failed |
| `LibraryWebPlaywright` (Longer) | 122 cases (94 before; 14 of `shell.spec.js` in two engines), 120 passed, 2 skipped as before |
| `LibraryShellNoProduct` | passes; a planted `"TI-59"` in a copy of the folders makes it fail |

### Acceptance criteria

The C++ model case and the JS model case of the same name check the same
rule on the same inputs (`inputs.txt` is written from the payload types
and compared by `inputs_fixture_matches_payloads`, so a change of a
payload's `toValue()` fails both). "Qt6" is `calcu_qt6_tests` on the
offscreen platform, "scenario" `calcu_qt6_scenarios`, "Web" `shell.spec.js`.

| ID | Result | Evidence |
|---|---|---|
| AC-1 | Pass | Model: `ac1_menus_and_items_in_order`, `ac1_item_without_function_and_empty_menu_left_out` (C++, JS). Qt6: `ac1_menus_and_items_on_widgets` (menu titles and item order from `QMenuBar`, Examples ▸ sends `load-preset {ref}`, Load Preset… opens the file dialog). Scenario `ac1_the_menus`; Web `ac1 the menus` |
| AC-2 | Pass | Model: `ac2_model_menu_marks_running_and_sends_model`, `ac2_variant_submenus_for_model_and_peripheral`. Qt6: `ac2_model_and_variant_menus_on_widgets` (the product's label "Revision", `variant {name}`, `model {name}`, the new model checked) |
| AC-3 | Pass | Model: `ac3_peripherals_attach_replace_and_detach`. Qt6: `ac3_peripherals_on_widgets` (attach, detach of the checked one, no entry for the host drive) |
| AC-4 | Pass | Model: `ac4_shortcuts_qt_and_none_on_web`. Qt6: `ac4_shortcuts_on_widgets` (Ctrl+O, Ctrl+R, Ctrl+Shift+R as window shortcuts; Ctrl+R and Ctrl+Shift+R typed on the focused faceplate reach Reset and Cold boot first, with Qt Test). Scenario `ac4_a_shortcut`; Web `ac4 no shortcut is bound` (Ctrl+R, Ctrl+O, Ctrl+Shift+R are not taken, nothing sent) |
| AC-5 | Pass | Model: `ac5_unavailable_disabled_with_reason`. Qt6: `ac5_disabled_with_reason_on_widgets` (disabled `QAction` with the reason as tooltip, `toolTipsVisible`, the model picker's item disabled with `Qt::ToolTipRole`, the place button's tooltip, the product item from its entry). Web `ac1 the menus` (`aria-disabled` and `title`) |
| AC-6 | Pass | Model: `ac6_roles_and_no_quit_on_web`. Qt6: `ac6_roles_on_widgets` (Preferences, About, Quit roles; every other item `NoRole`). Scenario `ac6_the_platforms_places`; Web `ac6 no quit`. The native macOS application menu is not drawn offscreen; it is seen in M-49's shot scenarios (design §8) |
| AC-7 | Pass | Model: `ac7_product_item_in_its_group` (FileOpen, FileKinds, FileMedia, FileEnd, Edit, Help; disabled from its product entry). Qt6: `ac7_product_item_on_widgets` (its act runs, nothing is sent). Web `ac1 the menus` (`load-program` acts) |
| AC-8 | Pass | Model: `ac8_groups_and_order`, `ac8_slot_numbers_with_two_slots`. Qt6: `ac8_bar_on_widgets` (left to right model, slots with "1:", places, media from a registered factory; the model picker sends the switch). Scenario `ac8_the_bar`; Web `ac8 the bar` |
| AC-9 | Pass | Model: `ac9_button_for_one_picker_for_several`, `ac9_button_attaches_and_detaches`, `ac9_place_needing_more_has_no_control`. Qt6: `ac9_button_attaches_and_detaches_on_widgets`; Web `ac8 the bar` (button attaches, picker attaches, no host drive) |
| AC-10 | Pass | Model: `ac10_no_entry_no_gap`. Qt6: `ac10_no_control_no_gap_on_widgets` (empty groups hidden). Web `ac8 the bar` (Model-B: no slot group) |
| AC-11 | Pass | `ac11_sections_shown_for_keys_present`, `ac11_change_written_at_once` (SettingsPart on LIB-9's test declaration: written through at once, a refused value, a page's integer, a change from elsewhere shown). Qt6: `ac11_settings_dialog_on_widgets` (title, sections, Close only, the file written at once). Scenario `ac11_the_settings_dialog`; Web `ac11 the settings dialog` |
| AC-12 | Pass | `ac12_startup_preset_choices` (C++, JS). Qt6: `ac12_startup_preset_row_on_widgets` ("None", "name — description", "Choose…" sets a file) |
| AC-13 | Pass | `ac13_folder_row_home_change_reset` (C++, JS). Qt6: `ac13_folder_row_on_widgets` (`~`, Change…, Reset enabled only while it differs) |
| AC-14 | Pass | `ac14_set_setting_while_operation_runs`; Qt6: `ac14_settings_while_operation_runs_on_widgets` |
| AC-15 | Pass | Model: `ac15_loading_after_500_ms_at_full_speed` (C++, JS). Qt6: `ac15_loading_after_500_ms_on_widgets` (not at 300 ms, shown at 600 ms, on the faceplate below the display, no button; gone after Reset). Scenario `ac15_loading`; Web `ac15 loading` |
| AC-16 | Pass | Qt6: `ac16_loading_blocks_nothing` (transparent to input, bar, model picker, Reset, Cold boot enabled; a key under the overlay reaches the faceplate; Reset ends the operation, no notice). Web `ac16 the bar works while loading` |
| AC-17 | Pass | Model: `ac17_notices_one_after_the_other`, `refusal_equal_to_last_notice_not_repeated`, `asks_refusal_is_a_question_yes_resends_confirmed`. Qt6: `ac17_notices_one_after_the_other_on_widgets` (no title, window-modal, in order, the session called behind it, a refusal as a notice), `ac17_question_yes_resends_confirmed`. Scenario `ac17_a_notice`; Web `ac17 a notice` |
| AC-18 | Pass | Qt6: `ac18_log_to_platform_no_view` (a log line reaches `qInfo()`; no view of its own) |
| AC-19 | Pass | Model: `ac19_no_machine_menu_without_control_state`, `b7_no_bar_without_control_state`. Qt6: `ac19_roms_missing_no_bar_no_machine_menu` (the text in place of the faceplate, no bar, no Machine menu, About works). Web `ac19 roms missing` |
| AC-20 | Pass | Model: `ac20_help_opens_product_address`. Qt6: `ac20_help_opens_product_address_on_widgets`. Scenario `ac20_help_opens`; Web `ac20 help opens` (through `hostServices.js`'s open-url) |
| AC-21 | Pass | Model: `ac21_about_shows_product_texts`. Qt6: `ac21_about_dialog_on_widgets` (name, version and build, texts, links, one button). Web `ac19 roms missing` (About, its version line) |
| AC-22 | Pass | Both sets show AC-1 to AC-21 on TestShellPart's inputs (the rows above). `LibraryShellNoProduct`: no product include or name in the code of `library/shell`, `library/qt6`, `library/web/components`. Web `ac22 the shell without product code` (the set's part of `calc.js` names no product) |
| AC-23 | Pass | Model: `ac23_paper_panel_always_with_or_without_owner`. Qt6: `ac23_paper_panel_always_on_widgets` (LIB-8's panel in the area from the start, the same panel after a printer change, a product panel into the same area). Web `ac22 the shell without product code` (`paper-panel` in `panel-area`) |
| AC-24 | Pass | `ac24_hit_cases` (every line of `hit_cases.txt`, C++ and JS alike), `ac24_several_pointers_hold_keys`. Qt6: `ac24_mouse_and_touch_on_test_faceplate` (mouse, the distance, two touch points with Qt Test). Scenario `ac24_a_key_under_the_pointer`; Web `ac24 a key under the pointer` |
| AC-25 | Pass | The scenarios of AC-25 under their IDs in both sets: `calcu_qt6_scenarios` (`ac1`, `ac4`, `ac6`, `ac8`, `ac11`, `ac15`, `ac17`, `ac20`, `ac24`) and `shell.spec.js` (the same IDs, `ac6` as "no quit", plus `ac16`, `ac19`, `ac22`, `ac43`). The pictures per layout (AC-33, AC-40) are the products' (M-49, M-57) |
| AC-42 | Pass | Model: `ac42_shell_reads_no_product_entry`. Qt6: `ac8_bar_on_widgets` (`addBarControl` into the group after the control it names); Web `ac8 the bar` (`extra` in the Places group) |
| AC-43 | Pass | `ac43_display_hold_full_speed_until_up_or_leave` (C++, JS). Qt6: `ac43_display_hold_on_widgets` (the `busy` topic at full speed while held, no "Loading…"). Web `ac43 holding the display` |
| AC-44 | Built; the criterion is Calc-U-1600's (M-49) | `attach_host_tests.cpp`: `qt_attach_host_line_runs_on_gui_thread` (a line is answered only when the GUI thread's event loop runs), `qt_attach_host_wait_on_app_clock`, `qt_attach_host_port_zero` (`--attach-port 0`, the bound port), `qt_attach_host_bind_fails_logs_and_runs` (the log line, the app runs on). The object names of §3.10 are in `ObjectNames.hpp` |

### Edge cases

| ID | Result | Evidence |
|---|---|---|
| EC-1 | Pass | `ec1_short_operation_shows_nothing` (C++, JS) |
| EC-2 | Pass | `ec2_notice_over_loading` (C++, JS); Qt6 `ec2_notice_over_loading_on_widgets` (OK closes the notice, "Loading…" and the operation go on) |
| EC-3 | Pass | `ec3_missing_startup_preset_still_shown` (C++, JS); Qt6 `ac12_startup_preset_row_on_widgets` (a file set from a folder shown as set). The start's fallback is LIB-18 EC-1's |
| EC-4 | Pass | `ec4_model_switch_changes_entries` (C++, JS); Qt6 `ec4_model_switch_changes_menus_in_place` (the same `QMenu`, filled anew); Web `ac1 the menus` |
| EC-5 | Pass | `ec5_printer_change_moves_paper_owner` (C++, JS); Qt6 `ac23_paper_panel_always_on_widgets` |
| EC-6 | Pass | Qt6 `ec6_help_failure_changes_nothing` (the address not opened: no notice, nothing sent, nothing running) |
| EC-7 | Pass | `ec7_back_to_full_speed_counts_again` (C++, JS) |
| EC-8 | Pass | `ec8_no_key_at_down_nothing_on_move` (C++, JS); Qt6 `ac24_mouse_and_touch_on_test_faceplate`; Web `ac24 a key under the pointer` |

Every AC of the Library column (AC-1 to AC-25, AC-42, AC-43) and EC-1 to
EC-8 pass: the column is Done in `features/INDEX.md`. LIB-7's AC-29 (the
File menu's Import Media… and Saved Media…, LIB-7 D-7) is drawn by the
menu model here (`media_items_where_saved_media_and_kinds`, C++ and JS) and
by Web `ac29 file menu has import and saved media` on a scripted `media`
topic; LIB-7's own `media.spec.js` case `ac29` comes with M-41's branch.

### Deviations and details decided here

- **`useCalcuShell`** for the design's `useShell(shell, product, traits)`:
  `useShell` is LIB-15's hook that creates the shell (`src/embed.js`), in
  the same closure; the set's prefix (D-17) avoids the clash.
- **Files.** The panel model is in `BarModel.*` (no `PanelModel.*`); the
  Value reading and the command names are in `Inputs.*`; `TestShellPart`
  is header only, so `calcu_qt6_tests` and the Web test product use it
  without a source of their own. `ShellModal.jsx` holds `ShellDialogs`.
- **`ShellProduct`**: `copyScreen` is the product's function (D-12 makes
  Copy Screen a product item); `about()` gives the About texts, and the
  dialog's title drops a trailing "…" of the product's label. The JS form
  has `presetAccept`, `settingsSpec`, `keysShown`.
- **M10** holds also for the product's items and Edit: without a control
  state only Settings…, Quit, Help and About remain (a product item would
  act on no machine).
- **Resume** is always in the Machine menu, disabled with "The machine is
  not paused." while `run-state` does not say paused (no `run-state` yet:
  disabled).
- **Notices (D-7):** a refusal "Stopped." (an operation the user ended
  with Reset, Cold boot or a model switch) is not shown; `refusal()` takes
  the command it answers, for N4's resend.
- **Settings:** `SettingsPartSetup` holds pointers; `[folders]` rows have
  the kind `folder` (value: the fixed folder; in use: the start folder);
  the extra command `remember-folder {kind, folder}` keeps LIB-9 AC-29's
  "folder last used" for the shell's Load Preset… and Choose…; the
  family label of a `[folders]` key is "<Kind> folder" (the product may
  give its own section instead). The notice of D-19 reads "The setting
  holds until you quit; it was not saved: <LIB-9's problem>.". The Web
  dialog has no "Change…" (a page has no folder picker, LIB-15); the Web
  frontends show no folder rows (AC-30, AC-37).
- **Qt6 shell API** beyond §3.8: `setChoosers()` (file and folder dialogs,
  the browser, the clipboard; the tests' seam), `sendChecked()`,
  `choose()`, `open…()`, `inputs()`, `notices()`; `ShellOptions` carries
  the traits and the input names. The shell rebuilds menus and bar one
  event-loop turn after a topic changes, so no widget is deleted inside
  its own signal; a menu whose model is unchanged is not touched (EC-4).
  `addBarControl(group, "", w)` puts `w` at the group's end. `PanelArea`
  makes LIB-8's paper panel at once, so it takes the paper topics from the
  listen on. Without LIB-7's module picker (M-41, a parallel branch) a
  slot shows a plain picker of what fits; `registerSlotPicker()` and
  `registerMediaControls()` take LIB-7's and C59-7's.
- **`SessionHost`** hands the messages of a host call on after the call
  returns, emits `topicChanged` per state or stream message, drains the
  mirror's streams after its handlers (the panels keep what they need),
  and answers clipboard (LIB-8's `ClipboardService`) and open-url itself
  unless the frontend sets a handler. `attachPortFrom()` reads
  `--attach-port 0` (LIB-11's `attachPort()` treats 0 as no port and is
  left unchanged).
- **Web Load Preset…** picks a file (`calcuPickFiles`) and sends
  `load-preset {content, name}` (LIB-6), since a page has no path.
- **React `extra`**: a product's controls go at the end of the group they
  name (no "after" on the Web).
- **LIB-1 §2a**: the area `shell` and its includes; `qt6` may include
  `harness` for attach only (design §3.9 asks for `HarnessProduct` in
  `SessionHost::openAttach`).
- **Qt Test** (`find_package(Qt6 COMPONENTS Test)`, optional) drives the
  shortcut and touch cases; a Qt without it skips them.
- **`qt_attach_host_wait_on_app_clock`** checks at least 0.1 s of wall
  time for `wait 200ms`: a late 60 Hz timer lets the pacer catch up, so
  the wall time can be shorter than the emulated time; the harness never
  advances the machine itself.

No user-visible change beyond the ledger's rows (59-5, 59-9, 59-14, 59-44,
1600-9, 1600-10): the shell is not in a product yet.

### CI

`calcu_qt6` is compiled on every Qt platform of both product workflows
(design §8): this step changes platform code, so it needs one CI dispatch
at its merge (not dispatched from the branch).

## Calc-U-1600 (M-49, 2026-11-16, branch `m49-shell-1600`)

Both frontends of Calc-U-1600 are put together from the shell. Qt6:
`MainWindow` is a `calcu::qt6::Shell` over the `SessionHost` that
`MachineController` makes (with LIB-14's `SettingsPart` in the session);
`ControlBar`, `NamedComboBox`, `SettingsDialog`, `AboutDialog`,
`Operations`, `EmulationPacer` are deleted; the product's data is
`app/SharpShellProduct` (one source with the Web, D-32); the bar's media
controls are `frontends/qt6/app/MediaControls` (LIB-7's `MediaPicker`).
Web: `src/CalcU1600.jsx` on `useCalcuShell` and the React set's
components; `engine.js`, `MenuBar.jsx`, `ControlBar.jsx` are deleted;
`SharpWebPart` keeps the product's requests (`c1600.shell-product`,
`c1600.faceplate-map`, …). The screenshot runner and the pacing probe are
attach clients; the attached harness has the serve form's commands and the
bare `wait` (D-27, a library change).

Tests: `CalcU1600QtTests` (24 cases, `frontends/qt6/tests/`, the real
window over the real ROMs on the real clock, offscreen; Longer, 25 s),
`CalcU1600QtTests.romsMissing` and `CalcU1600ShotsThroughAttach`
(Frequent), `CoreTests` (`app/tests/shell_product_tests.cpp`), the Web's
`shell.spec.js` (Chromium and WebKit, in `CalcU1600WebPlaywright`), the
library's `attach_registry_matches_serve`, `attach_bare_wait_ends_when_idle`,
`qt_attach_host_preset_key_type`, and the screenshot scenarios through
attach.

| AC | Result | Evidence |
|---|---|---|
| AC-34 | Pass | `lib14_ac34_qt_menus_in_family_order` (File: Load Preset…, Examples ▸, Load BASIC Program…, Load Machine Code…, Mount Directory…, Unmount, Settings…, Quit; Edit: Copy Screen, Paste Text; Machine: Model ▸, ROM Revision ▸ on the PC-1500, ROM Version ▸ on the PC-1600, CE-1600P ROM ▸ while it is plugged in, Peripherals ▸, Reset, Cold boot, Resume; Help: Calc-U-1600 Help, About Calc-U-1600…; the platform's roles), `lib14_ac34_copy_log_selection_while_inspector_text_selected`, `lib14_ac34_shortcuts_ctrl_o_r_shift_r`; `lib14_ac34_shell_product_complete` (`checkShellProduct` against each model's control state). The macOS application menu: the shot `02-machine-menu` (Longer, needs Screen Recording) |
| AC-35 | Pass | `lib14_ac35 the web menus` (no Mount Directory…, Unmount, Quit; Import Media…, Saved Media…; Load Preset… and Examples ▸ as two items), `lib14_ac35 no shortcut is bound` |
| AC-36 | Pass | `lib14_ac36_bar_pc1500_pc1500a` (the model picker, `slot-1` with Name & Save, the buttons CE-150 and CE-158, the tape controls with the CE-150), `lib14_ac36_bar_pc1600` ("1:" and "2:", the printer picker None/CE-150/CE-1600P, the CE-158 button, the disk controls disabled with "The disk drive is in the CE-1600P." until the CE-1600P is plugged in, the tape controls with it); Web `lib14_ac36 the bar per model` |
| AC-37 | Pass | `lib14_ac37_settings_sections_and_rows` (General: Startup model, by the model's name; Startup presets: three rows; Storage: Save folder, Tapes; Folders: Examples, Basic, Assembly, Host drive folder; Tracing; Serial ports with the link folder and the two status lines, not on Windows; Debugger with the server and its port, its status and Disconnect beside); Web `lib14_ac37 the web settings` (General, Startup presets, the sentence on saved media) |
| AC-38 | Pass | `lib14_ac38_loading_after_500_ms_blocks_nothing` (a preset at full speed: nothing at 300 ms, "Loading…" by 800 ms; the model picker, Reset and Cold boot enabled, the variants not; Reset ends it), `lib14_ac38_notice_without_title` |
| AC-39 | Pass | `lib14_ac39_help_opens_calc_u_org_1600`, `lib14_ac39_about_as_today`, `lib14_ac39_help_address_and_about_texts` (CoreTests); Web `lib14_ac39 help and about` |
| AC-40 | Pass | `lib14_ac40_layout_faceplate_bar_panel_row` (the faceplate, the bar, the row: INSPECTOR, paper panel always, the CE-158's panel only with a CE-158); Web `lib14_ac40 panels only with room` (two viewport sizes). Pictures per layout: the shots `01-app-overview`, `02-control-bar-pc1600` |
| AC-41 | Pass | `lib14_ac41_copy_screen_picture_and_text` (the picture with its physical size and `LcdText::plainText`, through the window's clipboard sink: the tests never touch the real clipboard); Web `lib14_ac41 copy screen` |
| AC-44 | Pass | `lib14_ac44_scenario_through_attach` (two shots: `preset`, `key`, `type`, `wait-idle`, `expect-lcd`, `cold-boot` and two captures through the window's own attach endpoint); `CalcU1600ShotsThroughAttach`; the scenarios through attach, offscreen: `tests/gui/` all seven pass (`lib5-ac33`, `lib6-ac54`, `lib7-ac16-…`, `lib7-ac8-…`, `lib8-ac36`, `lib8-ac38`, `lib10-ac41`), guide `04` to `09` pass (19 images); `01` to `03` need the Screen Recording and Automation grants for their `method: system` shots and native menus, which this environment lacks (not a code failure) |
| AC-19 (this product) | Pass | `lib14_ac19_roms_missing_in_window` (CTest `CalcU1600QtTests.romsMissing`: the text naming the file in place of the faceplate, no bar, menus File and Help, About and Help work, the window stays; ledger 1600-36); Web `rom-missing.spec.js` (the text in place of the calculator, no "stopped" title) |
| AC-24 (this product) | Pass | `lib14_ac24_key_under_pointer_pc1500_pc1600` (a key's centre holds it in `held-keys` and lets go; a point in the gap below ENTER within 24 px finds ENTER; ledger 1600-37); Web `lib14_ac24 a key under the pointer` |
| AC-43 (this product) | Pass | `lib14_ac43_display_hold_full_speed` (the pointer on the LCD: `busy`'s speed full, regular after) |
| C1600-2 AC-15 | Pass | `c1600_2_ac15_file_items_and_filters` (the two items in FileKinds, their filters, the start folder remembered by kind); Web `c1600_2_ac15 the file items` |

Golden runs: no core change but the catalogue's pin-outs; the replays
pass unchanged. Frequent (`build/tests`): 35/35, `CoreTests` 59 s of the
60 s budget (58 s before; the new cases cost about 1.5 s). The Qt tree's
Frequent: 41/41.

### Deviations from the design, chosen by the simpler structure

- **`CalcU1600QtTests` is Longer, not Frequent** (D.8 says about 5 s). It
  runs the real window on the real clock (presets load at full speed, a
  window starts per case): 25 s, which the Frequent budget has no room
  for. Its ROMs-missing case (0.8 s) and the attach check are Frequent.
- **The real clock instead of a scripted wall clock** in
  `CalcU1600QtTests`: `SessionHost` is made inside `MachineController`
  with the steady clock; the tests wait for conditions with a bound.
- **`placesWithoutControl` holds `bus-rom` too** (D.5 lists `host-drive`):
  with one pin-out per place `bus-rom` lists the bus ROM alone and would
  get a button, but a bus ROM is plugged in by a preset only.
- **CE-1600P ROM ▸ only while it is plugged in** (AC-34 lists it on the
  PC-1600): the shell's rule M5 offers a peripheral's variants while it is
  plugged in; ledger 1600-45 (LIB-4 Part D deviations).
- **The disk controls follow the tape controls** in the Media group: a
  product control goes after the shell's control it names, or at the end
  (B2); ledger 1600-45. `ShellControlBar` now leaves a product control's
  visibility to its product (the disk controls hide on the PC-1500), a
  library change.
- **The serial status lines and the debug server's status** are added to
  the shell's dialog by the window while it is open (`MainWindow::
  refreshSerialStatus`), since a dialog row of the shell is a setting; the
  Serial ports group is named `settings-serial` for the scenarios.
- **The product's acts** (Load BASIC Program…, Load Machine Code…, Mount
  Directory…, Unmount) use the shell's choosers and `startFolder` /
  `rememberFolder` for the `[folders]` kinds, so the tests replace the
  dialogs as the shell's own do. Unmount reads "Unmount "<folder>"" from
  the product entry `c1600.host-drive-folder`.
- **The indicator layout as data** (D-31): `app/LcdLayout`'s
  `indicatorSlots()` is the table both the Qt6 painter and the Web's
  `c1600.faceplate-map` use; `DisplayFrame` is gone, the LCD paints the
  `display` topic's output.
- **Attach in development builds**: the Qt6 app opens attach on
  `attachPortFrom(arguments)` (59059 by default) in every `CALCU_ATTACH`
  build, as C59-5's Swift app does; `--shots` and `--pacing-probe` open it
  on port 0 for their client in the same process.
- **The bare `wait` through attach** is the session's `wait-idle`
  operation, replied with the time it took and `hour reached`, as the
  serve form replies.
- **The attached registry test** compares the commands by name; the
  preset steps that come from a session's vocabulary are left out of the
  comparison, since the library's attach test session has no LIB-6 part.
  `qt_attach_host_preset_key_type` checks `key` and `type` through the Qt6
  host and that `preset` and `cold-boot` are forwarded (that session has
  no LIB-6 or LIB-18 part); the full `preset` through attach is
  `lib14_ac44_scenario_through_attach`.
- **Web** (agent report, branch `m49-web-1600`): an optional
  `product.loadPreset` in `useCalcuShell` (the page sends picked files
  through `open`, LIB-13 AC-8); `CalcuMediaPicker` shows a held medium no
  entry lists (a tape before its first write); LIB-7's `KindDecl::
  companionSuffixes` so Import Media… takes a tape's WAV with its
  definition (library `ac30_file_picker_takes_a_companion_along`); the page
  draws the paper panel itself in the INSPECTOR / paper / CE-158 order with
  the room rules; notices of the page's own flows in the shell's notice;
  `lib14_ac24` by a mouse click 3 px below a key.
- **The screenshot run times** grow: `run:` is wall time, `04-basic`
  alone takes about 90 s.

### CI

The Qt6 frontend's build changes (new target `CalcU1600QtTests`, files
gone, `calcu1600_harness` linked into the app with `CALCU_ATTACH`), and
library code of `calcu_qt6` and `harness/attach` changes: one CI dispatch
at the merge.

Column status: Done.

## Calc-U-59 (M-57, 2026-11-17, branch `m57-shell-59`)

Design §10. Both frontends of Calc-U-59 are put together from the shell;
the Swift frontend is C59-8 (M-58).

Qt6: `MainWindow` is a `calcu::qt6::Shell` over the `SessionHost` that
`EmulatorController` makes (the session with LIB-14's `SettingsPart` in
front); `ControlBar`, `SettingsDialog`, `AboutDialog`, `PaperPane` and
`buildMenus` are deleted; C59-7's card buttons are `CardControls`, the bar's
media controls of `card-reader`; the slot picker is LIB-7's `MediaPicker`;
the faceplate's pointer is the shell's (`FaceplateGeometry`), the host keys
go through `calcu::qt6::HostKeys`; harness attach and `--pacing-probe` as
its client (`CALCU_ATTACH`). Web (branch `m57-web`, merged): the page on
the React set's shell (`src/page.jsx`, `PlayCalculator.jsx`), the More
button in the portrait layout, `engine.js` and `ControlBar.jsx` deleted,
`TIWebPart` without `c59.web.model`, `c59.web.settings`, the settings and
pointer commands and `c59.static-data`, with `c59.shell-product` and
`c59.faceplate-map`. The product's data is `app/TIShellProduct` (one source,
D-32). Without the full ROM set the app session runs without a machine and
publishes `roms-missing` (`TIAppSetup::romsMissing`).

Library additions (§10.3; Calc-U-1600 keeps its forms byte for byte): the
More model (`Traits::menuBar`, `settings`, `about`; `moreItems`;
`MenuItem::confirm`, `destructive`; `ProductItem::onBar`, `mediaPlace`;
`BarGroup::Start`, the bar's `Action` and `More`), labelled choices
(`DialogRow::choices`), the value forms (`copyScreen` as the product has it,
`aboutLinks`, `onBar`, `mediaPlace`, `choices`), `Shell::FaceplateGeometry`
(origin, a press the product takes), the About icon, the storage banner of
the React set (`ShellStorageBanner`, used by both products: ledger 1600-48
and 59-55 in effect), the More menu of the React set (`ShellMoreMenu`), and
`calcu::qt6::AttachLineClient` (moved from Calc-U-1600's screenshot runner
into the Qt6 set, for both products' attach clients).

Checks run: `build/tests` Frequent 46/46 (`calcu_library_tests` 958,
`CalcU59Tests` 546, the CTest `CalcU59FrontendsDrawControlState` new);
`build/qt` Frequent 51/51 (`calcu_qt6_tests` 57, `calcu_qt6_scenarios`,
`CalcU59QtTests` 25 cases in 22 s); `CalcU1600QtTests` (Longer) 25 cases
pass; `CalcU59QtPacingProbe` (Checkpoint, through attach) passes: wall
59.513 s, emulated 59.521 s, difference +0.008 s within 0.037 s;
Playwright: Calc-U-59 149 passed, 5 skipped; Calc-U-1600 112 passed,
2 skipped; the library's 140 passed, 2 skipped; `shell_model.test.js` 48.

| AC | Result | Evidence |
|---|---|---|
| AC-26 | Pass | `CalcU59QtTests` `lib14_ac26_qt_menus_in_family_order` (File: Load Preset…, Examples ▸, Arm Card…, Arm Fresh Card…, Settings…, Quit, the card items on the TI-59 only; no Edit; Machine: Model ▸, Peripherals ▸ with the PC-100C, Reset, Cold boot, Resume; Help: Calc-U-59 Help, About Calc-U-59), `lib14_ac26_shortcuts_ctrl_o_r_shift_r` (and Load Preset…'s filter), `lib14_ac26_roles_settings_about_quit`; `CalcU59Tests` `lib14_ac26_shell_product_complete` (`checkShellProduct` against each model's control state; no Edit menu; the card items by the media place) |
| AC-27 | Pass | `lib14_ac27_bar_per_model` (the model picker; `slot-1`'s picker with the 14 modules and the empty slot; "PC-100C" pressed while plugged in; the card controls on the TI-59, none on the TI-58); Web `lib14_ac27 the bar per model` |
| AC-28 | Pass | `lib14_ac28_no_reset_preset_settings_on_the_bar`, `lib14_ac28_cold_boot_is_the_menu_item` (Cold boot runs from the Machine menu; no button changes); Web `lib14_ac28 the wide layout has the menus and no reset preset or settings on the bar` |
| AC-29 | Pass (Web) | `lib14_ac29 portrait bar keeps reset preset settings and more`, `lib14_ac29 more holds examples cold boot resume and help` (and Import Media…, Saved Media…, About, D-36; the popup inside the client at 280 px), `lib14_ac29 cold boot asks first`, `lib14_ac29 no long press for clean`, `lib14_ac29 settings prop off`; the model rules M13 to M18, B8 in `calcu_library_tests` and `shell_model.test.js` (L12) |
| AC-30 | Pass | `lib14_ac30_settings_sections_and_rows` (General with Startup model, "Last used" first, and the version; Startup presets TI-59, TI-58, TI-58C; Storage: Save folder; Folders: Preset folder; Tracing; Display: Segment Style "Modernized"/"Classic", a change written at once and drawn; no Solid State Module row); `CalcU59Tests` `lib14_ac30_settings_spec_per_frontend_and_layout`; Web `lib14_ac30 the web settings per layout` (Keyboard Feedback "Off"/"Click Sound" in portrait only, Maximum file size in wide only) |
| AC-31 | Pass | `lib14_ac31_loading_after_500_ms_blocks_nothing` (nothing at 300 ms, "Loading…" by 1.5 s; the model picker, Reset and Cold boot usable; Reset ends it), `lib14_ac31_notice_without_title`; Web `lib14_ac31 loading and the notice dialog` |
| AC-32 | Pass | `lib14_ac32_help_opens_calc_u_org_59`, `lib14_ac32_about_icon_name_version_summary_links` (the window icon, the name, "Version …", the two lines, GitHub, Support, Privacy, "Built with Qt (LGPL-3.0)."); `CalcU59Tests` `lib14_ac32_help_address_about_texts_and_links`; Web `lib14_ac32 help and about in the help menu and not in the settings` |
| AC-33 | Pass | `lib14_ac33_three_columns_with_dividers` (calculator, the panel area with the paper panel, the debug panel; the dividers move; the window's picture written to `build/qt/products/calc-u-59/frontends/qt6/test-pictures/`); Web `lib14_ac33 the layouts portrait wide and auto` (a screenshot each) |
| AC-19 (this product) | Pass | `lib14_ac19_roms_missing_in_window` (the text naming TMC0583.txt in place of the faceplate, no bar, menus File and Help, About and Help work, the window stays; row 59-54); `CalcU59Tests` `lib14_ac19_roms_missing_topic_without_a_machine`; Web `rom-missing.spec.js` `lib14_ac19 roms missing in the window, help and about work (wide)`, `(portrait)` |
| AC-24 (this product) | Pass | `lib14_ac24_key_under_pointer_ti59_ti58` (a key's centre held in `held-keys`; a point in a widened area is that key; a point in a gap is no key, distance 0), `lib14_ac24_press_on_the_fitted_card_feeds_it` (with a card on show a top-row key is still a key; the card's corner feeds side 0); `CalcU59Tests` `lib14_ac24_faceplate_hit_map` (the 45 centres); Web `lib14_ac24 a key under the pointer` |
| AC-43 (this product) | Pass | `lib14_ac43_display_hold_full_speed` |
| AC-1, AC-4, AC-6, AC-8, AC-11, AC-15, AC-17, AC-20 (this product) | Pass | in the cases of AC-26 to AC-32 above |
| L12 to L15 (library) | Pass | `more_items_are_the_items_without_a_control`, `more_cold_boot_asks_first`, `more_follows_availability`, `media_place_item_only_with_its_media_place`, `no_settings_item_without_settings`, `bar_without_menu_bar_keeps_reset_preset_settings_and_more`, `choice_row_with_labels_offers_only_those`, `choice_value_not_listed_shown_as_written`, `shell_product_form_has_links_and_copy_screen_flag` (C++ and `shell_model.test.js`); Calc-U-1600 `lib14_calc_u_1600_forms_unchanged`; `calcu_qt6_tests` `faceplate_origin_moves_the_hit_map`, `faceplate_press_the_product_takes_passes_through`, `about_shows_the_window_icon`; library `shell.spec.js` `faceplate origin and a press the product takes`, `storage banner shows the library's sentence while not kept`; Calc-U-1600 `storage.spec.js` on `storage-banner` |
| Q12 (attach) | Pass | `qt_attach_drives_calc_u_59` (`model TI-58`, `key 7`, `wait`, `lcd` through the window's endpoint; the window follows); `CalcU59QtPacingProbe` through attach |

Adapters removed (§10.8): `--pacing-probe` outside attach; W59-1
(`c59.static-data`; the heatmap's grid is the product request
`c59.heatmap-grid`); `src/engine.js`; `c59.web.model`; the settings
commands and `c59.web.settings` of `TIWebPart`; `c59.web.pointer-*`; A-4,
the frontends' own "ROMs missing" (Qt6's message box and quit, the Web's
failed-start line); `cu59::PointerTracker` in Qt6 and the Web (it stays
for the Swift bridge until M-58). The `c59.*` commands of LIB-4 §10.3
(`c59.start-machine`, …) are no longer sent by Qt6 or the Web; they go
with the Swift frontend's last use in M-58.

Golden runs: no core change; the replays pass unchanged.

### Deviations from the design, chosen by the simpler structure

- **`CalcU59QtTests` on the real clock**, as `CalcU1600QtTests` (Part D
  deviation): the host is made inside `EmulatorController`; the cases wait
  for conditions with a bound. It stays Frequent (22 s in the Qt tree).
- **The PC-100C's controls on the panel area's own paper panel**: the
  window sets PRINT, ADV and TRACE on the default paper panel while the
  PC-100C owns it (`MainWindow::followPrinter`, TRACE's position from
  `c59.printer`), rather than registering a panel of its own.
- **`AttachLineClient` moved into the Qt6 component set** so that
  Calc-U-59's pacing probe does not copy Calc-U-1600's; in trees with
  `CALCU_ATTACH` `calcu_qt6` links Qt Network (both products' Qt projects
  find it there).
- **Two test hooks in the Qt6 frontend**: `QtResourceBundle::hideForTests`
  (the ROMs-missing case; the ROMs are compiled in) and
  `AppPaths::useBundledExamplesDir` (the examples beside a test program).
- **A new save or trace folder is used from the next start**, as today:
  `SettingsPartSetup::changed` has nothing to apply (C59-3 reads the
  constant memory file at the TI-58C's build).
- **Web: Load Preset… sends LIB-13's `open {way: dialog, upload}`**, with
  no extension filter, as the page's preset upload did and as Calc-U-1600's
  page does, rather than the shell's `load-preset {content}`.
- **Web: the assistive key buttons** send `screen-key-down`/`up`; the
  commands `c59.web.key-button-*` stay in `TIWebPart` (not in §10.8's list;
  its tests use them).
- **Web: the card dialog** no longer has Import… and Saved Media… (they are
  in File and behind More, LIB-7 D-29).
- **Web, a second tab**: the banner stays (row 59-55) and the library's
  storage notice shows as a dialog as well.
- **The test preset of AC-31** runs a program loop (Lbl A, GTO A) while it
  waits: a bare `wait:` at full speed passes 300 s of emulated time in
  under 300 ms.

- **Web portrait without ROMs shows the menu bar** (by AC-19: Help and
  About still work): the portrait layout has no menu bar while the product
  runs, and B7 hides the bar with its More button when there is no
  machine, so the page draws the menu bar (File, Help) in that case only.
  The same holds for iOS in C59-8 (no About there, C59-8 AC-11).
- **A drag keeps the key first pressed** (Qt6 and Web): ledger row 59-44,
  in effect from M-57.

### CI

The Qt6 frontend's build changes (files gone and new, `HarnessProduct`
and Qt Network with `CALCU_ATTACH`), the library's `calcu_qt6` links Qt
Network in attach trees, and Calc-U-1600's screenshot sources moved: one
local Linux CI run (`qt-59`, `qt-1600`, `web-59`, `web-1600`) at the merge,
GitHub CI at the end of Part E.

Column status: Done (AC-26 to AC-33 pass; the Swift frontend's family
structure is C59-8).
