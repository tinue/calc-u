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
| AC-16 | Pass | `ac16_copy_asks_clipboard_with_picture_and_text` (TD-20: picture, text joined by line ends, name, resolution; TP-4: picture only; `copy_cut_tests.cpp`); `ac16_full_width_all_it_reaches_at_stated_resolution`, `ac16_reference_pictures_of_the_test_printers`, `ac16_reference_picture_of_the_pc100c` (`picture_tests.cpp`; 1311 pixels, byte for byte against `fixtures/pc100c.png`); the plotters' pictures in Calc-U-1600 since Part D: `lib8_ac16_reference_pictures_of_the_plotters` (`products/calc-u-1600/tests/plotter_paper_tests.cpp`; 2646 pixels for the CE-150, 9921 for a short CE-1600P paper, byte for byte against `tests/fixtures/paper/*.png`); `png_rgb_bands_and_phys` (`formats/tests/png_tests.cpp`); Qt6 `ac16_copy_puts_image_and_text_on_clipboard` (`ClipboardService`: image size, dots per metre, text; the offscreen clipboard works on macOS) |
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

The five pictures of `library/paper/tests/fixtures/` (since Part D
`ce150.png` and `ce1600p.png` are in `products/calc-u-1600/tests/fixtures/paper/`, unchanged; `td20.png`,
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
| AC-37 | Pass | `ce1600p_tests.cpp`, `ce150_tests.cpp`: the cases renamed `lib8_ac37_…` on a connected paper (`_stepper_steps_become_places`, `_a_stroke_per_pen_down`, `_ce150_colour_every_third_arrival`, `_ce1600p_colour_turret` (new: the CE-1600P had no turret case), `_reset_keeps_the_paper` (also: it plots on below, D-11)); `plotter_paper_tests.cpp`: `_places_map_to_the_panel`, `_strokes_split_by_colour`, `_copied_picture_size` (they replace `geometry_tests.cpp`'s three); `ce158_tests.cpp` reads the CE-150's paper |
| AC-16 (the plotters' reference pictures; Part D) | Pass | `plotter_paper_tests.cpp`: `lib8_ac16_reference_pictures_of_the_plotters` (moved from the library's `ac16_reference_pictures_of_the_three_printers`: the same pictures byte for byte, now on `sharp::ce150PaperDescription()` and `sharp::ce1600pPaperDescription()`; design §10.14) |
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

## Calc-U-59 (M-53, 2026-11-16, branch `m53-paper-debugger-59`)

Built and run on an M4 Pro, macOS 27.2, Apple clang, Ninja,
RelWithDebInfo: `build/tests` (`-DCALCU_FRONTEND_QT=OFF`, Frequent
42 of 42; `CalcU59Tests` 324 `TEST`s in 8.3 s; `CalcU59ReferenceRuns` with `CALCU59_REFERENCE_COMPARE=1`: 0 of 57 differ, 36 only as accepted), `build/qt` (`-DCALCU_FRONTEND_QT=ON`: `CalcU59QtTests`, new, 3 `TEST`s, 35 checks, 4.2 s, Frequent where Qt6 is found; the app builds without warnings; not launched), the Web `dist/`
through `frontends/web/build.sh` and `frontends/web/run-tests.sh`
(Playwright 95 passed, 5 skipped (Checkpoint and pacing), 0 failed, 1.5 min). Swift: `xcodebuild` from the worktree stopped with "There is no XCFramework found" (known for worktrees under `.claude/worktrees/`); the bridge was checked with `clang++ -fsyntax-only` (macOS, and `TIAppSession.mm` for the iOS simulator) and the Swift sources with `swiftc -typecheck`; `CalcU59XcodeProjectCheck`, `CalcU59NoAttachSources` pass. The Swift build and `UIRegressionTests` are left to the user.

The code (design §11.3): `core/PC100CPaper.*` (new: the description, the
font, the character table, the dot rules and the placement `lineDots`,
D-20, D-22); `core/PC100C.*` on `calcu::paper::PaperOutputBase` (text at
PRT.GO, the dots and the feed when the line ends, D-21; `PC100C::CodeLine`
recorded only while `recordCodeLines` is on, P-E3);
`core/PrinterOutput.hpp`, `core/TIPrinterTypes.hpp` and
`app/PrinterFont.hpp` deleted; `TIMachine` offers `printer/paper-output`;
`TIMachineTesting::printedCodeLines`; `app/TIPrinters.*` (new);
`TIAppPart` (the printer lines, `cutPaper`, `c59.cut-paper`,
`c59.printer-print`, `c59.printer-adv`, `c59.toggle-printer-trace` go;
`c59.printer` keeps `connected` and `trace`, P-E1; `set-control`, P-E2;
P-E4 in `applyState`; KEYSTROKES `CutPaper:` is `PaperPart::cutPlugged`,
P-E5); `makeAppSession` owns a `PaperPart` after `TIAppPart` (D-23);
`FrameSampler` no longer reads the paper; `TIWebPart` (`c59.web.printer`
holds `connected` and `trace` only; the printer part of `c59.static-data`
goes); `HarnessProduct` (`papers`, `printers`); the headless tool's
`printer_lines` from a connected `Paper`, `printer_code_lines` from
P-E3 (`tests/check_core_seam.sh` allows that one include until M-59);
the converter of AC-39 on `core/PC100CPaper.hpp`; the library's
`pc100cDescription()`, `ac16_reference_picture_of_the_pc100c` and the
fixtures `pc100c.png`, `pc100c.marks` moved to the product (§10.14).
Frontends: Qt6 `frontends/qt6/app/PaperPane.*` (new; the splitter's
second pane, object name `paper`: the PC-100C's connect toggle,
`paper-connect`, above `calcu::qt6::PaperPanel`; owner and TRACE from
`c59.printer`, P-E1), `PrinterWidget.*` deleted, `EmulatorController`
answers `clipboard-image` and `clipboard-text` with `ClipboardService`
(P-E6), the app links `calcu_qt6`; Web `src/Paper.jsx` (new, `PaperPane`:
the toggle above `CalcuPaperPanel`, `data-panel=paper`), `Printer.jsx`,
`paintStrip` and `stripTops` deleted, `build.js` takes the component set
and its CSS, `bindings.cpp` builds the session with the `PaperPart` and
the `DebuggerPart` (the Web makes its own session, not `makeAppSession`);
Swift (S-P1, until C59-8): the bridge keeps the `paper PC-100C` topic and
`PrinterView` draws its dots and text, Copy and Cut send `paper.copy` and
`paper.cut`, PRINT, ADV and TRACE send `set-control`, the bridge answers
`clipboard-image` and `clipboard-text`.

| ID | Result | Test case |
|---|---|---|
| AC-24 | Pass | `pc100c_paper_tests.cpp`: `lib8_ac24_pc100c_states_its_paper` (`checkPaperOutput` on the TI-59, TI-58, TI-58C; 430/1000 mm, 129.07 units, the whole width printable, 21 greys, 600 dpi, a text layer); `descriptor_tests.cpp` `lib1_ac32_pc100c_and_14_modules` (the capability of the kind `paper-output`) |
| AC-25 | Pass | `pc100c_paper_tests.cpp`: `lib8_ac25_line_gives_20_positions_and_font_dots`, `_complete_line_takes_10_18`, `_feed_takes_7_09_and_an_empty_line`, `_adv_gives_two_feeds`, `_characters_of_today_for_every_code` |
| AC-26 | Pass | `pc100c_paper_tests.cpp`: `lib8_ac26_cut_short_rows_and_columns_with_their_darkness`, `_takes_only_the_paper_of_its_rows` |
| AC-27 | Pass | `lib8_ac27_copy_is_600_dpi_about_1311_pixels_wide`; `lib8_ac16_reference_picture_of_the_pc100c` (moved from the library: `tests/fixtures/paper/pc100c.png` byte for byte on the product's description) |
| AC-28 | Pass | Qt6 `CalcU59QtTests` `lib8_ac28_qt6_print_adv_held_trace_switch_view_control` (PRINT and ADV held, TRACE a switch reaching `c59.printer`, the view control; detached: no owner); Web `play-calculator.spec.js` `LIB-8 AC-28 PRINT and ADV are held, TRACE is a switch, the view control shows the view that is on` (also detached: no owner, no controls) |
| AC-29 | Pass | `part_flows_tests.cpp`: `lib8_ac29_detached_panel_owner_empty` (`c59.printer` not connected, the `papers` topic keeps the PC-100C, the unnamed command refused with EC-8's sentence), `lib8_ac29_paper_back_on_attach_on_any_model` (TI-59, detach, TI-58C, attach: it prints on below; was `c59_6_ac1_paper_kept_across_detach_and_attach`); `peripherals_tests.cpp` `lib8_ec7_detach_mid_line_keeps_rows` |
| AC-30 | Pass | `data_tables_tests.cpp`: `lib8_ac30_font_shape`, `lib8_ac30_feed_complete_and_cut_short_heights`; `core_tests.cpp`: `lib8_ac30_interrupt_rows_against_the_scan` (the ten cases of the scan, and the marks only in the drawn rows), `lib8_ac30_print_gives_a_line` (text layer and dots) |
| AC-31 | Pass | Web `play-calculator.spec.js` `LIB-8 AC-31 the PC-100C prints a number, the text view shows it, Cut empties the paper and is disabled` (renamed); Qt6 `CalcU59QtTests` `lib8_ac31_qt6_print_text_cut` (until LIB-14's attach scenarios, M-57) |
| AC-8 (59, the 2.3.0 presets; again on format 2 in M-54) | Pass | `part_flows_tests.cpp`: `lib8_ac8_preset_naming_printer_starts_empty`, `lib8_ac8_preset_without_printer_line_keeps_paper` (P-E4, D-29) |
| AC-22 (59) | Pass | `lib8_ac22_keystrokes_cut_paper` (P-E5) |
| AC-23 (59) | Pass | `harness_tests.cpp`: `lib8_ac23_harness_paper_text_of_the_pc100c`, `lib8_ac23_harness_paper_marks_and_png` |
| EC-7 (59) | Pass | `lib8_ec7_detach_mid_line_keeps_rows` (was `c59_6_ec2_detach_mid_line_keeps_rows_then_power_cycle`) |

### Golden and reference runs

- The 18 golden runs replay unchanged
  (`c59_1_ac4_golden_runs_through_the_machine_interface`): the `print`
  lines from the text layer of a `Paper` bound to the capability, the
  `dots` lines from P-E3. No difference.
- The 57 reference runs (`reference_runs_tests.cpp`): the `--- printed`
  lines from the `PaperPart`'s PC-100C paper; the `.txt` files unchanged.
- `lib8_ac39_ti_printouts_turned_into_paper_replies`: the committed
  `.paper-text` and `.paper-marks` files come out byte for byte from the
  converter on the core's placement.
- `printer-timing.ti59` was not re-recorded
  (`lib5_exit_printer_timing_same_paper_at_regular_and_full_speed`: 104
  lines at every speed).

### Deviations

- **The order of a line's dots** is the converter's (character, row,
  column), not row by row (D-21 says "row by row"): with one placement
  function (`pc100cpaper::lineDots`) for the core and the converter, a
  complete line's `paper marks` reply equals the conversion of the M-9
  files, which M-54 compares (C59-2 AC-11).
- **A feed's code line** is a line of zero codes with `rowCount` 7, as
  2.3.0's `PrinterCodeLine{}`, so the golden `dots` lines stay the same.
- **The code lines are kept only while recorded** (`recordCodeLines`,
  turned on by the first `TIMachineTesting::printedCodeLines`), so the app
  keeps none.
- **The tests' paper.** `tests/TestRoms.hpp` keeps a `Paper` per machine
  (`paperTap`, `freshMachine`) in place of the adapter's `takeLines`; the
  session tests read the `PaperPart`'s paper (`SessionRig::printedText`).
- **Qt6** keeps no PRINT/TRACE/ADV bar of its own: they are the panel's
  controls. **Web**: the new file is `src/Paper.jsx` (`PaperPane`), not an
  edit of `Printer.jsx`; the product's `.calcu-paper*` and printer-bar CSS
  went (their names clashed with the library's `paper.css`); at 220 px the
  panel's header wraps onto a second row (the PWA's compact "PRN" bar has no
  user on the Web any more). **Swift**: Copy gives the library's picture and
  "Copy failed" on a failure (row 59-48 names Qt6 and Web; Swift's own
  panel is C59-8).

### Findings

- The Web panel's hook for the website changed from `[data-panel=printer]`
  to `[data-panel=paper]` (`reference/WebClient.md` lists the hooks as the
  contract with `calc-u-site`).
- Swift: not built (above); the Swift build and `UIRegressionTests` are the
  user's check of this step.

Every criterion of the column passes in Qt6 and the Web (the Swift panel is
C59-8): **Done**.

### M-58 (2026-11-17, branch `m58-swift-59`): the Swift part

The Swift paper panel is C59-8's (AC-12): `PrinterView` draws the paper of
the panel's owner (`TIShell.paperOwner`, the bar model's B6) under its
name, an empty paper under none; the "PC-100C" toggle above it went to the
bar (row 59-14); Copy and Cut are `paper.copy` and `paper.cut` as since
M-53 (S-P1 ends: the panel is the native counterpart).

| ID | Result | Test case |
|---|---|---|
| AC-12 (Swift) | Pass | XCUITest `FamilyStructureTests.test_lib8_ac31_the_printer` (the paper under "PC-100C" once the bar's button attached it), `test_c59_8_ac15_one_picture_per_layout` (the paper page) |
| AC-16, AC-18 (Swift) | Pass | `test_lib8_ac31_the_printer` (Cut empties the paper); `ShellBridgeTests.test_c59_8_ac13_copy_forms` (Copy's pasteboard entries, macOS) |
| AC-31 (Swift) | Pass | `test_lib8_ac31_the_printer` (the PC-100C prints 123, the text view shows it, Cut empties it) |
