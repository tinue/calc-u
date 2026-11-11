# LIB-10: Debugger core

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: LIB-3 (sessions, topics, operations, suspend), LIB-6 (the
  place for the steps), LIB-11 (the harness); also named here: LIB-1
  (debug targets are listed by the machine), LIB-9 (the trace folder and
  the size limit), LIB-14 and LIB-15 (where buttons are shown and files
  are handed over).
- Contracts and decisions: `docs/contracts/debugger.md` (the minimal
  agreement, DB-1 with its note, DB-3, DB-4, DB-6, I-1 to I-4, Q-1),
  `docs/contracts/presets.md` (PR-7); ADR 0006, 0009, 0023 as amended,
  0037; the open point "Traces" of `features/INDEX.md`.
- Facts: Calc-U-59 at `v2.3.0`, Calc-U-1600 at `b4ea8c8`; every
  `path:line` below is at those pins.

Glossary terms used: Debugger, Debug target, Run control, Level, Pause,
Trace, Debug panel. Words of this spec, of which breakpoint, watch and
history are glossary terms since (OQ-5):
- A **breakpoint** is an address of a debug target at which the run
  control pauses the machine before the instruction there is carried
  out.
- A **watch** is a range of cells of an address space; the run control
  pauses the machine after an instruction that read or wrote one of
  them.
- The **history** of a debug target is its last instructions, each with
  the registers as they were before it.
- A breakpoint or a watch is **hit** when the machine pauses for it. A
  **qualifier** narrows a breakpoint to a state of the target, such as
  the bank that is switched in. Pause-on-start is **armed** from the
  command until it fires. "Processor" stands for the CPU a debug target
  shows. A **program step boundary** is the moment between two program
  steps of a keystroke program (glossary, Program step).

What the three status columns mean for this feature:
- **Library:** the debug-target interface, the run control, its topic,
  its steps and commands, the trace session and the run-control buttons
  of the two component sets exist and are shown to work with a machine
  built for the tests.
- **Calc-U-59:** the TI machines offer their debug target, and every
  pause, step, breakpoint and trace of the product goes through the run
  control. The debug panel on top is C59-4.
- **Calc-U-1600:** the Sharp machines offer their debug targets, and
  every pause, step, breakpoint, watch and trace goes through the run
  control. The DAP server on top is C1600-3.

## User Stories
- As a user of Calc-U-59, I want to freeze the calculator, step through
  a keystroke program or through the ROM, and resume, as today.
- As a developer of a program for a Sharp machine, I want breakpoints,
  watches and stepping that behave the same whichever tool sets them.
- As the author of a preset, I want to pause, step and resume from a
  preset in every product, so that demos and screenshots can be
  scripted.
- As a developer or an agent, I want to read and write registers and
  memory, set a breakpoint and record a trace through the harness.

## Out of Scope
- The debug panel of Calc-U-59 with its tabs, FREEZE ON START buttons,
  LOG levels, heatmap and ASM overlay (owner: C59-4); the INSPECTOR of
  Calc-U-1600, which stays as it is, its TRACE button included, which
  only calls AC-23.
- The DAP server and everything it adds on top of this feature: source
  lines, listings and symbols, function breakpoints, conditions, hit
  counts and log messages, expressions, stepping by source line,
  `stopOnEntry`, Build & Load, the `debug:` block (owner: C1600-3).
- A BASIC level for the Sharp machines, and breakpoints in a GUI of
  Calc-U-59 (backlog; `debugger.md`).
- What a debug target's registers and cells mean (the product's core:
  C59-1, C1600-1).
- Product commands of the harness that only one product has (C59-9,
  C1600-5).

## Acceptance Criteria

### Library

Debug targets
- [ ] **AC-1** — Given a machine, then it lists one debug target for
  each processor it models (LIB-1), and each target states: its name;
  its registers, each with a name and a width in bits; its address
  spaces, each with a name, the width of a cell in bits and its size;
  the address space its instructions are in, which is the one a
  breakpoint's address means; and which of the optional parts it offers:
  watches, stepping over and out, decoding an instruction to text, a
  history, and a qualifier for breakpoints, with the names of its fields,
  the values each takes and the value each has now (`debugger.md` DB-1 and its note). A target
  is named as its descriptor names it (LIB-1 AC-36).
- [ ] **AC-2** — Given a target, then every register can be read, and
  written where the target says so; every cell of an address space can
  be read without any effect on the machine, or is reported as
  unreadable when reading it would disturb a device; a write is carried
  out or refused, and a refused write changes nothing (`debugger.md`
  I-2; Calc-U-1600 `Core/Debug/MachineDebugTargets.cpp:68-105`).
- [ ] **AC-3** — Given a target, then it gives the address of the
  instruction it will carry out next, and what it reads out between two
  instructions is the state before that instruction (`debugger.md`
  I-1).
- [ ] **AC-4** — Given a target with a history, then it gives its last
  instructions, the newest first, each with its address and the
  registers before it; how many it keeps is the target's.

Run control
- [ ] **AC-5** — Given a machine session, then it has one run control,
  whether or not anything uses it; the machine is either running or
  paused. While it is paused no emulated time passes: the display keeps
  its last picture, nothing is printed, and what waits for emulated time
  waits; a preset's `wait:` with a duration is the exception: what is
  left of it passes in the host's time, and the step ends with the
  machine still paused (LIB-6 AC-19). A paused machine has every target
  between two
  instructions. An advance of the machine that the run control stops
  ends there and gives the ticks it ran, which can be fewer than were
  asked for (LIB-1 AC-7).
- [ ] **AC-6** — Given the command pause with a level, then the machine
  pauses where the product says a pause of that level lands, and the
  reply comes when it has, with the run state; given resume, then the
  machine runs again, whichever level owned the pause. Every target has
  the level `cpu`; a product may declare the level `program`
  (`debugger.md`, minimal agreement, point 3).
- [ ] **AC-45** — Given a pause, then one level owns it: the level of
  the pause, step or pause-on-start that led to it, and `cpu` for a
  breakpoint or a watch. A step of another level is carried out, and
  that level owns the pause afterwards (Calc-U-59
  `Shared/SessionController.cpp:694-700`).
- [ ] **AC-7** — Given a paused machine and the command step with a
  level, a count and optionally a target, then the machine carries out
  that many steps of that level and is paused again, and the reply comes
  then, with the run state; while the steps are carried out the run
  state says running. One step of `cpu` is one instruction of the named
  target, or of the current target, which is the one the machine says
  will carry out the next instruction; a product may say that an
  instruction that only waits is carried out again until the processor
  moves on (AC-29). One step of `program` is what the product says. A
  breakpoint or a watch that is hit ends the steps there. Given the
  machine is not paused, then the command is refused.
- [ ] **AC-8** — Given a target that offers stepping over and out, then
  a step over carries out an instruction that calls a routine together
  with that routine, and a step out runs until the routine the target is
  in has returned; a breakpoint or a watch inside still pauses the
  machine (Calc-U-1600 `Core/Debug/RunControl.cpp:46-49, 92-100, 138-151`,
  `Core/tests/run_control_tests.cpp:159`).
- [ ] **AC-46** — Given the command to pause at the next start, then
  after the next reset, cold boot or build the machine is paused before
  its first instruction, with the reason entry; it holds once. This is
  not pause-on-start (`debugger.md`, minimal agreement: the two stay
  separate; C1600-3 builds `stopOnEntry` and `boot: debug` on it).
- [ ] **AC-9** — Given the command pause-on-start with a level, then the
  run control is armed for that level, and the machine pauses at the
  next event the product names for it (the next program step; the
  processor leaving key readiness). Arming one level disarms the other;
  a pause disarms both; the command with `off` disarms both; it fires
  once (Calc-U-59 `Shared/FreezeController.cpp:5-37`).
- [ ] **AC-10** — Given breakpoints, each an address of a target with,
  where the target offers it, a qualifier, when the target is about to
  carry out the instruction there and no operation runs, then the
  machine pauses before it. Given resume or a step from a breakpoint,
  then the instruction there is carried out without pausing again
  (`debugger.md` DB-3, I-3; Calc-U-1600
  `Core/tests/debug_target_tests.cpp:111-193`).
- [ ] **AC-47** — Given breakpoints and watches, then each belongs to
  the one that set it (the harness, a debugger that is attached) and has
  an ID the run control gave it; taking breakpoints away takes only
  one's own; a hit names the IDs and to whom they belong (AC-17).
- [ ] **AC-11** — Given watches on a target that offers them, each a
  range of an address space with the kind of access (read, write or
  both), when an instruction reads or writes a cell in the range as data
  and no operation runs, then the machine pauses after that instruction;
  fetching the instruction itself is no access (Calc-U-1600
  `Core/CPU/WatchSet.hpp:11-16`, `Core/tests/debug_target_tests.cpp:269-301`).
- [ ] **AC-12** — Given a breakpoint or a watch whose setter gave a test
  with it, when it is hit, then the run control asks the test and goes
  on without pausing when it says so (C1600-3 builds conditions, hit
  counts and log messages on this).
- [ ] **AC-13** — Given an operation runs, then breakpoints and watches
  do not pause the machine (`debugger.md` I-3); a pause, a step and a
  resume that are commanded, by a preset's own steps or from outside,
  are carried out, and a pause-on-start that is armed fires (AC-9), as
  in the preset `examples/screentest/test_cdebug.ti59` (Calc-U-59
  `tests/shared_cooperative_tests.cpp:849`,
  `Shared/FreezeController.cpp:24-28`).
- [ ] **AC-14** — Given a machine with several targets, then a pause
  stops all of them, and the run state names the current target
  (AC-7).
- [ ] **AC-15** — Given the machine is reset or cold booted while
  paused, then it runs again, and a pause-on-start that is armed stays
  armed. Given a new machine is built, then it is not paused, nothing is
  armed, and the breakpoints and watches that were set are set again on
  the targets of the same name; those whose target is gone are dropped
  (Calc-U-59 `Shared/SessionController.cpp:156-158`,
  `tests/shared_controller_tests.cpp:119-132`; Calc-U-1600
  `Qt6/app/debug/DebugController.cpp:160-169, 280-297`).
- [ ] **AC-16** — Given a paused machine is suspended and continued,
  then it is still paused (LIB-3 AC-25). Given resume, then every
  real-time clock of the machine is handed the host's time as at the end
  of an operation (LIB-2 AC-23), and pacing starts afresh, so that the
  time of the pause is not made up (LIB-3 AC-23).

The run state
- [ ] **AC-17** — Given a machine session, then the run state is a state
  topic named `run-state` that says: running or paused; when paused, the
  level that owns the pause, the reason (a pause, a step, a breakpoint,
  a watch, a pause-on-start, entry), the current target, and for a hit
  the IDs of the breakpoints or watches and whose they are; and the
  level that is armed, if one is.
- [ ] **AC-18** — Given the component sets, then each has three
  run-control buttons for a level: pause, step and resume. Pause can be
  used while the machine runs; step and resume while it is paused and
  that level owns the pause. Step repeats while it is held, four times a
  second. Their labels and their place are the product's
  (`debugger.md` DB-6; Calc-U-59 `App/Views/HoldRepeatButton.swift:11-12`).

Steps and commands
- [ ] **AC-19** — Given a preset, then these steps exist in every
  product (`presets.md` PR-7); none of them needs emulated time before
  it is carried out (LIB-6 AC-19):

  | Step | Value | What it does |
  |---|---|---|
  | `pause:` | a level | AC-6 |
  | `resume:` | none | AC-6 |
  | `step:` | a level, then optionally a count from 1 to 100000 (1 without), then optionally a target | AC-7; while the machine is not paused it does nothing but a line in the log (OQ-4) |
  | `pause-on-start:` | a level, or `off` | AC-9 |
  | `debug-panel:` | the name of a view of the product's debug panel | the frontend is told to show that view; without a frontend nothing is shown |
  | `trace:` | a file name, or `off` | AC-22 |

  A level the model does not have, a view the product does not have and
  a count out of range refuse the preset when it is read (Calc-U-59
  `Shared/StateFile.cpp:269-310`).
- [ ] **AC-20** — Given the harness, then the steps of AC-19 exist as
  commands with the same names and values, and also these:

  | Command | Reply |
  |---|---|
  | `targets` | each target with what AC-1 says it states |
  | `registers [<target>]` | every register with its value |
  | `set-register <target> <register> <value>` | done, or refused |
  | `peek <target> <space> <address> [<count>]` | the cells, an unreadable one marked |
  | `poke <target> <space> <address> <value>…` | done, or refused at the first cell that cannot be written |
  | `break <target> <address> [<qualifier>]`, `break off [<target> <address>]`, `break list` | done; the list |
  | `watch <target> <space> <address> <count> read\|write\|both`, `watch off`, `watch list` | done, or refused when the target offers no watches |
  | `history [<target>] [<count>]` | the last instructions as AC-4 gives them, with their text where the target decodes |
  | `disassemble <target> <address> [<count>]` | the instructions as text; refused when the target does not decode |
  | `profile on`, `profile off`, `profile report [<count>]` | while on, counts how often each address of each target was carried out; the report lists the most frequent ones and how many different addresses there were |
  | `step-over [<target>]`, `step-out [<target>]` | AC-8; the reply as for `step`; refused when the target does not offer them |
  | `pause-at-start` | AC-46 |

  A qualifier is written as its fields, `name=value` each. `break off`
  and `watch off` take away the harness's own only (AC-47). The run
  state and the trace state are read with `state run-state` and `state
  trace` (LIB-11 AC-9). A `step` of the harness on a machine that is not
  paused is refused.
  (LIB-11 AC-14, AC-18; today Calc-U-59 `tools/headless/ti59.py:178-325`,
  Calc-U-1600 `tools/CliCommon.hpp:98-148`, `tools/pc1500_cli.cpp:228-333`.)
- [ ] **AC-21** — Given the harness command `wait` with a duration, when
  the machine pauses before the duration is over, then the wait ends
  there and its reply gives the time that passed and the run state;
  given the machine is paused when the command is given, then it ends
  at once, with no time passed (LIB-6 AC-19).

Traces
- [ ] **AC-22** — Given `trace:` with a file name, in a preset or as a
  command, then every instruction the machine carries out from then on
  is recorded in a trace file of that name in the trace folder (LIB-9
  AC-12; per frontend AC-17, AC-24 and OQ-7 there), in the product's
  trace format, replacing a file of that name, until `trace: off`, the end of
  the preset that started it, or a new machine. A name with `/` or `\`
  is refused when the preset is read. Starting a trace ends one that is
  running, and given the file cannot be made, then the step fails (LIB-6
  AC-17) (both OQ-3) (Calc-U-1600 `Core/Preset/PresetRunner.cpp:102-122, 308-315`;
  Calc-U-59 `Shared/EmulatorSession.cpp:269-287, 335-347`).
- [ ] **AC-23** — Given a product's frontend starts a trace by a button,
  then it is the same trace under the file name the product gives, and
  each start replaces the file of that name (both products today).
- [ ] **AC-24** — Given a trace file reaches the size of
  `[trace] max-file-size-mb` (LIB-9 AC-11), then the trace ends, the
  file is complete, and a notice says so (OQ-3). Given a trace ends because a
  new machine was built, then a line of the log says so.
- [ ] **AC-25** — Given a trace runs, then a state topic named `trace`
  says so, with the file name, so that a frontend shows it whoever
  started it.
- [ ] **AC-26** — Given a Web frontend that has traces (Calc-U-59's;
  LIB-15 keeps Calc-U-1600's Web frontend without them), then a trace
  file that has ended is handed to the user to keep (LIB-15 AC-9;
  Calc-U-59 `Web/bindings.cpp:431-435`).

### Calc-U-59

- [ ] **AC-28** — Given a TI machine, then it has one debug target for
  its processor, with the registers of today's processor frame, the
  address spaces ROM (13-bit words), RAM registers (16 digits each),
  SCOM and program steps, a history and decoding; it offers no watches
  and no stepping over or out (`debugger.md`, fit table;
  `reference/DebugAPI.md:23-43, 251-292, 315-319`).
- [ ] **AC-29** — Given a TI machine, then it has the levels `cpu` and
  `program`. A pause of either level lands at the next program step
  boundary, or, while none comes because the calculator waits for a key,
  after 50000 instructions as the ROM's idle state counts them, as today
  (`Core/TI59Machine.hpp:124-129`). One step of `program` runs to the next program
  step boundary, one program byte as SST does, also through the programs
  of the ROM and of a module; one step of `cpu` is one instruction,
  carried out again, up to 32 times, while the processor stays at the
  same address (`Shared/EmulatorSession.cpp:115-124, 173-194`;
  `reference/DebugAPI.md:99, 136-139`).
- [ ] **AC-30** — Given pause-on-start on a TI machine, then `program`
  fires at the next program step boundary, and `cpu` fires when the
  processor leaves key readiness after it was seen there, or starts
  again at address 0000. A Reset of the TI-59 or TI-58 does not fire
  `cpu`; a Cold boot does, and so does a Reset of the TI-58C
  (`Shared/FreezeController.cpp:5-37`,
  `tests/shared_controller_tests.cpp:154-169`).
- [ ] **AC-31** — Given a breakpoint is set through the harness, then a
  TI machine in an app session pauses at it like at a pause (AC-10);
  today the session ignores breakpoints. No frontend of Calc-U-59 sets
  one (`debugger.md` DB-3).
- [ ] **AC-32** — Given the presets of Calc-U-59, then `pause: program`
  and `pause: cpu` are today's `Freeze: Calculator` and `Freeze: CPU`,
  `pause-on-start:` today's `FreezeOnStart:`, `step:` today's `Step:`,
  `resume:` today's `Resume:`, and `debug-panel:` takes `calculator`,
  `cpu` or `log` (`Shared/SessionController.cpp:668-705`).
- [ ] **AC-33** — Given a trace of a TI machine, then its file is in the
  format of today, version 2: the header with `TI59`, the version and
  the model; the records for the start and the end of a session, for an
  instruction with its processor frame, for a key and a card event, and
  for a gap; a trace started by the button is named `CALCU59_TRACE.bin`,
  `CALCU58_TRACE.bin` or `CALCU58C_TRACE.bin` after the model
  (`reference/DebugAPI.md:417-493`, `Shared/TraceWriter.cpp:22, 44-50`).
  `tools/read_trace.py` stays a script and reads it.
- [ ] **AC-34** — Given the Swift, Qt6 and Web frontends, then FREEZE,
  STEP and RESUME of each tab of the debug panel are the run-control
  buttons of AC-18 for `program` and for `cpu` (the panel: C59-4).
- [ ] **AC-35** — Given what the tests of Calc-U-59 check today about
  freezing, stepping and traces, then it holds
  (`tests/shared_runtime_tests.cpp:73-401`,
  `tests/shared_controller_tests.cpp:119-209`,
  `tests/shared_debug_tests.cpp:78-128`, `tests/shared_trace_tests.cpp:18-70`,
  `tests/shared_cooperative_tests.cpp:849`).

### Calc-U-1600

- [ ] **AC-36** — Given a PC-1500 or PC-1500A, then it has one debug
  target, the LH5801; given a PC-1600, then two, the SC7852, its Z-80,
  and the LH5803, named as C1600-1 names them. Each has the registers of today, the address space of its
  memory and, for the LH5801 and LH5803, a second one for ME1; each
  offers watches, stepping over and out, decoding and a history of 32
  instructions; a breakpoint's qualifier names a bank or the state of
  PU, PV and ME (`Core/Debug/DebugTarget.hpp:30-72`,
  `Core/Debug/CpuRegisters.cpp:34-227`, `Core/CPU/LH5801/LH5801.hpp:189`).
- [ ] **AC-37** — Given the Sharp targets, then reading and writing is
  as today: ROM, cells where nothing is and I/O refuse a write; ME1 is
  never written; a cell of ME1 whose reading would disturb a device is
  unreadable; on the PC-1600 the LH5803's memory is written only where
  it is RAM (`Core/Debug/MachineDebugTargets.cpp:68-105`).
- [ ] **AC-38** — Given a Sharp machine, then it has the level `cpu`
  only; a pause lands before the next instruction. `pause-on-start: cpu`
  fires when the processor leaves key readiness: on the PC-1600 as it is
  found today, on the PC-1500 and PC-1500A once its key handling has
  been researched (`debugger.md` Q-1; C1600-1 AC-21).
- [ ] **AC-39** — Given a PC-1600, then a step of one target runs the
  machine until that target has carried out an instruction, the other
  target running meanwhile when it has the bus; after a resume only the
  target that is about to run passes the breakpoint it stands on
  (`Core/Debug/RunControl.cpp:88-89, 145-148`,
  `Core/tests/debug_target_tests.cpp:464`).
- [ ] **AC-40** — Given Calc-U-1600, then the run control is there in
  the Qt6 and in the Web frontend, with or without a debugger attached,
  and a preset's `pause:`, `step:` and `resume:` work in both;
  `debug-panel:` is refused when a preset is read, because Calc-U-1600
  has no views to choose, and `trace:` fails in the Web frontend (AC-26;
  EC-8). Today the run control
  exists only in the Qt6 frontend while a debugger is attached
  (`Qt6/app/debug/DebugController.cpp:156`,
  `Shared/MachineSession.cpp:450-453`; `presets.md` PR-7).
- [ ] **AC-41** — Given a paused machine, then the Qt6 window's title
  says "Paused (debugger)" as today, and both frontends offer Machine ▸ Resume
  (OQ-2) (`Qt6/app/MainWindow.cpp:251-255`).
- [ ] **AC-42** — Given a trace of a Sharp machine, then its file is in
  the format of today, version 3: the header with `PC15` and the
  version; the records for the start and the end of a session, for an
  instruction of the LH5801 or LH5803, for an instruction of the Z-80,
  and for a gap, with the instructions of both processors of a PC-1600
  in one file; a trace started by the INSPECTOR's button is named
  `TRACE.bin` (`Core/PC1500/PC1500TraceFile.cpp:9-144`).
  `tools/read_trace.py` stays a script and reads it.
- [ ] **AC-43** — Given a trace started by a preset, then the size limit
  of AC-24 holds for it too; today only the button's trace is limited
  (`Qt6/app/DebugPanel.cpp:251-272`).
- [ ] **AC-44** — Given what the tests of Calc-U-1600 check today about
  targets, breakpoints, watches, stepping and traces without source
  lines, conditions or expressions, then it holds
  (`Core/tests/debug_target_tests.cpp:111-613`,
  `Core/tests/run_control_tests.cpp:159-243`,
  `Core/tests/presetloader_trace_tests.cpp:107-289`).

## Edge Cases
- **EC-1** — Given a pause is commanded while the machine is paused,
  then nothing changes and the reply is done.
- **EC-2** — Given `resume` while the machine runs, then nothing changes
  and the reply is done.
- **EC-3** — Given a watch whose range runs past the end of its address
  space, then it is refused (today it is taken and never hit,
  `Core/CPU/WatchSet.hpp:40-43`).
- **EC-4** — Given several watches are hit by one instruction, then the
  machine pauses once, after the instruction, and the run state names
  the one with the lowest address.
- **EC-5** — Given a breakpoint on the instruction a paused machine
  stands on, then a step carries that instruction out (AC-10).
- **EC-6** — Given a `step` with a large count, then the machine can be
  paused, reset or replaced before the count is reached, and the step
  ends there.
- **EC-7** — Given a preset ends while the machine is paused by one of
  its steps, then the machine stays paused and the load is over.
- **EC-8** — Given `trace:` in a preset on a frontend without a trace
  folder that can be written, then the step fails with the reason
  (AC-22).
- **EC-9** — Given `peek` or `poke` with an address outside the space,
  then it is refused.

## Technical Requirements (optional)
- The run control runs where the session runs; it takes no lock
  (`debugger.md` I-4; ADR 0009).
- The library's run control and debug-target interface name no
  processor and no model (`debugger.md` DB-1).
- The debug panel is not in the component sets; only the three buttons
  of AC-18 are (`debugger.md` DB-6).
- A trace file is an own format of its product (ADR 0037); neither
  format changes in the migration, so neither gets a new version. The
  product supplies what a file holds, its header and its records, those
  for events only it knows (a key, a magnetic card) included; the
  library starts, limits and ends the trace (OQ-1).

## Open Questions
- [x] **OQ-1** — *Decided as recommended (user, 2026-11-10, batch 6 gate).* Traces (open point of `features/INDEX.md`): is starting,
  limiting and ending a trace library code? *Recommendation:* yes, as
  AC-22 to AC-27 say: one trace session in the library for the step, the
  command and the buttons, and the file's content per product. No
  product row is then needed for traces; the two formats are stated in
  this spec (AC-33, AC-42).
- [x] **OQ-2** — *Decided as recommended (user, 2026-11-10, batch 6 gate).* The way out of a pause in Calc-U-1600 (AC-41).
  `debugger.md` keeps Calc-U-1600 without an in-app stepping debugger,
  but a preset can now pause the machine in both frontends, and without
  a debugger attached only Reset would end it. *Recommendation:* the
  Machine menu of the family gets the item Resume, which can be used
  while the machine is paused, in every product (LIB-14 AC-1); the
  INSPECTOR stays as it is.
- [x] **OQ-3** — *Decided as recommended (user, 2026-11-10, batch 6 gate).* Trace rules where the products differ today.
  *Recommendation:* one trace at a time (Calc-U-59 can run a button's
  and a preset's trace side by side); the size limit for every trace
  (AC-43); a notice when the limit ends a trace (Calc-U-59 tells the
  user only for the button's trace, Calc-U-1600 writes a line into the
  INSPECTOR); a trace that cannot be started fails its preset
  (Calc-U-59 shows an alert and goes on). One row in
  `docs/behaviour-changes.md` for each product.
- [x] **OQ-4** — *Decided as recommended (user, 2026-11-10, batch 6 gate).* A preset's `step:` while the machine is not paused
  (AC-19). Today Calc-U-59 writes a line into the log and goes on.
  *Recommendation:* keep that for presets; the harness command is
  refused.
- [x] **OQ-5** — *Decided as recommended (user, 2026-11-10, batch 6 gate).* "Breakpoint", "watch" and "history" for the glossary,
  as defined above.

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| One debug-target interface in the library, generalised from Calc-U-1600's: address spaces with their cell width, registers of any width; a target states which optional parts it offers | `debugger.md` DB-1 and its note | 2026-11-10 |
| One run control on the session's thread: pause, resume, step, breakpoints and watches; snapshots are pre-execution | `debugger.md`, minimal agreement, I-1, I-4 | 2026-11-10 |
| Named levels: `cpu` on every target, `program` where a product declares it; the product says what one step is | `debugger.md`, minimal agreement, point 3 | 2026-11-10 |
| Breakpoints and watches are checked whenever some are set and no operation runs; Calc-U-59's session no longer ignores them; Calc-U-59 gets no breakpoint GUI | `debugger.md` DB-3, I-3 | 2026-11-10 |
| The debug panel is per product; the component sets share only the pause, step and resume buttons | `debugger.md` DB-6; ADR 0023 as amended | 2026-11-10 |
| The debugger steps work in both products | `presets.md` PR-7 (user) | 2026-11-10 |
| Trace file formats stay per product | `debugger.md`, non-responsibilities | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 6 gate): conditions, hit counts and log messages are tests of the setter (AC-12) and belong to C1600-3, not to the targets | `debugger.md` DB-1 lists them as optional per target; only DAP uses them | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 6 gate): the commands of AC-20 and their names, `profile` among them | LIB-11 AC-14 and AC-18 assign these functions of today's tools to this feature | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 6 gate): `tools/read_trace.py` of each product stays a script | The formats do not change; LIB-11 OQ-5 kept other one-purpose tools as scripts | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 6 gate): on resume a real-time clock is handed the host's time (AC-16); today the Sharp clocks fall behind after a pause | LIB-2 AC-23 does the same at the end of an operation | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 6 gate): a watch past the end of its space is refused (EC-3) | Today it is accepted and never hit | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 6 gate): a step of another level than the one that owns the pause is carried out and takes the pause over (AC-45) | A preset's `Step:` does so today; only the buttons follow the owner | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 6 gate): a product may let a `cpu` step carry out an instruction that only waits again (AC-7, AC-29) | The TI step does so today; the minimal agreement leaves only the `program` step to the product | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 6 gate): pausing before the first instruction after a start (AC-46), and breakpoints that belong to the one that set them (AC-47) | C1600-3 needs both for `stopOnEntry` and for a debugger next to the harness | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 6 gate): after a resume pacing starts afresh (AC-16) | Otherwise regular speed would race to make up the pause | 2026-11-10 |
| A pause-on-start that is armed fires while an operation runs (AC-13); AC-9 never excepted an operation, and AC-13's list left it out | Phase 5, preset pass (`docs/walkthroughs/presets.md`, A-13); LIB-6 AC-19 and its decision of 2026-11-11 rest on the preset `test_cdebug.ti59`, which arms and fires inside its own load | 2026-11-11 |
