# Emulator Core Architecture

This document describes the C++ emulation core (`core/`) for the TI-59,
TI-58 and TI-58C. How the firmware uses the hardware (SCOM map, flags,
run loop, fast mode) is in `rom/TI59-ROM-Guide.md` and
`rom/TI58C-ROM-Guide.md`; everything above the hardware (pacing, debugger,
file formats) is in `docs/developer/App-Layer.md`.

---

## Component Overview

```
calcu::machine::Machine     (library/machine/Machine.hpp: the machine interface)
 └─ TIMachine               (core/TIMachine.hpp/.cpp)
      ├─ TI59Machine        (core/TI59Machine.hpp/.cpp, core-internal)
      │   ├─ ROM            (core/ROM.hpp/.cpp)
      │   ├─ RAM            (core/RAM.hpp/.cpp)
      │   └─ TMC0501        (core/TMC0501.hpp/.cpp; references ROM & RAM, uses TraceTypes;
      │                      sees the PC-100C and the module through their pin-outs only)
      ├─ PC100C             (core/PC100C.hpp/.cpp: the PC-100C in the place `printer`)
      ├─ SolidStateRomModule (core/SolidStateRomModule.hpp/.cpp: a module in `slot-1`)
      └─ the typing support (core/TITypingSupport.hpp/.cpp) and the machine state (core/TIMachineState.*)
tiCatalogue(), TIMachineBuilder (core/TICatalogue.hpp/.cpp: the three descriptors,
                             the PC-100C and the 14 modules)
```

The only interface above the core is
`calcu::machine::Machine`: `app/` builds a `TIMachine` through
`calcu::machine::build(tiCatalogue(), TIMachineBuilder, request)`
(`cu59::buildMachine`, `app/ProductRoms.hpp`, from the ROM lookup's `RomSet`) and reaches the model, the
advances and tick count, Reset / power cycle / cold boot, key readiness,
the keys and the display output through it only (CTest
`CalcU59CoreSeam`). Everything else goes through capabilities and kinds:
the debug log and snapshots read the kind `c59.program-memory`, the core
tests `TIMachineTesting`; the card reader is `TICardReaderPlace`, a media
place; `libExecPC` is `c59.calculator-state`'s; `core/TITypingSupport.hpp` holds the typing
support and `idleAndKeyDetectRange()`, a model's scan/detect
range (for `c59 coverage`, the run probe and the tests). For the debug
panel, what the debug target does not
give is four product kinds of the processor (`core/TIDebugKinds.hpp`,
`TMC0501/c59.…`): `c59.asm-overlay` (the overlay's words over 1800 to 1FFF,
its entry with COND set, whether it runs: from the entry until a Reset, a
Cold boot or `clear`; a Cold boot removes it), `c59.rom-heatmap` (counts
per ROM address 0000 to 13FF in `TIMachine::stepOne`, while on),
`c59.calculator-state` (`snapshotCPU`, `lastStepBoundary`,
`readCpuFrames`, `readROMKeycode`, the panel's frame ring) and
`c59.machine-log` (the debug events at their level). The debugger reaches the core
through the debug target (`core/TIDebugging.hpp`; see
"Debug target" below).
`TI59Machine` is the core-internal facade over the three hardware
components. The core holds no lock (see "Threading Model").

**Places.** The PC-100C and the Solid State ROM modules are
peripherals in the places `printer` and `slot-1`, attached and detached
through `Machine::attach/detach/replace` (`TIMachine::plug/unplug`)
or with the build request's `peripherals` (attached at the build, no power
cycle). Both pin-outs are functional and take one peripheral; an attach or
a detach answers that a power cycle follows, which the caller runs
(`AfterChange::PowerCycle`: the TI-58 and TI-59 lose program and data
registers, the TI-58C keeps its constant memory). `TMC0501` holds a
`TiConnectors` (`wire()`): a `PrinterPinOut*` with the digit its D0 pad is
on, and a `ModuleSlotPinOut*`; nullptr when nothing is plugged in. It keeps
no printer state and no module image. Every machine lists the capabilities
`TMC0501/typing-support`, `TMC0501/c59.trace-records`,
`TMC0501/c59.asm-overlay`, `TMC0501/c59.rom-heatmap`,
`TMC0501/c59.calculator-state`, `TMC0501/c59.machine-log`,
`RAM/c59.program-memory`, `TMC0501/c59.library-program` and
`TMC0501/machine-state`; the TI-58C also `RAM/c59.constant-memory`, the
TI-59 `card-reader/media-place` (`TIMachine::capabilities`). With the
PC-100C it lists two more, `printer/controls` (the library's `Controls`: PRINT and
ADV keys, TRACE switch) and `printer/paper-output` (the library's paper output
kind, `calcu::paper::PaperOutput`; the PC-100C derives from
`PaperOutputBase`, its description and dot rules are `core/PC100CPaper.hpp`).
TRACE's position comes with the
attach request (`PeripheralRequest::switches`) and stays through reset,
power cycle and cold boot; the app session keeps it across a model switch
and a detach. The machine tells both peripherals of each
reset, power cycle and cold boot (`TIMachine::cpuReset`).

**Ticks, operations, keys and display.** One instruction is 32
ticks of the tick rate (455,000 / 384,000 / 394,000 per second, the
resonator; `TICatalogue.cpp`), 128 in IDLE on the TI-59/58 (the step weight
× 32). `advance(n)` runs whole instructions until at least `n` ticks
passed; the tick count is `TIMachine`'s own and goes on across every
operation (`cpuReset()` carries it). `reset()` on the TI-59/58 saves the
RAM and the partition, resets the CPU and writes both back at the first
instruction that ends in the scan loop, before `isKeyReady()` is true;
`powerCycle()` resets the CPU (the ROM's start-up clears the RAM on the
TI-59/58; the TI-58C keeps it); `coldBoot()` zeroes the RAM first. A Reset on
the TI-58C is a power cycle. None of them runs instructions: the start-up
runs in the advances that follow. Keys are the 45 key IDs of
`core/TIKeys.hpp` (row and column of the keyboard; other spellings are the
tools' to resolve); `readDisplay()` turns the display snapshot into 12
positions of segments A–G and DP (index 0 rightmost) with the duty as
brightness and the indicator C (`core/TISegments.hpp`).

| File | Emulated chip | Role |
|---|---|---|
| `ROM.hpp/.cpp` | TMC0582/0583/0571B, CD2400/2401/TMC0573 | Instruction ROM (13-bit words) plus the debug overlay |
| `RAM.hpp/.cpp` | TMC0598 | User RAM (120 × 16 BCD nibbles) |
| `TMC0501.hpp/.cpp` | TMC0501 + SCOM, card reader | CPU; full instruction execution; the printer and module words go to the pin-outs |
| `PrinterPinOut.hpp`, `ModuleSlotPinOut.hpp` | — | The printer connector's and the module slot's functional pin-outs |
| `PC100C.hpp/.cpp` | PC-100C | The print cradle: character tables, buffer, busy count, dot timing, PRINT/ADV/TRACE |
| `SolidStateRomModule.hpp/.cpp` | TMC0540 family (TMC0541 …) | A module: 5000-byte image, address register |
| `PC100CPaper.hpp/.cpp` | PC-100C | Its paper: description (unit 0.430 mm, 129.07 units wide), 5×7 font, character table, dot rules of a cut-short line, placement of a line's dots |
| `TIDebugging.hpp/.cpp` | — | The debug target `TMC0501`, the run control's checks, `c59.trace-records` |
| `TraceRecords.hpp` | — | The product kind `c59.trace-records`: the frame ring for a trace |
| `TI59Machine.hpp/.cpp` | — | Core-internal facade; variant setup |
| `TIMachine.hpp/.cpp` | — | `calcu::machine::Machine`: ticks, operations, Reset restore, key readiness, keys, display output |
| `TICatalogue.hpp/.cpp` | — | `tiCatalogue()` (models, PC-100C, 14 modules with their `<ID>.rom.yaml`), `TIMachineBuilder`, `TIConstantMemory` |
| `TITypingSupport.hpp/.cpp` | — | The typing support (`typing-support`) and the model's key-detect range |
| `TIMachineState.hpp/.cpp`, `TIStateRecords.hpp` | — | The library's `machine-state` on the TI models: tagged records, each part's fields in its `stateIo` |
| `TIKeys.hpp/.cpp`, `TISegments.hpp`, `TIRomRanges.hpp`, `TIRomText.hpp/.cpp` | — | Key IDs; glyph rules; scan-loop, idle and card-wait ranges; ROM text decoding |
| `TIMachineTesting.hpp` | — | Test-only reads inside the core; the PC-100C's code lines (`printedCodeLines`, for the golden runs) |
| `MachineVariant.hpp` | — | `TI59 \| TI58 \| TI58C` (core-internal), `hasConstantMemory()`, `hasLargeMemory()` |
| `TraceTypes.hpp` | — | `CpuFrame`, `DebugEvent`, `TIStepBoundary`, `TRACE_*` flags |

### Two different "PCs"

| Term | Meaning |
|---|---|
| **ROM address** (CPU PC, `addr`) | The 13-bit address the TMC0501 is executing. Never user-visible. |
| **Calculator step** | The step (0–959) of the user's keystroke program, shown in LRN. Kept by the firmware in SCOM[0] nibbles 4–7 (RAM programs) or in the module's own counter (solid-state programs, see "Solid-State Library Module"). |

One calculator step typically runs dozens to thousands of ROM instructions.

---

## Hardware Background

The TI-59 family uses a **4-bit BCD digit-serial architecture**: one digit
per clock, so one instruction takes 16 digit times.

```
TI-59:      455 kHz ÷ 2 (two-phase) ÷ 16 → 14,219 instructions/s
TI-58:      384 kHz ÷ 2 ÷ 16             → 12,000 instructions/s
TI-58C:     394 kHz ÷ 2 ÷ 16             → 12,312.5 instructions/s (one unit's clock; see "TI-58C clock")
idle (display) mode: ÷ 4 more on the TI-59/58; full speed on the TI-58C
```

Sources: Hynek Sladký, *HW guide for calculators TI-58/59* (2014), p. 6 —
instruction cycle ≈ 70 µs in calculate mode, ≈ 280 µs in display mode; the
TP0240 clock chip page (http://www.datamath.org/Chips/TP0240.htm) — 455 kHz
(TI-59) and 384 kHz (TI-58) ceramic resonators, ±1%, ÷ 4 while /IDLE is
active. The TI-59 rates are confirmed on a real TI-59: videos (see "Printer
line in the idle scan") and a logic analyser on its clock chip, whose PHI1/PHI2
pins run at f/2 (226.6 kHz) while active and f/8 (56.7 kHz) while idle, for a
453.3 kHz resonator. The rates are the catalogue's tick rates `kTI59Rate`,
`kTI58Rate` and `kTI58CRate` (`core/TICatalogue.cpp`); an instruction is
32 ticks.

### TI-58C clock

The TI-58C has no clock chip and no resonator on its board, so it cannot use
the TP0240 scheme above. It runs on the **clock generator built into a
TMC0580 DSCOM**: its CD2400 and CD2401 are TMC0580 dies
(http://www.datamath.org/Chips/TMC0580.htm), mounted piggyback with their
pins in parallel: on unit B every pin of both packages is soldered through,
none bent away (visual inspection), pin 14 included. Probes on that unit
confirm the oscillator.

- **The oscillator.** The TMC0580 has a two-phase clock generator on an
  extra bond pad (CLK), specified for PHI1/PHI2 up to 230 kHz. The
  frequency is set by external parts on CLK: a resistor to VGG alone (an
  internal capacitor sets the time constant), a resistor to VGG plus a
  capacitor to VSS, or a PZT resonator to VSS.
- **The cost: D0.** In the 28-pin package the CLK pad and digit line D0
  share pin 14, so a chip bonded for its oscillator has no D0. On unit B
  the stack's pin 14 carries no digit pulses but the oscillator's timing
  node. **PHI = oscillator ÷ 4**: captured together, every PHI1 period
  holds exactly four pin-14 cycles. A probe on pin 14 loads the node and
  halves the rate, so the free-running oscillator (≈ 4 × PHI ≈ 790 kHz)
  cannot be observed directly. The TI-59/58 sense the PC-100 printer
  cradle by a diode between D0 and KP. Without D0 the TI-58C ROM moved
  that test to D10 and rebuilt its idle loop around it
  (`rom/TI58C-ROM-Guide.md`, "The printer-present line moved to digit 10").
- **PHI reaches the other chips as usual.** The third ROM, the TMC0573, is
  an 8-pin BROM (http://www.datamath.org/Chips/TMC0560.htm) whose pins 6
  and 7 are the PHI2/PHI1 inputs; they carry the same PHI as pin 4 of the
  printer connector.
- **No idle slow-down.** On the TI-59/58 the ÷ 4 idle divide is done by the
  TP0240, which watches /IDLE. The TMC0580's oscillator has no such
  divider, and the TI-58C runs idle instructions at full speed.

**Rate per unit.** An R or RC oscillator sets no standard frequency:

| Unit | PHI | Measured |
|---|---|---|
| A | 194.7 kHz | scope on PHI; error blink 0.9185 s stand-alone, 0.7234 s on the PC-100C |
| B | 196.1–197.8 kHz | logic analyser on PHI1 (printer connector pin 4), five readings over two days; error blink 0.9090 s stand-alone, 0.7140 s on the PC-100C (one session, PHI 197.2 kHz) |
| C | ≈ 200 kHz | error blink 0.896 s only |

All are within the 230 kHz PHI limit. The TI-59 needed a resonator so that
a card written on one calculator reads on another; the TI-58C has no card
reader.

**Stability (unit B).** The rate wanders by about 0.8% between readings
taken hours or days apart (PHI1 196.14 to 197.8 kHz) but holds within a session: lighting all ten
digits slows it by only 68 ppm, and it drifts about 40 ppm over 8 s. It
needs a clean supply: on the charger without its battery pack, the display
shows deep 100 Hz ripple and the error blink slows by 0.3%. Measure with
the pack in, or with a bench supply on the battery terminals.

**Emulator model.** The emulator keeps a base clock for every variant,
because the rest of the timing code derives from it (instruction rate =
base ÷ 32). For the TI-58C that base is 2 × PHI, not a frequency found on
the board. `kTI58CRate` is 394 kHz (12,312.5 instructions/s), unit B's
rate from its error blinks with and without the printer.

**Accuracy.** The emulator runs the TI-58C **instruction-exact**. On unit B,
PHI1 cycles counted between IDLE events (÷ 16 = instructions) match the
emulator to the instruction in every block of this loop, in every cycle:

| Block (program `2 STO 00`, then the loop) | Instructions |
|---|---|
| `Pause Pause` (start to start) | 2,032 |
| 20 × `Nop`, `Pause` | 6,256 |
| 10 × `RCL 00`, `Pause` | 16,416 |
| 10 × `STO 00`, `Pause` | 16,432 |
| 10 × (`x²` `√x`), `Pause` | 16,720 |
| `GTO 000`, `Pause` | 3,392 |

Each `Pause` holds the display for exactly 1,489 IDLE instructions, on the
unit and in the emulator. The error blink takes 11,200 instructions per
period stand-alone and 8,800 on the PC-100C; both blinks of unit B give the
same rate. So **a TI-58C rarely matches a live unit 100% in seconds**: the
instruction counts agree, but the unit's clock of the day differs from the
emulator's fixed 394 kHz, and between units by several percent.

To compare a program with a real TI-58C:
- **Exact:** count PHI1 cycles (printer connector pin 4) between edges of
  IDLE (also on the printer connector) and divide by 16. IDLE pulses about
  5 µs low once per instruction and stays low for a whole instruction while
  the CPU is idle, so each `Pause` or idle phase is a run of long lows.
  Clock, drift and supply drop out.
- **On video:** film the error blink in the same session and power setup,
  take the unit's rate from it (11,200 or 8,800 instructions per period),
  and compare the program's times at that rate.

**Not yet known:** whether one or both dies wire their CLK pad to pin 14
inside the package (the shared pin carries no D0 pulses, so neither die
drives D0 there), which parts set the rate, the nominal rate, and what moves it
between sessions (temperature, supply voltage, battery state).

`step()` returns a **cycle weight** — 4 for an IDLE instruction on the
TI-59/58, otherwise 1 (`getStepWeight()`) — so callers can pace in real time.

---

## CPU Registers (TMC0501)

### BCD data registers — 16 × 4-bit nibbles each

| Register | Purpose |
|---|---|
| `A` | Accumulator; `A[2..13]` drives the 12 display digits |
| `B` | Accumulator; `B[2..13]` holds the display-control nibbles |
| `C`, `D` | Working registers |
| `E` | Only reachable through `A<>E` exchanges |

Number layout: nibble 0 = flags (bit 1 mantissa sign, bit 2 exponent sign),
nibbles 1–2 = exponent, nibbles 3–15 = mantissa (MSD in 15). The firmware's
number formats: `rom/TI59-ROM-Guide.md` § "Number formats".

### SCOM registers (TMC0571)

16 × 16-nibble scratch registers inside the CPU model (`SCOM[row][nibble]`),
not user-accessible and never persisted: `reset()` clears them, and on the
TI-58C the power-on code rebuilds partition and Fix from E000
(`rom/TI58C-ROM-Guide.md` § "Constant memory"). The firmware's use of every
row is in `rom/TI59-ROM-Guide.md` § "SCOM register map". The rows the C++
code and the debugger decode:

- **SCOM[9]** — library/partition bookkeeping: nibble 15 = list-data flag,
  nibbles 6–5 = current library program ("current page"), 4–3 = pending
  `Pgm nn` selection ("new page"; read by `insertedModuleNumber()`), 2 =
  module security code (TI-59 memory protection), 1 = number of RAM chips
  (TI-58C: high nibble of E000's address), 0 = program-partition registers
  ÷ 10 (set by Op 17, read by `partitionProgramRegs()`).
  `setPartitionProgramRegs()` writes nibble 0 and SCOM[13] nibbles 8–9, as
  Op 17 does.
- **SCOM[14]/[15]** — the 6-level subroutine return stack: SCOM[15] holds
  levels 1–3, SCOM[14] levels 4–6, and SCOM[15] nibble 0 the number of
  pushed levels (F after the error-halt reset at ROM `11E1` = empty; the
  next push restarts at depth 1). Each level is 5 nibbles, from its lowest
  nibble upward: the caller's PRG SOURCE flag, then a 4-digit BCD position
  LSD-first — RAM register · 10 + byte for RAM-program and keyboard callers,
  or the CROM address for library callers. Pushed by every calculator-level
  call (`SBR`, and launches of keycode sub-programs such as P→R; push
  routine at ROM `0D50`). Example: a keyboard caller at step 008 pushes
  source 0, position 0010, giving SCOM[15] = `…001001`.

The SCOM[9] and SCOM[14]/[15] layouts follow the SCOM register table in
Sladký's *Calculators TI-58/59 HW programming guide* (not in this
repository), trace-verified in `rom/traces/sbr444-quirk-analysis.md`.

### Constant ROM

`m_constant[64][16]`, loaded from the two ROM chips' `*-CONST-K.txt` files
(32 rows each, `app/MachineModelInfo.hpp`). Rows 0–15 are the numeric
constants of the transcendental functions; rows 16–63 hold the 384 keycodes
of the keycode sub-programs (PRG SOURCE 8, `romKeycode()`). An ALU
`…CON` operand reads the row selected by KR bits 4–6 and 8–11; the ROM walks
it with `INC KR`. Contents: `rom/TI59-ROM-Guide.md` § "Transcendental
functions" and `rom/TI59-keycode-programs.asm`.

### Control / address registers

| Register | Width | Purpose |
|---|---|---|
| `KR` | 16 bit | Key register: scan result, jump target (bits 15–4), constant-row index, PREG trigger (bit 1) |
| `SR` | 16 bit | Return-address parking; `SWAP KR,SR` is the ROM's call/return mechanism |
| `fA`, `fB` | 16 bit | ROM-visible flag registers; meanings in `rom/TI59-ROM-Guide.md` § "Flags" |
| `EXT` | 16 bit | External data latch (card reader / library module); valid one cycle after `CRD.IN`/`LIB.*` |
| `PREG` | 13 bit | Computed-jump latch (see "Computed jump (PREG)") |
| `Sout` | 16 × 4 bit | ALU output / IO bus; carries SCOM and RAM addresses and data |
| `R5` | 4 bit | Scratch nibble: result digit at the field's constant position after every ALU op, `MOV R5,#n`, decimal-point position |
| `digit` | 4 bit | Digit counter, 15 → 0 → 15, one step per instruction |
| `addr` | 13 bit | Program counter (0x0000–0x1FFF) |
| `flags` | 16 bit | Emulator-internal state (`FLG_*` in `TMC0501.hpp`): IDLE, HOLD, COND, pending memory operations, validity windows, printer BUSY |

---

## Memory Map

### ROM

All variants load 6144 × 13-bit words (0x0000–0x17FF):

- **TI-59 and TI-58**: the same image, `TMC0582` + `TMC0583` + `TMC0571B`.
  The TI-58 runs the identical firmware (card code included) and differs only
  in RAM size, which the ROM autodetects (see below).
- **TI-58C**: `CD2400` + `CD2401` + `TMC0573` (constant memory,
  `MEMWR`/`MEMRD`). Differences: `rom/TI58C-ROM-Guide.md`.

Each DSCOM holds 2,560 words and the BROM 1,024, so the image splits
0000–09FF (DSCOM at chip select 0), 0A00–13FF (second DSCOM), 1400–17FF
(BROM 6). The CPU sends the address serially on EXT every cycle, the top
three bits as the chip select; each chip's select code is mask-programmed,
and only the matching chip drives IRG
(http://www.datamath.org/Chips/TMC0580.htm). The `roms/` dumps
(datamath's ROM recorder, one file per chip) put 0000–09FF in CD2400 and
0A00–13FF in CD2401. The recorder plays the CPU, so a chip read on its own
answers only its own range; if the stacked pair was read in place, the two
names could be swapped. Either way the image is correct by address, which
is all the emulator uses.

`ROM::load` also accepts 5120 words (`TI58_WORDS`), which no variant uses.
`read()` masks to 13 bits and returns 0 beyond the image. Addresses
0x1800–0x1FFF can hold debug ASM overlay words (`loadDebugOverlay`;
`docs/developer/Debug-API.md` § "ASM Overlay").

### User RAM (TMC0598)

120 registers × 16 BCD nibbles (`RAM::setLimit` caps the TI-58 at 60, the
TI-58C at 64). Program steps fill registers from the bottom (2 nibbles per
step, units digit first, 8 steps per register); data registers count down
from the top, Rnn = RAM[top − nn]. The ROM keeps top as BCD in SCOM[13]
nibbles 5–3: 119 (TI-59) or 59 (TI-58/58C); `TI59Machine::dataRegCount()`
is top + 1 and `readDataReg`/`writeDataRegister` use it. The
partition (SCOM[9] nibble 0 = program registers ÷ 10) is set by Op 17; the
defaults are 479.59 (TI-59) and 239.29 (TI-58/58C).

- **STO/RCL reach only 00–99.** On the TI-59 the lowest 20 registers
  (RAM[0..19]) can only ever be program steps.
- **Op 17 erases nothing.** Steps and registers share the same cells from
  opposite ends; moving the boundary only changes how a cell is read, and
  moving it back restores the old contents.
- **Memory-size autodetection.** The hardware has no model identifier. The
  reset routine (`rom/TI59-commented.asm` `0377`) writes register 90 and reads
  it back. On the 60-register TI-58 the write is out of range
  (`RAM_ADDR < ram.size()` fails) and reads back 0. With the read-back in B it
  issues `RAM.OP` op 2 at `0385`, which clears the RAM (§ "Two-Cycle Memory
  Operations"). The compare at `0390` and the branch at `0393` then select the
  60-register layout. The TI-58 and TI-59 reset traces differ only there.

### TI-58C Extra (Constant Memory) Registers

The TI-58C has **60 addressable registers, exactly like the TI-58**, plus 4
raw RAM cells `RAM[60]`–`RAM[63]` outside that space. Do not call them
"registers 60–63". They:

- are not reachable via `STO`/`RCL` in the TI-58C's own partitions (top =
  59). A partition hacked like a TI-59's (`examples/historic/repartition.ti58c`, TI
  PPC Notes V5N7) sets top = 119; the cells are then R59–R56 (RAM[119 − nn]),
  and the debugger shows them as such;
- are read and written only by `MEMWR`/`MEMRD`, which address raw RAM;
- persist with the 60 normal registers in the constant memory file
  `ti58c.memory.yaml` (constant memory format 1, `docs/formats/media.md` §4;
  `R060`–`R063`), which is read at a build before the processor starts
  (`TIConstantMemory`), and are written by a preset's `data:`
  (`H00`–`H03`) and by the debugger.

RAM[60] is the firmware's system register (signature, partition, Fix):
`rom/TI58C-ROM-Guide.md` § "E000: the system register".

**Naming:** **E000–E003** for the raw RAM cells (code comments about raw
indices), **H00–H03** for the same cells as preset or debugger variables
(a preset's `data:` keys `HNN`, `docs/formats/preset.md` §4.2; the debugger's
`H##` labels). The
raw base 60 is `cu59::kExtraRegisterBase` (`app/MachineModelInfo.hpp`);
everywhere else tags extra registers explicitly (`DataKey::extra`,
`ProgramMemory::writeExtra`) instead of testing a register number ≥ 60,
because normal TI-59 registers go up to 99.

**The product kinds of memory** (`core/TIProductKinds.hpp`, implemented in
`core/TIMemoryKinds.*`): `c59.program-memory` (capability
`RAM/c59.program-memory` on every model: program steps, data registers,
H00–H03, the partition, raw registers; what a preset's sections and the
`c59` harness commands write) and `c59.constant-memory`
(`RAM/c59.constant-memory`, TI-58C only: the 64 raw registers and a counter
that changes with them).

The ROM's own `MEMWR`/`MEMRD` addressing is uniform over 0–63, and the core
models it that way (`RAM::setLimit(64)`, guarded only by
`RAM_ADDR < ram.size()`). The naming rule is for the layers above.

---

## Instruction Set Summary

Opcodes are 13 bits. Bit 12 selects branch or non-branch. Mnemonics follow
`tools/mnemonics.tsv` (as printed in `rom/*.asm`).

### Branch (bit 12 = 1)

```
[ 1 | COND | offset (bits 10:1) | dir ]
 bit 12  bit 11                  bit 0
```

- Taken when opcode bit 11 equals `FLG_COND` (also bit 11 of `flags`):
  `!((flags ^ opcode) & FLG_COND)`.
- **offset**: 10-bit magnitude, added (dir 0) or subtracted (dir 1).

**COND is active-low.** It rests at `1`; a satisfied test (`TST`, `KEY`,
ALU carry) drives it to `0`. So `JC` (cond bit 1) is taken when COND is at
rest and `JNC` when a test hit: `TST fB[x]; JNC handler` reads "if `fB[x]`
is set, branch". "Clear COND" in the listing means *restore it to 1*.

Every branch sets `FLG_JUMP`; the next non-branch instruction clears it and
restores COND to 1 (the hardware's restore between branch chains).

### Non-branch classes (bits 11:8)

| Hi nibble | Class | Instructions |
|---|---|---|
| `0x0` | Flag ops | TST/SET/CLR/INV/MOV/CMP on `fA`, `fB` bits; SET/CLR `KR` bits |
| `0x8` | Keyboard | `KEY […]` (test row), `KEY […] ALL` (scan) |
| `0xA` | Control / peripheral | WAIT, SET/CLR.IDLE, INC KR, STOF/RCLF, card, printer, RAM, library |
| other | **ALU** | add/subtract/shift over a digit field, register moves and exchanges |

### ALU opcode layout

```
[ 0 | ffff | ooooo | ddd ]
      11:8    7:3    2:0
```

- `ffff` — field mask (digit range DPT, EXP, MANT, ALL, …, `mask_info`)
- `ooooo` — operation and source pair (0–31; 24–31 are constant, SCOM/RAM
  recall and R5 forms)
- `ddd` — destination: A, IO only, A↔B, B, C, C↔D, D, A↔E

Digit 0 is hexadecimal (wraps at 16), digits 1–15 are decimal. `Sout` gets
the result inside the field and 0 outside it. A carry out of the field
clears COND.

---

## Two-Cycle Memory Operations

The first instruction latches an address and sets a flag; the next ALU
instruction moves the data, through `Sout` (the IO bus). If that instruction
does not drive IO, the bus reads 0 and 0 is written.

### SCOM store / recall

```
1. STOF / RCLF → REG_ADDR := Sout[0], FLG_STORE / FLG_RECALL
2. ALU         → store: Sout → SCOM[REG_ADDR]; recall: SCOM[REG_ADDR] is the operand
```

### RAM (TI-59/58: `RAM.OP`, opcode 0x0AF8)

```
1. RAM.OP → FLG_RAM_OP
2. ALU    → op := Sout[0] (0 read, 1 write, 2 clear all, 4 clear 10),
            RAM_ADDR := Sout[3]·10 + Sout[2] (Sout[3] may be hex: registers 100–119)
3. ALU    → read: RAM[RAM_ADDR] is the operand; write: Sout → RAM[RAM_ADDR]
```

**Op 2 clears the whole RAM, whatever the address.** The ROM issues it only in
the reset routine (`0385`, address 90). A real TI-59 lost all program steps and
registers and kept t when a program reached that code (fast-mode entry,
`rom/fastmode.md` § "The LRN entry"). This is an inference from that test, the
ROM having no other clear at power-on. `TI_58_59-HW-manual.pdf` calls op 2 "clear
1 RAM register", but that table is itself reverse-engineered, and the ROM never
shows the difference elsewhere. The TI-58 has the same CPU and RAM chips and is
assumed to behave the same; there the address is even out of range.

So a CPU reset followed by the ROM's boot clears the RAM on the TI-59 and
TI-58, as switching on does: that is `powerCycle()`, and `coldBoot()` is the
same from zeroed RAM. The Reset button, which the real calculator doesn't
have, keeps the memory inside the machine: `TIMachine::reset()`
saves the RAM and the partition and puts them back when the boot reaches the
scan loop. The boot runs in the run loop's advances, so CPU
FREEZE ON START and a trace see the reset routine on a Reset as on a clean
reset (`tests/part_flows_tests.cpp`). The TI-58C's ROM
has no `RAM.OP`: its constant memory stays through a reset, and its clean reset
zeroes the RAM.

### RAM (TI-58C: `MEMWR` 0x0A76, `MEMRD` 0x0A86)

```
1. MEMWR / MEMRD → RAM_ADDR := Sout[1]·16 + Sout[0] (binary), FLG_RAM_WRITE / FLG_RAM_READ
2. ALU           → write Sout into RAM[RAM_ADDR] within this instruction's field / read as operand
```

The opcodes are decoded on every variant; the TI-59/58 ROM never emits them.

### Computed jump (PREG)

```
1. SET KR[1] → at the end of this step: PREG := KR[15:4] | KR[0] << 12, KR[1] cleared
2. next instruction (delay slot) executes, then addr := PREG, PREG := 0
```

HOLD outranks PREG: a held instruction keeps the redirect pending. A branch
in the delay slot defers it to the next non-branch instruction. `PREG = 0`
counts as "no jump" (see `rom/TI58C-ROM-Guide.md` § "Emulator issues found").

---

## Execution Flow (`TMC0501::step()`)

1. **Printer countdown** — with a PC-100C, one Φ1 on the printer connector
   (`PrinterPinOut::instructionCycle`): the PC-100C decrements its busy
   count and at zero commits the line with all 7 dot rows; the CPU copies
   the connector's Busy into `FLG_BUSY` (kept in the flags word for the
   trace format).
2. **`beginNextStep()`** — fetch the opcode, restore COND after a branch
   chain, patch the previous trace frame to post-execution state, start the
   new frame (breakpoint check).
3. **Digit counter** — 15 → 14 → … → 0 → 15.
4. **HOLD** — clear `FLG_HOLD`; `WAIT`/`KEY … ALL` re-assert it.
5. **Validity windows** — `EXT` and `Sout` survive one cycle after they are
   written, then read as 0.
6. **Decode and execute** — a branch sets `addr` and skips steps 8 and 10.
7. **Display strobe** (`postOperation()`) — see "Display Subsystem".
8. **Advance** — unless HOLD: PREG redirect if pending, else `addr++`.
9. **Accounting** — weight, trace post-step, C-indicator and poll counters,
   `elapsedTicks()` (since the last CPU reset; `TIMachine::tickCount()` is
   32 × it plus the count before the reset).
10. **PREG latch** — if `KR[1]` is set.
11. **Step boundary** (`noteStepBoundary()`) — see below.

The return value carries the weight in `kWeightMask` and two bits:

- **`kPolledHardware`** — a backward branch that retries a `KEY` or
  `TST.BUSY` poll executed just before it: a spin-wait on a key, the card
  switch and so on, whose real duration does not depend on CPU speed. A
  `TST.BUSY` retry counts only while the KR line is active and the printer
  is not busy. Nothing above the core reads the bit: at full speed the
  library's pacer (`pacing/Pacer`) runs a machine that waits for input at
  regular speed.
- **`kStepBoundary`** — the next instruction is the interpreter's step
  dispatch, `stepDispatchPC()` = `0837` (TI-59/58) or `082B` (TI-58C), which
  every executed program byte (keycode or operand) passes once, from RAM,
  the module or the keycode table. The CPU state there, and on idle entry
  (`kIdleEntryPC` = `0624`, no bit), is latched as `lastStepBoundary()` for
  the debugger's CALCULATOR view (`docs/developer/Debug-API.md` §
  "Frozen-display semantics").

`TI59Machine::step()` adds `kBreakpointHit` (the core's own breakpoints,
`TRACE_BREAKPOINTS`; the harness's `break` and the tests only).

**Debug target (`core/TIDebugging.hpp`).**
`TIMachine::debugging()` gives `TIDebugging` with one target, `TMC0501`
(`TIDebugTarget`): the registers of the processor frame (`A`–`E` as 16
digits, `KR`, `SR`, `fA`, `fB`, `flags`, `R5`, `digit`, `EXT`, `PREG`, `pc`;
read only), the spaces `rom` (13-bit words with the overlay range), `ram`
(16-digit registers), `scom` and `program` (program steps 00–99; `ram` and
`program` take writes), a history (the frame ring, newest first) and
decoding; no watches, no step over or out. While the run control's
`Checks` are off, `TIMachine::advance()` takes the fast path. While they
are set it stops before an instruction at a breakpoint (passed once after a
pause there) and asks the observer after each instruction with
`InstructionEvent::marks`: bit 0 the next instruction is a step boundary
(`kStepBoundary`), bit 1 it lies in the keyboard scan loop, bits 8–15 the
instruction's weight (1 active, 4 IDLE). When the observer stops the
advance, the next instruction's pre-execution phase runs at once
(`beginNextStep()`, idempotent), so the state read while paused is the next
step's. With `setRunOn` set (the app session: while a pause or step waits
to land), an advance goes on past the ticks asked for until the observer
stops it, at most 50000 weights later, so a pause lands in the host call
that asked for it. The levels (`program`, `cpu`)
and what lands where are the app's `TILevelRules`
(`docs/developer/App-Layer.md`). The frame ring is on while the debug
panel (`c59.calculator-state`'s `setFrameRing`), a trace
(`c59.trace-records`) or the run control's checks want it.

---

## Display Subsystem

In IDLE, each instruction strobes one position: the position the digit
counter is about to reach, 13 → 2 (`postOperation()`). `A` gives the digit,
`B` the control nibble (digit, minus, degree, double quote, apostrophe, blank;
glyphs in `app/SevenSegment.hpp`). The leading-zero
suppression circuit is modelled per strobe (`suppressedMask`). While it is
active and the digit is 0, nibbles 2 and 4 stay dark and nibble 5 shows a minus
instead of the degree sign; a leftmost 0 stays dark with any nibble. Normal
displays use neither: these rules come from the pictures of a real TI-59 in
`control_nibble_glyphs` (`tests/core_tests.cpp`). A strobe that
shows a glyph latches it and adds the instruction's weight to the position's
driven time; the decimal point of the position `R5` names is driven the same
way. Nothing is driven in RUN mode, so the display goes dark while the
calculator computes.

`takeDisplay()` (inside the core; above it `TIMachine::readDisplay()`) returns per position the
last latched glyph and its **duty**: the driven time since the previous call
× 16 (a position is strobed one instruction in 16), so a steadily multiplexed
display reads 1.0 and a computing one 0. `pollTicks` is the interval in
weighted cycles; a poll with no cycle since the last one returns the last
snapshot again. Each call restarts the window, so a core has one display
consumer (`FrameSampler`, through `readDisplay()`). The core makes no visibility decision: brightness and
persistence of vision are `DisplayPov` (`app/FrameSampler`).

The **C indicator** (`calcIndicator`) is its mean drive over the interval: the
number of set `fA` bits in RUN mode, `fA` bit 14 in IDLE (Sladký). That RUN
counts bits rather than "any `fA ≠ 0`" comes from a film of a real TI-59 running
`Pgm 01 SBR =`; the serial output of the flags is the likely cause.

**Limitation:** the CPU's digit and the display strobe share one counter, so
they are always in phase. A program that shifts `SET IDLE` out of phase on
purpose (`examples/assembly/Decoder.asm`) shows other positions on real
hardware but normal digits on the emulator.

**Limitation:** on a real TI-59 the C comes from the main processor and the other
segments of the leftmost position from the serial ROM chip, so a weak C and a lit
minus show there at the same time. The emulator draws one glyph per position at
one brightness, the brighter of the two (`ledCell`, `app/SevenSegment.hpp`):
the minus without the C. Only the leftmost position's rare states, such as
`LRN SST Del…` in PRG SOURCE 8, show the difference.

---

## Keyboard Matrix and Scanning

```cpp
key[col]  // bitmask of active K-lines; col = digit-counter slot 0–15
```

The free-running digit counter is the only scanner: it drives the display
multiplex and the keyboard row strobe at once. Keyboard rows sit at digits
1–9 (D1–D9). The 7 sense lines KN, KO, KP, KQ, KR, KS, KT (bits 0–6) are read
in parallel, so in the emulator a scan is `key[digit]`. Instructions only
sample the row being strobed, or stall until a chosen row comes up.

### Instructions that attend to the scan lines

| Mnemonic | Cycles | What it does |
|---|---|---|
| `KEY […] ALL` | HOLDs | Re-executes every cycle, riding the counter down. Stops on a masked-in K-line active at the current row (COND cleared, `KR` loaded) or at digit 0, the last row it samples (no key, COND stays 1). |
| `KEY […]` | 1 | Samples the current row against the mask; clears COND on a hit. **Never writes `KR`.** |
| `TST.BUSY` | 1 | Clears COND if KR (bit 4) is active at the current row **or** the printer is busy. |
| `WAIT Dn` | HOLDs | Stalls until the counter equals *n*; releases at once if it already does. Reads no K-lines; it makes the *next* instruction run at a known row. |
| `WAIT.BUSY` | 1 | Undocumented; a no-op in the emulator. |

The bracket list in a `KEY` mnemonic is the K-line mask. The idle scan
`KEY [N,O,P,Q,S,T] ALL` leaves out KR, which carries peripheral signals.

**`WAIT` off-by-one:** the counter decrements before each instruction, so
the instruction after `WAIT Dn` runs at digit **n−1**. `WAIT D11` +
`TST.BUSY` samples digit 10; `WAIT D1` + `KEY [P]` samples digit 0.

**Printer line in the idle scan:** the PC-100C ties KP to D0 (printer
present), and `KEY […] ALL` samples digit 0 like any other row. So on a
TI-59 or TI-58 with a printer, the idle loop's arming scan (`0657`) stops at
"row 0" on every pass, the confirmation scan (`065A`) one sweep later finds
it again, and the row guard (`065C`–`0660`) discards it: two extra digit
sweeps per pass (the confirmation's `WAIT D14` and its scan). The steady idle pass is 9.0 ms stand-alone and 18.0 ms with
a printer (TI-59). The two release gates (`062D`, `0635`) fall through a
row-0 hit the same way. Measured on a real TI-59 (videos at 120 and 30 fps):
the error blink runs 0.899 s on the PC-100C (dark 0.456 s, lit 0.444 s) and
0.449 s stand-alone; the emulator gives 0.897 and 0.447 s at 455 kHz. A
logic-analyser capture of the stand-alone blink shows the same D12 rhythm
as the emulator in both phases. A stand-alone TI-58 blinks 0.684 s (lit
about 0.28 s, dark about 0.40 s; filmed in a reflection); the emulator gives
0.681 s (lit 0.278, dark 0.403). The TI-58C moved the line to D10; its idle
loop sends rows 10 and 11 to `0A25`, which rescans below the line, and adds
a blink-counter step there to keep the blink near its stand-alone rate
(`rom/TI58C-commented.asm` at `0A25`).

**`KR` after a scan-all hit:**

```
KR = (digit << 4) | (K-line index << 8)
      bits 7:4 = row (1–9)     bits 10:8 = line (0 = KN … 6 = KT)
```

No hit leaves `KR` unchanged. Two masked-in lines active in the same row
discard the hit (multi-key rejection).

### The two ROM idioms

| Goal | Idiom |
|---|---|
| Scan the keyboard | `WAIT D14` + `KEY […] ALL`: start at the top, ride down to row 1 |
| Probe one signal | `WAIT Dn+1` + `KEY […]` or `TST.BUSY`: park until the row is strobed, sample once |

### Peripheral signals in the matrix

| col | line | Signal | How the ROM samples it |
|---|---|---|---|
| 10 (TI-59) | KR | Card switch (active = no card). Open on the TI-58, which has no reader. Confirmed on a real TI-59: a card inserted while it sits idle starts the motor and is read without a key press (idle loop → `0459` → `045D` read → `053F`), as in the emulator | `WAIT D11` + `TST.BUSY` in the idle loop (`063E`); invisible to the scan |
| 7 (TI-58/58C) | KR | Card veto: a D7–KR connection, absent on the TI-59 (Sladký, *HW guide*: "Reader detection: no D7-KR connection (TI-59 mode)"). With digit 10 open, every TI-58 idle pass goes through the second card poll (`0459`) and returns, and `2nd Write` (which enters at `045F`) finds no card to wait for. `app/` counts `0459`–`045B` as part of the TI-58's scan loop (`kCardVetoLo/Hi`), for FREEZE ON START, key sequencing and the full-speed throttle. The TI-58C ROM never probes digit 7 and samples digit 10 itself (`064D`) | `WAIT D8` + `TST.BUSY` at `0459`, reached only from the idle loop's `JC 0459` (`0640`) |
| 0 (TI-59/58) | KP | Printer connected | `WAIT D1` + `KEY [P]`. The idle scan sees it too, as a row-0 key that it discards (see "Printer line in the idle scan"); `app/` counts that confirmation scan (to `0660`) as part of the scan loop |
| 10 (TI-58C) | KP | Printer connected (the TI-58C PCB routes it to digit 10) | `WAIT D11` + `KEY [P]` |
| 12 | KP | Printer PRINT button | Ordinary scan hit at row 12 |
| 12 | KN | Printer ADV button | Ordinary scan hit at row 12 |
| 15 | KP | Printer TRACE switch (latch) | Above the scan window; polled in the keystroke/print path |

These printer lines are not in the key matrix: they are
the PC-100C's (`PrinterPinOut::keyLines` at the pads D0, D12, D15), and
`TMC0501::keyRow(digit)` ORs them into the row it scans, at digit 0 for D0
on the TI-59/58 and at digit 10 on the TI-58C (the TI machine maps the pad;
the PC-100C does not know the host). The multi-key rejection runs on the
combined row. A reset releases PRINT and ADV (the PC-100C is
told of the operation); TRACE keeps its
position.

---

## Magnetic Card Reader (TI-59)

A pass reads or writes one bank of **246 bytes**. A card has two sides, one
bank each (`core/TIMagneticCard`, the content of the media place
`core/TICardReaderPlace`); the core's card image can also hold all
four banks (984 bytes).

1. `2nd Read`/`Write` makes the ROM poll the card switch with `TST.BUSY`.
   The switch is held active (no card); when `TST.BUSY` samples it at its
   column, `isWaitingForCard()` becomes true.
2. The caller calls `insertCard(data, count)`: 984 bytes = all four banks,
   246 = one bank, 0 = a blank card to write. This releases the switch.
3. `CRD.RD` copies a bank of the card image into `m_cardBankBuffer` (a
   246-byte card is bank slot 0); `CRD.IN` reads it byte by byte. For a
   write, `CRD.WR` starts and `CRD.OUT` stores `KR[11:4]` per byte.
4. `CRD.OFF` ends the pass and ejects the card; a written bank is filed by
   its header byte 2 (3·(bank−1)). `cardEject()` returns the bank that was
   written, or nothing after a read.

Bank format and the firmware's checks: `rom/TI59-ROM-Guide.md` §
"Magnetic cards".

---

## Solid-State Library Module (TMC0540 CROM)

A 5,000 × 8-bit keycode memory with its **own address counter**, read
serially over the EXT line. A peripheral in the place
`slot-1`: `SolidStateRomModule` (`core/SolidStateRomModule.hpp`) holds the
image and the address register; `TMC0501` passes the four module words to
the module slot's pin-out (`ModuleSlotPinOut::word`) and takes the EXT it
drives. Its bytes come in with the attach, the module file `<ID>.rom.yaml`
as bundled, decoded with `core/ModuleFile`. An empty slot is no
object: every module word reads EXT = 0, as an image of zeros would
(the golden runs `ti59-no-module`, `ti58-no-module` show that the
address register an empty slot reads back as 0 makes no difference). The
14 modules are the catalogue's (`tiModules()`, `core/TICatalogue.hpp`);
`CpuFrame::m_libAddr` and `m_libAddrReadPos` read the module's register
(0 with an empty slot).

### Module commands (control class 0xA, low nibble 0xE)

| Mnemonic | Hardware command | Effect |
|---|---|---|
| `LIB.IN` | FETCH | `EXT = data[addr++] << 4`; wraps at 5000 |
| `LIB.HI` | FETCH HIGH | High nibble of the current byte; no advance |
| `LIB.PC` | LOAD PC | `addr = addr/10 + KR[7:4]·1000`; four calls load a 4-digit BCD address, LSD first |
| `LIB.PC.IN` | UNLOAD PC | One BCD digit of `addr` per call (read position 0–3); no change |

The module resets the digit position when the ROM switches between LOAD PC
and UNLOAD PC (`SolidStateRomModule`). An address of 5000 or more reads 0. Every fetch is preceded by a `LIB.HI` of the
same byte.

### Module image layout

```
0000          number of programs N (BCD)
0001          copy protection (00 = copyable, 01 = protected)
0002+2(n−1)   2-byte BCD start address of program n
…             2-byte BCD pointer one past the last keycode of program N
…             keycodes (one 2-digit BCD keycode per byte)
… 4999        filler keycode 92 (INV SBR)
```

Labels repeat across programs; the label search scans from the program
start and never leaves the program. `app/ModuleImage` parses the header
(`ModuleImage::libraryStep`).

### The user-visible module step (`libExecPC`)

The module's counter is also moved by header reads and label searches, so
the CPU keeps a separate latch, `m_libExecPC`. The `LIB.IN` handler copies
the module's pre-increment address (`ModuleSlotPinOut::address()`) into it only when the fetch comes from the
interpreter's execution site, `libExecFetchPC()`:

| Main-ROM address | Purpose |
|---|---|
| `1377` / `137C` | Header reads (program table) |
| `1390` / `1394` | Label search |
| `082B` (`LIB.HI`) / **`082F`** (`LIB.IN`) | **Execution fetch**, TI-59/58 |
| `081F` (`LIB.HI`) / **`0823`** (`LIB.IN`) | **Execution fetch**, TI-58C |
| `1370`, `1389`, `12B4` | LOAD PC (header / search start / execution start) |

So the step holds steady during label searches, which a taken label branch
repeats in full on every pass. `fA`/`fB` cannot tell execution from search
fetches (verified by trace), hence the address check. The latch starts at
`kLibExecPCNone` (0xFFFF) and is reset by `reset()`, so also by the power
cycle that follows a module change. It
is exposed as `TI59Machine::libExecPC()` and the kind `c59.calculator-state`'s
`libExecPC()` (`core/TIDebugKinds.hpp`), not as a `CpuFrame`
field. `ModuleImage::libraryStep` turns
it into program number and program-relative step.

### PRG SOURCE flag (SCOM[0] nibble 3)

Where the running program's steps come from. The firmware tests it two
ways: the run loop (`07C9`), the display formatter (`0553`), the key
epilogue (`06BD`) and the SBR launch (`0A57`) ask only "non-zero?"; the step
fetch tests bit 0 (`08B7`, module) and then bit 3 (`0B98`, keycode table).
No code tests bits 1 or 2.

| Value | Meaning |
|---|---|
| 0 | Keyboard / RAM program |
| 1 | Module program; the CROM counter is live |
| 2, inside a RTN back into a module program | RTN writes the popped level into SCOM[0] (`1192`); `LIB_RETURN` reloads the CROM PC with `LIB.PC` ×4 and sets the flag back to 1 at `1506`, 16 instructions later. A debugger STEP stops only at the step dispatch, so it never lands in this window. |
| 2, 4, 6 elsewhere | **Fast mode**: a RAM program on the sub-program path. Only reachable through firmware bugs; see `rom/fastmode.md` and `rom/TI59-ROM-Guide.md` § "Fast mode". |
| 3, 5, 7, 9 | Undocumented; act like 1 (bit 0 is tested first) |
| 8 (and A, C, E) | Keycode sub-program in the constant ROM (Σ+, x̄/σ, Op 11–15, P→R/R→P, D.MS; `rom/TI59-keycode-programs.asm`); the SCOM[0] step counter (000–383) is used, `m_libExecPC` holds |

The debugger decodes the flag the same way (`cu59::programSourceFrom`,
`app/ModuleImage.cpp`).

Source 2 exists because a module caller cannot be pushed as source 1:
source 1 means "the CROM counter is live", and header and search traffic
clobber the counter, so the saved address must be restored from the return
level before fetching resumes.

---

## Printer (PC-100C)

The PC-100C is a peripheral in the place `printer`:
`PC100C` (`core/PC100C.hpp`) holds the buffer, the ready gate, the busy
count, the pending code line and the PRINT, ADV and TRACE controls.
`TMC0501` passes the six printer words to the printer connector
(`PrinterPinOut::word` with KR as data) and reads Busy for `TST.BUSY`;
without a PC-100C the words reach nothing. The ROM then executes one
`PRT.STEP` and no `PRT.GO`, `PRT.OUT` or `PRT.FEED`, so a program is not
held up by a printer that is not there (golden run
`ti59-print-without-printer`).

A 20-character buffer filled right to left; `PRT.GO` reverses it into a
left-to-right line.

| Opcode | Effect |
|---|---|
| `PRT.CLR` | Clear the buffer; arm the ready gate |
| `PRT.OUT` | Store one character (6-bit code from `KR[9:4]`) |
| `PRT.FUNC` | Store a 3-character function mnemonic (`PRN_STR` table) |
| `PRT.STEP` | Store a blank |
| `PRT.GO` | Print the line; Busy for `PC100C::kLineInstructions` (4165) |
| `PRT.FEED` | Half a line of paper; Busy for `PC100C::kFeedInstructions` (1370) |

**Timing.** The PC-100 runs on the calculator's PHI clock (PHI1 is on the
printer connector), so its BUSY times are instruction cycles, the same on
every model, counted down once per `step()` (one Φ1 on the connector per
instruction). A TI-59 in IDLE still gets 16
PHI1 pulses per instruction, so the per-step count holds there too. On the
TI-59 (14,219/s) a printed line takes 293 ms and a feed 96 ms; on the TI-58C
(12,312.5/s) 338 and 111 ms. Adv (calculator key, printer button or program
step) issues two `PRT.FEED`s for one line on paper (`0F50`–`0F56` on the
TI-58C, the same loop at `0F38` on the TI-59). The ROM does not wait after a
print: it carries on (the next program steps, the next List line) and polls
`TST.BUSY` right before its next print operation, so ROM work overlaps the
printer's cycle.

Every printer timing here was measured on a PC-100A, not on a PC-100C; no
difference between the two is expected, and
the PC-100C keeps these numbers.

Measured from audio recordings of a PC-100A (2026-10-24), one unit each of
TI-59 (453.5 kHz) and TI-58C (≈394.3 kHz). The loop period minus the ROM's
own work after its last `TST.BUSY` (from the emulator) gives the printer's
cycle, in instruction cycles:

| Loop | TI-59 period | TI-58C period | Printer cycle TI-59 / TI-58C |
|---|---|---|---|
| List (empty program) | 307.0 ms | 356.9 ms | line 4165 / 4212 |
| `1 SUM 00 Prt GTO 000` | 297.9 ms | 342.9 ms | line 4168 / 4171 |
| `1 SUM 00 Adv GTO 000` | 332.9 ms | 398.8 ms | feed 1366 / 1377 |

The Adv loop's other half (second feed to first feed) is ROM-bound: 3348
cycles on the TI-59, 3534 on the TI-58C. In milliseconds the two models
differ by 15%; in cycles they agree within the TI-58C's ≈1% session-to-session
clock wander (see "TI-58C clock"). Two more recordings support the clock
dependence: the dot-row strokes inside a printed line (≈7 per line, 35.9 ms
on the TI-59, 41.3 ms on the TI-58C, ratio 1.150 = the clock ratio) and the
paper motor's hum while ADV is held (146.77 Hz vs 128.20 Hz, ratio 1.145).
The strokes take 7 × 508 ≈ 3556 of a line's 4165 cycles; the rest fits the
paper advance to the next line. Of the measured 4165–4168 for a printed
line, 4165 is used: the TI-59 printer-interrupt lines below fit it and not
4168 (see "Dot timing within a line").

The paper (`core/PC100CPaper.hpp`): `PRT.GO` puts the line's
20 characters on the text layer at once, at the y where the paper stands;
the line's dots go down when it ends (Busy expires: `rowCount` = 7), each
character's 5×7 dots at `(i × 6.53 + c + 0.5, top + r + 0.5)` in dot pitches,
and the paper is fed 10.18 units (a line cut short: only its drawn rows). A
`PRT.FEED` gives an empty text line and 7.09 units. The capability is
`printer/paper-output`; the paper itself is the session's (`PaperPart`).
`PC100C::CodeLine` (the 20 codes, `rowCount`, `partialColumns`,
`partialHeat`) is kept only for the golden runs
(`TIMachineTesting::printedCodeLines`). A `PRT.GO` while a line is printing is the
**printer interrupt** (during a feed's BUSY it prints normally): the line
in progress is committed with what is on paper so far, and the new print
is ignored. Firmware side and the
1F keycode that triggers it: `rom/printerinterrupt.md`. A `PRT.FEED` during
a print commits the pending line first, cut short at that moment like an
interrupt, so lines keep their order; a reset, power cycle or cold boot
does the same (`PC100C::operation()`), and so does a detach
(`endLineHere()`): the rows printed go onto the paper before
the printer goes.

**Dot timing within a line.** A stroke fires the 5 dot columns of all 20
characters one column at a time, right to left. Column c (5 = rightmost) of
row r (from 0) starts `r·508 + 18 + (5 − c)·64` cycles after `PRT.GO`, so a
row's columns take 320 of its 508 cycles. A cut leaves `rowCount` complete
rows and `partialColumns` (0–5) columns of the next one, the last of them
heated `partialHeat` cycles. `core/PC100CPaper.hpp` (`inkedColumns`,
`faintColumns`, `drawnRows`, `lineDots`) turns these into the marks on the
paper; the faint column's darkness is its heating time (`inkAlpha` gives
the grey of each darkness in the description; full from 20 cycles).

Measured on the user's PC-100A (2026-10-25), on a TI-58C and on a TI-59.
That unit's head has dead and flaky elements (a test line of 20 `Π`, code
76): dead in columns 4–5 of characters 7, 9, 12, 13 and column 5 of 16 and
18, flaky in several columns of characters 2, 9, 10, 11 and 20. Only dots on
working elements count below.

*TI-58C, `examples/debug/printer-interrupt-sweep.ti58c`*, cuts 16 cycles apart.
The first cut at which each column shows, with the last cut before it where
it doesn't:

| Row (from 1) | Column 5 | 4 | 3 | 2 | 1 |
|---|---|---|---|---|---|
| 5 | 2049 → 2065 | 2113 → 2129 | 2161 → 2193 | 2241 → 2257 | 2305 → 2337 |
| 6 | | 2625 → 2641 | 2689 → 2705 | | |

Row 5 allows an offset from 18 to 33 cycles, row 6 from 22 to 37. Rows 1–4
were complete at 1905 cycles and later.

*TI-59, `examples/debug/printer-interrupt-bits.ti59`.* Ten lines differ only in
the 1-bits of three character codes. Op 05 takes longer to send codes with
more bits, which moves its `PRT.GO` against the CPU's digit counter, and the
1F path ends in a `WAIT D0` (05A9) that waits 0–15 cycles for digit 0. So
the cut time depends on the digit phase left by the busy wait before the
print, and with it on the line busy time. With 4165 the lines are cut 1034,
1035, 1034, 1046, 1045, 1044, 1042, 1040, 1036 and 1035 cycles after
`PRT.GO`. All ten show column 5 of row 3, between barely and about 30% dark;
in two runs line 4 was the darkest, lines 5–8 next, the rest faintest, in
the order of the cut times. No other line busy time from 4160 to 4191 gives
that order, and 4168 makes lines 2, 3, 9 and 10 (cut 1047–1049) the latest,
though they are among the faintest. All lines marking means column 5 starts
by 1034: an offset of 18 or less. `examples/historic/printer-quirks.ti59` agrees: cut at 1036
and 1038, it shows barely visible specks on most rows with a candidate dot.

18 fits both calculators if a dot heated for only 0–4 cycles may or may not
show: the TI-58C's row 6 had no new column at cuts 3 cycles after its start
(k = 62, 67), and the TI-59's faintest lines are cut 0–2 cycles after it.
Darkness grows with the heating time before the cut: barely visible at 0–4
cycles, about 30% at 12, weak or dark at 15–19 (the TI-58C's first cuts of
each column), a normal dot after that. Repeats with the same cut time came
out once weaker, once darker; near the threshold the paper and the head
decide.

---

## Threading Model

The core holds no lock, no atomic and starts no thread:
a machine is used from one thread at a time, and the caller serialises.
Every frontend runs the machine on the one thread of its app session, so
no caller needs to serialise.

---

## Trace / Debug API

`TRACE_*` flags (`TraceTypes.hpp`) select what each `CpuFrame` records:

| Flag | Cost | Content |
|---|---|---|
| `TRACE_NONE` | One load per step | — |
| `TRACE_PC` | Low | pc, opcode, digit, cycle weight, seqno |
| `TRACE_REGS_LIGHT` | Medium | + KR, SR, fA, fB, R5, cpuFlags |
| `TRACE_REGS_FULL` | High | + A–E, SCOM, Sout, EXT, PREG, library address |
| `TRACE_BREAKPOINTS` | Binary search per step | Stop at a ROM address |

Frames go into a 1024-entry ring (~392 bytes each) with **post-execution
semantics**: `beginNextStep()` patches the previous frame with the state
after its instruction. Seqno gaps mean ring overflow; the drain reports
the lost count. The flags are set by `TIMachine` alone: TRACE_PC
| TRACE_REGS_FULL while the debug panel, a trace or the run control's checks
want the ring, else none. A trace drains the ring through the capability
`TMC0501/c59.trace-records` (`core/TraceRecords.hpp`: `begin`, `take`,
`end`), which the app's `TITraceRecorder` turns into records; the panel
reads it without draining (`c59.calculator-state`'s `readCpuFrames`). Field list and the trace file:
`docs/developer/Debug-API.md` § "CPU Inspector" and `docs/formats/trace.md`. `emitDebug()` events
(`setDebugLevel`, `drainDebugEvents`): `docs/developer/Debug-API.md` § "Core
debug events".

---

## Variant Differences

| Feature | TI-59 | TI-58 | TI-58C |
|---|---|---|---|
| ROM | TMC0582/0583/0571B | same as TI-59 | CD2400/2401/TMC0573 |
| Tick rate (ticks/s; instructions/s = ÷ 32) | 455,000 (14,219) | 384,000 (12,000) | 394,000 (12,312.5) |
| IDLE weight | 4 | 4 | 1 |
| RAM registers | 120 | 60 | 60 + E000–E003 |
| RAM access | `RAM.OP` | `RAM.OP` | `MEMWR`/`MEMRD` |
| Card reader | Yes (switch at digit 10) | No (D7–KR veto, digit 10 open) | No (digit 7 held; the ROM never reads it) |
| Printer-present digit | 0 | 0 | 10 |
| Printer BUSY (line / feed) | 4165 / 1370 cycles | same | same |
| Library module | Yes | Yes | Yes |
| Constant memory | No | No | Yes (`ti58c.memory.yaml` through the kind `c59.constant-memory`; `TIConstantMemory` at build) |
| Reset | RAM and partition kept (restored at the scan loop) | same | a power cycle; constant memory kept |
| Step dispatch / module fetch | `0837` / `082F` | same | `082B` / `0823` |
