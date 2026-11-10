# LIB-2: Peripherals, connectors and small capability kinds

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: LIB-1 (machines, descriptors, places, the list of
  capabilities, ticks). Only the host link of a serial port (AC-18 to
  AC-20) also needs LIB-3 (it is served by the app session) and LIB-9
  (the folder for serial links).
- Contracts and decisions: `docs/contracts/peripherals.md` (P-1 to P-6,
  I-1 to I-4, fit table), `docs/contracts/capabilities.md` (C-1; the
  kinds controls, serial port and real-time clock; I-1 to I-3),
  `docs/contracts/machine.md` (M-3, M-5, M-8),
  `docs/contracts/settings.md` (ST-3, serial links); ADR 0007, 0009,
  0010; `docs/behaviour-changes.md` rows 59-11, 1600-4.

What the three status columns mean for this feature:
- **Library:** attach and detach on a machine, the rules a pin-out
  states, and the kinds controls, serial port and real-time clock exist
  and are shown to work with machines and peripherals built for the
  tests.
- **Calc-U-59, Calc-U-1600:** every peripheral of the product is plugged
  in and removed only this way, and its controls, serial ports and
  real-time clocks are reached only through these kinds. The columns are
  worked together with C59-6 and C1600-1, which build the peripherals.

Two words used here that the glossary does not define yet (OQ-5): a
**host link** is what connects a machine's serial port to the user's
computer; a **serial port file** is the name under which a host link
appears there, a pseudo-terminal that any serial program can open.

## User Stories
- As a user, I want to plug a printer, an interface or a module into my
  machine and take it out again, and to be told why when something does
  not fit.
- As a user, I want the keys and switches on a peripheral (the PC-100C's
  PRINT, ADV and TRACE, the write-protect switch of a CE-161) to work as
  on the real thing.
- As a user of a Sharp machine, I want its serial ports to appear as
  serial port files on my computer, so that SharpDataExchange and any
  terminal program can use them, as today.
- As a user of a Sharp machine, I want its clock to show the time of my
  computer, as today.
- As the developer of a product, I want a peripheral to know only the
  connector it is plugged into, so that the same peripheral works on
  every model that has that pin-out.
- As the developer of a later product, I want to add peripherals and
  pin-outs in my core without changing the library.

## Out of Scope
- The commands of the app session that attach and detach, and running
  the power cycle that follows as an operation (owner: LIB-18, where
  behaviour changes 59-11 and 1600-4 are stated for the user). What a
  power cycle does to a model's memory: C59-1, C1600-1.
- What each peripheral does when its machine is reset, power-cycled or
  cold booted (the product's core: C59-6, C1600-1). This feature only
  says that it stays plugged in (AC-10).
- Which peripherals fit where, shown to the user with "available now"
  and a reason (owner: LIB-4); the pickers and buttons (LIB-7, LIB-14).
- The signals of each pin-out, and what each control does (the
  product's core: C59-6, C1600-1; `peripherals.md` P-4).
- Media places, the content of a peripheral that holds memory, and
  saving a switch position with it (owner: LIB-7); paper output (LIB-8);
  typing support (LIB-5); debug targets (LIB-10).
- The steps `switch` and `syncclock` and their harness commands (owner:
  LIB-6, which builds on this feature); harness commands for a serial
  port (`harness.md` H-2: a Calc-U-1600 product command; see the open
  point in `features/INDEX.md`).
- Module definitions as files (LIB-7, C1600-1); finding the ROM images
  of peripherals (LIB-12).
- The CE-158's printer port (a Calc-U-1600 product kind,
  `capabilities.md` C-2; carried over unchanged).
- Connectors that today are implicit (the CE-1600P's rear connector, the
  CE-1600F with the CE-1600P, the CE-158's printer port): they stay
  implicit (`peripherals.md` P-3).
- Serial ports on Windows and in the Web frontends: not available today
  and not added (Calc-U-1600 `docs/User-Guide.md:610`;
  `docs/inventory/divergence-matrix.md`, M64).

## Acceptance Criteria

### Library

Pin-outs and fit
- [ ] **AC-1** — Given a pin-out, then it states: whether it is contact
  level or functional; whether its connector takes one peripheral or a
  chain; and what follows an attach or a detach on it, a power cycle or
  nothing (`peripherals.md` P-5, P-6).
- [ ] **AC-2** — Given a place, then it presents exactly one of the
  pin-outs its connector can carry, and a peripheral fits the place when
  its descriptor says it fits that pin-out. The list of peripherals that
  fit a place follows from this rule, also for a module definition read
  after the catalogue was built.
- [ ] **AC-3** — Given a place, then it takes as many peripherals at
  once as its descriptor says (`machine.md` M-8); a place on a chain can
  take one while the connector as a whole holds several.

Attach and detach
- [ ] **AC-4** — Given a machine, a place and a peripheral that fits it,
  with the variant to use and the ROM images and content it needs, when
  the peripheral is attached between two advances, then it shows under
  that place in what is plugged in, its capabilities are in the
  machine's list with the peripheral and its place as owner, and the
  machine says what follows by the pin-out: a power cycle, or nothing.
- [ ] **AC-5** — Given a peripheral in a place, when it is detached
  between two advances, then it no longer shows under that place, its
  capabilities are no longer listed, and the machine says what follows
  by the pin-out.
- [ ] **AC-6** — Given a machine, a place and a peripheral, when the
  machine is asked whether the peripheral can be attached there now,
  then it answers yes, or no with a reason; an attach is refused in
  exactly the cases where the answer is no, with the same reason, and a
  refused attach changes nothing.
- [ ] **AC-7** — Given a place that already holds as many peripherals as
  it takes, when another is attached, then the attach is refused with a
  reason.
- [ ] **AC-35** — Given a place that holds a peripheral, when it is
  replaced by another in one step (the user picks another module), then
  the first is detached, the second attached, and what follows is asked
  once.
- [ ] **AC-8** — Given a connector that takes a chain, when peripherals
  are attached to different places on it, then all of them are plugged
  in at once, each under its own place.
- [ ] **AC-9** — Given a peripheral that offers a connector and a place
  of its own, when another peripheral is attached there, then what is
  plugged in is a tree: machine, place, peripheral, place, peripheral;
  what follows is what the pin-out at the place of the change says
  (`peripherals.md` P-3).
- [ ] **AC-10** — Given a machine with peripherals, when it goes through
  a reset, a power cycle or a cold boot, then every peripheral is still
  plugged into its place afterwards, and each is told of the operation
  (both products today: Calc-U-1600 `Shared/MachineSession.cpp:229-240`
  and the fact that Reset and Reset All keep the peripherals; Calc-U-59
  `Core/TI59Machine.cpp:39-45`, which re-applies the printer after a
  reset).
- [ ] **AC-11** — Given a peripheral reads no file, then everything it
  needs comes in with the attach (AC-4).

What a peripheral knows
- [ ] **AC-12** — Given one peripheral that fits two machines with
  different tick rates, when it is attached to each and the same
  stretch of emulated time is run, then what it does in emulated seconds
  is the same on both (`peripherals.md` P-2).
- [ ] **AC-13** — Given a machine with a peripheral, when the machine is
  not advanced, then nothing the peripheral puts out changes, however
  much wall time passes (`peripherals.md` I-3).

Controls
- [ ] **AC-14** — Given a peripheral or a component with keys or
  switches that are not machine keys, then it has a controls capability
  that lists them, each with an ID that is unique within its owner,
  whether it is a key or a switch, and its state: down or up for a key,
  on or off for a switch. A control is named by its owner and its ID.
- [ ] **AC-15** — Given a control that is a key, when it is pressed and
  released through the capability, then its owner sees it down and up at
  those tick counts.
- [ ] **AC-16** — Given a control that is a switch, when it is set on or
  off through the capability, then its owner sees the new position, and
  the capability reports it. What its owner does about it, and when, is
  the owner's.

Serial port
- [ ] **AC-17** — Given a component or a peripheral with a serial port,
  then it has a serial port capability through which the bytes the
  machine sends can be taken out and bytes can be put in for the machine
  to receive, in order, and which gives the line settings the machine
  has set as far as the model has them. The capability itself reaches no
  host (`capabilities.md` I-3).
- [ ] **AC-18** — Given a serial port capability and a platform with
  pseudo-terminals, then the app session gives it a host link: every
  byte the machine sends reaches the host link and every byte arriving
  on the host link reaches the machine, moved between two advances on
  the session's thread (ADR 0009).
- [ ] **AC-19** — Given a host link, then it appears to the user as a
  serial port file with a fixed name in the folder for serial links
  (LIB-9), and the name stays the same for as long as the product runs.
- [ ] **AC-20** — Given a host link of a serial port that belongs to a
  machine or a peripheral that goes away (a model switch, a detach),
  then the serial port file stays, so that a program on the user's
  computer that has it open is not cut off; it is connected again when a
  serial port of the same kind exists again
  (Calc-U-1600 `Shared/MachineSession.hpp:372-380`).

Real-time clock
- [ ] **AC-21** — Given a component with a real-time clock, then it has
  a real-time clock capability through which the clock can be read and
  set, in the fields its chip keeps.
- [ ] **AC-22** — Given a machine with a real-time clock, when emulated
  time passes, then the clock has advanced by that emulated time,
  whatever wall time has passed; nothing in the machine reads the host's
  clock.
- [ ] **AC-23** — Given a machine with a real-time clock, then whoever
  builds it or runs an operation on it hands the clock the host's date
  and time through the capability: when the machine is built and at the
  end of every operation (LIB-18; today Calc-U-1600
  `Shared/AppSession.cpp:86-92`, `Shared/MachineSession.cpp:224`).

### Calc-U-59
- [ ] **AC-24** — Given a TI machine, then the place `slot-1` takes one
  Solid State ROM module and the place `printer` takes the PC-100C;
  both pin-outs are functional and say that a power cycle follows an
  attach or a detach (`peripherals.md`, fit table, P-1, P-5; behaviour
  change 59-11).
- [ ] **AC-25** — Given a TI machine with a module in `slot-1`, when
  another module is attached there, then the attach is refused until the
  first is detached; given no module, then the slot is empty and the
  machine runs (`presets.md` PR-9, C59-6).
- [ ] **AC-26** — Given a PC-100C in the place `printer`, then it has a
  controls capability with the keys PRINT and ADV and the switch TRACE;
  what they do is stated in C59-6. Given no PC-100C is plugged in, then
  no machine lists these controls.
- [ ] **AC-27** — Given Calc-U-59, then no machine has a serial port or
  a real-time clock capability (`capabilities.md`, fit table).

### Calc-U-1600
- [ ] **AC-28** — Given a Sharp machine, then each module slot takes one
  memory module and the 60-pin connector takes a chain; all of their
  pin-outs are contact level and say that a power cycle follows an
  attach or a detach (`peripherals.md`, fit table, P-1, P-5, P-6;
  behaviour change 1600-4).
- [ ] **AC-29** — Given a CE-150 or a CE-1600P in the place `printer`,
  when another printer is attached, then the attach is refused, because
  the place takes one (AC-7). Given a CE-1600P is plugged in, when a
  CE-158 is attached, or the other way round, then the attach is refused
  with the reason that the two are not supported together. Today the
  core removes the other peripheral without a word and the frontends
  grey the controls out (Calc-U-1600
  `Core/PC1600/PC1600Machine.cpp:90-91, 265, 290`,
  `Shared/ControlState.cpp:21-31`; `docs/User-Guide.md:198-201, 647-649`).
- [ ] **AC-30** — Given a CE-150 in `printer`, when a CE-158 is attached
  to `interface`, then both are plugged in on the 60-pin connector
  (`peripherals.md`, fit table, "Peripherals at once").
- [ ] **AC-31** — Given a CE-158 on a PC-1500 and on a PC-1600, when it
  sends the same bytes at the same baud rate, then they take the same
  emulated time on both, although the tick rates differ
  (`peripherals.md` P-2).
- [ ] **AC-32** — Given a CE-161 in a module slot, then it has a
  controls capability with its write-protect switch; while the switch is
  on, every write to the module's RAM is blocked, and the module's media
  place reports the same position as its write protection
  (`Core/Connector/ExpansionCard.hpp:159-165`; `capabilities.md`,
  generic kinds).
- [ ] **AC-33** — Given a PC-1600 exists, then on macOS and Linux its
  RS-232C port has a host link with the serial port file
  `calcu1600-rs232c.serial`; given a CE-158 first uses its serial port,
  on any of the three models, then that port has a host link with the
  serial port file `calcu1600-ce158.serial`
  (`docs/User-Guide.md:585-651`, `Shared/MachineSession.hpp:372-380`).
- [ ] **AC-34** — Given a PC-1500, PC-1500A or PC-1600, then it has a
  real-time clock capability, and AC-22 and AC-23 hold as today: the
  clock runs on emulated time and is set from the host when the machine
  is built and at the end of every operation.

## Edge Cases
- **EC-1** — Given a peripheral and a place whose pin-out it does not
  fit, when it is attached, then the attach is refused with a reason.
- **EC-2** — Given a place that holds nothing, when a detach is called
  for it, then nothing changes.
- **EC-3** — Given a peripheral with peripherals plugged into its own
  places, when it is detached, then they are detached with it, and what
  follows is asked once, at the place of the detached peripheral.
- **EC-4** — Given a peripheral that needs a ROM image, when it is
  attached without it, then the attach is refused and the missing file
  is named (as for a machine, LIB-1 EC-1; today Calc-U-1600 says "The …
  could not be attached", `Shared/AppSession.cpp:19-22`).
- **EC-5** — Given a serial port on a platform without pseudo-terminals,
  then the machine runs and what it sends goes nowhere, as on Windows
  today.
- **EC-6** — Given a control ID that its owner does not have, when it is
  pressed or set, then the call is refused and nothing changes.
- **EC-7** — Given a serial port file that no program has open, when the
  machine sends, then the machine is not held up.

## Technical Requirements (optional)
Invariants checked in review and in the phase 5 walkthroughs:
- Attach and detach are calls on the machine between two advances of
  emulated time, never inside one (`peripherals.md`, responsibilities).
- A peripheral never knows which machine or which connector of a machine
  it is in; it reaches the machine only through the signals of its
  connector (`peripherals.md` I-1, I-2).
- Nothing above a core reaches a peripheral except through the machine's
  capabilities (`peripherals.md` I-2).
- Moving a connector from functional to contact level changes the
  pin-out and the peripherals on it, never the machine interface or the
  capabilities exposed (`peripherals.md` I-4).
- The library defines no hardware signal; each pin-out's signals are
  defined by the core that owns the connector (`peripherals.md` P-4).

## Open Questions
- [ ] **OQ-1** — The reason of a refused attach (AC-6): is it an English
  sentence, shown as it is (`frontend.md`: "messages to the user are
  English sentences from the app layer"), or a code the library turns
  into text? *Recommendation:* a sentence, from the core for product
  rules (AC-29) and from the library for the general cases (AC-7, EC-1);
  the control state shows the same sentence.
- [ ] **OQ-2** — The serial port in detail (AC-17, AC-18): which line
  settings the capability gives; whether the host link follows them;
  how much may wait in each direction, and what happens when the machine
  does not take bytes as fast as the user's program sends them. Today
  the user sets up the port on the calculator and opens the serial port
  file with any settings. *Recommendation:* carry today's behaviour over
  as it is and write it into the Calc-U-1600 criteria once it has been
  read from the code (`Core/` serial link, `Shared/PtySerialLink`); the
  capability reports what the machine set and the host link ignores it.
- [ ] **OQ-4** — The write-protect switch of the CE-161 (AC-32): does
  the ROM see a change at once, or at its next power-on? The glossary
  says a CE-161 "becomes" a program module with the switch set. Today
  the switch is moved without anything following
  (`Shared/MachineSession.hpp:171-173`). And the position a control has
  when its peripheral is attached: off, or what was saved with the
  module? *Recommendation:* nothing follows a switch, as today; the
  position comes with the content handed in at the attach (LIB-7).
- [ ] **OQ-5** — "Host link" and "serial port file" for the glossary, as
  defined above.
- [ ] **OQ-6** — Controls of a component of the machine itself. The
  contract allows them ("a peripheral or component"); neither product
  has one today. Nothing to decide unless the user knows of one.

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| Peripherals are keyed by connector: a peripheral declares the pin-outs it fits and sees only its connector's signals | ADR 0010; `peripherals.md` I-1 | 2026-11-09 |
| Every attach or detach on today's pin-outs is followed by a power cycle | `peripherals.md` P-1 (the user's choice) | 2026-11-10 |
| What follows an attach is stated per pin-out, not as a library rule | `peripherals.md` P-5 | 2026-11-10 |
| The host's ticks reach a peripheral through its connector; it learns the tick rate when attached | `peripherals.md` P-2 | 2026-11-10 |
| A peripheral may offer connectors of its own; what is plugged in is a tree; today's implicit cases stay implicit | `peripherals.md` P-3 | 2026-11-10 |
| Pin-out signals are defined in the core of the product that owns the connector | `peripherals.md` P-4 | 2026-11-10 |
| A pin-out says whether its connector takes one peripheral or a chain; a place says how many it takes | `peripherals.md` P-6; `machine.md` M-8 | 2026-11-10 |
| Controls, serial port and real-time clock are generic kinds | `capabilities.md` C-1; ADR 0007 | 2026-11-10 |
| The power cycle after an attach is run by the app session as an operation, so it is specified in LIB-18 | Glossary, Operation; rule 1 of the cut | 2026-11-10 |
| Peripherals stay plugged in through reset, power cycle and cold boot | Both products today (AC-10) | 2026-11-10 |
| Proposed in this spec, for the gate: a place presents one pin-out and fit follows from it (AC-2); the machine can be asked whether an attach would succeed (AC-6); a detach takes the peripherals plugged into the detached one with it (EC-3); replacing a peripheral is one step with one power cycle (AC-35); a serial port file outlives the machine or peripheral it serves (AC-20, today's behaviour made a rule) | The control state needs "available now" with a reason (`frontend.md` FE-6); no contract states the others | 2026-11-10 |
| The real-time clock is set from the host when a machine is built and at the end of every operation; Calc-U-1600's note "seeded once" is out of date and is corrected in phase 6 | user, 2026-11-10, batch 2 gate: the clock runs on emulated time, so every stretch at full speed moves it ahead; later changes re-seed it more often | 2026-11-10 |
