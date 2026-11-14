# Phase 6 — Migration plan, then code: agent prompt

Prompt for a session that runs phase 6 (`docs/Process.md`). Read this
whole file, then start with "Part A", step 1. Part B starts only after
the user approved the plan.

## Context

Calc-U merges Calc-U-59 (TI-58/58C/59) and Calc-U-1600 (Sharp
PC-1500/1500A/1600) into one repository with a library. Phases 0 to 5
are done:
- the 33 feature specs of `features/INDEX.md` are approved
  ("Specified"); the preset formats are in `docs/formats/`; the decided
  user-visible changes are in `docs/behaviour-changes.md`;
- the preset pass of phase 5 was applied to the specs, the ADRs and
  `docs/formats/`; its findings (F-n, B-n, C-n) are cited in decision
  logs and mean `git show 536f713:docs/walkthroughs/presets.md`;
- `docs/walkthroughs/first-steps.md` (approved 2026-11-13) lists 15
  spikes, checks, records and research steps that come before anything
  is built on what they test.

Already in the repository (user-approved exception to "no code",
2026-11-13): the ADR 0002 tree with empty product and library
directories, `products/calc-u-59/frontends/swift/Calc-U-59.xcodeproj`
(MARKETING_VERSION 3.0.0) and `products/<p>/roms/fetch_roms.sh`
(commit `db5500a`). In the ROM repositories: `TI-59-ROM` @ `90cd4a7`
(the 14 `modules/<ID>.rom.yaml`; no remote yet) and `PC-1500-ROM` @
`b664f0b` (`modules/CE-502B.card.yaml`). Open: LIB-12 AC-9's md5 table
still lists the 14 dumps that AC-14 replaces.

Read first, completely:
- `CLAUDE.md`, `Briefing.md` (§1.1, §1.10, §1.11, §3, §7, §8 above
  all), `docs/Process.md`, `docs/GLOSSARY.md`,
  `docs/glossary-mapping.md`
- `features/INDEX.md` (dependencies, the three status columns, "the
  unit of work is a feature and a column")
- `docs/walkthroughs/first-steps.md`
- the ADRs that are migration steps, not features: 0002-0005,
  0027-0031, 0034, 0038; and 0013-0015, 0033 for the Swift frontend
- `docs/behaviour-changes.md`, `docs/inventory/divergence-matrix.md`,
  `docs/inventory/release-tooling.md`, `docs/inventory/agent-knowledge.md`,
  `docs/inventory/backlog.md`
- the specs themselves as a step leads to them; every spec's "Technical
  Requirements" (golden runs before a core is changed, tests before a
  pacer moves, research recorded first)

## Rules

1. **No code until Part A is approved.** Until then the phase 5 rule
   holds: products only read at their pins (`git show <pin>:<path>`):
   Calc-U-59 `/Users/me/Development/public/Calc-U-59` at `v2.3.0`,
   Calc-U-1600 `/Users/me/Development/sharp/Calc-U-1600` at `b4ea8c8`
   (`dev-0.8.1`) until `v0.8.1` is tagged. When the user approves the
   plan, the "Current phase" section of `CLAUDE.md` is rewritten with
   the user, not before.
2. **Every step leaves every product green**: building, its Frequent
   tests passing, releasable (`docs/Process.md`, phase 6). A step that
   cannot is split.
3. **Golden runs before core changes.** What a core does is recorded
   on the code as imported before the core is touched (first steps 2,
   6, 13, 14). A difference afterwards is a finding, not a new
   reference.
4. **The old repositories are not changed.** Every later change in
   them is a delta against the pin, recorded (`docs/Process.md`, phase
   1). The source repository starts from imported trees, not from the
   histories (ADR 0030).
5. **The specs are approved.** A spec changes only where a first step
   turns out the other way (each step says what then changes) or the
   user rules; every change gets a decision log row, new criteria the
   next free number, nothing is renumbered. A user-visible change gets
   a row in `docs/behaviour-changes.md`.
6. **Agent knowledge follows the code** (Briefing §1.11): a rename in
   code and the skills, `CLAUDE.md` files, memory and agent docs that
   use the old name change in the same step
   (`docs/glossary-mapping.md`). Today's tools and harnesses keep their
   capabilities in every step (Briefing §8).
7. **Constraints** (Briefing §8): language versions only raised, each
   raise an ADR; the minimum OS versions stay; each artifact keeps its
   build location; a local build is always possible.
8. **Don't invent, don't reopen.** Briefing §3 and accepted ADRs stay
   closed without the user. Where the plan needs a decision nobody has
   made (for example which product is imported first, or whether the
   import waits for the `v0.8.1` tag), it goes to the user as a
   question with a recommendation. Glossary terms only.
9. **Status** lives only in `features/INDEX.md` (features) and
   `docs/Process.md` (phases); the plan names steps, not states.
10. Commit per step, with the attribution line; do not push unless the
    user asks. In Part B work happens on short branches merged into
    `main` (ADR 0029).

## Part A — the migration plan

1. **Dependencies.** From `features/INDEX.md` draw the graph of
   feature columns: direct dependencies, plus "a product column needs
   its machine row first" and "a product column may complete only with
   the product row that builds on it" (the spec says where). Add the
   first steps each criterion waits for (`first-steps.md`, "Depends on
   it"). A cycle or a column nothing can start is a finding.
2. **Steps that are not features.** List what INDEX.md calls "not
   features": importing each product's tree into the ADR 0002 layout,
   the root CMake project (0003), one CI workflow per product (0004),
   C++20 (0005), the test runner and the test groups with their budgets
   (0031, 0038), the ROM fetch at build time (0034), versions, tags and
   releases (0027-0029), archiving the old repositories (0030). Each
   becomes a step or part of one.
3. **Order.** Write `docs/migration-plan.md` as numbered steps `M-n`:
   - first the first steps, in the order of `first-steps.md`; the ones
     that need only the products at their pins (checks, records,
     research) may come before the import, the spikes after the
     infrastructure they need;
   - then the infrastructure that makes "green" measurable (import,
     build, CI, Frequent tests, golden runs);
   - then one feature column at a time along the graph of step 1,
     machine rows first.
4. **Each step says:**
   - what is done, in one paragraph;
   - the feature and column it completes, or the ADR it carries out,
     and the acceptance criteria (by ID) that become tests in it;
   - the first steps it waits for, and what is re-planned if one of
     them turns out the other way (taken from `first-steps.md`, not
     re-derived);
   - what "green" means after it: which test groups, on which
     platforms, and the budget of ADR 0031;
   - the agent knowledge that changes with it (from
     `docs/inventory/agent-knowledge.md`) and the ledger rows of
     `docs/behaviour-changes.md` that become visible in it;
   - its exit check: what the user or a test sees when it is done.
5. **Coverage.** Check, and state in the plan, that every acceptance
   criterion of every spec, every row of `docs/behaviour-changes.md`,
   every item of `docs/inventory/backlog.md` targeted at design or done,
   and every success criterion of Briefing §7 lands in a step, or is
   named as outside phase 6 (for example the skill cut, Briefing §6).
   Work that `first-steps.md` lists under "Not first steps" (the upgrade
   of the shipped and hosted presets, the Python helper for
   `calcu serve`, the PC-100A note) gets a step too.
6. **Release points.** Mark the steps after which a product can be
   released from this repository (Briefing §7: one release each, the
   App Store for Calc-U-59, calc-u.org updated), and what each release
   still lacks if it is cut there.
7. **Cold read.** A fresh Opus 5.5 subagent gets the plan, INDEX.md,
   `first-steps.md` and the ADRs, not this session's reasoning, and
   checks: every step can be started with what the steps before it
   leave; no step leaves a product red; nothing of step 5 is missing.
   The main session checks each finding against the sources before it
   counts.
8. **Questions to the user.** As one numbered list, facts first, one
   recommendation per point. The user answers by number and tersely;
   say what you read into an answer and build nothing on what was not
   stated.

**Output of Part A:** `docs/migration-plan.md`; the questions of step 8
answered and applied. Then the user reviews the plan.

## Part B — carrying out the plan

Per step of the plan, in order:
1. **First steps** are carried out as `first-steps.md` describes; the
   result goes where the step says (the spec, a golden run, the
   research repository). A result that turns out the other way goes to
   the user before a spec changes.
2. **Feature columns:** `features/<ID>-*/design.md` (HOW: modules,
   seams, files, the tasks with the AC each serves) is written and
   approved by the user before code. Then tests from the acceptance
   criteria, then the code (chain AC → task → test). Opus implements
   from the specs; where a spec reads two ways, ask, do not choose.
3. **`qa-report.md`** reports per AC ID: the test that shows it and
   its result, or why it is not shown yet.
4. **Status:** the column in `features/INDEX.md` moves to In progress
   when its design is approved and to Done when its qa-report shows
   every AC of that column passing.
5. **Green before the next step:** every product builds and its
   Frequent tests pass; the Longer group before a merge into `main`.

## Done when

- `docs/migration-plan.md` is approved by the user (end of Part A;
  `docs/Process.md` notes it in the phase 6 row);
- every step of the plan is done and every column of `features/INDEX.md`
  is Done;
- the success criteria of Briefing §7 are met. Then `docs/Process.md`
  gets the phase status, and the skill cut of Briefing §6 follows.
