# Part E designs: what is open for the D/E boundary review

Approved by the user 2026-11-16 ("do part e"), with every recommendation
below (Q-1 to Q-3, §3 to §5); applied in the commit that adds this line
(ledger rows 59-49 to 59-56; 59-48 is left to M-53).

Status: for the user's review (2026-11-16). The Part E designs are drafted
and merged into `main` as drafts ("Draft for the Part D/E boundary"). No
Part E code is written: the user stopped the work after the designs
(2026-11-16). The Part D review is `docs/reviews/d-summary.md`; the Part D
simplification `docs/reviews/d-simplification.md`.

## 1. The designs

| Step | Design sections | Merge |
|---|---|---|
| M-53 | LIB-8 §11, LIB-10 §12 | `592fbdc` |
| M-54 | LIB-6 §11, LIB-18 §11, LIB-5 §10, C59-2 (new), C59-3 (new) | `3e113b4` |
| M-55 | LIB-7 §11, LIB-12 §10, C59-7 (new) | `997ec6f` |
| M-56 | LIB-13 §10, C59-4 (new) | `592fbdc` |
| M-57 | LIB-4 §10, LIB-14 §10 | `2def19f` |
| M-58 | C59-8 (new) | `2def19f` |
| M-59 | LIB-11 §11, C59-9 (new; §4 maps every function of today's tools) | `997ec6f` |
| M-60 | LIB-16 §13 | `2def19f` |

Every open point was decided by best guess. The decisions are the `D-n` rows of each
design's "Decided" section.

## 2. Questions for the user (each design is written on the recommendation)

- **Q-1 (LIB-10 §12.13).** LIB-10 AC-30 says a Reset of the TI-59/58 does
  not fire pause-on-start `cpu`; ledger row 59-34 and the C59-1 qa-report
  say it fires today (the ROM's start-up runs in the run loop).
  Recommended: keep today's behaviour, a decision-log row in LIB-10's spec
  says row 59-34 replaces AC-30's Reset clause.
- **Q-2 (C59-2 §10).** C59-2 AC-9 reads two ways: `ti59 program 010 05`
  could read 5 steps or write keycode 05. Recommended: the read count is
  written `count=<n>` (like C59-9's `limit=`).
- **Q-3 (C59-7 Q-1).** The spec names the card buttons "Fit Card" / "Fit
  Fresh Card"; `docs/glossary-mapping.md` (2026-11-12) renames them "Arm
  Card" / "Arm Fresh Card". Recommended: "Arm Card", with decision-log rows
  in C59-7 and LIB-14 and a ledger row.

## 3. Changes the designs make to "Done in" (plan §5.1)

- LIB-10 59: M-53 (not "or with M-58"); the Swift debug buttons change in
  M-53 because the interim topics go (LIB-10 D-27, LIB-8 D-27).
- LIB-6 59 and LIB-5 59: M-58 (AC-42's GUI scenarios need the Qt6 Examples
  menu of M-57 and Swift; LIB-5 AC-22, AC-24 Swift).
- LIB-7 59: M-57 (the Web page gets its File menu there).
- C59-7: M-58 (AC-28's Qt6 and Swift scenarios come with attach).
- C59-4: M-56, AC-20's hosted overlays M-65, AC-24's Qt6 scenarios M-57.
- M-58 also completes LIB-8 59's Swift part, C59-6 and LIB-3 59.

## 4. Proposed ledger rows (to be numbered from 59-48 when applied)

- LIB-8 §11.11 (M-53): Copy of the paper without the white margin, with
  "Copied to clipboard" / "Copy failed"; the Qt6 paper follows the newest
  line only near the end; a 2.3.0 preset without `PRINTER:` keeps the paper
  (until M-54).
- LIB-7 / C59-7 (M-55): `card:` or `arm:` naming a missing card refuses the
  preset (2.3.0 put a blank card in); "Arm Card" names (Q-3); the card
  dialogs list cards by the name inside (or folded into 59-6).
- LIB-14 / C59-8 (M-57, M-58): Web Help and About in the Help menu or
  behind More, More also holds Import Media…, Saved Media…, About; settings
  dialog changes (Startup model "Last used", "Save folder", Qt6 groups,
  changes applied at once); Qt6 opens with a ROM file missing and names
  it; the Web storage banner; Swift notices as alerts, "Loading…", the family
  settings rows. Row 1600-48 (storage banner) takes effect in M-57.
- M-54: none new (rows 59-1, 3, 4, 5, 10, 11, 12, 15, 16, 17, 26, 39, 42);
  59-15's "to be confirmed" is answered by C59-3 test T2.

## 5. Proposed plan notes

- M-53: Waits for + M-36; Does + the Swift paper strip on the library's
  paper topic until C59-8; Green after + the Swift app builds and
  UIRegressionTests pass; knowledge list extended.
- M-54: `globe-print` and the other three card examples stay in the 2.3.0
  format until M-55 (left out by name); skill `calc-u-59-statefiles`
  renamed `calc-u-59-presets`, `reference/StateFileFormat.md` deleted;
  startup preset rows in Settings come in M-57/M-58.
- M-55: Waits for + M-53.
- M-56: Waits for + M-54.
- M-57: Waits for + M-53 (and M-49); Does + the library additions
  (faceplate origin, media-dependent menu items), Calc-U-1600's storage
  banner (D-simplification Q-4), Qt6 attach, `--pacing-probe` as an attach
  client; Tests from + C59-4 AC-24 (Qt6), LIB-6 AC-42 (Qt6).
- M-58: Tests from + LIB-6 AC-42, LIB-5 AC-22, AC-24 (Swift); its Swift
  builds run from the main checkout.
- M-59: Waits for + M-57, M-58; CI dispatch (`calcu` with Calc-U-59 on
  Linux and Windows).
- M-60: Does + the Q-3 change of the shared chapter 2 (D-simplification);
  CI dispatch (workflow `help` job, `release.yaml`); §5.1 LIB-16 "1–8,
  14–15: M-52 / M-60".

## 6. Points to watch (from the designs)

- M-53 keeps `makeAppSession` owning `PaperPart` and `DebuggerPart`; the
  move to `LibraryParts` is M-54's (a `StartupPart` would start a second
  machine before then).
- The reference comparison of M-54 compares display, partition, raw RAM
  and paper, no longer the log, ticks or `Elapsed:`; `kAccepted` goes.
  `printer-timing.ti59` is not re-recorded.
- C59-8 §8: not verified that an XCUITest runner can write a settings file
  the simulator app reads by path; M-58's first test checks it.
- A branch `m53-paper-debugger-59` / M-53 worktree was started and stopped
  before any commit; it holds no work.
