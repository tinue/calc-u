# LIB-6: Presets

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: LIB-2 (places, what fits them, the controls and real-time
  clock kinds), LIB-5 (the steps `key` and `type`, typing profiles),
  LIB-9 (the `[preset]` settings, the folder for screenshots), LIB-11 (the harness,
  `wait <duration>`, the display as text), LIB-18 (building a machine,
  the start of a product, what ends a running operation); through them
  LIB-1 (descriptors, key readiness) and LIB-3 (operations, the busy
  topic, notices, the log).
- The format: `docs/formats/` (the user, 2026-11-12): `README.md` (the
  syntax, version, name, references, errors, platform markers) and
  `preset.md` (preset format 2). It says what a preset holds and means;
  this spec says what the loader carries out and how it is checked.
  Where an earlier decision below and the format differ, the format
  holds; the decision log says where.
- Contracts and decisions: `docs/contracts/presets.md` (PR-1 to PR-11,
  I-1 to I-3; the syntax of its forks is superseded by `docs/formats/`), `docs/contracts/typing.md` (T-4),
  `docs/contracts/frontend.md` (Q-1 as amended, Q-3, SP-2); ADR 0016,
  0017, 0018, 0021, 0037; `docs/behaviour-changes.md` rows 59-1, 59-3,
  59-4, 59-5, 1600-1, 1600-3, 1600-9.
- Facts: Calc-U-59 at `v2.3.0`, Calc-U-1600 at `b4ea8c8`; every
  `path:line` below is at those pins.

Glossary terms that this spec introduced:
- A **header key** is a top-level key of a preset that describes the
  preset or its machine, wherever it stands; it has a value on its line
  or, where its owner says so, a block (`bus-rom:`, `debug:`, C1600-4).
- A **section** is a top-level key with a block below it, or, where its
  owner says so, with only a value (`partition: 479`, C59-2). Sections
  are carried out in file order.
- A **step** is one item of a step list (`- verb: value`). It is not a
  program step (glossary).
- A machine is **idle** when it has finished what it was doing and waits
  for the user; a TI-59 that waits for a magnetic card is idle too
  (glossary; `preset.md` §6.2). A paused machine is not idle. Each
  product says for its models how this is told from what the machine
  reports (AC-53, AC-36, AC-45).
- Since 2026-11-12 also in the glossary: **implicit wait**, **skip
  wait**, **let it finish** (`preset.md` §6). An **item** is a section
  or a step.
- A **reference** names a bundled preset without a path:
  `examples:<path inside the product's bundled presets>`.
- The **preset list** of a product names the presets it bundles: its
  startup presets and its examples. It is not the catalogue of the
  glossary, which lists descriptors.

Words of other specs used here: "reply" and "notice" (LIB-3 AC-13,
AC-10), "live input" and "prompt readiness" (LIB-5 AC-2, AC-14).

What the three status columns mean for this feature:
- **Library:** the loader, the grammar, the step engine, the preset list
  and the harness commands exist and are shown to work with machines and
  sections built for the tests.
- **Calc-U-59, Calc-U-1600:** every preset of the product is read and
  carried out by this loader only, in every frontend. The columns are
  worked together with C59-2 and C1600-4, which bring the product
  sections and the upgraded files.

## User Stories
- As a user, I want a preset to give me the same machine every time, so
  that I can share it and rely on it.
- As a user, I want a preset with a mistake to be refused with the line
  and the reason, and my running machine left alone.
- As the author of a preset, I want one preset language for every
  product, so that what I learned for one works for the other.
- As the author of a demo, I want to slow a preset down to regular speed
  for the part the viewer should see.
- As a user, I want to pick an example from a list, and to choose the
  preset a model starts with.
- As a developer or an agent, I want to load a preset through the
  harness and to check a set of presets without running them.

## Out of Scope
- How a key is pressed and how text is typed, the names a `key` takes
  (owner: LIB-5).
- Media and modules in presets: references by `name:`, `file:` and
  `mem:` (`docs/formats/README.md` §3), the header keys `floppy:` and
  `tape:`, the step `saveas` (owner: LIB-7); the header key
  `magnetic-cards:` and the steps `card` and `arm` (owner: C59-7).
- The steps `cut-paper` (owner: LIB-8) and `debug-panel`, `pause`,
  `pause-on-start`, `step`, `resume`, `trace` (owner: LIB-10; the open
  point "Traces" of `features/INDEX.md`).
- The product sections and keys and their content: `partition:`,
  `program:` with `keycodes:` or `file:`, `data:`, `cue-card:` (owner:
  C59-2); `program:` with `file:` and `text:`, the value of
  `host-drive:`, `bus-rom:`, `slot-N-rom:`, `debug:` (owner: C1600-4).
- Which presets a product ships, their upgrade to version 2, and the
  content of each product's preset list (owners: C59-2, C1600-4).
- The ways to open a preset: drop, Open With, file pickers, and how a
  file is recognised as a preset (owner: LIB-13); the menu items, the
  lists, "Loading…" and how a notice is shown (LIB-14, C59-8); the
  `preset` prop of a Web frontend (LIB-15).
- What Reset, Cold boot and a model switch do, and the start of a
  product (owner: LIB-18).
- A converter for older files (ADR 0037: there is none).

## Acceptance Criteria

### Library

The file
- [ ] **AC-1** — Given a preset, then it is a text file in preset format
  version 2 as `docs/formats/preset.md` specifies it, read by the syntax
  of `docs/formats/README.md` §1 (simplified YAML: block and flow
  mappings and sequences, quoted scalars without escapes, `|` block
  scalars, `#` comments, the raw values of `type:` and `expect:`, numbers
  with `&`, `0x` or `$` for hex, booleans `true` and `false`, a tab
  anywhere an error, an unknown key an error at every level) and the
  rules of its §2 to §5 (version, name and description, references,
  errors, platform markers). Paths not absolute are taken from the
  folder of the preset, `~/` is the user's home folder (README §3).
  Names of models, variants and peripherals are matched without regard
  to case; their hyphen is required (`PC-1600`, `pc-1600`, not
  `PC1600`). Header keys may stand anywhere and appear once each;
  sections repeat and run in file order (`preset.md` §1). The syntax
  holds for the whole file, so a product's sections and blocks (C59-2,
  C1600-4) are read by it too; no block is handed to its owner as raw
  lines.

- [ ] **AC-2** — Given a preset, then its header keys are:
  `format-version` (required; the value 2), `model` (required; a model
  of the product, optionally followed by `:` and one of its variants),
  `preset-name` (optional; at most 18 characters; without it the
  preset's name is its file stem, shown as it is), `description`
  (optional; one line), `timeout` (optional; AC-57), and one key for
  each place of the model's descriptor, under the name of that place
  (`slot-1`, `slot-2`, `printer`, `interface`, `host-drive`, `floppy`,
  `tape`; `presets.md` PR-2, PR-9, PR-11; `docs/formats/README.md` §2,
  `preset.md` §3). The product header keys of `preset.md` §3
  (`magnetic-cards`, `slot-N-rom`, `bus-rom`, `debug`) are added by
  their owners (AC-29). That a `preset-name` differs from those of the
  other presets of the same model is checked over the preset list
  (C59-2 AC-15, C1600-4 AC-18), not when one file is read.
- [ ] **AC-3** — Given a key for a place, then its value names a
  peripheral that fits that place (LIB-2 AC-2), optionally followed by
  `:` and one of its variants (`printer: CE-1600P:old`); a module slot
  takes a reference by `name:` or `file:` (`slot-1: { name: ML }`,
  `docs/formats/README.md` §3, `modules.md`). A place that no key names
  is empty (ADR 0016; behaviour change 59-4). Where the value is a
  reference to a module or a medium, or the owner of the place gives the
  value a form of its own, resolving it is that owner's (LIB-7; the host
  drive's folder: C1600-4).
- [ ] **AC-4** — Given the section `keys:`, then it has a step list, it
  may repeat, and its steps are carried out in file order
  (Calc-U-1600 `Core/Preset/PresetFile.cpp:758-767`).
- [ ] **AC-5** — Given a file whose name ends in any of the product's
  preset extensions, then the model comes from `model:`, not from the
  extension (both products today: Calc-U-59 `Shared/StateFile.cpp:545-552`,
  the example `historic/trace-quirk.ti59`; Calc-U-1600 `Core/DropFile.cpp:20-50`).

Reading: all or nothing
- [ ] **AC-6** — Given the command to load a preset, then the whole file
  is read and checked before anything is built; given any error, then
  nothing is built, the machine that runs is untouched, and the reply
  and a notice give the first error in file order with its line number
  and what was expected (ADR 0017; `presets.md` I-2; behaviour change
  59-3).
- [ ] **AC-7** — Given the check of AC-6, then it covers everything that
  can be told without a running machine: the rules of AC-1; unknown
  keys, fields and verbs; values a key does not accept; that the model
  has each place named and that the peripheral named fits it by the
  descriptors (LIB-2 AC-2); that every key of a `key` step is a
  name the model takes, and every character of a `type` step has a key
  (LIB-5 AC-15, AC-16); that each keyword is marked for the product and
  the model (AC-62); the places of `skip-wait:` and `let-it-finish:`
  (AC-58, AC-59); and what each feature and product checks
  for its own keys, sections and steps, including that the files they
  name can be read (OQ-2).
- [ ] **AC-8** — Given a file without `format-version`, or with a value
  below 2, then it is refused as outdated: the notice says that the file
  was written for an earlier release, that it must be upgraded by hand,
  and where the preset format is described (ADR 0037; behaviour changes
  59-1, 1600-1). Given a value above 2, then it is refused with the
  notice that it was written by a newer release.
- [ ] **AC-9** — Given `model:` names a model the product does not have,
  then the preset is refused and the notice lists the product's models
  (Calc-U-1600 `Core/Preset/PresetFile.cpp:755`).

Building the machine
- [ ] **AC-10** — Given a preset that was read without an error, then
  one machine is built from a cold boot with its model, its variant (the
  model's default one when none is given) and everything its place keys
  name, in one step (LIB-18 AC-1); the machine before it is gone (LIB-18
  AC-2), and the steps and sections start when the new machine reports
  key readiness.
- [ ] **AC-55** — Given the load command with the option to build only,
  then the preset is read and checked as always, the machine is built
  (AC-10), and the load is over at once: it does not wait for key
  readiness, and no step and no section is carried out (Calc-U-1600
  today, `Shared/AppSession.hpp:111-114`; C1600-3 AC-6 uses it).
- [ ] **AC-52** — Given the machine refuses what the preset names when
  it is built (a combination its pin-outs do not allow, LIB-2 AC-6,
  AC-7), then the load is refused as in AC-6, with the machine's reason
  and the line of the key: the machine that ran before is untouched.
- [ ] **AC-11** — Given the same preset is loaded twice, with the same
  files it names and the same host time handed to a real-time clock,
  then both machines have the same memory, the same display output and
  the same tick count at the end, and the log holds the same lines
  (`presets.md` I-1).
- [ ] **AC-12** — Given a load, then it is one operation from the
  command until its last item has been carried out, or, when that item
  is `let-it-finish:`, until the machine is idle (AC-59, AC-60;
  `presets.md` PR-4; `preset.md` §6.1; LIB-3 AC-14, AC-17): the busy
  topic says so, live input is dropped (LIB-5 AC-18), and another load
  is refused (LIB-3 AC-16). Reset, Cold boot and a model switch, and
  nothing else, end it at once, without an error, whether it hangs or
  not (LIB-18 AC-16; `preset.md` §6.1).
- [ ] **AC-13** — Given a load is run in parts by a frontend, and the
  same load in one go by the harness, then the result is the same: the
  log, the reply, the memory and the display output (LIB-3 AC-19;
  Calc-U-1600 `Core/tests/job_tests.cpp:230-234`).

Carrying it out
- [ ] **AC-14** — Given the sections of a preset, then they are carried
  out from top to bottom, each to its end before the next starts; the
  header keys describe the machine that is built before the first
  section, wherever they stand in the file.
- [ ] **AC-15** — Given a section that is not a step list (a product
  section that writes a program or data, or fits a cue card), then it
  starts when the item before it has finished (AC-56), and it has
  finished when its content is written or its cue card fitted; a
  `program:` with machine code that starts itself has finished when the
  code has returned (`preset.md` §6.2; C1600-2).
- [ ] **AC-16** — Given a load, then the machine runs at full speed
  while it is built and while its items are carried out, unless a
  `regular-speed:` step asks for regular speed (AC-22; `presets.md`
  PR-4; `preset.md` §6.1; behaviour change 59-5).
- [ ] **AC-17** — Given a step or a section fails while it is carried
  out, or has not finished when the `timeout:` of AC-57 has passed,
  then the preset stops there (`docs/formats/README.md` §4): the steps after it are not carried
  out, the machine stays as it is at that moment, and the reply and a
  notice give the line of the step and the reason (LIB-18 EC-6; LIB-3
  AC-18).
- [ ] **AC-18** — Given a load, then the log gets one line for the file
  and the machine built, one for each peripheral plugged in, one for
  each section and each step with its outcome, and a last line that says
  whether the preset was carried out to its end; a failure line holds
  the error. The wording of these lines is not part of this contract,
  except the line of `elapsed-time: report` (AC-23).
- [ ] **AC-19** — Given the machine is paused (by the debugger or the
  user) while a preset is carried out, then a step that needs the
  machine to run (`key`, `type`, `card`, a section that writes memory)
  waits until it runs again, and the debugger steps (LIB-10) and the
  frontend steps (AC-63) are carried out (`preset.md` §5.6). A paused
  machine is not idle, so the implicit wait after an item (AC-56) lasts
  until the machine has been resumed and is idle; the `timeout:` does
  not run meanwhile, since no emulated time passes (`preset.md` §6.3).
  Of this feature's steps, the speed steps, `elapsed-time`, `expect`,
  `screenshot` and `syncclock` need no running machine. A `wait:` with
  a duration passes in the host's time while the machine is paused and
  ends with the machine still paused, as `Wait:` does in 2.3.0; in the
  harness, which has no wall clock, it ends at once (user, 2026-11-11;
  `preset.md` §5.3, §6.1; Calc-U-59 `Shared/KeySequencer.cpp`, the case
  `K::Wait`). `let-it-finish:` waits until the machine runs again and
  is idle. A preset that ends paused leaves the machine paused. Every
  other step says which it is in the spec that owns it (Calc-U-59
  `tests/shared_cooperative_tests.cpp`,
  `cooperative_script_waits_out_a_freeze_but_runs_its_directives`).
- [ ] **AC-56** — Given two items in a row, then the second starts only
  when the first has **finished** (the implicit wait, `preset.md` §6.1,
  §6.2): a `key:` when its last press is done and the machine is idle; a
  `type:` when ENTER is done and the machine is idle; a section as AC-15
  says; `card:` when the card has passed and is fitted, `arm:` when the
  card is fitted (C59-7); `wait:` when its duration has passed; every
  other step when it has taken effect. The wait runs at the speed in
  force. So a key that starts a program finishes when the program
  stops.
- [ ] **AC-57** — Given the header key `timeout:` (seconds of emulated
  time; default 3600), then it bounds every implicit wait of the
  preset at both speeds: an item that has not finished by then fails
  (AC-17). It does not bound the last item, which is not waited for,
  nor `let-it-finish:` (`preset.md` §6.3).
- [ ] **AC-58** — Given `skip-wait:` after an item, then the implicit
  wait after that item is dropped: the next item starts as soon as that
  one has been carried out (its keys pressed, ENTER typed, the card
  passed). As the first or the last item it is refused when the preset
  is read (`preset.md` §5.3, §6.1).
- [ ] **AC-59** — Given `let-it-finish:` as the last item, then the
  machine runs on at the speed in force until it is idle, without a
  limit, and the preset ends then; anywhere else it is refused when the
  preset is read (`preset.md` §5.3, §6.1).
- [ ] **AC-60** — Given the last item of a preset, when it is not
  `let-it-finish:`, then it is carried out and not waited for: the
  preset ends at once, the machine runs at regular speed, and what the
  item started runs on under the user's control (`preset.md` §2, §6.1).
  A preset that ends paused stays paused (AC-19).
- [ ] **AC-61** — Given a preset runs, then the speed is the preset's: a
  frontend's gesture for full speed (LIB-17) does not change it, and the
  user's key presses on the machine are dropped (`preset.md` §6.1;
  AC-12).

The steps of this feature
- [ ] **AC-20** — Given the step `wait:` with a duration, then, after
  the implicit wait of the item before (or at once after a
  `skip-wait:`), that much emulated time passes (LIB-11 AC-9), at full
  speed as fast as the host allows, on an idle TI machine too (LIB-17
  AC-16); the duration needs a unit and may
  have a fraction (`2s`, `1.5s`, `500ms`); a value without a unit is
  refused when the preset is read (`presets.md` PR-5; `preset.md` §5.3,
  §6.1; behaviour change 1600-3).
- [ ] **AC-21** — Given the step `wait:` without a value, then the
  preset is refused when it is read: the duration is required
  (`preset.md` §5.3). What version 1's bare `- wait:` did is now the
  implicit wait (AC-56), or `let-it-finish:` as the last item (AC-59)
  (`preset.md` §9.2).
- [ ] **AC-22** — Given the step `regular-speed:`, then the items after
  it run paced at regular speed, the display and the paper show what
  happens, and the busy topic says that the operation runs at regular
  speed (LIB-3 AC-14); given `full-speed:`, then full speed is back.
  Several in a row: the last counts. Both take no value and stand in a
  `keys:` section. At the end of the preset the machine runs at regular
  speed, whatever ran before the load (AC-60; LIB-18 AC-2; Calc-U-59
  `Shared/KeySequencer.cpp:209-210`, `Shared/EmulatorSession.cpp:340`;
  `preset.md` §5.3, §6.1). In the harness, which has no wall clock
  (LIB-11 AC-6), the steps change nothing that can be read from the
  machine.
- [ ] **AC-23** — Given the step `elapsed-time: start`, then the
  machine's emulated time at that moment is noted; given
  `elapsed-time: report`, then the log gets the line
  `Elapsed: m:ss.mmm` with the emulated time since the last `start`, or
  since the start of the preset when there was none; the minutes count
  on beyond 59 (`preset.md` §5.4; Calc-U-59 `Shared/EmulatorSession.cpp:256-265`,
  `Shared/EmulatorSession.hpp:201-206`).
- [ ] **AC-24** — Given the step `expect:` with a text (a raw value, not
  empty), then it is checked after the item before has finished (after
  `- type: RUN`, when the program has ended); it passes when the
  product's comparison finds the text (Calc-U-59: AC-40; Calc-U-1600:
  AC-48); otherwise the preset fails (AC-17) and the reason holds the
  text and what the display shows (`preset.md` §5.4; Calc-U-1600
  `Core/Display/LcdText.cpp:137-164`,
  `Core/Preset/PresetRunner.cpp:137-146`).
- [ ] **AC-25** — Given the step `screenshot:` with a file name, then a
  PNG picture of the display as it is at that moment (AC-53) is written
  under that name to the folder for screenshots, which is the save
  folder in a Qt6 or Swift frontend (LIB-9 AC-12, AC-17); in a Web
  frontend the picture is handed to the user to keep, as a download
  (LIB-15 AC-9; OQ-4); a name that is empty or
  holds `/` or `\` is refused when the preset is read; given the file
  cannot be written, then the preset fails (Calc-U-1600
  `Core/Preset/PresetFile.cpp:262-276`, `Core/Preset/PresetRunner.cpp:124-136`).
  `preset.md` §5.4 says the same since 2026-11-13 (OQ-6).
- [ ] **AC-26** — Given the step `switch:`, then the preset is refused
  when it is read: the format has no such step. A peripheral's switch is
  set by a step of its own, which its owner adds (AC-29): the PC-100C's
  TRACE switch by `printer-trace: on|off` (AC-40; `preset.md` §5.5).
- [ ] **AC-27** — Given the step `syncclock:`, which takes no value, on
  a Sharp model, then every real-time clock of the machine is handed the
  host's date and time as at a build (LIB-2 AC-23), and the log gets the
  time set; on a TI model it is refused when the preset is read, since
  it is marked [1600] (AC-62; `preset.md` §5.4; Calc-U-1600
  `Core/Preset/PresetFile.cpp:287-294`, `Core/Preset/PresetRunner.hpp:177`).
- [ ] **AC-28** — Given the steps `key:` and `type:`, then they are
  LIB-5's (LIB-5 AC-15 to AC-17); `type:` and `expect:` take a raw
  value. When they have finished in a preset is AC-56: the last press
  done, or ENTER done, and the machine idle.

The place for other features and for products
- [ ] **AC-29** — Given another library feature or a product's app
  layer, then it can add header keys, sections with their fields and
  block scalars, and steps to the format; the loader reads them by the
  rules of AC-1, checks them with the rest before anything is built
  (AC-6), carries them out in file order (AC-14), and gives their errors
  with the line number (ADR 0018; `presets.md`, responsibilities).
- [ ] **AC-30** — Given a key, a section or a step that neither this
  feature nor any feature or product in the running program has added,
  then the preset is refused (AC-6); a preset with a product section of
  another product is thus refused.
- [ ] **AC-64** — Given a preset that drives a scenario at full speed
  and reports `elapsed-time: report` at its end, on any model and in
  any frontend or the harness, then the emulated time it reports is
  about what a real user who drives the real calculator quickly would
  take: the time the machine computes, plus each key held and spaced as
  a quick user does (the minimum hold and the gaps of the typing
  profile, LIB-5 AC-20, AC-28), plus the `wait:` durations the preset
  asks for; no time is added because the machine waits for the next key
  longer than that, and the host's speed does not change it (LIB-6
  AC-11; behaviour change 59-23). The test suite checks it on the
  examples with an `Elapsed:` line against the times their authors give
  (`selftest-speed-check`, the calendar examples, `picard`,
  `globe-print`, `printer-timing`).
- [ ] **AC-62** — Given a keyword marked for one product
  (`docs/formats/README.md` §5, `preset.md` §7), then it is refused in a
  preset for the other product's models, and a keyword used with a model
  it does not apply to is refused too (`slot-2:` on a PC-1500); given a
  keyword marked [undefined] for the model, then the preset is refused
  as using what the format has not defined yet. All of it when the
  preset is read (AC-6).
- [ ] **AC-63** — Given a frontend step (`preset.md` §5.7: on the TI
  models `debug-panel:`), then it never fails and has
  finished when it has taken effect; a frontend that does not have the
  panel concerned passes it over, and so does the harness. Its owner is
  LIB-10; `cut-paper:` is a session step (LIB-8 AC-22).
- [ ] **AC-53** — Given a product's app layer, then it tells the loader
  for each of its models, and the loader itself names no model: when a
  machine of that model is idle, from what the machine reports; what, if
  anything, is done after the build and before the first section; and
  the picture of the display that `screenshot` writes. The text that
  `expect` reads is the one LIB-11 AC-10 gives. Each of the three is
  shown to work with a machine built for the tests.

Preset list and startup presets
- [ ] **AC-31** — Given a product, then it bundles one preset list: the
  startup presets it offers for each model, and its examples in named
  groups, in the order in which they are to be listed. The list is part
  of the product, is never read from a folder of the user, and is not
  an own format with a version (Calc-U-1600 `examples/bundled.yaml`,
  `Shared/PresetCatalog.cpp`).
- [ ] **AC-32** — Given the request for the preset list, then the reply
  lists each entry with its reference, its model, its name and its
  description, whether it is a startup preset or an example, and for an
  example its group (AC-31; LIB-14 AC-1, AC-12); the name is the
  preset's `preset-name`, or the file name
  without its extension when there is none. Only the header keys of each
  file are read for this, so an entry whose preset names a missing file
  is still listed (Calc-U-1600 `Core/tests/preset_tests.cpp:52`).
- [ ] **AC-33** — Given a place where a preset is named (the load
  command, `[preset]` in the settings, LIB-9 AC-10), then it takes a
  path or a reference; a reference is found in the product's bundled
  presets on every platform (Calc-U-1600 `Shared/AppPaths.cpp:55-59`).

Harness
- [ ] **AC-34** — Given the harness, then these commands exist at the
  top level, with the meaning of the steps:

  | Command | What it does | Reply |
  |---|---|---|
  | `preset <path or reference>` | loads the preset (AC-6 to AC-18) | done, or refused or failed with the line and the reason |
  | `check-preset <file>…` | reads and checks each file as AC-6 and AC-7 do, and builds nothing | one line per file, `ok` or `FAIL` with the error; the run ends as failed when a file failed |
  | `preset-list` | the request of AC-32 | the entries |
  | `wait` | harness only: waits until the machine is idle, at most one hour of emulated time; the preset format has no such step since 2026-11-12 (AC-21; OQ-7) | the emulated time that passed, and whether the hour was reached |
  | `full-speed`, `regular-speed` | the command of LIB-3 AC-21; not paced in the harness (AC-22) | done |
  | `elapsed-time start`, `elapsed-time report` | AC-23 | done; for `report` the time |
  | `expect <text>` | AC-24 | done, or refused with what the display shows |
  | `screenshot <path>` | AC-25, written to that path | done, or refused |
  | `switch <peripheral> <control> on\|off` | harness only: sets a switch through the controls capability (LIB-2 AC-16); the preset step is the peripheral's own (AC-26) | done, or refused for an unknown control |
  | `syncclock` | AC-27 | done |

  (Calc-U-1600 `tools/pc1600_cli.cpp:97-99, 243-258`; LIB-11 AC-14,
  AC-18.)

### Calc-U-59

- [ ] **AC-35** — Given Calc-U-59, then its presets end in `.ti59`,
  `.ti58` or `.ti58c`; `model:` takes `TI-59`, `TI-58` or `TI-58C`, and
  no variant; the place keys are `slot-1:` with a reference by `name:`
  to the ID of one of the 14 Solid State ROM modules
  (`slot-1: { name: ML }`; C59-6) and `printer: PC-100C` (`presets.md`
  PR-2, PR-9, PR-11; `preset.md` §3); `file:`, which the format allows,
  is refused in this release (OQ-8; C59-6 AC-8).
- [ ] **AC-36** — Given a TI machine, then it is idle when it has
  reported key readiness (C59-1 AC-18) without a break for 100 ms of
  emulated time, the minimum hold of LIB-5 AC-20, which is to tell a
  stopped machine from a running program whose ROM samples a key. The
  first task checks this against 2.3.0 with a running program on each
  model, with and without the PC-100C; if no stretch tells the two
  apart, idle on a TI machine becomes a state of its own in the machine
  interface, and the spec is changed then (OQ-1). A TI-59 that waits for
  a magnetic card after `Write` (C59-7 AC-9) is idle too (`preset.md`
  §6.2), so that `- key: 3 2nd Write` finishes and the next step,
  `card:`, feeds the card.
- [ ] **AC-37** — Given a preset without `slot-1:` or without
  `printer:`, then the machine has no module, or no printer. Today a
  preset keeps the module and the printer it does not name
  (`Shared/SessionController.cpp:656-657`; behaviour change 59-4).
- [ ] **AC-38** — Given a preset written for 2.3.0, which has no
  `format-version`, then it is refused as outdated (AC-8). Given a
  preset with any error, then it is refused whole; today the errors are
  shown and the rest of the file is loaded
  (`Shared/SessionController.cpp:609-615`; behaviour changes 59-1, 59-3).
- [ ] **AC-39** — Given a preset with steps, then they run at full speed
  unless `regular-speed:` comes first, and the keys the user presses
  meanwhile are dropped. Today the steps run at regular speed, the user
  can press keys, and a second preset waits behind the first
  (`Shared/KeySequencer.cpp:247-252`; behaviour change 59-5).
- [ ] **AC-40** — Given a TI model, then `type:` is refused when the
  preset is read, because no TI typing profile maps characters (LIB-5
  AC-16); `printer-trace: on` and `off` set the TRACE switch of the
  PC-100C and are refused without `printer: PC-100C` (`preset.md` §5.5;
  today `PrinterTrace:`, `Shared/SessionController.cpp:674-676`);
  `elapsed-time: report` counts emulated time as LIB-11 AC-17 does;
  `screenshot:` writes the picture Calc-U-59 supplies of its segment
  display, in the classic segment style (C59-2; OQ-3); `expect:` takes a
  number and passes when it equals the value in the display register X,
  whether or not the display blinks (`preset.md` §5.4); how the number
  is compared with X (digits, rounding, the display format) is open
  (OQ-9).
- [ ] **AC-41** — Given a preset is loaded by the user, then the paper
  of the PC-100C it names is empty at the start, as today; a startup
  preset at a model switch leaves the paper as it is
  (`Shared/SessionController.cpp:655`). The same rule holds for every
  printer of both products (LIB-8 AC-8).
- [ ] **AC-42** — Given the Swift, Qt6 and Web frontends, then each
  hands a preset to the session as a file in a folder or, where it has
  only the content (Swift on iOS, Web; EC-5), as its content, and
  everything else is this loader's. The GUI test of each frontend has
  one scenario for it under this ID: a bundled example is loaded and the
  display shows its result (today
  `Calc-U-59UITests/PresetLoadTests.swift:35`,
  `Web/tests/play-calculator.spec.js:210-232`; ADR 0032).
- [ ] **AC-43** — Given the examples and startup presets Calc-U-59
  ships (C59-2), then the test suite checks each with `check-preset` and
  loads each to its end without a failure (today
  `tests/shared_state_applier_tests.cpp:97-122`). Given the presets that
  calc-u.org hosts for Calc-U-59 (`59/software-files/`,
  `59/calc/presets/`), then `check-preset` over them reports no failure
  before a release (ADR 0017, 0037); the release checklist of the
  product names this step.

### Calc-U-1600

- [ ] **AC-44** — Given Calc-U-1600, then its presets end in `.pc1500`,
  `.pc1500a` or `.pc1600`; `model:` takes `PC-1500` with the variants
  `A01`, `A03`, `A04`, `PC-1500A` without a variant, and `PC-1600` with
  the variants `new` and `old`; the place keys are `slot-1:`,
  `printer: CE-150` and `interface: CE-158` on every model, and on the
  PC-1600 also `slot-2:`, `printer: CE-1600P` with the variants `new`
  and `old`, `host-drive:` and `floppy:`
  (`Core/Preset/PresetFile.cpp:745-946`; `presets.md` PR-11), and
  `tape:` with a printer that has a cassette interface (`preset.md` §3;
  which printers on the PC-1600 is open, OQ-10). The key `plotter:` is
  gone (behaviour change 1600-3). The keys `ctrl`, `kbii` and `bs` are
  refused in a `key:` step (`preset.md` §5.1).
- [ ] **AC-45** — Given a Sharp machine, then it is idle when its typing
  support reports prompt readiness (LIB-5 AC-35), as today
  (`Core/Preset/PresetRunner.cpp:75-91`; OQ-1).
- [ ] **AC-46** — Given a PC-1500 or PC-1500A is built by a preset and
  its ROM asks `NEW0? :CHECK`, then the load answers with the key `cl`,
  at most five times, before the first section starts; on the PC-1600
  the load presses nothing by itself, as today
  (`Core/PC1500/PC1500PresetLoader.cpp:156-168`). This is what
  Calc-U-1600 does after the build (AC-53).
- [ ] **AC-47** — Given a preset that names hardware the pin-outs do not
  allow together (a CE-1600P on a PC-1500, a CE-158 with a CE-1600P, a
  `floppy:` without a CE-1600P, a `slot-2:` on a PC-1500), then it is
  refused when it is read, with a notice that names both, as today
  (`Core/Preset/PresetFile.cpp:861-930`).
- [ ] **AC-48** — Given `screenshot:`, then the picture is the one the
  menu item Copy Screen gives (LIB-14 AC-41): the display at its
  physical size, on the PC-1600 the
  graphics area without the indicator row
  (`Core/PC1500/PC1500PresetLoader.cpp:63-65`,
  `Core/PC1600/PC1600PresetLoader.cpp:164-167`). Given `expect:`, then
  it passes when one line of the display, as the text of LIB-11 AC-19,
  contains the text, with its case and spaces as written; in the text a
  backslash is written `\\` and a character outside ASCII `\xHH`
  (`preset.md` §5.4).
- [ ] **AC-49** — Given a preset is loaded in the Qt6 or the Web
  frontend, then its log lines are on the log topic. Today the Qt6
  frontend writes them to the platform's log and the Web frontend keeps
  them without showing them (`Qt6/app/MachineController.cpp:71-73`,
  `Web/src/engine.js:152`; where the log is shown: LIB-14).
- [ ] **AC-50** — Given what the tests of Calc-U-1600 check today about
  the rules of AC-1 and the steps of this feature, then it holds: the
  comment and raw-value rules, a `#` without a space before it, quoted
  values, keys given twice, `wait` with a value (the form without one
  is now refused, AC-21),
  `syncclock`, `expect`, `screenshot`, names in any case with the hyphen
  required, and a run in parts against a run in one go
  (`Core/tests/preset_tests.cpp:36-159, 277-331, 423-437`,
  `Core/tests/lcd_text_tests.cpp:348-397`,
  `Core/tests/lcd_screenshot_tests.cpp:144-160`,
  `Core/tests/job_tests.cpp:230-234`).
- [ ] **AC-51** — Given the presets in the source repository that belong
  to Calc-U-1600 (examples, startup presets, test fixtures, the presets
  under `dev/`, the presets of the developer documentation and of the
  VS Code extension), then the test suite checks each with
  `check-preset`; today `tools/check_presets.sh` checks the same files
  and is run by hand; files with placeholders that are filled in later
  are left out.
- [ ] **AC-54** — Given the Qt6 and Web frontends, then each hands a
  preset to the session as a file in a folder or, where it has only the
  content (a file dropped on the Web frontend; EC-5), as its content,
  and everything else is this loader's. The GUI test of each frontend
  has one scenario for it under this ID: a bundled example is loaded and
  its result shows (today `Web/tests/play.spec.js:208`; ADR 0032).

## Edge Cases
- **EC-1** — Given a file that cannot be read or is not UTF-8, then the
  preset is refused and the notice names the file (Calc-U-59
  `Shared/SessionController.cpp:586-597`).
- **EC-2** — Given a preset with header keys only, then the machine is
  built and the load ends when it reports key readiness.
- **EC-3** — Given Reset, Cold boot or a model switch while a preset is
  carried out (AC-12), then the preset ends there (LIB-18 AC-16): its remaining
  steps are not carried out, the reply to its load says that it was
  stopped, and the machine is as that command leaves it.
- **EC-4** — Given the last step of a preset starts a program that keeps
  running, then the load is over after that step, and the program runs
  on at regular speed (AC-60). With `let-it-finish:` after it, the load
  waits without a limit; Reset, Cold boot or a model switch end it
  (AC-12).
- **EC-5** — Given a preset that was handed over as its content, without
  a folder, and that names a file by a path that is not absolute, then
  it is refused when it is read, with a notice that names the file
  (today the Web frontend of Calc-U-1600 fails on the file when the step
  is reached, `Web/src/CalcU1600.jsx:173-179`; OQ-2).
- **EC-6** — Given the session is suspended while a preset is carried
  out, then the preset makes no progress until it is continued (LIB-3
  EC-4).
- **EC-7** — Given `expect:` while the display is off, then it fails,
  and the reason says that the display is off.
- **EC-8** — Given a step list with no item, then the preset is refused
  when it is read (Calc-U-1600 `Core/Preset/PresetFile.cpp:172`).
- **EC-9** — Given an entry of the preset list whose file is missing or
  whose header keys cannot be read, then the entry is left out of the
  reply and a line of the log says so; the test suite fails on it
  (AC-43, AC-51).
- **EC-10** — Given a machine that does not report key readiness after
  it was built, within the time of LIB-18 EC-7, then the load fails
  there as in AC-17, and no step is carried out.

## Technical Requirements (optional)
- One loader in the library reads every preset of every product; a
  product's app layer adds only its sections, keys and steps (Briefing
  §1.3; `presets.md`, responsibilities).
- The loader names no model: it finds the place keys through the
  descriptor (`presets.md` PR-11).
- A key or a step that exists in two products has one name and one
  meaning (`presets.md` I-3).
- The format of `docs/formats/` is the reference chapter of the help on
  presets (LIB-16; `presets.md` PR-1).
- The readers of the earlier formats and their tests are not carried
  over (Calc-U-59's sectioned format and the `STATE` records of
  `tests/golden/formats.txt`; Calc-U-1600's version 1).

## Open Questions
- [x] **OQ-1** — *Decided as recommended (user, 2026-11-10, batch 5 gate).* What idle means for a `wait:` without a value
  (`frontend.md` Q-3). Sharp: prompt readiness, as today (AC-45). TI:
  the step is new, and key readiness alone will not do, because
  readiness can be a short window and the ROM samples R/S and GTO while
  a program runs (`typing.md` T-2 as amended). AC-36 therefore asks for
  key readiness without a break for a stretch, and the stretch is not
  known. *Recommendation:* 100 ms of emulated time, the minimum hold of
  the TI typing profile (LIB-5 AC-20), checked in the first task against
  2.3.0 with a running program on each model, with and without the
  PC-100C; if no stretch tells the two apart, idle on a TI needs a state
  of its own in the machine interface. The Calc-U-59 column waits for
  this.
- [x] **OQ-2** — *Decided as recommended (user, 2026-11-10, batch 5 gate).* How far the check before the build reaches (AC-7).
  Today Calc-U-1600 finds a module or a disk that is missing, a host
  drive folder that is none, a ROM file of the wrong size, a key that is
  not a PC-1600 key and a character without a key only after the old
  machine is gone (`Shared/AppSession.cpp:197-262`,
  `Core/PC1600/PC1600PresetLoader.cpp:61-153`). *Recommendation:*
  everything that needs no running machine is checked first, as AC-7
  says, a build the machine refuses leaves the running machine alone
  (AC-52), and a preset handed over without its folder is refused when
  it names a file (EC-5), so that `presets.md` I-2 holds as far as it
  can. These are small visible changes for Calc-U-1600 and get one row
  in `docs/behaviour-changes.md`.
- [x] **OQ-3** — *Decided as recommended (user, 2026-11-10, batch 5 gate).* The picture of `screenshot:` on a TI machine.
  `presets.md` PR-5 gives the step to every product; Calc-U-59 draws its
  display only in its frontends today. *Recommendation:* the product
  supplies the picture of its segment display below the frontend, in the
  classic segment style, and a reference picture in the tests fixes its
  size and look.
- [x] **OQ-4** — *Decided (user, 2026-11-10, batch 5 gate): the save folder in Qt6, as LIB-9 AC-17 says, and a download on the web. Swift (user, 2026-11-10, batch 6 gate): the iCloud Drive folder, which is its save folder.* The folder for screenshots outside Qt6 (AC-25) depends
  on LIB-9 OQ-7. Today Calc-U-1600 writes a preset's screenshot to the
  trace folder (`Core/Preset/PresetRunner.cpp:124-136`), while LIB-9
  AC-17 gives screenshots the save folder. *Recommendation:* settle it there; on the web
  the picture is handed to the user to keep (LIB-15 AC-9).
- [x] **OQ-5** — *Decided as recommended (user, 2026-11-10, batch 5 gate).* "Header key", "section", "step" (of a preset), "idle",
  "reference" and "preset list" for the glossary, as defined above.
  "Step" stands next to "program step".

- [x] **OQ-6** — *Decided (user, 2026-11-13, F-11): the folder for screenshots, the save folder, as AC-25 and OQ-4 say; `preset.md` §5.4 corrected.* Where `screenshot:` writes its picture. `preset.md`
  §5.4 (user, 2026-11-12) says the trace folder, whose place is the
  product's; LIB-9 AC-17 and OQ-4 above (user, batch 5 and 6 gates)
  give screenshots the save folder and traces a folder of their own.
- [ ] **OQ-7** — The harness and the waiting rules. The format has no
  `wait:` without a value; the harness keeps its `wait` until idle
  (AC-34), as the counterpart of the implicit wait, which `harness.md`
  H-1 ("one vocabulary") does not yet cover. Open with the harness
  questions of LIB-11 (implicit wait, `skip-wait`, `let-it-finish`,
  frontend steps as commands).
- [x] **OQ-8** — *Decided (user, 2026-11-13, F-9): Calc-U-59 takes `name:` only in this release and refuses `file:` with a notice (C59-6 AC-8).* `slot-1: { file: … }` on the TI models: `preset.md` §3
  allows a Solid State ROM module file; the decision of 2026-11-11
  (C59-6, "by its two-letter ID only") and LIB-12 ("no own dumps")
  exclude it. The format README leaves to each product which keywords
  its loader carries out.
- [ ] **OQ-9** — `expect:` on a TI model compares a number with the
  display register X (`preset.md` §5.4): with how many digits, and
  whether `3.141592654` matches the 13-digit value in X.
- [ ] **OQ-10** — `tape:` on the PC-1600: `preset.md` §3 allows it with
  `printer: CE-150` or `CE-1600P`; LIB-1 AC-39 and C1600-1 AC-16 say a
  CE-150 on a PC-1600 has no cassette.

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| One family preset format, a YAML-shaped subset with a written grammar, read by the library's own loader | ADR 0018; `presets.md` PR-1 | 2026-11-10 |
| `format-version: 2`; the extensions stay per model; `model:` is required | `presets.md` PR-2 | 2026-11-10 |
| A preset is read completely before anything is built; any error refuses it whole | ADR 0017; behaviour change 59-3 | 2026-11-10 |
| A preset builds its machine from a cold boot; a place it does not name is empty | ADR 0016; `presets.md` PR-9; behaviour change 59-4 | 2026-11-10 |
| An older or unversioned preset is refused with a hint; there is no converter | ADR 0037; behaviour changes 59-1, 1600-1 | 2026-11-10 |
| Full speed by default, a `speed` step for paced stretches; the whole preset is one operation and user keys are dropped | `presets.md` PR-4; `typing.md` T-4; behaviour changes 59-5, 1600-9 | 2026-11-10 |
| `wait:` needs a unit and counts emulated time; without a value it waits until the machine is idle; `expect:` and `screenshot:` exist in every product; names are kebab-case | `presets.md` PR-5; behaviour change 1600-3 | 2026-11-10 |
| `#` line or ` #` after a value is a comment, except in raw values | `presets.md` PR-6 | 2026-11-10 |
| A peripheral's switch is set with `- switch: <peripheral> <control> on` | `presets.md` PR-7 | 2026-11-10 |
| The hardware keys are the places and media places of the descriptor; `printer:` in every product | `presets.md` PR-11 | 2026-11-10 |
| Reset, Cold boot and a model switch end a running preset; another load is refused | `frontend.md` Q-1 as amended; LIB-18 AC-16 | 2026-11-10 |
| A preset that fails while it is carried out leaves the machine as it is at that moment | LIB-18 EC-6 | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 5 gate): the step-list section keeps Calc-U-1600's name `keys:` | It reads well for both products; `steps:` is taken by the TI program | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 5 gate): every header key may appear once (today a repeated `model:` or `plotter:` is accepted and the last one counts) | One rule for all keys | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 5 gate): the reply and the notice give the first error in file order (Calc-U-59 collects all errors today, Calc-U-1600 stops at the first) | It makes AC-6 checkable | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 5 gate): the harness commands of AC-34 and their names; `check-preset` ends the run as failed when a file failed | They make the criteria checkable | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 5 gate): after a preset the machine runs at regular speed, whatever ran before (AC-22) | LIB-18 AC-2: full speed does not carry over to a new machine; Calc-U-59 today | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 5 gate): a machine that never reports key readiness after its build fails the load (EC-10); today both products go on after a fixed time | Its steps would wait without end (LIB-5 EC-3) | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 5 gate): `expect:` compares case and spaces exactly; the minutes of `Elapsed:` count on beyond 59 | They make the criteria checkable | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 5 gate): inside a block scalar, empty lines and `#` lines are skipped as everywhere (Calc-U-1600 today); a ` #` later in a line is its owner's | One rule for skipped lines; `presets.md` PR-6 | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 5 gate): a preset handed over without a folder that names a relative file is refused when it is read (EC-5) | It follows from all-or-nothing | 2026-11-10 |
| Added with C1600-3's cold read and accepted (user, 2026-11-10, batch 7 gate): a load can build only (AC-55) | The debugger's `boot: debug` needs the machine before its first instruction; Calc-U-1600 has it today | 2026-11-10 |
| While the debugger has the machine paused, a `wait:` with a duration passes in the host's time and ends with the machine still paused; in the harness it ends at once. Keys and a `wait:` without a value still wait for the machine to run again (AC-19) | user, 2026-11-11; 2.3.0's `Wait:` is wall-clock time, and the screenshot test preset `test_cdebug.ti59` waits after its freeze | 2026-11-11 |
| The waits of AC-15 and AC-21 stay at one hour, after which the preset goes on; only a card that the ROM does not take ends a preset, after 30 minutes without output (C59-7) | user, 2026-11-11 | 2026-11-11 |
| The reply to the preset list says for each entry whether it is a startup preset or an example, and an example's group (AC-32) | Phase 5, preset pass (`docs/walkthroughs/presets.md`, A-6); AC-31 and LIB-14 AC-1, AC-12 already decide groups and the two kinds of entry | 2026-11-11 |
| AC-51 names the presets under `dev/` and of the VS Code extension, which today's check covers, and no longer says that the test suite runs that check today | Phase 5, preset pass (`docs/walkthroughs/presets.md`, A-10); `tools/check_presets.sh:2-11` | 2026-11-11 |
| The format of a preset is `docs/formats/` (README.md, preset.md); AC-1 points to it instead of carrying a grammar of its own; no block is taken raw | user, 2026-11-12 (`docs/formats/`); supersedes the grammar of `presets.md` PR-1 and the raw blocks of `debug:` and `bus-rom:` | 2026-11-13 |
| Header keys `timeout:` and `tape:`; `preset-name` at most 18 characters and, without it, the file stem; module slots take a reference `{ name: … }` or `{ file: … }` (AC-2, AC-3, AC-57) | user, 2026-11-12 (`docs/formats/README.md` §2, §3; `preset.md` §3) | 2026-11-13 |
| The implicit wait between items, bounded by `timeout:` after which the item fails; `skip-wait:`; `let-it-finish:` as the last item; the last item is not waited for; a preset ends at regular speed; while it runs the user's gesture does not change the speed (AC-12, AC-15, AC-56 to AC-61). Replaces the hour after which the preset went on (row of 2026-11-11 above) | user, 2026-11-12 (`preset.md` §6; `docs/research/preset-conversion-check.md` S17, S18) | 2026-11-13 |
| `wait:` needs a duration, which may have a fraction (`1.5s`); the form without a value is refused (AC-20, AC-21). Replaces PR-5's bare `- wait:` | user, 2026-11-12 (`preset.md` §5.3, §9.2) | 2026-11-13 |
| The steps are `regular-speed:`, `full-speed:`, `elapsed-time: start\|report`; `switch:` is gone and the PC-100C's TRACE switch is `printer-trace:` (AC-22, AC-23, AC-26, AC-40). Replaces the names of PR-5 and PR-7 | user, 2026-11-12 (`preset.md` §5.3 to §5.5) | 2026-11-13 |
| Reset, Cold boot and a model switch are the only actions that end a running preset, hung or not; another load stays refused (AC-12, EC-3, EC-4). `preset.md` §6.1 had "anything that resets, directly or indirectly, loading a preset included"; corrected there | user, 2026-11-13 (question F-3 of `docs/walkthroughs/presets.md` §5.4) | 2026-11-13 |
| A paused machine is not idle; steps that need it to run wait, debugger and frontend steps go on; `let-it-finish:` waits for it to run again (AC-19) | user, 2026-11-12 (`preset.md` §5.6, §6.2) | 2026-11-13 |
| Keywords marked for one product, or [undefined], are refused (AC-62); `syncclock:` is refused on the TI models (AC-27); frontend steps never fail and the harness passes them over (AC-63) | user, 2026-11-12 (`docs/formats/README.md` §5; `preset.md` §5.4, §5.7) | 2026-11-13 |
| `expect:` on a TI model compares a number with the display register X; on the Sharp models one line contains the text, with `\\` and `\xHH` (AC-24, AC-40, AC-48) | user, 2026-11-12 (`preset.md` §5.4) | 2026-11-13 |
| The elapsed time a preset reports at full speed is about that of a quick real user: computing time, quick key holds and gaps, the asked `wait:` durations, nothing for idle waiting; on every model (AC-64). A preset's `wait:` at full speed is not paced on an idle TI (AC-20, LIB-17 AC-16) | user, 2026-11-13 (question F-12 of `docs/walkthroughs/presets.md` §5.4) | 2026-11-13 |
