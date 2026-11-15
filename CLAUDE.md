# Calc-U

One repository for the Calc-U products, released separately
(Calc-U-59, Calc-U-1600, possibly a third for SC61860 models later).

- Scope, decisions and constraints: `Briefing.md`
- How we get from the briefing to code, and the current phase: `docs/Process.md`

## Current phase: 6, migration and code

- Work follows `docs/migration-plan.md` in its order of steps (`M-n`).
  A step is a merge into `main` that is green by the local checks
  (build, Frequent tests); no approval is asked between steps. CI runs
  are slow: dispatch them only at the end of a part, before a release,
  or when a step changes the workflows, packaging or platform-specific
  code (user, 2026-11-15; plan §1). The
  user approves at the part boundaries only (A, B, C1, C2, D, E; plan
  §1): the review of the part that ends and the `design.md` files of the
  part that starts. Between boundaries ask only what cannot wait (a first
  step that turns out the other way, a spec that reads two ways, a
  user-visible change not in `docs/behaviour-changes.md`).
- No code for a feature before its `design.md` is approved; tests come
  from its acceptance criteria before the code (AC → task → test);
  `qa-report.md` reports per AC ID.
- Every step leaves every product green and releasable. Golden runs are
  recorded before a core is touched; a difference afterwards is a
  finding, not a new reference.
- Specs change only where a first step turns out the other way or the
  user rules: a decision-log row, new criteria the next free number,
  nothing renumbered.
- Agent knowledge follows the code: a rename changes the skills,
  `CLAUDE.md` files, memory and agent docs that use the old name in the
  same step (`docs/glossary-mapping.md`).
- The old repositories are not changed. Until a product is imported it
  is read only at its pin: Calc-U-59 at `/Users/me/Development/public/Calc-U-59`
  (tag `v2.3.0`), Calc-U-1600 at `/Users/me/Development/sharp/Calc-U-1600`
  (tag `v0.8.1`; `b4ea8c8` plus a docs-only delta). Pins and history:
  `docs/Process.md`, phase 1.
- Library code: one target `calcu_library`, a folder per area under
  `library/`; the layout and its rules are in
  `features/LIB-1-machine-interface-and-descriptors/design.md` §2a.
- Short branches merged into `main` (ADR 0029); commit per step; no push
  unless asked.
- Don't reopen anything listed in `Briefing.md` §3 (decisions already made).
- Don't invent missing facts (sizes, behaviour, limits). Ask, or record them
  as open questions in the document you are writing.

## Key conventions

- **Working language: English**, for all documents, research notes, skills,
  code, comments and commit messages. Source material (PDFs, manuals) may be
  in other languages.
- **Terms:** once `docs/GLOSSARY.md` exists, use only its terms.
- **Acceptance criteria and edge cases:** "Given [precondition], when
  [action], then [outcome]", with stable IDs (AC-1, EC-1, …).
- **Feature files** (`features/<ID>-<name>/`; IDs `LIB-n` for library
  features, `C59-n` / `C1600-n` for product features):
  - `spec.md` is the stable contract and describes WHAT.
  - `design.md` describes HOW.
  - `qa-report.md` reports results per AC ID.
  - Chain: AC → task → test. Status lives only in `features/INDEX.md`.
- **Decisions** are recorded as ADRs in `docs/adr/`, one per decision.
- Phase status lives only in the table in `docs/Process.md`.

## Research

Hardware and firmware facts come from the research skills and repositories
listed in `Briefing.md` §5 (`PC1500`, `SC61860`, `pc1500-build`,
`ti59-research`). Don't restate them here; link to them.
