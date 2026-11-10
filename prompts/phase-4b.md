# Phase 4b — Feature specs: agent prompt

Prompt for a session that writes or reviews feature specs
(`docs/Process.md`, phase 4b). Read this whole file, then work on the
specs you were given.

## Context

Calc-U merges Calc-U-59 (TI-58/58C/59) and Calc-U-1600 (Sharp
PC-1500/1500A/1600) into one repository with a library. The contracts
(phase 4) are approved. Phase 4b turns them into one `spec.md` per
feature of `features/INDEX.md`. The specs are what Claude Opus 5.5
implements from in phase 6, in fresh sessions, so a spec must be
readable cold: with the contracts it names, and nothing else.

Read first, completely:
- `CLAUDE.md`, `docs/GLOSSARY.md`
- `features/INDEX.md` (the rules of the cut, the status columns)
- `docs/behaviour-changes.md`
- the contracts and ADRs named in the feature's row, and the specs of the
  features it depends on

## Rules

1. **No code changes**, here or in the product repositories. Read the
   products only at their pins, with `git show <pin>:<path>` and
   `git grep <pattern> <pin>`; never check out, stash or commit there.
   - Calc-U-59: `/Users/me/Development/public/Calc-U-59`, tag `v2.3.0`.
   - Calc-U-1600: `/Users/me/Development/sharp/Calc-U-1600`,
     `b4ea8c8` (`dev-0.8.1`) until `v0.8.1` is tagged.
2. **WHAT, not HOW.** No class names, signatures or file layouts of the
   new code. Names from today's code appear only as sources of facts.
3. **Today's behaviour, or a decided change.** An acceptance criterion
   describes what the product does today, with its source
   (`path:line` at the pin, or an inventory section), unless
   `docs/behaviour-changes.md` lists a change; then it describes the new
   behaviour and names the row (for example "59-11").
4. **Don't invent.** A size, limit or behaviour that no contract, ADR or
   pinned source gives is an open question, with a recommendation where
   one is possible. Hardware and firmware facts come from the research
   skills (`Briefing.md` §5).
5. **Don't reopen** `Briefing.md` §3, an accepted ADR or a decided fork
   of a contract. If a spec cannot be written without changing one, stop
   and ask the user.
6. **Glossary terms only.** A term the glossary lacks is proposed to the
   user, not coined in a spec.
7. **Stay inside the row.** What another feature owns is named under
   "Out of scope" with that feature's ID (rules of the cut,
   `features/INDEX.md`). If the row itself turns out wrong, say so
   instead of bending the spec.
8. **A new user-visible change** found while writing goes to the user
   first; once decided, it gets a row in `docs/behaviour-changes.md` in
   the same step.

## The file

`features/<ID>-<name>/spec.md`, the name in kebab-case from the index.
The template is the Kaldris one
(`/Users/me/Development/mac/Kaldris/BriefingPrompt.md`), with the
acceptance criteria grouped by status column:

```
# <ID>: <Name>

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: …
- Contracts and decisions: …

## User Stories
- As a [user, developer or agent], I want … so that …

## Out of Scope
- … (owner: <ID>)

## Acceptance Criteria
### Library
- [ ] **AC-1** — Given …, when …, then …
### Calc-U-59
- [ ] **AC-n** — …
### Calc-U-1600
- [ ] **AC-n** — …

## Edge Cases
- **EC-1** — Given …, when …, then …

## Technical Requirements (optional)

## Open Questions
- [ ] **OQ-1** — … *Recommendation:* …

## Decision Log
| Decision | Rationale | Date |
```

- A product feature (`C59-n`, `C1600-n`) has one group of acceptance
  criteria, split by frontend where the frontends differ.
- A library feature's groups are its status columns: a column is Done
  when its group passes. A criterion that holds in every frontend of a
  product says so once; one that differs names the frontend (Swift iOS,
  Swift macOS, Qt6, Web).
- `AC-n`, `EC-n` and `OQ-n` each run through the whole file. A draft
  may be renumbered until the user approves the spec; from then on IDs
  are stable and never reused, and a criterion that is dropped keeps its
  number, struck through.
- Every criterion must be checkable: by a test at the feature's seam,
  through the harness, or by a named GUI scenario (ADR 0032). One that
  can only be checked by reading code is an invariant and goes under
  "Technical Requirements".
- The decision log starts with the decisions the spec inherits, each
  with its contract or ADR ID, so the implementer sees why. A number or
  rule the spec itself adds to make a criterion checkable is listed
  there as "proposed in this spec", so the user sees it at the gate.
- Words the glossary lacks are used only with a definition in the spec
  and an open question that proposes them for the glossary.
- "Web frontend", not "web client" (glossary, Frontend).

## Order

By dependency, so a spec can cite the ones before it:

1. LIB-1, LIB-9, LIB-17
2. LIB-2, LIB-3, C59-1, C1600-1, C59-6
3. LIB-4, LIB-11, LIB-12, LIB-15, C59-5
4. LIB-5, LIB-18
5. LIB-6, LIB-14, C59-3
6. LIB-7, LIB-8, LIB-10, C59-2, C1600-4, C59-8
7. LIB-13, C59-7, C1600-3
8. C59-4, C1600-2
9. LIB-16, LIB-19

## Per spec

1. Collect the facts (contracts, ADRs, pinned sources) before writing.
2. Write the spec. Set the row to "Specified" only after the user has
   approved it.
3. Review: a fresh Opus 5.5 session that gets only the spec and the
   contracts it names lists what is ambiguous, untestable or missing.
   Its findings are answered in the spec or become open questions.
4. Show the user the open questions of the batch; answers go into the
   spec and its decision log, and the question is removed.
5. Commit per batch; do not push.
