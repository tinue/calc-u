# LIB-1: Machine interface and descriptors

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: none.
- Contracts and decisions: `docs/contracts/machine.md` (M-1 to M-8,
  I-1 to I-6, fit table), `docs/contracts/capabilities.md` (C-1, I-1 to
  I-3, fit table), `docs/contracts/peripherals.md` (P-3, P-6 and the fit
  table, for what a descriptor says about connectors, pin-outs and
  places), `docs/contracts/roms.md` (R-1, R-4, for the ROM entries of a
  descriptor), `docs/contracts/typing.md` (T-3, for several keys down at
  once); ADR 0006, 0007, 0008, 0009, 0012, 0035.

What the three status columns mean for this feature:
- **Library:** the machine interface, the descriptors and the catalogue,
  the emulated-time type, the display kinds and the listing of
  capabilities exist and are shown to work with machines built for the
  tests.
- **Calc-U-59, Calc-U-1600:** the product's catalogue describes every
  model and peripheral it offers, and each running machine agrees with
  its descriptor. The descriptor content is specified here, not in the
  machine rows (rule 2 of the cut); these columns are worked together
  with C59-1, C59-6 and C1600-1, which build the machines.

## User Stories
- As the developer of a product, I want one interface that every machine
  offers, so that sessions, typing, presets, the debugger and the harness
  are written once and work for every model.
- As the developer of a product, I want each model and peripheral
  described before any machine is built, so that the ROM lookup, the
  control state and the preset loader can work from the description and
  never branch on the model.
- As the developer of a later product (SC61860 models), I want to add
  models by writing a core and its descriptors, so that the library does
  not change.
- As an agent working through the harness, I want to ask a machine what
  it is made of and what it offers right now, so that I do not need to
  know the model beforehand.
- As a user, I want time, sound and the display to behave as on the real
  device at regular speed and at full speed, so that programs, tapes and
  timings stay correct.

## Out of Scope
- How a TI or Sharp machine behaves behind the interface: what a reset,
  a power cycle and a cold boot do to its memory, and removing the model
  branches above the cores (owners: C59-1, C1600-1).
- Attach and detach, pin-out signals, what follows an attach, what a
  cold boot does to plugged peripherals, and the kinds controls, serial
  port and real-time clock (owner: LIB-2).
- The typed interface of each other capability kind: typing support
  (LIB-5), media place (LIB-7), paper output (LIB-8), debug target
  (LIB-10), and every product kind (the product feature that needs it).
- Pacing, regular and full speed as the user sees them (owner: LIB-17);
  sessions, operations and messages (owner: LIB-3).
- Finding and checking ROM files (owner: LIB-12). This feature only says
  which files a descriptor lists and that a machine is handed its images.
- Building a machine on a model or variant switch, Reset and Cold boot
  as commands (owner: LIB-18).
- Rendering display output: pixels, glyph styles, colours (the
  frontends; LIB-14, open point "Calculator view" in `features/INDEX.md`).
- Descriptors for SC61860 models (paper machines; phase 5).

## Acceptance Criteria

### Library

Descriptors and catalogue
- [ ] **AC-1** — Given a product's catalogue, when it is asked for its
  models, then it gives one machine descriptor per model, with the
  model's variants inside it, and every descriptor is available without
  building a machine and without any ROM file present.
- [ ] **AC-2** — Given a machine descriptor, then it states: the model
  and its model family; the variants and the default variant; per
  variant the ROM files needed, each with file name, size of the decoded
  content and md5 of the known dump; the tick rate; the display kind
  with its geometry and its indicators by name, and for a segment
  display its segment layout; the key IDs; the connectors, each with its
  form and the pin-outs it can carry; the places, each with its name,
  the connector behind it, the peripherals that fit and how many it
  takes at once; the components, each with the chip or block it stands
  for and its modelling depth (separate, folded into another component,
  or functional); the media places, each with its kind, pass-through or
  holding, and its sides; the debug targets.
- [ ] **AC-3** — Given a product's catalogue, when it is asked for its
  peripherals, then it gives one peripheral descriptor per peripheral,
  available without building a machine, that states: the name; the
  variants and the default variant; the ROM files needed per variant;
  the pin-outs it fits; its own connectors and places; its media places;
  its controls; its components and their modelling depth. A module
  definition read after the catalogue was built gives a peripheral
  descriptor of the same form (who reads the files: LIB-7, C1600-1).
- [ ] **AC-4** — Given a place name of a model (for example `slot-1` or
  `printer`), when the descriptor is asked, then it gives the connector
  behind the place and the peripherals that fit it, so that no caller
  needs to know the model to go from a place to its connector.

Building and identity
- [ ] **AC-5** — Given a model, a variant and the ROM images its
  descriptor lists, when a machine is built, then it reports that model
  and that variant for as long as it exists.
- [ ] **AC-6** — Given two machines built alike (the same model,
  variant, ROM images and peripherals, and the same starting value for
  anything a machine is handed at its build, such as the time of a
  real-time clock), when both are given the same calls at the same tick
  counts, then their display output, sound output and capability
  outputs are identical, however much wall time passes between the
  calls.

Emulated time
- [ ] **AC-7** — Given a machine, when it is advanced by a number of
  ticks, then it runs whole instructions only, runs at least that many
  ticks, reports the ticks it actually ran, and its tick count has grown
  by exactly the reported number. Where an instruction ends is the
  core's to say (the PC-1600 with its two CPUs: C1600-1). An advance
  that a debug target stops early is LIB-10's.
- [ ] **AC-8** — Given a machine that is advanced and has keys pressed,
  when its tick count is read at any two moments, then the later reading
  is never smaller than the earlier one, also across a reset, a power
  cycle and a cold boot; a newly built machine starts at zero.
- [ ] **AC-9** — Given a machine that is not advanced, when any amount
  of wall time passes, then its tick count, its display output and its
  capability outputs do not change.
- [ ] **AC-10** — Given a tick count and the tick rate of its
  descriptor, when the library's emulated-time type converts it to
  seconds and back, then the same count results, for every tick rate in
  the catalogues (384,000, 394,000, 455,000, 1,300,000 and 3,580,000 per
  second) and every count up to one year of emulated time; a count that
  is a whole multiple of the rate gives whole seconds exactly.

Reset, power cycle, cold boot
- [ ] **AC-11** — Given any machine, then it offers reset, power cycle
  and cold boot; each call returns at once, the ROM's start-up runs with
  the advances that follow, and the operation is over when the machine
  reports key readiness again (AC-42). After each of them the machine
  still reports the same model and variant. What each does to the machine's
  memory is stated per model in C59-1 and C1600-1.

Keys
- [ ] **AC-42** — Given any machine, then it reports key readiness:
  whether its ROM is ready to accept a key press now. A machine that has
  just been built, reset, power-cycled or cold booted reports it only
  when its start-up is over.
- [ ] **AC-12** — Given a machine and a key ID from its descriptor, when
  key down and later key up are called with it, then the machine sees
  that key pressed and released at those tick counts; the caller never
  names a row or column of the key matrix.
- [ ] **AC-13** — Given a machine with one key down, when key down is
  called with a second key ID, then the machine sees both keys down
  until each gets its key up (`typing.md` T-3; behaviour change 59-13).
- [ ] **AC-14** — Given a peripheral with keys or switches of its own,
  then they are not in the machine's key ID list; they are controls of
  the peripheral (LIB-2).

Display and sound
- [ ] **AC-15** — Given a machine with a segment display, when its
  display output is read, then it gives, per position, which of the
  named segments of the segment layout are lit, and a brightness per
  position where the hardware has one, and a separate one for a segment
  the hardware drives separately (the TI decimal point), each from 0 to
  1, as the duty the core measures (what the eye sees is decided above
  the machine); the
  number of segments per position is not limited to seven or eight, and
  the decimal point is a segment.
- [ ] **AC-16** — Given a machine with a dot-matrix display, when its
  display output is read, then it gives the state of each dot of the
  width and height its descriptor states, grouped into character cells
  where the descriptor says so.
- [ ] **AC-17** — Given a machine of either display kind with
  indicators, when its display output is read, then each indicator of
  its descriptor is given by name with its state: on or off, and a
  brightness where the hardware has one.
- [ ] **AC-43** — Given any display output, then it says whether the
  display is on or off; a display that is off shows nothing. No machine
  reports a power state besides that.
- [ ] **AC-18** — Given any display output, then it contains no pixels
  in a host format, no glyph style and no colour.
- [ ] **AC-19** — Given a machine with sound, then its sound output is
  samples at the rate its caller named when the machine was built,
  derived from the ticks alone; when the same stretch of emulated time
  is run once in many small advances and once in one large advance, the
  samples of both runs are identical. A machine without sound gives no
  sound output.

Connectors and capabilities
- [ ] **AC-20** — Given a running machine, when it is asked what is
  plugged in, then it gives, per place, the peripherals in it, and per
  connector the peripherals on it; where several places share a
  connector (a chain), each peripheral is given under its own place. The
  descriptor is not changed by what is plugged in.
- [ ] **AC-21** — Given a running machine, when it is asked for its
  capabilities, then it lists every capability present now, each with
  its kind, an ID that is unique within the machine and the same every
  time a machine is built alike, and its owner (a component, or a
  peripheral with the place it is in), and hands out the typed interface
  of that kind on request.
- [ ] **AC-22** — Given two machines built for the tests, one with a
  peripheral and one without, then the first lists that peripheral's
  capabilities and the second does not (the change at an attach or
  detach: LIB-2).
- [ ] **AC-23** — Given a product kind defined by a product's core, when
  a machine of that product lists its capabilities, then the product
  kind is listed and handed out exactly like a generic kind.
- [ ] **AC-24** — Given the library alone, without the code of any
  product, when it is built and its tests are run, then it builds and
  the tests pass; they use machines built for the tests that have a
  segment display, a dot-matrix display, sound, a peripheral and a
  product kind between them.
- [ ] **AC-25** — Given a running machine, when it is asked for its
  debug targets, then it gives the ones its descriptor lists (their
  interface: LIB-10).

### Calc-U-59
- [ ] **AC-26** — Given the Calc-U-59 catalogue, then it lists the
  models TI-59, TI-58 and TI-58C, none with a variant, with the tick
  rates 455,000 (TI-59), 384,000 (TI-58) and 394,000 (TI-58C) per
  second (`machine.md`, fit table).
- [ ] **AC-27** — Given any of the three models, then its descriptor
  states a segment display of 12 positions with a segment layout of the
  segments A to G and the decimal point, a brightness per position and a separate one for
  the decimal point, and the indicator C with a brightness of its own; and it states no sound
  (`machine.md`, fit table).
- [ ] **AC-28** — Given the TI-59 and the TI-58, then their descriptors
  list the three instruction ROM files and the two constant files of
  that set, and the TI-58C's descriptor those of its set, as Calc-U-59
  `roms/calculator/files.txt` names them at the pin (`machine.md` Q-2;
  the form of a ROM entry and the md5 values: LIB-12).
- [ ] **AC-29** — Given any of the three models, then its descriptor
  lists one key ID for each key of the calculator and no key of the
  PC-100C (the list of IDs: C59-1); the connectors module slot (8 contacts) and
  printer connector (11 pads), each with a functional pin-out and taking
  one peripheral; the places `slot-1` and `printer`; and the debug
  target TMC0501 (`machine.md` and `peripherals.md`, fit tables).
- [ ] **AC-30** — Given the TI-59, then its descriptor lists the card
  reader as a pass-through media place with the sides 1 and 2; given
  the TI-58 or the TI-58C, then it lists no media place
  (`capabilities.md`, fit table).
- [ ] **AC-31** — Given any of the three models, then its descriptor
  lists its components with their modelling depth as the machine is
  built: which chips and blocks are separate, which are folded into
  another, and that the module slot and the printer connector are
  functional (`machine.md`, fit table, "Depth today"; ADR 0011, 0012).
- [ ] **AC-32** — Given the Calc-U-59 catalogue, then it holds a
  peripheral descriptor for the PC-100C, with its controls PRINT, ADV
  and TRACE and its paper, and one for each of the 14 Solid State ROM
  modules, each with its ROM file (`capabilities.md` and
  `peripherals.md`, fit tables).
- [ ] **AC-33** — Given a running machine of any of the three models,
  when it is compared with its descriptor, then its model, tick rate,
  display kind and geometry, key IDs, places and debug targets are the
  descriptor's, each media place of the descriptor shows as one media
  place capability, and every peripheral plugged in fits its place by
  the descriptor.

### Calc-U-1600
- [ ] **AC-34** — Given the Calc-U-1600 catalogue, then it lists the
  models PC-1500 (variants A01, A03, A04; default A04), PC-1500A (one
  variant, A04) and PC-1600 (variants New, Old; default New), with the
  tick rates 1,300,000 (PC-1500, PC-1500A) and 3,580,000 (PC-1600) per
  second (Calc-U-1600 `docs/User-Guide.md:82-88`,
  `Shared/MachineSession.hpp:369`, `Core/PC1500/PC1500Clocks.hpp:9`,
  `Core/PC1600/PC1600Clocks.hpp:8`).
- [ ] **AC-35** — Given the PC-1500 or the PC-1500A, then its descriptor
  states a dot-matrix display of 156 × 7 dots with 14 indicators; given
  the PC-1600, then 156 × 32 dots with 18 indicators; all three state
  sound (`machine.md`, fit table, which says 16 for the PC-1600; the
  code has 18, Calc-U-1600 `Core/Display/StatusLine.hpp:86-132`; the
  lists of key IDs and indicator names: C1600-1).
- [ ] **AC-36** — Given the PC-1500 or the PC-1500A, then its descriptor
  lists the 40-pin connector with the PC-1500 memory slot pin-out,
  taking one module, with the place `slot-1`; the 60-pin connector,
  taking a chain, with the places `printer` and `interface`; and the
  debug target LH5801. Given the PC-1600, then two 40-pin connectors
  with the pin-outs Slot 1 and Slot 2 and the places `slot-1` and
  `slot-2`; the 60-pin connector with `printer`, `interface` and
  `host-drive`; the RS-232C connector; and the debug targets SC7852 and LH5803. All
  pin-outs of the 40-pin and 60-pin connectors are contact level
  (`machine.md` and `peripherals.md`, fit tables, P-6).
- [ ] **AC-37** — Given any of the three models, then its descriptor
  lists each module slot as a holding media place for battery-backed
  modules, and its components with their modelling depth as the machine
  is built (`capabilities.md`, fit table; `machine.md`, fit table,
  "Depth today").
- [ ] **AC-38** — Given any of the three models and each variant, then
  its descriptor lists the ROM entries that variant needs; the list of
  files with their md5 values is LIB-12 AC-16.
- [ ] **AC-39** — Given the Calc-U-1600 catalogue, then it holds a
  peripheral descriptor for every peripheral the product offers today:
  the nine bundled memory modules (AC-41); the CE-150, with its paper
  and its cassette as a media place (which a machine lists only on the
  PC-1500 and PC-1500A, C1600-1 AC-16); the CE-158; and the CE-1600P (variants New, Old;
  default New), with its cassette and paper and, described with it as
  today, the CE-1600F with a holding media place for floppy disks with
  the sides A and B. Each lists its ROM files and the pin-outs it fits;
  the CE-161 lists its write-protect switch as a control
  (`peripherals.md` and `capabilities.md`, fit tables; P-3). The host
  drive is a peripheral of the PC-1600 in the place `host-drive`, with
  its ROM always bundled (`roms.md` Q-1).
- [ ] **AC-40** — Given a running machine of any of the three models
  and variants, when it is compared with its descriptor, then its model,
  variant, tick rate, display kind and geometry, key IDs, places and
  debug targets are the descriptor's, each media place of the descriptor
  shows as one media place capability, and every peripheral plugged in
  fits its place by the descriptor.
- [ ] **AC-41** — Given the bundled memory modules, then each fits
  these places, by its module definition (Calc-U-1600
  `Qt6/resources/cards/*.card.yaml`, `compatible-hosts`):

  | Module | PC-1500, PC-1500A `slot-1` | PC-1600 `slot-1` | PC-1600 `slot-2` |
  |---|---|---|---|
  | CE-151 (4 KB), CE-155 (8 KB) | yes | yes | no |
  | CE-161 (16 KB), CE-1638 (128 KB), CE-163F (256 KB), all battery-backed | yes | yes | yes |
  | CE-502B (ROM) | yes | no | no |
  | CE-1600M (32 KB, battery-backed) | no | yes | yes |
  | CE-1601M (64 KB), superRAM (512 KB), both battery-backed | no | no | yes |

## Edge Cases
- **EC-8** — Given a machine, when it is advanced by zero ticks, then
  it runs nothing and reports zero.
- **EC-1** — Given a model and a variant, when a machine is to be built
  and one of the ROM images its descriptor lists is not handed in, then
  no machine is built and the missing file is named.
- **EC-2** — Given a machine, when key down or key up is called with a
  key ID that is not in its descriptor, then the call is refused and the
  machine's state does not change.
- **EC-3** — Given a machine, when key down is called for a key that is
  already down, or key up for a key that is not down, then nothing
  changes.
- **EC-4** — Given emulated times of two machines with different tick
  rates, when they are compared or added, then this happens in seconds
  through the emulated-time type, never tick count against tick count.
- **EC-5** — Given a machine, when its capabilities are asked for a kind
  it does not have, then the answer is an empty list, not an error.
- **EC-6** — Given a model without variants (TI-59, TI-58, TI-58C), then
  its descriptor lists no variant and its machine reports none; given a
  model with a single variant (PC-1500A), then that variant is the
  default.
- **EC-7** — Given the PC-1600, whose components run on three clocks,
  then its tick is the T-state alone; the other clocks are converted
  inside the core and never appear above the machine.

## Technical Requirements (optional)
Invariants that cannot be checked by a test at this seam; they are
checked in review and in the phase 5 walkthroughs:
- Nothing in a machine reads wall time, starts a thread or holds a lock;
  a machine is used from one thread at a time (`machine.md` I-2, I-3).
- A core contains no ROM data (`machine.md` I-5).
- A capability never reaches the host: no files, no wall time, no
  threads (`capabilities.md` I-3).
- Refining a component to a different modelling depth changes the
  descriptor's depth entries, never the machine interface or the
  capabilities the machine exposes (`machine.md` I-4).
- The library names no product kind (`capabilities.md` I-1); AC-24
  shows the build side of this.

## Open Questions
None in this spec.

Settled in other specs: the lists of key IDs and indicator names
(C59-1, C1600-1); the form of a ROM entry (LIB-12).

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| The machine is the only seam; nothing above a core uses core types | ADR 0006; `machine.md` I-1 | 2026-11-09 |
| A small machine interface plus capabilities; power cycle joins reset and cold boot | ADR 0007, amended by `machine.md` M-6 | 2026-11-10 |
| One canonical tick per machine, a whole-number rate in the descriptor, one emulated-time type for every conversion | `machine.md` M-1 | 2026-11-10 |
| Display output comes in display kinds (segment, dot-matrix), both with named indicators | `machine.md` M-2 | 2026-11-10 |
| Keys are key IDs; a peripheral's keys belong to the peripheral | `machine.md` M-3 | 2026-11-10 |
| Keys held together reach the machine as they are | `typing.md` T-3 | 2026-11-10 |
| The descriptor is static; what is plugged in and present now is asked from the running machine | `machine.md` M-4 | 2026-11-10 |
| The machine lists its capabilities with kind, ID and owner | `machine.md` M-5 | 2026-11-10 |
| Product kinds are listed and handed out like generic kinds; the library never names one | `machine.md` M-7; `capabilities.md` C-1 | 2026-11-10 |
| Peripherals have descriptors; descriptors list places; a catalogue per product | `machine.md` M-8 | 2026-11-10 |
| A connector takes one peripheral or a chain; implicit connectors stay implicit until refined | `peripherals.md` P-3, P-6 | 2026-11-10 |
| A ROM entry is a file with name, size of the decoded content and md5 | `roms.md` R-1, R-4 | 2026-11-10 |
| This feature's product columns hold the descriptor content; the machines' behaviour is C59-1, C59-6 and C1600-1 | Rules 2 and 3 of the cut, `features/INDEX.md` | 2026-11-10 |
| Proposed in this spec, for the gate: the bound of AC-10 (one year of emulated time), EC-3 (a repeated key down or key up changes nothing), and that a capability ID is unique within a machine and repeatable (AC-21) | They make the criteria checkable; no contract states them | 2026-11-10 |
| The tick count never goes back for as long as a machine exists, also across reset, power cycle and cold boot; a new machine starts at zero | Recommendation applied; the user raised no objection at the batch 1 gate | 2026-11-10 |
| Sound output is samples at a rate named at the build, derived from the ticks alone | Recommendation applied; the user raised no objection at the batch 1 gate | 2026-11-10 |
| An advance runs at least the ticks asked for, in whole instructions; the caller carries the overshoot | Calc-U-59 today; Calc-U-1600 drops the overshoot today (C1600-1 OQ-6); recommendation applied, the user raised no objection at the batch 1 gate | 2026-11-10 |
| The catalogue holds the descriptors of the bundled module definitions; one read later has the same form | Recommendation applied; the user raised no objection at the batch 1 gate | 2026-11-10 |
| Brightness is the duty the core measures, from 0 to 1 | Recommendation applied; the user raised no objection at the batch 1 gate | 2026-11-10 |
| The lists of key IDs and indicator names are written in the C59-1 and C1600-1 specs, not left to phase 6 | Recommendation applied; the user raised no objection at the batch 1 gate; replaces the assignment of `presets.md` Q-3 | 2026-11-10 |
| Reset, power cycle and cold boot return at once and are over when the machine is ready to accept a key press; key readiness is part of the machine interface | user, 2026-11-10, batch 2 gate; amends `typing.md` T-2 | 2026-11-10 |
| The host drive is a peripheral on the 60-pin connector in a place of its own | user, 2026-11-10, batch 2 gate; amends `presets.md` PR-11 | 2026-11-10 |
| Only the display has an on and an off; it is part of display output. A machine reports no power state | user, 2026-11-10, batch 2 gate (prompted); answers `capabilities.md` Q-1 | 2026-11-10 |
