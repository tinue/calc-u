# Trace file [59]

The binary trace file of Calc-U-59: one record per instruction the CPU
carried out while a trace ran, read by `products/calc-u-59/tools/read_trace.py`
and by any other tool that wants the CPU's log. A trace started with the
TRACE button is called `CALCU59_TRACE.bin`, `CALCU58_TRACE.bin` or
`CALCU58C_TRACE.bin` after the model; a preset's `trace:` step and the
harness's `trace` name their own file. Each trace starts a fresh file. All
multi-byte fields are little-endian.

Terms follow `docs/GLOSSARY.md`.

## 1. Versions

v1 (written by Calc-U-59 1.0.0) is stable. v2 is the current version and
may change between v2 revisions; readers accept both.

## 2. Layout

```
[file header, 16 bytes]
[record: type u8, payload_length u16, payload]
...
```

### 2.1 File header

| Offset | Size | v1 | v2 |
|---|---|---|---|
| 0 | 4 | magic `0x54493539` ('TI59') | same |
| 4 | 2 | version 1 | version 2 |
| 6 | 2 | reserved | model: 0 TI-59, 1 TI-58, 2 TI-58C |
| 8 | 8 | reserved | reserved |

### 2.2 Records

Unknown types are skipped.

| Type | Name | Payload |
|---|---|---|
| 0x01 | SESSION_START | u64 Unix time (s); v2 adds u8 model |
| 0x02 | TRACE_EVENT | One `CpuFrame`: 125 bytes (v2), 124 (v1) |
| 0x03 | SESSION_END | u32 TRACE_EVENT count; u32 suppressed total, always 0 |
| 0x04 | USER_EVENT | u8 kind, u8 p1, u8 p2, u8 reserved (no longer written) |
| 0x05 | TRACE_GAP | u32 frames lost to ring overflow |

USER_EVENT kinds, in older files: 0x01 key down, 0x02 key up
(p1 = row 1–9, p2 = col 1–5), 0x03 card insert, 0x04 card eject. A trace
is the CPU's log alone: no USER_EVENT is written, whoever starts the
trace. The version stays 2.

### 2.3 TRACE_EVENT payload (v2, 125 bytes)

Post-execution state of one instruction.

| Offset | Size | Field |
|---|---|---|
| 0 | u32 | suppressed (always 0) |
| 4 | u32 | seqno |
| 8 | u16 | pc |
| 10 | u16 | opcode (13 bits) |
| 12 | u16 | fA |
| 14 | u16 | fB |
| 16 | u16 | KR |
| 18 | u16 | SR |
| 20 | u16 | EXT |
| 22 | u16 | PREG |
| 24 | u16 | cpu_flags (bit 0 IDLE, bit 11 COND) |
| 26 | u16 | m_libAddr |
| 28 | u8 | R5 |
| 29 | u8 | digit |
| 30 | u8 | RAM_ADDR |
| 31 | u8 | RAM_OP |
| 32 | u8 | REG_ADDR |
| 33 | u8 | m_libAddrReadPos |
| 34 | u8 | cycle_weight (1 active, 4 idle) |
| 35 | u8 | displayOn |
| 36 | u8 | maxDigitDecay |
| 37 | 5 × 16 | A, B, C, D, E: one nibble per byte, index 0 = least significant |
| 117 | 8 | Sout, packed: low nibble = Sout[2i], high = Sout[2i+1] |

v1 (124 bytes) has one byte at offset 35, `dispFilter` (0–3, ≥ 3 = display
blanked), and the registers from offset 36; a reader derives
`displayOn` from it (blanked: 0).

**Gaps.** A TRACE_GAP record says how many frames the writer lost; the
next TRACE_EVENT's `seqno` is higher by the same amount.
