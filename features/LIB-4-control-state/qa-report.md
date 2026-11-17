# LIB-4: QA report

Results per acceptance criterion and edge case of `spec.md`, per column.

## Library (M-37, 2026-11-15)

Built and run on an M4 Pro, macOS 27.2, Apple clang, CMake, Ninja,
RelWithDebInfo (`build/tests`, `-DCALCU_FRONTEND_QT=OFF`). The code is
`library/control/` (namespace `calcu::control`, in `calcu_library`):
`ControlState.hpp/.cpp` (the payload, `same`, `toValue`, the topic
`control-state`), `ControlRules.hpp` (`EntryKey`, `ControlRules`),
`Compute.hpp/.cpp` (`ControlSetup`, `computeControlState`) and
`ControlPart.hpp/.cpp`. LIB-3 gets `SessionPart::beforeFlush`, called at
the start of every flush (`library/sessions/SessionPart.hpp`,
`AppSession.cpp`; LIB-18 design §8). The harness session carries a
`ControlPart` after `StartupPart`, set up by the new
`HarnessProduct::controlSetup()` (default: the product's catalogue, no
printers, no rules; design §7, D-9).

The tests are `library/control/tests/` (`control_state_tests.cpp`,
`availability_tests.cpp`, `control_part_tests.cpp`, the product rules
`TestControlRules.hpp`), `before_flush_runs_at_every_flush` in
`library/sessions/tests/topic_tests.cpp` and
`ac9_state_control_state_reads_the_topic` in
`library/harness/tests/commands_tests.cpp`, all compiled into
`calcu_library_tests` (Frequent). They use LIB-1's test machines; the
test hub (`test-hub`) gets the holding media place `hub-disk` (kind
`test-disk`, side 0) for EC-4. The session tests use LIB-3's rig
(`library/sessions/tests/TestParts.hpp`: a scripted clock, the test
frontend, the test part's operation command) with a `ControlPart` among
its parts, paced.

Result of the run: `calcu_library_tests` 405 tests (31 of them LIB-4's:
14 in `control_state_tests.cpp`, 11 in `availability_tests.cpp`, 4 in
`control_part_tests.cpp`, one in `topic_tests.cpp`, one in
`commands_tests.cpp`), 3,279 checks passed, 0 failed. The whole Frequent
group passed: 28 of 28 CTest entries. CI was not dispatched (plan §1: at
the end of the part).

| ID | Result | Test case (file in `library/control/tests/` unless named) |
|---|---|---|
| AC-1 | Pass | `ac1_models_and_running_model`, `ac1_variants_of_running_model` (test-dot V1, V2 and the running one; test-seg none), `ac1_places_fitting_and_plugged_with_variant` (test-seg: `slot-1`, `printer`, `aux`, the fitting peripherals in catalogue order, the module's running variant 8k), `ac1_nested_place_after_its_peripheral` (the hub in test-dot's `a`, its `jack` at once after `a`; gone with the hub), `ac1_empty_place_is_a_choice`, `ac1_media_places_owner_kind_passage_sides`, `ac1_paper_panel_owned_by_plugged_printer_or_none` (also a printer in `a/jack`), `ac1_actions_in_order` (`reset`, `cold-boot`, `load-preset`, `open`; the first two equal LIB-18's command names), `ac1_entries_in_descriptor_order`, `control_state_value_round_trip` (the product's entries last, under `product`) (`control_state_tests.cpp`) |
| AC-2 | Pass | `ac2_machine_reason_for_peripheral` (the UART beside the printer: the machine's sentence, the same as `canAttach`'s; the module in `card-2` while `card-1` holds one, Takes::One), `ac2_full_place_asked_as_replace` (`canAttach` refuses, the choice is available), `ac2_variant_of_plugged_peripheral` (8k of the plugged 4k module available as a replace; a variant refused alone by its key), `ac2_no_entry_refused_for_rom` (`availability_tests.cpp`) |
| AC-3 | Pass | `ac3_choices_and_actions_refused_while_operation_runs` (variants, empty choices, fitting peripherals and their variants, inserts, `load-preset`, `open`: "Another operation is running."), `ac3_models_reset_cold_boot_stay_available`, `ac3_recomputed_from_machine_after_operation` (an operation attaches the printer: refused while it runs, plugged and available after, the UART then refused by the machine) (`availability_tests.cpp`) |
| AC-4 | Pass | `ac4_computed_at_end_of_call_and_command` (the end of a host call, a command and a request each send the machine as it is), `ac4_sent_only_when_it_differs` (host calls and commands that change nothing send nothing; an operation's start and end send one each; a late listener gets it) (`control_part_tests.cpp`); `before_flush_runs_at_every_flush` (host call, command, request, service answer, `advanceUnpaced`, `listen`; after the parts' `afterCall`) (`library/sessions/tests/topic_tests.cpp`) |
| AC-5 | Pass | `ac5_place_media_place_panel_absent_where_model_lacks_them` (test-seg and test-dot against each other's places and media places; `hub-disk` absent without the hub; no panel without printers, none on a model where no printer fits) (`control_state_tests.cpp`) |
| AC-6 | Pass | `ac6_no_machine_no_control_state` (no machine: nothing sent, not to a late listener either; the topic is declared) (`control_part_tests.cpp`). In the harness a product whose ROM set is incomplete starts no session at all (LIB-11 EC-1) |
| AC-7 | Pass | `ac7_product_entry_carried_unread` (the very payload the rule gave, in `toValue` by its own `toValue`), `ac7_product_refuses_generic_entry_with_its_reason` (a model, a variant, an action, an insert, an empty choice), `ac7_refused_place_passes_reason_to_its_choices` (`availability_tests.cpp`) |
| AC-8 | Pass | `ac8_two_sessions_same_machine_same_control_state` (two sessions, one script of host calls, keys and an operation that attaches; payloads `equals` and `toValue` equal after each step, the same number sent) (`control_part_tests.cpp`) |
| AC-9 | Pass | `ac9_state_control_state_reads_the_topic` (the harness's test product: `state control-state` before and after `attach printer test-printer`; the machine's reason of the UART in `aux`) (`library/harness/tests/commands_tests.cpp`) |
| EC-1 | Pass | `ec1_plugged_peripheral_says_plugged` (`control_state_tests.cpp`) |
| EC-3 | Pass | `ec3_module_added_to_catalogue_listed_next_time` (a module definition added to a copy of the catalogue: `slot-1` lists it last, available) (`control_state_tests.cpp`) |
| EC-4 | Pass | `ec4_peripheral_media_place_owned_by_peripheral_no_place_for_it` (`hub-disk` with the hub in `a` as owner; no place stands for it) (`control_state_tests.cpp`) |
| EC-5 | Pass | `ec5_operation_before_product_before_machine` (one entry with all three reasons; the first rules object that refuses gives its reason) (`availability_tests.cpp`) |

Coverage: AC-1 to AC-9, EC-1, EC-3, EC-4, EC-5 of the Library column
(the spec has no EC-2). Every criterion of the column passes, so the
Library column is Done (`features/INDEX.md`; design §1: "Done in M-37").
The commands behind the actions `load-preset` and `open` and behind an
insert come with LIB-6, LIB-13 and LIB-7 (M-38, M-42, M-41); their
entries are computed here from the busy state and the rules, as the
design says (§1).

### Harness and products

The harness session of every product carries `control-state` from M-37
(D-9). The products do not override `controlSetup()` yet, so
`state control-state` on Calc-U-59 and Calc-U-1600 shows the generic
entries without a paper panel and without product entries until their
column steps (M-49, M-57). No product code and no frontend changed; the
products' interim topics (`c59.model`, `c1600.control-state`) are
untouched (design §5). Both products' tests pass in Frequent.

### Golden runs

M-37 changes no core and no frontend; the control state only reads the
machine. Both products' golden replays pass unchanged in Frequent.

### Deviations from the design, chosen by the simpler structure

- `ac1_nested_place_after_its_peripheral`: the design names the test hub
  in test-seg's `aux`, but the hub fits only `test.chain`; the test plugs
  it into test-dot's `a` (as LIB-18's `ac15_attach_place_path` does).
- `control_state_value_round_trip`: there is no reader of the value (a
  translator only writes it), so the case checks the shape of §3.6 (keys
  in order, `reason` only when not available) and that two computations
  from the same machine give equal values and equal payloads.
- `toValue` details §3.6 leaves open: the top-level `variant` is `null`
  on a model without variants; a fitting peripheral without variants has
  no `variant` key; a panel without an owner has no `owner` key.
- Rule 2 of AC-2 on a choice: the reason of a refused place comes before
  a rule on the choice itself (both are the product's; D-5).
- While an operation runs, the place as a whole stays available (only a
  product rule refuses it, design §3.1); its empty choice and every
  peripheral and variant in it are refused (AC-3). The rules are still
  asked for the entries rule 1 does not cover (the models, `reset`,
  `cold-boot`, the places); no machine check runs.
- `ControlState` has lookups the design does not list (`place`, `action`,
  `mediaPlace`, `productEntry`, and the free function `choice`), for
  translators and tests.
- `computeControlState` with no catalogue gives the running model alone
  (no lists); `ControlPart` sets nothing without a machine (AC-6).
- `HarnessProduct::controlSetup()` is a `const` member returning the
  setup by value.
- No `docs/behaviour-changes.md` row: no frontend draws from the control
  state before M-49, so nothing visible to a user changes in M-37.

## Calc-U-1600 (M-49, 2026-11-16, branch `m49-shell-1600`)

The product's session has the library's `ControlPart` with
`app/SharpControlRules` (`makeSharpSession`, `frontends/web/bindings.cpp`,
the harness product's `controlSetup()`); the 60-pin places have one
pin-out each (`core/Machine/SharpCatalogue`); `app/ControlState.*`, the
topics `c1600.control-state` and `c1600.machine` (its lamps are the
product's `c1600.lamps`) and the Web's `c1600.ui` are gone. Both frontends
draw from `control-state` (LIB-14 1600). Tests: `CoreTests`
(`tests/control_state_tests.cpp`, `tests/descriptor_tests.cpp`,
`tests/harness_tests.cpp`), `CalcU1600QtTests` (Qt6), `shell.spec.js`
(Web, Chromium and WebKit).

| AC | Result | Evidence |
|---|---|---|
| AC-16 | Pass | `lib4_ac16_ac17_pc1500_models_variants_and_fields` (the three models; A01, A03, A04 on the PC-1500, A04 running; the PC-1500A's one variant), `lib4_ac16_ac17_pc1600_fields_ce1600p_variants_and_next_variant` (New, Old) |
| AC-17 | Pass | the same two cases, one block per row of the table: the slots, `printer` and `interface` with what fits and what is plugged in (EC-1), the CE-1600P's variants in `printer` and `c1600.ce1600p-next-variant` while unplugged, the media places `cassette` and `floppy` with their owner, `c1600.disk-controls` (shown on the PC-1600, available with the CE-1600P, else "The disk drive is in the CE-1600P."), `c1600.ce158-printer-panel`, `c1600.host-drive-folder` (the folder mounted), the paper panel's owner; `lib4_ac17_each_60_pin_place_lists_only_what_fits` (the catalogue: CE-150 in `printer`, CE-158 in `interface`, CE-150 and CE-1600P in the PC-1600's `printer`, the host drive in `host-drive`, the bus ROM in `bus-rom`); `lib1_ac36_connectors_pinouts_places_debug_targets`, `lib1_ac36_…` peripherals' fits updated |
| AC-18 | Pass | `lib4_ac18_host_drive_refused_where_no_folder_can_be_mounted` (two sessions on a PC-1600: `host-drive` and its choices refused with "A host folder cannot be mounted here.", every other place, the models, variants, media places, panels and actions equal); `lib3_ac38_mirror_holds_every_callback` (the same through the fixture) |
| AC-19 | Pass | Qt6: `lib14_ac36_bar_pc1500_pc1500a`, `lib14_ac36_bar_pc1600`, `lib14_ac34_qt_menus_in_family_order` draw the models, variants, places, media places and product entries from `control-state`; Web: `lib14_ac36 the bar per model`, `lib14_ac35 the web menus`; `app/ControlState.*` and `c1600.ui` deleted |
| AC-20 | Pass | `lib4_ac20_operation_refuses_model_variant_and_place_choices` (a preset loading: variants, every place's choices and the inserts refused with "Another operation is running."; models, `reset`, `cold-boot` available, `load-preset` not); Qt6 `lib4_ac20_qt_controls_disabled_while_loading`, `lib14_ac38_loading_after_500_ms_blocks_nothing`; Web `lib4_ac20 peripheral buttons, disk picker and tape controls disabled while loading` |
| T16-7 (AC-9 for this product) | Pass | `harness_tests.cpp` `lib4_harness_state_control_state_has_paper_panel_and_product_entries` |

Golden runs: `c1600_1_ac4_golden_runs_through_the_interface` passes
unchanged (the pin-out change is descriptor data; the core's bus is not
touched). Cost per flush: not visible; `CoreTests` grew by the new cases
only (about 1.4 s).

### Deviations from the design, chosen by the simpler structure

- **The new 60-pin pin-outs are chains, not `Takes::One`** (design D.5).
  The places share the one connector `60-pin`; with `Takes::One` LIB-2's
  rule (AC-7) keeps every other place of that connector empty, so the
  CE-158 could not go beside a CE-150. One peripheral per place comes from
  the places' `takesAtOnce` 1, as before. `productRefusal`'s place rule is
  gone as the design says; the bus ROM's rule and ledger 1600-22 stay.
- **Two more product entries** (D.4 lists four): `c1600.mount-directory`
  `{available, reason}` for File ▸ Mount Directory… (a PC-1600 in a
  frontend that can mount a folder), and `c1600.host-drive-folder` carries
  `available`/`reason` beside `folder` for Unmount, because a product item
  follows its entry's availability (LIB-14 M9).
- **`c1600.lamps` moved to `SharpAppPart`** (D.7 removes what was left of
  `c1600.machine`): the tape counter and the floppy lamp, which both
  frontends draw, are a product topic of their own; the model and the
  build counters of `c1600.machine` went (the control state's model, the
  machine session's generation).
- **The CE-1600P's variant while unplugged** is still a product setting
  of the run (`c1600.ce1600p-rom`, the entry `c1600.ce1600p-next-variant`),
  but no menu sets it any more: the shell offers a peripheral's variants
  while it is plugged in (LIB-14 AC-2, M5), and an attach from the menus
  or the bar uses the default variant. Ledger 1600-45.
- `SharpAppPart::model()` (a `ModelId`) and `ce1600pVariant()` ("New",
  "Old") replace `currentModel()` and the ROM enums (LIB-14 D-28).

Column status: Done.

## Calc-U-59 (M-57, 2026-11-17, branch `m57-shell-59`; Qt6 and Web)

Design §10. M-54 left `LibraryPartsSetup::printers = {PC-100C}` and M-56
`controlRules = {&TIDebugPart}` with C59-4's product entries, so the
product side was only checked; no `TIControlRules` file was needed (the
debug part is the rules, D-16 holds: no refusal of the product's own).
The Qt6 and Web frontends draw the model, the module, the printer and the
card controls from `control-state` (LIB-14 §10); `c59.web.model` is gone,
Qt6 no longer reads `c59.model` (the Swift frontend does until M-58, D-17).

Tests: `CalcU59Tests` `tests/control_state_tests.cpp` on `tests/AppRig.hpp`
(the real ROMs), the CTest `CalcU59FrontendsDrawControlState`, the drawing
in `CalcU59QtTests` and the Web's `shell.spec.js`.

| AC | Result | Evidence |
|---|---|---|
| AC-10 | Pass | `lib4_ac10_models_ti59_ti58_ti58c_without_variants` |
| AC-11 | Pass | `lib4_ac11_slot_1_lists_14_modules_and_the_empty_slot` (each model; ML plugged at the start), `lib4_ac11_printer_lists_pc100c_and_the_empty_place` (attach and detach), `lib4_ac11_choices_refused_while_a_preset_loads` (the empty slot and a module disabled with a reason while a preset runs, available after; row 59-45) |
| AC-12 | Pass | `lib4_ac12_card_reader_on_the_ti59_only` (kind `magnetic-card`, pass-through, sides 0 and 1, the machine's own); the drawing: `CalcU59QtTests` `lib14_ac27_bar_per_model`, `c59_7_ac13_…` (no card controls on the TI-58), `lib14_ac26_qt_menus_in_family_order` (no card items on the TI-58); Web `lib14_ac27 the bar per model` |
| AC-13 | Pass | `lib4_ac13_paper_panel_owned_by_pc100c_or_none` (attach, model switch, detach) |
| AC-14 | Pass | `lib4_ac14_debug_panel_entries_carried` (`c59.asm-run`, `c59.trace`, `c59.calculator-tab` with `available`, under `product`) |
| AC-15 | Pass for Qt6 and the Web; Swift in M-58 | `CalcU59FrontendsDrawControlState` (no file of the Qt6 and Web frontends reads `hasCardReader`, `cu59::info(`, `c59.model`, `c59.web.model` or `allModules`); the drawing cases above. `App/MachineModel.swift` goes with C59-8 |
| AC-9 (this product) | Pass | `lib4_harness_state_control_state_has_paper_panel_and_card_reader` (`state control-state` through `calcu`) |

Ledger: 59-9 and 59-45 in effect for Qt6 and the Web from M-57.
Cost per flush (§10.9): not visible; the TI-59's 14 `canReplace` per flush
did not move the Frequent timings (`CalcU59Tests` 546 cases as before
within the budget).

Column status: In progress (Done in M-58 with the Swift frontend).

### M-58 (2026-11-17, branch `m58-swift-59`; Swift)

The Swift frontend draws the model, the module, the printer and the card
controls from `control-state` through the library's shell models
(`Bridge/TIShell`, C59-8): the model picker, the picker of `slot-1`, the
button "PC-100C" and the card controls of the bar, the art by the control
state's `model`. `App/MachineModel.swift`, the interim topic `c59.model`
(`TIModelState`, `TIAppPart::allModules`) and the bridge's model, module
and printer members went (LIB-4 design §10.7, C59-8 §5).

| AC | Result | Evidence |
|---|---|---|
| AC-10 to AC-14 (Swift) | Pass | `swift_shell_tests.cpp` `c59_8_ac3_bar_per_model` (the model picker, 15 entries in the slot picker, "PC-100C", the card controls on the TI-59 only), `c59_8_ac18_…` (the card items on the TI-59 only), `c59_8_ec1_more_follows_the_control_state` (a preset loads: the examples disabled); XCUITest `test_lib14_ac8_the_bar` (LE chosen on the bar is in `state control-state`), `test_c59_8_ec3_no_card_controls_on_the_ti58`, `test_lib8_ac31_the_printer` (the bar's "PC-100C" plugs the printer in, the paper panel names it) |
| AC-15 | Pass (Qt6, Web, Swift) | `CalcU59FrontendsDrawControlState` over the Swift sources too (no `MachineModel`, `c59.model`, `allModules`; `App/MachineModel.swift` gone); `CalcU59NoTestAids` |

Ledger: 59-9 and 59-45 in effect for Swift from M-58. Column status:
**Done**.
