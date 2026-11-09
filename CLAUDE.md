# Calc-U

One repository for the Calc-U emulators, released as separate products
(Calc-U-59, Calc-U-1600, possibly a third for SC61860 machines later).

- Scope, decisions and constraints: `Briefing.md`
- How we get from the briefing to code, and the current phase: `docs/Process.md`

## Current phase: specification, no code

- Do not write or change code, in this repository or in the product
  repositories. Reading them is fine.
- Product baselines: Calc-U-59 at `/Users/me/Development/public/Calc-U-59`
  (tag `v2.3.0`), Calc-U-1600 at `/Users/me/Development/sharp/Calc-U-1600`
  (`v0.8.1`; until it is tagged, branch `dev-0.8.1` @ `b4ea8c8`). Pins and history:
  `docs/Process.md`, phase 1.
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
