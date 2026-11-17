# LIB-11: Harness

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: LIB-1 (machines), LIB-3 (app sessions, commands with
  replies, requests, topics, operations run in one go).
- Contracts and decisions: `docs/contracts/harness.md` (H-1 to H-4, I-1
  to I-3, the table of functions to keep); ADR 0009, 0026, 0032, 0033;
  `Briefing.md` §8.
- Facts: Calc-U-59 at `v2.3.0`, Calc-U-1600 at `b4ea8c8`.

What the three status columns mean for this feature:
- **Library:** the program `calcu` exists with its one-shot form, its
  `serve` form, its protocol and its first commands, shown with a
  product made for the tests.
- **Calc-U-59, Calc-U-1600:** `calcu` runs the product, and every
  function of the product's tools of today (AC-14, AC-18) is available
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
- The commands of other features: `key` and `type` (LIB-5); the command
  `preset`, the other steps as commands, the bare `wait` that waits until
  the machine is idle, `expect`, `screenshot`, checking presets (LIB-6);
  media (LIB-7); paper (LIB-8); `dump`, `peek`, `poke`, `break`,
  stepping, traces (LIB-10); model, variant, attach, reset (LIB-18).
  Each is specified with its feature (rule 4 of the cut).
- Product commands (`harness.md` H-2) for functions that are carried
  over without a row of their own: owners C59-9 and C1600-5 (OQ-4).
- Attach to a running frontend (owners: C59-5 for the Swift frontend,
  LIB-14 AC-44 for Qt6; Web later, `harness.md` H-4). This feature defines the
  protocol that attach uses.
- A Python helper for `serve` (`harness.md` Q-2: phase 6, with the
  skills).
- GUI automation, and the screenshot scenarios of the Qt6 frontends
  (`--shots`): they are GUI tests of a frontend (ADR 0032) and stay with
  it; they reach the session through harness attach (LIB-14 AC-44).
- The smoke test of the DAP server, `dap_smoke.py` (owner: C1600-3).
- When the old tools are removed (the migration plan).

## Acceptance Criteria

### Library

The program
- [ ] **AC-1** — Given the source repository, then it builds one program
  named `calcu` for the family, which takes the product to run from
  `--product`, in both forms: its value is the product's name as
  written or a short form (`59`, `1600`), matched without regard to
  case (ADR 0026; `harness.md` Q-1; OQ-2).
- [ ] **AC-2** — Given `calcu` with one command or several and their
  arguments, then it builds the machine the product starts with (LIB-18)
  under the default settings, carries out the commands in the order
  given on that one machine, prints each result and ends, with exit
  status zero when all were done and non-zero, with the reason, at the
  first that was refused; the commands follow each other on the command
  line, parted by a word of their own that the design names (OQ-7).
- [ ] **AC-3** — Given `calcu serve`, then it builds the same machine,
  reads one command per line from its standard input and writes one
  reply per command to its standard output, until `quit` or the end of
  the input.
- [ ] **AC-4** — Given either form, then a reply is plain lines of text;
  with `--json` it is one JSON object on one line instead, with the
  same three keys for every command: whether it was done, the result,
  and the reason of a refusal (`harness.md` H-3; OQ-3).
- [ ] **AC-5** — Given a command, then its reply says done, with the
  result where the command reads something, or refused with the reason
  (LIB-3 AC-13); a line that is not a command gets a refusal, and
  `serve` goes on. `serve` writes nothing but replies: notices and lines
  of the log are read with `state`.

How it runs
- [ ] **AC-6** — Given two runs with the same product, options and
  commands, then every reply is the same in both. The harness has no
  wall clock: emulated time passes only by commands, as fast as the host
  allows, and the pacer is not used (`harness.md` I-2; ADR 0009).
- [ ] **AC-7** — Given a command that starts an operation, then the
  harness runs it to its end before it replies (LIB-3 AC-19).
- [ ] **AC-8** — Given a run without options that name them, then: the
  settings are the defaults, and no settings file is read or written;
  the save folder is an empty one made for the run and removed at its
  end, whose path appears in no reply; bundled ROMs, bundled media and
  bundled presets are found as a frontend finds them (LIB-12, LIB-7,
  LIB-6). The traces and screenshots of the run go to that save folder
  and are gone with it. Options name a settings file, a save folder, and
  a folder for the run's traces and screenshots to use instead (LIB-9
  AC-18), for an agent that has to look at them or at its files
  afterwards.
- [ ] **AC-21** — Given a run that hands a real-time clock the host's
  date and time (a build, the end of an operation, `syncclock:`; LIB-2
  AC-23, LIB-6 AC-27), then without an option it hands a fixed instant,
  the same in every run, so that AC-6 holds; an option names a date and
  time to hand instead, or asks for the host's date and time, and with
  the host's a run is not repeatable.

The first commands
- [ ] **AC-9** — Given a product behind the machine interface, then
  these commands work, at the top level, in both forms:

  | Command | What it does | Reply |
  |---|---|---|
  | `wait <duration>` | lets that much emulated time pass; the duration has a unit, `2s` or `500ms`; while the debugger has the machine paused it ends at once (LIB-10 AC-21) | the emulated time that passed, which can exceed the duration by less than one instruction |
  | `key-down <key ID>`, `key-up <key ID>` | calls key down or key up on the machine with that key | done, or refused for an unknown key ID |
  | `lcd` | reads the display from the display topic | the display as text (AC-10) |
  | `state <topic>` | reads a topic by its name | the last state of a state topic, or what a stream topic appended since it was last read |
  | `capabilities` | asks the session what the machine offers now | the capabilities with kind, ID and owner; the places and what is plugged in |
  | `ticks` | asks the session for the machine's emulated time | the tick count and the same in seconds |
  | `audio <file>` | from now on writes all of the machine's sound output to a WAV file | done; the file is complete at the end of the run |
  | `quit` | ends `serve` | none |

- [ ] **AC-10** — Given `lcd`, then the reply gives what the display
  shows as text: for a dot-matrix display the rows of text the dots
  form, the indicators that are on, and how many character cells could
  not be read as text; for a segment display one character per position
  from left to right, by a rule the product supplies for its segment
  layout, and the indicators that are on. When the display is off, the
  reply says so.
- [ ] **AC-11** — Given `state`, then it takes the name of any topic of
  the app session: those of LIB-3 (`display`, `busy`, `notices`, `log`),
  those other features define, and a product's own.
- [ ] **AC-12** — Given the command `wait`, then this feature specifies
  its form with a duration, the preset step's; LIB-6 adds the form
  without a duration, which has no step: the harness counterpart of a
  preset's implicit wait (LIB-6 AC-34; `harness.md` H-1 as reworded
  2026-11-13).
- [ ] **AC-13** — Given a product's commands that only that product has,
  then they are reached under the product's name (`ti59 …`, `pc1600 …`),
  and a command that every product has is at the top level, with
  product-specific output where the hardware differs (`harness.md` H-2).
- [ ] **AC-22** — Given a command whose preset step takes a mapping as
  its value (a reference, `saveas:`), then the command takes that value
  exactly, as a YAML flow mapping after the command's name, by the
  syntax of `docs/formats/README.md` §1: `floppy { name: Disk2, side: 1 }`,
  `card { mem: X, side: 0 }`, `saveas { place: slot-1, as: live, name: My card }`;
  a preset step becomes the command by dropping `- ` and the colon.
- [ ] **AC-23** — Given `display`, then the reply gives the display
  output as the machine gives it (LIB-1 AC-15 to AC-17, AC-43), without
  reading it as text: for a dot-matrix display one line per row of dots,
  each dot `1` or `0`; for a segment display one line per position with
  its segments; then the indicators as a bit string in the order of the
  descriptor, with their names in that order on the line after; and
  whether the display is on. With `--json` the same as arrays.
- [ ] **AC-24** — Given a machine that offers the capability kind
  machine state, when `state-save <file>` is given, then the file holds
  the full state of the machine and its plugged peripherals, in a form
  that is internal to the product and not documented; when
  `state-restore <file>` is given later, in the same run or another,
  then the machine continues from that state so that what follows (the
  display, the emulated time, the sound, the replies) is the same as
  after the save. A restore is refused, and changes nothing, when the
  file comes from another product version, model, variant or set of
  plugged peripherals, or the machine does not offer the kind. Files of
  the host (the save folder, a host drive's folder, a serial port file)
  are not part of the state.

### Calc-U-59
- [ ] **AC-14** — Given the tools of Calc-U-59 at the pin, then each of
  their functions is available through `calcu`, delivered by the feature
  named here:

  | Today | Function | Delivered by |
  |---|---|---|
  | `tools/headless/` (C library, `ti59.py`) | run without a window; press and release keys; let instructions pass | this feature (AC-9) |
  | | drive step by step from a script | this feature (`serve` over a pipe) |
  | | type keys by name, second functions included | LIB-5, LIB-6 |
  | | a record per instruction (address, instruction, registers); disassembly | LIB-10 |
  | | read and write the RAM; read the registers and the SCOM | LIB-10 |
  | | write a program | C59-2 |
  | | the display as text | this feature (AC-10, AC-16) |
  | | printer lines and dot lines; the PC-100C's PRINT, ADV and TRACE | LIB-8; LIB-2 (controls), C59-6 |
  | | insert and eject a card, whether the machine waits for one | LIB-7, C59-7 |
  | | which module is plugged in, with or without printer, the model | LIB-18 |
  | `tools/headless/coverage.py` | which ROM addresses a run reaches | C59-9 (`ti59 coverage`); the script, with its scenarios, stays a script that drives the harness (C59-9 AC-13) |
  | `tools/agentic/ti59file.py` | run a preset and report the emulated time and the printout | LIB-6, LIB-8 |
  | `tools/agentic/linetimes.py`, `runtime.py`, `hotsteps.py`, `stepcost.py` | timings per printed line and per key, the hottest program steps, the cost of a step | C59-9 |
  | `tools/agentic/fastmode_matrix.py` | its table of cases, each run in fast mode and in normal mode | stays a script that drives the harness (C59-9 AC-13) |
  | `tools/agentic/kdis.py`, `asm59.py` | list a program's keycodes; assemble keycodes | C59-9 (`ti59 keycodes list` and `assemble`); the assembler with labels stays a library of the calendar scripts |
  | `tools/read_trace.py` | a trace file as text or JSON | stays a script of the product (LIB-10 AC-33, AC-42; user, 2026-11-10, batch 6 gate) |

  (`tools/headless/capi.cpp:47-196`, `tools/headless/ti59.py:155-437`,
  `tools/agentic/README.md`.)
- [ ] **AC-15** — Given `calcu --product` with Calc-U-59 on macOS, Linux
  and Windows, then it runs. Today the headless tools build on macOS
  only (`tools/headless/README.md:5-6`).
- [ ] **AC-16** — Given a TI machine, then `lcd` gives the twelve
  positions as characters, leftmost first, with the decimal points:
  a digit, a minus sign, the signs for degree, quote and apostrophe, or
  a blank, as `display()` of today's tool does
  (`tools/headless/ti59.py:268-289`); and, new, whether the indicator C
  is on, on a line of its own like the Sharp indicators, its brightness
  only with `--json` (OQ-6).
- [ ] **AC-17** — Given `wait` on a TI machine, then the emulated time
  it reports counts as the tools count today: an instruction in the
  ROM's idle state weighs four on the TI-59 and TI-58 (C59-1 AC-1).

### Calc-U-1600
- [ ] **AC-18** — Given the tools of Calc-U-1600 at the pin, then each
  of their functions is available through `calcu`, delivered by the
  feature named here:

  | Today (`pc1500_cli`, `pc1600_cli`) | Function | Delivered by |
  |---|---|---|
  | `--preset <file>` | build a machine from a preset and run it | LIB-6 |
  | `--check-preset <file>…`, `tools/check_presets.sh` | check that presets can be read | LIB-6 |
  | `--rom`, `--ce1600p-rom` | choose a variant | LIB-18 |
  | `--run-after <ticks>`, the run for a number of ticks | let emulated time pass | this feature (`wait`) |
  | `--lcd-text <file>` | the display as text | this feature (AC-10, AC-19) |
  | `--lcd-png <file>` | the display as an image | LIB-6 (`screenshot`) |
  | `--wav <file>` | the buzzer of the whole run as a WAV file | this feature (`audio`) |
  | `--dump-mem <address>,<length>` | memory as a hex dump | LIB-10 |
  | `--inspect <view>` | an INSPECTOR view as text | C1600-5 (`pc1600 inspect`; C1600-1 AC-19) |
  | `--dump-basic` | the BASIC pointers and the program area | C1600-5 (`pc1600 dump-basic`) |
  | `--tape-in <wav>`, `--tape-out <wav>` | play or record a cassette | LIB-7 (`cassette`, `eject`); C1600-5 (`pc1600 cassette record`, `cassette status`) |
  | `--ce158-pty`, `--ce158-rx`, `--ce158-rx-hold`, `--ce158-tx` | feed and capture the CE-158's serial port | C1600-5 (`pc1600 serial`) |
  | `--save-dir`, `--modules-dir` | the save folder; where module definitions are | this feature (AC-8), LIB-7 |
  | the summary after a run: registers, the last instructions, where the program counter spent its time | | LIB-10 |
  | `pc1600_plotter_probe`, `pc1600_power_probe`, `pc1600_uart_probe` | diagnostics of the plotter, of power-off and of the serial port | not taken over (OQ-5 as amended) |
  | `tools/read_trace.py` | a trace file as text or JSON | stays a script of the product (LIB-10 AC-33, AC-42; user, 2026-11-10, batch 6 gate) |
  | `tools/dap_smoke.py` | a smoke test of the DAP server | C1600-3 |
  | `--shots` of the Qt6 frontend | screenshot scenarios | not a harness function (ADR 0032); a client of harness attach (LIB-14 AC-44) |

  (`tools/pc1500_cli.cpp:92-377`, `tools/pc1600_cli.cpp:90-365`,
  `tools/pc1600_plotter_probe.cpp`, `tools/pc1600_power_probe.cpp`,
  `tools/pc1600_uart_probe.cpp`.)
- [ ] **AC-19** — Given a Sharp machine, then `lcd` gives the rows of
  text, a line with the indicators that are on, and the number of cells
  that are not text, and says "display off" when it is off, as
  `--lcd-text` does today (`tools/pc1500_cli.cpp:271-277`,
  `Core/Display/LcdText.cpp`).
- [ ] **AC-20** — Given `calcu --product` with Calc-U-1600, then it
  finds the bundled ROMs and module definitions as the frontends do, not
  in folders next to where it is run, and it runs the app session. Today
  the tools use their own lookup and do not run the app session
  (`tools/pc1600_cli.cpp:110-141`; `docs/inventory/divergence-matrix.md`,
  M73).

## Edge Cases
- **EC-1** — Given a file of the product's full ROM set is missing or
  bad, then `calcu` ends at its start with an error that names the files
  (LIB-12 AC-7).
- **EC-2** — Given `wait` with a duration without a unit, then it is
  refused.
- **EC-3** — Given the standard input of `serve` ends without `quit`,
  then `serve` ends as after `quit`, and a file being written by `audio`
  is complete.
- **EC-4** — Given `--product` names no product of this build, then
  `calcu` ends with an error that lists the products.
- **EC-5** — Given `state` for a name that is no topic, then it is
  refused.
- **EC-6** — Given `audio` on a machine without sound (every TI
  machine), then it is refused.
- **EC-7** — Given `key-down` for a key that is already down, or
  `key-up` for one that is not, then the reply is done and nothing
  changes (LIB-1 EC-3).

## Technical Requirements (optional)
- The harness runs the library's app session and the product's app
  layer, the same as a frontend does, and reaches the machine only
  through them (`harness.md` I-1).
- The harness is not a frontend and not for end users (glossary); it is
  documented for agents, in the skills, in the same step in which a tool
  of today is replaced (`Briefing.md` §8).
- The serve protocol is the one a frontend exposes for attach
  (`harness.md`, responsibilities; ADR 0033): line-based text, the same
  commands and replies.
- The commands are the preset step vocabulary plus commands to look and
  to control (`harness.md` H-1): a command that is also a step has the
  same name and meaning in both, and is specified once; a command
  replies when its action is carried out, and waiting is asked for with
  `wait`, since the preset's waiting rules have no commands.

## Open Questions
- [x] **OQ-1** — *Decided as recommended (user, 2026-11-10, batch 3 gate).* The names of the first commands (AC-9). `harness.md`
  H-1 names `lcd`, `state` and `quit`; `wait` is a preset step with the
  same meaning. `key-down`, `key-up`, `capabilities`, `ticks` and
  `audio` are proposed here. *Recommendation:* as in AC-9.
- [x] **OQ-2** — *Decided as recommended (user, 2026-11-10, batch 3 gate).* How a run names its product. H-2 says `--product`
  selects it for one-shot runs. *Recommendation:* `--product` in both
  forms, the product's name as written or a short form (`59`, `1600`),
  matched without regard to case.
- [x] **OQ-3** — *Decided as recommended (user, 2026-11-10, batch 3 gate).* What `--json` looks like. C59-5 builds attach on it, so
  it must be fixed before that feature starts. *Recommendation:* one
  object per reply with the same three keys for every command: whether
  it was done, the result, and the reason of a refusal.
- [x] **OQ-4** — *Decided as recommended (user, 2026-11-10, batch 3 gate).* The product commands. AC-14 and AC-18 name functions
  that only one product has and that no row of the index owns
  (coverage, timings and keycode tools of Calc-U-59; BASIC dump, tape
  files, serial peer and INSPECTOR views of Calc-U-1600).
  *Recommendation:* one row per product, "Harness product commands",
  depending on this feature (the open point in `features/INDEX.md`).
- [x] **OQ-5** — *Decided as recommended (user, 2026-11-10, batch 3 gate).* The three probes of Calc-U-1600 and the calendar
  scripts of Calc-U-59 (`tools/agentic/calendar/`) were written for one
  investigation each. `harness.md` lists the probes among the functions
  to keep. *Recommendation:* they stay as scripts that use
  `calcu serve`, not as commands. *Amended (user, 2026-11-13, B-25 of
  `docs/walkthroughs/presets.md`): old scripts and probes that are no
  longer relevant are not taken over; the three probes go, and so does
  `dev/tape-matrix/run.py`; the calendar scripts stay and are upgraded
  to version 2 (C59-2 AC-18).*
- [x] **OQ-6** — *Decided as recommended (user, 2026-11-10, batch 3 gate).* The indicator C in `lcd` for a TI machine (AC-16).
  Today's tool does not report it. *Recommendation:* report it, on a
  line of its own like the Sharp indicators; brightness only with
  `--json`.
- [x] **OQ-7** — *Decided as recommended (user, 2026-11-10, batch 3 gate).* How a one-shot run writes several commands (AC-2).
  Today one call of a Sharp tool combines a preset, a run, the display
  and a WAV file. *Recommendation:* the commands follow each other on
  the command line, separated by a word of their own; decide the word in
  the design.

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
| `dump` belongs to LIB-10 with `peek` and `poke`; the index row of this feature no longer lists it | Memory and registers are read through debug targets | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-11): a run touches no settings file and no save folder of the user unless options say so (AC-8); the first commands and their names (AC-9); a one-shot run can carry several commands (AC-2); `audio` takes all sound output, also that of an operation (AC-9) | LIB-9 left the default to this spec; H-1 names only some commands; today's tools combine several functions in one call and record the buzzer of a whole run | 2026-11-10 |
| Product commands get one row per product in the index (C59-9, C1600-5); the Calc-U-1600 probes and the Calc-U-59 calendar scripts stay scripts that use `calcu serve` | user, 2026-11-10, batch 3 gate; amends the table of functions to keep in `harness.md` | 2026-11-10 |
| Of Calc-U-59's tools, the coverage scenarios and the fast-mode matrix stay scripts that drive the harness; only the addresses reached become a command. AC-14 is worded so | user, 2026-11-10, batch 8 gate; C59-9 OQ-5 | 2026-11-10 |
| Commands reply when carried out; `wait` without a duration is the counterpart of the implicit wait; the preset's waiting rules have no commands (AC-12) | question F-15 of `docs/walkthroughs/presets.md` §5.4, left to the specification by the user on 2026-11-13 ("what works best for agentic use") | 2026-11-13 |
| A run's traces and screenshots go to its own save folder by default; an option names a folder for them (AC-8) | user, 2026-11-13 (B-17 of `docs/walkthroughs/presets.md` §5.2) | 2026-11-13 |
| The harness hands a real-time clock a fixed instant by default; an option names another date and time or the host's (AC-21) | user, 2026-11-13 (B-20 of `docs/walkthroughs/presets.md` §5.2): "a plain fixed instant as default"; today only a test pins the clock (`Core/tests/job_tests.cpp:228-235`) | 2026-11-13 |
| The Qt6 screenshot scenarios are a client of harness attach (Out of Scope, AC-18) | user, 2026-11-13 (B-22 of `docs/walkthroughs/presets.md` §5.2, left to the specification: "what works best for an agentic workflow") | 2026-11-13 |
| The three probes are not taken over; the calendar scripts stay (AC-18, OQ-5 amended) | user, 2026-11-13 (B-25 of `docs/walkthroughs/presets.md` §5.2): "old, no longer relevant scripts and probes are not taken over into the new project. calendar yes, upgrade it" | 2026-11-13 |
| Aligned with `docs/formats/` after the Q&A round: the command `preset`; Qt6 attach (LIB-14 AC-44); tape commands `tape`, `eject` (AC-18) | user rulings of 2026-11-13 (`docs/walkthroughs/presets.md` §5.2, §5.4); `docs/formats/` | 2026-11-13 |
| A command whose step takes a mapping takes the step's value as a YAML flow mapping (AC-22) | user, 2026-11-13 (consistency pass after the Q&A round): one spelling for presets and commands | 2026-11-13 |
| `display` gives the raw display output: dots or segments, and the indicators as a bit string in the descriptor's order (AC-23) | user, 2026-11-14 (debugging and testing through the harness) | 2026-11-14 |
| `state-save` and `state-restore` keep and restore the full machine state in an internal form; a restore is refused for another version or machine configuration (AC-24) | user, 2026-11-14 (debugging and testing through the harness) | 2026-11-14 |
| M-51: a product whose app layer brings the library's parts (Calc-U-1600's `SharpAppPart`: peripherals, presets, startup, control, media, open) hands them to the run (`HarnessProduct::appLayerParts`), which then builds none of its own; `prepare(RunSetup&)` builds the app layer on the run's settings and folders (D-11). Design D.3 took the app layer's parts to hold none of the library's | one set of parts for the frontends and the harness, so no part is in the session twice; the simpler structure (Part D, D.3; qa-report) | 2026-11-16 |
| In the implementation (M-59): Calc-U-59's entry runs its app layer `TIAppPart` (§11.3), as Calc-U-1600's runs `SharpAppPart`; `CALCU59_ROMS_DIR` names another ROM folder for the ROM-missing build | design §11.3; LIB-12's build with one ROM file taken away | 2026-11-17 |
| The cassette's step and command is `cassette` (`cassette { … }`, `eject cassette`), the product command `pc1600 cassette record|status` (AC-18's table edited in place; was `tape`) | user, 2026-11-17 (Part E review Q-6); C1600-5; behaviour change 1600-51 | 2026-11-17 |
