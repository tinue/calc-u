# LIB-18: Machine control and startup

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: LIB-2 (attach, detach and replace on a machine, what follows
  them, the real-time clock), LIB-3 (operations, busy, commands with
  replies), LIB-9 (the settings `[startup] model` and `[preset]`),
  LIB-11 (the harness, for this feature's commands), LIB-12 (the ROM
  lookup and the full set).
- Contracts and decisions: `docs/contracts/machine.md` (M-6, I-6),
  `docs/contracts/peripherals.md` (P-1, P-5), `docs/contracts/frontend.md`
  (Q-1), `docs/contracts/capabilities.md` (C-3, Q-3); ADR 0016, 0021,
  0024, 0025; `docs/behaviour-changes.md` rows 59-9, 59-10, 59-11,
  59-12, 1600-4.
- Facts: Calc-U-59 at `v2.3.0`, Calc-U-1600 at `b4ea8c8`.

What the three status columns mean for this feature:
- **Library:** the commands that change the machine and the start of a
  product exist, each as an operation, shown with machines built for the
  tests.
- **Calc-U-59, Calc-U-1600:** the product starts and changes its machine
  only through them; its own code for the start, the model switch, the
  resets and the attaching of peripherals is gone.

## User Stories
- As a user, I want the product to start with the model I chose, or the
  one I used last, set up the way I chose.
- As a user, I want to switch the model, plug things in and take them
  out, reset the machine and start it from scratch, and to find these in
  the same place under the same names in every Calc-U product.
- As a user, I want the machine to react to a plugged-in peripheral or
  module the way the real device does.
- As a user, I want to stop something long that I started by mistake.
- As the author of a harness script, I want the same commands.

## Out of Scope
- Loading a preset, the startup presets themselves and what they
  contain (owners: LIB-6, C59-2, C1600-4). This feature says when a
  startup preset is used.
- What a reset, a power cycle and a cold boot do inside a machine
  (owners: C59-1, C1600-1), and whether an attach is possible (LIB-2).
- Where the commands sit and what they are called on screen: the Machine
  menu, the control bar, shortcuts, the long press (owners: LIB-14,
  C59-8; ADR 0024).
- "Available now" for a model, a variant or a peripheral (owner: LIB-4).
- The `ti58c.mem` file (owner: C59-3). This feature says when a TI-58C
  is built with its constant memory.
- Media in a media place, inserting and ejecting (owner: LIB-7).
- The debugger's pause across a reset (owner: LIB-10).
- What is saved when a product quits (LIB-7, LIB-9, C59-3).
- The message of a product that cannot start for a missing ROM (owner:
  LIB-12).

## Acceptance Criteria

### Library

Building a machine
- [ ] **AC-1** — Given a model, a variant and what is to be plugged into
  which place, then the session builds one machine with all of it in one
  step, from a cold boot, hands a real-time clock the host's date and
  time (LIB-2 AC-23), and the machine is ready when it reports key
  readiness (LIB-1 AC-11). No power cycle follows for what it was built
  with.
- [ ] **AC-2** — Given a machine is built, then the one before it is
  gone with everything it held in its memory; what was saved as a medium
  stays saved (LIB-7), and typed text, held keys and full speed do not
  carry over.

Start of a product
- [ ] **AC-3** — Given a product starts and its full ROM set is there
  (LIB-12), then it builds the startup model: the model named by
  `[startup] model`, or with `last` the model that was built last, or
  with nothing recorded the product's fallback model (LIB-9 AC-9, AC-20,
  AC-27).
- [ ] **AC-4** — Given a startup preset is set for the startup model
  (`[preset]`, LIB-9 AC-10), then the machine is built by loading that
  preset (LIB-6); given none is set, then the product's built-in default
  machine for that model is built (ADR 0021).
- [ ] **AC-5** — Given the start, then it is one operation: the busy
  topic says so until the machine is ready, and exactly one machine is
  built, also when the frontend names a model or a preset to start with
  (LIB-15 AC-16).

Model and variant
- [ ] **AC-6** — Given the command to switch to another model, then a
  new machine of that model is built as at a start: from that model's
  startup preset, or as its built-in default machine (glossary, Startup
  preset; `machine.md` I-6).
- [ ] **AC-7** — Given the command to switch to another variant of the
  running model, then a new machine of that variant is built with the
  same peripherals in the same places and the same media in them; its
  own memory starts empty, and no startup preset is loaded
  (Calc-U-1600 today, `Shared/MachineSession.cpp:126-142`).
- [ ] **AC-8** — Given a switch to the model or variant that already
  runs, then nothing happens and the reply is done (both products
  today).
- [ ] **AC-9** — Given a machine was built by any of these commands or
  by a preset, then its model is recorded as the one built last, for
  `[startup] model = last` (both products today: Calc-U-59
  `Shared/SessionController.cpp:116`, Calc-U-1600
  `Shared/MachineSession.cpp:122, 179`).

Attach, detach, replace
- [ ] **AC-10** — Given the command to attach a peripheral to a place,
  to detach it or to replace it by another, then the session makes the
  change on the machine (LIB-2 AC-4, AC-5, AC-35), runs what the machine
  says follows, and the command is one operation that is over when the
  machine reports key readiness again.
- [ ] **AC-11** — Given a power cycle follows, then the machine's memory
  is what its model keeps through a power cycle (C59-1 AC-6, C1600-1
  AC-6), and afterwards its ROM has found what is plugged in now
  (behaviour changes 59-11, 1600-4).
- [ ] **AC-12** — Given the machine refuses the change, then nothing
  changes, no power cycle runs, and the reply is the refusal with the
  machine's reason (LIB-2 AC-6).

Reset, power cycle, cold boot
- [ ] **AC-13** — Given the commands Reset and Cold boot, then each runs
  that operation of the machine (LIB-1 AC-11) as an operation of the
  session that is over when the machine reports key readiness; what is
  plugged in stays plugged in (LIB-2 AC-10), and the real-time clock is
  handed the host's time at the end (LIB-2 AC-23).
- [ ] **AC-14** — Given Reset or Cold boot, then typed text that is
  still waiting and the keys the typing engine holds are dropped
  (LIB-5 EC-5), and the machine goes back to regular speed.
- [ ] **AC-15** — Given the harness, then these commands exist at the
  top level, each with the reply of its operation: `model <name>`,
  `variant <name>`, `attach <place> <peripheral>`, `detach <place>`,
  `reset`, `cold-boot` and `power-cycle` (OQ-5).

While an operation runs
- [ ] **AC-16** — Given an operation runs, then every command of this
  feature is refused (`frontend.md` Q-1), except as OQ-1 decides for
  stopping an operation.

### Calc-U-59
- [ ] **AC-17** — Given no settings yet, then Calc-U-59 starts with a
  TI-59 (LIB-9 AC-20).
- [ ] **AC-18** — Given no startup preset is set for a model, then its
  built-in default machine is: the TI-59 with the Master Library module
  in `slot-1` and nothing in `printer` (ADR 0021; behaviour change
  59-10; today the printer is connected at every start,
  `Shared/SessionController.hpp:313`); the TI-58 and the TI-58C the same
  (OQ-2).
- [ ] **AC-19** — Given a TI-58C is built without a startup preset, at a
  start or at a model switch, then it is built with the constant memory
  of `ti58c.mem`; given it is built from a startup preset or from any
  preset, then it is cold booted like any machine (`capabilities.md`
  C-3; behaviour change 59-12).
- [ ] **AC-20** — Given a model switch in Calc-U-59, then the module and
  the printer of the old machine are not carried over: the new machine
  has what its startup preset or the built-in default gives it. Today
  the printer's connection and the module of the settings stay
  (`Shared/SessionController.cpp:93-154`; behaviour changes 59-9, 59-10).
- [ ] **AC-21** — Given the module is changed or the PC-100C is attached
  or detached, then a power cycle follows: a TI-59 and a TI-58 lose
  program and data registers, a TI-58C keeps its constant memory. Today
  both changes reset the machine and keep the memory
  (`Shared/SessionController.cpp:220-241, 362-371`; behaviour change
  59-11).
- [ ] **AC-22** — Given Reset, then the TI-59 and TI-58 keep program and
  data registers, and on the TI-58C it is a power cycle (C59-1 AC-6);
  given Cold boot, then all memory is empty, the constant memory too.
  Today these are "Reset" and the clean reset
  (`Shared/SessionController.cpp:182-207`; the names: glossary and
  `docs/glossary-mapping.md`).
- [ ] **AC-23** — Given Reset or Cold boot in Calc-U-59, then the
  printer's paper, the module, the printer and the position of the TRACE
  switch are as before (`Shared/SessionController.cpp:156-207`; C59-6
  AC-6).
- [ ] **AC-24** — Given a preset names another model than the one that
  runs, then the machine of that model is built by the preset (LIB-6),
  and AC-9 holds for it, as today (`Shared/SessionController.cpp:598-614`).

### Calc-U-1600
- [ ] **AC-25** — Given no settings yet, then Calc-U-1600 starts with a
  PC-1500A (LIB-9 AC-27), whose ROM shows `NEW0? :CHECK` after a cold
  boot, as today (`Web/tests/play.spec.js:44-47`).
- [ ] **AC-26** — Given no startup preset is set for a model, then its
  built-in default machine is the model in its default variant (A04 for
  the PC-1500, New for the PC-1600) with nothing plugged in
  (`Shared/MachineSession.hpp:367-370`, `Shared/Settings.hpp:64-66`).
- [ ] **AC-27** — Given a model switch in Calc-U-1600, then the new
  machine has no module and no peripheral of the old one, as today
  (`Shared/AppSession.cpp:282-293`).
- [ ] **AC-28** — Given a variant switch (the ROM of the PC-1500 or of
  the PC-1600), then AC-7 holds: the printer, the interface, the host
  drive and the modules are plugged in again, the disk is in its drive
  again, and no startup preset is loaded
  (`Shared/MachineSession.cpp:126-142, 653-664`;
  `docs/User-Guide.md:848-854`).
- [ ] **AC-29** — Given a module is chosen for a slot, then it is a
  replace with a power cycle: the machine's memory is kept, and its ROM
  then finds the changed memory and shows `NEW0? :CHECK`. Today choosing
  a module builds a new machine and loses the memory
  (`Shared/AppSession.cpp:308-312`; behaviour change 1600-4).
- [ ] **AC-30** — Given a printer or the CE-158 is attached or detached,
  or the host drive's folder is first mounted or unmounted, then a power
  cycle follows and the memory is kept, as today
  (`Shared/AppSession.cpp:339-380`, `Shared/MachineSession.cpp:244-275`).
- [ ] **AC-31** — Given the ROM of the CE-1600P is switched while it is
  plugged in, then it is a replace of the CE-1600P by its other variant
  (C1600-1 AC-18).
- [ ] **AC-32** — Given Reset, then the machine's memory is kept; given
  Cold boot, then it is empty; these are today's Reset and Reset All
  (`Shared/AppSession.cpp:276-280`; the names: glossary and
  `docs/glossary-mapping.md`).
- [ ] **AC-33** — Given a Sharp machine is switched off when a
  peripheral is attached or detached, then the change is made, nothing
  follows, the machine stays off, and its ROM finds the change when it
  is switched on, as today (`Shared/MachineSession.cpp:266-269`; OQ-3).

## Edge Cases
- **EC-1** — Given the startup preset of the startup model cannot be
  loaded (the file is gone, it is outdated, it is for another model),
  then the built-in default machine of that model is built and a notice
  says why (today Calc-U-1600 says "The default preset … is for a
  different model", `Shared/AppSession.cpp:264-274`).
- **EC-2** — Given `[startup] model = last` and the model built last was
  built by a preset, then that model is the startup model, and its
  startup preset or built-in default applies, not the preset that built
  it.
- **EC-3** — Given a machine is waiting at a prompt of its ROM that
  takes a key (`NEW0? :CHECK`), then it reports key readiness there, and
  an operation that waits for readiness is over (C1600-1 AC-21).
- **EC-4** — Given a model switch while typed text is being typed, then
  the text is dropped with the old machine (AC-2).
- **EC-5** — Given a detach of a peripheral that has others plugged into
  it, then one power cycle follows, not one per peripheral (LIB-2 EC-3).

## Technical Requirements (optional)
- Every command of this feature is an operation of the app session: it
  runs in parts with live updates (LIB-3 AC-14, AC-15), and the harness
  runs it in one go (LIB-3 AC-19).
- A machine is never changed from outside an operation, and nothing
  above the machine runs a power cycle by pressing the machine's own
  keys; how a model goes through a power cycle is its core's (C1600-1
  OQ-1).
- Switching the model builds a new machine; no machine changes its model
  (`machine.md` I-6).

## Open Questions
- [ ] **OQ-1** — Stopping an operation. `frontend.md` Q-1 refuses every
  command that would start an operation while one runs, Reset included,
  and a preset is one operation until its last step. Today Reset is the
  only way to stop a preset that plays in Calc-U-59
  (`Shared/SessionController.cpp:156-163`), and in Calc-U-1600 nothing
  stops one, with waits of up to an hour of emulated time
  (`Core/Preset/PresetRunner.cpp:75-91`). With Q-1 as it stands the user
  could not stop anything. *Recommendation:* Reset, Cold boot and a
  model switch are not refused while an operation runs: they end it
  first, and it replies that it was stopped. This amends Q-1 and LIB-3
  AC-16, and it keeps what Calc-U-59 users know.
- [ ] **OQ-2** — The built-in default machine of the TI-58 and the
  TI-58C (`capabilities.md` Q-3). ADR 0021 states it for the TI-59 only.
  *Recommendation:* the same: the Master Library module, no printer.
- [ ] **OQ-3** — AC-33 needs the machine to say that nothing follows an
  attach while it is switched off, although its pin-out says a power
  cycle (LIB-2 AC-4). No machine reports a power state (LIB-1 decision
  log). *Recommendation:* "what follows" is the machine's answer at that
  moment, and a switched-off Sharp machine answers "nothing"; that keeps
  today's behaviour without a power state at the interface.
- [ ] **OQ-4** — What the Calc-U-59 Reset does besides resetting the
  machine today: it takes the fitted card and the card in the reader
  away and clears the cue cards (`Shared/SessionController.cpp:156-172`).
  *Recommendation:* that stays, and is specified with the cards in
  C59-7.
- [ ] **OQ-5** — The names of the harness commands (AC-15). Presets name
  hardware in their header and have no such steps. *Recommendation:* as
  in AC-15.
- [ ] **OQ-6** — Calc-U-59 keeps the paper, the log and the debug
  settings across a model switch today
  (`tests/shared_controller_tests.cpp:119-132`). The paper belongs to a
  printer that the new machine may not have. *Recommendation:* the log
  and the debug settings stay; the paper stays while the new machine has
  a PC-100C and goes otherwise; settle the paper in the LIB-8 spec.
- [ ] **OQ-7** — A PC-1500 that is power-cycled with a newly attached
  CE-150 stops at `NEW0? :CHECK` like after a module change
  (`Core/PC1500/PC1500PresetLoader.cpp:156-162`). Row 1600-4 names the
  module change only. *Recommendation:* word the row for any change of
  what is plugged in that changes the memory the ROM finds.

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| Settings hold the startup model and each model's startup preset; without a startup preset a product starts with its built-in default machine | ADR 0021 | 2026-11-09 |
| A preset always builds its machine from a cold boot | ADR 0016 | 2026-11-09 |
| Calc-U-59's built-in default is a TI-59 with the Master Library module and no printer | ADR 0021; behaviour change 59-10 | 2026-11-09 |
| Every attach or detach on today's pin-outs is followed by a power cycle | `peripherals.md` P-1 (user); behaviour changes 59-11, 1600-4 | 2026-11-10 |
| A reset, power cycle or cold boot is over when the machine is ready to accept a key press | LIB-1 decision log (user) | 2026-11-10 |
| After a module change and its power cycle the PC-1500 and PC-1600 ask the user to set BASIC up again, as the hardware does | C1600-1 decision log (user) | 2026-11-10 |
| A TI-58C without a startup preset is built with its constant memory; with one it is cold booted | `capabilities.md` C-3 (user); behaviour change 59-12 | 2026-11-10 |
| Reset and Cold boot are the family's names; "clean reset" and "Reset All" go | ADR 0024; glossary | 2026-11-09 |
| Proposed in this spec, for the gate: a start builds exactly one machine (AC-5); a variant switch keeps what is plugged in and loads no startup preset (AC-7, Calc-U-1600's rule for the family); a switch to what already runs does nothing (AC-8); a startup preset that cannot be loaded gives the built-in default with a notice (EC-1) | Today the Calc-U-1600 Web frontend builds two machines when a model is named; no contract states the others | 2026-11-10 |
