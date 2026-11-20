# The harness with Calc-U-59

How to run, trace, time and profile TI-59, TI-58 and TI-58C programs without
a window: the family's harness `calcu` with the product commands under the
product word `c59`. The common harness (the reply forms, `--json`, the
options, the exit status, `serve` and the other transports, the library's
commands) is in [`docs/formats/harness.md`](../../../docs/formats/harness.md);
this file holds what is Calc-U-59's own.

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
  which the scripts below use. `--calcu PATH`, `CALCU`, else the
  `build/tests`, `build/all` or `build/tools` tree's (the library's
  `calcu_serve.find_calcu`), else `build/tools` built on first use.
- `calcu --help` lists every command; the IDE run configuration is
  `.run/calcu-59.run.xml` (repository root).

## The common commands on the TI models

| What | Calc-U-59's part |
|---|---|
| Start | The startup model (the TI-59 under the default settings) with its startup preset or the built-in default: the Master Library module (`ML`) in `slot-1`, no printer, at key readiness |
| Models | `model TI-59`, `model TI-58`, `model TI-58C` |
| Places | `slot-1` (a Solid State ROM module by its ID: `attach slot-1 ML`), `printer` (`attach printer PC-100C`); the media place `card-reader` on the TI-59 |
| Keys | `key`, `key-down`, `key-up` take the key IDs of [`docs/formats/preset.md`](../../../docs/formats/preset.md) §8.1 (`2nd Pgm`, `x^2`, `RTN`); a TI model has no `type` |
| Presets | `preset <file>\|examples:<path>`: format 2 only (`.ti59`, `.ti58`, `.ti58c`); the product's sections and rules are `app/TIPresetKeys`, `app/TIPresetRules` |
| `open <path>` | A preset by its extension or its `model:` line; an `.asm` file as the ASM overlay; a `.keystrokes.yaml` keystroke program file written into the running machine from step 000. Refused: `.rom.yaml` (`… was not opened: this release takes a Solid State ROM module by its ID only.`) and `ti58c.memory.yaml` (`… was not opened: it is Calc-U-59's own constant memory file.`) |
| `screenshot <name>` | The classic segment picture, into `--output-folder` |

## Time base

All times are **emulated seconds**: ticks of the model's clock (`ticks`,
`wait`). One instruction is 32 ticks; an instruction in the ROM's idle state
weighs four on the TI-59 and TI-58, one on the TI-58C. Ticks ÷ 32 are
"weighted instructions" (14219 a second on the TI-59 at 455 000 ticks per
second; the TI-58C runs at 394 000). The times match the published
real-hardware times of the calendar programs within a few seconds.

A **run time** is what a stopwatch on a filmed calculator shows: from the
key press until the display is lit again and no program runs (`c59
run-time`). A Pause flash does not end it; the blinking of an error display
after the stop does not move it. A preset's `elapsed-time: start` / `report`
measures the same interval within a few hundredths of a second.

## The `c59` commands

All are product commands under `c59`, on the TI-59, TI-58 and TI-58C. Each
has a plain reply (`done`, or `done <n>` and n lines) and a `--json` reply
whose `result` is the object named below. Without a machine every command is
refused (`There is no machine.`, or `No machine is running.` for the
program-memory commands); a wrong word count gives the command's
`usage: …` line.

### Units and reply rules

| What | Written as |
|---|---|
| A time | Seconds from ticks and the model's tick rate, nine decimals, truncated (`12.345678901`); a JSON number with the same digits. No floating point reaches a reply, so two runs agree byte for byte |
| An instruction count | Ticks ÷ 32, on every model |
| A share | Per thousand of the notes, written as a percent with one decimal (`12.3%`); a JSON string |
| A program step | Three digits (`007`); a JSON integer |
| A keycode | Two digits (`43`) |
| A duration argument | A number with a unit, `200s`, `1.5s`, `500ms` (the preset format's durations) |

The timing commands (`line-times`, `run-time`, `hot-steps`, `step-trace`,
`step-cost`) run the machine in slices of emulated time, as `wait` does, and
read the run probe between slices; the parts of the session (display,
paper, topics) follow once at the end of the command.

### The run probe

What the timing commands read between two slices (`app/TIRunProbe`):

| Field | Read from |
|---|---|
| Program step | SCOM register 0: digits 7, 6, 5 the register (BCD), digit 4 the byte; step = register × 8 + byte |
| Program source | SCOM register 0, digit 3 (the Prg Src flag) with the program counter: memory (user program; fast mode counted apart too), ROM, module |
| Printer busy | Bit `0x8000` of the processor flags |
| Waiting for the printer | The program counter at one of the ROM's print-wait loops (`154C`, `154D`, `1425`–`1427`, `17C1`–`17C3`, `15E4`, `15E5`, `1612`, `1613`); never on the TI-58C |
| A program runs | Bit 0 of `fB` |
| Display lit | A digit or point driven in the last revolution of the digit counter |
| Processor idle | No program, the idle flag (bit `0x0001` of the flags) set, the program counter in the model's idle and key-detect range |
| Finished | The program has ended and the machine waits for the user (`reference/Idle.md`): the scan loop with fB[0] clear; the printer has drained before the scan loop |

### Timing and profiling

| Command | Arguments | Plain reply | JSON `result` | Refusals |
|---|---|---|---|---|
| `c59 run-time <key>…` | One or more key IDs, each pressed as `key` presses it (one word, one key) | One line per key, `<key> <s>` (`not finished` appended to a key not finished), then `total <s>` | `{"keys":[{"key","seconds","finished"}],"total"}` | No key: usage; a key that `key` refuses: its reason |
| `c59 line-times [limit=<d>]` | `limit=` the longest run, default `4000s` | One line per printed line, `<time> <gap> \|<text>\|`; `no lines printed` when none; `feeds: n=<n> mean <s> min <s> max <s> sum <s>` (or `feeds: n=0`); `lines: …` the same for the other lines; `printer floor: <s>`; `limit: yes\|no` | `{"lines":[{"time","gap","text"}],"feeds":{"n","mean","min","max","sum"},"other":{…},"floor","limited"}` (`mean`…`sum` only when `n` > 0) | `There is no PC-100C: attach printer PC-100C first.`; a word other than `limit=`: `usage: limit=<duration>`; a bad duration: its reason |
| `c59 hot-steps [limit=<d>]` | `limit=` the run's length, default `200s` | `notes: <n>`, `printer busy <p>`, `printer wait <p>`, `memory <n> (fast mode <n>)`, `rom <n>`, `module <n>`, then up to 25 lines `<step>: <p>` | `{"notes","printerBusy","printerWait","sources":{"memory","fastMode","rom","module"},"steps":[{"step","share"}]}` | As `line-times`' limit |
| `c59 step-trace <from> <to>` | Two durations from the start of the command | One line `<step>:<count> …`, or no line when nothing was noted | `{"steps":[{"step","count"}]}` | Not two words: usage; a bad duration: its reason; `The second time must come after the first.` |
| `c59 step-cost <keycode>…` | Two-digit keycodes, the sequence to measure | `fast <n> instructions <ms> ms`, `normal <n> instructions <ms> ms`, `printer: yes\|no` | `{"fast":{"instructions","ms"},"normal":{"instructions","ms"},"printer"}` | No keycode: usage with an example; `A keycode is two digits, 00 to 99: <word>.`; `A program runs: let it end first (`wait`).`; `The program does not fit into the memory.`; `The entry keys were refused.`; `The machine stopped running.`; `The program still runs after 300 s: a jump that loops back?` |
| `c59 coverage [all]` | `all`: keep the idle and key-detect range | The ROM addresses 0000–17FF carried out since `profile on`, four hexadecimal digits, sixteen to a line, then `reached: <n> of 6144` | `{"addresses":["0000",…],"reached","of":6144}` | Another word: usage; `Profile is off: send `profile on` first.` |

The rules of each:

- **`run-time`**: the machine runs in slices of 100 instructions from the
  press until the display is lit and no program has run for a quarter of a
  second; the time is from the press until the display was lit again, 0 for
  a key that started no program. A key not finished after 4000 s is marked
  and the command ends there, done.
- **`line-times`**: slices of 200 instructions until the machine has been
  finished for 15 s with no line printed in between, or the limit. A line's
  time and gap are from the start of the command and from the line before.
  A line of spaces only is a feed. The printer floor is the least time the
  PC-100C needs for that printout: 4165 instructions per printed line, 1370
  per feed.
- **`hot-steps`**: one note every 97 instructions for the limit; the steps
  noted most often first, equal shares in step order. Without a PC-100C
  both printer shares are `0.0%`; on the TI-58C `printer wait` is always
  `0.0%`.
- **`step-trace`**: runs to `<to>`; from `<from>` on notes the program step
  every 20 instructions and folds equal neighbours into `<step>:<count>`.
- **`step-cost`**: on the session's machine, which it leaves changed. Writes
  from step 000 the program `91 00 00 00 00 00 00 86`, the sequence 30
  times, `25 81`, and sets R00 = 10, R01 = 5, R04 = 1000, R10–R12 =
  2.000311000312, R22 = 98.99 (those the model has). Runs it once entered
  in fast mode by the SST method and once in normal mode by `GTO 008 R/S`,
  each from the last entry key until the processor has been idle for 20
  slices of 25 instructions in a row; then the same without the sequence.
  Cost = (with − without) ÷ 30, rounded, in instructions and in ms. The
  cost of a step that prints depends on a PC-100C, which `printer:` names.

### Program memory

| Command | Arguments | Plain reply | Refusals |
|---|---|---|---|
| `c59 partition` | — | One line, the last program step of the partition (`479`) | — |
| `c59 partition <value>` | The value as a preset's `partition:` reads it: the last program step, `479` or `479.59`; rounded up to a whole number of 80 steps | `done` | `partition: is the number of the last program step (479, or 479.59).`; `partition: <n> is beyond the last program step of this model, <last>.`; busy |
| `c59 program [<step>] <keycode>…` | `<step>`: three or more digits, default 000; the keycodes in the compact notation of a preset's `keycodes:` (two digits each, on one line) | `done` | `The step is a number of three digits (000 to <last>).`; `The step <n> is beyond the last step, <last>.`; `The keycodes are two-digit numbers on one line.` (a `#` or line break); `program: <reason>`; busy |
| `c59 program <step> count=<n>` | `<n>` 1 to 9999 | n lines `NNN KK` (step, keycode), stopping at the model's last step | `count= is a number of steps, at least 1.`; a step refusal as above |
| `c59 data-registers <NN>=<value>…` | `NN` 00 to the model's last register, `H00`–`H03` on the TI-58C; the value a decimal number, an exponent allowed (`-1.5e-3`) | `done` (all or none written) | `data: <key> is no register; a register is NN (00 to 99)[ or H00 to H03].`; `data: <key>: the <model> has the registers 00 to <n>.`; `data: <key>: only the TI-58C has the registers H00 to H03.`; `data: <key>: the TI-58C's extra registers are H00 to H03.`; `data: <key>: the value is no number (a decimal number, an exponent allowed: -1.5e-3).`; busy |
| `c59 data-registers <NN>…` | Register names as above | One line per register, `NN: <value>` | The register refusals |

- A value line is written as a preset's `data:` writes it: the mantissa's
  significant digits (13 at most) and the exponent, `1.5e0`, `-2.000311e-3`,
  `0` for zero.
- Writing and reading in one command is refused: `Either write registers
  (NN=value) or read them (NN), not both.`
- **Busy**: while an operation runs, the writes (`partition <value>`,
  `program … <keycode>…`, `data-registers <NN>=<value>…`) are refused with
  `Another operation is running.`; the reads are not.
- `--json`: `result` is the array of the reply lines, `null` for a write.
- `c59 program` takes keycodes (`2` is 02, `Pgm` is 36); `key` and a
  preset's key steps take key IDs.

### Listings and assemblies

| Command | Plain reply | JSON `result` |
|---|---|---|
| `c59 keycodes list [<first> [<last>]]` | One line per step as the PC-100C's `List` prints it: `010 43 RCL`; an operand step shows its two digits as the mnemonic; keycode 65 is `x` | `{"steps":[{"step","keycode","mnemonic"}]}` |
| `c59 keycodes disassemble [<first> [<last>]]` | One line per instruction: `007  Dsz Ind 03 A` | `{"lines":[{"step","text"}]}` |
| `c59 keycodes assemble <file>` | `steps: <first>-<last>` (three digits each, a hyphen) | `{"first","last"}` |

- **Range**: `<first>` default 000; `<last>` default the last step that is
  not `00` within the partition, so an empty program gives no lines. Each
  is three or four digits up to the model's last step (959 on the TI-59,
  479 on the TI-58 and TI-58C); `<last>` not before `<first>`. Refusals:
  `A step is three digits, 000 to <last>: <word>.`, `The last step is three
  digits, from the first to <last>: <word>.`. The grammar is read from step
  000, so `<first>` falls on an instruction as the calculator reads the
  program. No printer is needed.
- A listing is the listing notation of a preset's `keycodes:`; it can be
  pasted into `keycodes:` or a keystroke program file as it is.

#### The assembly grammar

A line: the step (three digits, or more), two spaces when written, the
mnemonic, and its operands each after one space.

| Element | Form |
|---|---|
| Mnemonic | The key ID of [`docs/formats/preset.md`](../../../docs/formats/preset.md) §8.1 (`Lbl`, `STF`, `IFF`, `Dsz`, `Ind`, `x⇄t`); keycode 20 is `2nd CLR`, 92 `RTN`, 82 `HIR`; 26 and 27, which no key enters, their two digits |
| Merged code | Its key and `Ind`: `STO Ind 05` for `72 05` (62 `Pgm Ind`, 63 `Exc Ind`, 64 `Prd Ind`, 72 `STO Ind`, 73 `RCL Ind`, 74 `SUM Ind`, 83 `GTO Ind`, 84 `Op Ind`) |
| Register, flag, program or digit number | Two digits; `Ind NN` when indirect (`Pgm`, `STO`, `RCL`, `SUM`, `Exc`, `Prd`, `Fix`, `Op`, `HIR`, `STF`) |
| Label (after `Lbl`) | The label key's mnemonic |
| Destination (`GTO`, `SBR`, `x=t`, `x≥t`) | A step address in three digits (stored as two steps, first byte 00–09), `Ind NN`, or a label as its mnemonic |
| Number then destination (`IFF`, `Dsz`) | Both: `Dsz Ind 03 A`, `IFF 01 123` |
| `INV SBR` | No operand |
| Operand past the end of memory | `?` (disassemble only) |

Reading back (`assemble`):

- Blank lines and everything after `#` are passed over.
- Each line is written from the step it names; the file is written all or
  nothing, and `disassemble` assembled again gives the same program.
- A mnemonic is any name `key` takes for that key (the key IDs, their
  "Also" names, the printer names of §8.1.1), in any case; `2nd <key>` for a
  second function; `<key> Ind` is merged where a merged code exists; `HIR`
  and two-digit `26`/`27` as above.
- Refused, nothing written, `Line <n>: <reason>`:

| Reason | When |
|---|---|
| `A line starts with its step, three digits or more: <word>.` | No step first |
| `The step has no mnemonic.` | A step alone |
| `Unknown mnemonic: <word>.` | No key takes that name |
| `<mnemonic> needs its operand.` | An operand missing |
| `<mnemonic>: not an operand: <word>.` | An operand not two digits, not a three-digit step, not a label |
| `More than the instruction takes: <word>.` | Words left on the line |
| `Beyond the last step, <last>.` | The instruction reaches past the model's last step |
| `Step <nnn> is written twice.` | Two lines cover one step |

Other refusals: `usage: c59 keycodes assemble <file>`, `The file <path>
cannot be read.`, `The file holds no instruction.`

### Debug panel

| Command | Reply |
|---|---|
| `c59 asm load <file>` | `<n> instructions` (the ASM overlay; format in `examples/assembly/README.md`) |
| `c59 asm run` | The overlay's status line (entered at 1800) |
| `c59 asm clear` | The overlay taken away |
| `c59 dump scom\|memory\|prog\|vars` | The lines of the dump (also into the log) |
| `c59 log-level off\|info\|debug` | The level of the machine's own log lines |
| `c59 heatmap reset` | The ROM heatmap's counts set to zero |

`state c59.calculator-debug|c59.program-listing|c59.cpu-debug|c59.heatmap|c59.asm-overlay|c59.debug-view`
reads what the panel shows (`app/TIDebugPart`).

### Machine state

`state-save <file>` and `state-restore <file>` cover the TMC0501 (the
processor frame, flags, program counter and stack, SCOM, HOLD and card
switch, tick count), the RAM (the TI-58C's constant memory), the PC-100C's
mechanism and line buffers, the module's ID and address latches and a card
in the reader (its bytes, position in the pass and mode)
(`core/TIMachineState.*`, `core/TIStateRecords.hpp`); the paper, the media
part's bindings and the debugger's state are the session's. A file of
another product, model or set of peripherals is refused.

## Recipes

**A ROM trace of a key sequence:**

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

**The cost of a step:** `c59 step-cost 43 22`. Reference figures (fast /
normal instructions, a fresh TI-59 with a PC-100C each run): Nop 144 / 225,
a digit 119 / 254, x⇄t 205 / 514, RCL 22 580 / 1332, STO 30 582 / 1208,
SUM 30 693 / 1318, Exc 30 643 / 1269, RC* 00 931 / 1730, HIR 05 548 / 1189,
HIR 36 657 / 1303, INV Int 392 / 758, Op 20 832 / 1463, Op 05 (print)
4539 / 4566, `+ 1 =` 874 / 1634. The command gives a few instructions more
(584 / 1333 for RCL 22 with a PC-100C, 549 / 1253 without): it runs on the
session's machine.

**ROM coverage by scenario:** `tools/coverage.py build [name…]` runs the
scenarios on one `calcu serve` (each from a cold boot) into
`tools/build/coverage.json`; `show 0400 04FF`, `unique sqrt`, `gaps` read
it; `--variant TI58C`. A scenario's words leave out the key-detect range
0627-0660 but keep the release gate 0627-062A; `boot` is read with `all`.

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
  ID too.
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
