# C59-9 Harness product commands of Calc-U-59: design
Status: approved by the user at the Part D/E boundary (2026-11-16)

Paths without a prefix are under `products/calc-u-59/`; line numbers at
`7bdd51c`. The spec cites the tools at `v2.3.0`; they are the same files
here (`tools/headless/`, `tools/agentic/`), moved onto the machine
interface in C1 (C59-1 T14) and onto the library's ROM lookup (M-24).

## 1. Scope

| Row | Step | Tests from (plan §3) | Done in (plan §5.1) |
|---|---|---|---|
| C59-9 | M-59 (with LIB-11 59) | AC-1–11, AC-13–15; EC-2–6 (AC-12 withdrawn; there is no EC-1) | M-59 |

M-59 does three things, in this order, as separate commits on short
branches (ADR 0029), as M-51 did for Calc-U-1600:
1. the `c59` commands of this row (this design, §3);
2. the harness's Calc-U-59 entry through the app layer, `display-text` and `wait`
   checked against today's tools (LIB-11 AC-16, AC-17), and today's
   tools removed, the scripts that stay moved onto the harness (LIB-11
   design §11; §4 here has the table of every function);
3. the adapter check of plan §1 over Calc-U-59 and the library (LIB-11
   §11.5).

The commands of part 1 are written against the entry of part 2 (the app
layer's parts in the run), so both are merged together.

What M-59 finds, taken as done by the earlier designs (an earlier step
that left it otherwise is adapted to and named in the qa-report):
- M-53 (LIB-10 59, LIB-8 59): the debug target `TMC0501` (LIB-10
  AC-28) with the registers of the processor frame and the address
  spaces ROM, RAM, SCOM and program steps; `profile`, `history`,
  `disassemble`, `trace`, `registers`, `peek`, `poke`; the PC-100C's
  paper on the paper output kind (`paper text`, `paper marks`).
- M-54 (C59-2, LIB-6 59, LIB-18 59): the keystroke program memory kind
  and `c59 partition`, `c59 program`, `c59 data-registers` (C59-2 AC-9);
  `preset`, `check-preset`, `model`, `attach`, `detach`, `reset`,
  `cold-boot`, `power-cycle` on every TI model; `tools/agentic/` loading
  presets through `calcu`, `ti59file.py` gone; the calendar scripts on
  version 2 (C59-2 AC-18).
- M-55 (LIB-7 59, C59-7): the card reader as a media place; `card`,
  `arm`, `saveas { card: … }`, `media`; `tools/headless/capi.cpp`'s card
  calls on the place (LIB-7 §11.7).
- M-56 (C59-4): the ASM overlay and its `c59` commands, the dumps.
- LIB-5 (M-36): `key` with the names of `preset.md` §8.1 on the TI
  models.

Not here: the commands of other features that serve a function of the
tools (they are mapped in §4, not designed); the calendar scripts and the
fast-mode matrix themselves (AC-13: they stay scripts).

## 2. Modules and files

| File | Holds |
|---|---|
| `app/TIHarnessCommands.hpp/.cpp` (new) | `cu59::addTIHarnessCommands(calcu::harness::CommandRegistry&)`: the commands of §3 under `c59` (`CommandRegistry::addProduct`), next to those of C59-2 and C59-4 |
| `app/TIMnemonics.hpp/.cpp` (new) | one table keycode → key name (`preset.md` §8.1, the names `key` takes, LIB-5 AC-26) and the keycode grammar (operands, addresses, labels, `Ind`) for the assembly (§3.6; AC-10, AC-11, AC-14; Technical Requirements) |
| `app/TIRunProbe.hpp/.cpp` (new) | the run facts the timing commands read through capabilities: the program step, the program source, printer busy, the print-wait loops, a program running, the display lit, idle (§3.2) |
| `core/TIMachineState.hpp/.cpp` (new) | the generic kind `machine-state` (`library/harness/MachineState.hpp`) on the TI machines (§3.7; AC-15) |
| `core/` (TMC0501, RAM, PC-100C, module, card reader) | `saveState`/`restoreState` per part that holds state (§3.7) |
| `app/KeyNames.hpp/.cpp` | unchanged table (the PC-100C's printer names, which the listing of §3.5 and the debug panel show); `stepsAfter` and `argumentSteps` are the keycode grammar §3.6 builds on |
| `app/HarnessProduct.cpp` | `addCommands()` calls `addTIHarnessCommands` (and C59-2's, C59-4's) |
| `tests/harness_product_commands_tests.cpp` (new) | §6, T1 to T9 |
| `tests/machine_state_tests.cpp` (new) | §6, T10 |

## 3. The commands

All are product commands under `c59` (AC-1; `harness.md` H-2), on the
TI-59, TI-58 and TI-58C. A command that needs what the machine lacks
refuses with a sentence that says so. Every handler reaches the machine
through capabilities (the debug target of LIB-10, the keystroke program
memory of C59-2, the paper output of LIB-8, the PC-100C's controls of
C59-6) and names no machine class (C59-1 AC-17; the CTest
`CalcU59CoreSeam`).

**Units and replies.** A time is written in seconds from ticks and the
model's rate, with LIB-11 §3.1's exact `Decimal` (nine decimals,
truncated), and, where the spec says so, in instructions (ticks ÷ 32,
also on the TI-58C, EC-5). A share is per thousand of the notes, written
as a percent with one decimal from integers. No `double` reaches a reply
(LIB-11 AC-6). Every reply has a plain form (lines) and a `--json` form
(LIB-11 AC-4); the JSON keys are named in each section.

**Running inside a command.** The timing commands run the machine
through `HarnessHost::advance` in slices (LIB-3 `advanceUnpaced`), the
same emulated time a `wait` passes; they read the machine between
slices. A slice is the spec's sampling distance (97 or 20
instructions) or, where the spec names none, today's tool's (200
instructions for `line-times`, `linetimes.py:23`; 100 for `run-time`,
`ti59file.py:114`) (D-1).

### 3.1 `c59 coverage [all]` (AC-2)

Refused unless `profile` is on ("Profile is off: send `profile on`
first."). Reads the debug target's profile (`debugger::Profile`, every
entry of target `TMC0501`, address order; `Profile::top(distinct())`
gives all), keeps the addresses 0000 to 17FF of the calculator's ROM,
and leaves out those the ROM is at while the machine reports key
readiness: the model's key-detect range (`idleAndKeyDetectRange`,
`core/TIAdapters.hpp:149-151`, which stays as a product function, D-7)
unless `all` follows. Plain: the addresses as four-digit hexadecimal
numbers, sixteen to a line, then `reached: <n> of 6144`. JSON:
`{"addresses":["0000",…],"reached":n,"of":6144}`. Today:
`tools/headless/coverage.py:217-261`.

### 3.2 The run probe (AC-3 to AC-7)

`TIRunProbe` reads, between slices, through the debug target's registers
and SCOM space (the names are LIB-10 59's):
- **the program step**: SCOM register 0, digits 7, 6, 5 the register (in
  BCD) and digit 4 the byte: step = register × 8 + byte (today
  `tools/agentic/hotsteps.py:25-26`; memory `preg_rtn_bug_fix`);
- **the program source**: SCOM register 0, digit 3, read with
  `programSourceFrom` (`app/ModuleImage.hpp:19-37`) into memory (user
  program, fast mode included), ROM and module;
- **printer busy**: the busy bit of the processor flags (today
  `FLG_BUSY = 0x8000`, `tools/agentic/ti59file.py:22`);
- **waiting for the printer**: the processor at one of the ROM's
  print-wait loops (today the list `hotsteps.py:22`, which becomes the
  product's table `kPrintWaitLoops` in `app/TIRunProbe.cpp`; addresses of
  the TI-59's ROM; on the TI-58C the shares are given as zero, D-6);
- **a program runs**: bit 0 of `fB` (today `ti59file.py:106`);
- **the display lit**: any position of the display output brighter than
  zero since the last look (today `ti59file.py:109-111`);
- **idle**: the machine is idle as LIB-6 AC-36 says (key readiness, no
  program, printer not busy).

### 3.3 Timings

| Command | Design |
|---|---|
| `c59 line-times [limit=<duration>]` (AC-3, AC-4, EC-2, EC-6) | Refused without a PC-100C plugged in (EC-6). Runs in slices of 200 instructions until the machine has been idle for 15 s without a printed line since the last, or the limit (default `4000s`). After each slice the new lines of the paper (LIB-8's paper output, its text layer) are noted with the time from the start of the command. Plain: one line per printed line, `<time> <gap> |<text>|`; then `feeds: n=<n> mean <s> min <s> max <s> sum <s>` and `lines: …` for the others; `printer floor: <s>` (AC-4: 4165 instructions per printed line and 1370 per feed, `PC100C::kLineInstructions`, `kFeedInstructions`, `core/PC100C.hpp:26-27`); `limit: yes|no`. Nothing printed (EC-2): `no lines printed`, after the first 15 s of idle. JSON: `{"lines":[{"time","gap","text"}],"feeds":{…},"other":{…},"floor","limited"}` |
| `c59 run-time <key>…` (AC-5) | Each key is pressed as `key` presses it (LIB-5 AC-15: the hold of the TI profile, 2nd before a second function). From its press the machine runs in slices of 100 instructions until the display is lit again and no program runs (the run probe; today `dark_time`, `ti59file.py:114-143`): its time, or 0 when it started no program. A key not finished after 4000 s is marked `not finished` and the command ends there, done. Plain: `<key> <s>` per key, then `total <s>`. JSON: `{"keys":[{"key","seconds","finished"}],"total"}` |
| `c59 hot-steps [limit=<duration>]` (AC-6, EC-6) | Runs for the limit (default `200s`), notes the run probe every 97 instructions. Plain: `notes: <n>`; `printer busy <p>%`, `printer wait <p>%` (zero without a PC-100C, EC-6); `memory <n> (fast mode <n>)`, `rom <n>`, `module <n>` (D-2); then the 25 steps noted most often, `<step>: <p>%`, the most first, equal shares by step. JSON: `{"notes","printerBusy","printerWait","sources":{"memory","fastMode","rom","module"},"steps":[{"step","share"}]}` |
| `c59 step-trace <from> <to>` (AC-7) | Two durations from the start of the command, `to` after `from` (else refused). Runs to `to`; from `from` on notes the program step every 20 instructions and folds equal neighbours. Plain: one line, `<step>:<count> …`; JSON: `{"steps":[{"step","count"}]}` |
| `c59 step-cost <keycode>…` (AC-8, EC-3) | The keycodes as C59-2 AC-9 writes them. On the session's machine, which it leaves changed (spec decision log): writes the program `91 00 00 00 00 00 00 86` + the sequence 30 times + `25 81` from step 000 and the registers of 2.3.0's tool (R00 = 10, R01 = 5, R04 = 1000, R10–R12 = 2.000311000312, R22 = 98.99; `stepcost.py:10-11, 21`) through C59-2's memory kind; runs it once entered in fast mode by the SST method (the keys of `stepcost.py:18`) and once in normal mode by `GTO 008 R/S` (`:19`), each from the last key until the processor is idle 20 slices of 25 instructions in a row (`:29-51`); the same for the program without the sequence; cost = (with − without) ÷ 30. A run past 300 s of emulated time fails the command ("The program still runs after 300 s: a jump that loops back?", EC-3). Plain: `fast <n> instructions <ms> ms`, `normal <n> instructions <ms> ms`. JSON: `{"fast":{"instructions","ms"},"normal":{…}}`. Without keycodes: refused with the usage (D-3) |

The machine's state between the runs of `step-cost` is the one the
previous run left (spec: the session's machine); 2.3.0's tool built a
fresh TI-59 with a PC-100C for each run (`stepcost.py:37`). The cost of
a step that prints (`Op 05`) depends on whether a PC-100C is plugged in;
the reply says which (`printer: yes|no`).

### 3.4 Listings: `c59 keycodes list [<first> [<last>]]` (AC-9, EC-4)

The program memory from `first` (default 000) to `last` (default the
last step of the partition), through C59-2's memory kind, one line per
step: the step in three digits, the keycode in two, the mnemonic as the
PC-100C's `List` prints it (`app/KeyNames.hpp`'s `mnemonic`, the
printer's names, e.g. `010 43 RCL`; an operand step shows its digits as
the printer does). It is `preset.md` §4.1's listing notation, so it can
be pasted into `keycodes:` or a `.keystrokes.yaml` as it is. An empty
program (all steps `00` from `first`): trailing steps of `00` after the
last step that is not are left out, so an empty program gives no lines
(EC-4; D-4). No printer is needed. JSON: `{"steps":[{"step","keycode","mnemonic"}]}`.

### 3.5 Assemblies: `c59 keycodes disassemble [<first> [<last>]]` (AC-14)

One line per instruction: the step in three digits, two spaces, the
mnemonic, and after one space each operand: a register, a flag or a
program number in two digits, a step address in three, a label as its
mnemonic, each after `Ind` when indirect; an instruction with two
operands (`Dsz`, `IFF`) has both (AC-14's example: `007  Dsz Ind 03 A`).
The grammar is `KeyNames`' (`stepsAfter`, `argumentSteps`): a branch's
first operand byte 00–09 is a two-step address, 40 is `Ind` and a
register, anything else a one-step label. A merged code (62, 63, 64, 72,
73, 74, 83, 84; `preset.md` §8.1.1) is written as its key and `Ind`
(`STO Ind 05` for `72 05`), as LRN shows it pressed (D-5). The
mnemonics are the key IDs of §8.1 (`Lbl`, `STF`, `IFF`, `Dsz`, `Ind`,
`x⇄t`; AC-11), from `TIMnemonics`, the one table `key` resolves names
with. Trailing `00` steps as in §3.4. JSON: `{"lines":[{"step","text"}]}`.

### 3.6 `c59 keycodes assemble <file>` (AC-10, AC-11)

Reads a file in the form of §3.5, one instruction per line: the step
(three digits or more), the mnemonic and its operands; blank lines and
`#` comments are passed over. Each line is written from the step it
names. A mnemonic is any name `key` takes for that key (AC-11: the IDs
and the "Also" names of §8.1, the printer names of §8.1.1, any case);
`<key> Ind` is merged where a merged code exists (D-5). Refused, with
the line, and nothing written: an unknown mnemonic, an operand missing
or out of range, an instruction that reaches beyond the model's last
step (959 on the TI-59, 479 on the TI-58/58C), two lines on one step.
`disassemble` assembled again gives the same program (round trip, AC-10);
`c59 program` (C59-2 AC-9) reads and writes the same memory as bare
keycodes. Reply: `steps: <first>–<last>`.

### 3.7 Machine state (AC-15)

The TI machines offer LIB-11's generic kind `machine-state`
(`library/harness/MachineState.hpp`): `save()` gives bytes, `restore()`
takes them or refuses. The form is product-internal (LIB-11 AC-24):
tagged, length-prefixed records, little-endian, one per part, in the
machine's order:
- the TMC0501: every register of the processor frame, the flags, the
  program counter and its stack, the SCOM (16 × 16 digits), the HOLD and
  card-switch state, the tick count;
- the RAM (the model's registers; on the TI-58C this is its constant
  memory, C59-3);
- the PC-100C, when plugged: its mechanism and its line buffers (the
  paper itself is LIB-8's output, not machine state);
- the Solid State ROM module, when plugged: its ID and the module's
  address latches (the ROM bytes come from the ROM set);
- the card reader: the card in it, if any, with the side's bytes, its
  position in the pass and its mode.

The library writes the header (product, version, model, variant,
plugged peripherals) and refuses a file that does not match (LIB-11
AC-24). After a restore, a card in the reader whose pass ends is given
back to the media part like any card; a written bank goes to the card
fitted then, and when none is fitted it is dropped with a log line
(D-8).

## 4. Today's tools and what replaces each function (LIB-11 AC-14)

Every function of today's Calc-U-59 tools, with the command (or the
script on the harness) that serves it after M-59, and the feature that
delivers it. `lib11_ac14_every_function_of_the_table` (LIB-11 §11.6)
runs one block per row. Paths are under `tools/` at `7bdd51c`.

**`headless/` (`capi.cpp`, `ti59.py`)**

| Today | Function | After M-59 (`calcu --product c59 …`) | Feature |
|---|---|---|---|
| `h_new`, `h_load_model_roms` (`capi.cpp:118-139`); `Machine()` | run without a window, the ROMs through the lookup | the one-shot form or `serve` | LIB-11 |
| `ti59.py` as a library (`from ti59 import Machine`) | drive step by step from a script | `serve` over a pipe; `library/harness/python/calcu_serve.py` | LIB-11 |
| `h_error`, `RomsMissing` (`ti59.py:174`) | the ROMs missing, named | the end of the run at its start, naming the files | LIB-11 EC-1 |
| `--variant TI58C`, `h_new(model)` | the model | `model TI-58C` | LIB-18 |
| `--module ML`, `h_load_module` (`capi.cpp:148-157`) | a module in the slot | `attach slot-1 ML` (the power cycle follows, LIB-18 AC-21) | LIB-18 |
| `--printer`, `h_printer` (`:173-177`) | with or without printer | `attach printer PC-100C`, `detach printer` | LIB-18 |
| `h_reset` (`:160`) | a power cycle | `power-cycle`; `reset`, `cold-boot` | LIB-18 |
| `h_key_down`, `h_key_up` (`:163-170`) | press and release a key by name | `key-down`, `key-up` | LIB-11 |
| `Machine.key`, `keys`, `SECOND` (`ti59.py:257-273, 55-66`) | type keys by name, second functions included | `key <key ID>…` | LIB-5 |
| `h_trace_sw`, `h_printer_print`, `h_printer_adv` (`:178-180`) | the PC-100C's TRACE, PRINT and ADV | `set-control <printer> TRACE on|off`, `set-control <printer> PRINT on|off`, `… ADV …` (the PC-100C's controls) | LIB-6 (`set-control`), C59-6 AC-5 |
| `h_stepn`, `run_until_idle` (`:183-186`; `ti59.py:275-287`) | let instructions pass; run until idle | `wait <duration>` (whole instructions, LIB-11 AC-9); `wait` without a duration (until idle, LIB-6 AC-34); `step { level: cpu, count: n }` on a paused machine for an exact count | LIB-11, LIB-6, LIB-10 |
| `h_elapsed` (`:190`), `INSTR_PER_SEC` | the emulated time, in weighted instructions | `ticks` (ticks ÷ 32 are today's instructions, LIB-11 AC-17) | LIB-11 |
| `h_record`, `Machine.record`, `format` (`:194-203`; `ti59.py:215-239, 352-389`) | a record per instruction with the registers it changed; the idle loop folded; ROM labels inline | `trace <file>`, the keys, `trace off`, then `read_trace.py --clean-heavy --skip-repeating --labels rom/TI59-labels.asm --changes <file>` (D-9) | LIB-10 (`trace`); `read_trace.py` stays (LIB-10 AC-33, AC-42) |
| `--boot`, `boot_recs` | the power-on trace | `trace <file>` then `cold-boot` | LIB-10, LIB-18 |
| `--summary`, `summary()` (`ti59.py:392-410`) | the addresses a key runs, with their counts | `profile on`, the key, `profile report <n>` (by count, the most first; D-9) | LIB-10 |
| `h_snapshot`, `regA_display` (`:205-208`; `ti59.py:314-317`) | the processor's registers and SCOM | `registers TMC0501`; `peek TMC0501 scom <n>` | LIB-10 |
| `h_disasm` (`:274-280`) | disassemble an instruction | `disassemble TMC0501 <address> [<count>]` | LIB-10 |
| `h_ram_read`, `h_ram_write` (`:210-211`) | read and write the RAM | `peek`, `poke TMC0501 ram …` | LIB-10 |
| `h_write_program`, `--program` (`:212`) | write a program | `c59 program [<step>] <keycode>…` | C59-2 AC-9 |
| `h_lib_exec_pc` (`:213`) | the module program's address | `registers TMC0501` (the debug target's register, M-53/M-55) | LIB-10 |
| `h_display`, `display()` (`:218-229`; `ti59.py:297-312`) | the display as text; the raw segments | `display-text` (LIB-11 AC-16); `display` (LIB-11 AC-23) | LIB-11 |
| `h_card_insert`, `h_card_eject`, `h_card_present`, `h_card_mode`, `h_card_waiting` (`:233-242`) | insert and eject a card, whether the machine waits for one | `card { mem: X, side: 0 }`, `card { file: … }`, `arm { … }`; `state media` (the place's `waits` and `passing`); what a pass wrote: `saveas { card: X, as: live, file: … }`, `media export magnetic-card <name> <path>` (D-10) | LIB-7, C59-7 |
| `h_printer_lines` (`:245-252`) | the printer's text lines | `paper text` | LIB-8 |
| `h_printer_code_lines` (`:259-272`) | the dot lines | `paper marks` | LIB-8 |
| `h_instruction_hz`, `h_printer_busy_cycles`, `h_idle_range` (`:106-115`) | facts the scripts needed | inside the `c59` commands of §3 (seconds in replies; the printer floor; the key-detect range of `coverage`) | C59-9 |
| `coverage.py build|show|unique|gaps` (`coverage.py:1-14, 217-307`) | which ROM addresses a scenario reaches | `c59 coverage [all]` per scenario; the script stays, on `calcu serve`, as `tools/coverage.py` (AC-13) | C59-9 |

**`agentic/`** (`ti59file.py` went in M-54, plan M-54)

| Today | Function | After M-59 | Feature |
|---|---|---|---|
| `ti59file.py file.ti59 [--lines]` (until M-54) | run a preset, its time from the start key, the printout, the printer's floor | `preset <file>`, `c59 run-time <last key>`, `paper text`; the floor in `c59 line-times` | LIB-6, LIB-8, C59-9 |
| `linetimes.py file [--lines] [--limit S]` | the time of each printed line, the gaps by kind | `c59 line-times [limit=<d>]` after the preset and its keys but the last, then `key <last>` and the command; the kind `week` (a 20-character day row) is computed by the calendar scripts from the lines (D-11) | C59-9 AC-3 |
| `runtime.py file [--keys …]` | the run time per key that runs a program, the total | `c59 run-time <key>…` (the keys named, or the preset's last ones) | C59-9 AC-5 |
| `hotsteps.py file [--until S]` | printer busy, print-wait, the program sources, the 25 hottest steps | `c59 hot-steps [limit=<d>]` | C59-9 AC-6 |
| `hotsteps.py file --window A B` | the steps run between two times | `c59 step-trace <A> <B>` | C59-9 AC-7 |
| `stepcost.py ["43 22" …]` | the cost of a step in fast and normal mode | `c59 step-cost <keycode>…`, one sequence per command; the table of common cases is the skill's example list (D-3) | C59-9 AC-8 |
| `kdis.py file` | the program instruction by instruction; the registers, the keys, the partition | `preset <file>`, `c59 keycodes disassemble`; `c59 data-registers <NN>…`, `c59 partition` | C59-9 AC-14; C59-2 AC-9 |
| (none: a listing by the printer) | a listing to paste into a preset | `c59 keycodes list` | C59-9 AC-9 |
| `asm59.py` (library: mnemonics, labels, computed addresses) | write a program with labels | stays a library of the calendar scripts, `tools/calendar/asm59.py`; a plain assembly is read by `c59 keycodes assemble <file>` | C59-9 AC-10 (spec OQ-2) |
| `fastmode_matrix.py [substring]` | each instruction class in fast and normal mode; the stop table | stays a script on `calcu serve`, `tools/fastmode_matrix.py` (AC-13), on `c59 program`, `c59 data-registers`, `key`, `wait`, `registers`, `history`, `display-text`, `paper text` | C59-9 AC-13 |
| `calendar/` (`check.py`, `compare.py`, `years.py`, `fastcal.py`, `swapcal.py`) | the calendar programs' checks and generators | stay scripts, `tools/calendar/`, on `calcu serve` with `c59 line-times`, `c59 run-time` (LIB-11 OQ-5 as amended) | C59-2 AC-18 |
| `tools/read_trace.py` | a trace file as text or JSON | stays (two options added, D-9) | LIB-10 AC-33, AC-42 |

The scripts that stay find `calcu` through one helper,
`tools/calcu59.py` (new, a few lines): the program built in
`build/tools` (configured and built on first use, as
`tools/headless/build.sh` does today), opened with
`library/harness/python/calcu_serve.py`'s `Calcu(program, "59")`. Their
output that was under `tools/headless/build/` goes to `tools/build/`
(ignored).

## 5. Adapters

None new. The adapter check over Calc-U-59 and the library is LIB-11
§11.5. `TIAdapters`' last members that are not adapters (the typing
support, the key-detect range) move to `core/TITypingSupport.hpp` so that
no file named for adapters remains (LIB-11 D-17).

## 6. Tasks

Tests before the code; `CalcU59Tests`, Frequent; names `c59_9_acN_…`,
`c59_9_ecN_…`; on the harness entry of LIB-11 §11 through
`harness::Server` lines, the expected reply written out in the test with
a program small enough for it (Technical Requirements); `SKIP_UNLESS` the
ROMs are there. Expected numbers are taken from today's tools on the
same program before they are removed (a commit of part 1 that records
them in the test), so a difference is a finding, not a new value.

| Task | What | ACs/ECs | Test (`tests/harness_product_commands_tests.cpp` unless named) |
|---|---|---|---|
| T1 | Registry under `c59` on every model; the platforms | AC-1 | `c59_9_ac1_commands_under_c59_on_ti59_ti58_ti58c` (`--help` lists them; each model answers `c59 keycodes list 000 000`); the build on macOS, Linux and Windows is LIB-11 AC-15's CI case |
| T2 | `coverage` | AC-2 | `c59_9_ac2_refused_without_profile`, `c59_9_ac2_addresses_of_one_key_in_order_without_key_readiness` (`profile on`, `key 5`, `c59 coverage`: the addresses written out, compared with today's `coverage.py` scenario `digit5`), `c59_9_ac2_all_includes_key_readiness` |
| T3 | `line-times` | AC-3, AC-4, EC-2, EC-6 | `c59_9_ac3_lines_gaps_summary_floor` (a short program that prints three lines and a feed: `Op 05`, `Adv`), `c59_9_ac3_limit_ends_it`, `c59_9_ac4_floor_4165_per_line_1370_per_feed`, `c59_9_ec2_nothing_printed_after_15_s_idle`, `c59_9_ec6_line_times_refused_without_printer` |
| T4 | `run-time` | AC-5 | `c59_9_ac5_time_per_key_and_total` (a label that loops a known count; a digit key gives 0), `c59_9_ac5_not_finished_after_4000_s_marked_done` (a program that never stops) |
| T5 | `hot-steps`, `step-trace` | AC-6, AC-7, EC-6 | `c59_9_ac6_shares_sources_and_top_steps` (a loop of known steps), `c59_9_ec6_printer_shares_zero_without_printer`, `c59_9_ac7_steps_between_two_times_folded` |
| T6 | `step-cost` | AC-8, EC-3 | `c59_9_ac8_cost_of_rcl_in_fast_and_normal_mode` (`43 22`: today's numbers of `stepcost.py` recorded in part 1), `c59_9_ac8_leaves_the_machine_changed`, `c59_9_ec3_endless_sequence_fails_after_300_s` (`61 00 08`) |
| T7 | `keycodes list` | AC-9, EC-4 | `c59_9_ac9_listing_as_list_prints_it` (step, keycode, printer name; the same lines as the PC-100C's `List` of the program, `paper text`), `c59_9_ac9_listing_pasted_into_a_preset_loads_the_same_program`, `c59_9_ec4_empty_program_no_lines` |
| T8 | `disassemble`, `assemble` | AC-10, AC-11, AC-14 | `c59_9_ac14_assembly_lines_as_the_spec_example` (the four lines of AC-14 and a merged code), `c59_9_ac10_round_trip`, `c59_9_ac10_refused_unknown_mnemonic_missing_operand_beyond_last_step_nothing_written`, `c59_9_ac10_program_reads_the_same_memory`, `c59_9_ac11_every_mnemonic_is_a_key_name` (each mnemonic of the table resolves through `key`'s names) |
| T9 | TI-58C time | EC-5 | `c59_9_ec5_ti58c_instructions_are_ticks_over_32` |
| T10 | Machine state | AC-15 | `tests/machine_state_tests.cpp`: `c59_9_ac15_restore_continues_identically_<model>` (TI-59, TI-58, TI-58C: save in a running program, `wait 2s`, restore, `wait 2s`: the display, the ticks and the paper equal), `c59_9_ac15_with_printer_module_and_card_in_reader`, `c59_9_ac15_refused_for_other_peripherals` |
| T11 | Scripts that stay | AC-13 | `CalcU59Scripts` (CTest, Longer): `tools/coverage.py build` for three scenarios (`boot`, `digit5`, `add`) and `show`, compared with the result today's script gives (recorded in part 1); `tools/fastmode_matrix.py label` gives today's lines for the `label` cases (`tools/agentic/README.md:158-171`) |

Coverage: AC-1 to AC-11, AC-13 to AC-15 (14 of 14; AC-12 withdrawn),
EC-2 to EC-6 (5 of 5).

## 7. Tools and agent knowledge

What goes, and the knowledge that changes in the same step, is LIB-11
§11.7. This row adds to it:
- `products/calc-u-59/CLAUDE.md:43-47, 78`: the section on
  `tools/agentic/` and `tools/headless/` becomes the Harness section:
  `calcu --product c59`, the `c59` commands, the scripts that stay,
  and where the time base and the pitfalls are (`reference/Harness.md`).
- `reference/Harness.md` (new; replaces `tools/headless/README.md` and
  `tools/agentic/README.md`, plan M-59): the time base (emulated seconds,
  weighted instructions; `tools/agentic/README.md:32-44`), the recipes of
  §4 (a ROM trace per key, timing a program, the hottest steps, the cost
  of a step, coverage, the fast-mode matrix, the calendar), and the
  pitfalls that stay true (`:225-246`: two numbering systems, a program
  longer than the partition, Op 2x/3x, register values, fast mode,
  jumps in `step-cost`, settle overhead).
- `docs/GLOSSARY.md` has "mnemonic", "listing" and "assembly" (spec
  OQ-4, decision log of 2026-11-13); no change.

## 8. Golden runs and risks

- **Machine state** is the large part: a `saveState` on every core part
  that holds state. Saving only reads; the golden replays must pass
  unchanged. A restore that misses a piece of state shows in T10. If T10
  cannot be made green within the step, the commit of §3.7 is split off
  and the row stays In progress (the rest of M-59 does not wait).
- **Numbers of today's tools.** The tests take their expected values
  from today's tools on the same program before the tools go (§6); a
  difference after the move is a finding (rule 3 of the prompt). Known
  sources of difference: the slices (D-1) and `step-cost` on the
  session's machine instead of a fresh one (§3.3).
- **The print-wait loops** of `hot-steps` are ROM addresses of the TI-59
  (`hotsteps.py:22`); for the TI-58C they are not known (D-6).
- **Speed of the commands.** `hot-steps` with its default 200 s makes
  about 29 000 advances of 97 instructions; `line-times` up to 4000 s in
  slices of 200 instructions. The tests use short programs and limits so
  that `CalcU59Tests` stays inside Frequent's 60 s.

## 9. Decided (best guess)

- **D-1 (AC-3, AC-5).** Where the spec names no sampling distance, a
  command runs in today's tool's slices (200 instructions for
  `line-times`, 100 for `run-time`), so that its times are today's.
- **D-2 (AC-6).** `hot-steps` gives the three sources the spec names and,
  inside memory, the notes in fast mode, which today's tool shows as the
  Prg Src values 2, 4, 6 (`hotsteps.py:44`).
- **D-3 (AC-8).** `step-cost` measures the sequences it is given; without
  one it is refused with its usage. Today's table of common cases
  (`stepcost.py:22-26`) becomes an example list in `reference/Harness.md`.
- **D-4 (AC-9, EC-4).** The default last step of `list` and
  `disassemble` is the last step that is not `00`, so an empty program
  gives no lines; a range given is listed whole.
- **D-5 (AC-10, AC-14).** A merged code is written as its key and `Ind`
  and assembled back into the merged code, as LRN stores `STO Ind`; the
  round trip is tested on programs as the calculator stores them.
- **D-6 (AC-6).** On the TI-58C the print-wait share is zero, because
  the loops of today's tool are the TI-59's ROM addresses and no list
  for the TI-58C exists; the printer busy share is read from the flag on
  every model.
- **D-7 (AC-2).** The addresses left out without `all` are the model's
  key-detect range, the product function `idleAndKeyDetectRange` that
  today's tools and the full-speed rule use (C59-1 AC-18's key
  readiness).
- **D-8 (AC-15).** A restored card in the reader belongs to no binding;
  a bank it ends up holding goes to the card fitted then, or is dropped
  with a log line.
- **D-9 (LIB-11 AC-14).** `ti59.py`'s annotated trace becomes a trace
  file read by `tools/read_trace.py`, which gets `--labels <file>` (the
  ROM labels of `rom/TI59-labels.asm`, `TI58C-labels.asm`, inline as
  today) and `--changes` (only the registers and SCOM rows an
  instruction changed, as `ti59.py` printed them); `--summary` is
  `profile report`, ordered by count instead of first execution.
- **D-10 (LIB-11 AC-14).** "Eject a card" of the tools served to read
  back what a pass wrote; with cards in the media model a pass gives the
  card back by itself and its content is in the in-file card or its file
  (`saveas`, `media export`). The harness has no `eject` for the card
  reader, as presets have none (LIB-7 AC-26).
- **D-11 (AC-3).** `line-times` groups feeds and other lines, as the
  spec says; the `week` group of `linetimes.py:31-32` is a calendar
  matter and moves into `tools/calendar/compare.py`.

## 10. Questions for the user

None that splits the design.
