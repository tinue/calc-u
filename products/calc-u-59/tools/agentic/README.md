# Agentic tools

Command-line tools for interactive work by a coding agent (or a person at a
terminal) on TI-59 programs: run a preset (`.ti59`, format 2,
`docs/formats/preset.md` at the repository root) without a window, time it,
see where the time goes, measure what single steps cost, disassemble and
assemble keycode programs. Each tool prints a few plain lines that are easy
to read back in a conversation; none of them opens a window or needs the app.

Since M-54 they load presets through the family's harness `calcu`
(`calcu --product 59 serve`, LIB-11; `calcu59.py` here, over
`library/harness/python/calcu_serve.py`): `preset <file>`, the bare `wait`
(until idle), `paper text`, `ti59 program 000 count=<n>`, `lcd`,
`registers`, `peek TMC0501 scom`. The calcu program: `--calcu PATH` on a
tool's command line, else `CALCU59_CALCU`, else
`build/tests/library/harness/cli/calcu` of the repository (`cmake --build
build/tests --target calcu`). `stepcost.py` and `fastmode_matrix.py` still
drive the headless machine of `tools/headless/` (the `core/` emulator from
Python via ctypes, with the helpers `regbytes`, `press`, `settle`, `idle`
there) until C59-9 (M-59); `tools/headless/` is for ROM-level traces.

## Running

From the repository root:

```bash
uv run python tools/agentic/<tool>.py <args>
```

- Paths inside the tools are resolved from their own location, so they also
  work from another directory.
- Build `calcu` after changes to `core/` or `app/`. The headless library
  (`stepcost.py`, `fastmode_matrix.py`) builds itself on first import
  (`tools/headless/build.sh`); after changes to `core/` or
  `tools/headless/capi.cpp`, rebuild it.
- In zsh, `uv` may be aliased with `noglob`, so `examples/calendar/calendar-0*.ti59`
  is passed on literally. Expand globs with `$(ls examples/calendar/calendar-0*.ti59)`.
- A full calendar year takes about 1–2 s of real time to emulate; the slow
  1978 programs take up to ~30 s.

## Time base

All times are **emulated seconds** (calcu's `ticks`: weighted CPU
instructions, an idle-loop instruction counting 4, at 455 kHz / 32 = 14219
a second). They match the published real-hardware times of the calendar
programs within a few seconds.

A run time is what a stopwatch on a filmed calculator shows: from the key
press until the display lights up again and no program runs (`dark_time` in
`calcu59.py`: `lcd` and fB[0] of `registers`, sampled every 10 ms of emulated
time with `wait 10ms`; the headless tool before M-54 sampled every 100
instructions, so a time moves by at most one sample). A Pause flash doesn't
end it, and the blinking of an error display after the stop doesn't move it.
A preset's `elapsed-time: start` / `elapsed-time: report` measures the same
interval, within a few hundredths of a second.

## The tools

| Tool | What it answers |
|---|---|
| `calcu59.py` | Does the preset run, how long from the start key, what did it print? Also the shared library for the other tools. |
| `linetimes.py` | When did each printer line appear? Which lines are slow? |
| `runtime.py` | How much program run time does each key cost, including preparation keys before the start key? |
| `hotsteps.py` | Which program steps are hot; is the printer the bottleneck; is fast mode on; what runs between two lines? |
| `stepcost.py` | What does one step (or a short sequence) cost in fast mode and in normal mode? |
| `fastmode_matrix.py` | Which instructions work in fast mode, and which stops hang the calculator? |
| `kdis.py` | What does the program in a `.ti59` file say, instruction by instruction? |
| `asm59.py` | Library: write a keycode program with mnemonics and labels instead of numbers. |
| `calendar/` | Tools for the calendar-printer programs (see below). |

### calcu59.py — load and run

```bash
uv run python tools/agentic/calcu59.py examples/calendar/calendar-09-dispatch.ti59 [--lines]
# examples/calendar/calendar-09-dispatch.ti59:   75.94 s  finished=True  lines=113 (non-blank 85)
```

Loads the preset without its `keys:` sections (its partition, program, data,
cue card and machine), presses its key steps but the last, each until idle,
and times the **last** key step until the display lights up again.
`--lines` appends the printout (`paper text`).

Library use (`sys.path.insert(0, 'tools/agentic')`):

- `split_preset(path)` → `(text without keys:, [(verb, value)])`
- `Run(program)`: one `calcu serve`, with `cmd`, `preset`, `step`,
  `wait_idle`, `seconds`, `lit`, `running`, `registers`, `scom0`, `paper`,
  `program`
- `run_preset(path, steps=None)` → `(seconds, lines, finished)`
- `dark_time(run, value)` → `(seconds, finished, ran)`: press one key step,
  time it until the display lights up again; `ran` = it ran a program
- `play_until_last(path, steps=None)` → `(Run, last key step)`

The machine is the preset's (its `slot-1`, its `printer`); sections after
its keys are loaded with the rest, before the keys.

### linetimes.py — time of each printed line

```bash
uv run python tools/agentic/linetimes.py file.ti59 [--lines] [--limit S]
# blank  n= 28 avg  0.31s  total    8.6s  min 0.08 max 2.92
# text   n= 24 avg  0.53s  total   12.8s  min 0.42 max 0.79
# week   n= 61 avg  0.89s  total   54.5s  min 0.89 max 1.01
# total 75.85
```

Seconds from the last key to each printed line. The summary groups the gaps
by line kind (`week` = a 20-character line of digits and spaces). `--lines`
lists `time gap |line|` for every line, which shows at once which lines carry
extra work (e.g. the first row of each month). `total` is the time of the
last printed line, a little before the display lights up again.

### runtime.py — program run time per key

```bash
uv run python tools/agentic/runtime.py file.ti59 [--keys "2 0 2 6 A ..."]
# A:0.17 STO:75.95  total 76.1 s
```

Times every key that runs a program (`fB[0]` set) until the display lights
up again, so a label key that does preparation work before the start key is
counted too.
**Use this when comparing two programs that split their work differently.**
Timing only from the start key can hide seconds of preparation: a version of
the calendar-09 program whose `A` key called ML-20 looked 4 s faster than
Acosta's from the start key, but was 1.5 s slower in total. `--keys`
replaces the preset's key steps (key IDs, one step each).

### hotsteps.py — where the time goes

```bash
uv run python tools/agentic/hotsteps.py file.ti59 [--until S]
# ...: 20s  printer busy 33% of samples, print-wait spin 0%, Prg Src {2: 2869}
# hottest program steps: 319:2.0%, 207:1.3%, ...
uv run python tools/agentic/hotsteps.py file.ti59 --window A B
# 049:2 050:11 051:29 ... 018:14 019:20 ...
```

The default mode samples the run every millisecond of emulated time
(`registers` and `peek TMC0501 scom 0 1` after each `wait 1ms`):
- **Printer busy:** when this is low, the program is limited by its own
  calculation, not by the printer.
- **Print-wait spin:** time in the ROM loops that wait for the printer.
- **Prg Src:** the Prg Src Flag values seen. 2, 4 or 6 means fast mode; 0 is
  normal mode.
- **Hottest steps:** the 25 program steps with the most samples.

`--window A B` lists the program steps executed between A and B seconds as
`step:samples` (one sample per millisecond). Take A and B
from `linetimes.py --lines` to see exactly what runs before a slow line.

### stepcost.py — cost of single steps

```bash
uv run python tools/agentic/stepcost.py                 # table of common steps
uv run python tools/agentic/stepcost.py "43 22" "73 00" # your own keycode sequences
# 43 22            fast    580  normal   1332  instructions  (41 / 94 ms)
```

Each sequence runs 30 times from step 008, once in fast mode (SST entry) and
once in normal mode, minus an empty program. Registers are preset (see the
docstring); choose register numbers in the cases accordingly. Useful values
from the table: RCL/STO ≈ 580, SUM/Exc ≈ 650–700, RC* ≈ 930, Op 05 (print a
line) ≈ 4500, a digit ≈ 120 instructions in fast mode. Normal mode costs about
twice as much.

### fastmode_matrix.py — what works in fast mode

```bash
uv run python tools/agentic/fastmode_matrix.py          # all cases, then the stop table
uv run python tools/agentic/fastmode_matrix.py label    # only cases whose name contains "label"
# SAME GTO nnn                          fast [0, 2]  normal [0]
# DIFF SBR nnn                          fast [0, 1]  normal [0]
# RCL 01 RST           HANGS 030A-030E  Prg Src 0  display ['']
```

Runs each instruction class once in fast mode (SST entry) and once in
normal mode, and compares X, the data registers, the printout and the
display. The brackets list the Prg Src Flag values seen while the program
ran: a 1 means the run went into the module. The stop table ends a fast
program with R/S or RST in various X states and reports whether the
calculator hangs. The results are summarized in `rom/fastmode.md`; the ROM
explanation is in `rom/TI59-ROM-Guide.md` § "What a fast-mode program can do".

### kdis.py — disassembler

```bash
uv run python tools/agentic/kdis.py file.ti59
# 000  R/S
# 001  Lbl A
# 003  STO 69
```

One line per instruction with its operands folded in (the preset's program
as calcu wrote it, read back with `ti59 program 000 count=<n>`). Then come
the key steps and the number of steps of the partition. `dis(keycodes)` is the library
form.

### asm59.py — assembler (library)

```python
from asm59 import assemble, digits
code, labels = assemble(['RCL', 4, ('L', 'LOOP'), 'Op', 20, 'Dsz', 1, ('A', 'LOOP'), 'R/S'])
```

Tokens:
- mnemonic strings (table `KC`) and raw keycode ints;
- `('L', name)` defines a label, `('A', name)` emits a 2-byte absolute
  address;
- `('D2', a, b)` emits the label distance a − b as two digit steps;
- `('N', text)` attaches a comment to the next step.

`assemble(tokens, listing={})` fills `listing` with step → (shown text,
notes) for writing an annotated `program:` section (see
`calendar/fastcal.py`). Add missing mnemonics to `KC` as needed.

## Calendar tools (`calendar/`)

For the 1978–1981 calendar-printer programs (`examples/calendar/calendar-0*.ti59`,
`examples/calendar/CALENDAR_PROGRAMS.md`).

| Tool | Use |
|---|---|
| `check.py printout.txt year` | Day rows against Python's `calendar` (Sunday first). Library: `check(lines, year)`. |
| `years.py file.ti59 year… [--no-acosta] [--write FOLDER]` | Runs the preset for each year (replaces the 4 digit key steps before the first step that names `A`), checks the dates, and compares line by line with Acosta's program for the same year; `--write` writes a copy of the preset per year. Library: `with_year(path, year)`, e.g. for `runtime.py`'s `key_runtimes`. |
| `compare.py file.ti59…` | Steps, time of the last printed line, and date check for each program with its own keys. |
| `fastcal.py out.ti59` | Generator of `examples/calendar/calendar-09-dispatch.ti59`, a preset of format 2 (program in the listing notation, registers, cue card, keys; header text from `calendar-09-header.txt`). Edit the generator, not the example; the CTest `CalcU59CalendarScripts` checks that it writes the tracked file. |
| `swapcal.py out.ti59` | Generator of the earlier swap design (correct but 92.6 s), kept for reference. |

Typical check after changing `fastcal.py`:

```bash
uv run python tools/agentic/calendar/fastcal.py examples/calendar/calendar-09-dispatch.ti59
uv run python tools/agentic/calendar/years.py examples/calendar/calendar-09-dispatch.ti59 1900 2000 2024 2026
uv run python tools/agentic/runtime.py examples/calendar/calendar-09-dispatch.ti59
```

## Pitfalls

- **Two numbering systems.** Key steps name keys by their key IDs
  (`docs/formats/preset.md` §8.1: `2`, `A`, `Pgm`); `program:` holds keycodes
  (`2` is 02, `Pgm` is 36). The headless helpers' `press()` still takes key
  matrix codes (row × 10 + column: `2` is 83, `A` is 11).
- **Program longer than the partition.** A `program:` section writes past
  the partition into the top registers, as the calculator keeps them. Check
  that the step count fits.
- **Op 2x / Op 3x** increment or decrement only R00–R09.
- **Values in `data:`** are written with 13 significant digits. For print
  codes, the position of the digits within the 13-digit mantissa matters (a
  2-digit vs 3-digit integer part shifts them).
- **Fast mode:** the rules (no SBR, no labels as targets, safe endings) are
  in `rom/fastmode.md`; `examples/fastmode-sst.ti59` is the SST entry.
- **Jumps in `stepcost.py` cases.** Each case is repeated 30 times in a
  row, so a jump to a fixed address lands inside the repetitions and can
  loop forever. `measure()` gives up after 300 emulated seconds with an
  error. Measure a jump with a target that moves along with each
  repetition instead.
- **Settle overhead.** Waiting for idle after each key costs time. Don't
  sum per-key times of `run_preset`; use `runtime.py`.
- **Headless requirements.** `stepcost.py` and `fastmode_matrix.py` need
  `h_elapsed` in `tools/headless/capi.cpp` and `Machine.elapsed()` /
  `INSTR_PER_SEC` in `tools/headless/ti59.py`.
- **Sections after the keys.** The tools load a preset without its `keys:`
  sections and press the keys themselves, so a section after a `keys:`
  section (a reload, `preset.md` §4) is loaded before the keys. Magnetic cards
  in presets come with C59-7 (M-55).
