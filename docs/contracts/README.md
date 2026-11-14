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
| 10 | [Capabilities and product kinds](capabilities.md) | ADR 0006, 0007 | Draft, forks decided |

Contract 3 also holds "Sessions, pacing and operations" (SP-1 to SP-3)
and the web embedding interface (FE-7). The contracts were
reviewed on 2026-11-10; the forks added then are marked "added in the
phase 4 review" in each contract. User-visible changes that the
decisions cause are collected in
[`../behaviour-changes.md`](../behaviour-changes.md).

## Rules that cross contracts

- **Generic first, product kinds beside it.** Every seam has a generic
  part the library owns and a defined place for what one product alone
  has. A product-only function never bends a generic interface and never
  goes around the seam. It becomes generic when a second product needs
  it (`capabilities.md`, C-1).

  | Seam | Generic part | Product part |
  |---|---|---|
  | Machine | machine interface, generic capability kinds | product kinds (`machine.md`, M-7) |
  | Frontend messages | generic topics and commands | product topics and commands (`frontend.md`, FE-6) |
  | Control state | computed by the library from the descriptors | product entries (`frontend.md`, FE-6) |
  | Preset format | header, steps, media | product sections (ADR 0018, `presets.md`) |
  | Harness | common commands | product commands (`harness.md`, H-2) |
  | Files | presets, media | product file kinds (`frontend.md`, SP-3) |
  | Debug panel | run control and its buttons | the panel itself (`debugger.md`, DB-6) |

- **Formats.** For the migration release only the current version of an
  own format is read; afterwards a release reads the previous version
  and silently upgrades the file in place (ADR 0037, "Scope";
  `docs/formats/README.md` §2).

## What is library and what is app layer

ADR 0002 left the library's structure to phase 4. The table collects
what the contracts say, plus one decision of its own (L-1).

**L-1 (decided 2026-11-10, user):** the app session and the machine
session are library classes. A product does not write a session of its
own; its app layer supplies the parts listed in the right-hand column
and whatever only that product has. The alternative, a session class per
product assembled from library parts, was rejected: the glue would be
written twice and could drift again.

| Library part | Contract | The product supplies |
|---|---|---|
| Machine interface, descriptors, catalogue, emulated-time type | 1 | its core: machines, model and peripheral descriptors |
| Peripheral and connector model, places | 2 | pin-out signal interfaces and peripherals (in the core) |
| Generic capability kinds | 10 | implementations; product kinds |
| Machine session: pacer, operations (jobs) | 3 | a pacing profile per model (SP-1) |
| App session: messages, control state, busy, audio policy | 3 | product topics, commands and control entries |
| Typing engine | 4 | a typing profile and keyboard map per model |
| Preset loader and step engine | 5 | product sections and their parsers |
| Media model, pickers' data, save rules | 6 | readers and writers of its media formats (`capabilities.md`, Q-2) |
| Settings file, paths | 7 | its keys and defaults |
| Debug target interface, run control | 8 | targets (in the core); the debug panel's content; DAP (Calc-U-1600) |
| Harness program and serve protocol | 9a | product commands |
| ROM lookup | 9b | the ROM lists in its descriptors |
| Qt6 and React component sets (ADR 0023) | 3 | faceplate, hit map, layout, the debug panel |
| C++ test runner (ADR 0038) | — | its tests |

Dependencies point one way: a product's frontends use its app layer and
the library; its app layer uses the library and its core through the
machine; a core uses the library's interfaces; the library uses no
product code.

## Open questions left after the forks (2026-11-10)

Every contract lists its open questions with their status. Still open (answered rows removed 2026-11-13 and 2026-11-14):

| Where | Question | Settled in |
|---|---|---|
| `machine.md` Q-1 | PC-1350 / PC-1403 ROM versions as variants | an SC61860 product (the phase 5 scenarios were dropped; `docs/walkthroughs/first-steps.md`, "Not first steps") |
| `peripherals.md` Q-2 | PC-1403 / PC-1350 behaviour on a peripheral or RAM card change | an SC61860 product (as above) |
| `peripherals.md` Q-3 | TMC0540 slot signals = the chip's pins (assumed) | when the slot is modelled at contact level (ADR 0011) |
| `typing.md` Q-1 | PC-1403 / PC-1350 hold, readiness, sequences | an SC61860 product |
| `presets.md` Q-1 | upgrade of the shipped and hosted presets | phase 6 |
| `harness.md` Q-2 | a Python helper for `calcu serve` | phase 6, with the skills |
