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
| AC-13 | Not done | Split off (design §8; spec decision log): the machine state needs a save and a restore on every part of the core that holds state (the CPUs, the memory with its banks, the LH5811, the clocks, the LCD controllers, the peripherals with their media). Not started in M-51; the column stays In progress |
| EC-1 | — | (none in the spec) |
| EC-2 | Pass | `c1600_5_ec2_tape_without_cassette_refused` (`record` and `status` refused on the PC-1500A and PC-1600 without a CE-150 or CE-1600P; with the CE-1600P the status) |
| EC-3 | Pass | `c1600_5_ec3_port_missing_refused_not_held` (`ce158` without a CE-158, `rs232c` on the PC-1500A and PC-1500: refused; the CE-158 attached later has no feed) |
| EC-4 | Pass | `c1600_5_ec4_basic_not_set_up_pointers_only` (right after the cold boot: the six pointer lines, no rows) |
| EC-5 | Pass | `c1600_5_ec5_refused_on_pc1500_and_for_no_folder` |
| EC-6 | Pass | `c1600_5_ec6_unwritable_path_notice_at_motor_stop` (the command done; at the motor stop LIB-7's notice "The cassette recording “x.wav” could not be saved: …") |

Coverage: 11 of 12 criteria (AC-13 open), 5 of 5 edge cases.

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
