# Part D: summary for the D/E boundary review

Status: for the user's review at the Part E/F boundary (the user
pre-approved the D/E boundary on 2026-11-16 and reviews D together with E).
Part D (M-44 … M-52 and the AC-13 machine-state step: the Calc-U-1600
column) is merged into `main`, and so is the Part D simplification
(`docs/reviews/d-simplification.md`, reviewed by the user; its answers
to Q-1 … Q-5 are recorded in `7bdd51c`). Plan §1: at this boundary the
user reviews D (this summary, the qa-reports, the results of its first
steps, the release-point status).

## 1. What D did

Merges on `main` from the C2/D boundary (`c75cbc4`) on.

| Step | Feature | Merge | Result |
|---|---|---|---|
| M-44 | C1600-2 Sharp program files | `83100ff` | Done in M-49 (AC-15, AC-20's Qt6 scenarios); ledger 1600-38 |
| M-45 | LIB-8 and LIB-10 Calc-U-1600; C1600-3's layer in `app/dap/` | `b380e26` | Both columns Done in M-46 (their format 2 clauses) |
| M-46 | LIB-6, C1600-4, LIB-18 Calc-U-1600 (preset format 2) | `0f1d319` | LIB-8, LIB-10, LIB-15, LIB-18 1600 Done; LIB-6 1600 and C1600-4 Done in M-47; ledger 1600-39 |
| (M-36 rest) | LIB-5 AC-33 in the Qt6 frontend | `7459a9a` | LIB-5 1600 Done (`CalcU1600Shots.lib5-ac33`) |
| M-47 | LIB-7 Calc-U-1600 | `00b62f5` | LIB-7, LIB-6, LIB-1, LIB-2 1600 and C1600-4 Done; floppy format 2; ledger 1600-40 … 43 |
| M-48 | LIB-13 Calc-U-1600 | `81d09ef` | Done in M-49 (the Qt6 hand checks became M-49's attach scenarios, user 2026-11-16); ledger 1600-44 |
| M-49 | LIB-4 and LIB-14 Calc-U-1600 | `405deed` | LIB-3, LIB-4, LIB-13, LIB-14 1600 and C1600-2 Done; ledger 1600-45 |
| M-50 | C1600-3 DAP in the app layer | `51acc6e` | Done; row 1600-20 in effect |
| M-51 | C1600-5 (without AC-13), LIB-11 Calc-U-1600 | `4cab569` | LIB-11 1600 Done; the old CLIs, probes, matrices and `LegacyCalls` gone; ledger 1600-46 |
| M-51, part 2 | C1600-5 AC-13 (machine state) | `818374d` | C1600-5 Done (`CalcU1600MachineStateTests`, Longer) |
| M-52 | LIB-16 Library and Calc-U-1600 | `8b86ef2` | In progress (Library Done in M-60, 1600 in M-61); help tool, chapters, `help` job, asset in `release.yaml`; ledger 1600-47 |
| Simplification | P-1 … P-10 | `350bc61` … `1ad3da9` | Report `bebeba3`: `docs/reviews/d-simplification.md` |

Besides the steps: the local Linux CI in Podman (`c29cebe`, ADR 0039)
and the testing scheme with it (`cc329ee`, `605c66a`, `668fefd`); fixes
from CI and local runs (`c69b5d2`, `5bbd6f2`, `8ba768c`, `c6d1370`).

Every Calc-U-1600 column of the Library features and every C1600 row is
Done, but LIB-16 and LIB-19 (§5). Each step has its qa-report section
(`features/<ID>-*/qa-report.md`, "Calc-U-1600 (M-nn …)") with a result
per AC and its deviations.

Tests on `main` (`1ad3da9`): Frequent 42 entries, 31.6 s wall (`-j8`);
Longer 33 entries, 670 s. New programs in D: `CalcU1600DapTests`,
`CalcU1600SessionTests` (Frequent); `CalcU1600QtTests`,
`CalcU1600HarnessTests`, `CalcU1600MachineStateTests`, the DAP smokes
(Longer); `LibraryHelpTool`, `CalcU1600HelpTests`, `…HelpCheck`,
`…HelpBuild` (Frequent); `CalcU1600HelpPictures` (Checkpoint).

## 2. Rulings of C2/D applied

- **Q-1** The Sharp golden driver stays on 0.8.1's key timing; nothing
  re-recorded.
- **Q-2** A preset's typed `program:` goes through one typer from M-44 on,
  in the app session. The old typer stayed for session-less presets (CLIs,
  probes, matrices) until they went (M-46, M-51); it is now test support
  only (§6, item 5).
- **Q-3** A drag of any one file over the Qt6 window is taken; the drop
  refuses what the product does not open, with a notice (row 1600-15, in
  effect from M-48).
- **Q-4** The DAP was built once in `app/dap/` (M-45, M-50); no
  `DapRunControlBridge`.
- **Q-5** `dev/loader-matrix/` dropped with the tape matrix (row 1600-46).
- The plan changes the designs proposed were applied in `c75cbc4`
  (M-44's menus with M-49, M-46's waiting presets with M-47, LIB-16
  Library Done in M-60, …).
- During D: row 1600-43 (PC-1600 idle waits for the disk motor) confirmed
  by the user (`9363a9b`); M-48's Qt6 hand checks moved to M-49's attach
  scenarios (user); the local Linux CI before every CI dispatch (user).
- Visible changes got ledger rows without a question (rows 1600-38 …
  1600-47 from the steps; 1600-48 from the simplification's Q-4, not
  coded yet).

## 3. Changes beyond the approved designs (from the qa-reports)

Only the ones a reviewer may want to know; each qa-report lists all.

- **Library additions.** LIB-7: `ReadOutcome::Status::NotMedium`,
  `MediaPlace::writesWhenAsked()`, `KindDecl::fitsPinOut`,
  `KindDecl::companionSuffixes`, `MediaPart::readOpened` (an opened medium
  is read with its companions). LIB-6/LIB-5 for the DAP in virtual time:
  `PresetsPart::loadWithin`, `TypingEngine::setMachine` takes the
  session's paused state, `TypingPart` ignores a repeated
  `machineChanged`. LIB-14: `ShellControlBar` leaves a product control's
  visibility to the product.
- **Tests outside `CoreTests`** for its budget: `CalcU1600DapTests`
  (Frequent), `CalcU1600HarnessTests`, `CalcU1600MachineStateTests`,
  `CalcU1600QtTests` (Longer; D.8 had it Frequent at ~5 s, it takes 25 s).
  LIB-5's `CalcU1600QtTyping` is the check scenario
  `CalcU1600Shots.lib5-ac33`.
- **C1600-2:** `type-basic` takes text only and names lines not stored in
  its reason; new refusal sentences in the menu's style; `load-code … run`
  loads in the handler and starts as an operation.
- **LIB-4/LIB-14 1600:** the 60-pin places are chains, not `Takes::One`;
  two more product entries (`c1600.mount-directory`, availability on
  `c1600.host-drive-folder`); `c1600.lamps` a topic of its own; CE-1600P
  ROM ▸ only while it is plugged in, disk controls after the tape
  controls (row 1600-45); `bus-rom` gets no control.
- **LIB-13 1600:** `c1600.upload` stays for Import Media… only; the
  Qt6 drag sends the size, large files are refused unread.
- **LIB-18 1600:** a second Mount Directory is a replace with a power
  cycle (row 1600-39); a start without the full ROM set is refused with
  the lookup's text.
- **LIB-10 1600:** bank windows by `bank=`/`vbank=`; the `io` space reads
  only the bank registers; watches on `io` and the `-written` views never
  hit; the DAP's target view casts to `SharpDebugTarget`.
- **LIB-11 1600:** the run takes the app layer's library parts
  (`appLayerParts()`, since P-2 `LibraryParts`); `prepare` takes a
  non-const `RunSetup`; the yield hook, `syncFromCore` and
  `ProductMachines::startUp` removed beyond D.5.
- **C1600-5:** `c1600.routine-call` is one capability on the main CPU;
  machine state has no `SharpMachineState` files (a `StateKind` in
  `SharpMachine`, forms in `StateRecords.hpp`, `CardState.hpp`); the
  session's state (media bindings, paper, serial queues, breakpoints) is
  not part of the machine state.
- The simplification's departures from approved designs: its §8.

## 4. End-of-part checks

- **GitHub CI per step** (plan §1, after the local Linux CI from `c29cebe`
  on):
  - `b380e26` (M-45): red (Qt 6.4 moc with `<concepts>`; a Calc-U-59
    attach-test race; a stapler transient). Fixed in `c69b5d2`; re-run
    43824544353 / 43824547556 green.
  - `0f1d319` (M-46): Calc-U-59 43835271497 green; Calc-U-1600 43835267030
    red (GCC 13 internal compiler error, temporaries in `co_await`).
    Fixed in `5bbd6f2`; re-run 43838332498 green.
  - `00b62f5` (M-47): Calc-U-1600 43844589515 green; Calc-U-59
    43844593463 red in `web` only (`offline.spec.js`, a test race; fixed
    in `c6d1370`, test only). Green again from `81d09ef`.
  - `81d09ef` (M-48) 43856314469 / 43856319055, `405deed` (M-49)
    43875523176 / 43875527869, `51acc6e` (M-50) 43887368205 /
    43887372645: green. `4cab569` (M-51): local Linux CI only.
  - End of part, `818374d`: Calc-U-1600 43906288279, Calc-U-59
    43906293745: green.
  - After the simplification, `bebeba3`: 43928718095 / 43928724573: green
    on every job.
- **Local Linux CI** (GCC 13): green on every step's merge and on the
  final state (default jobs, `web-59`, `web-1600`, `help-1600`).
- **Longer.** A run on `818374d` was started; its result is not recorded.
  The run after the simplification (`1ad3da9`, `build/groups`): 32 of 33
  passed at first, all 32 Sharp and 57 TI reference runs among them, and
  all sixteen `CalcU1600Shots` (C2's open re-run of Shots 01–03 is done).
  `CalcU59UIRegression` failed once after 33 s and passed unchanged on the
  rerun; the cause is not known (the log was overwritten).
- **Checkpoint.** `CalcU1600HelpPictures` run by hand on macOS
  (2026-11-16): 22 pictures written, four not (§6, item 2). No other
  Checkpoint run in D.
- **Golden runs and references:** none differs, none re-recorded.
  `CalcU1600ExampleReferences`: 32 of 32 equal, with the exceptions of
  C1600-4's qa-report (`Calculat`, `ce150_demo`, biorhythmus' cursor,
  `DiskWorks`' display, `make_diskworks_media`'s file dates).

## 5. Columns not Done and where they close

| Feature | Column | Status | Closes |
|---|---|---|---|
| LIB-16 Help content | Library | In progress | M-60 (AC-8 builds both products' assets) |
| LIB-16 | Calc-U-1600 | In progress | M-61 (the asset in a release) |
| LIB-16 | Calc-U-59 | Specified | M-60 |
| LIB-19 calc-u-site | all three | Specified | M-62 (Library, 1600), M-65 (59) |
| Every other Calc-U-59 column; C59-2, -3, -4, -6, -7, -8, -9 | Calc-U-59 | Specified / In progress | Part E, M-53 … M-60 (plan §3) |

## 6. Findings for the user

1. **PC-1600 idle** (LIB-6 AC-45): key readiness counts running BASIC
   (the BREAK check) and machine code with interrupts on as idle; a
   narrower rule broke `INKEY$` programs and was taken back. The disk
   motor rule (row 1600-43) is confirmed. `DiskWorks`' display stays
   excepted for it.
2. **Four help pictures** (`method: system`) need the Automation /
   Accessibility grant the agent lacks: run
   `tools/make_screenshots.sh 01-introduction 02-getting-started 05-peripherals`
   from iTerm2. They are 0.8's pictures until then.
3. **About 920 glossary "Avoid" warnings** in the help check, mostly
   common words in another sense (code, load, slot, host, …). Narrow the
   glossary's Avoid lists?
4. **LH5803 never returns from `pc1600 call`:** it does not run while the
   Z-80 runs the machine (MODE 0, or after `MODE 1`); a call on it ends at
   its timeout.
5. **Sharp typers and `JobForward`** are still the way about 25 core test
   files type into a bare machine (moved to `tests/support/` in P-4, out
   of `calcu_core`). Moving them onto LIB-5's engine is a step of its own:
   the user's call.
6. **`tape record` binding ends at eject:** after a `state-restore` in the
   same run, an eject wrote no WAV (the binding is the session's, not the
   machine's state).
7. **Sharp golden runs with the real typing profile** (LIB-5 AC-31) still
   differ in two cycle counts and one audio sample count for
   `pc1500a-ce150` (Q-1 kept 0.8.1's timing; it stands as known).
8. **`screentest_test_58_glyphs.ti58`, `screentest_test_cdebug.ti59`**
   never end; wait for M-53.
9. **Untested:** the Swift app from a worktree (`xcodebuild`: "no
   XCFramework found"); `CalcU59QtTyping` not written (Part E); LIB-8's
   reference pictures not compared with a real Copy of 2.3.0/0.8.1; the
   debugger's speed against 0.8.1 not measured (LIB-10 1600).
10. **A `.tape.yaml` dropped alone on the Web** is refused (its WAV is
    missing); Import Media… with both files works, as before M-48.
11. **The VS Code extension** still calls a Cold boot "All Reset & Stop"
    (`vscode/calcu1600-debug/package.json`); the help says "(a Cold boot)".
12. **Outside the repository:** the M-51 agent edited the global skill
    `~/.claude/skills/PC1500/SKILL.md` (the CLIs → `calcu`).
13. **ReleaseIt** branch `several-products` (`ad537aa`, `d8a5c49`,
    `4a4cbb4`) is not merged or installed; needed before Part F.
14. During M-45 one `calcu_qt6_tests` run wrote a white test picture to the
    machine's general pasteboard (fixed: the test uses a pasteboard of its
    own).

## 7. Release-point status (plan §4)

No release in D, as planned: R-1600 is M-61, R-59 M-64, both after Part E.
Calc-U-1600 is complete on the library but for its help asset in a release
(LIB-16 1600, M-61); the asset is built by the `help` job and listed in
`release.yaml`. Open before R-1600: the four help pictures (§6, item 2),
row 1600-48 (the Web storage banner, with M-57's banner or before M-61),
ReleaseIt `several-products` (§6, item 13). Calc-U-59's open columns are
Part E's; it builds and its tests are green, and it need not be releasable
before Part E ends (plan §4).

## 8. What Part E starts with

M-53: LIB-8 and LIB-10, Calc-U-59 columns (Qt6, Web), then M-54 … M-60 in
the plan's order. The Part E designs are the Calc-U-59 parts of the C2
designs plus what the simplification's §9 lists per step (`LibraryParts`
for `TIAppPart` in M-54, the fitted card in M-55, `opening::startFile` in
M-56, the shell value forms and host keys in M-57, …). The user's
simplification rulings to carry out: Q-1 in M-56 (ledger 59-47), Q-3 in
M-60, Q-4 (row 1600-48) with M-57's banner or before M-61, Q-5 (delete
`products/calc-u-1600/tools/delactions.sh`) in any small step. The user
asked for Part E to run to its end and then stop (no Part F).
