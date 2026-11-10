# C1600-1: Sharp machines on the machine interface

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: LIB-1 (the machine interface; its Calc-U-1600 column holds
  the descriptors' content), LIB-2 (attach and detach in places,
  controls, serial port, real-time clock).
- Contracts and decisions: `docs/contracts/machine.md` (M-1 to M-8, I-1
  to I-6, fit table), `docs/contracts/peripherals.md` (P-1 to P-6, fit
  table), `docs/contracts/capabilities.md` (product kinds of
  Calc-U-1600; Q-1); ADR 0006, 0009, 0012;
  `docs/behaviour-changes.md` row 1600-4.
- Facts: Calc-U-1600 at `b4ea8c8`; every `path:line` below is at that
  commit.

## User Stories
- As a user of Calc-U-1600, I want the PC-1500, PC-1500A and PC-1600 to
  behave exactly as today after the rebuild: the same ROMs, display,
  sound and timing, the same peripherals.
- As the developer, I want one interface for all three models, so that
  nothing above the cores asks which model it is.
- As the author of a preset or a harness script, I want the same key
  names on every Sharp model wherever the key exists.
- As the developer, I want to add a model later (a PC-1500 variant, an
  SC61860 machine in another product) without touching the code above
  the machine.

## Out of Scope
- The descriptors' content: models, variants, tick rates, display
  geometry, connectors, places, ROM files, peripheral descriptors
  (owner: LIB-1, Calc-U-1600 column).
- Attach and detach as such, the rule of each pin-out, the controls,
  serial port and real-time clock kinds (owner: LIB-2). This feature
  supplies the Sharp peripherals and their product rules.
- Module definitions as files, battery-backed modules as media, floppy
  disks and cassettes (owner: LIB-7); the plotters' paper (LIB-8).
- Key readiness, prompt readiness, the latches, and how a key is held
  or queued (owner: LIB-5); the pacing profile (LIB-17).
- The debug targets, INSPECTOR views and the trace (owners: LIB-10,
  C1600-3; the views are carried over as a product kind).
- Loading BASIC and machine code into memory (owner: C1600-2).
- The host drive's files and its ROM, the CE-158's printer port
  (carried over as product kinds).
- Which operation follows the user's Reset, Cold boot, a model or
  variant switch or an attach, and running the machine to the prompt
  (owner: LIB-18; behaviour change 1600-4).
- Drawing the display and the indicators (the frontends).
- The known deviation of the PC-1600's BASIC speed (`TODO.md:8`).

## Acceptance Criteria

Emulated time
- [ ] **AC-1** — Given a PC-1500 or PC-1500A, then its tick is one cycle
  of the LH5801 at 1,300,000 per second; given a PC-1600, then its tick
  is one T-state of the SC7852 at 3,580,000 per second, and the time of
  the LH5803 and of everything else in the machine is counted in those
  ticks (`Core/PC1500/PC1500Clocks.hpp:9`,
  `Core/PC1600/PC1600Clocks.hpp:8`, `Core/PC1600/PC1600Machine.hpp:109-112`).
- [ ] **AC-2** — Given a Sharp machine advanced by a number of ticks,
  then it runs whole instructions of the CPU that has the bus at that
  moment, overshoots by less than the last one, and reports what it ran
  (LIB-1 AC-7). On the PC-1600 an instruction is one step of the SC7852
  or of the LH5803, whichever has the bus
  (`Core/PC1600/PC1600Machine.cpp:340-523`).
- [ ] **AC-3** — Given a machine that is switched off, when it is
  advanced, then emulated time still passes, its clock goes on, and it
  can be switched on again (`Core/PC1600/PC1600Machine.cpp:348-366`).

Behaviour unchanged
- [ ] **AC-4** — Given the ROMs of a model and variant, the same
  peripherals and keys pressed at the same tick counts as in a run of
  Calc-U-1600 0.8.1, when the machine is driven through the machine
  interface, then its display output, its sound output and the content
  of its memory are the same as in that run.
- [ ] **AC-5** — Given what the core tests check today for the CPUs, the
  machines, the peripherals, tape and sound
  (`Core/tests/`: `lh5801_tests`, `lh5803_tests`, `sc7852_tests`,
  `pc1600_machine_tests`, `pc1600_bank_tests`,
  `pc1600_keyboard_display_tests`, the slot, connector, CE-150, CE-158,
  CE-1600P, CE-1600F, host drive, module, tape and `piezo_sampler`
  tests), then it holds with the machines behind the interface.

Reset, power cycle, cold boot
- [ ] **AC-6** — Given a Sharp machine, then the three operations leave
  it as follows:

  | | PC-1500, PC-1500A | PC-1600 |
  |---|---|---|
  | Reset | the reset button: memory kept (`Core/PC1500/PC1500Machine.cpp:14-32`) | the reset button: internal memory kept (`Core/PC1600/PC1600Machine.cpp:44-50`) |
  | Power cycle | switched off and on again: all memory kept | the same |
  | Cold boot | all memory of the machine empty (today's Reset All, `Core/PC1500/PC1500Memory.cpp:20-29`) | all internal memory empty (ALL RESET, `Core/PC1600/PC1600Machine.cpp:52-62`) |

  In all three, the peripherals stay plugged in and the content of the
  modules in the slots is not touched.
- [ ] **AC-7** — Given a Sharp machine, when the user presses its OFF
  key or it switches itself off, then it is off until the ON key is
  pressed, as today (`Core/PC1500/PC1500Machine.cpp:309-340`,
  `Core/PC1600/PC1600Machine.cpp:544-552`).

Keys
- [ ] **AC-8** — Given a PC-1500 or PC-1500A, then its 65 keys have
  these key IDs: `0` to `9`; `a` to `z`; `+` `-` `=` `*` `/` `(` `)`
  `.`; `cl` `enter` `mode` `def` `sml` `rcl` `shift` `off` `up` `down`
  `left` `right` `rsv` `space`; `f1` to `f6`; and `on`
  (`Core/PC1500/PC1500Keyboard.cpp:32-71`,
  `Shared/KeyLayout/PC1500KeyLayout.cpp:10-83`).
- [ ] **AC-9** — Given a PC-1600, then its 68 keys have the key IDs of
  AC-8 and `ctrl`, `kbii` and `bs`
  (`Core/PC1600/PC1600Keyboard.cpp:94-137`).
- [ ] **AC-10** — Given the key ID `on`, then it is a key like the
  others at the interface, and it is also the BREAK key: on a machine
  that is off it switches the machine on, on a running machine it
  breaks (today it has a call of its own,
  `Core/PC1500/PC1500Machine.cpp:309-340`).
- [ ] **AC-11** — Given two keys held down together, then both are in
  the key matrix (LIB-1 AC-13).

Display and sound
- [ ] **AC-12** — Given a PC-1500 or PC-1500A, then its display output
  is 156 × 7 dots and these 14 indicators: `BUSY` `SHIFT` `SMALL`
  `KANA` `DE` `G` `RAD` `RUN` `PRO` `RESERVE` `DEF` `I` `II` `III`.
  Given a PC-1600, then it is 156 × 32 dots and those 14 plus `S`
  `ROMAJI` `CTRL` `BATT`
  (`Core/Display/StatusLine.hpp:86-132`,
  `Core/PC1500/PC1500Display.cpp:26-30`,
  `Core/PC1600/PC1600Display.cpp:153-186`).
- [ ] **AC-13** — Given a Sharp machine, then its sound output is the
  buzzer as today, mono, derived from the ticks alone, so that it is the
  same at regular speed and at full speed
  (`Core/Audio/PiezoSampler.hpp:38-47`; today 48,000 samples per second).

Peripherals
- [ ] **AC-14** — Given a Sharp machine, then these peripherals can be
  plugged in, each in its place, through LIB-2 only:

  | Peripheral | Place | Models |
  |---|---|---|
  | memory modules: CE-151, CE-155, CE-502B | `slot-1` | PC-1500, PC-1500A; the first two also PC-1600 Slot 1 |
  | CE-161, CE-1638, CE-163F | `slot-1`, `slot-2` | all three (Slot 2 on the PC-1600 only) |
  | CE-1600M | `slot-1`, `slot-2` | PC-1600 |
  | CE-1601M, superRAM | `slot-2` | PC-1600 |
  | CE-150 | `printer` | all three |
  | CE-1600P with the CE-1600F | `printer` | PC-1600 |
  | CE-158 | `interface` | all three |

  (`Qt6/resources/cards/*.card.yaml`, each file's `compatible-hosts`;
  `Core/PC1500/PC1500Machine.cpp:42-99`,
  `Core/PC1600/PC1600Machine.cpp:81-124, 259-294`; the host drive:
  OQ-3.)
- [ ] **AC-15** — Given a CE-1600P is plugged in, when a CE-158 is
  attached, or the other way round, then the attach is refused with the
  reason that the two are not supported together; given a printer is in
  `printer`, when another is attached, then it is refused because the
  place takes one. Today the core removes the other peripheral without
  a word and the frontends grey the buttons out
  (`Core/PC1600/PC1600Machine.cpp:90-91, 265, 290`,
  `Shared/ControlState.cpp:21-31`).
- [ ] **AC-16** — Given a CE-150 on a PC-1500 or PC-1500A, then its
  cassette interface works; given a CE-150 on a PC-1600, then it prints
  but is not connected to a cassette recorder, as today
  (`Core/PC1500/PC1500Machine.cpp:49`,
  `Core/PC1600/PC1600Machine.cpp:259-270`).
- [ ] **AC-17** — Given several peripherals on the 60-pin connector,
  then the order in which they answer does not depend on the order in
  which the user plugged them in (OQ-4).

One seam
- [ ] **AC-18** — Given Calc-U-1600 after this feature, then nothing
  above its core names a machine class or asks which model runs. Today
  `Shared/MachineSession.cpp` alone does so at about 60 places and
  through 27 dispatches; they fall into these groups, each of which goes behind the
  interface as shown:

  | Group today | Places | After |
  |---|---|---|
  | building a machine and its ROM set | 7 | the catalogue and the ROM lookup (LIB-1, LIB-12) |
  | reset level and booting | 3 | reset, power cycle, cold boot (AC-6) |
  | power state and power cycle | 4 | AC-6, AC-7; OQ-2 |
  | the built-in serial port | 1 | serial port capability (LIB-2) |
  | keys and typing | 9 | key IDs (AC-8 to AC-10); typing profile and typing support (LIB-5) |
  | clock rate | 1 | the tick rate of the descriptor |
  | display, text and image | 6 | display output (AC-12) |
  | debugger and INSPECTOR | 8 | debug targets and a product kind (LIB-10) |
  | one or two slots | 10 | places (LIB-1, LIB-2) |
  | peripherals only the PC-1600 has, the tape interface | 13 | peripherals in places (AC-14); media places (LIB-7) |
  | the same call on either class | 27 | the machine interface and capabilities |

- [ ] **AC-19** — Given the two machine classes of today, then a
  function only one of them offers, or that they offer under different
  names, is reached the same way on every model that has it: whether the
  machine is off (today `isPoweredOff` on the PC-1600 only and the CPU's
  state on the PC-1500), whether the display is on (today `isDisplayOn`
  on the PC-1500 only), the modules in the slots, the cassette.

## Edge Cases
- **EC-1** — Given a key ID that the model does not have (`kbii` on a
  PC-1500), then key down is refused (LIB-1 EC-2). Today an unknown key
  name is ignored without a word
  (`Core/PC1500/PC1500Machine.cpp:301-308`).
- **EC-2** — Given a PC-1600 that is off, when it is advanced, then it
  may overshoot by up to 255 ticks, the length of one step while off
  (`Core/PC1600/PC1600Machine.hpp:607`).
- **EC-3** — Given the PC-1500 is reset, then its real-time clock chip
  starts again from 1 January 2000 as today
  (`Core/PC1500/PC1500Memory.cpp:56`); the clock is set from the host
  afterwards by whoever ran the reset (LIB-2, LIB-18).
- **EC-4** — Given a memory module that does not fit the place by its
  definition (a CE-1601M in `slot-1`), then the attach is refused
  (LIB-2).

## Technical Requirements (optional)
- The machines hold no lock and start no thread (ADR 0009); the locks
  the machines keep today are not contended and have no successor
  (`docs/background/Decisions.md:731`).
- No file outside the core includes a machine class of the core; this is
  checked by a search in the build, so that a model branch cannot come
  back unnoticed.
- The descriptors' depth entries say how each chip and block is modelled
  as the code stands: the two CPUs and the bus arbiter separate, the
  sub-CPU functional, the gate arrays folded into memory (`machine.md`,
  fit table, "Depth today").
- Before the cores are touched, the behaviours of AC-4 are recorded from
  0.8.1 for the shipped examples (display output and memory at the end
  of each), as golden files for this feature.

## Open Questions
- [ ] **OQ-1** — Do reset, power cycle and cold boot finish at the call?
  Today a power cycle is played through the ROM: OFF held, the machine
  waited for until it is off, the change made, ON held, then the machine
  run to the prompt (`Shared/MachineSession.cpp:244-275`); a reset is
  followed by a run to the prompt as well. The same question exists for
  the TI machines (C59-1 OQ-1). *Recommendation:* the call returns at
  once and the ROM's start-up runs with the following advances; the
  operation that asked for it (LIB-18) waits for prompt readiness.
  Whether a power cycle goes through the ROM's own OFF and ON as today
  is the core's to decide, as long as AC-6 holds.
- [ ] **OQ-2** — The power state at the interface (AC-19). Today the
  display frame carries "powered on" and the frontends leave the LCD
  blank when it is not (`Shared/DisplayFrame.hpp:12-18`).
  `capabilities.md` Q-1 leaves this to a phase 5 walkthrough.
  *Recommendation:* decide now, because both machines need it on the
  first day: "the display is on" is part of display output for every
  display kind, and "the machine is off" is a state every machine
  reports (a TI machine is never off).
- [ ] **OQ-3** — The host drive. In the code it is a peripheral on the
  60-pin connector with a ROM of its own
  (`Core/PC1600/PC1600HostDriveCard.hpp:11-40`); `presets.md` PR-11
  keeps `host-drive:` as a product key and not as a place.
  *Recommendation:* a peripheral with a descriptor in a place of its own
  on the 60-pin connector, named `host-drive`; the folder it shows is a
  product kind. The preset key then names that place, which amends the
  wording of PR-11.
- [ ] **OQ-4** — The order on the 60-pin connector (AC-17). The first
  peripheral that answers a bus cycle wins. Today the order is the order
  of plugging in when the user does it, and a fixed one (CE-150,
  CE-1600P, CE-1600F, CE-158, host drive) when a machine is rebuilt
  (`Core/Connector/CardChain.hpp:18-33`,
  `Shared/MachineSession.cpp:657-664`). *Recommendation:* always the
  fixed order.
- [ ] **OQ-5** — Other names of keys. Presets accept `ent` for `enter`
  and `break` for `on` today (`Core/Preset/PresetFile.cpp:202`); upper
  case letters are accepted too. *Recommendation:* one key ID per key as
  in AC-8, matched without regard to case; `break` stays as another name
  of `on`, because BREAK is what the key says on a running machine;
  `ent` goes.
- [ ] **OQ-6** — The carry of an overshoot. Today Calc-U-1600 throws
  away what an advance overshot (`Shared/MachineSession.cpp:452`), so
  each call of the host runs a few ticks more than was due and they add
  up; LIB-17 AC-3 says overshoots do not add up. The new pacer is then
  slightly more exact than the old, and the before-and-after test of
  LIB-17 AC-21 must allow for it. *Recommendation:* carry the overshoot;
  state the expected difference in LIB-17.
- [ ] **OQ-7** — A PC-1500 that is power-cycled with a newly attached
  CE-150 may stop at the ROM's `NEW0? :CHECK` prompt; only the preset
  loader answers it today (`Core/PC1500/PC1500PresetLoader.cpp:153-171`).
  Not verified. *Recommendation:* verify on 0.8.1; what the user sees
  stays as it is.

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| The machine is the only seam; no branches on the model above the machine | ADR 0006; `machine.md` I-1 | 2026-11-09 |
| One canonical tick per machine: the LH5801 cycle, the SC7852 T-state | `machine.md` M-1 | 2026-11-10 |
| Keys are key IDs; the matrix stays in the core | `machine.md` M-3 | 2026-11-10 |
| A dot-matrix display with named indicators | `machine.md` M-2 | 2026-11-10 |
| Every attach or detach is followed by a power cycle; a module change keeps all memory | `peripherals.md` P-1 (user); behaviour change 1600-4 | 2026-11-10 |
| A connector takes one peripheral or a chain; today's implicit connectors stay implicit | `peripherals.md` P-3, P-6 | 2026-11-10 |
| The key IDs and indicator names are written in this spec, not left to phase 6 | LIB-1 decision log | 2026-11-10 |
| The PC-1600 has 18 indicators in the code, not 16 as the fit table of `machine.md` says; the code counts | `Briefing.md` §2: where a note and a product disagree, the product code is authoritative | 2026-11-10 |
| Proposed in this spec, for the gate: `on` is a key ID like the others (AC-10); a refused combination replaces today's silent removal (AC-15) | `machine.md` M-3 lists the ON key's own call as a fact, not a decision; `peripherals.md` says an attach can be refused with a reason | 2026-11-10 |
