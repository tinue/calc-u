# The harness with Calc-U-59

How to run, trace, time and profile TI-59, TI-58 and TI-58C programs without
a window: the family's harness `calcu` (LIB-11, `features/LIB-11-harness/` at
the repository root) with the product commands under `c59` (C59-9,
`features/C59-9-harness-product-commands-of-calc-u-59/`). It replaced
`tools/headless/` and `tools/agentic/` in M-59; every function of theirs is
a command or a script below (LIB-11 AC-14; C59-9 design §4 has the table).

## Running it

Build the target `calcu` of a root tree (`cmake --build ../../build/tests
--target calcu`; the program is `build/tests/library/harness/cli/calcu`) and
run it from `products/calc-u-59` or anywhere (it finds `roms/` and
`examples/` of the source tree; `CALCU59_ROMS_DIR` names another ROM
folder). The run is the product's app layer (`app/TIAppPart`, as the
frontends host it) in emulated time only, so two runs give the same replies.

- One-shot: `calcu --product c59 preset examples:sum.ti59 then key 5 A then wait then display-text`
- `calcu --product c59 serve`: one command per line, one reply per line; from
  Python `library/harness/python/calcu_serve.py`, or `tools/calcu59.py`
  (`Run`, `split_preset`, `play_until_last`, `run_preset`, `line_times`),
  which the scripts below use. `--calcu PATH`, `CALCU59_CALCU`, else the
  `build/tests` tree's, else `build/tools` built on first use.
- `calcu --help` lists every command; the IDE run configuration is
  `.run/calcu-59.run.xml` (repository root).

## Time base

All times are **emulated seconds**: ticks of the model's clock (`ticks`,
`wait`). One instruction is 32 ticks; an instruction in the ROM's idle state
weighs four on the TI-59 and TI-58 (C59-1 AC-1), one on the TI-58C (LIB-11
AC-17). Ticks ÷ 32 are the "weighted instructions" of 2.3.0's tools (14219
a second on the TI-59 at 455 000 ticks per second; the TI-58C runs at
394 000). The times match the published real-hardware times of the
calendar programs within a few seconds.

A **run time** is what a stopwatch on a filmed calculator shows: from the
key press until the display is lit again and no program runs (`c59
run-time`; memory `run_time_measurement`). A Pause flash does not end it;
the blinking of an error display after the stop does not move it. A
preset's `elapsed-time: start` / `report` measures the same interval within
a few hundredths of a second.

## The `c59` commands (C59-9)

| Command | What it answers |
|---|---|
| `c59 run-time <key>…` | Each key pressed as `key` presses it; its run time (0 for a key that starts no program), the total; a key not finished after 4000 s is marked and ends the command |
| `c59 line-times [limit=<d>]` | Runs until idle 15 s with nothing printed (or 4000 s, the limit): each printed line's time from the command and gap, the feeds and the other lines summed up, the least time the PC-100C needs for that printout (4165 instructions a line, 1370 a feed), whether the limit ended it. Needs a PC-100C |
| `c59 hot-steps [limit=<d>]` | Runs 200 s (the limit) and notes every 97 instructions: the shares of printer busy and of the ROM's print-wait loops, the notes by program source (memory, of them fast mode; ROM; module), the 25 hottest program steps |
| `c59 step-trace <from> <to>` | The program steps between two times from the command, every 20 instructions, equal neighbours folded (`step:count`) |
| `c59 step-cost <keycode>…` | What one run of the sequence costs in fast and normal mode, instructions and ms (30 repetitions from step 008, minus the program without them); on the session's machine, which it leaves changed; refused while a program runs |
| `c59 coverage [all]` | After `profile on`: the ROM addresses 0000-17FF carried out since, without the key-detect range unless `all` |
| `c59 keycodes list [<first> [<last>]]` | The program as the PC-100C's `List` prints it (`010 43 RCL`), pasteable into a preset's `keycodes:` |
| `c59 keycodes disassemble [<first> [<last>]]` | The program as an assembly, one line per instruction (`007  Dsz Ind 03 A`); the mnemonics are the key IDs of `docs/formats/preset.md` §8.1 |
| `c59 keycodes assemble <file>` | An assembly written back into program memory (all or nothing) |
| `c59 partition`, `c59 program`, `c59 data-registers` | Program memory as bare keycodes and the data registers (C59-2 AC-9) |
| `c59 asm`, `c59 dump`, `c59 log-level`, `c59 heatmap` | The debug panel (C59-4) |

The machine state of LIB-11 AC-24 (`state-save <file>`, `state-restore
<file>`) covers the TMC0501, the RAM (the TI-58C's constant memory), the
PC-100C's mechanism, the module's latches and a card in the reader
(`core/TIMachineState.*`, `core/TIStateRecords.hpp`); the paper, the media
part's bindings and the debugger's state are the session's.

## Recipes

**A ROM trace of a key sequence** (was `tools/headless/ti59.py 5 + 7 =`):

```bash
calcu --product c59 --output-folder /tmp trace k.trace then key 5 + 7 = then trace off
python3 tools/read_trace.py --clean-heavy --skip-repeating --labels rom/TI59-labels.asm --changes /tmp/k.trace
```

`--labels` puts the ROM labels inline (`rom/TI58C-labels.asm` for the
TI-58C), `--changes` prints one line per instruction with the registers it
changed. The power-on trace: `trace boot.trace then cold-boot then trace
off`. The addresses a key runs with their counts: `profile on then key 5
then profile report 50` (by count, the most first). Registers and SCOM:
`registers TMC0501`, `peek TMC0501 scom 0 16`; breakpoints: `break`.

**Timing a program:** load the preset without its last key, press the last
one with `c59 run-time`, or press it with `key` and run `c59 line-times`.
From Python: `tools/calcu59.py`'s `run_preset(path)` and `line_times(path)`.
Compare programs that split their work between preparation keys and the
start key with `c59 run-time` over all their keys, not from the start key
alone.

**The hottest steps and what runs between two lines:** `c59 hot-steps`;
take two times from `c59 line-times` and run `c59 step-trace <a> <b>` on
the same run.

**The cost of a step:** `c59 step-cost 43 22`. The registers of 2.3.0's
tool are set: R00 = 10, R01 = 5, R04 = 1000, R10-R12 = 2.000311000312,
R22 = 98.99. The table of 2.3.0's `stepcost.py` (fast / normal
instructions; a fresh TI-59 with a PC-100C each run): Nop 144 / 225, a digit
119 / 254, x⇄t 205 / 514, RCL 22 580 / 1332, STO 30 582 / 1208, SUM 30
693 / 1318, Exc 30 643 / 1269, RC* 00 931 / 1730, HIR 05 548 / 1189, HIR 36
657 / 1303, INV Int 392 / 758, Op 20 832 / 1463, Op 05 (print) 4539 / 4566,
`+ 1 =` 874 / 1634. The command gives a few instructions more (584 / 1333 for
RCL 22 with a PC-100C, 549 / 1253 without): it runs on the session's machine.

**ROM coverage by scenario:** `tools/coverage.py build [name…]` runs the
scenarios on one `calcu serve` (each from a cold boot) into
`tools/build/coverage.json`; `show 0400 04FF`, `unique sqrt`, `gaps` read
it; `--variant TI58C`. A scenario's words leave out the key-detect range
0627-0660 (2.3.0's tool left out an instruction there only with the idle
flag set, so it kept the release gate 0627-062A; `boot` is read with `all`).

**What works in fast mode:** `tools/fastmode_matrix.py [substring]`; the
results are in `rom/fastmode.md`, the ROM side in `rom/TI59-ROM-Guide.md`
§ "What a fast-mode program can do".

**The calendar programs:** `tools/calendar/` (`check.py`, `years.py`,
`compare.py`, the generators `fastcal.py` and `swapcal.py` with their
assembler library `asm59.py`); `examples/calendar/CALENDAR_PROGRAMS.md`.

```bash
python3 tools/calendar/fastcal.py examples/calendar/calendar-09-dispatch.ti59
python3 tools/calendar/years.py examples/calendar/calendar-09-dispatch.ti59 1900 2000 2024 2026
```

## Pitfalls

- **Two numbering systems.** `key` and a preset's key steps name keys by
  their key IDs (`2`, `A`, `Pgm`); `program:`, `c59 program` and `c59
  step-cost` take keycodes (`2` is 02, `Pgm` is 36); `key-down` takes a key
  ID too (2.3.0's matrix codes are gone).
- **A program longer than the partition** writes past it into the top
  registers, as the calculator keeps them. Check that the step count fits.
- **Op 2x / Op 3x** increment or decrement only R00-R09.
- **Register values** are written with 13 significant digits. For print
  codes the position of the digits within the mantissa matters.
- **Fast mode:** the rules (no SBR, no labels as targets, safe endings) are
  in `rom/fastmode.md`; `examples/fastmode-sst.ti59` is the SST entry.
- **Jumps in `c59 step-cost`.** The sequence is repeated 30 times in a row,
  so a jump to a fixed address lands inside the repetitions and can loop
  forever; the command fails after 300 emulated seconds. Measure a jump
  with a target that moves with each repetition.
- **Settle overhead.** A `wait` until idle after each key costs time; don't
  sum per-key times of separate runs, use `c59 run-time` over the keys.
- **The session's machine.** `step-cost` writes its own program and
  registers and leaves them; load the preset again after it.
