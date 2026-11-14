# Migration plan

Phase 6, Part A (`docs/Process.md`; prompt `prompts/phase-6.md`). The
order of steps that takes Calc-U-59 (`v2.3.0`) and Calc-U-1600
(`dev-0.8.1` @ `b4ea8c8`) into this repository and onto the library,
each step leaving every product green. Status lives in
`features/INDEX.md` (features) and `docs/Process.md` (phases); this plan
names steps, not states.

Sources: `features/INDEX.md`, the 33 specs, `docs/walkthroughs/first-steps.md`
(below "FS n" is its step n), ADR 0002-0005, 0013-0015, 0027-0031, 0033,
0034, 0038, `docs/behaviour-changes.md` (the ledger),
`docs/inventory/`. Draft of 2026-11-14; the questions of §7 were
answered on 2026-11-14 and are applied. For the user's review.

## 1. How to read a step

Each step `M-n` has six fields:
- **Does:** what is done.
- **Completes:** the feature columns it finishes, or the ADR it carries
  out; **Tests from:** the acceptance criteria that become tests in it
  (`AC`/`EC` IDs of the spec named). The edge cases of a spec become
  tests in the step that completes the column they belong to; where a
  spec does not say, with its Library column or its product row.
- **Waits for:** the first steps it needs, and what is re-planned if one
  turns out the other way (taken from `first-steps.md`).
- **Green after:** what passes after it (§1.1); only what goes beyond
  the default is written.
- **Knowledge, ledger:** the agent knowledge that changes with it
  (`docs/inventory/agent-knowledge.md`; renames per
  `docs/glossary-mapping.md`) and the ledger rows that become visible.
- **Exit check:** what the user or a test sees when it is done.

**Steps and parts** (user, 2026-11-14). A step `M-n` is the order of
the work and a merge into `main` that is green by the automated checks
(build, Frequent, CI); no approval is asked between steps, and several
steps may be done in one session or branch. The user approves at the
boundaries of the parts only:

| Part | Steps | Contents |
|---|---|---|
| A | M-1 to M-5 | first steps at the pins, ReleaseIt |
| B | M-6 to M-17 | import, golden and reference runs, build infrastructure, spikes |
| C1 | M-18 to M-34 | machine interface, machine rows, sessions, harness, ROM lookup |
| C2 | M-35 to M-43 | the other Library columns |
| D | M-44 to M-55 | Calc-U-1600 to its release, `/1600/`, archive |
| E | M-56 to M-66 | Calc-U-59 to its release, `/59/`, archive, closing check |

At a boundary the user reviews the part that ends (a summary, its
`qa-report.md` files, the results of its first steps, the release-point
status) and approves the designs of the part that starts: one
`design.md` per feature, covering every column the part works on. The
Longer group runs at the end of a part and before a release; Frequent
and CI at every merge. Between boundaries the user is asked only what
cannot wait: a first step that turns out the other way, a spec that
reads two ways, a user-visible change not in the ledger.

Rules that hold for every step of Part B of the prompt (not repeated per
step):
- A feature's `design.md` is approved before its code (at the boundary
  that starts its part); tests from the criteria come before the code;
  `qa-report.md` reports per AC ID. A column moves to In progress when
  its design is approved and to Done when its qa-report shows every AC of
  that column passing.
- **Adapters.** Until the step that takes an old call behind a seam is
  done, the old call stays, behind an adapter that the design names. A
  step lists the adapters it removes. No adapter is left at the end
  (Briefing §7: a product that keeps its own implementation of a shared
  feature does not meet the criterion; M-62 checks it).
- **Today's tools** (`tools/headless/`, `tools/agentic/`,
  `pc1500_cli`, `pc1600_cli`, the probes, `tools/check_presets.sh`) stay
  usable until the step that replaces them (Briefing §8; M-51, M-62). A
  step that changes what a tool uses moves the tool onto the new part in
  the same step; the steps name where this happens.
- **"Completes" and Done.** "Completes" names the columns whose work a
  step does. A criterion that names a feature built later (a topic, a
  place, a file of another column) passes in that feature's step, and
  the column becomes Done there: §5.1, "Done in", is the place that says
  where each column becomes Done.
- A golden run that differs after a step is a finding, not a new
  reference (rule 3 of the prompt).
- Each step is one or more short branches merged into `main` (ADR 0029),
  one commit per step at least, with the attribution line.
- The steps run in their numbered order unless the user moves one;
  "Waits for" names the earlier steps whose result a step uses, not every
  step before it. Steps of Part A (M-1 to M-5) and M-17 may run side by
  side.

### 1.1 Green

- **Before the import** a product is green in its old repository at its
  pin; Part A of this plan does not change it.
- **Default, after the import of a product:** its workflow
  (`calc-u-59.yml`, `calc-u-1600.yml`, ADR 0004) passes every job it has
  today (core tests on ubuntu-24.04; Linux x86_64 and aarch64; Windows
  x86_64 and arm64; Web with Playwright in Chromium and WebKit;
  Calc-U-1600 also macOS arm64, signed and notarised); the Calc-U-59
  Swift app builds locally for macOS and the iOS simulator; the Frequent
  group passes locally within 60 s per product; the Longer group
  (10 min) passes locally at the end of each part: Playwright of
  both Web frontends, XCUITest `UIRegressionTests` on one simulator,
  Calc-U-1600's screenshot scenarios run as checks, `tools/dap_smoke.py`
  (ADR 0031). Measured on 2026-11-09: Frequent 6.75 s (Calc-U-59) and
  37 s (Calc-U-1600).
- The Checkpoint group runs at the release steps only.
- "Every product" means every imported product; a product not yet
  imported is green in its old repository.

### 1.2 Done before the plan

Approved by the user as an exception to "no code" on 2026-11-13, outside
this plan; the steps build on it and do not make it again:
- the ADR 0002 tree with empty product and library directories,
  `products/calc-u-59/frontends/swift/Calc-U-59.xcodeproj` from 2.3.0
  with `MARKETING_VERSION` 3.0.0, and `products/<product>/roms/fetch_roms.sh`
  with the fetch lists and md5 values of LIB-12 AC-9, AC-12, AC-14
  (`db5500a`);
- the ROM repositories prepared: TI-59-ROM (`90cd4a7`, public on GitHub)
  with the calculator and module dumps and the 14 `.rom.yaml` files made
  from the dumps and `cuecards.txt`; PC-1500-ROM (`b664f0b`) with
  `modules/CE-502B.card.yaml`. ADR 0034's TI ROM repository is therefore
  named and filled, and LIB-12 AC-14's files are made: M-24 and M-27 only
  switch the products to them.

## 2. Dependencies (Part A, step 1)

### 2.1 The graph of feature columns

`L` = Library column, `59` / `1600` = product column, `C…` = product
row. "→" reads "needs". Direct dependencies from `features/INDEX.md`,
plus "a product column needs its machine row" (C59-1, C1600-1) and the
couplings the specs state (marked ⊕: a column complete only together
with a row, or worked together with it).

| Column | Needs | Coupled (⊕) |
|---|---|---|
| LIB-1 L | — | |
| LIB-9 L | — | |
| LIB-16 L | — (a chapter is finished when the specs it follows are) | |
| LIB-17 L | LIB-1 L | |
| LIB-3 L | LIB-1 L, LIB-17 L | |
| LIB-2 L | LIB-1 L; the host link also LIB-3 L, LIB-9 L | |
| LIB-11 L | LIB-1 L, LIB-3 L | |
| LIB-12 L | LIB-1 L, LIB-3 L | |
| LIB-15 L | LIB-3 L, LIB-9 L | |
| LIB-4 L | LIB-2 L, LIB-3 L | |
| LIB-5 L | LIB-1 L, LIB-3 L, LIB-11 L | |
| LIB-18 L | LIB-2, 3, 9, 11, 12 L (a stand-in for the loader) | |
| LIB-6 L | LIB-2, 5, 9, 11, 18 L | |
| LIB-8 L, LIB-10 L | LIB-3, 6, 11 L | |
| LIB-7 L | LIB-4, 6, 9, 11 L | |
| LIB-13 L | LIB-6, 7 L | |
| LIB-14 L | LIB-4, 9, 18 L | |
| LIB-19 L | LIB-15 L, LIB-16 L; a release with a help asset | |
| C59-1 | LIB-1 L | LIB-1 59 ⊕ C59-1, C59-6 |
| C59-6 | C59-1, LIB-2 L; module files from LIB-12 AC-14 | LIB-2 59 ⊕ C59-6 |
| C1600-1 | LIB-1 L, LIB-2 L | LIB-1 1600, LIB-2 1600 ⊕ C1600-1 |
| LIB-3 59 | LIB-3 L, C59-1 | ⊕ C59-5 (Swift), LIB-15 59 (Web) |
| LIB-3 1600 | LIB-3 L, C1600-1 | ⊕ LIB-15 1600 (Web) |
| C59-5 | LIB-3 L, LIB-11 L; FS 3, FS 4 | |
| C59-3 | C59-1, LIB-9, LIB-18 (and LIB-3, LIB-15 for the Web) | ⊕ LIB-18 59 (AC-7 rests on LIB-18 AC-19), LIB-6 59 (AC-8, AC-10: presets that cold boot) |
| LIB-5 59 / 1600 | LIB-5 L, the product's sessions (LIB-3 59 / 1600) | |
| LIB-9 59 / 1600 | LIB-9 L, machine row | 59: AC-23 with LIB-15 59 (the `calcu59/` keys move) |
| LIB-12 59 / 1600 | LIB-12 L, machine row | 59: module names with C59-6 (AC-8), cue cards with C59-7 (AC-22, AC-23) |
| LIB-15 59 / 1600 | LIB-15 L, LIB-3 59 / 1600, LIB-9 59 / 1600 | 59: AC-20 with C59-3 and C59-7 (what moves to IndexedDB) |
| LIB-17 59 / 1600 | LIB-17 L, the product's sessions; FS 14 | |
| C1600-2 | C1600-1, LIB-5, LIB-11, LIB-14 L | AC-15 with LIB-14 1600 (AC-34) |
| LIB-2 1600 | LIB-2 L, C1600-1 | AC-32 with LIB-7 1600 (LIB-7 AC-19); AC-33 with LIB-3 1600 (the host link) |
| LIB-3 59 / 1600 (topics) | — | AC-29, AC-30, AC-38 map today's flags and callbacks to topics of LIB-4, 7, 8, 10, 18, C59-3, C59-4, C59-7: Done with the last of them |
| LIB-7 1600, LIB-8 1600, LIB-10 1600, LIB-14 1600 | L column, C1600-1, LIB-3 1600 | LIB-10 1600's preset criteria with LIB-6 1600 |
| LIB-6 59 / 1600 | LIB-6 L, machine row | ⊕ C59-2 / C1600-4 |
| C59-2 | C59-1, C59-6, LIB-6, LIB-8, LIB-10 | AC-10: the four card examples with C59-7; AC-12: the site copies with LIB-19 59 |
| C1600-4 | C1600-1, C1600-2, LIB-6, LIB-8, LIB-10 (LIB-7 for the presets of the last row of AC-15's table, and AC-15's version 2 disk) | |
| LIB-18 59 / 1600 | LIB-18 L, machine row, LIB-6 | |
| LIB-7 59 | LIB-7 L, C59-1 | ⊕ C59-7 |
| C59-7 | C59-1, LIB-7 | |
| LIB-13 59 / 1600 | LIB-13 L; the file kinds' owners: C59-7, C59-4 (AC-13: complete without the overlay) / C1600-2 | |
| C59-4 | C59-1, LIB-10, LIB-13 | AC-20: the hosted overlays with LIB-19 59 |
| LIB-4 59 / 1600 | LIB-4 L, machine row | ⊕ the rows that draw it: LIB-14, C59-8 |
| LIB-14 59 | LIB-14 L (Qt6, Web) | Swift is C59-8 |
| LIB-8 59, LIB-10 59 | L column, C59-6 (printer) / C59-1 | Swift parts in C59-8 |
| C59-8 | C59-5, LIB-4, 6, 7, 8, 10, 14 | |
| C1600-3 | LIB-10, LIB-11, C1600-2, C1600-4 | |
| C1600-5 | LIB-11, LIB-7, LIB-10 | |
| C59-9 | LIB-11, LIB-6, LIB-8, LIB-10, C59-2, C59-6 | |
| LIB-11 59 / 1600 | LIB-11 L, machine row | ⊕ every feature that delivers one of today's tools' functions: LIB-5, 6, 7, 8, 10, 18, C59-2, C59-6, C59-7, C59-9 / C1600-3, C1600-5 (AC-14, AC-18) |
| LIB-16 59 / 1600 | the specs its chapters follow | the help asset in the product's release |
| LIB-19 59 / 1600 | LIB-19 L, the product's first release | |

### 2.2 First steps the criteria wait for

| First step | Criteria that wait | Step that carries it out |
|---|---|---|
| FS 1 pin of Calc-U-1600 | every `path:line` of Calc-U-1600 | M-1 |
| FS 2 golden runs | C59-1 AC-4, AC-5; C1600-1 AC-4; C59-6 AC-3, AC-7, AC-9, AC-10 | M-8 |
| FS 3 Swift session spike | C59-5 AC-4 to AC-6; LIB-3 59; C59-8 | M-16 |
| FS 4 attach in the simulator | C59-5 AC-10 to AC-14, EC-1 to EC-4; C59-8 AC-17, AC-21, OQ-5; ADR 0032 for the Swift GUI; LIB-14 AC-44; LIB-16 AC-12 | M-16 |
| FS 5 key readiness and idle (TI) | LIB-6 AC-36, AC-56, AC-59; C59-1 AC-18; LIB-5 AC-20, AC-25; C59-2 AC-1, AC-7, AC-11; LIB-10 AC-13 (C-1) | M-3 |
| FS 6 TI reference runs | C59-2 AC-10, AC-11; C59-7 AC-30; LIB-6 AC-43; LIB-8 AC-39 | M-9 |
| FS 7 Reset and the partition | C59-1 AC-6; LIB-18 AC-18, AC-22 (through C59-1 AC-6); row 59-22 | M-8 |
| FS 8 print without a printer | C59-6 AC-7 | M-8 |
| FS 9 empty module slot | C59-6 AC-10; LIB-18's built-in TI-58/58C machine | M-8 |
| FS 10 empty card side | C59-7 EC-2; row 59-20 | M-3 |
| FS 11 TI-58C constant memory | C59-3 AC-2, AC-8, AC-9; C59-2 EC-3, AC-1; C59-1 AC-6; LIB-18 AC-19; row 59-15 | M-3 |
| FS 12 Mac app in the background | C59-5 AC-8, AC-9 | M-3 |
| FS 13 Sharp reference runs | C1600-4 AC-15, AC-16; LIB-6 AC-45, AC-46; LIB-5 AC-14, AC-17, AC-28; C1600-2 AC-13; LIB-8 AC-39 | M-4 |
| FS 14 long-run pacing tests | LIB-17 AC-18, AC-21 | M-10 |
| FS 15 PC-1500 key handling | LIB-5 AC-31, AC-32; C1600-1 AC-21; LIB-10 AC-38; row 1600-8; C1600-4 AC-4 (C-7) | M-5 (research), M-8 (C-7) |

### 2.3 Findings of the graph

- **No cycle.** The table lists every column. Every column can be
  started once the columns it needs are done; §3 follows that order. The
  couplings (⊕) do not form a cycle: each coupled row needs only the
  Library column of the feature it completes, and a criterion that names
  a later column passes there (§1). The nearest to a cycle was C59-3,
  which needs LIB-18's and LIB-6's Calc-U-59 columns; it is therefore
  built with them (M-57).
- **The LIB-11 product columns come last** of their product: they are
  complete only when every feature that replaces one of today's tools is
  (LIB-11, Dependencies). Today's tools stay until then (Briefing §8).
- **LIB-6's product columns are a convergence point.** A product reads
  only format 2 once its column is done, and its shipped presets use
  steps of LIB-5, LIB-7, LIB-8, LIB-10 and of the product rows. The
  specs already stage this: LIB-6 AC-43 and AC-51 leave out by name the
  presets that wait for LIB-7, LIB-10 or C59-7, and C1600-4 AC-13 says
  the presets of its last line are upgraded when LIB-7 and LIB-10 are
  built (the last row of its AC-15 table). Between the two steps those
  presets cannot be read. The plan
  keeps them out of the bundle file until their step (question 6).
- **ROMs at the import.** The 2.3.0 code reads the module dumps
  (`roms/solid-state/TMC05xx.txt`) and `cuecards.txt`, which also holds
  the module list, names and cue cards (`Shared/ModuleCatalog.hpp:9-36`);
  the fetch script of the scaffold (`db5500a`) fetches the 14 `.rom.yaml`
  files, which no code of 2.3.0 reads. TI-59-ROM has the dumps
  (`dumps/modules/`) but not `cuecards.txt`, which LIB-12 AC-14 says
  "then leaves the source repository", so it is in the source repository
  until then. The import fetches what the imported code reads (M-6); M-24
  takes the ROM bytes from the `.rom.yaml` files, M-25 the module names
  (C59-6 AC-8), M-58 the cue cards (C59-7 AC-22, AC-23), and
  `cuecards.txt` leaves then. The same holds for Calc-U-1600's
  `CE-502B.card.yaml` (read from `Qt6/resources/cards/` by 0.8.1;
  M-27).
- **Small inconsistencies in the specs**, fixed on 2026-11-14 with a
  decision-log row each (question 9; commit `76e9452`):
  - LIB-12 AC-9 still lists the md5 values of the 14 dumps that AC-14
    replaces (handover).
  - LIB-14, Dependencies, cites ledger row 1600-9 (keys dropped while
    something runs); the menus row is 1600-10, which LIB-14 OQ-7 asked
    for.
  - LIB-13 OQ-4 speaks of "one line in row 59-14 and its Calc-U-1600
    twin"; the file changes are rows 59-20 and 1600-15 now.
  - C1600-2 OQ-1 ("how it is got for every platform is a point for the
    migration plan"): SDE is fetched as today, a release download for
    Linux and Windows and the committed macOS and WASM libraries (M-7);
    nothing else is needed.
  - LIB-12 AC-14 (the TI modules) and LIB-16 AC-16 (Calc-U-59's help
    pictures) stand under "### Calc-U-1600"; the plan tests them with
    Calc-U-59 (M-24, M-62).
  - `docs/contracts/README.md` still names "a phase 5 walkthrough" for
    `machine.md` Q-1 and `peripherals.md` Q-2 and lists `capabilities.md`
    Q-1 as open (`first-steps.md`, "Not first steps").
- **Since the pin.** `dev-0.8.1` of Calc-U-1600 has two commits after
  `b4ea8c8` (`f0b3732`, `76baadf`): docs only (a PC-1500 RAM disk idea,
  `docs/background/PC1500-Card-Disk-Idea.md`, nine lines in
  `docs/background/README.md` and four in `TODO.md`); `v0.8.1` is not tagged yet (2026-11-14). Calc-U-59 has only
  the version bump on `dev-2.3.1`. TI-59-ROM is on GitHub, public, at
  `90cd4a7`.

## 3. The steps

### Part A — before the import (products at their pins)

#### M-1 The pin of Calc-U-1600 (FS 1)
- **Does:** when `v0.8.1` is tagged, its tree is compared with `947b850`.
  Any difference is recorded as a delta against the pin in
  `docs/Process.md` (phase 1) and `docs/inventory/calc-u-1600.md`; each
  spec that cites a changed file is read again. The new `TODO.md` item
  of `f0b3732` gets a backlog row (Briefing §1.7).
- **Completes:** first step 1.
- **Waits for:** the tag, at the tip `76baadf` (the user; question 1).
- **Green after:** unchanged.
- **Knowledge, ledger:** none.
- **Exit check:** the delta is written down, or "identical" is.

#### M-2 ReleaseIt for several products in one repository
- **Does:** in ReleaseIt's own repository (Briefing §2: an upstream
  dependency, extended there): a product directory as project root
  (S1, S2), prefixed tags `calc-u-59-v…` (S4), `--latest=false` (S11),
  per-product titles (S15), the trunk model of ADR 0029 instead of
  squash and `dev-` branches (S6-S8), one `appstore` worktree for the one
  product that has it (S13), version presets for the moved version files
  (S16). Its own tests (`cargo test`, fixtures for both products).
- **Completes:** ADR 0027-0029 on the tool side.
- **Waits for:** none. It comes before the import so that a product can
  still be released after the import (no more hotfixes from the old
  repositories then, Briefing §3).
- **Green after:** unchanged; ReleaseIt's own `checks`.
- **Knowledge, ledger:** ReleaseIt's README; its `examples/Calc-U-59.yaml`
  and `Calc-U-1600.yaml`.
- **Exit check:** a dry run (`release --build-only`) on a test repository
  with two product directories.

#### M-3 Checks on Calc-U-59 at `v2.3.0` (FS 5, FS 10, FS 11, FS 12)
- **Does:** runs the released app and `tools/headless/` at the tag, as
  `first-steps.md` describes: key readiness and idle on each model (5),
  an empty card side (10), the TI-58C's constant memory, (a) and (b)
  (11), the covered Mac window and App Nap (12). Throwaway scripts stay
  outside both repositories. The result of each goes under its step in
  `first-steps.md`; a result that turns out the other way goes to the
  user before a spec changes.
- **Completes:** first steps 5, 10, 11, 12.
- **Waits for:** none. **If the other way:** FS 5: idle a state of its
  own (LIB-1, C59-1, LIB-6 AC-36 rewritten), or the engine holds a key
  until the ROM sampled it (LIB-5); FS 10: back to the user (no fallback
  by time, C59-7 OQ-8); FS 11: row 59-15 goes if 2.3.0 already restores;
  FS 12: C59-5 AC-9 says a covered window is suspended.
- **Green after:** unchanged.
- **Knowledge, ledger:** rows 59-15 and 59-20 confirmed or corrected.
- **Exit check:** four result lines in `first-steps.md`.

#### M-4 Reference runs of the 32 Sharp examples (FS 13)
- **Does:** with the tools of `b4ea8c8` (`pc1500_cli`, `pc1600_cli`), in
  a copy of `examples/`: what each example leaves at the idle machine
  after its load (display as text, BASIC program area, placed machine
  code, paper, media), and the C-2 and C-4 notes from the logs. The
  references are data and go into the source repository as test
  fixtures (`products/calc-u-1600/tests/reference/`, created before the
  import; question 3), with the command that made them.
- **Completes:** first step 13.
- **Waits for:** M-1 (the pin the runs are made at). **If the other
  way:** C-2 bound hit: C1600-1 says how a PC-1600 in that mode reports
  prompt readiness; C-4 write: C1600-4 AC-16 says such runs use a copy.
- **Green after:** unchanged.
- **Knowledge, ledger:** none.
- **Exit check:** 32 reference files and two notes.

#### M-5 The PC-1500's key handling (FS 15, research)
- **Does:** reads the key scan loop of the PC-1500 and PC-1500A ROMs
  until a signal for key readiness is found or shown not to exist;
  records it in the PC-1500 research repository first (LIB-5, Technical
  Requirements). The C-7 part (the mode after a cold boot) is a golden
  run and is done in M-8.
- **Completes:** first step 15, research part.
- **Waits for:** none. **If found:** LIB-5 AC-31 holds for every key,
  AC-32 is struck; **if not:** AC-32 stays, row 1600-8 is corrected,
  `pause-on-start: cpu` is not offered on the PC-1500. Either way the
  spec is changed with the user (LIB-5 OQ-10).
- **Green after:** unchanged.
- **Knowledge, ledger:** the PC-1500 research notes and the `PC1500`
  router skill if it names the key loop; row 1600-8.
- **Exit check:** the research note, and the user's ruling on LIB-5.

### Part B — the infrastructure that makes green measurable

#### M-6 Import Calc-U-59 (development starts)
- **Does:** the tree of `v2.3.0` goes into `products/calc-u-59/`
  (ADR 0002): `Core/` → `core/`, `Shared/` → `app/`, `App/`, `Bridge/`,
  `Calc-U-59UITests/`, the plist and entitlements → `frontends/swift/`
  (into the scaffold's Xcode project, which still compiles the C++
  sources), `Qt6/` → `frontends/qt6/`, `Web/` → `frontends/web/`,
  `tests/`, `examples/`, `docs`-like material and the directories
  ADR 0002 does not name as in question 3. Only paths and build files
  change, no source line of the core or the app layer. `roms/` holds no
  ROM: the fetch list is what the imported code reads (the calculator
  dumps, `dumps/modules/TMC05xx.txt`), `cuecards.txt` stays until M-58.
  The workflow `calc-u-59.yml` replaces `build.yml` with the same jobs,
  each fetching the ROMs. `release.yaml` and `CHANGELOG.md` sit in the
  product directory; the version is `MARKETING_VERSION` 3.0.0 of the
  scaffold. This is the first commit that imports product code: no more
  hotfixes from the old repositories (Briefing §3). tinue/calc-u is made
  public with it (question 8; the user).
- **Completes:** ADR 0002 and 0004 for Calc-U-59; ADR 0034 for its build
  (fetch at build time).
- **Waits for:** M-2; the user's go for the import (question 2).
- **Green after:** default (§1.1) for Calc-U-59, first measurement of
  Frequent and Longer in the new tree.
- **Knowledge, ledger:** Calc-U-59 `CLAUDE.md` and the nine skills
  (paths), `reference/*.md` paths, `tools/*/README.md`; the product's
  memory directory (question 4). No ledger row.
- **Exit check:** a workflow run passes every job; the Swift app builds
  for macOS and the iOS simulator; `tools/agentic` and `tools/headless`
  run from the new paths.

#### M-7 Import Calc-U-1600
- **Does:** the tree of the pin (`v0.8.1` once tagged, M-1) goes into
  `products/calc-u-1600/`: `Core/` → `core/` (its tests → `tests/`),
  `Shared/` → `app/`, `Qt6/` → `frontends/qt6/`, `Web/` →
  `frontends/web/`, `vscode/`, `examples/`, `docs/`, and the rest as in
  question 3. The fetch list is 0.8.1's (`CE-502B.card.yaml` stays where
  0.8.1 reads it until M-27). SDE as today: the committed macOS and WASM
  libraries, the release download for Linux and Windows in CI. The
  workflow `calc-u-1600.yml` replaces `build.yml`, with the six signing
  secrets set on tinue/calc-u (`renew-certs apply` there; the user).
- **Completes:** ADR 0002 and 0004 for Calc-U-1600.
- **Waits for:** M-1, M-6.
- **Green after:** default for both products.
- **Knowledge, ledger:** Calc-U-1600 `CLAUDE.md`, `docs/developer/*.md`
  (paths), `headless/README.md`, `dev/README.md`, the `SessionStart` hook
  of `.claude/settings.json`; its memory directory (question 4).
- **Exit check:** a workflow run passes every job including the signed
  macOS DMG; `tools/dap_smoke.py` and the screenshot scenarios pass
  locally.

#### M-8 Golden runs of both cores (FS 2, FS 7, FS 8, FS 9, C-7 of FS 15)
- **Does:** a recorder that drives the old machine classes directly
  (C59-1 and C1600-1, Technical Requirements) records scripts of calls
  (keys at tick counts, attaches, advances) with the display output, the
  sound (Sharp), the printed lines and dots (PC-100C) and the memory at
  the end, covering at least what FS 2 lists; then the three TI checks
  by golden run (Reset with a partition, print instructions without a
  printer, `2nd Pgm 01` without a module) and the PC-1500/1500A mode
  after a cold boot. Replaying them is a test of the Frequent group.
- **Completes:** first steps 2, 7, 8, 9 and C-7.
- **Waits for:** M-6, M-7. **If the other way:** a run that cannot be
  recorded is cut and its criterion says so; FS 7: row 59-22 goes if
  2.3.0 keeps the partition; FS 8: C59-6 AC-7 holds trivially; FS 9: the
  slot's model of "no module" is corrected until the blink shows.
- **Green after:** default; Frequent now holds the golden replays.
- **Knowledge, ledger:** `tools/goldens/README.md`; row 59-22.
- **Exit check:** the golden files, and their replay passing in CI.

#### M-9 Reference runs of the 57 TI examples (FS 6)
- **Does:** the recorder FS 6 asks for: a driver on 2.3.0's own
  `SessionController` (code: app session) in its cooperative mode, with
  a clock the driver supplies; then for each example what its 2.3.0 file
  leaves behind (display, program steps, registers, partition, paper,
  the cards it wrote), before any file is upgraded and before core code
  is touched. LIB-8 AC-39: the printouts are turned into `paper` replies
  once, here (or when LIB-8 is built, from these files).
- **Completes:** first step 6.
- **Waits for:** M-6. **If an example's two runs differ** later: the
  upgrade rule is a finding; the spec is changed with the user.
- **Green after:** default.
- **Knowledge, ledger:** none.
- **Exit check:** 57 reference files.

#### M-10 The long-run pacing tests on the old code (FS 14)
- **Does:** per product two tests: one with a scripted wall clock
  (Frequent), one of ten minutes on the real clock (Checkpoint), run on
  the old pacer first; the measured old drift of Calc-U-1600 is the
  expected difference (LIB-17, decision log).
- **Completes:** first step 14.
- **Waits for:** M-6, M-7. **If the old code misses AC-1's bound:** the
  "before" test uses the measured bound and says so.
- **Green after:** default plus the ten-minute runs recorded.
- **Knowledge, ledger:** none.
- **Exit check:** four tests; two passing in Frequent.

#### M-11 One root CMake project (ADR 0003)
- **Does:** one `CMakeLists.txt` at the root with options per product
  and frontend (`CALCU_PRODUCT_59`, `CALCU_PRODUCT_1600`,
  `CALCU_FRONTEND_QT`, …); an empty `library` target; the cores and the
  app layers build without Qt, so Calc-U-1600's core tests no longer need
  Qt; the Qt6 frontends build from it; Calc-U-1600's `tools/build_*.sh`
  and the probes take their sources from CMake (backlog B1600-51). The
  version sources stay where ReleaseIt reads them (M-2).
- **Completes:** ADR 0003 for the C++ builds.
- **Waits for:** M-8 (the golden replays check the move).
- **Green after:** default; the `core-tests` jobs without Qt.
- **Knowledge, ledger:** both `CLAUDE.md`, Calc-U-1600
  `docs/developer/Building.md`, the `calc-u-59-qt` skill (build commands).
- **Exit check:** one configure builds both products' cores, tests and
  Qt6 frontends.

#### M-12 The Web frontends through CMake (ADR 0003)
- **Does:** both Web frontends build through the root project with the
  Emscripten toolchain; Calc-U-59's `Web/build.sh` (em++) is replaced.
  Each workflow keeps its Emscripten version (6.0.12 and 6.0.13); a
  raise would be an ADR (Briefing §8).
- **Completes:** ADR 0003 for the Web.
- **Waits for:** M-11.
- **Green after:** default.
- **Knowledge, ledger:** `calc-u-59-web` skill, `reference/WebClient.md`,
  Calc-U-1600 `docs/developer/Web-Client.md`.
- **Exit check:** both `web` jobs build through CMake and Playwright
  passes.

#### M-13 One test runner and the three test groups (ADR 0038, 0031)
- **Does:** `tests/TestHarness.hpp` moves to `library/`; Calc-U-1600's
  ~788 test functions are registered in it (mechanical, ADR 0038); CTest
  labels for Frequent, Longer and Checkpoint; every existing test of
  both products is assigned to a group (Briefing §7), with the budgets
  measured.
- **Completes:** ADR 0038, ADR 0031 (the groups and their places).
- **Waits for:** M-11.
- **Green after:** default; `ctest -L frequent` within budget per
  product.
- **Knowledge, ledger:** Calc-U-1600 `docs/developer/Building.md`, the
  test sections of both `CLAUDE.md`.
- **Exit check:** one runner; a table of every test and its group.

#### M-14 C++20 for Calc-U-59 (ADR 0005)
- **Does:** the Calc-U-59 core, app layer, Qt6 frontend and bridge
  (gnu++20) compile as C++20.
- **Completes:** ADR 0005.
- **Waits for:** M-8, M-11.
- **Green after:** default; the golden replays unchanged.
- **Knowledge, ledger:** Calc-U-59 `CLAUDE.md`, the `calc-u-59-core`
  skill if they name the standard.
- **Exit check:** CI and the local Swift build at C++20.

#### M-15 The Swift frontend links an XCFramework (ADR 0013)
- **Does:** the root project builds the library, the Calc-U-59 core and
  app layer as one XCFramework for macOS, iOS devices and the iOS
  simulator; the Xcode project links it and builds only Swift and the
  bridge; Xcode stops compiling `core/` and `app/`.
- **Completes:** ADR 0013. **Tests from:** C59-5 AC-1 to AC-3.
- **Waits for:** M-11, M-14.
- **Green after:** default; the Xcode project check of C59-5's Technical
  Requirements.
- **Knowledge, ledger:** `calc-u-59-swift` skill, Calc-U-59 `CLAUDE.md`
  (build), `calc-u-59-uitesting` skill.
- **Exit check:** a change in the app layer reaches the app built from
  Xcode (C59-5 AC-2).

#### M-16 The two Swift spikes (FS 3, FS 4)
- **Does:** throwaway builds: the app session on a thread of its own at
  about 60 Hz with the bridge as a queue, with a TI-59 and with a PC-1600
  on iOS (FS 3); a Debug build in a simulator opening its session on a
  localhost port, `calcu` attaching from the Mac, the Xcode plugin,
  several simulators, and how an XCUITest reaches the attached session
  (FS 4). `calcu` comes in M-28: a throwaway client on the Mac and a
  stand-in for the attach end are part of the spike.
- **Completes:** first steps 3 and 4.
- **Waits for:** M-15 (the XCFramework), M-11 (a PC-1600 core for iOS).
  **If FS 3 does not hold:** ADR 0015 is reopened with the user (the
  alternative is the main thread). **If FS 4 does not hold:** C59-8
  AC-21 keeps the three aids, ADR 0033 is amended with the user, and
  LIB-14 AC-44 and LIB-16 AC-12 are re-planned.
- **Green after:** unchanged (throwaway).
- **Knowledge, ledger:** the results are written into C59-5 (the default
  port, OQ-1), C59-8 (AC-17, AC-21, OQ-5), with the user.
- **Exit check:** two result notes in `first-steps.md`.

#### M-17 Agent knowledge outside the products (Briefing §1.11)
- **Does:** the research skills and notes take the glossary terms: the
  `PC1500` and `pc1500-build` router skills, `SC61860`, the
  `Sharp1500-1600-Ref` skill (Briefing §1.11 names it), the research memory
  directories. The memory directories of paths that no longer exist
  (`claude/ti59-emul`, `sharp/Calc-U-1600-Binary`): what is still valid
  moves into the live memory, then they are deleted. Code renames do not
  happen here; the product guidance follows its code in the steps below.
- **Completes:** the part of Briefing §1.11 that has no code.
- **Waits for:** none; it may run beside Part A (§1).
- **Green after:** unchanged.
- **Knowledge, ledger:** the elements named.
- **Exit check:** a grep for the old terms of `glossary-mapping.md` in
  those elements finds only quoted history.

### Part C1 — the machine interface, the machine rows and the sessions

#### M-18 LIB-1 Library
- **Does:** the machine interface, descriptors and the catalogue,
  emulated time, display kinds, key IDs, places, capabilities, product
  kinds, in `library/`, tested with machines built for the tests.
- **Completes:** LIB-1 L. **Tests from:** LIB-1 AC-1–25, AC-42, AC-43;
  EC-1–8.
- **Waits for:** M-3 (FS 5 may add idle as a state, AC-11, AC-42).
- **Green after:** default; AC-24 (the library builds and tests without
  product code) is a CI check.
- **Knowledge, ledger:** none yet.
- **Exit check:** the library tests pass.

#### M-19 C59-1 TI machines on the machine interface
- **Does:** TI-58/58C/59 behind the interface; the core types used
  outside the core go (C59-1, Technical Requirements); the four mutexes
  have no successor; the app layer's calls on the core stay behind
  adapters, each named with the row that takes it (C59-6, C59-7, C59-2,
  C59-3, LIB-10, C59-4). `tools/headless/` moves onto the interface in
  the same step.
- **Completes:** C59-1 (LIB-1 59 waits for C59-6). **Tests from:** C59-1
  AC-1–19, EC-1–4; LIB-1 AC-26–33.
- **Waits for:** M-18, M-8 (FS 2, 7), M-3 (FS 5, FS 11).
- **Green after:** default; the golden replays now through the
  interface.
- **Knowledge, ledger:** `calc-u-59-core` skill,
  `reference/CoreArchitecture.md`; rows 59-13 (with LIB-5), 59-22.
- **Exit check:** the TI golden runs pass through the machine interface.

#### M-20 LIB-9 Library
- **Does:** one INI file per product, its place and the save folder per
  platform, keys and defaults.
- **Completes:** LIB-9 L. **Tests from:** LIB-9 AC-1–18, AC-33; EC-1–10.
- **Waits for:** none.
- **Green after:** default.
- **Knowledge, ledger:** none.
- **Exit check:** library tests on every platform's paths.

#### M-21 LIB-17 Library
- **Does:** one pacer with a pacing profile per model, regular and full
  speed, lag dropped.
- **Completes:** LIB-17 L. **Tests from:** LIB-17 AC-1–14; EC-1–6.
- **Waits for:** M-18.
- **Green after:** default.
- **Knowledge, ledger:** none.
- **Exit check:** library tests with a scripted clock.

#### M-22 LIB-3 Library
- **Does:** app and machine sessions, sliced operations with live
  updates, busy and what is refused, audio policy, background pause,
  topics, commands, product topics and commands, host services; a test
  frontend.
- **Completes:** LIB-3 L. **Tests from:** LIB-3 AC-1–27, AC-39; EC-1–6.
- **Waits for:** M-18, M-21.
- **Green after:** default.
- **Knowledge, ledger:** none yet.
- **Exit check:** library tests with the test frontend.

#### M-23 LIB-2 Library
- **Does:** attach and detach in places, refusal with a reason, the
  power cycle after an attach, peripherals keyed by pin-out, chains;
  controls, serial port with the host link, real-time clock.
- **Completes:** LIB-2 L. **Tests from:** LIB-2 AC-1–23, AC-35; EC-1–7.
- **Waits for:** M-18, M-20, M-22.
- **Green after:** default; the pseudo-terminal tests on macOS and Linux
  only (AC-18, EC-5).
- **Knowledge, ledger:** none yet.
- **Exit check:** library tests.

#### M-24 LIB-12 Library and Calc-U-59 column
- **Does:** one ROM lookup; Calc-U-59 gets every ROM image through it in
  every frontend, the harness and `tools/headless/`; the ROM bytes of the
  14 Solid State ROM modules come from their `.rom.yaml` files (AC-14);
  the fetch list becomes the scaffold's (24 files); the module list and
  cue cards are still read from `cuecards.txt` (§2.3); the product's own
  ROM loading and error handling go. AC-9's md5 table is corrected
  (§2.3).
- **Completes:** LIB-12 L, LIB-12 59. **Tests from:** LIB-12 AC-1–10,
  AC-14 (the TI modules); EC-2, EC-3; "a build with one ROM file taken away" (Technical
  Requirements).
- **Waits for:** M-19, M-22.
- **Green after:** default.
- **Knowledge, ledger:** Calc-U-59 `roms/README.md`, `calc-u-59-core`
  and `calc-u-59-shared` skills (ROM loading).
- **Exit check:** a build without one ROM file fails with the file named.

#### M-25 C59-6 PC-100C and module slot as functional connectors
- **Does:** the PC-100C and the 14 modules as peripherals in `printer`
  and `slot-1`, out of the TMC0501 class; a machine without a module; the
  power cycle after an attach; the module names from the module files
  (AC-8). The documents say the printer timings were
  measured on a PC-100A (in `ti59-research` and Calc-U-59's documents;
  `first-steps.md`, "Not first steps").
- **Completes:** C59-6, LIB-1 59, LIB-2 59. **Tests from:** C59-6
  AC-1–11, EC-1–3; LIB-2 AC-24–27.
- **Waits for:** M-8 (FS 8, FS 9), M-23, M-24.
- **Green after:** default; the printer and module golden runs.
- **Knowledge, ledger:** `calc-u-59-core` skill,
  `reference/CoreArchitecture.md` (printer, modules); rows 59-11, 59-25.
- **Exit check:** the PC-100C and module golden runs pass through the
  places.

#### M-26 C1600-1 Sharp machines on the machine interface
- **Does:** PC-1500/1500A/1600 behind one interface, no model branches
  above the machine, the peripherals in their places on the 60-pin chain;
  the ~60 places and 27 dispatches of `MachineSession.cpp` and the other
  callers behind adapters; `pc1500_cli`, `pc1600_cli` and the probes move
  onto the interface in the same step; a check in the build that no file
  outside the core includes a machine class.
- **Completes:** C1600-1, LIB-1 1600, LIB-2 1600 (Done in M-47, §2.1).
  **Tests from:** C1600-1
  AC-1–21 (AC-21 per FS 15), EC-1–3; LIB-1 AC-34–41; LIB-2 AC-28–34.
- **Waits for:** M-5, M-8, M-23; M-4 (prompt readiness per mode).
- **Green after:** default; Sharp golden runs through the interface.
- **Knowledge, ledger:** Calc-U-1600
  `docs/developer/PC1600-Core-Limitations.md`,
  `Shared-Architecture.md`, `CLAUDE.md`; rows 1600-4, 1600-16.
- **Exit check:** the Sharp golden runs pass through the machine
  interface; the include check is in CI.

#### M-27 LIB-12 Calc-U-1600 column
- **Does:** Calc-U-1600 gets every ROM image through the lookup; the
  CE-502B definition is fetched from PC-1500-ROM, where it already is
  (§1.2; AC-14), and leaves `Qt6/resources/cards/`; the CLI's
  own ROM lookup goes (LIB-11 AC-20); New-for-Old substitution goes
  (AC-13).
- **Completes:** LIB-12 1600. **Tests from:** LIB-12 AC-11–13, AC-14
  (CE-502B); EC-1.
- **Waits for:** M-24, M-26.
- **Green after:** default.
- **Knowledge, ledger:** `docs/developer/Building.md` (ROMs).
- **Exit check:** a build without one ROM file fails with the file named.

#### M-28 LIB-11 Library
- **Does:** `calcu` with one-shot commands and `serve`, the line protocol
  with `--json` (fixed before C59-5 starts, OQ-3), the first command set,
  the place for product commands, the table of today's tools' functions;
  the Python helper for `calcu serve` (`contracts/harness.md` Q-2).
  Built on macOS, Linux and Windows (AC-15).
- **Completes:** LIB-11 L. **Tests from:** LIB-11 AC-1–13, AC-21–22;
  EC-1–7.
- **Waits for:** M-22, M-26 (a machine of each product behind the
  interface to drive).
- **Green after:** default; the harness built in every workflow.
- **Knowledge, ledger:** a harness section for agents in both products'
  guidance; today's tools stay.
- **Exit check:** `calcu` drives a TI-59 and a PC-1600 headless.

#### M-29 LIB-15 Library
- **Does:** one embedding interface, the Web Worker host, the persistent
  IndexedDB save folder, the installable offline app.
- **Completes:** LIB-15 L. **Tests from:** LIB-15 AC-1–18; EC-1–5.
- **Waits for:** M-20, M-22, M-12.
- **Green after:** default.
- **Knowledge, ledger:** none yet.
- **Exit check:** library tests in Chromium and WebKit.

#### M-30 LIB-9 Calc-U-1600 column
- **Does:** Calc-U-1600's settings in its INI file; the platform stores
  no longer used.
- **Completes:** LIB-9 1600. **Tests from:** LIB-9 AC-26–32.
- **Waits for:** M-20, M-26.
- **Green after:** default.
- **Knowledge, ledger:** row 1600-2.
- **Exit check:** a first start writes a fresh INI file.

#### M-31 Calc-U-1600 on the library's sessions (LIB-3, LIB-17, LIB-15)
- **Does:** Calc-U-1600's `AppSession` and `MachineSession` are replaced
  by the library's (Qt6 and the harness first, then the Web frontend on
  LIB-15); its own pacer goes; `runNow` goes; today's change flags and
  callbacks become topics (AC-29, AC-38).
- **Completes:** LIB-3 1600, LIB-17 1600, LIB-15 1600. **Tests from:**
  LIB-3 AC-34–38; LIB-17 AC-20–22; LIB-15 AC-24–28.
- **Waits for:** M-10 (FS 14), M-28, M-29, M-30.
- **Green after:** default; the long-run pacing test gives the old
  result (AC-21).
- **Knowledge, ledger:** `docs/developer/Shared-Architecture.md`,
  `Cloud-Sessions.md`, `Web-Client.md`; renames `AppSession` → app
  session, `MachineSession` → machine session (`glossary-mapping.md`);
  rows 1600-9 (keys dropped while an operation runs).
- **Exit check:** Qt6 and the Web frontend run every example as before.

#### M-32 LIB-9 Calc-U-59 column
- **Does:** Calc-U-59's settings in its INI file in every frontend
  (AC-34: the one exception kept).
- **Completes:** LIB-9 59 (AC-23, the `calcu59/` keys, with M-34).
  **Tests from:** LIB-9 AC-19–25, AC-34–35.
- **Waits for:** M-20, M-19.
- **Green after:** default.
- **Knowledge, ledger:** row 59-2; `Shared/SettingsKeys.hpp` rule in
  `CLAUDE.md`.
- **Exit check:** a first start writes a fresh INI file.

#### M-33 C59-5 and Calc-U-59 on the library's sessions (Swift, Qt6)
- **Does:** the Swift frontend hosts the library's app session on a
  background thread with the bridge as a message boundary; harness attach
  on a localhost TCP port in Debug builds; the Qt6 frontend on the same
  session; `EmulationRunner` and the threaded runner go; the pacer goes
  (LIB-17); `TIPlatform`'s duties as LIB-3 AC-30 says.
  `SessionController` stays, in its cooperative mode only, for the Web
  frontend and its tests (`Web/bindings.cpp`, `tests/FakePlatform.hpp`,
  `shared_controller_tests.cpp`, `shared_cooperative_tests.cpp`) until
  M-34.
- **Completes:** C59-5, LIB-17 59. **Tests from:** C59-5 AC-4–14,
  EC-1–4; LIB-3 AC-28–30, AC-32–33 (AC-31 with M-34); LIB-17 AC-15–19.
- **Waits for:** M-16 (FS 3, FS 4), M-3 (FS 12), M-10 (FS 14), M-15,
  M-28, M-32.
- **Green after:** default; attach used by one XCUITest of
  `UIRegressionTests`.
- **Knowledge, ledger:** `calc-u-59-swift`, `calc-u-59-shared`,
  `calc-u-59-qt` skills, `reference/SharedArchitecture.md`,
  `DebugAPI.md`, `AppArchitecture.md`; renames `SessionController`,
  `EmulatorSession`, `Platform`.
- **Exit check:** `calcu` attaches to the app in a simulator; the
  display runs smoothly on iOS.

#### M-34 LIB-15 Calc-U-59 column
- **Does:** the Calc-U-59 Web frontend on the library's Web shell and
  session; `localStorage` `calcu59/` moves to IndexedDB (AC-20); the
  `module` prop goes (AC-19); no message of today's own protocol remains;
  `SessionController` goes, and its cooperative tests are ported to the
  library's session or replaced by its tests.
- **Completes:** LIB-15 59 (AC-20's constant memory and cards with M-57
  and M-58), the session part of LIB-3 59. **Tests from:** LIB-15
  AC-19–23; LIB-3 AC-31; LIB-9 AC-23.
- **Waits for:** M-29, M-33.
- **Green after:** default.
- **Knowledge, ledger:** `calc-u-59-web` skill, `reference/WebClient.md`;
  row 59-8 (the one-writer rule).
- **Exit check:** Playwright passes; a second tab says it cannot save.

### Part C2 — the other Library columns

#### M-35 LIB-18 Library
- **Does:** model and variant switch, attach and detach, Reset, Cold
  boot as commands; the start of a product, the startup model, the
  built-in default machine; tested with a stand-in for the loader.
- **Completes:** LIB-18 L. **Tests from:** LIB-18 AC-1–16; EC-1–7.
- **Waits for:** M-20, M-23, M-24, M-28.
- **Green after:** default.
- **Knowledge, ledger:** none yet.
- **Exit check:** library tests.

#### M-36 LIB-5 Typing, all columns
- **Does:** one typing engine for host keys, on-screen keys and text,
  holds in emulated time, readiness through typing support, two keys,
  Paste Text, typing profiles and keyboard maps per model, the `key` and
  `type` commands (the steps with LIB-6); the products' key sequencers
  and queues go; the PC-1500 cadence goes after the research (M-5).
- **Completes:** LIB-5 L, 59, 1600. **Tests from:** LIB-5 AC-1–41;
  EC-1–6.
- **Waits for:** M-3 (FS 5: AC-20, AC-25), M-4 (C-2: AC-14, AC-17,
  AC-28), M-5 (AC-31, AC-32), M-31, M-34.
- **Green after:** default.
- **Knowledge, ledger:** `prompt/ti59-agent-prompt.md` if it names
  keys; Calc-U-1600 memory files on typing; rows 59-13, 59-23, 1600-8,
  1600-9.
- **Exit check:** a host key held on a PC-1500 auto-repeats as the ROM
  does; `Adv` feeds the same lines at every speed.

#### M-37 LIB-4 Library
- **Does:** the control state from the descriptors and the live machine.
- **Completes:** LIB-4 L. **Tests from:** LIB-4 AC-1–9; EC-1, EC-3–5.
- **Waits for:** M-22, M-23.
- **Green after:** default.
- **Knowledge, ledger:** none.
- **Exit check:** library tests.

#### M-38 LIB-6 Library
- **Does:** the loader of preset format 2 (all or nothing), the machine
  from a cold boot, the step engine and its steps over typing, places,
  controls and clock, the place for product sections, the preset list
  from the bundle file, the check of shipped and hosted presets as test
  suite checks (AC-51).
- **Completes:** LIB-6 L. **Tests from:** LIB-6 AC-1–34, AC-52–53,
  AC-55–65; EC-1–10.
- **Waits for:** M-3 (FS 5: AC-56, AC-59), M-35, M-36.
- **Green after:** default.
- **Knowledge, ledger:** none yet (the products still read their old
  formats).
- **Exit check:** library tests with sections built for the tests.

#### M-39 LIB-8 Library
- **Does:** one paper output kind (marks and a text layer), the paper
  panel, Copy and Cut, `cut-paper`; a reference picture per printer.
- **Completes:** LIB-8 L. **Tests from:** LIB-8 AC-1–9, AC-12–23, AC-39
  (from the references of M-4 and M-9); EC-1–8.
- **Waits for:** M-38, M-4, M-9.
- **Green after:** default.
- **Knowledge, ledger:** none yet.
- **Exit check:** library tests.

#### M-40 LIB-10 Library
- **Does:** debug targets, run control, breakpoints and watches, the
  trace session, the run-control buttons, the steps and commands.
- **Completes:** LIB-10 L. **Tests from:** LIB-10 AC-1–26, AC-45–47;
  EC-1–9.
- **Waits for:** M-3 (C-1 of FS 5: AC-13), M-38.
- **Green after:** default.
- **Knowledge, ledger:** none yet.
- **Exit check:** library tests.

#### M-41 LIB-7 Library
- **Does:** templates and instances, Name & Save, autosave, names and
  collisions, outdated files, preset-named media, in-file magnetic cards,
  the media steps and commands, the module and media pickers.
- **Completes:** LIB-7 L. **Tests from:** LIB-7 AC-1–31, AC-46; EC-1–9.
- **Waits for:** M-37, M-38.
- **Green after:** default.
- **Knowledge, ledger:** none yet.
- **Exit check:** library tests.

#### M-42 LIB-13 Library
- **Does:** one "open file" command, classification, the place for
  product file kinds.
- **Completes:** LIB-13 L. **Tests from:** LIB-13 AC-1–10; EC-1–7.
- **Waits for:** M-38, M-41.
- **Green after:** default.
- **Knowledge, ledger:** none.
- **Exit check:** library tests.

#### M-43 LIB-14 Library
- **Does:** the family menu and control-bar structure, the settings
  dialog, "Loading…", notices, About and Help, the shells of the Qt6 and
  React component sets, the hit test by one rule (AC-24); Qt6 harness
  attach for development builds (AC-44, built here, used by
  Calc-U-1600's scenarios in M-49).
- **Completes:** LIB-14 L. **Tests from:** LIB-14 AC-1–25, AC-42–43;
  EC-1–8.
- **Waits for:** M-16 (FS 4, for AC-44), M-35, M-37.
- **Green after:** default.
- **Knowledge, ledger:** none yet.
- **Exit check:** components tested without a window.

### Part D — Calc-U-1600 (first: question 5)

#### M-44 C1600-2 Sharp program files
- **Does:** the loaders on the loader memory access product kind:
  telling a program file's kind, BASIC, typing a program in, machine
  code, tape fast-load; one classifier; the two menu items and their
  dialogs in the LIB-14 shell.
- **Completes:** C1600-2. **Tests from:** C1600-2 AC-1–25, EC-1–6.
- **Waits for:** M-4 (C-2: AC-13), M-26, M-36, M-43.
- **Green after:** default.
- **Knowledge, ledger:** `docs/background/plans/Loader-*` stay history;
  `docs/developer/README.md`; row 1600-15 (loader parts).
- **Exit check:** every loader checked through the harness.

#### M-45 LIB-8 and LIB-10, Calc-U-1600 columns
- **Does:** the CE-150 and CE-1600P on the paper output kind, the panel
  always shown; the run control in Qt6 and Web with and without a
  debugger; the size limit for every trace. Criteria that need the
  upgraded presets pass in M-46.
- **Completes:** LIB-8 1600, LIB-10 1600 (Done in M-46). **Tests from:** LIB-8
  AC-32–38; LIB-10 AC-36–44 (AC-38 per FS 15).
- **Waits for:** M-5, M-31, M-39, M-40.
- **Green after:** default.
- **Knowledge, ledger:** `docs/developer/Debugger-Internals.md`; rows
  1600-12, 1600-14.
- **Exit check:** the plotter's paper survives a model switch.

#### M-46 Calc-U-1600 on preset format 2 (LIB-6, C1600-4, LIB-18)
- **Does:** every preset of Calc-U-1600 is read by the library's loader
  only; the product sections and keys of C1600-4; the 32 examples, the
  test, `dev/` and docs presets, the VS Code templates and tool-written
  presets upgraded (those of the last row of C1600-4 AC-15's table in
  M-47); `pc1500_cli` and `pc1600_cli` load presets through the library's
  loader; `tools/check_presets.sh` becomes the suite check of LIB-6
  AC-51; the
  bundle file; the start of the product, the model switch, the resets on
  LIB-18; version 1 refused with a hint; the readers of version 1 go.
- **Completes:** LIB-6 1600, C1600-4 (Done in M-47: AC-15's disk and
  waiting presets), LIB-18 1600, LIB-10 1600. **Tests from:** LIB-6
  AC-44–51, AC-54; C1600-4 AC-1–20 (AC-4 per C-7), EC-1–6; LIB-18
  AC-25–33.
- **Waits for:** M-4 (FS 13), M-8 (C-7), M-44, M-45.
- **Green after:** default; the reference runs of M-4 compared at the
  idle machine (AC-16).
- **Knowledge, ledger:** Calc-U-1600 memory files on presets
  (`project_preset_parameterless_wait` and the like), `screenshots/README.md`,
  the VS Code extension's templates; rows 1600-1 (presets), 1600-3
  (syntax), 1600-9 (Reset and switch stop what runs), 1600-11, 1600-21
  (bundle).
- **Exit check:** the 32 examples match their references.

#### M-47 LIB-7 Calc-U-1600 column
- **Does:** Calc-U-1600's media under the media model; the readers and
  writers of `.card.yaml`, `.floppy.yaml` (version 2) and the WAV behind
  one seam (AC-46); tape definitions (`.tape.yaml`); `slot-1-file:` and
  `floppy-file:` go; the presets that waited are upgraded.
- **Completes:** LIB-7 1600. **Tests from:** LIB-7 AC-37–45.
- **Waits for:** M-41, M-46.
- **Green after:** default.
- **Knowledge, ledger:** `docs/developer/Floppy-Image-Format.md`;
  memory files on media; rows 1600-1 (disks), 1600-3 (media steps),
  1600-5, 1600-6, 1600-13, 1600-17, 1600-18, 1600-19.
- **Exit check:** DiskWorks runs from its upgraded preset.

#### M-48 LIB-13 Calc-U-1600 column
- **Does:** Calc-U-1600's file kinds on the loaders of C1600-2; drop,
  Open With and pickers through the one command; `.rom` no longer a
  machine-code name.
- **Completes:** LIB-13 1600. **Tests from:** LIB-13 AC-17–22.
- **Waits for:** M-42, M-44, M-47.
- **Green after:** default.
- **Knowledge, ledger:** `Drag-And-Drop-Plan.md` stays history; row
  1600-15.
- **Exit check:** a dropped disk file is used where it lies.

#### M-49 LIB-4 and LIB-14, Calc-U-1600 columns
- **Does:** the Qt6 and Web frontends on the family structure and the
  control state; the screenshot scenarios rewritten as an attach client
  (AC-44).
- **Completes:** LIB-4 1600, LIB-14 1600. **Tests from:** LIB-4
  AC-16–20; LIB-14 AC-34–41, AC-44.
- **Waits for:** M-43, M-46, M-47.
- **Green after:** default; the screenshot scenarios through attach.
- **Knowledge, ledger:** `docs/developer/screenshots/README.md`; visible
  renames of `glossary-mapping.md`; row 1600-10.
- **Exit check:** the scenarios pass through attach.

#### M-50 C1600-3 DAP in the app layer
- **Does:** the DAP server out of the Qt frontend into the app layer on
  the library's debug target and run control, without Qt types; the
  `dap_smoke.py` scenarios become suite tests.
- **Completes:** C1600-3. **Tests from:** C1600-3 AC-1–25, EC-1–7.
- **Waits for:** M-45, M-46.
- **Green after:** default; DAP tests in Frequent or Longer per budget.
- **Knowledge, ledger:** `DAP-Debugger-Plan.md` stays history;
  `docs/developer/Debugger-Internals.md`; row 1600-20.
- **Exit check:** VS Code debugs a program as before.

#### M-51 C1600-5 and the LIB-11 Calc-U-1600 column
- **Does:** today's CLI functions as `pc1600` commands, each with a test;
  `pc1500_cli`, `pc1600_cli`, the three probes and `dev/tape-matrix/run.py`
  go (LIB-11 OQ-5). Then the adapter check of §1 over Calc-U-1600 and
  the library.
- **Completes:** C1600-5, LIB-11 1600. **Tests from:** C1600-5 AC-1–9,
  AC-11, EC-2–6; LIB-11 AC-18–20.
- **Waits for:** M-47, M-50.
- **Green after:** default; no old CLI and no adapter of Calc-U-1600
  left.
- **Knowledge, ledger:** `headless/README.md`, `Building.md`, the
  Calc-U-1600 memory files on the CLI, `Sharp1500-1600-Ref` memory
  `calc-u-1600-headless-cli`.
- **Exit check:** every function of the table of LIB-11 AC-18 through
  `calcu`.

#### M-52 LIB-16 Library and Calc-U-1600 column
- **Does:** the help tool (`tools/help build|check`), the shared
  chapters, Calc-U-1600's chapters converted from its `docs/`, the help
  asset `Calc-U-1600-help.zip` in the release build.
- **Completes:** LIB-16 L, LIB-16 1600 (Done in M-53, with the asset in
  the release). **Tests from:** LIB-16 AC-1–8, AC-11–15; EC-1–3.
- **Waits for:** M-46 to M-51 (the chapters follow those specs), M-49
  (AC-12 through attach).
- **Green after:** default; the tool's check in Frequent.
- **Knowledge, ledger:** `docs/developer/README.md` (where help lives).
- **Exit check:** the asset builds; every settings key is described.

#### M-53 Release Calc-U-1600 (R-1600)
- **Does:** the first release of Calc-U-1600 from tinue/calc-u
  (`calc-u-1600-v0.9.0`, question 7), with the Checkpoint group;
  release notes from the Calc-U-1600 rows of the ledger; no
  `Calc-U-1600-examples.zip`.
- **Completes:** Briefing §7 for Calc-U-1600, release side.
- **Waits for:** M-52; tinue/calc-u public since M-6 (question 8).
- **Green after:** Checkpoint for Calc-U-1600.
- **Knowledge, ledger:** release checklist; every 1600 row visible.
- **Exit check:** the GitHub release with its seven assets (help zip
  instead of the examples zip).

#### M-54 LIB-19 Library and the Calc-U-1600 column (calc-u-site)
- **Does:** in calc-u-site, by its own rules: rendering the pinned help
  asset, `/1600/` with the published Web frontend, its CI job (the /1600/
  design first, by the site's skills, OQ-6).
- **Completes:** LIB-19 L, LIB-19 1600. **Tests from:** LIB-19 AC-1–5,
  AC-15, AC-11–14; EC-1–4.
- **Waits for:** M-53.
- **Green after:** the site's tests.
- **Knowledge, ledger:** the site's `CLAUDE.md` and skills (a 1600 site
  skill does not exist yet).
- **Exit check:** calc-u.org/1600 shows the help and the Web frontend.

#### M-55 Archive Calc-U-1600 (ADR 0030)
- **Does:** tinue/Calc-U-1600 gets a README pointing to tinue/calc-u and
  is archived.
- **Completes:** ADR 0030 for Calc-U-1600.
- **Waits for:** M-53.
- **Green after:** unchanged.
- **Knowledge, ledger:** the old memory directory of
  `sharp/Calc-U-1600` per question 4.
- **Exit check:** the repository is read-only on GitHub.

### Part E — Calc-U-59

#### M-56 LIB-8 and LIB-10, Calc-U-59 columns (Qt6, Web)
- **Does:** the PC-100C's paper on the paper output kind, the paper
  panel empty while the printer is detached; every pause, step,
  breakpoint and trace through the run control; one trace at a time;
  the key and card records leave the trace (AC-33). Swift parts in M-61.
- **Completes:** LIB-8 59 (its Swift part is C59-8, LIB-8, Dependencies);
  LIB-10 59, or with M-61 if AC-34's Swift part needs C59-8's panels.
  **Tests from:** LIB-8 AC-24–31; LIB-10 AC-28–35.
- **Waits for:** M-25, M-34, M-39, M-40.
- **Green after:** default.
- **Knowledge, ledger:** `reference/DebugAPI.md`, `calc-u-59-uitesting`
  skill (traces); rows 59-18, 59-19.
- **Exit check:** a trace of a preset and the button's are one session.

#### M-57 Calc-U-59 on preset format 2, and the TI-58C's constant memory (LIB-6, C59-2, LIB-18, C59-3)
- **Does:** every preset of Calc-U-59 read by the library's loader only;
  the product sections (partition, program steps, registers, cue card);
  the 57 examples upgraded (the four card examples in M-58), their
  comments and READMEs, the calendar scripts (C59-2 AC-18), the three
  startup presets; the bundle file in all three frontends; the start of
  the product with a TI-59, the Master Library module and no printer;
  the readers of the 2.3.0 format and the `STATE` records go;
  `tools/agentic/` loads presets through `calcu` and its own parser
  (`ti59file.py`) goes. With it C59-3, which rests on the startup and the
  presets of this step (§2.1): `ti58c.memory.yaml` (constant memory
  format 1), in place before the ROM starts, never written by a machine a
  preset built; the Web frontend keeps it in the save folder;
  `ti58c.mem` ignored.
- **Completes:** LIB-6 59, C59-2 (Done in M-65: AC-10's card examples,
  AC-12's site copies), LIB-18 59, C59-3. **Tests from:** LIB-6
  AC-35–43; C59-2 AC-1–18, EC-1–6; LIB-18 AC-17–24; C59-3 AC-1–14,
  EC-1–8.
- **Waits for:** M-3 (FS 5, FS 11), M-8 (FS 7, FS 9), M-9 (FS 6), M-25,
  M-34, M-35, M-56.
- **Green after:** default; the 57 reference runs compared.
- **Knowledge, ledger:** `calc-u-59-statefiles` skill,
  `reference/StateFileFormat.md`, `prompt/ti59-agent-prompt.md`, the
  Calc-U-59 memory file `state_vs_preset_terminology` (renames
  `StateFile` → preset); rows 59-1 (presets, constant memory), 59-3,
  59-4, 59-5, 59-10, 59-12, 59-15, 59-16, 59-17 (C59-3 AC-4, AC-11),
  59-26.
- **Exit check:** the examples match their references; a 2.3.0 file is
  refused with the hint; a TI-58C quit and started again shows its
  partition and display format.

#### M-58 LIB-7 Calc-U-59 column and C59-7 Magnetic cards
- **Does:** the card reader as a pass-through media place, the card
  format 3 (`.magnetcard.yaml`, sides 0 and 1, the name inside), cards in
  the save folder also on the web, `card` and `arm` and the corner taps,
  the cue card of the running module program, and the cue cards from the
  module files (AC-22, AC-23), so `cuecards.txt` leaves the source
  repository; `.u59` ignored; the four card examples upgraded.
- **Completes:** LIB-7 59, C59-7. **Tests from:** LIB-7 AC-32–36, AC-47;
  C59-7 AC-1–30, EC-1–7.
- **Waits for:** M-3 (FS 10), M-9 (AC-30), M-41, M-57.
- (C59-2 AC-10's card examples pass here.)
- **Green after:** default.
- **Knowledge, ledger:** `calc-u-59-statefiles` skill (cards),
  `reference/StateFileFormat.md`, memory files on cards
  (`card_timing_audio` and the like); renames `CardDocument`, `CardFile`;
  rows 59-1 (cards), 59-6, 59-8, 59-17, 59-20 (cards), 59-24, 59-27.
- **Exit check:** a card written in a TI-59 on the web is there after a
  reload.

#### M-59 LIB-13 Calc-U-59 column and C59-4 Debug panel
- **Does:** Calc-U-59's file kinds through the one command, the
  keystroke program file opened into the running machine; the debug panel
  on the library's run control, its data as product topics; the ASM
  overlay as a product kind and file kind, ASM 2.0, the five overlays and
  the hosted copies upgraded.
- **Completes:** LIB-13 59, C59-4. **Tests from:** LIB-13 AC-11–16,
  AC-23; C59-4 AC-1–26, EC-1–5.
- **Waits for:** M-42, M-56, M-58.
- **Green after:** default.
- **Knowledge, ledger:** `reference/DebugAPI.md`, `FrontendParity.md`,
  the `-qt` and `-web` skills (panel); rows 59-1 (ASM), 59-20 (files),
  59-21, 59-28.
- **Exit check:** a dropped `.asm` opens; FREEZE works inside an overlay.

#### M-60 LIB-4 and LIB-14, Calc-U-59 columns (Qt6, Web)
- **Does:** Calc-U-59's Qt6 and Web frontends on the family structure and
  the control state; the module picker in the control bar; the More
  button where there is no menu bar.
- **Completes:** LIB-14 59 (LIB-4 59 with M-61). **Tests from:** LIB-14
  AC-26–33; LIB-4 AC-10–15 (Qt6, Web parts).
- **Waits for:** M-43, M-57, M-58, M-59.
- **Green after:** default.
- **Knowledge, ledger:** `calc-u-59-qt`, `calc-u-59-web` skills,
  `reference/NewGUIGuide.md`, `FrontendParity.md`; visible renames;
  rows 59-5 ("Loading…"), 59-9, 59-14 (Qt6, Web).
- **Exit check:** the Qt6 and Web scenarios with the same IDs pass.

#### M-61 C59-8 Swift frontend on the family structure
- **Does:** the family structure in the Swift frontend (iOS without a
  menu bar) with the native counterparts of the shared pickers and
  panels and the same scenario IDs; the model picker moves; the Swift
  copy of the model facts goes; `--UITesting`, the hidden R/S button and
  the preset-state element go (per FS 4).
- **Completes:** C59-8, LIB-4 59, and LIB-10 59 if it waited for it. **Tests from:** C59-8 AC-1–13, AC-15–21,
  EC-1–6; LIB-4 AC-10–15 (Swift parts).
- **Waits for:** M-16 (FS 3, FS 4), M-33, M-56 to M-60.
- **Green after:** default; `UIRegressionTests` through attach.
- **Knowledge, ledger:** `calc-u-59-swift`, `calc-u-59-uitesting`
  skills; rows 59-9, 59-14 (Swift).
- **Exit check:** the iPhone and Mac layouts pass their scenarios.

#### M-62 C59-9 and the LIB-11 Calc-U-59 column
- **Does:** today's tools' functions only Calc-U-59 has as `ti59`
  commands, each with a test; `tools/headless/` and `tools/agentic/` go
  (`coverage.py`, `fastmode_matrix.py` and `asm59.py` stay scripts on the
  harness). Then the adapter check of §1 over Calc-U-59 and the library.
- **Completes:** C59-9, LIB-11 59. **Tests from:** C59-9 AC-1–11,
  AC-13–14, EC-2–6; LIB-11 AC-14–17.
- **Waits for:** M-57, M-58, M-59.
- **Green after:** default; no old tool and no adapter left.
- **Knowledge, ledger:** `tools/agentic/README.md`,
  `tools/headless/README.md` replaced; the memory files `headless`,
  `headless_printer_lines`, `run_time_measurement`; the `ti59-research`
  memory if it names the tools.
- **Exit check:** every function of LIB-11 AC-14's table through `calcu`.

#### M-63 LIB-16 Calc-U-59 column
- **Does:** Calc-U-59's chapters, converted once from calc-u-site `/59/`
  at `acd629c` (AC-9), the six tutorial presets upgraded, the help asset.
- **Completes:** LIB-16 59 (Done in M-64). **Tests from:** LIB-16
  AC-9–10, AC-16.
- **Waits for:** M-52, M-57 to M-62.
- **Green after:** default.
- **Knowledge, ledger:** `calc-u-59-docs` skill.
- **Exit check:** `Calc-U-59-help.zip` builds.

#### M-64 Release Calc-U-59 (R-59)
- **Does:** the first release of Calc-U-59 from tinue/calc-u
  (`calc-u-59-v3.0.0`), macOS DMG notarised locally, iOS through
  TestFlight to the App Store; the Checkpoint group (App Store and User
  Guide screenshots, retaken only where something visible changed,
  Briefing §1.1); release notes from the Calc-U-59 rows of the ledger;
  the hosted presets and overlays pass `check-preset` (LIB-6 AC-43).
- **Completes:** Briefing §7 for Calc-U-59, release side.
- **Waits for:** M-63; tinue/calc-u public.
- **Green after:** Checkpoint for Calc-U-59.
- **Knowledge, ledger:** release checklist; every 59 row visible.
- **Exit check:** the GitHub release, and the version live in the App
  Store.

#### M-65 LIB-19 Calc-U-59 column (calc-u-site)
- **Does:** `/59/` moved to the pinned help asset and the 3.0.0 Web
  frontend; redirects for every address of today; the upgraded hosted
  copies of presets and overlays; the four links to `releases/latest`
  become exact links (ADR 0028); the embed code changed once (LIB-15
  AC-16).
- **Completes:** LIB-19 59. **Tests from:** LIB-19 AC-6–10.
- **Waits for:** M-54, M-64.
- **Green after:** the site's tests (navigation tests rewritten, AC-10).
- **Knowledge, ledger:** `site-59`, `site-59-design` skills, the site's
  `CLAUDE.md` and README (stale `gh-pages` text).
- **Exit check:** calc-u.org/59 shows the help of 3.0.0.

#### M-66 Archive Calc-U-59, and the closing check
- **Does:** tinue/Calc-U-59 gets the README and is archived (ADR 0030).
  Then the checks of §5: every column of `features/INDEX.md` Done, every
  success criterion of Briefing §7 met except the SC61860 one (§5.5); `docs/Process.md` gets the phase
  status.
- **Completes:** ADR 0030 for Calc-U-59; phase 6.
- **Waits for:** M-64, M-65.
- **Green after:** default.
- **Knowledge, ledger:** the Calc-U-59 memory directory per question 4;
  a last grep of every element of `agent-knowledge.md` for the old terms.
- **Exit check:** both old repositories archived; INDEX all Done.

## 4. Release points (Part A, step 6)

- **Technically releasable** is every product after M-2 and its import
  (M-6, M-7): ReleaseIt can cut a release from its directory and every
  step keeps it green. A release before the product's last format step
  would contain none of the decided changes, and it would spend the
  one-time exception of ADR 0037 (Briefing §1.2, scope: the first
  release from this repository); question 10.
- **R-1600, after M-52** (M-53): Calc-U-1600 complete. Lacks nothing of
  its own; calc-u.org/1600 follows in M-54.
- **R-59, after M-63** (M-64): Calc-U-59 complete, App Store included;
  `/59/` follows in M-65.
- A cut of Calc-U-1600 after M-46 would read only format 2 but lack:
  the disks and tapes of version 2 and the waiting presets (M-47), the
  file opening (M-48), the family structure (M-49), DAP on the library
  (M-50), the harness commands (M-51), the help asset (M-52).
- A cut of Calc-U-59 after M-59 would hold every format change but lack:
  the family structure in Qt6, Web (M-60) and Swift (M-61), the harness
  commands (M-62), the help asset (M-63).
- The success criteria of Briefing §7, but the SC61860 one (§5.5), are
  met after M-66.

## 5. Coverage (Part A, step 5)

### 5.1 Acceptance criteria

Every AC and EC of the 33 specs lands in the step named per column in
§3 ("Tests from"). "Done in" is the step after which the column's
qa-report can show every criterion passing (§1).

| Spec | L: tested / Done in | Calc-U-59: tested / Done in | Calc-U-1600: tested / Done in | EC |
|---|---|---|---|---|
| LIB-1 | 1–25, 42–43: M-18 / M-18 | 26–33: M-19, M-25 / M-25 | 34–41: M-26 / M-26 | 1–8: M-18 |
| LIB-2 | 1–23, 35: M-23 / M-23 | 24–27: M-25 / M-25 | 28–34: M-26, M-31, M-47 / M-47 | 1–7: M-23 |
| LIB-3 | 1–27, 39: M-22 / M-22 | 28–33: M-33, M-34 / M-61 | 34–38: M-31 / M-49 | 1–6: M-22 |
| LIB-4 | 1–9: M-37 / M-37 | 10–15: M-60, M-61 / M-61 | 16–20: M-49 / M-49 | 1, 3–5: M-37 |
| LIB-5 | 1–19, 38–39: M-36 / M-36 | 20–27: M-36 / M-36 | 28–37, 40–41: M-36 / M-36 | 1–6: M-36 |
| LIB-6 | 1–34, 52–53, 55–65: M-38 / M-38 | 35–43: M-57 / M-57 | 44–51, 54: M-46 / M-46 | 1–10: M-38 |
| LIB-7 | 1–31, 46: M-41 / M-41 | 32–36, 47: M-58 / M-58 | 37–45: M-47 / M-47 | 1–9: M-41 |
| LIB-8 | 1–9, 12–23, 39: M-39 / M-39 | 24–31: M-56 / M-56 | 32–38: M-45 / M-46 | 1–8: M-39 |
| LIB-9 | 1–18, 33: M-20 / M-20 | 19–25, 34–35: M-32, M-34 / M-34 | 26–32: M-30 / M-30 | 1–10: M-20 |
| LIB-10 | 1–26, 45–47: M-40 / M-40 | 28–35: M-56 / M-56 or M-61 | 36–44: M-45 / M-46 | 1–9: M-40 |
| LIB-11 | 1–13, 21–22: M-28 / M-28 | 14–17: M-62 / M-62 | 18–20: M-51 / M-51 | 1–7: M-28 |
| LIB-12 | 1–7: M-24 / M-24 | 8–10, 14 (TI): M-24 / M-24 | 11–13, 14 (CE-502B): M-27 / M-27 | 2–3: M-24; 1: M-27 |
| LIB-13 | 1–10: M-42 / M-42 | 11–16, 23: M-59 / M-59 | 17–22: M-48 / M-48 | 1–7: M-42 |
| LIB-14 | 1–25, 42–43: M-43 / M-43 | 26–33: M-60 / M-60 | 34–41, 44: M-49 / M-49 | 1–8: M-43 |
| LIB-15 | 1–18: M-29 / M-29 | 19–23: M-34 / M-58 | 24–28: M-31 / M-31 | 1–5: M-29 |
| LIB-16 | 1–8, 14–15: M-52 / M-52 | 9–10, 16: M-63 / M-64 | 11–13: M-52 / M-53 | 1–3: M-52 |
| LIB-17 | 1–14: M-21 / M-21 | 15–19: M-33 / M-33 | 20–22: M-31 / M-31 | 1–6: M-21 |
| LIB-18 | 1–16: M-35 / M-35 | 17–24: M-57 / M-57 | 25–33: M-46 / M-46 | 1–7: M-35 |
| LIB-19 | 1–5, 15: M-54 / M-54 | 6–10: M-65 / M-65 | 11–14: M-54 / M-54 | 1–4: M-54 |

| Row | Tested in | Done in | ACs | ECs |
|---|---|---|---|---|
| C59-1 | M-19 | M-19 | 1–19 | 1–4 |
| C59-2 | M-57 (AC-10's card examples M-58, AC-12 M-65) | M-65 | 1–18 | 1–6 |
| C59-3 | M-57 | M-57 | 1–14 | 1–8 |
| C59-4 | M-59 (AC-20's hosted overlays M-65) | M-65 | 1–26 | 1–5 |
| C59-5 | M-15 (1–3), M-33 (4–14) | M-33 | 1–14 | 1–4 |
| C59-6 | M-25 | M-25 | 1–11 | 1–3 |
| C59-7 | M-58 | M-58 | 1–30 | 1–7 |
| C59-8 | M-61 | M-61 | 1–13, 15–21 (14 withdrawn) | 1–6 |
| C59-9 | M-62 | M-62 | 1–11, 13–14 (12 withdrawn) | 2–6 |
| C1600-1 | M-26 | M-26 | 1–21 | 1–3 |
| C1600-2 | M-44 (AC-15 with M-49) | M-49 | 1–25 | 1–6 |
| C1600-3 | M-50 | M-50 | 1–25 | 1–7 |
| C1600-4 | M-46 (AC-15's disk and waiting presets M-47) | M-47 | 1–20 | 1–6 |
| C1600-5 | M-51 | M-51 | 1–9, 11 (10 withdrawn) | 2–6 |

The ID ranges were taken from the specs with a script (2026-11-14): 891
acceptance criteria and 202 edge cases; no ID of a spec is outside the
tables. LIB-12 AC-14 and LIB-16 AC-16 stand under "### Calc-U-1600" in
their specs and are placed by what they are about (§2.3).

### 5.2 Ledger rows

| Rows | Step |
|---|---|
| 59-1 | in parts: M-57 (presets, constant memory), M-58 (cards), M-59 (ASM overlay) |
| 59-2 | M-32 |
| 59-3, 59-4, 59-10, 59-26 | M-57 |
| 59-5 | M-33 (busy: what is refused, LIB-3 AC-32); M-57 (full speed, keys dropped); M-60 ("Loading…") |
| 59-6, 59-24, 59-27 | M-58 |
| 59-7, 1600-7 | withdrawn |
| 59-8 | M-34 (one writer), M-58 (cards kept) |
| 59-9 | M-33 (the module no longer saved, LIB-3 AC-30); M-60 (Qt6, Web), M-61 (Swift) |
| 59-14 | M-60 (Qt6, Web), M-61 (Swift) |
| 59-11, 59-25 | M-25 |
| 59-12, 59-15, 59-16 | M-57 |
| 59-13, 59-23 | M-36 |
| 59-17 | M-57 (C59-3 AC-4, AC-11), M-58 (C59-7 AC-25) |
| 59-18, 59-19 | M-56 |
| 59-20 | M-58 (cards), M-59 (files) |
| 59-21, 59-28 | M-59 |
| 59-22 | M-19 (or struck after M-8) |
| 1600-1 | M-46 (presets), M-47 (disks) |
| 1600-2 | M-30 |
| 1600-3 | M-46 (syntax), M-47 (media steps) |
| 1600-4, 1600-16 | M-26 |
| 1600-5, 1600-6, 1600-13, 1600-17, 1600-18, 1600-19 | M-47 |
| 1600-8 | M-36 (after M-5) |
| 1600-9 | M-31 (keys dropped), M-46 (Reset and switch stop what runs) |
| 1600-10 | M-49 |
| 1600-11, 1600-21 | M-46 (1600-21's examples zip: M-53) |
| 1600-12, 1600-14 | M-45 (1600-14's "Copied" and Resume: with M-49) |
| 1600-15 | M-44 (loaders), M-48 (opening) |
| 1600-20 | M-50 |

### 5.3 Backlog items targeted at design input or done

| Items | Where |
|---|---|
| B1600-2 to 7, B1600-75, B1600-76 (done) | done at the pin; no step |
| B59-6, B1600-69 | LIB-5 (M-36) |
| B59-13 | LIB-3 topics instead of change flags (M-22) |
| B59-48, B59-50, B59-51, B59-52, B59-53, B59-58 | LIB-6 and the format (M-38); B59-49 (lowercase spelling) in the help reference chapter (M-52) |
| B59-54 | LIB-7, preset-named media (M-41) |
| B1600-46, B1600-47 | LIB-19 (M-54), LIB-16 (M-52) |
| B1600-49 | C1600-3 (M-50) |
| B1600-50 | LIB-11 (M-28, M-51) |
| B1600-51 | M-11 |
| B1600-64, B1600-65 | LIB-4, LIB-18, LIB-14 (M-37, M-35, M-43) |
| B1600-66, B1600-67, B1600-68 | LIB-7 (M-41, M-47) |
| B59-3, B59-4, B59-18, B59-30, B1600-20, B1600-25, B1600-27, B1600-62 | retargeted to their product backlogs, not built in phase 6 (question 11; `backlog.md`) |

### 5.4 Briefing §7

| Criterion | Where |
|---|---|
| One release each from the new repository; Calc-U-59 in the App Store | M-53, M-64 |
| Each product keeps its features; GUI changes only for §1.3, §1.5 | every step (golden runs, reference runs, the product criteria) |
| Each product uses the library for every `LIB-n` | the product columns; the adapter checks in M-51 (Calc-U-1600) and M-62 (Calc-U-59) |
| Current format versions read, shipped and hosted files upgraded | M-46, M-47, M-57 to M-59, M-65 |
| calc-u.org reflects both releases and shares material; the /1600 site | M-54, M-65 |
| A design for SC61860 support, validated against the abstractions | not checked in phase 6: the SC61860 work starts once both products are released (question 12) |
| Every TODO item accounted for | `docs/inventory/backlog.md`; M-1 adds the new item; §5.3 |
| The testing concept covers both products; every test in a group | M-13 |
| Agent knowledge uses the glossary terms | M-17 and the "Knowledge" field of every step; M-66 |
| No new product features | the specs (Briefing §1.3) |

### 5.5 Outside phase 6

- The skill cut (Briefing §6), after the migration.
- The SC61860 work, and with it the check of Briefing §7's SC61860
  criterion: it starts once both products are released (question 12).
- The ROM folder of the user and own dumps (ADR 0035): designed only.
- The switches of the Sharp printers (`features/INDEX.md`).
- Installation and smoke tests on Windows and Linux (Briefing §1.10).
- Questions that stay with an SC61860 product (`first-steps.md`, "Not
  first steps").
- The invariants left to review: checked in review of each step, and
  where a spec says so by a check in the build (M-26's include check,
  M-15's Xcode project check, LIB-1 AC-24).

## 6. Cold read (Part A, step 7)

Done on 2026-11-14 by a fresh agent with this plan, `features/INDEX.md`,
`first-steps.md`, the ADRs and the specs. Of its 17 findings, the main
ones were checked against the sources and hold; all were applied:
- `SessionController` was removed in M-33 while the Web frontend and
  three test files of 2.3.0 still use it: it stays until M-34.
- Today's tools include the machine classes, the ROM loading and the old
  preset readers (`tools/headless/capi.cpp`, `tools/pc1600_cli.cpp`):
  the rule of §1 and the steps M-19, M-24, M-26, M-46, M-57 move them
  along.
- `cuecards.txt` also holds the module list and cue cards
  (`Shared/ModuleCatalog.hpp:9-36`): it leaves in M-58, not M-24.
- C59-3 rests on LIB-18's and LIB-6's Calc-U-59 columns (C59-3 AC-7,
  AC-8, AC-10, EC-6, EC-7): it moved into M-57.
- Criteria that name a feature built later: the rule of §1 and "Done in"
  of §5.1; §2.1 lists every column now.
- The adapter check moved before each release (M-51, M-62); the FS 4
  spike uses a throwaway client; missing "Waits for" added; LIB-4 has no
  EC-2; the waiting Sharp presets are the last row of C1600-4 AC-15's
  table; ledger rows 59-5 and 59-9 show partly in M-33; the stale lines of
  `docs/contracts/README.md` are in §2.3.

## 7. Questions to the user (Part A, step 8)

Facts first, one recommendation each. **Answered by the user on
2026-11-14:** 1 to 11 as recommended; 12 differently (below). The answers
are applied here and in the documents they touch: question 9 in the
specs and `docs/contracts/README.md` (`76e9452`), question 11 in
`docs/inventory/backlog.md`; the others are carried out in their steps
(question 1 in M-1, 2 and 8 in M-6, 3 and 4 in M-6 and M-7).

1. **Pin of Calc-U-1600.** `v0.8.1` is not tagged. `dev-0.8.1` is at
   `76baadf`, two commits after the pin `b4ea8c8`, both docs only (a
   PC-1500 RAM disk idea and four lines in `TODO.md`). *Recommendation:*
   tag `v0.8.1` at the tip; M-1 records the two commits as a docs-only
   delta and the new `TODO.md` item as a backlog row.
2. **Start of development.** M-6 is the first commit that imports product
   code; from then on no hotfix comes from either old repository
   (Briefing §3). *Recommendation:* import after M-1 to M-5, on your go;
   a hotfix you still want for 2.3.x or 0.8.x is released before.
3. **Directories ADR 0002 does not name.** Calc-U-59: `tools/`, `bin/`,
   `reference/`, `memory/` (five CPU traces, 5.7 MB), `prompt/`, `rom/`,
   `ideas/`, `work/`, `source-material/`, `licenses/`, `.claude/`,
   `.vscode/`; Calc-U-1600: `tools/`, `headless/`, `dev/`, `firmware/`,
   `cmake/`, `3d-print/`, `source-material/` (three TIFFs, 27 MB),
   `.run/`, `.claude/`; both: `LICENSE`, `NOTICE`, `THIRD-PARTY-NOTICES.md`.
   *Recommendation:* each goes into its product directory under its
   current name ("everything that belongs to one product", ADR 0002); one
   `LICENSE` at the root (both GPL-3.0), the notices per product; the
   skills under `products/<product>/.claude/skills/`; the reference data
   of M-4 in `products/calc-u-1600/tests/reference/`.
4. **The products' memory directories.** Calc-U-59's (34 files) and
   Calc-U-1600's (59 files) are keyed to the old repository paths and do
   not load in this repository. *Recommendation:* at each import the
   files still valid move into this repository's memory directory, in
   glossary terms, one index line each; the old directory is deleted
   when its repository is archived (M-55, M-66).
5. **Which product first.** Part D (Calc-U-1600) and Part E (Calc-U-59)
   are independent after Part C2. *Recommendation:* Calc-U-1600 first: the
   library's sessions follow its coroutine jobs (ADR 0009), its release
   has no App Store step, and `/1600/` has no old addresses to redirect.
6. **Presets that wait.** Between M-46 and M-47 the Sharp presets that
   need LIB-7 (DiskWorks, `make_diskworks_media`, `pc1500-maxed-out`, and
   those of the last row of C1600-4 AC-15's table), and between M-57 and M-58 the four
   TI card examples, cannot be read. *Recommendation:* they stay out of
   the bundle file until their step, their tests are listed as waiting,
   and no release is cut in between.
7. **Calc-U-1600's version.** Calc-U-59's first release is 3.0.0 (the
   scaffold). ADR 0028's example tag is `calc-u-1600-v1.0.0`; Calc-U-1600's
   `TODO.md` has a 1.0 announcement of its own. *Recommendation:* 0.9.0,
   leaving 1.0 to that announcement.
8. **tinue/calc-u public.** It is private; ADR 0028 needs it public before
   the first release; both old repositories are public. *Recommendation:*
   public at the import (M-6), so the code stays as open as today; it
   holds no ROM (ADR 0034).
9. **Spec fixes of §2.3** (LIB-12 AC-9's md5 table, LIB-14's ledger
   citation, LIB-13 OQ-4's row names, C1600-2 OQ-1, the stale lines of
   `docs/contracts/README.md`). *Recommendation:* apply them now, each
   with a decision-log row: AC-9 gets the md5 values of the 14 `.rom.yaml`
   files from the fetch script; LIB-14 cites 1600-10 as well; LIB-13 and
   C1600-2 get a note; the contracts README names `first-steps.md` and
   marks `capabilities.md` Q-1 answered.
10. **A release before R-1600 or R-59.** Every step keeps the products
    releasable, but a release before the product's last format step holds
    the old formats and would spend ADR 0037's one-time exception.
    *Recommendation:* such an urgent release is a patch release of the old
    formats and does not count as the first release of the exception;
    the exception stays with R-1600 and R-59.
11. **Eight design-input items no spec builds:** B59-3, B59-4 (display
    scan, TMC0501 and SCOM split), B59-18 (TI-57), B59-30 (card transport
    timing), B1600-20 (socket serial transport), B1600-25 (DME0 and ME1
    cycles), B1600-27 (interrupt lines), B1600-62 (floppy `dirty` key).
    *Recommendation:* retarget each to its product backlog in
    `docs/inventory/backlog.md`, with "not built in phase 6".
12. **The SC61860 criterion** ("a design … validated against the
    abstractions: adding it needs no changes to the library"). Every
    contract has a fit table with a PC-1403 and a PC-1350 (phase 4); the
    phase 5 scenarios were dropped. *Recommendation:* the fit tables are
    the design; M-66 reads them again against the library as built and
    lists what the two models would need changed; nothing listed means
    the criterion is met.
    **Answer:** not checked in phase 6; the SC61860 work starts once both
    products are fully released.
