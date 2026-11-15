# Part C2: summary for the C2/D boundary review

Status: for the user's review (2026-11-16). Part C2 (M-35 … M-43, the
other Library columns) is merged into `main`; the Part D designs are
drafted. Plan §1: at this boundary the user reviews C2 (this summary, the
qa-reports) and approves the Part D designs.

## 1. What C2 did

| Step | Feature | Merge | Result |
|---|---|---|---|
| M-35 | LIB-18 Machine control and startup | `ce44cdd` | Library Done (in M-41, with the media clauses) |
| M-36 | LIB-5 Typing, all columns | `eb429a8` | Library Done (in M-38, with the preset clauses); 59 and 1600 In progress (preset parts: M-54, M-46) |
| M-37 | LIB-4 Control state | `3391017` | Library Done |
| M-38 | LIB-6 Presets | `8036a45` | Library Done (after M-40 and M-41, `60e5b09`) |
| M-39 | LIB-8 Paper output | `bd980f2` | Library Done; creates `library/qt6/` (`calcu_qt6`) and `library/web/components/` |
| M-40 | LIB-10 Debugger core | `8333566` | Library Done |
| M-41 | LIB-7 Media and save folder | `a85d5b2` | Library Done (in M-42, with AC-29 on the Web) |
| M-43 | LIB-14 Window structure and shell | `d5e4ffc` | Library Done |
| M-42 | LIB-13 Opening files | `70c60a7` | Library Done |

Every Library column of C2 is Done (`features/INDEX.md`). Each step has
its qa-report (`features/<ID>-*/qa-report.md`) with a result per AC and
its deviations. Also in C2: CoreTests skip without ROMs (`e2fdc68`,
item 2 of the C1 report).

Tests on `main` now: Frequent 32 entries (34 with Qt);
`calcu_library_tests` 923 tests, `calcu_qt6_tests` 47,
`calcu_qt6_scenarios` 9, `LibraryWebPlaywright` 138 cases.

## 2. Your rulings of the C1/C2 boundary, applied

- `printer-timing.ti59` is not re-recorded. Since M-36 a preset key is
  held 100 ms of emulated time, and the example prints 104 lines again,
  as in the 2.3.0 reference. `CalcU59ReferenceRuns` lists 36 accepted
  differences with their reasons (mostly the driver's wall-time line and
  `Elapsed:` lines); no reference file changed.
- Visible changes: the steps added or updated ledger rows themselves.

## 3. Changes beyond the approved designs (from the qa-reports)

- **LIB-3 additions** not in the consolidated list (LIB-18 §8), each in its
  own commit: `MachineSession::nextDue()` (M-36); `yieldToHost()`,
  `Job::waitsOnHost()`, `SessionContext::wallTime/virtualTime` (M-38);
  `OperationContext::run` waits while the run control holds the machine
  paused, `setMachine` clears paused (M-40); `AppSession::close()` calls
  the parts' `closing` (not the destructor; M-41);
  `SessionContext::startOperation`, `OperationResult::result` (M-42).
- `expect:` and `screenshot:` read the display from the machine, not the
  `display` topic (M-38).
- A harness run starts at key readiness (+1/60 s; M-35).
- The Qt6 shell has no saved-media list, so Import Media… / Saved Media…
  are Web only (LIB-7 OQ-4; M-42).

## 4. End-of-part checks

- **CI** on `70c60a7`: both products failed on GCC 13 (a missing
  `<filesystem>` in `library/opening/tests/TestOpeners.hpp`), fixed in
  `b1774bf`; every other job passed. The re-run: see §7.
- **Longer** (20 entries): 15 passed at first.
  - `CalcU1600WebPlaywright`: the M-36 case "keys during a load are
    dropped" polled for busy and could miss a short model switch. Rewritten
    to send the keys when busy is set (`9e0bee1`); 15/15, group passes.
  - `CalcU59UIRegression`: passed on the re-run (a stale XCFramework).
  - `CalcU1600Shots.01–03`: failed because the screen was locked (window
    capture and native menus need a live display); 04–09 passed.
    **To re-run with the screen unlocked.**

## 5. Findings and open points

1. **Sharp golden runs and the PC-1500 typing profile (M-36).** The golden
   runs stay equal because the golden driver taps keys with 0.8.1's queue
   timing. With the real profile (LIB-5 AC-31), `pc1500a-ce150` differs in
   two cycle counts and one audio sample count; display, paper and memory
   are equal. Not re-recorded. Question Q-1 below.
2. `screentest_test_58_glyphs.ti58` and `screentest_test_cdebug.ti59`
   never end (they end frozen; so on `main` before M-36). Waits for
   LIB-10 59 (M-53).
3. Not tested in C2: the Swift app from a worktree (`xcodebuild`: "no
   XCFramework found"; it builds from the main checkout, and
   `CalcU59UIRegression`/`CalcU59SwiftUnitTests` pass there); Qt6 typing
   tests (`CalcU59QtTyping`, `CalcU1600QtTyping`) not written; LIB-8's
   reference pictures not compared with a real Copy of 2.3.0/0.8.1.
4. A Qt6 crash at window teardown (`MainWindow::releaseHeldKeys`) was
   found and fixed in M-36; it showed the macOS crash dialog.

## 6. Part D designs for approval

New: `C1600-2`, `C1600-3`, `C1600-4`, `C1600-5`, `LIB-16`. A "Part D: the
Calc-U-1600 column" section appended to `LIB-4`, `LIB-6`, `LIB-7`,
`LIB-8`, `LIB-10`, `LIB-11`, `LIB-13`, `LIB-14`, `LIB-18` (the approved
parts unchanged).

Proposed new ledger rows (visible changes; your C1/C2 ruling covers
them): 1600-34 (a plotter's paper continues after Reset or Cold boot;
LIB-8), 1600-35 (the Web page binds no menu shortcuts; Qt6 gets Ctrl+O),
1600-36 (a missing ROM is shown in the window, the app does not exit),
1600-37 (a click between keys presses the nearest key within 24 px; touch
on the Qt6 faceplate; all LIB-14). Renames "Reset All" → "Cold boot",
"Default presets" → "Startup presets" (LIB-18).

### Questions (each with a recommendation)

- **Q-1 (LIB-6 1600, finding 1).** Keep the Sharp golden driver on
  0.8.1's key timing (recommended), or move it to the real PC-1500
  profile and record `pc1500a-ce150` anew as the accepted difference?
- **Q-2 (C1600-2).** A preset's typed `program:` in M-44 (recommended:
  one typer from then on; if the reference runs differ, the old typer stays
  for presets until M-46) or in M-46?
- **Q-3 (LIB-13 1600).** Over the Qt6 window a drag of any one file is
  shown as taken, and the drop refuses with a notice a file that is
  nothing the product opens. Right (recommended), with the words added to
  row 1600-15?
- **Q-4 (LIB-10 1600, D-15).** Build the DAP's breakpoints and line steps
  once in `app/dap/` in M-45 (recommended), instead of a throwaway
  `DapRunControlBridge` in the Qt frontend for M-45 … M-50?
- **Q-5 (LIB-11 1600).** The loader matrix (`dev/loader-matrix/`, a
  one-time 330-cell check) uses adapters that go in M-51: drop it like the
  tape matrix (recommended; C1600-2's tests keep the loaders), or move it
  onto `calcu serve`?

### Plan changes the designs propose (applied on approval)

- M-44: its menu items and dialogs stay in today's menus and come into
  the LIB-14 shell "with M-49"; AC-20's Qt6 scenarios in M-49.
- M-46: five presets (DiskWorks, `make_diskworks_media`,
  `pc1500-maxed-out`, `startup/default-pc1600`, a test fixture) wait for
  M-47 (they need LIB-7's media); `default-pc1600` stays out of
  `bundle.yaml` until then.
- M-45: ledger rows 1600-32 and 1600-34 become visible there.
- M-49 also waits for M-44, M-45, M-48; it widens the attached harness to
  the serve command set (also for C59-5's Swift attach).
- M-50 also waits for M-44 and M-49.
- LIB-16 Library: Done in M-60, not M-52 (AC-8 builds both products'
  assets; Calc-U-59's chapters come in M-60).
- §5.1: LIB-6 Library needed M-40 and M-41, LIB-7 Library M-42 (done).

## 7. CI after the fixes

- On `b1774bf` both products built everywhere, but three LIB-10 checks
  failed on Linux only: `pause`, `resume` and `step` replied with the run
  state from **before** the call, because
  `fromResult(rc.resume(), runStatePayload(rc))` leaves the order of the
  two arguments to the compiler and GCC takes the state first. Fixed in
  `c6d9e6b` (`withRunState`). Calc-U-1600's mac-aarch64 job failed at
  Apple's stapler (network lost, error 68): transient.
- Re-run on `c6d9e6b`: Calc-U-1600 run 43775309855, Calc-U-59 run
  43775312407 (result: see the session's final note).
