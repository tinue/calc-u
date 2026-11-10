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
- The `ti58c.mem` file (owner: C59-3). This feature says what the
  machine keeps; C59-3 says what is stored.
- The debug target, the program level, the ASM overlay, the ROM heatmap
  and the trace (owners: LIB-10, C59-4).
- Key readiness and how a key is held (owner: LIB-5); the pacing profile
  (LIB-17).
- The descriptors' content: models, tick rates, display kind, ROM files,
  places (owner: LIB-1, Calc-U-59 column).
- Which operation follows a model switch, an attach or the user's Reset
  and Cold boot, and running the ROM's start-up to its end (owner:
  LIB-18).
- Glyph styles (Classic, Modernized) and drawing the display (the
  frontends).
- Today's headless tools on the new interface (owner: LIB-11).
- Making `Decoder.asm` run, and modelling single chips (`Briefing.md`
  §2).

## Acceptance Criteria

Emulated time
- [ ] **AC-1** — Given a TI machine, then one instruction takes 32
  ticks; on the TI-59 and the TI-58 an instruction executed while the
  calculator is in IDLE takes 128 ticks; on the TI-58C every instruction
  takes 32 (`Core/MachineVariant.hpp:29-39`, `Core/TMC0501.cpp:283-285`).
- [ ] **AC-2** — Given a TI machine advanced by any number of ticks,
  then it overshoots by less than one instruction (at most 127 ticks),
  and reports what it ran (LIB-1 AC-7).
- [ ] **AC-3** — Given a TI machine, then its tick count goes on across
  a reset, a power cycle and a cold boot (LIB-1 AC-8). Today the count
  starts again at every reset (`Core/TMC0501.cpp:156-157`).

Behaviour unchanged
- [ ] **AC-4** — Given the ROMs of a model and keys pressed at the same
  tick counts as in a run of Calc-U-59 2.3.0, when the machine is driven
  through the machine interface, then its display output position by
  position, and the content of its memory, are the same as in that run.
- [ ] **AC-5** — Given what `tests/core_tests.cpp` checks today, then it
  holds through the machine interface for all three models: the boot to
  the key scan; 2 + 3 = 5; the glyphs of the control nibbles; the period
  of the error blink per model; the normalised display duty; the TI-58C
  keeping its memory; the ROM's start-up clearing the RAM on the TI-59
  and TI-58 only (`tests/core_tests.cpp`, 17 tests at the pin).

Reset, power cycle, cold boot
- [ ] **AC-6** — Given a TI machine, then the three operations leave it
  as follows once the ROM's start-up has run:

  | | TI-59, TI-58 | TI-58C |
  |---|---|---|
  | Reset | program and data registers kept; everything else as after switching on | the same as a power cycle |
  | Power cycle | all memory lost | the constant memory kept (all of its RAM, including the registers H00 to H03); everything else as after switching on |
  | Cold boot | all memory empty | all memory empty, the constant memory too |

  (Glossary: Reset, Power cycle, Cold boot. Today: Reset is
  `Shared/StateApplier.cpp:95-107`, which saves the RAM, resets, runs the
  start-up and writes the RAM back; the clean reset is `:109-115`. A
  power cycle that loses the memory of the TI-59 and TI-58 is new:
  behaviour change 59-11.)
- [ ] **AC-7** — Given a newly built TI machine, then it is in the state
  after a cold boot.
- [ ] **AC-8** — Given a TI machine after any of the three operations,
  then no key is down and the display output is that of a calculator
  just switched on, once the start-up has run.

Keys
- [ ] **AC-9** — Given a TI machine, then its 45 keys have these key
  IDs, by row of the keyboard from top to bottom (the spelling: OQ-2):

  | Row | Key IDs, left to right |
  |---|---|
  | 1 | `A` `B` `C` `D` `E` |
  | 2 | `2nd` `INV` `lnx` `CE` `CLR` |
  | 3 | `LRN` `x<>t` `x2` `sqrt` `1/x` |
  | 4 | `SST` `STO` `RCL` `SUM` `y^x` |
  | 5 | `BST` `EE` `(` `)` `/` |
  | 6 | `GTO` `7` `8` `9` `*` |
  | 7 | `SBR` `4` `5` `6` `-` |
  | 8 | `RST` `1` `2` `3` `+` |
  | 9 | `R/S` `0` `.` `+/-` `=` |

  (`Shared/KeyboardLayout.hpp:55-75`; the names are those of
  `tools/headless/ti59.py:32-54`.)
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
  (`Core/TMC0501.hpp:36-49`, `Shared/SevenSegment.hpp:10-11`).
- [ ] **AC-14** — Given what a position shows, then the segments follow
  today's rules: a digit; the signs for degree, minus, quote and
  apostrophe; or blank, with zero suppression as today
  (`Shared/SevenSegment.hpp:15-52`, `Core/TMC0501.cpp:1428-1465`).
- [ ] **AC-15** — Given a running program, then the positions are dark
  and the indicator C is on with a brightness of its own; given the
  calculator waits for a key, then the positions show the number
  (`reference/CoreArchitecture.md:504`, `Core/TMC0501.cpp:296-305`).
- [ ] **AC-16** — Given a TI machine, then it has no sound output.

One seam
- [ ] **AC-17** — Given Calc-U-59 after this feature, when its app layer
  and frontends are built, then they use nothing of the core but the
  machine interface and capabilities. Today these core types are used
  outside `Core/` and go behind the interface: the display snapshot
  (`Shared/DisplayPov.hpp:2`, `Shared/FrameSampler.hpp:25`), the step
  boundary (`Shared/DebugSnapshots.hpp:127`), the printer's code line
  (`Shared/PrinterFont.hpp:2`, `Qt6/app/PrinterWidget.cpp:39`,
  `Web/bindings.cpp:659`), the model list and instruction rate
  (`Shared/MachineModelInfo.hpp:2`), the CPU trace frame and debug event
  (`Shared/TraceWriter.hpp:2`, `Bridge/BridgeConvert.h:5`,
  `Web/bindings.cpp:764`, `Shared/FrameSampler.cpp:19`).
- [ ] **AC-18** — Given the machine, then what the app layer does today
  through calls on it is reached as follows:

  | Today (callers in `Shared/`) | After |
  |---|---|
  | `step`, `stepN`, `pc`, `reset`, `pressKey`, `releaseKey`, `takeDisplay`, `elapsedTicks` | the machine interface (this feature) |
  | printer calls, `loadLibrary` | peripherals in places (C59-6) |
  | `insertCard`, `cardEject`, `isCardPresent`, `isWaitingForCard` | media place (C59-7) |
  | RAM, registers, program steps, partition | product kind keystroke program memory (C59-2); the constant memory (C59-3) |
  | `stepUntilNextKeycode`, `beginNextStep`, `snapshotCPU`, `disassemble`, breakpoints, trace, debug overlay | debug target and product kinds (LIB-10, C59-4) |
  | the scan-loop and key-detect ranges, "polled hardware" | typing support and the report "waits for input" (LIB-5; LIB-17 OQ-3) |

## Edge Cases
- **EC-1** — Given a machine in IDLE on the TI-59 or TI-58, when it is
  advanced by 32 ticks, then it runs one instruction of 128 ticks and
  reports 128.
- **EC-2** — Given the display output is read twice with no advance in
  between, then both readings are the same (`Core/TMC0501.cpp:227`).
- **EC-3** — Given a key ID that is not in AC-9, then key down is
  refused (LIB-1 EC-2); today an invalid matrix code is ignored.
- **EC-4** — Given the TI-58 or the TI-58C, which have no card reader,
  then the ROM still finds its card switch line as today
  (`Core/TI59Machine.cpp:14-23, 154-156`).

## Technical Requirements (optional)
- The machine holds no lock and starts no thread (ADR 0009). The core's
  four mutexes of today (`reference/CoreArchitecture.md:834-850`) have no
  successor.
- The descriptors' depth entries say what is folded into the CPU
  component after this feature and after C59-6 (ADR 0012).
- Before the core is touched, the behaviours of AC-4 are recorded from
  2.3.0 for the shipped examples (display output and memory at the end
  of each), as golden files for this feature.

## Open Questions
- [ ] **OQ-1** — Do reset, power cycle and cold boot finish at the call,
  or does the ROM's start-up run with the advances that follow? Today
  the session runs 300,000 instructions after a reset
  (`Shared/StateApplier.cpp:12-18`) and nothing after a model switch.
  The same question exists for the Sharp machines (C1600-1 OQ-1) and
  belongs to the interface. *Recommendation:* the call returns at once;
  the start-up runs with the following advances; the operation that
  asked for it (LIB-18) waits until the machine reports key readiness.
- [ ] **OQ-2** — The spelling of the key IDs (AC-9), and the second
  functions. `presets.md` PR-5 shows `- key: 2nd Pgm 0 1 SBR =`: "Pgm"
  is the second function printed above LRN, not a key. Either a key has
  one ID and the preset reads `2nd LRN`, or the descriptor also lists
  each second-function name as another name of the same key.
  *Recommendation:* one ID per key as in AC-9, matched without regard
  to case; the 43 second-function names of
  `reference/StateFileFormat.md:179-220` are accepted as other names of
  their keys in presets and in the harness, so that `2nd Pgm` reads as
  on the calculator. The list of those names then goes into this spec.
- [ ] **OQ-3** — The indicator C together with the minus sign in the
  leftmost position. On the real calculator both show; today one glyph
  shows per position (`Shared/SevenSegment.hpp:54-67`,
  `reference/CoreArchitecture.md:526-531`). The display kinds allow
  both. *Recommendation:* show both, as the hardware does, and add a row
  to `docs/behaviour-changes.md`.
- [ ] **OQ-4** — Where the persistence of vision lives. The machine
  gives the duty per position (AC-13). Today Calc-U-59 turns it into
  what the eye sees, with hold, fade and a perceptual curve, in its app
  layer for all three frontends (`Shared/DisplayPov.hpp:6-73`,
  `Shared/FrameSampler.cpp:56-69`). LIB-3 makes the display topic "the
  machine's display output". *Recommendation:* the code stays in the
  Calc-U-59 app layer, once for all frontends, and its result is what
  the frontends draw; whether it travels as the display topic or as a
  product topic is decided with LIB-3.
- [ ] **OQ-5** — Reset and the partition on the TI-59 and TI-58. Today's
  Reset keeps the RAM but not the partition, which goes back to the
  ROM's default; this was read from the code and is covered by no test.
  Is that what "program and data registers kept" should mean?
  *Recommendation:* verify on 2.3.0 first; if true, keep it as it is and
  say so in AC-6.
- [ ] **OQ-6** — TI-58C: Reset equals a power cycle (glossary), and the
  constant memory then comes from the running machine. Today Reset reads
  it back from `ti58c.mem`, which can be up to half a second old
  (`Shared/SessionController.cpp:186, 258-266`). *Recommendation:* from
  the running machine; nobody will see the difference.

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
| Proposed in this spec, for the gate: a newly built machine is in the state after a cold boot (AC-7); the key IDs of AC-9 | Today a new machine and a reset one differ in one CPU flag (`Shared/EmulatorSession.cpp:16-35`); no contract lists the IDs | 2026-11-10 |
