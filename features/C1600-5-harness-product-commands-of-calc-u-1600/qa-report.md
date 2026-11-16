# C1600-5: QA report

Results per acceptance criterion and edge case of `spec.md`.

## M-51 (2026-11-16, branch `m51-cli-1600`)

Built and run on an M4 Pro, macOS 27.2, Apple clang, CMake, Ninja: the
tree `build/tests` (Debug, the tests at `-O1`, Qt off) and `build/groups`
(RelWithDebInfo, Qt on, Longer on) of the worktree.

The code (design §2):
- `products/calc-u-1600/app/SharpHarnessCommands.*`: every command under
  `pc1600` (the loaders of C1600-2 moved here from `app/HarnessProduct.cpp`),
  on the session's commands, LIB-10's requests and the product kinds;
  `app/HarnessProduct.cpp` registers them (`addCommands`).
- `app/BasicDump.*`: the lines of `dump-basic` over `c1600.loader-memory`,
  moved from the CLIs.
- `app/SerialScripts.*` (`SerialScriptsPart`): the feeds as a slice hook in
  emulated ticks, the capture files, what `report` counts; in the harness
  run only (the product's `sessionParts()` after the app layer's).
- `core/Connector/PrintableBytes.hpp`: AC-8's rule, moved out of
  `Ce158Card.hpp`; the Web's printer pane (`app/SharpWebPart.cpp`) uses it too.
- `core/Machine/SharpProductKinds.hpp`: the product kind
  `c1600.routine-call` (`RoutineCall`), offered by both interface machines
  (`LH5801::callRoutine`, `SC7852::callRoutine`: the call instruction's push
  and jump).
- `core/Connector/Ce158Card.hpp`: the card passes the speed its ROM sets to
  the serial port's line settings (`onBaud`; spec decision log).
- `SharpAppPart` (options of the harness: typing in emulated time, serial
  port files only on request, the run's screenshot folder;
  `requestSerialLink`, `serialLinked`).

Tests: `products/calc-u-1600/tests/harness/product_commands_tests.cpp`, 26
`TEST`s, in the program `CalcU1600HarnessTests` (with LIB-11 Part D's 5;
CTest entry of the same name, Longer: 3.6 s RelWithDebInfo (10.2 s Debug), no room in Frequent next
to `CoreTests`). Each test is a `calcu --product 1600` run of its own
(`tests/harness/HarnessRun.hpp`) with its expected replies written out.
Result: 31 `TEST`s, 499 checks passed, 0 failed.

| ID | Result | Test case |
|---|---|---|
| AC-1 | Pass | `c1600_5_ac1_commands_under_pc1600_on_every_model` (`--help`'s usage lists the ten `pc1600` commands; each model answers `inspect`, `dump-basic`, `tape status`) |
| AC-2 | Pass | `c1600_5_ac2_dump_basic_pc1500a_after_a_program` (the six pointers and two rows written out), `c1600_5_ac2_dump_basic_pc1600` (the five pointer lines with the Z-80 addresses, two rows) |
| AC-3 | Pass | The PC-1500A reply of `c1600_5_ac2_dump_basic_pc1500a_after_a_program` is, line for line, what `pc1500_cli --preset … --dump-basic` of the base commit (51acc6e) printed for the same program, without the `---` headings (D-3; checked with that binary before it was removed); `c1600_5_ac3_lines_in_todays_form` (`--json`: the pointers and rows as data) |
| AC-4 | Pass | `c1600_5_ac4_every_view_per_model` (all 14 views on the PC-1500, PC-1500A and PC-1600, each reply equal to `c1600.inspector`'s view of the same machine; the `<title>: not on the PC-1500` / `PC-1600` line where the model lacks the view, D-2), `c1600_5_ac4_unknown_view_refused` |
| AC-5 | Pass | `c1600_5_ac5_record_writes_wav_at_motor_stop_and_eject` (PC-1500A with a CE-150: `CSAVE "A"`, the WAV written when the motor stopped; `pc1600 file-kind` reads it as the BASIC file A; at eject it stays; a recording taken out while the motor runs is written as LIB-7 writes it, to its last motor stop), `c1600_5_ac5_record_ejects_the_tape_in_first` |
| AC-6 | Pass | `c1600_5_ac6_status_playing_recording_empty` (empty, recording 12.398522308 s, playing 0 of 12.398520833 s after the recording was put back as a tape definition; `--json`) |
| AC-7 | Pass | `c1600_5_ac7_feed_one_byte_per_character_time_and_hold` (300 baud: 30 bytes in 1 s, 20 with `hold=10`), `c1600_5_ac7_second_feed_replaces_rest`, `c1600_5_ac7_capture_complete_at_quit_and_switch` (the CE-158 echo program of `tests/fixtures/basic/ce158_echo.bas`: its prompt in the first file at the switch, in the second at the end of the run; the line fed comes back on the printer port), `c1600_5_ac7_link_gives_the_path_and_refuses_feed` (`path: <save folder>/calcu1600-ce158.serial`), `c1600_5_ac7_report_ce158_with_printer_port` (the bundled `ce158_demo.pc1500a`), `c1600_5_ac7_rs232c_on_pc1600` (`setcom1600.pc1600` and `RUN "S"`: 308 bytes at 9600 baud, captured and reported) |
| AC-8 | Pass | `c1600_5_ac8_text_rule` (bytes 00H to FFH); the `<1A>` of the RS-232C report |
| AC-9 | Pass | `c1600_5_ac9_no_host_link_unless_asked` (a CE-158 on the PC-1500A and on the PC-1600 with its RS-232C port: no serial port file in the run's folders, `c1600.serial-links` empty, `host link: no`) |
| AC-10 | Withdrawn | — |
| AC-11 | Pass | `c1600_5_ac11_host_drive_attach_folder_and_off` (a folder attached, `FILES "S3:"` lists its file; a second folder while attached; `off` detaches) |
| AC-12 | Pass | `c1600_5_ac12_call_returns_registers_and_time` (LH5801 `LDI A,42H`/`RTN` and Z-80 `LD A,42H`/`RET`: returned, A = 42H, the program counter and stack pointer as before, the ticks; the same reply in a second run), `c1600_5_ac12_timeout_pauses_and_says_so` (a routine that loops: `returned: no, timeout`, paused after 100 ms), `c1600_5_ac12_refused_unless_paused`. Finding: the PC-1600's LH5803 does not run while the Z-80 runs the machine (in MODE 0, and after `MODE 1` typed in); a call on it ends at its timeout, as the test shows. A routine on the LH5803 that returns was not seen |
| AC-13 | Pass | M-51, second part (below): `c1600_5_ac13_restore_continues_identically_pc1500`, `_pc1500a` (also with a CE-150 recording a CSAVE, a CE-158 and a CE-161), `_pc1600`; `c1600_5_ac13_full_pc1600_with_ce1600p_disk_tape_ce158_modules` (two machines: the CE-1600P with a disk on side 1, a tape recording a CSAVE, a CE-1600M and a CE-161, the host drive; the CE-150 and the CE-158 with a CE-1600M and a CE-1601M, since the CE-1600P and the CE-158 cannot be used together); `c1600_5_ac13_refused_for_other_peripherals` (`tests/machine_state_tests.cpp`, CTest `CalcU1600MachineStateTests`) |
| EC-1 | — | (none in the spec) |
| EC-2 | Pass | `c1600_5_ec2_tape_without_cassette_refused` (`record` and `status` refused on the PC-1500A and PC-1600 without a CE-150 or CE-1600P; with the CE-1600P the status) |
| EC-3 | Pass | `c1600_5_ec3_port_missing_refused_not_held` (`ce158` without a CE-158, `rs232c` on the PC-1500A and PC-1500: refused; the CE-158 attached later has no feed) |
| EC-4 | Pass | `c1600_5_ec4_basic_not_set_up_pointers_only` (right after the cold boot: the six pointer lines, no rows) |
| EC-5 | Pass | `c1600_5_ec5_refused_on_pc1500_and_for_no_folder` |
| EC-6 | Pass | `c1600_5_ec6_unwritable_path_notice_at_motor_stop` (the command done; at the motor stop LIB-7's notice "The cassette recording “x.wav” could not be saved: …") |

Coverage: 12 of 12 criteria (AC-10 withdrawn), 5 of 5 edge cases.

### Deviations from the design

- **Tests** are not in `CoreTests` (design §6) but in the program
  `CalcU1600HarnessTests`, Longer: `CoreTests` uses nearly all of the
  Frequent budget (58.7 s of 60 s), and the 31 runs take 3.6 s.
- **`c1600.routine-call`** is one capability of the main CPU that calls on
  any processor by its debug target name (spec decision log).
- **`tape record`** is LIB-7's media part with a kind of its own for the
  binding (`cassette recording`, a codec that writes the tape's sound alone
  at the path; D-1): the writes at the motor stops and at eject, and the
  notice of EC-6, are the media part's.
- **`feed`'s `hold`** counts from the command (spec decision log).
- **The reply of `serial … report`** gives the text one reply line per line
  of text, each after `| `, so a reply stays one line per line.

### Golden runs

`c1600_1_ac4_golden_runs_through_the_interface` passes unchanged (in
`CoreTests`). The core changes of M-51 are a call method on the two CPUs,
the CE-158's speed passed to its serial port, and code no caller used any
more (the yield hook, `syncFromCore`, the probe hooks); no golden run
differs.

## M-51, second part: the machine state (2026-11-16, branch `m51b-machine-state`)

Built and run on an M4 Pro, macOS 27.2, Apple clang, CMake, Ninja: the
tree `build/tests` (Debug, the tests at `-O1`, Qt off, Longer on) and
`build/qt` (RelWithDebInfo, Qt on) of the worktree; `tools/build_wasm_check.sh`.

The code (design §2, §3.7, D-6):
- `core/Machine/StateRecords.hpp`: `StateWriter` / `StateReader`, tagged,
  length-prefixed records, little-endian. Each part names its fields once in
  a member template `stateIo(Io&)` that runs both ways, so a save and a
  restore cannot differ in their fields.
- The parts with state (design §3.7): the LH5801 / LH5803 and the SC7852
  (registers, interrupt state, what ME1 and the ports were written); the
  PC-1500's RAMs, LH5811 with its serial block, uPD1990AC, piezo sampler;
  the PC-1600's internal RAM, ports and latches, bank selection
  (`PC1600Bank`), bus arbiter, LH5803 side, LCD controllers with their RAM,
  sub-CPU with its clock, TC8576F, piezo sampler, the 64 Hz, 0.5 s and RTC
  accumulators, the key readiness counters; the emulated time; the cassette
  deck (the tape's sound and position, a recording under way) and the tape
  in play mode as it came (`CassettePlace`); every card (`CardBase::saveState`,
  `core/Connector/CardState.hpp`): the modules' memories, bank latches,
  flash decoders and write-protect switch (`SoftwareDefinedCard`), the
  CE-150's and the CE-1600P's LH5810 / port, relays, plotter mechanism and
  where the pen stands on the paper, the CE-158's LH5811, UART and printer
  port bytes, the CE-1600F with its disk, side, head and controller, the
  host drive's request under way and its open handles (the directory below
  the folder and the searches, each file by its path below the folder; not
  the folder); the serial ports' line settings and peer lines; the power
  cycle under way and the write-protect switches (`SharpMachine`).
- `SharpMachine::saveState()` / `restoreState()` and the generic kind
  `machine-state` (LIB-11's `harness::MachineState`), offered on the main
  processor (`LH5801/machine-state`, `SC7852/machine-state`); the library's
  `state-save` / `state-restore` and their header were there (LIB-11 T18).
  A restore that does not fit (a record of another tag or length, a module
  of another size, a state cut short) puts the state from before back and
  is refused with a sentence: a refused restore changes nothing.

Tests: `products/calc-u-1600/tests/machine_state_tests.cpp`, 5 `TEST`s,
in a program of its own, CTest `CalcU1600MachineStateTests`, Longer (12.4 s
Debug at `-O1`; `CoreTests` stays at 58.4 s RelWithDebInfo). Each case
saves while a program runs (a BASIC loop counting on the display with a
BEEP, or a CSAVE recording), runs 2 s and takes the replies (`wait`, `lcd`,
`display`, `ticks`, and per case `pc1600 tape status`, `inspect disks`,
`inspect inventory`, `dump-basic`, `serial ce158 report`), the sound
(`audio`) and the whole state after the 2 s (`state-save`); then restores
in the same run, and in a second run set up the same way 1.337 s later,
and compares all three. A save right after a restore gives the bytes
restored. Result: 5 `TEST`s, 358 checks passed, 0 failed. A field left out
of a record was tried once (the PC-1600's RTC accumulator): the case on the
second run fails, as it should.

### Deviations and findings

- **No `SharpMachineState.hpp/.cpp`** (design §2): the kind is a small
  class inside `SharpMachine` (`StateKind`) over `saveState`/`restoreState`,
  offered by each model on its main processor; the forms are in
  `core/Machine/StateRecords.hpp` and `core/Connector/CardState.hpp`. The
  kind is the library's generic one, so no product kind was listed.
- **The tests** are in a program of their own (`CalcU1600MachineStateTests`,
  Longer), not in `CoreTests` (design §6), for `CoreTests`' budget.
- **The full PC-1600 case is two machines**: the CE-1600P and the CE-158
  cannot be used together (LIB-2 D-6; ledger 1600-22), so the CE-158 is
  tested with the CE-150 on a second PC-1600 (spec decision log).
- **What is the session's, not the machine's, is not in the state** (spec
  decision log): LIB-7's record of what each media place holds and where
  `pc1600 tape record` writes its WAV, the paper drawn (LIB-8), the bytes
  in a serial port's queues on their way to or from the host, the
  debugger's breakpoints, watches, history and trace. Finding: after a
  restore in the same run, a medium ejected or inserted since the save is
  still what LIB-7's media part believes; an `eject tape` after the restore
  ejected the restored recording but wrote no WAV, since the `tape record`
  binding had ended at the earlier eject. In another run set up the same
  way it is written as after the save.
- **Refused** with a sentence: a state saved while a change of the
  peripherals waited for the off point of a power cycle (an attach or
  detach on a running machine; the change is not a part of the machine):
  "The state was saved while a change of the peripherals waited for the
  power cycle; save it again once the machine is ready." No other part was
  found whose state cannot be restored.

### Golden runs

`c1600_1_ac4_golden_runs_through_the_interface` passes unchanged (in
`CoreTests`, both trees). Saving only reads; the core's run code is not
changed.
