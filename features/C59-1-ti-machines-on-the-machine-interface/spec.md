# C59-1: TI machines on the machine interface

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: LIB-1 (the machine interface; its Calc-U-59 column holds the
  descriptors' content).
- Contracts and decisions: `docs/contracts/machine.md` (M-1 to M-7, I-1
  to I-6, fit table), `docs/contracts/capabilities.md` (product kinds of
  Calc-U-59); ADR 0006, 0009, 0012; `docs/behaviour-changes.md` rows
  59-11, 59-13.
- Facts: Calc-U-59 at `v2.3.0`; every `path:line` below is at that tag.

In this spec "the calculator" is the real device; the running thing is
"the machine" (glossary).

## User Stories
- As a user of Calc-U-59, I want the TI-58, TI-58C and TI-59 to behave
  exactly as today after the rebuild: the same ROM, the same display,
  the same timing.
- As the developer, I want the TI machines reached only through the
  machine interface, so that the library's sessions, typing, presets,
  debugger and harness work for them without knowing a TI.
- As the author of a preset or a harness script, I want to name a TI key
  by what is printed on it, not by a matrix code.
- As the developer, I want to refine the TI model later (single chips, so
  that `examples/assembly/Decoder.asm` runs) without touching anything
  above the machine.

## Out of Scope
- The PC-100C and the Solid State ROM modules as peripherals, and the
  printer's paper (owner: C59-6; the paper: LIB-8).
- The card reader as a media place, cards and cue cards (owner: C59-7).
- Writing program steps, registers and the partition (the keystroke
  program memory product kind; owner: C59-2).
- The `ti58c.mem` file, and handing a newly built TI-58C its constant
  memory (owner: C59-3). This feature says what the machine keeps.
- The debug target, the program level, the ASM overlay, the ROM heatmap
  and the trace (owners: LIB-10, C59-4).
- Key readiness and how a key is held (owner: LIB-5); the pacing profile
  (LIB-17).
- Other names for a key in presets and in the harness, such as the
  second functions (owner: LIB-5, AC-26 there).
- The descriptors' content: models, tick rates, display kind, ROM files,
  places (owner: LIB-1, Calc-U-59 column).
- Which operation follows a model switch, an attach or the user's Reset
  and Cold boot, and building a machine with its peripherals (owner:
  LIB-18).
- Glyph styles (Classic, Modernized) and drawing the display (the
  frontends).
- Today's headless tools on the new interface (owner: LIB-11).
- Making `Decoder.asm` run, and modelling single chips (`Briefing.md`
  §2).

## Acceptance Criteria

Emulated time
- [ ] **AC-1** — Given a TI machine, then one instruction takes 32
  ticks; on the TI-59 and the TI-58 an instruction executed in the ROM's
  idle state takes 128 ticks; on the TI-58C every instruction takes 32
  (`Core/MachineVariant.hpp:29-39`, `Core/TMC0501.cpp:283-285`).
- [ ] **AC-2** — Given a TI machine advanced by any number of ticks
  above zero, then it overshoots by less than one instruction (at most
  127 ticks), and reports what it ran (LIB-1 AC-7).
- [ ] **AC-3** — Given a TI machine, then its tick count goes on across
  a reset, a power cycle and a cold boot (LIB-1 AC-8). Today the count
  starts again at every reset (`Core/TMC0501.cpp:156-157`).

Behaviour unchanged
- [ ] **AC-4** — Given a golden run (Technical Requirements), when its
  script is played on the machine through the machine interface, then
  the display output at each recorded moment and the content of the
  memory at the end are the recorded ones, for each of the three models.
- [ ] **AC-5** — Given what `tests/core_tests.cpp` checks today, then it
  holds through the machine interface for all three models: the machine
  reaches the state in which it takes keys; 2 + 3 = 5; the glyphs of the
  control nibbles; the period of the error blink per model; the
  normalised display duty; the TI-58C keeping its memory; the ROM's
  start-up clearing the memory on the TI-59 and TI-58 only
  (`tests/core_tests.cpp`, 17 tests at the pin).

Reset, power cycle, cold boot
- [ ] **AC-6** — Given a TI machine, then the three operations leave it
  as follows once it reports key readiness again (LIB-1 AC-11; today the
  session runs a fixed 300,000 instructions instead,
  `Shared/StateApplier.cpp:12-18`):

  | | TI-59, TI-58 | TI-58C |
  |---|---|---|
  | Reset | the content of its RAM kept (program and data registers), and the partition; everything else as after switching on | the same as a power cycle |
  | Power cycle | as the ROM's start-up leaves a machine whose memory was lost | the constant memory kept (all of its RAM, including the registers H00 to H03); everything else as after switching on |
  | Cold boot | as the ROM's start-up leaves a machine whose memory was all zero | the same, the constant memory included |

  (Glossary: Reset, Power cycle, Cold boot. Today: Reset is
  `Shared/StateApplier.cpp:95-107`, which saves the RAM, resets, runs the
  start-up and writes the RAM back, because the ROM's start-up clears it
  on the TI-59 and TI-58; the clean reset is `:109-115`. A power cycle
  that loses the memory of the TI-59 and TI-58 is new: behaviour change
  59-11. The ROM's reset routine sets the partition up anew, so a Reset
  saves the partition and puts it back; today, as read from the code and
  covered by no test, it goes back to the ROM's default: behaviour
  change 59-22; user, 2026-11-11; OQ-5.)
- [ ] **AC-7** — Given a newly built TI machine, then it is in the state
  after a cold boot, unless it was handed content at its build (the
  constant memory of a TI-58C, C59-3).
- [ ] **AC-8** — Given a TI machine after any of the three operations,
  then no key is down, and once it reports key readiness the display
  output is that of a machine just switched on.

Keys
- [ ] **AC-9** — Given a TI machine, then its 45 keys have these key
  IDs, by row of the keyboard from top to bottom:

  | Row | Key IDs, left to right |
  |---|---|
  | 1 | `A` `B` `C` `D` `E` |
  | 2 | `2nd` `INV` `lnx` `CE` `CLR` |
  | 3 | `LRN` `x⇄t` `x²` `√x` `1/x` |
  | 4 | `SST` `STO` `RCL` `SUM` `yˣ` |
  | 5 | `BST` `EE` `(` `)` `÷` |
  | 6 | `GTO` `7` `8` `9` `×` |
  | 7 | `SBR` `4` `5` `6` `-` |
  | 8 | `RST` `1` `2` `3` `+` |
  | 9 | `R/S` `0` `.` `+/-` `=` |

  (`Shared/KeyboardLayout.hpp:55-75`.) The IDs, and the other
  spellings the descriptor lists for them (`x<>t`, `x^2`, `sqrt`, `y^x`,
  `/`, `*` and those of the table), are those of
  `docs/formats/preset.md` §8.1; the names of today's
  `tools/headless/ti59.py:32-54` that it lacks (`x2`) go.
- [ ] **AC-10** — Given a key ID of AC-9, when key down is called, then
  the ROM sees the key of that row and column of the keyboard: the key
  in row r, column c is the one today's matrix code r × 10 + c presses
  (`Shared/KeyMatrix.hpp:5-33`).
- [ ] **AC-11** — Given two keys held down together, then both are in
  the key matrix and the ROM decides what happens (LIB-1 AC-13;
  behaviour change 59-13).
- [ ] **AC-12** — Given a TI machine, then PRINT, ADV and TRACE are not
  among its keys (C59-6).

Display
- [ ] **AC-13** — Given a TI machine, then its display output has 12
  positions, the rightmost first, and per position: the lit segments out
  of A (top), B (upper right), C (lower right), D (bottom), E (lower
  left), F (upper left), G (middle) and the decimal point; a brightness
  from 0 to 1 for the position and a separate one for its decimal point
  (`Core/TMC0501.hpp:36-49`, `Shared/SevenSegment.hpp:10-11`; LIB-1
  AC-15). A brightness is the share of the time since the display output
  was last read in which the position was driven (OQ-7).
- [ ] **AC-14** — Given what a position shows, then the segments follow
  today's rules: a digit; the signs for degree, minus, quote and
  apostrophe; or blank, with zero suppression as today
  (`Shared/SevenSegment.hpp:15-52`, `Core/TMC0501.cpp:1428-1465`).
- [ ] **AC-15** — Given the indicator C and the leftmost position, then
  the display output gives each its own state and brightness, so that C
  and a minus sign can be lit together, each as bright as the hardware
  drives it. Given a running program, then the positions are dark
  and the indicator C is on; given the
  machine waits for a key, then the positions show the number
  (`reference/CoreArchitecture.md:504`, `Core/TMC0501.cpp:296-305`).
- [ ] **AC-16** — Given a TI machine, then it has no sound output, and
  its display is always on (LIB-1 AC-43).

Key readiness
- [ ] **AC-18** — Given a TI machine, then it reports key readiness
  (LIB-1 AC-42) whenever its ROM is ready to accept a key press, on
  every model and with or without a PC-100C. Today this is found from
  ROM address ranges that differ per model and with a printer present
  (`Shared/MachineModelInfo.hpp:59-105`); the spec does not name them,
  because there are several such loops. Key readiness is the steady
  state of that keyboard scan loop (with a printer, its confirmation
  scan), as 2.3.0 finds it; the moments in which a running program
  samples R/S or GTO are not key readiness. A key meant to reach a
  running program would have to be held down until the ROM samples it,
  which no preset step does in this release (a `key-down` step would).
- [ ] **AC-19** — Given a Reset on the TI-59 or TI-58, whose ROM clears
  the memory and sets the partition up anew in its start-up, then the
  machine itself puts the content of the RAM and the partition back
  before it reports key readiness (AC-6).

One seam
- [ ] **AC-17** — Given Calc-U-59 after this feature, then its app layer
  reaches the machine's model, its advances and tick count, its reset,
  power cycle and cold boot, its keys and its display output through the
  machine interface only.

## Edge Cases
- **EC-1** — Given a machine in the ROM's idle state on the TI-59 or
  TI-58, when it is advanced by 32 ticks, then it runs one instruction
  of 128 ticks and reports 128.
- **EC-2** — Given the display output is read twice with no advance in
  between, then both readings are the same (`Core/TMC0501.cpp:227`).
- **EC-3** — Given a key ID that is not in AC-9, then key down is
  refused (LIB-1 EC-2); today an invalid matrix code is ignored.
- **EC-4** — Given the TI-58 or the TI-58C, which have no card reader,
  then the ROM still finds its card switch line as today
  (`Core/TI59Machine.cpp:14-23, 154-156`).

## Technical Requirements (optional)
- **Golden runs.** A golden run is a script of calls on the machine:
  key down and key up with a key of AC-9, each at a tick count, and
  advances. It is recorded in phase 6 from the Calc-U-59 code as it is
  imported, before the core is changed, by a driver that calls the old
  machine directly. The golden file holds the script, the display output
  at moments the script names, and at the end the content of the RAM
  and the registers, read by the test inside the core. A comparison is
  exact. The runs cover at least, for each model: switching on;
  2 + 3 = 5; keying in and running a program of a few steps; an error
  blink; a reset, a power cycle and a cold boot after memory was filled.
- What the app layer still calls on the core after this feature, and
  which row takes it behind the seam:

  | Today (callers in `Shared/`) | Row |
  |---|---|
  | printer calls, `loadLibrary` | C59-6 |
  | `insertCard`, `cardEject`, `isCardPresent`, `isWaitingForCard` | C59-7 |
  | RAM, registers, program steps, partition | C59-2; the constant memory: C59-3 |
  | `stepUntilNextKeycode`, `beginNextStep`, `snapshotCPU`, `disassemble`, breakpoints, trace, debug overlay | LIB-10, C59-4 |
  | the scan-loop and key-detect ranges, "polled hardware" | key readiness (AC-18); needing a card: C59-7 |

  When this row and those rows are done, nothing above the core uses a
  core type (`machine.md` I-1). The core types used outside `Core/`
  today are: the display snapshot (`Shared/DisplayPov.hpp:2`,
  `Shared/FrameSampler.hpp:25`), the step boundary
  (`Shared/DebugSnapshots.hpp:127`), the printer's code line
  (`Shared/PrinterFont.hpp:2`, `Qt6/app/PrinterWidget.cpp:39`,
  `Web/bindings.cpp:659`), the model list and instruction rate
  (`Shared/MachineModelInfo.hpp:2`), the CPU trace frame and debug event
  (`Shared/TraceWriter.hpp:2`, `Bridge/BridgeConvert.h:5`,
  `Web/bindings.cpp:764`, `Shared/FrameSampler.cpp:19`).
- The machine holds no lock and starts no thread (ADR 0009). The core's
  four mutexes of today (`reference/CoreArchitecture.md:834-850`) have no
  successor.
- The descriptors' depth entries say what is folded into the CPU
  component after this feature and after C59-6 (ADR 0012).

## Open Questions
- [x] **OQ-4** — *Decided as recommended (user, 2026-11-11).* Where the persistence of vision lives. The machine
  gives the duty per position (AC-13). Today Calc-U-59 turns it into
  what the eye sees, with hold, fade and a perceptual curve, in its app
  layer for all three frontends (`Shared/DisplayPov.hpp:6-73`,
  `Shared/FrameSampler.cpp:56-69`). LIB-3 makes the display topic "the
  machine's display output". *Recommendation:* the code stays in the
  Calc-U-59 app layer, once for all frontends, and its result is what
  the frontends draw; whether it travels as the display topic or as a
  product topic is decided with LIB-3.
- [x] **OQ-5** — *Decided (user, 2026-11-11): the partition is set up by the ROM's reset routine, so Reset must back it up and restore it: a Reset keeps the partition (AC-6). The first task records on 2.3.0 what today's Reset does.* Reset and the partition on the TI-59 and TI-58. Today's
  Reset keeps the RAM but not the partition, which goes back to the
  ROM's default; this was read from the code and is covered by no test.
  *Recommendation:* verify on 2.3.0 first; if true, keep it and say so
  in AC-6.
- [x] **OQ-6** — *Decided (user, 2026-11-10, batch 5 gate): from the running machine. Reset keeps the memory and the fitted cue card and loses everything else (the SCOM is reset, a running program stops, a selected module program is no longer selected).* TI-58C: Reset equals a power cycle (glossary), and the
  constant memory then comes from the running machine. Today Reset reads
  it back from `ti58c.mem`, which can be up to half a second old
  (`Shared/SessionController.cpp:186, 258-266`). *Recommendation:* from
  the running machine; nobody will see the difference.
- [x] **OQ-7** — *Decided as recommended (user, 2026-11-11): only one GUI per core is allowed.* The stretch a brightness is measured over (AC-13).
  Today it is "since the last read", so a second reader would disturb
  the first. *Recommendation:* keep it, and let the machine session be
  the only reader, once per call of its host; the harness and a second
  listener get the display from the session.

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| The machine is the only seam; this holds for Calc-U-59's own app layer too | ADR 0006; `machine.md` I-1 | 2026-11-09 |
| The tick of a TI machine is the resonator period: 455, 384 and 394 kHz | `machine.md` M-1, fit table | 2026-11-10 |
| Keys are key IDs; the matrix stays in the core | `machine.md` M-3 | 2026-11-10 |
| A segment display with a segment layout, a brightness per position and the indicator C | `machine.md` M-2 | 2026-11-10 |
| A power cycle loses the memory of the TI-59 and TI-58 and keeps the constant memory of the TI-58C | Glossary; `peripherals.md` P-1; behaviour change 59-11 | 2026-11-10 |
| Component boundaries follow observable behaviour; depth is recorded in the descriptor | ADR 0012 | 2026-11-09 |
| The key IDs are written in this spec, not left to phase 6 | LIB-1 decision log | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-11): a newly built machine is in the state after a cold boot (AC-7); the key IDs of AC-9; what a golden run is and what the runs cover | Today a new machine and a reset one differ in one CPU flag (`Shared/EmulatorSession.cpp:16-35`); no contract lists the IDs or defines the golden runs | 2026-11-10 |
| Key readiness is reported by the machine; a Reset, power cycle or cold boot is over when it is reported again | user, 2026-11-10, batch 2 gate | 2026-11-10 |
| The machine takes the 45 key IDs of AC-9 exactly. In presets and in the harness a second-function name (`Pgm`) stands for 2nd and its key, and a `2nd` written before it is not pressed twice; names are matched without regard to case (specified in LIB-5, AC-26 there) | user, 2026-11-10, batch 2 gate | 2026-11-10 |
| C and the minus sign can be lit together, each with its own brightness as on the hardware; the frontends show at least what they show today | user, 2026-11-10, batch 2 gate: today both show, but not with independent brightness | 2026-11-10 |
| A Reset keeps the partition on the TI-59 and TI-58 (AC-6) | user, 2026-11-11; behaviour change 59-22 | 2026-11-11 |
| The persistence of vision stays in the Calc-U-59 app layer, once for all frontends; the machine session is the only reader of the display output | user, 2026-11-11 | 2026-11-11 |
| The key IDs and their other spellings are those of `preset.md` §8.1 (AC-9); `x2` goes. Replaces the names of `tools/headless/ti59.py` | user, 2026-11-13 (question F-5 of `docs/walkthroughs/presets.md` §5.4); `docs/formats/preset.md` §8.1 | 2026-11-13 |
| Key readiness on a TI is the steady state of the keyboard scan loop, not a running program's sampling of R/S or GTO (AC-18) | user, 2026-11-13 (B-3 of `docs/walkthroughs/presets.md` §5.2) | 2026-11-13 |
