# LIB-11: Harness

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: LIB-1 (machines), LIB-3 (app sessions, commands with
  replies, topics, operations run in one go).
- Contracts and decisions: `docs/contracts/harness.md` (H-1 to H-4, I-1
  to I-3, the table of functions to keep); ADR 0026, 0033;
  `Briefing.md` §8.
- Facts: Calc-U-59 at `v2.3.0`, Calc-U-1600 at `b4ea8c8`.

What the three status columns mean for this feature:
- **Library:** the program `calcu` exists with its one-shot form, its
  `serve` form, its protocol and its first commands, shown with a
  product made for the tests.
- **Calc-U-59, Calc-U-1600:** `calcu` runs the product, and every
  function of the product's tools of today (AC-13, AC-17) is available
  through it, each delivered by the feature the table names. A column
  is therefore complete only when those features are; the first
  commands work as soon as the product's machines are behind the
  machine interface.

## User Stories
- As an agent developing or testing a product, I want to run it without
  a window, send it input and read its state, so that I can check my
  work in seconds.
- As an agent, I want one tool and one language for every Calc-U
  product, and the same words in presets and in the harness.
- As an agent, I want a run to give the same result every time, so that
  a difference means a change in the code.
- As the developer, I want the harness to run the same sessions as the
  frontends, so that what it shows is what a user gets.
- As the developer, I do not want a test run to touch my own settings
  and media.

## Out of Scope
- The commands of other features: `key` and `type` (LIB-5); `load` of a
  preset, the steps as commands, `expect`, `screenshot`, checking
  presets (LIB-6); media (LIB-7); paper (LIB-8); `peek`, `poke`,
  `break`, stepping, traces (LIB-10); model, variant, attach, reset
  (LIB-18). Each is specified with its feature (rule 4 of the cut).
- Product commands (`harness.md` H-2) for functions that are carried
  over without a row of their own; see the open point in
  `features/INDEX.md` and OQ-4.
- Attach to a running frontend (owner: C59-5 for the Swift frontend;
  Qt6 and Web later, `harness.md` H-4). This feature defines the
  protocol that attach uses.
- A Python helper for `serve` (`harness.md` Q-2: phase 6, with the
  skills).
- GUI automation and the screenshot scenarios of the Qt6 frontends
  (ADR 0032).
- When the old tools are removed (the migration plan).

## Acceptance Criteria

### Library

The program
- [ ] **AC-1** — Given the source repository, then it builds one program
  named `calcu` for the family, which takes the product to run from
  `--product` (ADR 0026; `harness.md` Q-1).
- [ ] **AC-2** — Given `calcu` with a subcommand and its arguments, then
  it carries out that one command, prints its result and ends, with exit
  status zero on success and non-zero with a message on failure.
- [ ] **AC-3** — Given `calcu serve`, then it reads one command per line
  from its standard input and writes the reply to its standard output,
  until `quit` or the end of the input.
- [ ] **AC-4** — Given either form, then the result of a command is
  plain lines of text; with `--json` it is one JSON value per reply
  instead (`harness.md` H-3).
- [ ] **AC-5** — Given a command, then its reply is the reply of the app
  session (LIB-3 AC-13): done with its result, or refused with the
  reason; a line that is not a command gets a refusal and `serve` goes
  on.

How it runs
- [ ] **AC-6** — Given `calcu`, then it runs the library's app session
  and the product's app layer, the same as a frontend does, and reaches
  the machine only through them (`harness.md` I-1).
- [ ] **AC-7** — Given two runs with the same product, options and
  commands, then every reply is the same in both; the harness has no
  wall clock, and emulated time passes only by commands (`harness.md`
  I-2).
- [ ] **AC-8** — Given a command that starts an operation, then the
  harness runs it to its end before it replies (LIB-3 AC-19).
- [ ] **AC-9** — Given a run without options that name them, then the
  harness reads no settings file of the user and writes nothing into the
  user's save folder; its settings are the defaults and its save folder
  is a temporary one. Options name a settings file, a save folder and a
  ROM folder to use instead (LIB-9 AC-18).

The first commands
- [ ] **AC-10** — Given a product behind the machine interface, then
  these commands work, at the top level, in both forms:

  | Command | What it does | Reply |
  |---|---|---|
  | `wait <duration>` | lets that much emulated time pass; the duration has a unit, `2s` or `500ms` | the emulated time that passed |
  | `key-down <key ID>`, `key-up <key ID>` | presses or releases one key of the machine, as it is | done, or refused for an unknown key ID |
  | `lcd` | reads the display | the display as text (AC-11) |
  | `state <topic>` | reads a topic of the app session | the last state of a state topic, or what a stream topic appended since it was last read |
  | `capabilities` | reads what the machine offers now | the capabilities with kind, ID and owner; the places and what is plugged in |
  | `ticks` | reads the machine's emulated time | the tick count and the same in seconds |
  | `audio <file>` | from now on writes the machine's sound output to a WAV file | done; the file is complete at `quit` |
  | `quit` | ends `serve` | — |

- [ ] **AC-11** — Given `lcd`, then the reply gives what the display
  shows as text: for a dot-matrix display the rows of text the dots
  form, the indicators that are on, and how many character cells could
  not be read as text; for a segment display the characters of the
  positions from left to right and the indicators that are on. With
  `--json` it also gives whether the display is on.
- [ ] **AC-12** — Given a product's commands that only that product has,
  then they are reached under the product's name (`ti59 …`, `pc1600 …`),
  and a command that every product has is at the top level, with
  product-specific output where the hardware differs (`harness.md` H-2).

### Calc-U-59
- [ ] **AC-13** — Given the tools of Calc-U-59 at the pin, then each of
  their functions is available through `calcu`, delivered by the feature
  named here:

  | Today | Function | Delivered by |
  |---|---|---|
  | `tools/headless/` (C library, `ti59.py`) | run without a window; press and release keys; let instructions pass | this feature (AC-10) |
  | | drive step by step from a script | this feature (`serve` over a pipe) |
  | | type keys by name, second functions included | LIB-5, LIB-6 |
  | | a record per instruction (address, instruction, registers); disassembly | LIB-10 |
  | | read and write the RAM; read the registers and the SCOM | LIB-10 |
  | | write a program | C59-2 |
  | | the display as text | this feature (AC-11) |
  | | printer lines and dot lines; the PC-100C's PRINT, ADV and TRACE | LIB-8; LIB-2 (controls), C59-6 |
  | | insert and eject a card, whether the machine waits for one | LIB-7, C59-7 |
  | | which module is plugged in, with or without printer, the model | LIB-18 |
  | `tools/headless/coverage.py` | which ROM addresses a set of scenarios reaches | a product command (OQ-4) |
  | `tools/agentic/ti59file.py` | run a preset and report the emulated time and the printout | LIB-6, LIB-8 |
  | `tools/agentic/linetimes.py`, `runtime.py`, `hotsteps.py`, `stepcost.py`, `fastmode_matrix.py` | timings per printed line and per key, the hottest program steps, the cost of a step | product commands (OQ-4) |
  | `tools/agentic/kdis.py`, `asm59.py` | list a program's keycodes; assemble keycodes | product commands (OQ-4) |
  | `tools/read_trace.py` | a trace file as text or JSON | LIB-10 (the open point "Traces") |

  (`tools/headless/capi.cpp:47-196`, `tools/headless/ti59.py:155-437`,
  `tools/agentic/README.md`.)
- [ ] **AC-14** — Given `calcu --product calc-u-59` on macOS, Linux and
  Windows, then it runs. Today the headless tools build on macOS only
  (`tools/headless/README.md:5-6`).
- [ ] **AC-15** — Given a TI machine, then `lcd` gives the twelve
  positions as characters, leftmost first, with the decimal points, and
  says whether the indicator C is on, as `display()` of today's tool
  does (`tools/headless/ti59.py`, `Machine.display`).
- [ ] **AC-16** — Given `wait` on a TI machine, then the emulated time
  it reports counts as the tools count today: an instruction in the
  ROM's idle state weighs four on the TI-59 and TI-58 (C59-1 AC-1).

### Calc-U-1600
- [ ] **AC-17** — Given the tools of Calc-U-1600 at the pin, then each
  of their functions is available through `calcu`, delivered by the
  feature named here:

  | Today (`pc1500_cli`, `pc1600_cli`) | Function | Delivered by |
  |---|---|---|
  | `--preset <file>` | build a machine from a preset and run it | LIB-6 |
  | `--check-preset <file>…` | check that presets can be read | LIB-6 |
  | `--rom`, `--ce1600p-rom` | choose a variant | LIB-18 |
  | `--run-after <ticks>`, the run for a number of ticks | let emulated time pass | this feature (`wait`) |
  | `--lcd-text <file>` | the display as text | this feature (AC-11) |
  | `--lcd-png <file>` | the display as an image | LIB-6 (`screenshot`) |
  | `--wav <file>` | the buzzer of the whole run as a WAV file | this feature (`audio`) |
  | `--dump-mem <address>,<length>` | memory as a hex dump | LIB-10 |
  | `--inspect <view>` | an INSPECTOR view as text | a product command (C1600-1 AC-19, OQ-4) |
  | `--dump-basic` | the BASIC pointers and the program area | a product command (OQ-4) |
  | `--tape-in <wav>`, `--tape-out <wav>` | play or record a cassette | LIB-7; a product command for the file (OQ-4) |
  | `--ce158-pty`, `--ce158-rx`, `--ce158-rx-hold`, `--ce158-tx` | feed and capture the CE-158's serial port | a product command (OQ-4) |
  | `--save-dir`, `--modules-dir` | the save folder; where module definitions are | this feature (AC-9), LIB-7 |
  | the summary after a run: registers, the last instructions, where the program counter spent its time | | LIB-10 |
  | `pc1600_plotter_probe`, `pc1600_power_probe`, `pc1600_uart_probe` | diagnostics of the plotter, of power-off and of the serial port | product commands (OQ-4) |
  | `tools/read_trace.py` | a trace file as text or JSON | LIB-10 (the open point "Traces") |

  (`tools/pc1500_cli.cpp:92-377`, `tools/pc1600_cli.cpp:90-365`,
  `tools/pc1600_plotter_probe.cpp`, `tools/pc1600_power_probe.cpp`,
  `tools/pc1600_uart_probe.cpp`.)
- [ ] **AC-18** — Given a Sharp machine, then `lcd` gives the rows of
  text, a line with the indicators that are on, and the number of cells
  that are not text, as `--lcd-text` does today
  (`tools/pc1500_cli.cpp:271-277`).
- [ ] **AC-19** — Given `calcu --product calc-u-1600`, then it finds
  ROMs, module definitions and the save folder as the frontends do
  (LIB-12, LIB-7, LIB-9), not in folders next to where it is run. Today
  the tools use their own lookup and do not run the app session
  (`tools/pc1600_cli.cpp:110-141`; `docs/inventory/divergence-matrix.md`,
  M73).

## Edge Cases
- **EC-1** — Given `calcu` is asked for a machine whose ROMs are
  missing, then it ends with an error that names the files (LIB-12
  AC-12).
- **EC-2** — Given `wait` with a duration without a unit, then it is
  refused.
- **EC-3** — Given the standard input of `serve` ends without `quit`,
  then `serve` ends as after `quit`, and a file being written by `audio`
  is complete.
- **EC-4** — Given `--product` names no product of this build, then
  `calcu` ends with an error that lists the products.
- **EC-5** — Given `state` for a topic that does not exist, then it is
  refused.

## Technical Requirements (optional)
- The harness is not a frontend and not for end users (glossary); it is
  documented for agents, in the skills, in the same step in which a tool
  of today is replaced (`Briefing.md` §8).
- The serve protocol is the one a frontend exposes for attach
  (`harness.md`, responsibilities; ADR 0033): line-based text, the same
  commands and replies.
- The commands are the preset step vocabulary plus commands to look and
  to control (`harness.md` H-1): a command that is also a step has the
  same name and meaning in both, and is specified once.

## Open Questions
- [ ] **OQ-1** — The names of the first commands (AC-10). `harness.md`
  H-1 names `lcd`, `dump`, `state` and `quit`; `wait` is a preset step
  with the same meaning. `key-down`, `key-up`, `capabilities`, `ticks`
  and `audio` are proposed here. *Recommendation:* as in AC-10.
- [ ] **OQ-2** — How a run names its product. H-2 says `--product`
  selects it for one-shot runs. Is it needed for `serve` too, and is it
  the product's name as written (`Calc-U-59`) or a short form?
  *Recommendation:* always `--product`, matched without regard to case,
  with the short forms `59` and `1600`.
- [ ] **OQ-3** — What `--json` looks like: one object per reply with the
  command's result under a fixed key, and the refusal's reason under
  another? *Recommendation:* decide the shape in the design; this spec
  only requires that every reply is one JSON value on one line.
- [ ] **OQ-4** — The product commands. AC-13 and AC-17 name functions
  that only one product has and that no row of the index owns
  (coverage, timings and keycode tools of Calc-U-59; BASIC dump, tape
  files, serial peer, INSPECTOR views and probes of Calc-U-1600).
  *Recommendation:* one row per product, "Harness product commands",
  depending on this feature (the open point in `features/INDEX.md`).
- [ ] **OQ-5** — The three probes of Calc-U-1600 and the calendar
  scripts of Calc-U-59 (`tools/agentic/calendar/`) were written for one
  investigation each. Must they become harness commands, or may they
  stay as scripts that use `calcu serve`? *Recommendation:* scripts;
  `harness.md` lists the probes among the functions to keep, so this
  needs the user.
- [ ] **OQ-6** — The form of the display as text for a segment display
  (AC-15): how a position with a sign, a blank position and the
  brightness are written. *Recommendation:* today's string of
  `ti59.py`; brightness only with `--json`.

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| One program for the family, with one-shot commands and a `serve` form; it keeps every function of today's tools | ADR 0026; `Briefing.md` §8 | 2026-11-09 |
| The program is called `calcu` | `harness.md` Q-1 | 2026-11-10 |
| The commands are the preset step vocabulary plus commands to look and to control | `harness.md` H-1 | 2026-11-10 |
| Common commands at the top level, product commands under the product's name | `harness.md` H-2 | 2026-11-10 |
| Line-based text, `--json` on request; localhost only for attach; never in release builds | `harness.md` H-3 | 2026-11-10 |
| Attach is built for the Swift frontend in the migration | `harness.md` H-4; ADR 0033 | 2026-11-10 |
| Every command gets a reply | LIB-3 decision log (user) | 2026-11-10 |
| Proposed in this spec, for the gate: a run touches no settings and no save folder of the user unless options say so (AC-9); the first commands and their names (AC-10) | LIB-9 left the default to this spec; H-1 names only some of them | 2026-11-10 |
