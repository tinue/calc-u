# Reference runs of the 57 Calc-U-59 examples at `v2.3.0`

Recorded in M-9 (`docs/migration-plan.md`) by `tests/reference_runs_tests.cpp`
with `CALCU59_REFERENCE_RECORD`, one `<folder>_<file>.txt` per example;
`CalcU59ReferenceRuns` (Longer) compares the examples with them.

Paper replies (LIB-8 AC-39; M-39, 2026-11-15): for each example that
prints, `.paper-text` and `.paper-marks` hold the printout of its
`--- printed` lines as the replies of the harness commands `paper text`
and `paper marks` (LIB-8 AC-23), written once by
`tests/paper_replies_tests.cpp` with `CALCU59_PAPER_REPLIES=1`: each line a
text line at (0, top); an empty line a feed of 7.09 units; any other line
split into the 20 characters of the PC-100C's table and drawn in its
5 × 7 font, full darkness, 10.18 units a line (LIB-8 AC-25, design §4.1).
The three printer-interrupt examples (`debug_printer-interrupt-bits`,
`debug_printer-interrupt-sweep`, `printer-interrupt`) have a `.paper-text`
only: their lines cut short are not in the recordings (LIB-8 design D-13).
The Frequent case of that file checks that the files equal what the
converter gives; C59-2 AC-11 (M-54) compares new runs with them.
