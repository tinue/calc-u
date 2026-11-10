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
| 3 | [Frontend interface, session hosting and messages](frontend.md) | ADR 0009, 0015, 0023-0025 | Draft, forks decided |
| 4 | [Typing strategy](typing.md) | C11, ODD59-4, ADR 0009 | Draft, forks decided |
| 5 | [Preset engine and family preset format](presets.md) | ADR 0016-0018, 0021, 0037 | Draft, forks decided |
| 6 | [Media and save folder](media.md) | ADR 0019, 0020, 0037 | Draft, forks decided |
| 7 | [Settings and paths](settings.md) | ADR 0021, 0022, 0037 | Draft, forks decided |
| 8 | [Debugger (debug target, DAP, debug panel)](debugger.md) | ODD59-27 | Draft, forks decided |
| 9a | [Harness protocol](harness.md) | ADR 0026, 0033 | Draft, forks decided |
| 9b | [ROM lookup](roms.md) | ADR 0034, 0035 | Draft, forks decided |
| 9c | [Help structure](help.md) | ADR 0036 | Draft, forks decided |

## Open questions left after the forks (2026-11-10)

Every contract lists its open questions with their status. Still open:

| Where | Question | Settled in |
|---|---|---|
| `machine.md` Q-1 | PC-1350 / PC-1403 ROM versions as variants | an SC61860 product, or a phase 5 walkthrough |
| `peripherals.md` Q-2 | PC-1403 / PC-1350 behaviour on a peripheral or RAM card change | an SC61860 product, or a phase 5 walkthrough |
| `peripherals.md` Q-3 | TMC0540 slot signals = the chip's pins (assumed) | when the slot is modelled at contact level (ADR 0011) |
| `typing.md` Q-1 | PC-1403 / PC-1350 hold, readiness, sequences | an SC61860 product |
| `typing.md` Q-2 | 100 ms emulated hold enough everywhere | phase 6, Calc-U-59 tests |
| `typing.md` Q-3, `debugger.md` Q-1 | PC-1500 key-scan loop: research and implement key readiness | phase 6 |
| `presets.md` Q-1 | upgrade of the shipped and hosted presets | phase 6 |
| `presets.md` Q-3 | key ID lists per model | phase 6, with the descriptors |
| `harness.md` Q-2 | a Python helper for `calcu serve` | phase 6, with the skills |
