# Process: from briefing to code

How the Calc-U project gets from `Briefing.md` to the first line of merged
code. The project is not greenfield: most behaviour already exists twice
(Calc-U-59, Calc-U-1600). The hard parts are therefore facts, vocabulary,
seams and migration, not "what should the app do". The process borrows its
discipline from the Kaldris feature-spec process
(`/Users/me/Development/mac/Kaldris/BriefingPrompt.md`): ask instead of
inventing, explicit out-of-scope, stable IDs, Given/When/Then, spec / design /
QA in separate files, no code before the design is approved.

## Phases

Every phase ends with a **review gate**: the user reads, comments and
approves before the next phase starts. Status of the phases lives only in
the table below.

| # | Phase | Output | Settles | Status |
|---|---|---|---|---|
| 0 | Charter | `Briefing.md`, `CLAUDE.md` | Goals, non-goals, products, constraints, decisions already made, open points | In progress (grilling) |
| 1 | Inventory | `docs/inventory/` | Facts only, no decisions | Planned |
| 2 | Domain model | `docs/GLOSSARY.md` | One meaning per term | Planned |
| 3 | Architecture decisions | `docs/adr/NNNN-*.md` | The forks | Planned |
| 4 | Contracts | `docs/contracts/*.md` | The abstractions, as interface specs | Planned |
| 4b | Capability specs | `features/INDEX.md`, `features/KIT-n-*/spec.md` | What each shared capability does, per product | Planned |
| 5 | Validation on paper | `docs/walkthroughs/*.md`, optional throwaway spikes | Whether the abstractions hold | Planned |
| 6 | Migration plan, then code | `docs/migration-plan.md`, `features/KIT-n-*/design.md`, `qa-report.md` | Order of steps, each leaving every product green | Planned |

### Phase 0 — Charter
- `Briefing.md` is grilled until no open question is left that blocks
  phase 1. Answers go back into the briefing; questions that belong to a
  later phase are moved to briefing §7 with the phase named.

### Phase 1 — Inventory
- Baselines: Calc-U-59 current dev branch, Calc-U-1600 `dev-0.8.0`.
- `docs/inventory/calc-u-59.md`, `docs/inventory/calc-u-1600.md`: per
  capability (keyboard, presets, debugger, settings, media/files, audio,
  pacing, threading, display, web shell, release tooling, tests/headless,
  help-site material, …) how the product does it, where it lives, how mature
  it is.
- `docs/inventory/divergence-matrix.md`: capability × product, with the
  contradictions and a *suggested* base side (the decision comes in phase 3).
- `docs/inventory/backlog.md`: every item of both `TODO.md` files and of
  `Calc-U-59/ideas/`, each with a target: design input, shared backlog,
  product backlog, done, or dropped (with reason). Briefing §1.7, §8.
- Harvest both projects' "deliberate oddities" (Calc-U-1600
  `docs/background/Decisions.md`, Calc-U-59 `reference/`) so they survive the
  merge.
- Compatibility surface for Calc-U-59 (briefing §1.2): every file format it
  reads today.
- Method: one background agent per repository; the matrix is written from
  their reports, not from file dumps.

### Phase 2 — Domain model
- `docs/GLOSSARY.md`: Product, Kit/shared layer, Machine, Model, Variant,
  Family, Component, Connector/Port, Device, Card, Media, Preset, Session,
  Frontend, Platform, and the user-facing terms (briefing §1.5).
- Every later document uses only these terms. Conflicting names in the two
  codebases (for example `EmulatorSession` vs `MachineSession`) are mapped.

### Phase 3 — Architecture decisions
- One ADR per fork: context, options, decision, consequences. Candidates:
  - Threading model (Calc-U-59 threaded runner vs Calc-U-1600 coroutine
    jobs), with the Swift/iOS frontend in mind.
  - The core seam: one machine interface for all products, or a small
    kit-level machine interface plus product-level extensions
    (briefing §4).
  - Devices keyed by connector type or by machine.
  - Swift bridge boundary.
  - Preset format, versioning and the Calc-U-59 read-old/write-new upgrade.
  - Repository layout, CMake structure, CI.
  - Versioning, tags, branch model (replacing `dev-x.y.z`), release location.
  - History archive, product naming, ROM removability (design only).
  - Skill structure (per product plus shared).
- Each existing "deliberate oddity" either becomes an ADR or stays in a
  product-level decisions file.

### Phase 4 — Contracts
- One document per seam: machine interface and descriptor, frontend platform
  interface, device/port model, typing strategy (keyboard), preset engine and
  format, debugger (debug target and DAP), settings and paths, media library.
- Each contract states responsibilities, explicit non-responsibilities and
  invariants, and contains a **fit table**: how TI-59, PC-1500, PC-1600 and
  an SC61860 machine (PC-1403 for a character LCD, PC-1350 for a graphic LCD)
  map onto it.
- Design against three very different machine families, implement two. The
  SC61860 machines are paper machines; the TI-59 (TMC0501, PC-100C cradle,
  card reader) is the main stress test for anything Sharp-shaped.

### Phase 4b — Capability specs
- Kaldris template (`spec.md`: dependencies, user stories, out of scope,
  acceptance criteria, edge cases, open questions, decision log). IDs
  `KIT-1`, `KIT-2`, …; status only in `features/INDEX.md`.
- Acceptance criteria are written per product and frontend, which makes them
  the parity checklist. They describe today's behaviour of each product
  (briefing §1.3: no new product features).

### Phase 5 — Validation on paper
- Scenarios played against the contracts, for example: add a PC-1403; add a
  CE-126P; TI-59 App Store build without DAP or PTY; add a Swift frontend to
  Calc-U-1600; the PC-1600 using the PC-1500 tape codec; ship without ROMs.
- A scenario that needs a change to the shared layer means the abstraction
  is wrong; fix the contract, not the scenario.
- Reviewed by a fresh agent that sees only the contracts, not the reasoning
  behind them.
- Spikes only where paper is not enough (for example threading on iOS);
  thrown away afterwards.

### Phase 6 — Migration plan, then code
- Steps, each leaving every product building, testing and releasable:
  repository merge with history archived, CI for all products, golden tests
  as the safety net, then one capability extraction at a time.
- Each extraction is a `KIT-n` feature with `design.md` and `qa-report.md`
  (chain: AC → task → test).
- Refactoring inside either product (for example a machine interface in
  Calc-U-1600) happens here, after the contracts, not before.

## Per-phase prompts
Each phase gets a prompt file `prompts/phase-N.md` in the style of the
Kaldris `BriefingPrompt.md`, written at the start of the phase, so a phase can
be run or resumed in a fresh session.
