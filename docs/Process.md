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
| 0 | Charter | `Briefing.md`, `CLAUDE.md` | Goals, non-goals, products, constraints, decisions already made, open points | Done |
| 1 | Inventory | `docs/inventory/` | Facts only, no decisions | Done (approved 2026-11-09) |
| 2 | Domain model | `docs/GLOSSARY.md` | One meaning per term | Done (glossary approved 2026-11-09) |
| 3 | Architecture decisions | `docs/adr/NNNN-*.md` | The forks | Done (approved 2026-11-09) |
| 4 | Contracts | `docs/contracts/*.md` | The abstractions, as interface specs | Planned |
| 4b | Capability specs | `features/INDEX.md`, `features/<ID>-*/spec.md` | What each shared capability does, per product | Planned |
| 5 | Validation on paper | `docs/walkthroughs/*.md`, optional throwaway spikes | Whether the abstractions hold | Planned |
| 6 | Migration plan, then code | `docs/migration-plan.md`, `features/<ID>-*/design.md`, `qa-report.md` | Order of steps, each leaving every product green | Planned |

### Phase 0 — Charter
- `Briefing.md` is grilled until no open question is left that blocks
  phase 1. Answers go back into the briefing; questions that belong to a
  later phase are moved to briefing §6 with the phase named.

### Phase 1 — Inventory
- Baselines are pinned to the release tags, recorded in each inventory's
  header. A release tag is a squashed commit on `main`; its tree is
  identical to the tip of the dev branch, which holds the history.
  - Calc-U-59: `v2.3.0` @ `231792c` (= `dev-2.3.0` @ `915a2b5`).
  - Calc-U-1600: `v0.8.1`, assumed released. Until it is tagged, the pin
    is `dev-0.8.1` @ `b4ea8c8` (tree `947b850`), the release candidate.
    Re-pinned on 2026-11-09 from `v0.8.0` @ `d976d3f` (tree `bf9f277`).
    When `v0.8.1` is tagged, check that its tree is `947b850`; if not,
    record the difference as a delta.
  - The patch branch `dev-2.3.1` holds only the version bump. Every later
    change in an old repository (hotfix, briefing §3) is recorded as a
    delta against the pin.
- `docs/inventory/calc-u-59.md`, `docs/inventory/calc-u-1600.md`: per
  capability (keyboard, presets, debugger, settings, media/files, audio,
  pacing, threading, display, web shell, release tooling, tests/headless,
  help-site material, …) how the product does it, where it lives, how mature
  it is.
- Modelling depth per machine (briefing §1.9): which chips or blocks are
  modelled separately, which are folded together, which connectors between
  components and peripherals are explicit and which are implicit, and where
  functional emulation is used. For the TI-59 this includes the printer
  connector, the module slot, and why `examples/assembly/Decoder.asm` does
  not run.
- Test harnesses and headless tools are listed with the capabilities
  each one provides (briefing §8).
- Existing tests per product and frontend (briefing §1.10): kind (unit, GUI,
  golden, …), platform (macOS, iOS simulator, Windows, Linux, browser), where
  they run (locally, GitHub Actions), and elapsed time.
- `docs/inventory/agent-knowledge.md`: agent knowledge to update after
  phase 2 (briefing §1.11): every skill, `CLAUDE.md`, memory file and
  agent-facing developer doc of both products and of the research skills.
- `docs/inventory/calc-u-site.md`: site structure, material per product,
  duplications, how the web clients are embedded. Findings include the
  stale README (it still describes the `gh-pages`/calcu59.ch setup).
- `docs/inventory/release-tooling.md`: ReleaseIt's single-product
  assumptions (one root `release.yaml`, `vX.Y.Z`/`-pre` tags, `dev-x.y.z`
  branches, one CHANGELOG), both `release.yaml` and `build.yml` files, the
  SDE artifact pull, the App Store pipeline.
- `docs/inventory/divergence-matrix.md`: capability × product, with the
  contradictions and a *suggested* base side (the decision comes in phase 3).
- `docs/inventory/backlog.md`: every item of both `TODO.md` files and of
  `Calc-U-59/ideas/`, each with a target: design input, shared backlog,
  product backlog, done, or dropped (with reason). Briefing §1.7, §7.
  - One row per bullet or sub-bullet that is its own idea; headings and
    pure context lines get no row.
  - Each row has a stable ID (`B59-n`, `B1600-n`) and a source reference
    (file:line at the pinned SHA).
- Harvest both projects' "deliberate oddities" (Calc-U-1600
  `docs/background/Decisions.md`, Calc-U-59 `reference/`) so they survive the
  merge.
- Compatibility surface for Calc-U-59 (briefing §1.2): every explicitly
  versioned file format the latest release reads, including bundled files
  and files hosted on calc-u.org. Unversioned legacy formats it still
  accepts are listed as dropped, with every bundled or hosted file that
  uses one. For Calc-U-1600: which formats already carry a version number.
  Every format of both products is tagged own or external.
- GUI concepts per product and frontend (briefing §1.3, §1.5): calculator,
  printer, debugger, preset loader, magnetic card, module and floppy disk
  loaders, file load
  and save, bundled material. For each: where it sits in the GUI, how it is
  reached, and where the products already drift apart.
- Method: one background agent per product repository, plus one for
  calc-u-site, release tooling and agent knowledge; the matrix and the
  merged backlog are written from their reports, not from file dumps.
  Prompt: `prompts/phase-1.md`.

### Phase 2 — Domain model
- `docs/GLOSSARY.md`: Product, Library, Machine, Model, Variant,
  Family, Component, Connector, Peripheral, Module, Medium, Preset, Machine
  session, App session, Frontend, Platform, and the user-facing terms
  (briefing §1.5). Renames: `docs/glossary-mapping.md`.
- Every later document uses only these terms. Conflicting names in the two
  codebases (for example `SessionController` vs `AppSession`,
  `EmulatorSession` vs `MachineSession`) are mapped.
- The agent knowledge listed in phase 1 is updated to the glossary terms
  in phase 6, together with the code renames (briefing §1.11).

### Phase 3 — Architecture decisions
- One ADR per fork: context, options, decision, consequences. Candidates:
  - Threading model (Calc-U-59 threaded runner vs Calc-U-1600 coroutine
    jobs), with the Swift/iOS frontend in mind.
  - The core seam: one machine interface for all products, or a small
    library-level machine interface plus product-level extensions
    (briefing §6).
  - Peripherals keyed by connector or by machine.
  - Modelling depth and explicit connectors (briefing §1.9): how a machine
    moves along the chip-level ↔ single-core spectrum, and whether the
    TI-59 printer connector and module slot become explicit.
  - Swift bridge boundary.
  - Preset format, versioning and the Calc-U-59 read-old/write-new upgrade.
  - GUI sharing and family coherence: which GUI concepts, structure and
    components are shared across products and frontends (briefing §1.3,
    §1.5).
  - Repository layout, CMake structure, CI.
  - Versioning, tags, branch model (replacing `dev-x.y.z`), release location.
  - History archive, product naming, ROM removability (design only).
  - Skill structure: deferred until after the migration (Briefing §6).
  - Testing concept (briefing §1.10): test groups and their time budgets,
    GUI test strategy across Qt6, Swift and Web, installation and smoke
    tests on Windows and Linux.
- Each existing "deliberate oddity" either becomes an ADR or stays in a
  product-level decisions file.

### Phase 4 — Contracts
- One document per seam: machine interface and descriptor, frontend
  interface, peripheral and connector model, typing strategy (keyboard),
  preset engine and format, debugger (debug target and DAP), settings and
  paths, media and save folder.
- Each contract states responsibilities, explicit non-responsibilities and
  invariants, and contains a **fit table**: how TI-59, PC-1500, PC-1600 and
  an SC61860 model (PC-1403 for a character LCD, PC-1350 for a graphic LCD)
  map onto it, and at which modelling depth (briefing §1.9).
- Design against three very different model families, implement two. The
  SC61860 machines are paper machines; the TI-59 (TMC0501, PC-100C cradle,
  magnetic card reader) is the main stress test for anything Sharp-shaped.

### Phase 4b — Capability specs
- Kaldris template (`spec.md`: dependencies, user stories, out of scope,
  acceptance criteria, edge cases, open questions, decision log). IDs
  `LIB-n` for library features, `C59-n` and `C1600-n` for product
  features; status only in `features/INDEX.md`.
- Acceptance criteria are written per product and frontend, which makes them
  the parity checklist. They describe today's behaviour of each product
  (briefing §1.3: no new product features).

### Phase 5 — Validation on paper
- Scenarios played against the contracts, for example: add a PC-1403; add a
  CE-126P; TI-59 App Store build without DAP or PTY; add a Swift frontend to
  Calc-U-1600; the PC-1600 using the PC-1500 tape codec; ship without ROMs;
  refine the TI-59 to individual chips so `Decoder.asm` runs; attach the PC-100C printer and a Solid State ROM module through
  explicit connectors.
- A scenario that needs a change to the library means the abstraction
  is wrong; fix the contract, not the scenario.
- Reviewed by a fresh agent that sees only the contracts, not the reasoning
  behind them.
- Spikes only where paper is not enough (for example threading on iOS);
  thrown away afterwards.

### Phase 6 — Migration plan, then code
- Steps, each leaving every product building, testing and releasable:
  repository merge with history archived, CI for all products, golden tests
  as the safety net, then one capability extraction at a time.
- Agent knowledge (briefing §1.11) is updated to the glossary terms in the
  same steps as the code renames it describes, following
  `docs/glossary-mapping.md`.
- Each extraction is a `LIB-n` feature with `design.md` and `qa-report.md`
  (chain: AC → task → test).
- Refactoring inside either product (for example a machine interface in
  Calc-U-1600) happens here, after the contracts, not before.

## Per-phase prompts
Each phase gets a prompt file `prompts/phase-N.md` in the style of the
Kaldris `BriefingPrompt.md`, written at the start of the phase, so a phase can
be run or resumed in a fresh session.
