# LIB-8: QA report

Results per acceptance criterion and edge case of `spec.md`, per column.

## Library (M-39, 2026-11-15, branch `m39-paper`)

Built and run on an M4 Pro, macOS 27.2, Apple clang, CMake, Ninja,
RelWithDebInfo, in two trees: `build/tests` (`-DCALCU_FRONTEND_QT=OFF`)
and `build/qt` (`-DCALCU_FRONTEND_QT=ON`, Homebrew Qt 6). The Web parts with
Homebrew Emscripten in `build/web` and Playwright (Chromium, WebKit).

The code:
- `library/paper/` (`calcu::paper`, in `calcu_library`): `PaperOutput.*`
  (the kind, `PaperSink`, `PaperOutputBase`), `Paper.*`, `Picture.*`
  (compiled with `-ffp-contract=off`), `Replies.*`, `PaperTopics.*`,
  `PaperPart.*` (the part, `addPaperStep`, `checkPaperOutput`);
- `library/formats/Png.*` (`PngWriter`, RGB, band by band, `pHYs`);
- `library/harness/PaperCommands.cpp` (`cut-paper`, `paper text|marks|png|copy|cut`),
  `Commands.hpp`, `Product.hpp` (`HarnessProduct::paperPart()`), `Run.cpp`;
- `library/sessions/AppSession.cpp`: `advanceUnpaced` runs the parts'
  `afterCall` (A-6; a commit of its own, and one for its test's warning);
- the Qt6 component set `library/qt6/` (target `calcu_qt6`): `PaperPanel.*`,
  `PaperPainter.*`, `ClipboardService.*`, `CMakeLists.txt`; tests
  `library/qt6/tests/` (`QtTestMain.cpp`, `paper_panel_tests.cpp`), program
  `calcu_qt6_tests`;
- the React component set `library/web/components/` (`paper.js`,
  `PaperPanel.jsx`, `paper.css`), `COMPONENT_SOURCES` and `COMPONENT_CSS` in
  `library/web/build/dist.js`; `src/hostServices.js` (`armClipboard`, the
  `clipboard-image` form `{bytes, text, name, dotsPerInch}` and the fallback
  of AC-21; a bytes-only ask is answered as before); `src/shell.js`
  (`listenStream`, so the panel keeps a paper longer than the shell's 500
  kept stream items);
- the test printers TD-20 (`test-dotter`) and TP-4 (`test-plotter`) in
  `library/machine/tests/TestMachines.*`, in `paperTestCatalogue()`; the
  harness's test product (`library/harness/tests/TestProduct.*`) on that
  catalogue with a PaperPart; the Web test product
  (`library/web/tests/product/`) with PeripheralsPart, a PaperPart and the
  commands `tp.attach`, `tp.detach`, `tp.press`, the topic `tp.printer`;
- the converters of AC-39 and their reply files in the products (§4.1).

Results of the runs:
- `calcu_library_tests`: 629 `TEST`s (54 new: 43 in `library/paper/tests/`,
  9 harness, 1 formats, 1 sessions), 4,535 checks passed, 0 failed.
- `calcu_qt6_tests` (`build/qt`, offscreen): 10 `TEST`s, 85 checks, 0 failed, 2.1 s.
- `paper.spec.js`: 18 of 18 (9 cases, Chromium and WebKit); the whole
  `LibraryWebPlaywright` run: 92 passed, 2 skipped (as before);
  `LibraryWebNodeTests` 4 of 4.
- `CalcU59Tests` 283 (`lib8_ac39_ti_printouts_turned_into_paper_replies`),
  `CoreTests` 876 (`lib8_ac39_sharp_plots_turned_into_paper_replies`): 0 failed.
- Frequent: `build/tests` 30 of 30 (77 s), `build/qt` 31 of 31 (with `calcu_qt6_tests`).

| ID | Result | Test case (file in `library/paper/tests/` unless named) |
|---|---|---|
| AC-1 | Pass | `ac1_description_states_unit_width_form_drawing_resolution_text_layer` (both test printers; one capability per printer, owned by it, `<place>/paper-output`), `ac1_conformance_check_finds_a_different_description`; `test_printers_build_and_offer_paper_output` (`paper_kind_tests.cpp`) |
| AC-2 | Pass | `ac2_marks_in_order_never_changed` (a stroke's earlier positions stay while it grows; a move with the pen up gives no mark) |
| AC-3 | Pass | `ac3_each_line_and_each_feed_in_the_text_layer` (TD-20 lines and a feed's empty line; Unicode text as given) |
| AC-4 | Pass | `ac4_reach_from_least_to_greatest_mark_or_feed` (a feed without a mark, a feed back below zero; TP-4's `back`) |
| AC-5 | Pass | `ac5_cut_empties_and_next_print_starts_a_new_paper` |
| AC-6 | Pass | `ac6_one_stream_per_printer_type_plugged_or_not`, `ac6_message_carries_appended_marks_lines_and_reach`, `ac6_cut_sends_the_reset` (`paper_part_tests.cpp`); `ec3_late_listener_gets_whole_paper_once_then_appended` |
| AC-7 | Pass | `ac7_same_commands_same_ticks_same_marks` (two runs, `paper marks` and `paper text` equal) |
| AC-8 | Pass | `ac8_reset_cold_boot_power_cycle_keep_the_paper`, `ac8_detach_keeps_attach_again_prints_on` (test-seg, again on test-dot, in variant `r2`), `ac8_machine_replaced_without_printer_keeps_paper`, `ac8_user_preset_naming_printer_starts_empty_other_paper_stays`, `ac8_startup_preset_at_model_switch_keeps_paper`, `ac8_new_session_starts_with_empty_papers`, `ac8_attach_in_place_keeps_marks_made_before_the_part_binds` |
| AC-9 | Pass | `ac9_detach_and_attach_send_nothing_for_papers` (detach, attach, two model switches) |
| AC-12 | Pass | Qt6: `ac12_owner_name_whole_width_marks_at_their_position` (pixels at dot and stroke positions), `ac12_no_owner_empty_paper_no_name_copy_cut_disabled` (`library/qt6/tests/paper_panel_tests.cpp`); React: `ac12 owner name and marks at their position`, `ac12 no owner no name copy and cut disabled` (`library/web/tests/paper.spec.js`) |
| AC-13 | Pass | Qt6: `ac13_follows_within_64_points_stays_when_scrolled_back` (wheel events), `ac13_no_scroll_bar`; React: `ac13 follows near the end stays when scrolled back` |
| AC-14 | Pass | Qt6: `ac14_view_control_shows_view_on_text_selectable_fixed_width`; React: `ac14 view control shows the view that is on` |
| AC-15 | Pass | Qt6: `ac15_key_down_while_held_switch_shows_position`; React: `ac15 printer controls held and switched` (`set-control` sent) |
| AC-16 | Pass | `ac16_copy_asks_clipboard_with_picture_and_text` (TD-20: picture, text joined by line ends, name, resolution; TP-4: picture only; `copy_cut_tests.cpp`); `ac16_full_width_all_it_reaches_at_stated_resolution`, `ac16_reference_pictures_of_the_test_printers`, `ac16_reference_pictures_of_the_three_printers` (`picture_tests.cpp`; 1311 pixels for the PC-100C, 9921 for a short CE-1600P paper, byte for byte against `fixtures/*.png`); `png_rgb_bands_and_phys` (`formats/tests/png_tests.cpp`); Qt6 `ac16_copy_puts_image_and_text_on_clipboard` (`ClipboardService`: image size, dots per metre, text; the offscreen clipboard works on macOS) |
| AC-17 | Pass | `ac17_long_paper_lowered_to_16000_pixels_never_cropped`, `ac17_wide_long_paper_lowered_to_64_megapixels` |
| AC-18 | Pass | `ac18_cut_copies_then_cuts_also_when_copy_failed` (answer not given; nobody offers the clipboard); Qt6 `ac18_cut_empties_and_disables` |
| AC-19 | Pass | `ac19_empty_paper_copy_and_cut_refused`; Qt6 `ac19_empty_paper_copy_cut_disabled`; React `ac19 empty paper copy and cut disabled` |
| AC-20 | Pass | `ac20_copy_outcome_on_paper_copies` (copied, text-only, failed); Qt6 `ac20_toast_for_1_8_seconds` (`Copied to clipboard`; `Copy failed` with a refusing service); React `ac20 copied to clipboard for 1.8 seconds` |
| AC-21 | Pass | React: `ac21 picture refused text alone copied` (TD-20), `ac21 picture refused offered as download` (TP-4, `TP-4.png`), Chromium and WebKit |
| AC-22 | Pass | `ac22_cut_paper_cuts_the_plugged_printers_paper_only`, `ac22_without_printer_no_effect_no_failure`, `ac22_while_paused_finishes_at_once`, `ac22_cut_paper_in_the_harness` (`paper_step_tests.cpp`) |
| AC-23 | Pass | `ac23_cut_paper_plugged_or_by_name`, `ac23_paper_text_refused_without_text_layer`, `ac23_paper_marks_reply`, `ac23_paper_png_writes_the_picture`, `ac23_paper_copy_and_cut_refused_for_empty_paper`, `ac23_state_reads_what_was_appended` (`harness/tests/paper_commands_tests.cpp`); `ac23_numbers_whole_without_point_else_four_decimals`, `ac23_marks_reply_reach_then_dots_and_strokes`, `ac23_text_reply_one_line_per_text_line` (`replies_tests.cpp`) |
| AC-39 | Pass (library part, as design §1 says) | `ac39_marks_reply_read_and_written_unchanged` (the three fixtures read and written unchanged: the reply form is fixed); `lib8_ac39_ti_printouts_turned_into_paper_replies` (`products/calc-u-59/tests/paper_replies_tests.cpp`: 32 printing examples, 32 `.paper-text`, 29 `.paper-marks`), `lib8_ac39_sharp_plots_turned_into_paper_replies` (`products/calc-u-1600/tests/paper_replies_tests.cpp`: 14 `.paper` files, 14 `.paper-marks`). The comparison of new runs is C59-2 AC-11 (M-54) and C1600-4 AC-16 (M-46) |
| EC-1 | Pass | `ec1_cut_while_printing_puts_later_marks_on_the_new_paper` (TD-20 in a line; TP-4 in a stroke, D-5) |
| EC-2 | Pass | `ec2_empty_paper_replies_reach_0_0_and_no_lines`, `ec2_product_without_printers_has_no_paper_topic`, `ec2_kept_paper_by_name_without_printer_refused_without_name`; `ac8_machine_replaced_without_printer_keeps_paper`, `ac22_without_printer_no_effect_no_failure` |
| EC-3 | Pass | `ec3_late_listener_gets_whole_paper_once_then_appended` (both test printers) |
| EC-4 | Pass | `ec4_full_speed_loses_no_mark_shown_at_end_of_call` (four lines queued, one advance at full speed, one message) |
| EC-5 | Pass | `ec5_copy_holds_marks_up_to_the_command` |
| EC-6 | Pass | `ec6_paper_png_unwritable_path_refused_with_reason` |
| EC-7 | Pass | `ec7_detach_in_a_line_keeps_what_was_printed` (the row under the head faint, the reach ends there) |
| EC-8 | Pass | `ec8_commands_on_a_kept_paper_by_name`, `ec2_kept_paper_by_name_without_printer_refused_without_name` |

Support: `advance_unpaced_runs_parts_after_call` (`sessions/tests/host_call_tests.cpp`; A-6).

Column status: every criterion and edge case of the Library column
passes: **Done**.

### Reference pictures

The five pictures of `library/paper/tests/fixtures/` (`td20.png`,
`tp4.png`, `pc100c.png` 1311 × 246, `ce150.png` 2646 × 1700,
`ce1600p.png` 9921 × 671) were made by `Picture.cpp` in this step and
looked at once (2026-11-15): the PC-100C's characters in its 5 × 7 font
with round dots, the CE-150 frame demo's strokes in their colours with
round ends and joins inside the margins, the test printers' marks where
the tests put them. They were not set beside a Copy of 2.3.0 and 0.8.1
for the same printout (design §8 asks for that look): that needs the old
GUI apps launched at their pins, which this step did not do; it is left
for the Part C2 boundary review. From now on the files fix the pixels; a
difference is a finding.

### Deviations and details decided here

- **Reach** (AC-4): from the least to the greatest y of a mark, a text line
  or a feed since the paper was last empty (a text line counts as put
  down); 0..0 while empty. So the TI converter's reach starts at 0 (its
  first text line) and the Sharp converter's is the points' range (D-14).
- **Interfaces** (design §3 allows renames): `PaperOutput::connectStanding()`
  (where the paper stood at the first call a printer kept while
  unconnected, so kept marks land where they were made; a printer calls
  `emitFedTo` before its `standing()` moves); `Paper::release()` (forget a
  printer whose capability is gone without calling it); `readMarksReply`
  gives the reach and marks (`MarksRead`) rather than a `Paper`;
  `PaperPart` takes the machine control optionally and by `setControl`,
  because the harness builds LIB-18's machine control after the product's
  parts; `HarnessProduct::paperPart()` is new: the harness then hands the
  part the control and adds the step `cut-paper` to LIB-6's vocabulary
  (A-2, A-3).
- **Harness commands** (AC-23): `paper png <path> [<printer>]`; a command
  without a name and without a printer plugged in is refused "No printer
  is plugged in." (EC-8), `cut-paper` included; the step `- cut-paper:`
  then has no effect (AC-22). In the harness nobody offers the clipboard,
  so `paper copy` is refused "No frontend offers the clipboard." and
  `paper cut` cuts (§3.6).
- **A-6 test** is in `sessions/tests/host_call_tests.cpp` (the design names
  `app_session_tests.cpp`, which does not exist). `advanceUnpaced` reads no
  wall clock in virtual time (LIB-11 AC-6): its `afterCall` gets wall time 0.
  The products' Frequent tests (golden runs, harness) are unchanged by it.
- **Fixtures**: `ce1600p.marks` holds the 30 strokes of the
  `plotter_ascii` reply that stay within 140 units (its reach recomputed),
  so that its picture (9921 × 671) stays small in Frequent; `pc100c.marks`
  is the reply of `diag.ti59`, `ce150.marks` that of `plotter_ce150_demo`.
  The printers' descriptions as test data: the PC-100C's printable part
  is its whole width (D-15), its 21 greys `255 × (1 − inkAlpha(heat))`;
  a plotter's stroke in the picture is one point (25.4/72 mm = 3.5649
  units), as Calc-U-1600's Copy draws it; the plotter paper widths
  rounded to four decimals (565.8947, 2122.1053 units).
- **Sharp reply files**: one per plotter block of each of the 14 `.paper`
  files, also for a plotter without points (`reach 0 0`).
- **TI converter**: the PC-100C's character table is copied into the
  converter from `core/PC100C.cpp` (`PRN_CODE` is in an anonymous
  namespace there; M-39 changes no product code).
- **Web**: `armClipboard` keeps the design's name (no `calcu` prefix; no
  product uses the name); `COMPONENT_CSS` beside `COMPONENT_SOURCES` (the
  panel's CSS, scoped under `.calcu-client`); the panel decides "near the
  end" from the live `scrollTop` (a late scroll event in WebKit made the
  rule flaky otherwise).
- **Qt6**: the text view asks for the system's fixed font with
  `setFixedPitch(true)`; the offscreen platform has no monospaced family,
  so the test checks the requested font. `QtTestMain.cpp` silences the
  offscreen platform's debug noise. Not compiled on MSVC or on CI's older
  Qt yet (CI at the end of Part C2): `QImage::loadFromData` and the
  device-less `QWheelEvent` constructor are the calls to watch.
- **Panel empty check**: a paper that was only fed exactly to y = 0 has the
  reach 0..0 and is shown as empty by both panels; no printer does that.

## Calc-U-1600 (M-45, 2026-11-16, branch `m45-paper-debugger-1600`)

Built and run as the Library column was (M4 Pro, macOS 27.2, Apple
clang, Ninja, RelWithDebInfo): `build/tests` (`-DCALCU_FRONTEND_QT=OFF`,
Frequent 33 of 33) and `build/qt` (`-DCALCU_FRONTEND_QT=ON
-DCALCU_TESTS_LONGER=ON`, Frequent 35 of 35, `calcu_qt6_tests` 48 cases);
the Web `dist/` with Homebrew Emscripten and `frontends/web/run-tests.sh`
(Playwright, Chromium and WebKit). The column is **In progress**: it is
Done in M-46, where AC-36's preset clause is tested again on format 2
presets (design §10.1).

The code (design §10.3): `core/Connector/PlotterPaperOutput.hpp` (new: the
descriptions, the pen colours, the mapping of §10.4, `rawPlotPoints` for
today's summaries); `AlpsPlotterMechanism.hpp` (the strokes go; a
`PenListener`; the event log stays); `Ce150Card.hpp`, `CE1600PCard.hpp`
(own the output, D-16); `PC1500Machine`, `PC1600Machine` (the plot-point
and clear calls go; `ce150PaperOutput()`, `ce1600pPaperOutput()`); the
interface machines offer `<place>/paper-output`; `c1600.plotter-paper`
goes; `LegacyCalls::CoreParts::ce150`; `app/SharpPrinters.*` (new),
`app/PlotterPaper.*` deleted; `SharpAppPart` (a `PaperPart`, P-A2),
`ControlState::paperOwner`, `SharpWebPart` (the paper topic, commands and
request go; `ui.paper` the owner's ID); `HarnessProduct` (a `PaperPart`);
Qt6: `PlotterPaperWidget` deleted, `MainWindow` docks `calcu::qt6::PaperPanel`
always, `MachineController` hands it every message and answers the
clipboard with `ClipboardService` (P-A3); `library/qt6/MacClipboard.*`
(moved, D-17) and `ClipboardService` on Cocoa; the screenshot runner's
`plot` target from `paper.picture`, the verbs `expect-panel`,
`expect-title` (and `command`, for LIB-10 AC-41); Web: `build.js` takes
the component set and its CSS, `PaperDock` shows `CalcuPaperPanel`, the
old paper code of `Panels.jsx`, `painters.js`, `engine.js` goes; today's
tools write `CALCU_PAPER_OUT` from a connected paper.

| AC | Result | Test |
|---|---|---|
| AC-32 | Pass | `plotter_paper_tests.cpp`: `lib8_ac32_ce150_states_its_paper_on_every_model` (PC-1500, PC-1500A, PC-1600 with the CE-150: `checkPaperOutput`, 56 mm, 42.75 mm drawn on, the margins even, pen strokes, no text layer, 1200 dpi), `lib8_ac32_ce1600p_states_its_paper` (210 mm, 190 mm); `harness_tests.cpp`: `lib8_ac32_harness_paper_marks_of_the_ce150`, `lib8_ac32_harness_cut_paper_and_png` |
| AC-33 | Pass | `plotter_paper_tests.cpp`: `lib8_ac33_pen_down_move_up_one_stroke_in_the_pen_colour` (both plotters, their cards), `lib8_ac33_move_with_pen_up_and_feed_give_no_mark`, `lib8_ac33_feed_without_mark_grows_the_reach` |
| AC-34 | Pass | `lib8_ac34_four_pen_colours`; the width (one point, round ends) is the panels' (LIB-8 T13, T15) and shows in `CalcU1600Shots.lib8-ac38`'s capture |
| AC-35 | Pass | `paper_session_tests.cpp`: `lib8_ac35_copy_of_a_plotter_is_1200_dpi` (a CE-1600P printout: 9921 pixels wide, 1200 dpi), `lib8_ac35_long_roll_lowered_by_ac17`; macOS: `calcu_qt6_tests` `ac16_mac_clipboard_keeps_physical_size` (P16-9) |
| AC-36 | Pass (M-45); again on format 2 in M-46 | `paper_session_tests.cpp`: `lib8_ac36_papers_topic_lists_both_plotters_without_any_plugged`, `lib8_ac36_ce150_paper_kept_through_detach_rom_module_and_model_switch` (PC-1500 A04 → A03, a CE-161, a PC-1600 with the CE-150 in MODE 1: it prints on below), `lib8_ac36_ce1600p_paper_kept_through_detach_and_its_rom_variant`, `lib8_ac36_user_preset_naming_a_plotter_starts_it_empty` (version 1, P-A2); Qt6 `CalcU1600Shots.lib8-ac36`; Web `play.spec.js` `lib8_ac36 paper panel always shown, named while a plotter is plugged in` |
| AC-37 | Pass | `ce1600p_tests.cpp`, `ce150_tests.cpp`: the cases renamed `lib8_ac37_…` on a connected paper (`_stepper_steps_become_places`, `_a_stroke_per_pen_down`, `_ce150_colour_every_third_arrival`, `_ce1600p_colour_turret` (new: the CE-1600P had no turret case), `_reset_keeps_the_paper` (also: it plots on below, D-11)); `plotter_paper_tests.cpp`: `_places_map_to_the_panel`, `_strokes_split_by_colour`, `_copied_picture_size` (they replace `shared_geometry_tests.cpp`'s three); `ce158_tests.cpp` reads the CE-150's paper |
| AC-38 | Pass | Qt6 `CalcU1600Shots.lib8-ac38` (`plotter/lissajou-1600.pc1600`; `expect-panel { name: paper, title: CE-1600P, empty: false }`; captures of the panel and of the copy); Web `play.spec.js` `lib8_ac38 plotter example shows the paper panel with marks` |

Exit check of M-45 (the plotter's paper survives a model switch):
`lib8_ac36_ce150_paper_kept_through_detach_rom_module_and_model_switch`
switches from a PC-1500 to a PC-1600, and the CE-150 attached there again
prints on below its first printout.

### Golden and reference runs

- The 23 golden runs replay unchanged (`golden_runs_tests.cpp`, Frequent):
  the four with a paper (`pc1500a-ce150`, `pc1600-ce150`,
  `pc1600-ce1600p`, `pc1600-attach-ce1600p`) give their 108 and 218
  points and sums from the marks (§10.7). No difference.
- `lib8_ac39_sharp_plots_turned_into_paper_replies` unchanged.

### Deviations

- **The reach of a feed.** A feed from where the paper stands gives the
  reach from the first place fed to, not from the place it stood before
  (the library's `Paper::fedTo` touches the new place): a feed of 8 half
  steps from 0 reaches 1 to 8 units (`lib8_ac33_feed_without_mark_grows_the_reach`).
- **Where the output lives.** `ce150PaperOutput()` and
  `ce1600pPaperOutput()` are on the core classes, and the interface
  machines offer the output only once the card is on the bus (a plug on a
  running machine waits for the off point, C1600-1 D-2).
- **The test helper `takeRawPlotPoints`** (`PlotterPaperOutput.hpp`) reads
  what a plotter kept on a paper of its own; today's tools (`printCe150Report`,
  `CALCU_PAPER_OUT`, `pc1600_plotter_probe`) and the job tests use it.
- **The CE-1600P's own start draws a dot.** After a rebuild (its ROM
  variant) the plotter's start puts the pen down once below the paper; the
  AC-36 case checks that what was drawn stays as it was, not the count.
- **The narrow Web window.** Where only one docked panel fits (a phone in
  portrait), the paper panel gives way to the INSPECTOR while no plotter
  is plugged in, as the panels give way today (D-18; ledger 1600-12
  amended). On wider windows it is always there.
- **The macOS clipboard** goes through Cocoa only on the `cocoa` platform
  and without a test sink: the offscreen platform of `calcu_qt6_tests`
  keeps its own clipboard. Its test writes to a named pasteboard of its
  own, not the user's clipboard (`macSetClipboardImage(…, pasteboard)`,
  `macPasteboardImageSize`).
- **`expect-panel` reads the panel's own state** (`PaperPanel::paperEmpty()`,
  new) and the owner's name; the scenarios run in `build/.../shots-check`.

### Findings

- During development, one run of `calcu_qt6_tests` (before the platform
  check above) wrote a white test picture to the general pasteboard of
  the machine it ran on.
- CI should be dispatched at the merge: the macOS clipboard path is
  platform code (§10.10), and the Qt6 component set now links Cocoa.

### Calc-U-1600: AC-36 on format 2 (M-46, 2026-11-16, branch `m46-presets-1600`)

| ID | Result | Test case |
|---|---|---|
| AC-36 (the preset clause) | Pass | `lib8_ac36_user_preset_naming_a_plotter_starts_it_empty` (a format 2 preset naming `printer: CE-150` starts with empty paper), `paper_session_tests.cpp` on the library's loader; Web `play.spec.js` `lib8_ac38 …` on the format 2 example; Qt6 `CalcU1600Shots.lib8-ac36`, `lib8-ac38` |

`tests/gui/lib8-ac36.shots.yaml` no longer expects the paper empty right
after the CE-150 is attached: the attach replies when the machine is ready
for keys (LIB-18), and by then the CE-150's power-on has fed the paper.
Every criterion of the column passes: **Done**.
