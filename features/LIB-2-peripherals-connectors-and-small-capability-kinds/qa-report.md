# LIB-2: QA report

Results per acceptance criterion and edge case of `spec.md`, per column.

## Library (M-23, 2026-11-15)

Built and run on an M4 Pro, macOS 27.2, Apple clang, CMake, Ninja, in a
RelWithDebInfo tree. The code is `library/machine/Attach.*` (the attach
members of `calcu::machine::Machine`) and the new area
`library/peripherals/` (namespace `calcu::peripherals`, in
`calcu_library`): the kinds `Controls`, `SerialPort` and `RealTimeClock`
with the ready-made `ControlTable` and `SerialPortQueues`, the host link
(`PtyHostLink.cpp` where `CALCU_HAS_PTY` is set, macOS and Linux;
`NoHostLink.cpp` elsewhere), `HostLinks`, and `PeripheralsPart`, the
session part that pumps the host links and sets the real-time clocks.

The tests use LIB-1's test machines with the additions of design §6, T2
(`library/machine/tests/TestMachines.*`): on `test-dot` a chain connector
(places `a`, `b`; contact level, power cycle), a one-module connector
(places `card-1`, `card-2`, `Takes::One`) and a clock component `rtc`;
the peripherals `test-hub` (offers the place `jack`, two at once, nothing
follows) and `test-uart` (one byte every 10 ms of emulated time); the
test printer with the key `feed` and the switches `online`, `trace`; a
product rule that refuses the test printer with the test UART. Every
plugged peripheral counts the operations it is told of.

Result of the run: `calcu_library_tests` 212 tests (36 of them LIB-2's),
1,992 checks passed, 0 failed, 0.50 s; `calcu_library_pty_tests` 4 tests,
29 checks passed, 0 failed, 0.8 s (ten runs in a row, all passed). The
whole Frequent group passed (10 entries, 67 s: Calc-U-59 23 s,
Calc-U-1600 42 s). The sources of `library/machine/` and
`library/peripherals/` (with `NoHostLink.cpp`) and `library/sessions/`
also compile with Emscripten (`em++ -std=c++20 -fwasm-exceptions -Wall
-Wextra`). GCC 13 and MSVC were not run locally (no such compiler here;
CI runs batched at the end of the part): the POSIX calls are only in
`PtyHostLink.cpp` and `pty_tests.cpp`, which CMake leaves out on Windows
and Emscripten.

| ID | Result | Test case (file) |
|---|---|---|
| AC-1 | Pass | `ac1_pinout_states_level_takes_and_after` (level, takes and after of four pin-outs; the same peripheral gives a power cycle on `test.chain` and nothing on `test.bus`) (`machine/tests/attach_tests.cpp`) |
| AC-2 | Pass | `ac2_fit_follows_pinout_also_for_added_peripheral` (every place of `test-dot` against every peripheral: taken exactly when the pin-out is in `fits`; a module definition added to the catalogue later is listed and attached) (`attach_tests.cpp`) |
| AC-3 | Pass | `ac3_place_takes_its_count_chain_holds_several` (a and b one each on one chain; the hub's jack two; a `Takes::One` connector one over both its places) (`attach_tests.cpp`) |
| AC-4 | Pass | `ac4_attach_lists_peripheral_and_capabilities_and_says_what_follows` (under the place and the connector; its capabilities with the peripheral and its place as owner; the variant asked for), `ac4_switched_off_machine_says_nothing_follows` (`test-dot` switched off by `off` answers Nothing, switched on the pin-out's power cycle) (`attach_tests.cpp`) |
| AC-5 | Pass | `ac5_detach_removes_peripheral_and_capabilities` (the capability list as before the attach; power cycle on `test.chain`, nothing on `test.bus`) (`attach_tests.cpp`) |
| AC-6 | Pass | `ac6_can_attach_agrees_with_attach_refusal_changes_nothing` (nine refusals, one per rule of design §3.1 and the product rule: `canAttach` and `attach` give the same sentence, plugged in, the capabilities and the core's plug calls unchanged; then a yes and the attach) (`attach_tests.cpp`) |
| AC-7 | Pass | `ac7_full_place_refuses` ("slot-1 already holds the Test module.") (`attach_tests.cpp`) |
| AC-8 | Pass | `ac8_chain_holds_peripherals_in_their_places` (`attach_tests.cpp`) |
| AC-9 | Pass | `ac9_peripheral_place_makes_a_tree_and_its_pinout_decides` (machine, a, hub, jack, printer; the attach at `a` says power cycle, at `a/jack` nothing; the printer's controls at `a/jack/controls`) (`attach_tests.cpp`) |
| AC-10 | Pass | `ac10_operations_keep_peripherals_and_tell_each` (reset, power cycle, cold boot with a hub, a printer on it and a module: plugged in unchanged, each told of every operation) (`attach_tests.cpp`) |
| AC-11 | Pass | `ac11_attach_reads_no_file` (ROM bytes in memory under a file name that exists nowhere reach the peripheral as given). Review: `Attach.cpp` has no file call (`attach_tests.cpp`) |
| AC-12 | Pass | `ac12_same_emulated_seconds_on_two_tick_rates` (`test-uart` on `test-seg`, 100,000 ticks per second, and `test-dot`, 1,000,000: one emulated second gives the same 100 bytes on both) (`peripherals/tests/time_tests.cpp`) |
| AC-13 | Pass | `ac13_no_advance_no_output` (50 ms of wall time without an advance: nothing sent, the tick count unchanged) (`time_tests.cpp`) |
| AC-14 | Pass | `ac14_controls_listed_with_id_form_state` (ID, form and state in the descriptor's order; named by owner and ID: two printers on the hub's jack have their own; none without the peripheral) (`peripherals/tests/controls_tests.cpp`) |
| AC-15 | Pass | `ac15_key_down_up_at_tick_counts` (`controls_tests.cpp`) |
| AC-16 | Pass | `ac16_switch_position_seen_and_reported`, `ac16_restore_from_request` (positions of `PeripheralRequest::switches` at build and at an attach, without the owner's reaction; a key and an unknown ID ignored) (`controls_tests.cpp`) |
| AC-17 | Pass | `ac17_bytes_out_and_in_in_order` (through the capability of a machine), `ac17_line_settings_as_set` (a line the model lacks stays empty), `ac17_receive_limit_leaves_rest_with_caller` (8,192 bytes) (`peripherals/tests/serial_port_tests.cpp`) |
| AC-18 | Pass | `ac18_bytes_both_ways_moved_between_advances` (the app session with `PeripheralsPart` and a real pseudo-terminal: the test opens the serial port file at 9,600 baud while the machine set 1,200; the first 30 bytes in order; "HELLO" reaches the machine only with a host call, not in the 100 ms before; DCD on once the program has written) (`peripherals/tests/pty_tests.cpp`, macOS and Linux) |
| AC-19 | Pass | `ac19_fixed_name_in_the_folder` (`<folder>/test-uart.serial`, a symbolic link to a `/dev/` terminal, the same after a detach, an attach elsewhere and another machine; without a folder the raw device) (`pty_tests.cpp`) |
| AC-20 | Pass | `ac20_file_outlives_port_and_reconnects` (the port detached: the file stays, the program's descriptor is not hung up and takes bytes; a port of the same kind on another machine is connected again, its bytes arrive, and what the program sent meanwhile reaches it) (`pty_tests.cpp`) |
| AC-21 | Pass | `ac21_read_and_set_kept_fields` (no year on a chip that keeps none; an empty field left as it is), `ac21_host_date_time_is_local_time` (`peripherals/tests/real_time_clock_tests.cpp`) |
| AC-22 | Pass | `ac22_clock_moves_by_emulated_time_only` (50 ms of wall time change nothing; the preloaded half second and 29 s of ticks carry over midnight to the next day and weekday) (`real_time_clock_tests.cpp`) |
| AC-23 | Pass | `ac23_set_from_host_at_build_and_after_operation` (the app session with `PeripheralsPart` and the rig's fixed instant: set when the machine is built, ahead after two emulated seconds, set again at the end of an operation and of a stopped one) (`real_time_clock_tests.cpp`) |
| AC-35 | Pass | `ac35_replace_detaches_attaches_asks_once` (one unplug, one plug, one answer in a full place; a replace of what is not there and one of an unfitting peripheral refused, the old one kept; the product rule does not count what is taken out) (`attach_tests.cpp`) |
| EC-1 | Pass | `ec1_unfitting_peripheral_refused_with_reason` ("The Test printer does not fit slot-1.") (`attach_tests.cpp`) |
| EC-2 | Pass | `ec2_detach_of_empty_place_changes_nothing` (an empty place, another peripheral's ID, a place that does not exist) (`attach_tests.cpp`) |
| EC-3 | Pass | `ec3_detach_takes_children_asks_once` (the hub with two printers: one unplug, the answer of the hub's place) (`attach_tests.cpp`) |
| EC-4 | Pass | `ec4_missing_rom_named` ("The Test module could not be attached: test-module-8k.rom is missing.", the file in `missingRoms`) (`attach_tests.cpp`) |
| EC-5 | Pass | `ec5_without_pty_machine_runs_and_bytes_go_nowhere` (a link factory that gives none, as `NoHostLink.cpp`: the machine runs one emulated second, every sent byte is taken out and dropped, the status says no terminal) (`peripherals/tests/host_links_tests.cpp`, every platform) |
| EC-6 | Pass | `ec6_unknown_control_refused`, `ec6_wrong_form_refused` (a switch pressed, a key set) (`controls_tests.cpp`) |
| EC-7 | Pass | `ec7_sending_never_waits` (a million bytes nobody takes) (`serial_port_tests.cpp`); `ec7_no_reader_machine_not_held_up` (a million bytes through a real host link whose file nobody opened, well within 2 s) (`pty_tests.cpp`) |

Support cases: `ac29_product_refusal_reaches_caller` (the library form of
the hook of AC-29; `attach_tests.cpp`), `host_links_move_bytes_by_rule_and_rebind`
(a link made only when the port is first used, the CE-158's rule; unbound
and bound again; `host_links_tests.cpp`).

The tests were checked against broken code (each change made, the tests
run, the change undone): the full-place rule left out (AC-3, AC-6, AC-7
fail); the clocks not set at the end of an operation (AC-23 fails); the
received bytes not handed to the port (AC-18, AC-20 and the rebind case
fail).

Technical Requirements (review): attach, detach and replace are members
of `Machine`, called between two advances; the test peripherals know only
the tick rate handed to them at the plug and reach the machine through
their capabilities; nothing above the core reaches a peripheral but
through capabilities; the library defines no signal of a pin-out and reads
`PinOutDescriptor::level` nowhere. Only `PtyHostLink.cpp` starts threads
and takes locks (the host adapter of ADR 0009).

### Deviations from the design

- **`detach` takes the catalogue** (`detach(place, id, catalogue)`): the
  pin-out of a place on a peripheral, whose answer a detach gives (AC-9,
  EC-3), is in the peripheral's descriptor, which the machine does not
  keep. `attach` and `replace` take it already.
- **`RomImage`, `PeripheralRequest` and `SwitchPosition` are in
  `machine/Attach.hpp`**, with `Refusal` and `ChangeResult`; `Build.hpp`
  includes it, so every user of `Build.hpp` sees them as before. The
  design put `switches` in `Descriptor.hpp`, where `PeripheralRequest` is
  not.
- **`PeripheralsPart`** (`peripherals/PeripheralsPart.*`) is the session
  part of LIB-3 A-1 and LIB-2 A-2, A-4: it pumps the host links in
  `beforeCall` and `afterCall`, rebinds them in `machineChanged`, and sets
  the real-time clocks in `machineChanged` and in a new hook
  `SessionPart::operationEnded` (called at the end of an operation and of
  a stopped one). So `library/peripherals/` includes `sessions` for this
  one file; LIB-1 design §2a lists `peripherals: machine` only.
  `HostLinks`, the kinds and the host link include `machine` only.
- **Without a folder for serial links** a host link makes no serial port
  file, and its path is the raw terminal device. Today's fallback,
  `~/Library/Application Support/Calc-U-1600`, names a product and a
  macOS folder; the folder comes from LIB-9's `Paths::serialLinkFolder()`.
- **`HostLinks`** takes a link factory (the tests inject one; the default
  is `openHostLink`), rebinds at every `pump` before moving bytes (an
  attach inside an operation changes no machine generation), and has
  `machineGone()` for a session without a machine. `HostLinkStatus::open`
  means a pseudo-terminal is behind the file, `connected` that it is bound
  to a port now.
- **Sentences not in the table of §3.1:** a replace of a peripheral that
  is not in the place, "<place> does not hold the <name>."; several
  missing ROM files, "… could not be attached: a, b are missing.".
  Places are named by their path, "a/jack".
- **Small additions:** `placeText(PlacePath)`, `controlsCapability()`,
  `ControlTable::on()`, `SerialPortQueues::sentWaiting()` and
  `receivedWaiting()`, `fieldsOf(HostDateTime)`. `ClockFields::weekday`
  and `HostDateTime::weekday` count from Sunday = 0, as `std::tm`.
- **LIB-1 and LIB-3 tests:** the test printer now has a controls
  capability and the catalogue four peripherals, so the counts in
  `capability_tests.cpp` (`ac21_…`, `ac22_…`), `descriptor_tests.cpp`
  (`ac3_…`) and `sessions/tests/command_tests.cpp` (`ac39_…`) were raised;
  what they check is unchanged.

Not done in M-23 (other columns): AC-24 to AC-27 (Calc-U-59, M-25) and
AC-28 to AC-34 (Calc-U-1600, M-26, M-31, M-47).

## Calc-U-59 (M-25, 2026-11-15, branch `m25-connectors`)

With C59-6 (its qa-report has the run, the golden runs and the
deviations). The PC-100C and the Solid State ROM modules are plugged by
`TIMachine::plug()`/`unplug()` through `Machine::attach`, `detach` and
`replace`; the PC-100C's controls are a `ControlTable`. Tests in
`products/calc-u-59/tests/peripherals_tests.cpp` (LIB-2 design P1–P4),
`CalcU59Tests`, Frequent; all pass.

| ID | Result | Test case |
|---|---|---|
| AC-24 | Pass | `lib2_ac24_places_functional_power_cycle_follows` (every model: `slot-1` and `printer` functional, one at a time, a power cycle follows; 14 modules fit the slot, the PC-100C the printer place; on a running TI-59 each attach and detach answers `PowerCycle`) |
| AC-25 | Pass | `lib2_ac25_second_module_refused_until_detached` ("slot-1 already holds the Master Library.", then a detach and the attach of LE), `lib2_ac25_empty_slot_machine_runs` (each model with nothing in `slot-1` calculates) |
| AC-26 | Pass | `lib2_ac26_pc100c_print_adv_trace` (`printer/controls`, owned by the PC-100C at `printer`: PRINT and ADV keys, TRACE switch; a switch pressed and a key set refused, PAPER unknown), `lib2_ac26_no_pc100c_no_controls` (no machine lists them without a PC-100C); what they do: C59-6 AC-5 |
| AC-27 | Pass | `lib2_ac27_no_serial_port_no_clock` (no `serial-port` and no `real-time-clock` capability on any model, with or without the PC-100C; no such component) |

The Calc-U-59 column is Done.

## Calc-U-1600, the M-26 part (2026-11-15, branch `m26-sharp-interface`)

With C1600-1 (its qa-report has the run, the deviations and the open
points). Tests in `products/calc-u-1600/tests/peripherals_tests.cpp` and
`products/calc-u-1600/app/tests/serial_links_tests.cpp`,
`app_session_tests.cpp`, program `CoreTests` (label `frequent`):
11 tests, 188 checks passed, 0 failed. The column is Done in M-47
(plan §2.1; it also waits for M-31, the library's sessions), so it stays
In progress.

| ID | Result | Test case |
|---|---|---|
| AC-28 | Pass | `lib2_ac28_slots_one_chain_contact_level_power_cycle` (module slots take one, the 60-pin connector a chain, all contact level with a power cycle after an attach or detach; an attach on a running machine waits for the off point, design D-2, also the one the ROM reaches by itself; row 1600-4 in effect) |
| AC-29 | Pass | `lib2_ac29_second_printer_refused` (a CE-150 or CE-1600P in `printer`, another printer refused: the place takes one), `lib2_ac29_ce1600p_ce158_refused_both_ways` ("The CE-1600P and the CE-158 cannot be used together."; row 1600-22 in effect) |
| AC-30 | Pass | `lib2_ac30_ce150_and_ce158_together` |
| AC-31 | Pass | `lib2_ac31_ce158_same_emulated_time_pc1500_pc1600` |
| AC-32 | Pass | `lib2_ac32_ce161_switch_blocks_writes` (the controls capability and the write block; the save that sets the switch is LIB-7's, M-47) |
| AC-33 | Pass | `lib2_ac33_pc1600_rs232c_file`, `lib2_ac33_ce158_file_on_first_use_all_models` (`app/tests/serial_links_tests.cpp`, through the session's `HostLinks`, the product-held adapter until M-31) |
| AC-34 | Pass | `lib2_ac34_rtc_on_three_models`, `lib2_ac34_clock_runs_on_emulated_time` (`peripherals_tests.cpp`), `lib2_ac34_set_at_build_and_after_operation` (`app/tests/app_session_tests.cpp`) |

Not here: what the C1600-1 design (§1) gives the column's later steps,
M-31 (the library's sessions) and M-47 (media places).

## Calc-U-1600, completed (M-47, 2026-11-16, branch `m47-media-1600`)

What the column waited for (M-31's sessions: Done there; M-47's media
places): AC-32's save that sets the CE-161's switch is LIB-7's and passes
in `lib7_ac40_write_protect_power_cycle` (`app/tests/app_session_tests.cpp`:
Name & Save with the switch, the file says `write-protect: protected`, the
module refuses writes at once, the instance comes back protected, a module
without the switch refuses); the module's switch is the controls kind's
`write-protect` (D-9) through which LIB-7 sets it. The media places of the
CE-150, the CE-1600P and the CE-1600F: LIB-1 AC-40 (above in that report).
AC-28 to AC-34 pass (the M-26 part); the column is **Done**.
