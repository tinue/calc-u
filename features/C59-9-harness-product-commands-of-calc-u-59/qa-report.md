# C59-9: QA report

Results per acceptance criterion and edge case of `spec.md`.

## M-59 (2026-11-17, branch `m59-harness-59`)

Built and run on an M4 Pro, macOS 27.2, Apple clang, CMake, Ninja: the
worktree's `build/tests` (RelWithDebInfo, Qt off, Frequent), with the
Longer entries this step added or changed run by hand (`CalcU59Scripts`,
the harness half of `CalcU59RomMissingBuild`); `build/qt` for the Qt6 app.
With LIB-11's Calc-U-59 column (its qa-report has the entry, `lcd`, `wait`,
the table of AC-14 and the adapter check).

The code (design §2):
- `products/calc-u-59/app/TIHarnessCommands.*`: every `ti59` command (C59-2's
  and C59-4's moved here from `app/HarnessProduct.cpp`), in the harness
  target only (`calcu59_harness`).
- `app/TIRunProbe.*`: the run facts of §3.2 from `c59.calculator-state`'s
  processor frame (program step, Prg Src, printer busy, the print-wait loops
  `kPrintWaitLoops`, fB[0], the display state of the last digit-counter
  revolution, idle).
- `app/TIMnemonics.*`: the assembly mnemonics (the key IDs of `preset.md`
  §8.1), read back through the presses `key` makes for them (`tiKeyNames`,
  the one table, AC-11), the listing in the PC-100C's form, the assembler.
- `core/TIMachineState.*`, `core/TIStateRecords.hpp` (the form Calc-U-1600's
  `StateRecords.hpp` has): the generic kind `machine-state`,
  `TMC0501/machine-state`, over `stateIo` templates of the TMC0501, the RAM,
  `TI59Machine`, `TIMachine`, the PC-100C, the module and the card reader.

Tests: `products/calc-u-59/tests/harness_product_commands_tests.cpp` (26
`TEST`s) and `tests/machine_state_tests.cpp` (5) in `CalcU59Tests`
(Frequent; the whole program 32 s alone), each on a `calcu` run of its own
with its replies written out; `CalcU59Scripts` (Longer, `tests/check_scripts.py`).
Result: `CalcU59Tests` 591 `TEST`s, 40952 checks passed, 0 failed; Frequent
(`ctest --test-dir build/tests`) all passed.

The expected numbers were taken from today's tools on the same program at
the base of M-59 before they went (`tools/agentic/*.py` on `calcu`, the
headless `stepcost.py`, `fastmode_matrix.py` and `coverage.py` on the
CMake-built `ti59headless`); each difference is named below.

| ID | Result | Test case |
|---|---|---|
| AC-1 | Pass | `c59_9_ac1_commands_under_ti59_on_ti59_ti58_ti58c` (`--help`'s usage lists the commands; `ti59 keycodes list 000 000`, `ti59 program`, `ti59 keycodes disassemble` on each model; an unknown `ti59` word refused). macOS here; Linux and Windows: `calcu` with Calc-U-59 runs in every desktop job of `calc-u-59.yml` (LIB-11 AC-15, H5) at the CI dispatch |
| AC-2 | Pass | `c59_9_ac2_refused_without_profile`, `c59_9_ac2_addresses_of_one_key_in_order_without_key_readiness` (the 125 addresses of the 5 key written out: those of `coverage.py`'s scenario `digit5` at the base, without 0627-062A, see the finding), `c59_9_ac2_all_includes_key_readiness` (plain and `--json`) |
| AC-3 | Pass | `c59_9_ac3_lines_gaps_summary_floor` (`Lbl A 1 Prt 2 Prt Adv 3 Prt R/S`: five lines, two of them feeds; `linetimes.py` gave 0.11, 0.40, 0.69, 0.79, 0.89 s from before the key, here 0.014, 0.309, 0.605, 0.689, 0.802 from after it, gaps 0.295/0.295/0.084/0.113 against 0.29/0.29/0.10/0.10 in samples of 10 ms), `c59_9_ac3_limit_ends_it` |
| AC-4 | Pass | `c59_9_ac4_floor_4165_per_line_1370_per_feed` (3 × 4165 + 2 × 1370 instructions of 32 ticks: 1.071472527 s) |
| AC-5 | Pass | `c59_9_ac5_time_per_key_and_total` (`2 0 A` on `Lbl A STO 00 Lbl B Dsz 0 B R/S`: 0, 0, 3.187481318 s; `runtime.py` gave A:3.19; a Pause flash does not end the time), `c59_9_ac5_not_finished_after_4000_s_marked_done` |
| AC-6 | Pass | `c59_9_ac6_shares_sources_and_top_steps` (the loop: 147 notes in 1 s, steps 007 40.1 %, 006 32.6 %, 008 21.0 %; `hotsteps.py --until 3` gave 43.4 / 33.2 / 21.2 % sampling every millisecond from before the key; a module program, ML-01: memory 59, module 88; a `Prt` loop with a PC-100C: printer busy 98.6 %, print-wait 41.4 %) |
| AC-7 | Pass | `c59_9_ac7_steps_between_two_times_folded` (`006:3 007:11`; the second time before the first refused) |
| AC-8 | Pass | `c59_9_ac8_cost_of_rcl_in_fast_and_normal_mode` (`43 22` with a PC-100C: fast 584, normal 1333 instructions, 41 / 94 ms; `stepcost.py` gave 580 / 1332, see the finding), `c59_9_ac8_leaves_the_machine_changed` (its program and registers stay); refused while a program runs (`lib11_ac14_every_function_of_the_table`) |
| AC-9 | Pass | `c59_9_ac9_listing_as_list_prints_it` (all 100 keycodes in a row: the 100 lines equal what the PC-100C's `List` printed, `paper text`, the printer's columns as single spaces), `c59_9_ac9_listing_pasted_into_a_preset_loads_the_same_program` |
| AC-10 | Pass | `c59_9_ac10_round_trip` (`examples/sum.ti59` disassembled, a cold boot, assembled: the same 40 steps and the same assembly), `c59_9_ac10_refused_unknown_mnemonic_missing_operand_beyond_last_step_nothing_written` (unknown mnemonic, missing operand, beyond 959 and 479, a step written twice, an operand of three digits, a token too many; nothing written), `c59_9_ac10_program_reads_the_same_memory` |
| AC-11 | Pass | `c59_9_ac11_every_mnemonic_is_a_key_name` (every keycode: its mnemonic is pressed by `key` (`tiKeyNames`) and assembles back to it; 26, 27 and HIR have no key, decision log) |
| AC-12 | Withdrawn | — |
| AC-13 | Pass | `CalcU59Scripts` (Longer, `tests/check_scripts.py`): `tools/coverage.py build boot digit5 add` and `show` on `calcu serve`: the words of each scenario equal 2.3.0's (`tests/reference/scripts/coverage-base.json`) without the key-detect range; `tools/fastmode_matrix.py label`: the six `label` cases give 2.3.0's verdicts (`fastmode-label-base.txt`: whether the run ended, the registers that differ). The whole `coverage.py build` (all scenarios) reached 5354 of 6144 words (2.3.0: 5375, see the finding); the calendar scripts: `CalcU59CalendarScripts` (calendar-09 for 2026: 75.9 s, as `CALENDAR_PROGRAMS.md`'s 1:16) |
| AC-14 | Pass | `c59_9_ac14_assembly_lines_as_the_spec_example` (the four lines of AC-14, `STO Ind 05` for `72 05`, `INV` `SBR` not merged, `RTN`, `2nd CLR`, `IFF 01 E`; a range; `--json`) |
| AC-15 | Pass | `tests/machine_state_tests.cpp`: `c59_9_ac15_restore_continues_identically_ti59`, `_ti58`, `_ti58c` (a data register of the constant memory written before), `c59_9_ac15_with_printer_module_and_card_in_reader` (a PC-100C printing in a loop with the ML module; a card passing the reader, fed by `media.feed` while `1 Write` waits, checked passing at the save), `c59_9_ac15_refused_for_other_peripherals` (a detached printer, another model: refused, nothing changed). Each: save while the program runs, 2 s later `lcd`, `display`, `ticks`, the paper printed in the 2 s and the whole state equal after a restore in the same run and in a second run restored 1.337 s later; a save right after a restore gives the bytes restored |
| EC-1 | — | (none in the spec) |
| EC-2 | Pass | `c59_9_ec2_nothing_printed_after_15_s_idle` (`no lines printed`, the command took 15.0 s) |
| EC-3 | Pass | `c59_9_ec3_endless_sequence_fails_after_300_s` (`61 00 08`) |
| EC-4 | Pass | `c59_9_ec4_empty_program_no_lines` (no lines; a range given is listed whole, D-4; a step past 959 refused) |
| EC-5 | Pass | `c59_9_ec5_ti58c_instructions_are_ticks_over_32` (1 s of notes every 97 instructions of 32 ticks at the TI-58C's 394 000 ticks/s: 127; `step-cost 68`: 142 instructions, 12 ms) |
| EC-6 | Pass | `c59_9_ec6_line_times_refused_without_printer`, `c59_9_ec6_printer_shares_zero_without_printer` |

Coverage: AC-1 to AC-11, AC-13 to AC-15 (14 of 14; AC-12 withdrawn), EC-2
to EC-6 (5 of 5). C59-9 Done.

### Deviations from the design, chosen by the simpler structure

- **The slices run on the machine session** (`MachineSession::advanceUnpaced`:
  the slice hooks and the run control's gate, as every advance), not
  through `HarnessHost::advance` (design §3, "Running inside a command"):
  the app session's per-call duties after each of thousands of slices made
  `ti59 step-cost`'s 300 s refusal take 31 s; now 0.3 s. At the end of each
  timing command one instruction more runs through the app session, so its
  parts follow (the display read over the command's time, the paper, the
  topics).
- **The display lit** (§3.2) is the processor's display state of the last
  digit-counter revolution (`CpuFrame::displayOn`), not the display output
  since the last look: reading the output would take it from the session.
- **`ti59 step-cost` refuses while a program runs** ("A program runs: let it
  end first (`wait`)."): 2.3.0's entry keys pressed into a running program
  measure nothing (it gave −212 and 14017 instructions); spec: the
  session's machine.
- **Mnemonics without a key** (AC-11): 82 is written `HIR` (its printer
  name; `key` refuses it, `preset.md` §8.1.1), 26 and 27 (printed `2ND` and
  `INV` like 21 and 22) as their two digits; 20 is `2nd CLR` (two words),
  the merged codes `<key> Ind` (D-5). Decision log.
- **The listing's name of 65** is the printer's `x`; `app/KeyNames`' table
  (the debug panel's, `tests/golden/keynames.txt` of 2.3.0) names it `|x|`,
  keycode 50's name (finding).
- **`line-times` counts from the command** (spec decision log): after the
  last key is up, so its times are the key's hold shorter than
  `linetimes.py`'s (which counted from before the key).
- **No `core/TIMachineState` kind of its own in the product's list**: the
  kind is LIB-11's generic one. A card restored in the reader (D-8) is a
  card of its own, which the pass gives back with what it wrote; the media
  part's bindings, the paper and the debugger's state are the session's.
- **The tests stay in `CalcU59Tests`** (Frequent): 32 s alone with them.

### Findings for the user

- **`ti59 coverage` leaves out the whole key-detect range 0627-0660**
  (D-7); 2.3.0's `coverage.py` left out an instruction there only with the
  ROM's idle flag set, so it kept the release gate 0627-062A of a key
  press. A scenario's words differ by those; the whole `coverage.py build`
  reaches 5354 words (2.3.0: 5375). The `list` scenarios reach 57 words
  more, since `wait` lets the List run to its end (2.3.0 stopped after 2
  million instructions).
- **`ti59 step-cost` is a few instructions above 2.3.0's tool**: 584 / 1333
  for `43 22` with a PC-100C (580 / 1332), 152 / 227 for Nop (144 / 225),
  4546 / 4567 for `69 05` (4539 / 4566): the session's machine instead of a
  fresh one for each run, the holds in ticks instead of instructions.
- **The debug panel names keycode 65 `|x|`** (the table of 2.3.0, its
  golden `tests/golden/keynames.txt`): `|x|` is keycode 50; the PC-100C
  prints `x`. Left as it is (a golden table); the listing uses the printer's.
- **`fastmode_matrix.py`'s display samples** differ from 2.3.0's: its
  `1.-0.0` samples (a display read across a change) do not come with `lcd`,
  and `x=t label` sees the Prg Src value 2 in fast mode; the verdicts
  (whether the run ended, the registers) are the same.

### Golden runs

`golden_runs_tests.cpp` passes unchanged (in `CalcU59Tests`). The core
changes are the `stateIo` templates (saving only reads), the
`registerTop()` of the program memory kind and the memory adapter's calls
moved to `TIMachineTesting`; no golden run differs.
