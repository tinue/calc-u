# C1600-5 Harness product commands of Calc-U-1600: design
Status: approved by the user at the Part C2/D boundary (2026-11-16)

## 1. Scope

| Row | Step | Tests from (plan §3) | Done in (plan §5.1) |
|---|---|---|---|
| C1600-5 | M-51 (with LIB-11 1600) | AC-1–9, AC-11–13; EC-2–6 (AC-10 withdrawn; there is no EC-1) | M-51 |

M-51 does three things, in this order, as separate commits on short
branches (ADR 0029):
1. the `c1600` commands of this row (this design);
2. the harness's Calc-U-1600 entry built through `app/SharpAppPart`, and
   today's tools removed (LIB-11's design, Part D);
3. the adapter check of plan §1 over Calc-U-1600 and the library
   (LIB-11 Part D, D.7).

This design holds part 1. The commands are written against the
harness entry of part 2 (the product's own session parts in the run),
so both are merged together; the tests of part 1 run on the entry of
part 2.

What M-51 finds (taken as done by the earlier designs):
- M-44 (C1600-2): the loaders on the loader memory kind
  `c1600.loader-memory` and their `c1600` commands; C1600-5's commands
  sit "next to those of C1600-2" (AC-1).
- M-45 (LIB-10 1600): one `machine::DebugTarget` per processor, the
  run control (`pause`, `step-out`, `registers`, `peek`, `history`,
  `profile`), `DebuggerPart` in the session.
- M-46 (LIB-6, LIB-18 1600): `preset`, `check-preset`, `model`,
  `variant`, `attach`, `detach` for every Sharp model; the host drive
  attached with its folder (LIB-18 AC-30).
- M-47 (LIB-7 1600; renamed in release prep step 2: place `cassette-recorder`, kind `cassette`, step `cassette`): the media place `cassette` (holding, the tape
  deck behind it), the kind `cassette-tape` with its codec, `tape` and
  `eject`; the deck's counter and motor in a product kind.
- M-50 (C1600-3): the DAP in the app layer; nothing here depends on it.

Not here: the commands of other features that replace today's options
(LIB-11 AC-18's table, LIB-11 Part D); the probes and
`dev/tape-matrix/run.py`, which go without a successor (spec, Out of
Scope).

## 2. Modules and files

| File | Holds |
|---|---|
| `products/calc-u-1600/app/SharpHarnessCommands.hpp/.cpp` (new) | `shared::addSharpHarnessCommands(calcu::harness::CommandRegistry&, SharpHarnessParts&)`: every command of §3 under the word `c1600` (`CommandRegistry::addProduct`, LIB-11 §3.3) |
| `products/calc-u-1600/app/SerialScripts.hpp/.cpp` (new) | `shared::SerialScriptsPart : calcu::sessions::SessionPart`: per serial port the feed, the capture and the report counters (§3.4) |
| `products/calc-u-1600/app/BasicDump.hpp/.cpp` (new) | `shared::basicDump(const sharp::LoaderMemory&, Model)`: the lines of AC-2, AC-3, EC-4 (§3.1), moved from `tools/pc1500_cli.cpp:377-405` and `tools/pc1600_cli.cpp:207-237` |
| `products/calc-u-1600/core/Connector/PrintableBytes.hpp` (new) | the text rule of AC-8, moved out of `core/Connector/Ce158Card.hpp:363-382` so that the app layer and the card share it |
| `products/calc-u-1600/core/Machine/SharpRoutineCall.hpp/.cpp` (new) | the product kind `c1600.routine-call` (§3.6) |
| `products/calc-u-1600/core/Machine/SharpMachineState.hpp/.cpp` (new) | the generic kind `machine-state` of LIB-11 AC-24 on the Sharp machines (§3.7) |
| `products/calc-u-1600/core/...` (changed) | a `saveState(StateWriter&)` and `restoreState(StateReader&)` on each part that holds state (§3.7) |
| `products/calc-u-1600/core/Machine/SharpProductKinds.hpp`, `SharpCatalogue.cpp` | the new kinds listed, with their owners (C1600-1 AC-19) |
| `products/calc-u-1600/app/HarnessProduct.cpp` | `addCommands()` calls `addSharpHarnessCommands`; `sessionParts()` adds the `SerialScriptsPart` |
| `products/calc-u-1600/tests/harness_product_commands_tests.cpp` (new) | §6 |
| `products/calc-u-1600/tests/machine_state_tests.cpp` (new) | §6 (AC-13) |

## 3. The commands

All are product commands under `c1600` (AC-1; `harness.md` H-2),
registered for every Sharp model; a command that needs what a model
lacks refuses with a sentence that says so. Every handler reaches the
machine through capabilities (C1600-1's product kinds, LIB-2's serial
port kind, LIB-7's media) and names no machine class (Technical
Requirements; the CTest `CalcU1600MachineIncludeCheck` covers `app/`).

### 3.1 `c1600 dump-basic` (AC-2, AC-3, EC-4)

Reads through `c1600.loader-memory` (`peek`, PC-1500 ME0, PC-1600 the
Z-80's view). The lines are today's (`pc1500_cli.cpp:385-404`,
`pc1600_cli.cpp:220-236`) without the `---` headings, which a reply
does not need: the pointers of AC-2's table one per line in that order,
then the rows of the program area from its start to 16 bytes past its
end, 16 bytes each, `40C5: 00 0A … |…|`. A pointer line keeps today's
form, `BASPRG_END $7867 = $40D2   (len = 13)`, and on the PC-1600 the
Z-80 address beside the LH5803 one (`BASPRG_ST  $F865 = $40C5  (Z-80
$C0C5)`). EC-4: when the start does not lie before the end, the rows
are left out; the pointers are given as they are. `--json` gives
`{"pointers":[{"name","address","value","z80"?,"length"?}],"rows":[{"address","bytes"}]}`.

### 3.2 `c1600 inspect <view>` (AC-4)

`<view>` is one of the names of `inspect::viewName`
(`core/Debug/Inspect/Inspector.cpp:35-53`). The reply is
`c1600.inspector`'s `view()` as lines. A name that is no view is
refused ("No view <name>. The views: pointers, inventory, …."). A view
the model lacks gives the one line the inspector writes,
`<title>: not on the PC-1500` or `… PC-1600`
(`PC1500Inspector.cpp:388`, `PC1600Inspector.cpp:888`); that reply is
done, not refused, as the spec gives it as the reply.

### 3.3 `c1600 cassette record <path>`, `c1600 cassette status` (AC-5, AC-6, EC-2, EC-6)

- `record <path>`: refused without a media place `cassette` (EC-2;
  neither a CE-150 nor a CE-1600P plugged in). Otherwise a tape in the
  cassette is ejected first (written as LIB-7 says), and a new, empty
  tape goes in through `MediaPart::insert` with a binding to the file
  at `<path>`, relative to the working directory. `<path>` is the
  sound file, a WAV, as with today's `--tape-out`
  (`tools/CliCommon.hpp:178-212`); no cassette definition is written and no
  name is asked (D-1). The recording is written each time the motor
  stops and at `eject`, by the codec of M-47. A path that cannot be
  written fails at that write with LIB-7's notice, which `state notices`
  shows (EC-6); the command itself is done.
- `status`: from the deck's product kind (M-47): `playing <s> of <s>`,
  `recording <s>` or `empty`, the tape's length, and `motor on|off`;
  seconds with nine decimals from ticks (LIB-11 §3.1). `--json`:
  `{"tape":"playing|recording|empty","position","length","motor"}`.

### 3.4 `c1600 serial <port> …` (AC-7, AC-8, AC-9, EC-3)

`<port>` is `ce158` (the CE-158's port, owner `peripheral:CE-158@interface`)
or `rs232c` (the PC-1600's own port, owner the machine; OQ-1). The port
is LIB-2's `SerialPort` capability (`library/peripherals/SerialPort.hpp:32-41`).
A port that the running machine does not offer is refused, and nothing
is held for a machine built later (EC-3).

| Sub-command | Design |
|---|---|
| `feed <file> [hold=<n>]` | `SerialScriptsPart` reads the file and keeps the bytes for the port. In the machine session's slice hook (LIB-3, the one LIB-5 uses) it offers the next byte with `putReceived` once per character time (ten bits at `lineSettings()`'s speed, counted in ticks) in which the port took the last; the first `n` character times give nothing. A second `feed` replaces what was left. Today: `Ce158CliPeer.hpp:35-94` |
| `capture <file>` | the bytes of `takeSent` are appended to the file; the file is complete at the end of the run, at `quit` and when another `capture` names another file (closed then) |
| `link` | `PeripheralsPart` gives the port the host link of LIB-2 AC-18, its serial port file in the run's folder (LIB-11 AC-8); the reply is the path (the run folder written `<save folder>`, LIB-11 §3.1). `feed` and `capture` are refused on a linked port. Today `--ce158-pty` |
| `report` | the speed, the receiver's and the sender's state from `lineSettings()` and the port, how many bytes of the feed were taken and how many are left, all bytes the port sent since the machine was built as text (AC-8); for `ce158` also the bytes of `c1600.ce158-printer-port` as text. Today `Ce158CliPeer::report` (`:98-117`) and the Centronics dump |

AC-8's text: a line end and printable ASCII as they are, a carriage
return left out, any other byte `<XX>`; one rule, `PrintableBytes.hpp`.
AC-9: no port gets a host link unless `link` asks; the harness's
`PeripheralsPart` starts with every link off (LIB-11 AC-6, AC-8).

### 3.5 `c1600 host-drive <folder>|off` (AC-11, EC-5)

On a PC-1600 the command sends LIB-18's attach of the host drive in
`host-drive` with that folder (LIB-18 AC-30; the folder through the
product kind `c1600.host-drive-folder`), or, when the host drive is
attached, shows that folder from then on; `off` sends the detach. What
follows (the power cycle) is LIB-18 AC-30's. Refused on a PC-1500 or
PC-1500A and for a path that is no folder (EC-5).

### 3.6 `c1600 call <target> <address> [timeout=<duration>]` (AC-12)

- Refused unless the machine is paused (LIB-10's `run-state`).
  `<target>` is a debug target of LIB-10 1600 (the IDs of M-45: the
  PC-1500's processor; the PC-1600's main and sub processors).
- The product kind `c1600.routine-call`, offered per processor by the
  core, makes the call from the current state by that processor's own
  call: it saves the return as its call instruction does and sets the
  program counter to `<address>`. How each processor calls and returns
  is the research's (skill `PC1500`: the LH5801 and the Z-80 of the
  PC-1600), not this design's.
- The harness then runs LIB-10's "until the routine has returned"
  (`step-out`) with the timeout (default `10s` of emulated time): the
  machine pauses when the routine returns to the caller. The reply is
  the registers then (LIB-10's `registers` of the target) and the
  emulated time the routine took; past the timeout the machine is
  paused and the reply says `timeout` with the registers.
- Test routines: the machine-code examples of `examples/machine-code/`
  and a short routine poked with `poke`; no new ROM facts are needed
  for the test, which compares the returned state with a second run.

### 3.7 Machine state (AC-13)

The Sharp machines offer LIB-11's generic kind `machine-state`
(`library/harness/MachineState.hpp`): `save()` gives bytes, `restore()`
takes them or refuses. The form is internal (LIB-11 AC-24): a
product-internal `StateWriter` / `StateReader` (tagged, length-prefixed
records, little-endian), one record per part, in the machine's order:
- every processor (LH5801; SC7852 and LH5803 with the sub-CPU's model);
- all memory with its banks and the bank selection (PC-1600 banks,
  module RAM in both slots, ME0 and ME1);
- the clocks and timers (the LH5811, the real-time clock, the LCD
  controllers and their RAM, the bus arbiter, the piezo sampler);
- every plugged peripheral with its state and its medium: the CE-150's
  and the CE-1600P's mechanism; the CE-158's UART and printer port; the
  CE-1600F with its disk and side; the cassette deck with its tape's
  sound and position; the host drive's open handles (not its folder:
  host files are not part of the state, AC-24).

The library writes the header (product, version, model, variant,
plugged peripherals) and refuses a file that does not match (LIB-11
AC-24). Each core part gets `saveState`/`restoreState` next to its
state; a part that has none says so in its record list.

## 4. Product side

All of §3 is product side. What changes in the harness entry is in
LIB-11's Part D.

## 5. Adapters

None new. M-51 removes the CLI adapters (LIB-11 Part D, D.5).

## 6. Tasks

Tests before the code; `CoreTests`, Frequent; names `c1600_5_acN_…`,
`c1600_5_ecN_…`; on the harness entry of LIB-11 Part D through
`harness::Server` lines (the expected reply written out in the test, as
Technical Requirements ask); `SKIP_UNLESS` the ROMs are there.

| Task | What | ACs/ECs | Test (`tests/harness_product_commands_tests.cpp` unless named) |
|---|---|---|---|
| T1 | Registry under `c1600` on every model | AC-1 | `c1600_5_ac1_commands_under_c1600_on_every_model` (`--help` lists them; each model answers `c1600 inspect pointers`) |
| T2 | `dump-basic` | AC-2, AC-3, EC-4 | `c1600_5_ac2_dump_basic_pc1500a_after_a_program` (a one-line program typed in with `type`; the six pointer lines and the rows, written out), `c1600_5_ac2_dump_basic_pc1600`, `c1600_5_ac3_lines_in_todays_form`, `c1600_5_ec4_basic_not_set_up_pointers_only` (right after a cold boot) |
| T3 | `inspect` | AC-4 | `c1600_5_ac4_every_view_per_model` (the table of AC-4: each view on each model, the "not on the" lines), `c1600_5_ac4_unknown_view_refused`; the replies compared with `c1600.inspector` on the same machine |
| T4 | `tape` | AC-5, AC-6, EC-2, EC-6 | `c1600_5_ac5_record_writes_wav_at_motor_stop_and_eject` (PC-1500A with a CE-150: `type CSAVE`, the WAV after the motor stops; again at `eject`), `c1600_5_ac5_record_ejects_the_tape_in_first`, `c1600_5_ac6_status_playing_recording_empty`, `c1600_5_ec2_tape_without_cassette_refused`, `c1600_5_ec6_unwritable_path_notice_at_motor_stop` |
| T5 | `serial` | AC-7, AC-8, AC-9, EC-3 | `c1600_5_ac7_feed_one_byte_per_character_time_and_hold` (the ticks between the bytes the port took), `c1600_5_ac7_second_feed_replaces_rest`, `c1600_5_ac7_capture_complete_at_quit_and_switch`, `c1600_5_ac7_link_gives_the_path_and_refuses_feed`, `c1600_5_ac7_report_ce158_with_printer_port` (the example `examples/interfaces/ce158_demo.pc1500a`), `c1600_5_ac7_rs232c_on_pc1600` (the example `interfaces/setcom1600.pc1600`), `c1600_5_ac8_text_rule` (bytes 0x00–0xFF), `c1600_5_ac9_no_host_link_unless_asked`, `c1600_5_ec3_port_missing_refused_not_held` |
| T6 | `host-drive` | AC-11, EC-5 | `c1600_5_ac11_host_drive_attach_folder_and_off` (a temp folder with one file; a listing of `S3:` typed as `docs/PC1600-Host-Drive.md` describes it; the file's name on the display), `c1600_5_ac11_second_folder_when_attached`, `c1600_5_ec5_refused_on_pc1500_and_for_no_folder` |
| T7 | `call` | AC-12 | `c1600_5_ac12_call_returns_registers_and_time` (each target; a routine that sets a register and returns), `c1600_5_ac12_timeout_pauses_and_says_so` (a routine that loops), `c1600_5_ac12_refused_unless_paused` |
| T8 | Machine state | AC-13 | `tests/machine_state_tests.cpp`: `c1600_5_ac13_restore_continues_identically_<model>` (PC-1500, PC-1500A, PC-1600: save at a running program, run 2 s, restore, run 2 s; the display, the ticks and the sound equal), `c1600_5_ac13_full_pc1600_with_ce1600p_disk_tape_ce158_modules`, `c1600_5_ac13_refused_for_other_peripherals` |

Coverage: AC-1 to AC-9, AC-11 to AC-13 (12 of 12; AC-10 withdrawn),
EC-2 to EC-6 (5 of 5).

## 7. Tools and agent knowledge

What the commands replace, and the knowledge that changes, is in
LIB-11's Part D (D.5, D.6), because the tools go in the same step. This
row adds to it:
- `products/calc-u-1600/CLAUDE.md`, section Harness: the `c1600`
  commands (`dump-basic`, `inspect`, `tape record|status`, `serial`,
  `host-drive`, `call`) and `state-save`/`state-restore` on the Sharp
  models; the line "Not yet: `pc1600 …` commands" goes.
- `products/calc-u-1600/docs/developer/PC1600-Serial-Port.md`,
  `LCD-Text.md`: the CLI options become these commands.
- `docs/GLOSSARY.md` has "view" and "character time" (OQ-3); no change.

## 8. Golden runs and risks

- **Machine state** is the large part: a `saveState` on every core part
  that holds state. Saving only reads; the golden replays
  (`c1600_1_ac4_golden_runs_through_the_interface`) must pass unchanged,
  and a difference is a finding. A restore that misses a piece of state
  shows as a difference in T8; T8 runs each model with everything that
  can be plugged in. If T8 cannot be made green within the step, the
  commit of §3.7 is split off and the column stays In progress (the
  rest of M-51 does not wait for it).
- **The feed's timing** follows `lineSettings()`; a ROM that changes the
  speed while a feed runs makes the character time change from then on,
  as on a real line.
- **`call`** relies on LIB-10 1600's `step-out` from M-45. If M-45 does
  not give "until the routine has returned" for a call that the
  harness made, `c1600.routine-call` also sets a breakpoint on the
  return address and removes it afterwards.

## 9. Decided (best guess)

- **D-1 (AC-5).** `tape record <path>` writes the WAV at the path, as
  `--tape-out` does today and as the user story asks ("a WAV file of my
  choice"); it writes no cassette definition.
- **D-2 (AC-4).** A view the model lacks is a done reply with the
  inspector's one line, not a refusal: the spec gives it as the reply.
- **D-3 (AC-2, AC-3).** The reply drops today's `---` heading lines;
  the pointer lines and the rows keep their form.
- **D-4 (AC-7).** The feed paces itself in the machine session's slice
  hook, in emulated ticks, so a run repeats exactly (LIB-11 AC-6);
  today's CLI pumped between slices of its own.
- **D-5 (AC-12).** `call` is the product kind that makes the call plus
  LIB-10's `step-out`, so the run control has one way to run until a
  return.
- **D-6 (AC-13).** The state's form is a product-internal binary of
  records; the header is the library's.

## 10. Questions for the user

None that split the design.
