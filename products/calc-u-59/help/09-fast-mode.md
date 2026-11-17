# Fast mode

The TI-59's ROM runs some programs that are not in its memory, the
built-in functions Σ+, x̄, P→R, D.MS and Op 11–15 and the programs of a
module, on a faster path: between two steps it skips the display update,
the R/S check, the trace printing and the stop on an error. Such steps
run in 40–50 % less time. Users' clubs found firmware bugs that put a
program of your own on that path: **fast mode**. Calc-U-59 runs the
original ROM, so fast mode works as on the real calculator, with its
pitfalls. This page sums up what you need to use it; the whole story,
with its sources, is the article
[TI-59 Fast Mode](https://calc-u.org/59/articles/fast-mode.html).

## What changes in fast mode

- The display stays as it was until the program ends, and R/S cannot
  stop the program.
- An error does not stop the program, and nothing is traced on the
  printer.
- A program runs in about half the time: Martin Neef's 69! (CHIP
  10/1980) took 29.2 s in normal mode and 14.6 s in fast mode on his
  calculator; Calc-U-59 takes 29.0 s and 14.3 s.

## Rules for a fast-mode program

- **Allowed:** number entry, arithmetic, parentheses and all functions;
  STO, RCL, SUM, Prd, Exc with INV and Ind; HIR, CMs, CP, x⇄t, CE, CLR;
  St flg, If flg, Fix, Eng, Deg, Rad, Grad; Op 00–08, Op 10, Op 16–39;
  Prt, Adv, Pause; transfers to an absolute or Ind address (GTO, x=t,
  x≥t, If flg, Dsz).
- **Not allowed** (each leaves fast mode and loses the result): a label as
  a target, A to E′ called from the program, every SBR, and the built-in
  ROM routines Σ+, x̄, P→R, D.MS, Op 11–15.
- **Ending the program** is where most fast-mode programs go wrong. End
  with RST or R/S while X is 0, a number still being typed, a value just
  shown by Pause or Prt, or one that went through `EE INV EE`. Safe:
  `CLR RST`, `0 RST`, `RCL nn Pause RST`, `RCL nn EE INV EE R/S`. Any
  other result hangs the calculator with a dark display: only switching
  off helps, which erases the program (in Calc-U-59: Machine ▸ Cold boot).

## Ways in

Most ways in put X = 1.000000000002 on the display first
(`1 + 2 EE 12 +/- =`):

- **SST**: a St flg step at step 007 and the program from step 008; key
  `GTO 007 SST 0 STO`. The example **fastmode-sst** under
  **Examples ▸ Programs** does this.
- **St flg on the last step of the partition**: the program halts with a
  flashing display; key `7 EE`.
- **LRN inside a module program**, `00000 Pgm 02 SBR 239` with the Master
  Library: it reaches the same state by another bug, and it **clears the
  whole memory**. The first step must lie on a multiple of 8.

Entering fast mode overwrites the ten user flags and the Fix setting: set
the flags you need inside the program.

## In Calc-U-59

- The debug panel's CALCULATOR tab shows when a program runs in fast mode;
  while you step such a program, X may hold no number and is shown raw, in
  amber ([Debugger](07-debugger.md)).
- More fast-mode programs, among them the calendar programs, are in the
  [TI-59 Software Collection](https://calc-u.org/59/software/).
