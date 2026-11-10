# Contract: peripheral and connector model

Status: draft (2026-11-10). Forks P-1 to P-4 are decided (decision log);
open questions remain.

Inherits: ADR 0008 (composition), 0010 (peripherals keyed by connector),
0011 (TI-59 printer and module slot as functional connectors), 0012
(boundaries follow observable behaviour), 0021 (open point: does a module
change keep RAM or cold boot), 0025 (modules chosen in the control bar).
Builds on the machine contract (`machine.md`): M-1 (ticks), M-4 (static
descriptor, live queries), M-5 (capabilities listed by the machine), M-6
(power cycle).

## Responsibilities

- A **connector** belongs to a machine (or to a peripheral, see P-3). The
  descriptor lists it with its form and the pin-outs it can carry
  (machine contract, descriptor).
- A **pin-out** names the signals on a connector and what they carry. It
  is either contact level (pin-numbered contacts, every bus cycle, as the
  Sharp 40-pin and 60-pin connectors today) or functional (named signals
  and what they carry, not contact timing, as the TI-59 slot and printer
  connector after ADR 0011). Which one is stated per pin-out.
- A **peripheral** declares the pin-outs it fits and sees only the signals
  of the connector it is plugged into (ADR 0010). The machine wires its
  connectors to its components; the connector, not the peripheral, knows
  the host (Calc-U-1600 `Core/Connector/ExpansionCard.hpp`, "cards know
  only the bus").
- A peripheral exposes capabilities (media, printer output, serial port,
  real-time clock, …) through the machine (M-5), with itself as owner.
- Attaching or detaching a peripheral is a call on the machine, between
  two advances of emulated time, and is followed by a power cycle (P-1). It can be refused, with a reason (for
  example: one plotter at a time on the PC-1500/1600, CE-1600P with CE-158
  not supported, `User-Guide.md:198-201`).
- What the running machine has plugged in where is a live query (M-4).

## Non-responsibilities

- No files: module definitions, ROM images of peripherals and media
  contents are handed in by the app session (contracts 6, 9).
- No GUI: which picker or button attaches a peripheral is the frontend
  contract's (ADR 0025 for modules).
- No persistence: a battery-backed module's contents are a medium
  (contract 6).

## Invariants

- I-1: A peripheral never knows which machine or which connector of a
  machine it is in (ADR 0010).
- I-2: A peripheral reaches the machine only through its connector's
  signals; nothing above the core reaches a peripheral except through the
  machine's capabilities (ADR 0006).
- I-3: A peripheral's emulated time is the host's ticks, passed through
  the connector (P-2); a peripheral never looks at wall time.
- I-4: Moving a connector from functional to contact level changes the
  pin-out and the peripherals on it, never the machine interface or the
  capabilities exposed (ADR 0011, 0012).

## Open forks

### P-1: What an attach or detach does to the machine (ADR 0021)

**Decided (2026-11-10): option (b), a power cycle everywhere** (the
user's choice; (a) was recommended). Consequences:
- On the TI-58/59, a module change or connecting or disconnecting the
  PC-100C loses the program and the data registers, as a real power cycle
  does. Today Calc-U-59 keeps them. This is a user-visible change that
  the user accepted as an exception to Briefing §1.1; it goes into the
  Calc-U-59 release notes.
- On the TI-58C, the constant memory survives (glossary, Power cycle).
- On the PC-1500/1600, a module change no longer restarts the machine
  (today: like a model switch) but power-cycles it: all memory is kept and
  the ROM sees the new module when it powers on. Printers already work
  this way.
- The descriptor needs no per-connector operation; what survives is the
  model's business (M-6).

Facts:
- Calc-U-59: a module change and the printer connect toggle both call
  `resetMachine()` (`Shared/SessionController.cpp:240`, `:368`). On the
  TI-59/58 that saves the RAM, resets, runs the start-up and writes the
  RAM back (`Shared/StateApplier.cpp:95-107`), which the real calculator
  cannot do (glossary, Reset); on the TI-58C it restores the constant
  memory. The program and data registers survive.
- Calc-U-1600: attaching or detaching a printer runs a power cycle, "on a
  real machine the ROM only notices a new peripheral when it powers on"
  (`User-Guide.md:205-220`); choosing a module "restarts the machine, like
  a model switch does" (`User-Guide.md:389-390`).
- A real power cycle loses the RAM on the TI-58/59 (glossary, Power
  cycle). Whether the real devices require power off to change a module
  or peripheral is not in the research sources (Q-1).
- Briefing §1.1: no user-visible feature may break. Keeping the program
  across a module change is user-visible in Calc-U-59.

Options:
- (a) **Recommended:** each connector (pin-out) states in the descriptor
  which machine operation follows an attach or detach on it: reset,
  power cycle or cold boot. The app session runs it. Initial values are
  today's behaviour: TI-59 module slot and printer connector → reset;
  Sharp 60-pin printers → power cycle; Sharp memory modules → cold boot.
  One family rule (the descriptor decides), no behaviour change, and a
  later change per connector is a descriptor edit.
- (b) One family rule, power cycle everywhere: closest to the hardware,
  but the TI-59/58 lose their program on a module change (breaks §1.1).
- (c) One family rule, cold boot everywhere: Calc-U-1600's module rule;
  same break on Calc-U-59, and printers lose the RAM too.

### P-2: How emulated time reaches a peripheral

**Decided (2026-11-10): option (a).**

Facts: Calc-U-1600's 60-pin cards are credited host CPU clocks for the
recorder behind a tape interface (`SystemBusCard::advanceCassette`,
"LH5801 cycles on the PC-1500, SC7852 T-states on the PC-1600"); the
TapeDeck is built with that host's clock rate. The TI-59 printer's BUSY
time is counted in instruction cycles inside `TMC0501`.

Options:
- (a) **Recommended:** the connector passes the host's ticks (M-1), and
  the peripheral learns the host's tick rate when it is attached. A
  peripheral that fits two hosts with different clocks (CE-150 on PC-1500
  and, later, PC-1600) works on both.
- (b) Each peripheral keeps its own clock and converts from wall time:
  breaks I-3 and full speed (machine contract F-2, F-4).

### P-3: Connectors on peripherals

**Decided (2026-11-10): option (a), a tree.**

Facts: the CE-1600P has a rear connector for the CE-150, not modelled
(`calc-u-1600.md`, Modelling depth); the CE-1600F attaches together with
the CE-1600P as a union, implicit; the CE-158's printer port goes
straight to a panel. Briefing §1.9: as soon as components or peripherals
are plugged together, the connector is modelled explicitly.

Options:
- (a) **Recommended:** a peripheral may offer connectors of its own; the
  machine's plugged-in state is then a tree (machine → connector →
  peripheral → connector → …). Today's implicit cases stay implicit until
  they are refined (I-4); the model allows them.
- (b) Only machines have connectors: the CE-1600P rear connector and the
  CE-1600F could never become explicit, against Briefing §1.9.

### P-4: Where pin-out signal interfaces are defined

**Decided (2026-11-10): option (a), in the core.**

Facts: no peripheral crosses products: the TI-59 slot and printer
connector, the Sharp 40-pin and 60-pin connectors and the SC61860 11-pin
and 15-pin connectors are each used by one product only. Within a
product, peripherals do cross models (CE-161 in the PC-1500 slot and both
PC-1600 slots; CE-150 and CE-158 on both machines).

Options:
- (a) **Recommended:** the library defines what every peripheral and
  connector has (name, pin-outs fitted, attach and detach, ticks,
  capabilities, debug view of its memories); each pin-out's signal
  interface (bus cycles, BUSY, printer data, …) is defined by the core of
  the product that owns the connector. Keeps the library free of
  hardware signals; matches ADR 0006 (core types stay below the machine).
- (b) The library defines the signal interfaces too: shared code for
  pin-outs no other product uses.

## Fit table

Facts, except the last row, which is the decided behaviour (P-1). Sources: `docs/inventory/calc-u-59.md`
and `calc-u-1600.md` (Modelling depth, Media), datamath TMC0540 page,
`ti59-research/PC100C/service_manual.txt:33-45`, SC61860 knowledge base
(`Shared/11-Pin-Port.md`, `Shared/Peripherals.md`, machine overviews).

| | TI-59 (58, 58C) | PC-1500 (1500A) | PC-1600 | PC-1403 (paper) | PC-1350 (paper) |
|---|---|---|---|---|---|
| Connectors | module slot (8 contacts); PC-100C printer connector (11 pads) | 40-pin memory slot; 60-pin system bus | 2 × 40-pin slots (Slot 1, Slot 2); 60-pin system bus; RS-232C | 11-pin | 11-pin; 15-pin serial; RAM card slot |
| Pin-out level | functional (ADR 0011). Slot = the TMC0540 chip's pins: VGG, VDD, instruction words, calculating status, external access, Φ2, Φ1, common. Printer: Busy (KR), KP, KN, Φ1, D0, D15, IRG, IDLE, EXT, Φ2, D12, I/A | contact level (`PinState`, `SystemBusPins`) | contact level; Slot 1/2 carry different pin-outs on one connector form | 11-pin pin-out per model (port bits differ, `11-Pin-Port.md`) | 11-pin as PC-1350 column; 15-pin serial (`15-Pin-Serial.md`) |
| Peripherals | 14 Solid State ROM modules (TMC0541-0555 family); PC-100C | memory modules (CE-151, CE-155, CE-161, CE-502B, …); CE-150, CE-158 | memory modules (CE-1600M, CE-1601M, CE-161, CE-163F, …); CE-1600P (+ CE-1600F); CE-150, CE-158 (MODE 1); host drive | CE-126P (printer + cassette), CE-140F (floppy) | CE-126P, CE-124 (cassette), CE-130T, CE-516L → CE-515P; CE-201M / CE-202M RAM cards |
| Peripheral connectors (P-3) | — | CE-150 cassette jack (`TapeDeck`) | CE-1600P rear connector (not modelled); CE-1600F union (implicit); CE-158 printer port (implicit) | CE-126P cassette | — |
| After attach/detach today | reset, RAM kept | printers: power cycle; modules: restart | printers: power cycle; modules: restart | unknown (Q-2) | unknown (Q-2) |
| After attach/detach (P-1) | power cycle: TI-58/59 lose RAM, TI-58C keeps constant memory | power cycle, memory kept | power cycle, memory kept | power cycle | power cycle |

## Open questions

- Q-1 (closed 2026-11-10: no longer needed, P-1 decided a power cycle
  regardless): Do the real TI-58/59, PC-1500 and PC-1600 require power off to
  change a module or a peripheral? Not found in `ti59-research`,
  `Sharp1500-1600-Ref` or the datamath TMC0540 page.
- Q-2 (deferred 2026-11-10: to an SC61860 product or a phase 5 walkthrough):
  What the PC-1403 and PC-1350 ROMs do when an 11-pin peripheral or a
  PC-1350 RAM card changes (paper machines; needed only for their
  descriptors).
- Q-3 (deferred 2026-11-10: until the slot is modelled at contact level,
  ADR 0011): The datamath page gives the TMC0540 chip's pins; that the 8-pin
  slot carries exactly these signals is assumed, not stated there.

## Decision log

| Date | ID | Decision |
|---|---|---|
| 2026-11-10 | P-1 | Every attach or detach is followed by a power cycle, in every product. TI-58/59 lose RAM on a module or printer change (accepted exception to Briefing §1.1); settles ADR 0021's open point. |
| 2026-11-10 | P-2 | A peripheral is passed the host's ticks through its connector and learns the host's tick rate when attached. |
| 2026-11-10 | P-3 | A peripheral may offer connectors of its own; what is plugged in is a tree. Today's implicit cases stay implicit until refined. |
| 2026-11-10 | P-4 | The library defines what every peripheral and connector has; each pin-out's signal interface is defined by the core of the product that owns the connector. |
