# Contracts

Phase 4 (`docs/Process.md`): one contract per seam, each with
responsibilities, non-responsibilities, invariants and a fit table
(TI-59, PC-1500, PC-1600, and the SC61860 paper machines PC-1403 and
PC-1350). Decisions taken while writing a contract are recorded in the
contract's decision log; a decision that changes an accepted ADR goes
back to the user first.

| # | Contract | Inherits from | Status |
|---|---|---|---|
| 1 | [Machine interface and descriptor](machine.md) | ADR 0006-0009, 0012, 0035 | Draft, forks decided |
| 2 | [Peripheral and connector model](peripherals.md) | ADR 0010, 0011, 0021 | Draft, forks decided |
| 3 | Frontend interface, session hosting and messages | ADR 0009, 0015, 0023-0025 | Planned |
| 4 | Typing strategy | C11, ODD59-4, ADR 0009 | Planned |
| 5 | Preset engine and family preset format | ADR 0016-0018, 0021 | Planned |
| 6 | Media and save folder | ADR 0019, 0020 | Planned |
| 7 | Settings and paths | ADR 0021, 0022 | Planned |
| 8 | Debugger (debug target, DAP, debug panel) | ODD59-27 | Planned |
| 9 | Harness protocol, ROM lookup, help structure | ADR 0026, 0033, 0035, 0036 | Planned |
