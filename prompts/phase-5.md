# Phase 5 — Validation on paper (reduced): agent prompt

Prompt for a session that runs phase 5 (`docs/Process.md`). Read this
whole file, then start with "Method", step 1.

## Context

Calc-U merges Calc-U-59 (TI-58/58C/59) and Calc-U-1600 (Sharp
PC-1500/1500A/1600) into one repository with a library. The 33 feature
specs of `features/INDEX.md` are approved ("Specified"). Each was read
cold by Opus 5.5, one spec at a time. Nobody has yet checked how the
specs behave **together** on real material.

The user reduced phase 5 on 2026-11-11 to two things:

1. **The preset pass.** The presets, examples and test files both
   products ship today are played, step by step and on paper, against
   the specs.
2. **The first steps.** Everything the specs leave to a spike or to a
   check on a running product is collected, so that the migration plan
   (phase 6) starts with it.

The hypothetical scenarios of the old phase 5 (a PC-1403, a CE-126P, the
TI-59 refined to chips) are dropped; do not write them.

The finding that led to this phase, as a measure of what to look for:
`examples/screentest/test_cdebug.ti59` of Calc-U-59 arms FREEZE ON
START, starts a program and then has `Wait: 1.5s`. In 2.3.0 a `Wait:` is
wall-clock time and passes while the machine is frozen. The spec made
`wait:` emulated time that waits for the machine to run, so the preset
would have hung and its UI test with it. No cold read saw this; a real
preset did. (It is fixed: LIB-6 AC-19.)

Read first, completely:
- `CLAUDE.md`, `docs/GLOSSARY.md`, `features/INDEX.md`
- `docs/behaviour-changes.md` (where the specs deliberately differ from
  today)
- the specs that govern presets and what presets touch: LIB-6, C59-2,
  C1600-4, LIB-5, LIB-7, LIB-8, LIB-10, LIB-13, LIB-18, C59-7, C59-4,
  C1600-2, C1600-3; others as a trace leads to them

## Rules

1. **No code changes**, here or in the product repositories. Read the
   products only at their pins, with `git show <pin>:<path>` and
   `git grep <pattern> <pin>`; never check out, build or run them.
   - Calc-U-59: `/Users/me/Development/public/Calc-U-59`, tag `v2.3.0`.
   - Calc-U-1600: `/Users/me/Development/sharp/Calc-U-1600`,
     `b4ea8c8` (`dev-0.8.1`) until `v0.8.1` is tagged.
2. **Facts before findings.** What a product does today is read from
   its code or its tests at the pin and cited as `path:line`. A finding
   that rests on a guess about today's behaviour is not a finding.
3. **Don't invent.** Where a spec is silent, the finding is "silent",
   with a recommendation; it is not filled in.
4. **The specs are approved.** A spec changes only in two cases: a
   decision that is already recorded (a ticked question, a decision log
   row, a ledger row) is missing from a criterion, which you fix and
   note; or the user rules on a finding. Everything else goes to the
   user first.
5. **Stable IDs.** A new criterion gets a fresh number at the end of
   its group; nothing is renumbered. Every change to a spec gets a
   decision log row with its date and source.
6. **A new user-visible change** gets a row in
   `docs/behaviour-changes.md` in the same step, after the user decided.
7. **Don't reopen** `Briefing.md` §3 or an accepted ADR without the
   user. Glossary terms only.
8. Commit per step; do not push unless the user asks.

## Material

Calc-U-59 at `v2.3.0`:
- `examples/`: the ten presets at the top, `screentest/` (6),
  `debug/` (14), `historic/` (22), `calendar/` (10), and the ASM
  overlays in `assembly/` (5)
- `tests/fixtures/statefiles/`, `tests/fixtures/cards/`
- `Web/presets.json` (what the Web frontend bundles)
- the users of these files: `Calc-U-59UITests/` (XCUITest),
  `Web/tests/`, `tools/agentic/` and `tools/headless/`

Calc-U-1600 at `b4ea8c8`:
- `examples/**` (34 presets in eight folders)
- `docs/developer/screenshots/presets/` (9), `dev/presets/` (4),
  `dev/hardware-checks/` (3), `Core/tests/fixtures/presets/` (5)
- `vscode/calcu1600-debug/presets/` and `templates/**` (6)
- the users of these files: the screenshot scenarios, `Web/tests/`,
  `dev/tape-matrix/run.py`, `tools/dap_smoke.py`, the VS Code extension

calc-u-site at `acd629c` hosts copies of Calc-U-59's examples
(`docs/inventory/calc-u-site.md`); they need no trace of their own.

## Method

1. **Coverage table.** List every file of the material. For each, note
   the model, the header keys, sections and directives or steps it
   uses, and who uses the file (a test, a tool, the site, nobody). From
   this make a table: construct of today's formats → files that use it.
   A construct no file uses is listed as such.
2. **Choose what is traced.** Traced in full: every file a test or a
   tool depends on (the screenshot and UI test presets, the fixtures,
   the startup presets, the VS Code presets and templates), and for
   every construct of the table at least one file that uses it. The
   rest of a folder is traced when its first file produced a finding.
   Write down what was left out and why.
3. **Trace.** For each chosen file:
   - write it in the new format by the upgrade rules of the specs
     (C59-2 and C1600-4 map today's keys; LIB-6 gives the grammar); a
     line that has no mapping is a finding;
   - go through it line by line and name the criterion (spec and AC or
     EC) that governs each line;
   - say what happens: what is built, what emulated time passes, whether
     the machine is paused, what "busy" says, what is on the paper and
     the display, where media are read and written;
   - compare with what the product does today, from the code or the
     test at the pin;
   - follow the file's users: what a test or tool waits for, reads or
     compares afterwards, and whether that still holds.
4. **Findings.** Record one where: no criterion covers a line; two
   criteria disagree; the outcome differs from today and no row of
   `docs/behaviour-changes.md` says so; the preset would hang, fail or
   need a wall clock where it has none (the harness); a test or tool
   that depends on the file would break; or a spec can be read in two
   ways that give different outcomes.
5. **Who traces.** Fresh Opus 5.5 subagents trace; each gets the files
   of one group, this prompt's "Rules", "Trace" and "Findings", and
   reads the specs itself. It does not get this session's reasoning.
   Opus implements from these specs in phase 6, so a spec it misreads
   is a finding even when the spec is right. The main session checks
   every finding against the spec text and the pinned code before it
   counts, and drops what does not hold.
6. **Sort the findings.**
   - A: a recorded decision missing from a criterion. Fix it, note it.
   - B: needs the user. Put these to the user as one numbered list:
     facts first, one recommendation per point. The user answers by
     number, tersely, and often from knowledge of the real hardware; do
     not build consequences on an answer that the user did not state,
     and say what you read into one.
   - C: cannot be settled on paper. It goes into the first steps.
7. **The first steps.** Collect from the specs, the ADRs and the ledger
   everything that is left to a spike, to "the first task" or to a
   check on 2.3.0 or 0.8.1 (search for "first task", "spike", "verify
   on", "when this feature is built", "to be confirmed"), and add the C
   findings. For each: what is checked and how; which criteria depend
   on it; what changes in which spec if it turns out the other way.
   Order them so that what most specs depend on comes first. Known so
   far: the Swift session on a background thread (ADR 0015); harness
   attach in the iOS simulator and its port (ADR 0033, C59-5); 100 ms
   of key readiness as "idle" on a TI machine (LIB-6 AC-36); the
   PC-1500's key readiness (LIB-5 AC-32); Reset and the partition
   (C59-1 AC-6); print instructions without a printer (C59-6 AC-7); the
   error for an empty card side (C59-7 EC-2); the TI-58C's constant
   memory at a start (row 59-15).

## Output

- `docs/walkthroughs/presets.md`: the coverage table; what was traced
  and what was left out; per traced file one short entry (holds, or its
  findings); the findings, each with its class, its facts and what
  became of it.
- `docs/walkthroughs/first-steps.md`: the list of step 7.
- Spec changes, each with its decision log row; ledger rows where a
  visible change was decided.

## Done when

- every construct of the coverage table was traced at least once, and
  every file a test or tool depends on in full;
- every finding is fixed (A), ruled on by the user and applied (B), or
  in the first steps (C);
- the user has reviewed both documents. Then `docs/Process.md` gets the
  phase status, and phase 6 starts with the migration plan, whose first
  steps are those of `first-steps.md`.
