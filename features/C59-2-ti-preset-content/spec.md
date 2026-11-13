# C59-2: TI preset content

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: C59-1 (the TI machines), C59-6 (the module slot and the
  PC-100C, which the upgraded presets name), LIB-6 (the preset loader:
  the grammar, the place for a product's sections, the product's part of
  a load, the preset list), LIB-8 (the paper, for comparing what was
  printed), LIB-10 (reading memory through the harness; the debugger
  steps of four examples). The four examples with magnetic cards are
  upgraded and compared with C59-7 (AC-10).
- The format: `docs/formats/preset.md` §4 (the sections), §8.1 (key
  IDs), §9.1 (from version 1), and `docs/formats/cue-card.md` (user,
  2026-11-12). Where an earlier decision below and the format differ,
  the format holds; the decision log says where.
- Contracts and decisions: `docs/contracts/presets.md` (PR-3, PR-5,
  PR-6, PR-9, PR-11, I-1 to I-3), `docs/contracts/frontend.md` (FE-6,
  product topics); ADR 0016, 0018, 0021 as amended, 0037;
  `docs/behaviour-changes.md` rows 59-1, 59-3, 59-4, 59-5, 59-10.
- Facts: Calc-U-59 at `v2.3.0`; every `path:line` below is at that tag.

Words of other specs used here: "header key", "section", "step",
"block scalar", "preset list", "idle" (LIB-6), "notice" (LIB-3 AC-10).
A "stage" is today's word for the sections between two key sequences of
a 2.3.0 preset, and a "reload" for a stage after the first; both are
used here only to describe today. A **keycode** is the two-digit code of
one program step (glossary, Program step); it is not a key ID. A **data
register** is one of the numbered registers a keystroke program stores
values in. A **reference run** is a record of what a 2.3.0 preset leaves
behind, made with 2.3.0 (AC-11).

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
- Magnetic cards in presets: the header key `magnetic-cards:`, the
  steps `card`, `arm` and `saveas` with `card:` (owners: C59-7, LIB-7); the debugger steps and `trace` (LIB-10);
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
  | `partition:` | a value on the same line (`preset.md` §4.3) | sets the partition |
  | `program:` | exactly one of `keycodes: \|` (the program inline) and `file:` (a keystroke program file, `.keystrokes.yaml`; `preset.md` §4.1, §4.1.1) | writes the program steps it lists, from step 000; every other step stays |
  | `data:` | a mapping `NN: value` (`preset.md` §4.2) | writes the registers it lists; every other register stays |
  | `cue-card:` | a cue card block (`cue-card.md` §3) | fits it as a stand-alone cue card, the machine's own card (glossary, Fit; `cue-card.md` §2), in place of the one fitted before |

  They write through the product kind for program memory (AC-17). A
  machine that a preset has built is empty before its first section
  (ADR 0016), so the first `program:` and `data:` give what today's
  first stage gives (`Shared/StateApplier.cpp:59-87`).
- [ ] **AC-17** — Given a TI machine, then it lists a product kind
  "program memory", which this feature defines (C59-1, out of scope
  there): through it a program step, a data register and, on the TI-58C,
  one of the registers H00 to H03 can be read and written, and the
  partition can be read and set; what was written reads back.
- [ ] **AC-2** — Given `partition:`, then its value is the number of the
  last program step, optionally followed by a point and digits that are
  not read (`479`, `479.59`); the number of steps (the value + 1) is
  rounded up to a multiple of 80 (10 program registers), at most 960;
  given none, then the machine keeps the partition its ROM gives after a
  cold boot (479 on the TI-59, 239 on the TI-58 and TI-58C). A value
  that is not a number, or that is above the model's last step (959 on
  the TI-59, 479 on the TI-58 and TI-58C), refuses the preset when it
  is read (`preset.md` §4.3; today `Shared/StateFile.cpp:156-164`,
  `Shared/MachineModelInfo.hpp:80`). This replaces OQ-1's "only the
  partitions the ROM can set" (decision log).
- [ ] **AC-3** — Given `keycodes: |`, then it is read in the keycode
  notation of `preset.md` §4.1, and a ` #` starts a comment: on a line
  of bare keycodes every token of one or two digits with a value 0 to
  99 is a keycode, written to the steps that follow the last step
  written, and every other token is a label and is passed over; a line
  whose first token has three or more digits is a step number, which
  sets the position, then one keycode, then a label that is not read,
  even a number (`473 00 0`); a line that is exactly `...` writes
  nothing, and the position moves on at the next step number. A step
  number beyond the model's last step (959 on the TI-59, 479 on the
  TI-58 and TI-58C) refuses the preset when it is read. Given `file:`,
  then the file is a keystroke program file (`preset.md` §4.1.1, format
  `ti-keystroke-program`, version 1), read by the same rules, and one
  that cannot be read or breaks its format refuses the preset. This
  replaces OQ-2's refusal of a word on a line of keycodes (decision
  log); what is still open about the notation is OQ-5.
- [ ] **AC-4** — Given `data:`, then each of its entries is `NN: value`
  for the data register NN (00 to 99), or on the TI-58C `HNN: value` for
  one of the cells H00 to H03, which are not registers 60 to 63; the
  value is a decimal number with an optional exponent. A whole number
  is stored digit for digit; a number with more than 13 digits loses
  those after the 13th; every other value is rounded to 13 significant
  digits. A value that is no number and a register the model does not
  have (60 to 99 on the TI-58 and TI-58C; H on another model) refuse
  the preset when it is read (`preset.md` §4.2;
  `Shared/StateFile.cpp:54-82`, `Shared/Bcd.cpp:20-65`; OQ-2).
- [ ] **AC-5** — Given `cue-card:`, then it is a cue card block of the
  stand-alone kind (`cue-card.md`): the keys `title`, `row-1`, `row-2`,
  `row-2-r`, `row-1-align`, `row-2-align`, `row-2-r-align`, `style`,
  `pencil-color`, the labels `A` to `E` and `A'` to `E'` (`A′` to `E′`
  accepted, compared in any case), with the defaults, the `\blank`
  cell, the empty label and the text tokens of `cue-card.md` §3, §4. A
  stand-alone card always looks like the plain, hand-written card; there
  is no `Template` and no `Banks`, and `id` belongs to module cards
  only. A key that is not one of these, or does not apply to the kind,
  refuses the preset when it is read (`cue-card.md` §3; OQ-2). This
  replaces the decision that `Template` chooses the look (decision
  log).
- [ ] **AC-6** — Given a `cue-card:` section was carried out, then the
  product topic `cue-card` says which cue card is fitted, with the
  content of AC-5, and the Qt6 and Web frontends draw it in the window
  above the top row of keys (Swift: C59-8 AC-20); a magnetic card that
  is later inserted or armed is fitted in its place (C59-7;
  `cue-card.md` §2). Given a preset without the section, then the
  machine it builds has no such cue card. The GUI test of the Qt6 and of
  the Web frontend has one scenario under this ID: a preset with a
  `cue-card:` is loaded and its title shows
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
  each is refused where the section would refuse the preset. Without a
  value they read: `ti59 partition` gives the partition, `ti59 program
  <step> [<count>]` the keycodes, `ti59 registers <NN>…` the registers
  (LIB-11 AC-13, AC-14; today `tools/headless/ti59.py:265-266`).

The shipped and hosted presets
- [ ] **AC-10** — Given the 57 presets under `examples/`, and those
  presets under `tests/fixtures/` that the new tests still load, then
  each is in version 2 (ADR 0037; behaviour change
  59-1), written from its 2.3.0 file by the table of `preset.md` §9.1
  and these rules, which settle for Calc-U-59 what §9.1 leaves open:

  | 2.3.0 | Version 2 |
  |---|---|
  | no version | `format-version: 2` |
  | `MODEL:`; none | `model:` with that model; the model of the file's extension |
  | `SOLID-STATE-MODULE: XX`; none | `slot-1: { name: XX }`; `slot-1: { name: ML }` |
  | `PRINTER: on`; none; `PRINTER: off` | `printer: PC-100C`; `printer: PC-100C`; no key |
  | `PARTITION:`, `PROGRAM:`, `REGISTERS:`, `CUECARD:` | the sections of AC-1 (`partition:`, `program:` with `keycodes: \|`, `data:`, `cue-card:`), in the order partition, program, data, cue card within what was one stage; the cue card keys by `preset.md` §9.3, `Template` and `Banks` dropped |
  | section names in any case | the lower-case keys of version 2 |
  | `#` anywhere in a line | a `#` line, or ` #` after the value |
  | words between the keycodes of `PROGRAM:` | labels, which the notation passes over (AC-3), or a comment after ` #` |
  | `KEYSTROKES:` with matrix codes | `keys:` with one `- key:` step for each 2.3.0 line, naming the same keys in the same order by the key IDs of `preset.md` §8.1, a second function by its name; the words of the line become a comment after the step |
  | the first `KEYSTROKES:` not starting with `FullSpeed:` | its step list starts with `- regular-speed:`, which holds until a `- full-speed:` (LIB-6 AC-22) |
  | `FullSpeed:`, `RegularSpeed:`, `Wait:`, `ZeroElapseTime:`, `ReportElapseTime:` | `- full-speed:`, `- regular-speed:`, `- wait:`, `- elapsed-time: start`, `- elapsed-time: report` (LIB-6) |
  | the last key of a file, whose run 2.3.0 waited for at full speed | `- let-it-finish:` after it where that run is part of the result (`preset.md` §9.1); a step that has to reach a running program gets `- skip-wait:` before it (LIB-6 AC-58) |
  | `PrinterTrace:`, `CutPaper:`, `Trace:`, the debugger directives | `- printer-trace:` (LIB-6 AC-40), `- cut-paper:` (LIB-8), the steps of LIB-10 |
  | `CARD:` blocks, `ArmCard:`, `InsertCard:` | `magnetic-cards:`, `- card:`, `- arm:`, `- saveas:` as C59-7 writes them (`preset.md` §9.1) |

  The module and the printer that a 2.3.0 preset did not name were those
  of the running calculator; the rules write what a fresh 2.3.0 had: the
  Master Library module and the printer connected (behaviour change
  59-4). The four examples with magnetic cards (`globe-print`,
  `historic/characters-fastmode`, `historic/pdcm-card-tricks`,
  `historic/pdcm-make-cards`) are upgraded and compared when C59-7 is
  built; every other example with this feature.
- [ ] **AC-11** — Given each example, when its version 2 file is loaded
  through the harness and the machine is then idle, or an hour of
  emulated time has passed (the harness `wait`, LIB-6 AC-34), then the display (`lcd`), the
  program steps, the registers and the partition (AC-9) and the paper
  (`paper text`, `paper marks`; LIB-8 AC-39) are those of its reference
  run. A reference run is recorded before the files are upgraded, with
  the tools of 2.3.0 (`tools/agentic/ti59file.py`, `tools/headless/`),
  from the 2.3.0 file, with the model of AC-10, the Master Library
  module, the printer connected and no `ti58c.mem`, up to the same
  moment.
- [ ] **AC-12** — Given the copies of the examples that calc-u.org hosts
  under `59/software-files/`, then they are the version 2 files of
  AC-10, copied there with the site's own tools; the presets under
  `59/calc/presets/` are those of the preset list, which come with the
  Web frontend's release (LIB-6 AC-43).
- [ ] **AC-13** — Given the preset list of Calc-U-59 (LIB-6 AC-31), then
  it has three startup presets, one for each model, each with the Master
  Library module in `slot-1` and the PC-100C in `printer` and nothing
  else (ADR 0021 as amended), and as examples the ten presets the Web
  frontend bundles today, in today's order, with today's title as their
  `description`, in the groups "Diagnostics" (`diag.ti59`, `diag.ti58`,
  `ram_test.ti59`), "Programs" (`sum.ti59`, `characters.ti59`,
  `fastmode-sst.ti59`) and "Printer pictures" (`printer-interrupt.ti59`,
  `picard.ti59`, `globe-print.ti59`, `switzerland.ti59`), each under a
  short name chosen when the files are upgraded and checked by AC-15
  (`Web/presets.json:1-12`; OQ-3).
- [ ] **AC-14** — Given the Swift, Qt6 and Web frontends, then each
  bundles the preset list and its files, so that the scenario of LIB-6
  AC-42 can load an example in each; today only the Web frontend bundles
  presets (`Web/build.js:73-79`; the Examples menu: LIB-14 AC-26).
- [ ] **AC-15** — Given the preset list, then the test suite fails
  unless every preset in it has a `preset-name` of at most 18 characters
  (`docs/formats/README.md` §2) and a `description`, is listed once, and shares its name with no other
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
- **EC-2** — Given `data:` before `partition:` in a file, then they
  are carried out in that order (LIB-6 AC-14); what a register then
  holds follows from the calculator's memory, not from this spec.
- **EC-3** — Given a preset for the TI-58C with `H00` to `H03`, then
  they are written after the cold boot; a preset without them leaves
  them empty (C59-3 AC-8). Today a preset load keeps what they held
  (`Shared/StateApplier.cpp:67-73`; behaviour change 59-12).
- **EC-4** — Given two `cue-card:` sections, then the second replaces
  the first when its turn comes.
- **EC-5** — Given a `cue-card:` block with the key `model`,
  `printer` or `card`, then it is refused as an unknown key; today such
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
- The reference runs of AC-11 are recorded with 2.3.0 before any core
  code is touched, together with the golden runs of C59-1.

## Open Questions
- [x] **OQ-1** — *Decided as recommended (user, 2026-11-10, batch 6 gate).* Which values `partition:` takes (AC-2). Today any value
  is rounded up to the next multiple of 80 steps between 80 and 960, a
  value that cannot be read counts as 479 without a word, and negative
  values pass (`Shared/StateFile.cpp:156-164, 450-458`).
  *Recommendation:* only the partitions the model's ROM can set, taken
  from the research (`ti59-research`); every other value refuses the
  preset.
- [x] **OQ-2** — *Decided as recommended (user, 2026-11-10, batch 6 gate).* Strictness inside the blocks (AC-3 to AC-5). Today a
  word that is no keycode, a line without `=`, an unknown cue-card key
  and a step number out of range are passed over without a word.
  *Recommendation:* each refuses the preset, as ADR 0017 asks; the
  examples are rewritten anyway.
- [x] **OQ-3** — *Decided as recommended (user, 2026-11-10, batch 6 gate).* The names and groups of the ten examples (AC-13). They
  have titles of up to 43 characters and no groups today.
  *Recommendation:* the groups "Diagnostics" (`diag.ti59`, `diag.ti58`,
  `ram_test.ti59`), "Programs" (`sum.ti59`, `characters.ti59`,
  `fastmode-sst.ti59`) and "Printer pictures" (`printer-interrupt.ti59`,
  `picard.ti59`, `globe-print.ti59`, `switzerland.ti59`); short names
  chosen when the files are upgraded and checked by AC-15.
- [x] **OQ-4** — *Decided as recommended (user, 2026-11-10, batch 6 gate).* The three startup presets (AC-13) connect the PC-100C,
  as ADR 0021 says, while the built-in default machine has none
  (behaviour change 59-10). No startup preset is set at first.
  *Recommendation:* as written; the user who wants today's start, with
  the printer, picks the startup preset once.

- [ ] **OQ-5** — Two points of the keycode notation that `preset.md`
  §4.1 leaves open: whether a one-digit token on a line of bare keycodes
  (`1`) is the keycode `01` or a label, and where a line of bare
  keycodes right after `...` writes. 2.3.0 is the fact for both
  (`Shared/StateFile.cpp:26-51`).

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| TI bulk content as block scalars in today's notation: `program:` with `steps: \|`, `registers: \|`, `cue-card: \|`, `partition:`; reloads are repeated sections in file order | `presets.md` PR-3 | 2026-11-10 |
| ` #` starts a comment inside the TI blocks | `presets.md` PR-6 | 2026-11-10 |
| A preset builds its machine from a cold boot; a TI preset names its module and its printer | ADR 0016; `presets.md` PR-9, PR-11; behaviour change 59-4 | 2026-11-10 |
| Presets of 2.3.0 are refused; every shipped and hosted preset is upgraded and checked | ADR 0037; behaviour change 59-1 | 2026-11-10 |
| Three bundled startup presets, one per model: Master Library module, printer attached; none restores `ti58c.mem` | ADR 0021 as amended | 2026-11-10 |
| The picture of a TI display for `screenshot:` is supplied by the product, in the classic segment style | LIB-6 OQ-3 (user, batch 5 gate) | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 6 gate): `partition:` is a section that is only a value | `presets.md` PR-3 lists it with the sections; a partition can change in a reload today | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 6 gate): an upgraded example names the Master Library module and the PC-100C where its 2.3.0 file named none, and starts its keys with `- speed: regular` | It keeps what the example does in a fresh 2.3.0 | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 6 gate): the comparison of AC-11 with reference runs as the proof that the upgrade changed nothing | ADR 0037 asks for the upgrade, nothing says how it is checked | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 6 gate): this feature defines the product kind for program memory (AC-17) | C59-1 leaves it to this feature | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 6 gate): `cue-card:` fits a cue card; `Template` chooses only its look (AC-1, AC-5) | Today's three templates are artwork | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 6 gate): the check of the preset list is the one of C1600-4 AC-18, the 18 characters included (AC-15) | One rule for both products' lists | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 6 gate): the commands of AC-9 | LIB-11 AC-14 assigns "write a program" to this feature | 2026-11-10 |
| ` #` starts a comment in `registers: \|` and `cue-card: \|` too (AC-4, AC-5): the decision of the second row above was written into AC-3 only | Phase 5, preset pass (`docs/walkthroughs/presets.md`, A-1); `presets.md` PR-6 | 2026-11-11 |
| The sections are those of `docs/formats/preset.md` §4: `program:` with `keycodes: \|` or `file:` (a keystroke program file), `data:` as a mapping, `cue-card:` as a cue card block (AC-1, AC-3 to AC-5); replaces `steps: \|`, `registers: \|`, `cue-card: \|` of PR-3 | user, 2026-11-12 (`docs/formats/`) | 2026-11-13 |
| `partition:` rounds the steps up to a multiple of 80, at most 960; only a value above the model's last step is refused (AC-2). Replaces OQ-1 (batch 6 gate) | user, 2026-11-12 (`preset.md` §4.3) | 2026-11-13 |
| A token that is not a keycode on a line of bare keycodes is a label and is passed over (AC-3). Replaces that part of OQ-2 (batch 6 gate) | user, 2026-11-12 (`preset.md` §4.1) | 2026-11-13 |
| A stand-alone cue card has no `Template` and no `Banks`; its kind follows from where it stands, so a preset's card always looks like the plain card (AC-5). Replaces "`Template` chooses only its look" (batch 6 gate); six examples lose their look (`docs/research/preset-conversion-check.md` §1) | user, 2026-11-12 (`cue-card.md` §1, `preset.md` §9.3) | 2026-11-13 |
| The upgrade follows `preset.md` §9.1; the rules of AC-10 settle for Calc-U-59 what the conversion guide leaves open there (S3 `regular-speed:` first, S9 the model from the suffix, S10 program before data), and add `let-it-finish:` and `skip-wait:` where the run needs them | user, 2026-11-12 (`preset.md` §9.1; conversion check §3); AC-10 as decided at the batch 6 gate | 2026-11-13 |
