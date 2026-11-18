# Reference runs of the 57 Calc-U-59 examples at `v2.3.0`

Recorded from 2.3.0 by `tests/reference_runs_tests.cpp`
with `CALCU59_REFERENCE_RECORD`, one `<folder>_<file>.txt` per example;
`CalcU59ReferenceRuns` (Longer) compares the examples with them.

Paper replies: for each example that
prints, `.paper-text` and `.paper-marks` hold the printout of its
`--- printed` lines as the replies of the harness commands `paper text`
and `paper marks`, written once by
`tests/paper_replies_tests.cpp` with `CALCU59_PAPER_REPLIES=1`: each line a
text line at (0, top); an empty line a feed of 7.09 units; any other line
split into the 20 characters of the PC-100C's table and drawn in its
5 × 7 font, full darkness, 10.18 units a line.
The three printer-interrupt examples (`debug_printer-interrupt-bits`,
`debug_printer-interrupt-sweep`, `printer-interrupt`) have a `.paper-text`
only: their lines cut short are not in the recordings.
The Frequent case of that file checks that the files equal what the
converter gives; `CalcU59ReferenceRuns` compares new runs with them.
