# C59-2: TI preset content

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: C59-1 (the TI machines, the product kind for program memory,
  the golden runs), LIB-6 (the preset loader: the grammar, the place for
  a product's sections, the product's part of a load, the preset list).
- Contracts and decisions: `docs/contracts/presets.md` (PR-3, PR-5,
  PR-6, PR-9, PR-11, I-1 to I-3), `docs/contracts/frontend.md` (FE-6,
  product topics); ADR 0016, 0018, 0021 as amended, 0037;
  `docs/behaviour-changes.md` rows 59-1, 59-3, 59-4, 59-5, 59-10.
- Facts: Calc-U-59 at `v2.3.0`; every `path:line` below is at that tag.

Words of other specs used here: "header key", "section", "step",
"block scalar", "preset list", "idle" (LIB-6), "notice" (LIB-3 AC-10).
A "stage" is today's word for the sections between two key sequences of
a 2.3.0 preset; it is used here only to describe today.

## User Stories
- As a user of a TI machine, I want a preset to put a keystroke program,
  register values, a partition and a cue card into the machine it
  builds.
- As the author of a demo, I want to load more program or data in the
  middle of a preset, after some keys have run.
- As a user, I want every example that Calc-U-59 ships or hosts to do
  after the update what it did before.
- As a user, I want to pick a startup preset for each model.

## Out of Scope
- The grammar, the header keys of the family, the steps, all or nothing,
  the build of the machine and the preset list as such (owner: LIB-6);
  the names of keys in a `key` step (LIB-5).
- Magnetic cards in presets: `medium:` sections, `insert`, `arm`, `fit`
  (owners: LIB-7, C59-7); the debugger steps and `trace` (LIB-10);
  `cut-paper` (LIB-8).
- The magnetic card as a cue card and the cue card of a module program
  (owner: C59-7); how a frontend draws a cue card (the frontends; Swift:
  C59-8).
- Keeping the fitted cue card of a TI-58C (owner: C59-3).
- The presets of Calc-U-1600 (owner: C1600-4).
- A converter for presets of 2.3.0 (ADR 0037: there is none).

## Acceptance Criteria

The sections
- [ ] **AC-1** — Given a preset for a TI model, then it can have these
  sections, each any number of times, each carried out in its place in
  file order when the machine is idle (LIB-6 AC-14, AC-15; `presets.md`
  PR-3):

  | Section | Content | What it does |
  |---|---|---|
  | `partition:` | a value on the same line | sets the partition |
  | `program:` | a block with the one field `steps: \|` | writes the program steps it lists; every other step stays |
  | `registers: \|` | a block scalar | writes the registers it lists; every other register stays |
  | `cue-card: \|` | a block scalar | makes this the blank cue card with its labels, in place of the one before |

  They write through the product kind for program memory (C59-1). A
  machine that a preset has built is empty before its first section
  (ADR 0016), so the first `program:` and `registers:` give what today's
  first stage gives (`Shared/StateApplier.cpp:59-87`).
- [ ] **AC-2** — Given `partition:`, then its value is the number of the
  last program step, optionally followed by a point and digits that are
  not read (`479`, `479.59`); given none, then the machine keeps the
  partition its ROM gives after a cold boot. A value that is not a
  number, or lies beyond the last step of the model (959 on the TI-59,
  479 on the TI-58 and TI-58C), refuses the preset when it is read
  (`Shared/StateFile.cpp:156-164`, `Shared/MachineModelInfo.hpp:80`;
  OQ-1).
- [ ] **AC-3** — Given `steps: |`, then each of its lines is one of:
  keycodes, each two digits, written from step 000 on or from where the
  line before ended; a step number of three digits or more, then one
  keycode, then any text, which is not read; or `...` alone, which
  stands for steps that are left out. A ` #` starts a comment. A step
  number beyond the last step of the model, a keycode that is not two
  digits, and any other word on a line of keycodes refuse the preset
  when it is read (`Shared/StateFile.cpp:26-51`; OQ-2).
- [ ] **AC-4** — Given `registers: |`, then each of its lines is
  `NN = value` for the data register NN, or on the TI-58C `HNN = value`
  for one of the registers H00 to H03; the value is a decimal number
  with an optional exponent. A whole number is stored digit for digit; a
  number with more than 13 digits loses those after the 13th; every
  other value is rounded to 13 digits. A line without `=`, a value that
  is no number, a register the model does not have (60 to 99 on the
  TI-58 and TI-58C; H on another model) refuse the preset when it is
  read (`Shared/StateFile.cpp:54-82`, `Shared/Bcd.cpp:20-65`; OQ-2).
- [ ] **AC-5** — Given `cue-card: |`, then each of its lines is
  `Key: value` with one of these keys, in any case: `Template`
  (`CueCard`, `MagnetCard` or `SolidStateCard`), `Title`, `ID`, `Row1`,
  `Row2`, `Row2R`, `Row1Align`, `Row2Align`, `Row2RAlign` (`left`,
  `center` or `right`), `Style` (`none` or `button`), `PencilColor`
  (`black`, `pencil` or `sharpie`), `Banks`, and the labels `A` to `E`
  and `A'` to `E'`; their meaning and their defaults are today's
  (`Shared/CueCard.cpp:111-229`, `reference/StateFileFormat.md:551-581`).
  A key that is none of these refuses the preset when it is read (OQ-2).
- [ ] **AC-6** — Given a `cue-card:` section was carried out, then a
  product topic says which cue card is to be shown, with its content,
  and every frontend shows it; given a preset without one, then the
  machine it builds has no blank cue card with labels
  (`frontend.md` FE-6; `Shared/SessionController.cpp:174-180`).
- [ ] **AC-7** — Given sections after a step list, then they are what
  2.3.0 calls a reload: they are carried out when the keys before them
  are done and the machine is idle, and they change only what they list
  (`Shared/SessionController.cpp:707-724`,
  `tests/shared_preset_stages_tests.cpp`,
  `tests/shared_state_applier_tests.cpp`).

The product's part of a load
- [ ] **AC-8** — Given LIB-6 AC-53, then Calc-U-59 tells the loader for
  the TI-59, the TI-58 and the TI-58C when the machine is idle (LIB-6
  AC-36), that nothing is done after the build, and the picture of the
  display: the twelve positions and the indicator in the classic segment
  style, fixed by a reference picture in the tests (LIB-6 OQ-3 as
  decided).

Commands
- [ ] **AC-9** — Given the harness with Calc-U-59, then these commands
  do what the sections do: `ti59 partition <value>`,
  `ti59 program [<step>] <keycode>…` and `ti59 registers <NN>=<value>…`;
  each is refused where the section would refuse the preset (LIB-11
  AC-13, AC-14; today `tools/headless/ti59.py:265-266`).

The shipped and hosted presets
- [ ] **AC-10** — Given the 57 presets under `examples/` and the presets
  of the tests, then each is in version 2 (ADR 0037; behaviour change
  59-1), written from its 2.3.0 file by these rules:

  | 2.3.0 | Version 2 |
  |---|---|
  | no version | `format-version: 2` |
  | `MODEL:`; none | `model:` with that model; the model of the file's extension |
  | `SOLID-STATE-MODULE: XX`; none | `slot-1: XX`; `slot-1: ML` |
  | `PRINTER: on`; none; `PRINTER: off` | `printer: PC-100C`; `printer: PC-100C`; no key |
  | `PARTITION:`, `PROGRAM:`, `REGISTERS:`, `CUECARD:` | the sections of AC-1, in the order partition, program, registers, cue card within what was one stage |
  | `KEYSTROKES:` with matrix codes | `keys:` with `- key:` steps that name the same keys (LIB-5 AC-26); words between the codes become comments |
  | the first `KEYSTROKES:` not starting with `FullSpeed:` | its step list starts with `- speed: regular` |
  | `FullSpeed:`, `RegularSpeed:`, `Wait:`, `ZeroElapseTime:`, `ReportElapseTime:` | `- speed:`, `- wait:`, `- elapsed:` (LIB-6) |
  | `PrinterTrace:`, `CutPaper:`, `Trace:`, the debugger directives | `- switch: PC-100C TRACE …` (LIB-6), `- cut-paper:` (LIB-8), the steps of LIB-10 |
  | `CARD:` blocks, `ArmCard:`, `InsertCard:` | as LIB-7 and C59-7 write them |

  The module and the printer that a 2.3.0 preset did not name were those
  of the running calculator; the rules write what a fresh 2.3.0 had: the
  Master Library module and the printer connected (behaviour change
  59-4).
- [ ] **AC-11** — Given each of the 57 examples, when its version 2 file
  is loaded through the harness to its end, then the display, the
  program steps, the registers, the partition and the printout are what
  its 2.3.0 file gives in a fresh 2.3.0, recorded there as a golden run
  before the files are upgraded (C59-1, golden runs; LIB-8 AC-23).
- [ ] **AC-12** — Given the copies of the examples that calc-u.org hosts
  under `59/software-files/`, then they are the version 2 files of
  AC-10; the presets under `59/calc/presets/` come with the Web
  frontend's release (LIB-6 AC-43).
- [ ] **AC-13** — Given the preset list of Calc-U-59 (LIB-6 AC-31), then
  it has three startup presets, one for each model, each with the Master
  Library module in `slot-1` and the PC-100C in `printer` and nothing
  else (ADR 0021 as amended), and as examples the ten presets the Web
  frontend bundles today, in today's order, with today's title as their
  `description` (`Web/presets.json:1-12`; their names and groups: OQ-3).
- [ ] **AC-14** — Given the Swift, Qt6 and Web frontends, then each
  bundles the preset list and its files; today only the Web frontend
  bundles presets (`Web/build.js:73-79`; behaviour change 59-14).
- [ ] **AC-15** — Given the preset list, then the test suite fails
  unless every preset in it has a `preset-name` of at most 18 characters
  and a `description`, is listed once, and shares its name with no other
  listed preset of the same model (as C1600-4 AC-18).
- [ ] **AC-16** — Given what the tests of Calc-U-59 check today about
  writing programs and registers, then it holds: the default partition
  and the limits of the TI-58, a section changing only what it lists,
  the data registers matching what the ROM reads, and how a value
  becomes the digits of a register, by the single-line records `REG`,
  `BCDD`, `BCDT` and `MATH` of `tests/golden/formats.txt`
  (`tests/shared_state_applier_tests.cpp`,
  `tests/shared_formats_tests.cpp`).

## Edge Cases
- **EC-1** — Given a `program:` whose steps reach beyond the partition
  in force, then they are written all the same, as today, and lie where
  the calculator keeps registers (`Shared/StateApplier.cpp:25-34`).
- **EC-2** — Given `registers:` before `partition:` in a file, then they
  are carried out in that order (LIB-6 AC-14); what a register then
  holds follows from the calculator's memory, not from this spec.
- **EC-3** — Given a preset for the TI-58C with `H00` to `H03`, then
  they are written after the cold boot; a preset without them leaves
  them empty (C59-3 AC-8). Today a preset load keeps what they held
  (`Shared/StateApplier.cpp:67-73`; behaviour change 59-12).
- **EC-4** — Given two `cue-card:` sections, then the second replaces
  the first when its turn comes.
- **EC-5** — Given a line of a `cue-card:` block whose key is `Model`,
  `Printer` or `Card`, then it is refused as an unknown key; today such
  a line is taken as a directive of the preset
  (`Shared/StateFile.cpp:376-395`).
- **EC-6** — Given an example of 2.3.0 whose extension names another
  model than its `MODEL:` line (`historic/trace-quirk.ti59`), then its
  version 2 file keeps both (LIB-6 AC-5).

## Technical Requirements (optional)
- These sections are added through the place LIB-6 AC-29 gives; the
  loader itself knows none of them.
- The readers of the 2.3.0 format and the `STATE` records of
  `tests/golden/formats.txt` are not carried over (LIB-6).
- The golden runs of AC-11 are recorded with 2.3.0 before any core code
  is touched (C59-1).

## Open Questions
- [ ] **OQ-1** — Which values `partition:` takes (AC-2). Today any value
  is rounded up to the next multiple of 80 steps between 80 and 960, a
  value that cannot be read counts as 479 without a word, and negative
  values pass (`Shared/StateFile.cpp:156-164, 450-458`).
  *Recommendation:* only the partitions the model's ROM can set, taken
  from the research (`ti59-research`); every other value refuses the
  preset.
- [ ] **OQ-2** — Strictness inside the blocks (AC-3 to AC-5). Today a
  word that is no keycode, a line without `=`, an unknown cue-card key
  and a step number out of range are passed over without a word.
  *Recommendation:* each refuses the preset, as ADR 0017 asks; the
  examples are rewritten anyway.
- [ ] **OQ-3** — The names and groups of the ten examples (AC-13). They
  have titles of up to 43 characters and no groups today.
  *Recommendation:* the groups "Diagnostics" (`diag.ti59`, `diag.ti58`,
  `ram_test.ti59`), "Programs" (`sum.ti59`, `characters.ti59`,
  `fastmode-sst.ti59`) and "Printer pictures" (`printer-interrupt.ti59`,
  `picard.ti59`, `globe-print.ti59`, `switzerland.ti59`); short names
  chosen when the files are upgraded and checked by AC-15.
- [ ] **OQ-4** — The three startup presets (AC-13) connect the PC-100C,
  as ADR 0021 says, while the built-in default machine has none
  (behaviour change 59-10). No startup preset is set at first.
  *Recommendation:* as written; the user who wants today's start, with
  the printer, picks the startup preset once.

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| TI bulk content as block scalars in today's notation: `program:` with `steps: \|`, `registers: \|`, `cue-card: \|`, `partition:`; reloads are repeated sections in file order | `presets.md` PR-3 | 2026-11-10 |
| ` #` starts a comment inside the TI blocks | `presets.md` PR-6 | 2026-11-10 |
| A preset builds its machine from a cold boot; a TI preset names its module and its printer | ADR 0016; `presets.md` PR-9, PR-11; behaviour change 59-4 | 2026-11-10 |
| Presets of 2.3.0 are refused; every shipped and hosted preset is upgraded and checked | ADR 0037; behaviour change 59-1 | 2026-11-10 |
| Three bundled startup presets, one per model: Master Library module, printer attached; none restores `ti58c.mem` | ADR 0021 as amended | 2026-11-10 |
| The picture of a TI display for `screenshot:` is supplied by the product, in the classic segment style | LIB-6 OQ-3 (user, batch 5 gate) | 2026-11-10 |
| Proposed in this spec, for the gate: `partition:` is a section that is only a value | `presets.md` PR-3 lists it with the sections; a partition can change in a reload today | 2026-11-10 |
| Proposed in this spec, for the gate: an upgraded example names the Master Library module and the PC-100C where its 2.3.0 file named none, and starts its keys with `- speed: regular` | It keeps what the example does in a fresh 2.3.0 | 2026-11-10 |
| Proposed in this spec, for the gate: the comparison of AC-11 as the proof that the upgrade changed nothing | ADR 0037 asks for the upgrade, nothing says how it is checked | 2026-11-10 |
| Proposed in this spec, for the gate: the commands of AC-9 | LIB-11 AC-14 assigns "write a program" to this feature | 2026-11-10 |
