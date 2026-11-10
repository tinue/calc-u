# Contract: machine interface and descriptor

Status: draft (2026-11-10). Forks M-1 to M-6 are decided (decision log);
open questions remain.

Inherits: ADR 0006 (the machine is the only seam), 0007 (small interface
plus capabilities), 0008 (composition), 0009 (single-threaded sessions),
0012 (boundaries follow observable behaviour; depth in the descriptor),
0035 (no ROM data in a core). Related contracts: peripheral and connector
model (2), debugger (8), typing (4).

## Responsibilities

A machine:
- is one model (and variant) at run time, built by its product's core
  from components and from peripherals on its connectors (ADR 0008);
- offers, through the machine interface, what every machine has (ADR
  0007, amended by M-6): model and variant, reset, power cycle and cold
  boot, advancing emulated time,
  key down and up, display output, sound output, its connectors, its
  debug targets;
- exposes every other function as a capability of one of its components
  or plugged peripherals (ADR 0007);
- has a machine descriptor that states what the model is made of and how
  finely it is modelled (ADR 0012).

## Non-responsibilities

- No pacing, no wall time, no threads, no locks (ADR 0009): the machine
  advances only when called. Regular speed and full speed are the machine
  session's business.
- No files and no ROM lookup (ADR 0035): the app session hands the
  machine its ROM images; the descriptor only says which ROMs it needs.
- No typing strategy (contract 4): the machine takes key down and key up;
  how long a key is held, and sequences such as Shift or KBII, are above
  it.
- No rendering: display output is data, not pixels in a host format,
  glyph style or colour.
- No settings, presets, media files or save folder (contracts 5-7).
- No user messages.

## Invariants

- I-1: Nothing above a core uses core types or touches components
  directly (ADR 0006). There are no branches on the model above the
  machine.
- I-2: Emulated time advances only through the machine interface (ADR
  0009). Two machines built alike and fed the same calls produce the same
  outputs.
- I-3: A machine is used from one thread at a time; it holds no locks
  (ADR 0009).
- I-4: Refining a component (a different modelling depth) changes the
  descriptor's depth entries but never the machine interface or the
  capabilities the machine exposes (ADR 0012).
- I-5: A core contains no ROM data (ADR 0035).
- I-6: Switching the model builds a new machine (glossary, Machine).

## Machine descriptor

Static per model and variant; available before the machine is built (the
app session needs it to find ROMs and to build the GUI). What is plugged
in now and which capabilities are present is asked from the running
machine (M-4, M-5). Contents:
- model, model family, variant list and default variant;
- ROMs needed per variant (name, size, checksum) for the ROM lookup;
- the tick rate (M-1);
- display: kind, geometry and segment layout (M-2);
- keys: the list of key IDs (M-3);
- connectors: each with its form and the pin-outs it can carry (contract
  2);
- components: each with the chip(s) or block it stands for and its
  modelling depth (separate, folded into another component, functional);
- debug targets (contract 8).

## Open forks

### M-1: Unit of emulated time

**Decided (2026-11-10): option (a), one canonical tick per machine.**

Facts:
- TI-59/58: weighted instruction steps, IDLE steps weigh 4 on the
  TI-59/58 and 1 on the TI-58C; batches of 284 / 240 / 246 weighted steps
  per 20 ms (`docs/inventory/calc-u-59.md`, Pacing).
- PC-1500: LH5801 cycles at 1.3 MHz; PC-1600: SC7852 T-states at
  3.58 MHz, with the LH5803 on a 1.3 MHz clock (φOS) on the same machine
  (`docs/inventory/calc-u-1600.md`, Pacing;
  `Sharp1500-1600-Ref/PC-1600/PC-1600-Machine-Overview.md:48`).
- PC-1403 and PC-1350: SC61860 at 768 kHz (SC61860 knowledge base,
  machine overviews).
- Typing holds are wall time on Calc-U-59 (≥ 100 ms) and emulated frames
  on Calc-U-1600 (≥ 2 frames) (C11).

- Calc-U-1600 already has one canonical unit per machine although the
  PC-1600 has three clocks (T-states 3.58 MHz, φOS 1.3 MHz, sub-CPU CL2
  1.2288 MHz): T-states are canonical, the other clocks are converted
  inside the core (`Core/PC1600/PC1600Clocks.hpp`, `PC1500Clocks.hpp`).
- The TI-59 instruction rate is not a whole number on every model:
  TI-58C 12,312.5 instructions/s. The resonators are whole numbers
  (455 / 384 / 394 kHz; one instruction = 32 resonator periods, ×4 in
  IDLE on the TI-59/58) (Calc-U-59 `reference/CoreArchitecture.md`,
  Hardware Background).
- Buzzer and cassette are clocked by emulated time only: `PiezoSampler`
  and `TapeDeck` are credited CPU cycles and turn them into samples at
  `cpuHz / sampleRate` cycles per sample (`Core/Audio/PiezoSampler.hpp`,
  `Core/Tape/TapeDeck.hpp:20,150`). Pitch and duration therefore do not
  depend on how fast the host runs.
- Elapsed time "as on the real machine": Calc-U-59 divides the core's
  `elapsedTicks()` by the instruction rate (`ZeroElapseTime:` /
  `ReportElapseTime:`, `reference/StateFileFormat.md:281-282`).

Features that every option must keep (user, 2026-11-10):
- F-1 regular speed, measurably the same pace as the real machine;
- F-2 full speed;
- F-3 after a run at full speed, the time the real machine would have
  taken;
- F-4 audio, in particular cassette save and load, correct at full speed
  (no pitch or timing error).

All four rest on one property, whatever the unit: the machine counts its
own emulated time, exactly and without drift, and nothing inside the
machine looks at wall time. Pacing compares that count with the wall
clock (F-1) or does not wait (F-2); F-3 reads the count; audio and tape
derive their samples from it (F-4). The options differ only in where
the conversion to seconds happens and how exact it is.

Options:
- (a) **Recommended:** each machine counts in one canonical **tick**,
  the period of a real clock of that machine with a whole-number rate
  (TI-59: resonator period, 455 kHz; PC-1500: 1.3 MHz cycle; PC-1600:
  T-state, 3.58 MHz; SC61860: 768 kHz). The descriptor gives the rate.
  The interface advances by ticks and reports the ticks actually run (it
  stops on instruction boundaries) and a monotonic tick count. The
  library provides one emulated-time type (ticks plus rate) that converts
  to seconds for pacing, typing holds, F-3 reports and the harness, so
  nobody above converts by hand.
  - Pro: integer and exact, no rounding at the seam; both cores already
    work this way, audio and tape code stays as it is.
  - Con: a tick means something different per model; a caller that
    bypasses the library type could mix them up.
- (b) One common unit for every machine, such as emulated nanoseconds.
  - Pro: callers need no rate.
  - Con: no clock divides evenly (one T-state is 279.33 ns, one TI-59
    resonator period 2197.8 ns), so each core keeps a remainder at the
    seam and every core's time accounting changes; a value in ns that is
    then turned back into cycles rounds twice.
- (c) Frames: no natural frame on the TI-59; too coarse for audio.

Correction to the first draft: the PC-1600's several clocks do not make
(a) ambiguous; Calc-U-1600 already has one canonical unit for it.

### M-2: Shape of display output

**Decided (2026-11-10): option (b), display kinds**, with two additions
from the user: decimal points are part of a segment display (LED or LCD),
and a segment display is not limited to seven or eight segments. Other
segment layouts (for example 14- or 16-segment LCDs) exist in the family's
possible models, though in neither product today. The descriptor
therefore names a **segment layout** per display: the named segments of
one position (for the TI-59: seven segments and the decimal point) and
where they sit. Brightness per position is present where the hardware
has it (the TI-59's multiplexed LEDs) and absent otherwise.

Facts:
- TI-59/58: 12 LED positions; per position a glyph (digit plus control
  nibble) and a duty that `Shared/DisplayPov` turns into brightness; the
  Classic and Modernized glyph styles are drawn from segment data
  (`calc-u-59.md`, Display).
- PC-1500: 156 × 7 dots plus status-line indicators; PC-1600: 156 × 32
  dots plus a 16-symbol status line (`calc-u-1600.md`, Display;
  PC-1600 machine overview line 34). Calc-U-1600 already has a
  model-agnostic `shared::DisplayFrame`.
- PC-1403: 24 character cells of 5 × 7 dots plus status segments;
  PC-1350: 150 × 32 dots (SC61860 knowledge base).

Options:
- (a) One dot frame for every machine: the TI-59 would have to render
  its segments to dots in the core, losing the per-segment data the glyph
  styles need.
- (b) **Recommended:** a small set of display kinds, named in the
  descriptor: a **segment display** (positions × segments, with a
  brightness per position) and a **dot-matrix display** (width × height
  dots, optionally grouped into character cells). Both carry named
  indicators (status-line symbols, the TI-59's C). The PC-1403 is a
  dot-matrix display with cells, the PC-1350 one without.

### M-3: Identity of keys

**Decided (2026-11-10): option (a), key IDs.**

Facts: Calc-U-59 presses by matrix row and column
(`TI59Machine::pressKey(row, col)`); Calc-U-1600 by key name
(`PC1500Machine::pressKey(const std::string&)`). Keyboard maps, presets
and the harness all name keys. The PC-1500 ON key has its own call
(`setOnKeyPressed`). On the TI-59, PRINT, ADV and TRACE are keys of the
PC-100C, today signalled through the calculator's key matrix.

Options:
- (a) **Recommended:** keys are named by stable key IDs listed in the
  descriptor; the matrix stays inside the core. Keys of a peripheral
  (PC-100C PRINT/ADV/TRACE) belong to the peripheral, not to the
  machine's key list (follows ADR 0011).
- (b) Matrix coordinates: every user above needs the matrix of every
  model.

### M-4: Static descriptor and live state

**Decided (2026-11-10): option (a).**

Options:
- (a) **Recommended:** the descriptor is static per model and variant
  (what can exist); what is plugged in and which capabilities are present
  now is asked from the running machine.
- (b) One live description: the GUI cannot be planned before the machine
  is built, and the ROM lookup needs the ROM list before that.

### M-5: Reaching capabilities

**Decided (2026-11-10): option (a).**

Options:
- (a) **Recommended:** the machine lists the capabilities present now,
  each with its kind, an ID and its owner (component, or peripheral on a
  connector), and hands out the typed interface of that kind.
- (b) Callers walk connectors and peripherals and ask each for its
  capabilities: the app layer would need to know the machine's structure.

### M-6: Power cycle in the machine interface

**Decided (2026-11-10): option (a).** The user consented to the addition
to ADR 0007; the ADR carries a note.

Facts: ADR 0007 lists reset and cold boot. The glossary defines a power
cycle, which differs by model (TI-58/59 lose RAM, TI-58C keeps constant
memory, PC-1500/1600 keep all memory). Calc-U-1600 runs a power cycle
when a printer is attached or detached; Calc-U-59 has no off. Whether a
module change keeps RAM or cold boots is open for contract 2 (ADR 0021).

Options:
- (a) **Recommended:** power cycle is a third operation every machine
  has, next to reset and cold boot; what survives is the model's
  business. This adds to ADR 0007's list, so it needs the user's consent.
- (b) Power cycle stays inside the cores (for example as part of
  attaching a peripheral); the interface keeps reset and cold boot only.

## Fit table

Facts only until the forks are decided. Sources:
`docs/inventory/calc-u-59.md` and `calc-u-1600.md` (Modelling depth), the
SC61860 knowledge base (`~/Development/sharp/SC61860/ref/`, PC-140x and
PC-1350 machine overviews, `Shared/11-Pin-Port.md`, `Shared/Peripherals.md`).

| | TI-59 (58, 58C) | PC-1500 (1500A) | PC-1600 | PC-1403 (paper) | PC-1350 (paper) |
|---|---|---|---|---|---|
| Variants | none (models differ by ROM and RAM) | ROM A01/A03/A04 | ROM New/Old | three external-ROM versions, 0223/0229/0233; one dumped | ROM versions; version 0 not dumped |
| Tick (M-1) | resonator period: 455 kHz (TI-59), 384 kHz (TI-58), 394 kHz (TI-58C) | 1.3 MHz cycle | T-state, 3.58 MHz | 768 kHz | 768 kHz |
| CPU(s) and clock | TMC0501, one instruction = 32 resonator periods (×4 in IDLE on TI-59/58) | LH5801, 1.3 MHz | SC7852 (Z-80) 3.58 MHz + LH5803 1.3 MHz | SC61860A31, 768 kHz | SC61860A13, 768 kHz |
| Display (M-2) | segment display: 12 LED positions, segments A-G + decimal point, brightness per position; the C indicator lights segments A, F, E, D of the leftmost position with its own brightness, from another chip (`Shared/SevenSegment.hpp`, `reference/CoreArchitecture.md`, Display Subsystem). Today one glyph per position; M-2 lets C and minus show together as on hardware | dot matrix: 156 × 7 + status-line indicators | dot matrix: 156 × 32 + 16 status-line indicators | dot matrix: 24 cells of 5 × 7 + status segments | dot matrix: 150 × 32 |
| Sound | none emulated | piezo buzzer | piezo buzzer | buzzer via control port 5F | buzzer via control port 5F |
| Connectors | module slot (8 pins), PC-100C printer connector (11 pads) — both functional (ADR 0011) | 40-pin memory slot, 60-pin system bus | 2 × 40-pin slots, 60-pin system bus, RS-232C | 11-pin | 11-pin, 15-pin serial, RAM card slot |
| Built-in media | magnetic card reader (TI-59) | — | — | — | — |
| Depth today | coarse: SCOM, module, card reader, printer folded into `TMC0501` | CPU, RTC, keyboard separate; LH5811 and decoding folded into memory; LCD functional | two CPUs and arbiter separate; SC7852 blocks and LR38041 spread over memory/bank classes; sub-CPU functional | paper | paper |
| Debug targets | TMC0501 | LH5801 | SC7852, LH5803 | SC61860 | SC61860 |

## Open questions

- Q-1: The ROM version of the PC-1350 dump and how many PC-1403 versions
  a product would offer as variants (only version 3 is dumped).
- Q-2 (answered from the facts, 2026-11-10): the TI-59/58's constants
  are a section of the two SCOM ROM chips, not chips of their own;
  Calc-U-59 keeps them in separate files per chip
  (`roms/calculator/files.txt`: TMC0582, TMC0583, TMC0571B plus
  `TMC0582-CONST-K`, `TMC0583-CONST-K`; TI-58C likewise with CD2400,
  CD2401, TMC0573). The descriptor lists the ROM images a variant needs;
  whether the ROM lookup names images per chip or per file is for the ROM
  lookup contract (9).

## Decision log

| Date | ID | Decision |
|---|---|---|
| 2026-11-10 | M-1 | Emulated time is counted in one canonical tick per machine, the period of a real clock with a whole-number rate given in the descriptor; a library emulated-time type does every conversion to seconds. Keeps F-1 to F-4. |
| 2026-11-10 | M-2 | Display output comes in display kinds named in the descriptor: segment display (positions × named segments of a segment layout, decimal point included, any number of segments; brightness where the hardware has it) and dot-matrix display (optionally in character cells). Both carry named indicators. |
| 2026-11-10 | M-3 | Keys are identified by stable key IDs listed in the descriptor; the key matrix stays in the core. Keys of a peripheral belong to the peripheral. |
| 2026-11-10 | M-4 | The descriptor is static per model and variant (what can exist); live state (what is plugged in, which capabilities are present) is asked from the running machine. |
| 2026-11-10 | M-5 | The machine lists the capabilities present now, each with kind, ID and owner (component, or peripheral on a connector), and hands out the typed interface of that kind. |
| 2026-11-10 | M-6 | Power cycle joins reset and cold boot as an operation of every machine; what survives it is the model's business. ADR 0007 amended. |
