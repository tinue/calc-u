# Trace file [1600]

The binary trace file of Calc-U-1600: one record per instruction a CPU
carried out while a trace ran, read by `products/calc-u-1600/tools/read_trace.py`
and by any other tool that wants the CPUs' log. A trace started with the
TRACE button is called `CALCU1500_TRACE.bin`, `CALCU1500A_TRACE.bin` or
`CALCU1600_TRACE.bin` after the model; a preset's `trace:` step and the
harness's `trace` name their own file. Each trace starts a fresh file. All
multi-byte fields are little-endian.

The format is Calc-U-1600's own; Calc-U-59's trace file ([trace.md](trace.md))
has another magic, version, record set and frame. A PC-1600's file holds
the frames of both its CPUs.

Terms follow `docs/GLOSSARY.md`.

## 1. Version

The file header carries version 3, the only version of this format. A
reader refuses any other.

## 2. Layout

```
[file header, 16 bytes]
[record: type u8, payload_length u16, payload]
...
```

### 2.1 File header

| Offset | Size | Field |
|---|---|---|
| 0 | 4 | magic `0x50433135` ('PC15') |
| 4 | 2 | version 3 |
| 6 | 2 | reserved, 0 |
| 8 | 8 | reserved, 0 |

The header does not name the model; the CPU of each frame is in the frame
(§2.3, §2.4).

### 2.2 Records

Unknown types are skipped.

| Type | Name | Payload |
|---|---|---|
| 0x01 | SESSION_START | u64 Unix time (s) |
| 0x02 | TRACE_EVENT | One LH5801-shaped frame: 25 bytes (§2.3) |
| 0x03 | SESSION_END | u32 count of the TRACE_EVENT and TRACE_EVENT_Z80 records written |
| 0x04 | TRACE_EVENT_Z80 | One Z-80-shaped frame: 29 bytes (§2.4) |
| 0x05 | TRACE_GAP | u32 frames lost to ring overflow |

A file starts with the header and SESSION_START and ends with
SESSION_END. Two exceptions:

- **Size limit.** A trace that reaches the size limit ends with
  SESSION_END after the last whole record that fits; its count includes
  the frames that did not fit.
- **New machine.** A trace that ends because a new machine was built
  (a model switch, for example) has no SESSION_END.

**Which CPU.** A PC-1500 and a PC-1500A write TRACE_EVENT records of
their LH5801. A PC-1600 writes TRACE_EVENT_Z80 records of its SC7852 and
TRACE_EVENT records of its LH5803. Each frame carries a `cpuId`:

| cpuId | CPU |
|---|---|
| 0 | the LH5801 of a PC-1500 or PC-1500A |
| 1 | the SC7852 (Z-80) of a PC-1600 |
| 2 | the LH5803 of a PC-1600 |

**Order.** The writer drains each CPU's ring about every 256
instructions. In a PC-1600's file each drain writes the SC7852's frames,
then the LH5803's: within one CPU the frames are in the order the CPU
ran them; between the two CPUs the file is not in time order. A reader
that wants one CPU's run filters by `cpuId`.

**Gaps.** A TRACE_GAP record says how many frames of one CPU the writer
lost; it comes before the frames of that CPU's drain, and that CPU's next
`seqno` is higher by the same amount.

**What makes a frame.** One frame per instruction carried out. An
interrupt's entry is not an instruction and has no frame; the handler's
first instruction has the next one. A halted CPU writes none, and so does a PC-1600 CPU while the other one runs (one of the two runs at a time).

**The registers** of a frame are the ones its instruction started from
(pre-execution, as the debugger shows them); the instruction's effect
shows in that CPU's next frame. Every register field is filled.

### 2.3 TRACE_EVENT payload (25 bytes): LH5801, LH5803

| Offset | Size | Field |
|---|---|---|
| 0 | u32 | seqno: the CPU's own count of traced instructions, from 0 at the first one since the machine was built (a later trace on the same machine counts on) |
| 4 | u16 | pc: P at the start of the instruction |
| 6 | u16 | opcode: the opcode byte; for an `FD`-prefixed instruction `0xFD00` + the second byte |
| 8 | u8 | cycles: the CPU cycles of the instruction (5 for an illegal opcode) |
| 9 | u8 | A |
| 10 | u8 | XL |
| 11 | u8 | XH |
| 12 | u8 | YL |
| 13 | u8 | YH |
| 14 | u8 | UL |
| 15 | u8 | UH |
| 16 | u16 | S |
| 18 | u8 | T: bit 0 C, bit 1 IE, bit 2 Z, bit 3 V, bit 4 H |
| 19 | u8 | PU |
| 20 | u8 | PV |
| 21 | u8 | DISP |
| 22 | u16 | TM |
| 24 | u8 | cpuId: 0 or 2 |

### 2.4 TRACE_EVENT_Z80 payload (29 bytes): SC7852

| Offset | Size | Field |
|---|---|---|
| 0 | u32 | seqno: the CPU's own count of traced instructions, from 0 at the first one since the machine was built (a later trace on the same machine counts on) |
| 4 | u16 | pc: PC at the start of the instruction (at its first prefix) |
| 6 | u16 | opcode: see below |
| 8 | u8 | cycles: the T-states of the instruction, with the SC7852's M1 wait states |
| 9 | u8 | cpuId: 1 |
| 10 | u16 | AF |
| 12 | u16 | BC |
| 14 | u16 | DE |
| 16 | u16 | HL |
| 18 | u16 | IX |
| 20 | u16 | IY |
| 22 | u16 | SP |
| 24 | u8 | I |
| 25 | u8 | R |
| 26 | u8 | IFF1: 0 or 1 |
| 27 | u8 | IFF2: 0 or 1 |
| 28 | u8 | IM |

The opcode word:

| Instruction | opcode |
|---|---|
| unprefixed | the opcode byte |
| `CB xx`, `ED xx` | `0xCB00` + xx, `0xED00` + xx |
| `DD xx`, `FD xx` | `0xDD00` + xx, `0xFD00` + xx; for `DD CB d op` and `FD CB d op` it is `0xDDCB`, `0xFDCB` |
| `DD` or `FD` followed by `DD` or `FD` | a frame of its own for the first prefix, opcode `0x00DD` or `0x00FD` (a 4 T-state NOP plus the M1 wait); the instruction that follows starts at the second prefix |
| `DD ED xx`, `FD ED xx` | `0xED00` + xx, its pc at the ignored prefix, its cycles including that prefix |
