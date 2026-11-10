# C1600-1: Sharp machines on the machine interface

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: LIB-1 (the machine interface; its Calc-U-1600 column holds
  the descriptors' content, AC-34 to AC-40 there), LIB-2 (attach and
  detach in places, controls, serial port, real-time clock; its
  Calc-U-1600 column holds the rules for the Sharp places, AC-28 to
  AC-34 there).
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
  geometry, connectors, places, ROM files, the peripheral descriptors
  and where each fits (owner: LIB-1, Calc-U-1600 column).
- Attach and detach, the rule of each pin-out, which combinations are
  refused, the controls, serial port and real-time clock kinds, and
  setting the clock from the host (owner: LIB-2).
- Module definitions as files, battery-backed modules as media, floppy
  disks and cassettes (owner: LIB-7); the plotters' paper (LIB-8).
- Key readiness, prompt readiness, the latches, and how a key is held
  or queued (owner: LIB-5); the pacing profile and the carry of an
  overshoot (LIB-17, OQ-5 there).
- Other names for a key in presets and in the harness (owner: LIB-6,
  with the list in C1600-4; OQ-5).
- The debug targets, an advance that a debug target stops, and the trace
  (owners: LIB-10, C1600-3).
- Loading BASIC and machine code into memory (owner: C1600-2).
- Which operation follows the user's Reset, Cold boot, a model or
  variant switch or an attach, running the machine to the prompt, and
  what the user sees on the PC-1500 after a module change (owner:
  LIB-18; behaviour change 1600-4; OQ-6).
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
- [ ] **AC-2** — Given a Sharp machine advanced by a number of ticks
  above zero, then it runs whole instructions of the CPU that has the
  bus at that moment, overshoots by less than the last one, and reports
  what it ran (LIB-1 AC-7). On the PC-1600 an instruction is one step of
  the SC7852 or of the LH5803, whichever has the bus
  (`Core/PC1600/PC1600Machine.cpp:340-523`).
- [ ] **AC-3** — Given a machine that is switched off, when it is
  advanced, then its tick count grows, its real-time clock goes on, and
  its display output shows nothing (the power state: OQ-2)
  (`Core/PC1600/PC1600Machine.cpp:348-366`).

Behaviour unchanged
- [ ] **AC-4** — Given a golden run (Technical Requirements), when its
  script is played on the machine through the machine interface, then
  the display output at each recorded moment, the sound output and the
  content of the memory at the end are the recorded ones, for each model
  and variant.
- [ ] **AC-5** — Given what the core tests check today for the CPUs, the
  machines, the peripherals, tape and sound
  (`Core/tests/`: `lh5801_tests`, `lh5803_tests`, `sc7852_tests`,
  `pc1600_machine_tests`, `pc1600_bank_tests`,
  `pc1600_keyboard_display_tests`, the slot, connector, CE-150, CE-158,
  CE-1600P, CE-1600F, host drive, module, tape and `piezo_sampler`
  tests), then it holds with the machines behind the interface.

Reset, power cycle, cold boot
- [ ] **AC-6** — Given a Sharp machine, then the three operations leave
  it as follows once its start-up is over (OQ-1):

  | | PC-1500, PC-1500A | PC-1600 |
  |---|---|---|
  | Reset | the reset button: memory kept (`Core/PC1500/PC1500Machine.cpp:14-32`) | the reset button: internal memory kept (`Core/PC1600/PC1600Machine.cpp:44-50`) |
  | Power cycle | switched off and on again: all memory kept | the same |
  | Cold boot | the machine's own memory empty (today's Reset All, `Core/PC1500/PC1500Memory.cpp:20-29`) | the internal memory empty (ALL RESET, `Core/PC1600/PC1600Machine.cpp:52-62`) |

  Today none of the three touches the content of a module in a slot
  (whether a cold boot should empty a RAM module without a battery:
  OQ-4).
- [ ] **AC-7** — Given a Sharp machine, when its `off` key is pressed
  through the ROM or it switches itself off, then AC-3 holds until the
  key `on` is pressed, and then it runs again, as today
  (`Core/PC1500/PC1500Machine.cpp:309-340`,
  `Core/PC1600/PC1600Machine.cpp:544-552`).
- [ ] **AC-8** — Given the real-time clock of a machine, then a reset
  of the PC-1500 or PC-1500A puts it back to 1 January 2000, a reset of
  the PC-1600 leaves it, and a cold boot of the PC-1600 lets the ROM
  clear it, as today (`Core/PC1500/PC1500Memory.cpp:56`,
  `Core/PC1600/PC1600Machine.cpp:14-62`); it is set from the host
  afterwards by whoever ran the operation (LIB-2 AC-23).

Keys
- [ ] **AC-9** — Given a PC-1500 or PC-1500A, then its 65 keys have
  these key IDs: `0` to `9`; `a` to `z`; `+` `-` `=` `*` `/` `(` `)`
  `.`; `cl` `enter` `mode` `def` `sml` `rcl` `shift` `off` `up` `down`
  `left` `right` `rsv` `space`; `f1` to `f6`; and `on`
  (`Core/PC1500/PC1500Keyboard.cpp:32-71`,
  `Shared/KeyLayout/PC1500KeyLayout.cpp:10-83`).
- [ ] **AC-10** — Given a PC-1600, then its 68 keys have the key IDs of
  AC-9 and `ctrl`, `kbii` and `bs`
  (`Core/PC1600/PC1600Keyboard.cpp:94-137`).
- [ ] **AC-11** — Given the key ID `on`, then it is a key like the
  others at the interface, and it is also the BREAK key: on a machine
  that is off it switches the machine on, on a running machine it
  breaks. It can be down together with any other key (today it has a
  call of its own and is not in the key matrix,
  `Core/PC1500/PC1500Machine.cpp:309-340`).
- [ ] **AC-12** — Given two keys of the matrix held down together, then
  both are in the key matrix (LIB-1 AC-13).

Display and sound
- [ ] **AC-13** — Given a PC-1500 or PC-1500A, then its display output
  is 156 × 7 dots and these 14 indicators: `BUSY` `SHIFT` `SMALL`
  `KANA` `DE` `G` `RAD` `RUN` `PRO` `RESERVE` `DEF` `I` `II` `III`.
  Given a PC-1600, then it is 156 × 32 dots and those 14 plus `S`
  `ROMAJI` `CTRL` `BATT`
  (`Core/Display/StatusLine.hpp:86-132`,
  `Core/PC1500/PC1500Display.cpp:26-30`,
  `Core/PC1600/PC1600Display.cpp:153-186`).
- [ ] **AC-14** — Given a Sharp machine built for 48,000 samples per
  second, then its sound output is the buzzer as today, mono, derived
  from the ticks alone, so that it is the same at regular speed and at
  full speed (`Core/Audio/PiezoSampler.hpp:38-47`; LIB-1 AC-19).

Peripherals and product kinds
- [ ] **AC-15** — Given each peripheral of LIB-1 AC-39, then it exists
  as a peripheral that is plugged in and removed through LIB-2 only, and
  it behaves as today on every model it fits (AC-4, AC-5).
- [ ] **AC-16** — Given a CE-150 on a PC-1500 or PC-1500A, then the
  machine lists its cassette as a media place; given a CE-150 on a
  PC-1600, then it prints and the machine lists no cassette for it, as
  today (`Core/PC1500/PC1500Machine.cpp:49`,
  `Core/PC1600/PC1600Machine.cpp:259-270`).
- [ ] **AC-17** — Given several peripherals on the 60-pin connector,
  then the order in which they answer does not depend on the order in
  which they were plugged in (OQ-3).
- [ ] **AC-18** — Given the CE-1600P, whose ROM has the variants New and
  Old, then changing the variant is a detach and an attach of the
  CE-1600P with the other variant: the machine keeps its memory and the
  disk in the CE-1600F stays in, the same side up, as today
  (`Shared/AppSession.cpp:326-337`, `Shared/MachineSession.cpp:732-739`).
- [ ] **AC-19** — Given a Sharp machine, then it lists these product
  kinds, each with its owner, so that Calc-U-1600's app layer reaches
  them without naming a machine class (`capabilities.md`, product kinds):
  memory access for the BASIC and machine-code loaders (every model;
  specified in C1600-2); the INSPECTOR views (every model); the bytes
  sent to the CE-158's printer port (the CE-158); the folder of the host
  drive (the host drive on the PC-1600; OQ-7). What each offers is what
  the app layer calls today (`Core/PC1500/PC1500Machine.hpp:151-256`,
  `Core/PC1600/PC1600Machine.hpp:241-254, 383-426, 484`).

One seam
- [ ] **AC-20** — Given the two machine classes of today, then a
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
  runs in steps of 256 ticks and may overshoot by up to 255
  (`Core/PC1600/PC1600Machine.hpp:607`); given a PC-1500 that is off or
  halted, then it advances tick by tick and does not overshoot
  (`Core/PC1500/PC1500Machine.cpp:208-265`).
- **EC-3** — Given a memory module that does not fit the place by its
  definition (a CE-1601M in `slot-1`), then the attach is refused
  (LIB-2 EC-1).

## Technical Requirements (optional)
- **Golden runs.** A golden run is a script of calls on the machine:
  key down and key up with a key of AC-9 or AC-10, each at a tick count,
  attaches, and advances. It is recorded in phase 6 from the Calc-U-1600
  code as it is imported, before the cores are changed, by a driver that
  calls the old machine classes directly. The golden file holds the
  script, the display output at moments the script names, the sound
  samples of the whole run, and at the end the content of the memory,
  read by the test inside the core. A comparison is exact. The runs
  cover at least, for each model and variant: switching on to the
  prompt; typing and running a BASIC line that prints a number; a
  `BEEP`; a reset, a power cycle and a cold boot after memory was
  filled; and for each peripheral one run that uses it.
- **No model above the machine.** Nothing above the core names a machine
  class or asks which model runs (`machine.md` I-1); a search in the
  build checks that no file outside the core includes a machine class.
  Today `Shared/MachineSession.cpp` alone does so at about 60 places and
  through 27 dispatches, in these groups:

  | Group today | Places | Goes behind |
  |---|---|---|
  | building a machine and its ROM set | 7 | the catalogue and the ROM lookup (LIB-1, LIB-12) |
  | reset level and booting | 3 | AC-6 |
  | power state and power cycle | 4 | AC-6, AC-7; OQ-2 |
  | the built-in serial port | 1 | serial port capability (LIB-2) |
  | keys and typing | 9 | key IDs (AC-9 to AC-11); LIB-5 |
  | clock rate | 1 | the tick rate of the descriptor |
  | display, text and image | 6 | display output (AC-13) |
  | debugger and INSPECTOR | 8 | debug targets (LIB-10); the INSPECTOR views (AC-19) |
  | one or two slots | 10 | places (LIB-1, LIB-2) |
  | peripherals only the PC-1600 has, the tape interface | 13 | AC-15; media places (LIB-7) |
  | the same call on either class | 27 | the machine interface and capabilities |

  The other files that use the machine classes today are
  `Shared/FloppyBay.cpp`, `Shared/AppSession.cpp:388-406`,
  `Shared/MachineCodeLoad.cpp:36, 117`,
  `Qt6/app/debug/DebugController.cpp:127, 241` and the tools in `tools/`.
- The machines hold no lock and start no thread (ADR 0009); the locks
  the machines keep today are not contended and have no successor
  (`docs/background/Decisions.md:731`).
- The descriptors' depth entries say how each chip and block is modelled
  as the code stands: the two CPUs and the bus arbiter separate, the
  sub-CPU functional, the gate arrays folded into memory (`machine.md`,
  fit table, "Depth today").

## Open Questions
- [ ] **OQ-1** — When a reset, a power cycle or a cold boot is over
  (LIB-1 OQ-1). Today a power cycle is played through the ROM: OFF held,
  the machine waited for until it is off, the change made, ON held, then
  the machine run to the prompt (`Shared/MachineSession.cpp:244-275`); a
  reset is followed by a run to the prompt as well. *Recommendation:* as
  for the TI machines (C59-1 OQ-1): the call returns at once, the
  machine reports "starting up" until its start-up is over, and the
  operation that asked (LIB-18) waits for that. Whether a power cycle
  goes through the ROM's own OFF and ON as today is the core's to
  decide, as long as AC-6 holds.
- [ ] **OQ-2** — The power state at the interface (AC-3, AC-7, AC-20;
  LIB-1 OQ-2). Today the display frame carries "powered on" and the
  frontends leave the LCD blank when it is not
  (`Shared/DisplayFrame.hpp:12-18`). `capabilities.md` Q-1 leaves this
  to a phase 5 walkthrough. *Recommendation:* decide now: "the display
  is on" is part of display output, and "the machine is off" is a state
  every machine reports. AC-3 and AC-7 are then checked against those.
- [ ] **OQ-3** — The order on the 60-pin connector (AC-17). The first
  peripheral that answers a bus cycle wins. Today the order is the order
  of plugging in when the user does it, and a fixed one (CE-150,
  CE-1600P, CE-1600F, CE-158, host drive) when a machine is rebuilt
  (`Core/Connector/CardChain.hpp:18-33`,
  `Shared/MachineSession.cpp:657-664`). *Recommendation:* always the
  fixed order.
- [ ] **OQ-4** — A cold boot and a RAM module without a battery (CE-151,
  CE-155). The glossary says a cold boot leaves "all memory empty, on
  every model", as after a long battery removal; today's Reset All
  leaves every module as it is. A battery-backed module is a medium and
  keeps its content in any case. *Recommendation:* a cold boot also
  empties a RAM module that has no battery of its own, as the real
  device would; the user will hardly see it, because the ROM sets the
  memory up again after ALL RESET.
- [ ] **OQ-5** — Other names of keys. Presets accept `ent` for `enter`
  and `break` for `on` today (`Core/Preset/PresetFile.cpp:202`), and
  upper-case letters. *Recommendation:* the machine takes exactly the
  key IDs of AC-9 and AC-10. Presets and the harness match them without
  regard to case and accept `break` as another name of `on`; `ent` goes.
  That is specified in LIB-6, with the list in C1600-4.
- [ ] **OQ-6** — The PC-1500 after a module change. With behaviour
  change 1600-4, choosing a module power-cycles the machine and keeps
  its memory. But the ROM of the PC-1500 checks its memory at power-on
  when the memory layout changed, and stops at `NEW0? :CHECK`
  (`Core/PC1500/PC1500PresetLoader.cpp:156-162`); the User Guide already
  says that plugging a module in or out "costs the built-in RAM" on the
  real device (`docs/User-Guide.md:436-440`). So on the PC-1500 the
  program survives the power cycle only until the user answers that
  prompt. The same may hold for a newly attached CE-150. Row 1600-4 says
  "keeps all memory" without this. *Recommendation:* keep the power
  cycle, because it is what the real device does; correct row 1600-4 to
  say what the PC-1500's ROM does next; and LIB-18's operation ends when
  the machine shows that prompt, as well as at the BASIC prompt. Verify
  on 0.8.1 what the PC-1600 does.
- [ ] **OQ-7** — The host drive. In the code it is a peripheral on the
  60-pin connector with a ROM of its own
  (`Core/PC1600/PC1600HostDriveCard.hpp:11-40`); `presets.md` PR-11, a
  decided fork, keeps `host-drive:` as a product key and not as a place,
  and the index lists the host drive as carried over. Either it becomes
  a peripheral in a place named `host-drive`, which changes the wording
  of PR-11 and needs the user, or it stays outside the places as a
  product kind of the PC-1600 that the product attaches by a product
  command. *Recommendation:* a peripheral in a place of its own, because
  that is what the code already is; the folder it shows stays a product
  kind (AC-19).

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| The machine is the only seam; no branches on the model above the machine | ADR 0006; `machine.md` I-1 | 2026-11-09 |
| One canonical tick per machine: the LH5801 cycle, the SC7852 T-state | `machine.md` M-1 | 2026-11-10 |
| Keys are key IDs; the matrix stays in the core | `machine.md` M-3 | 2026-11-10 |
| A dot-matrix display with named indicators | `machine.md` M-2 | 2026-11-10 |
| Every attach or detach is followed by a power cycle | `peripherals.md` P-1 (user); behaviour change 1600-4 | 2026-11-10 |
| A connector takes one peripheral or a chain; today's implicit connectors stay implicit | `peripherals.md` P-3, P-6 | 2026-11-10 |
| Product kinds are listed by the machine like generic kinds | `machine.md` M-7 | 2026-11-10 |
| The key IDs and indicator names are written in this spec, not left to phase 6 | LIB-1 decision log | 2026-11-10 |
| The PC-1600 has 18 indicators in the code, not 16 as the fit table of `machine.md` says; the code counts | `Briefing.md` §2: where a note and a product disagree, the product code is authoritative | 2026-11-10 |
| Proposed in this spec, for the gate: `on` is a key ID like the others (AC-11); changing the CE-1600P's ROM variant is a detach and an attach (AC-18); what a golden run is and what the runs cover | `machine.md` M-3 lists the ON key's own call as a fact, not a decision; no contract defines the others | 2026-11-10 |
