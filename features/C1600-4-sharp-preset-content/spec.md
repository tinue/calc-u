# C1600-4: Sharp preset content

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: C1600-1 (the Sharp machines, their places and product
  kinds), C1600-2 (the loaders that put a program into a machine), LIB-6 (the preset loader: the grammar, the place for a
  product's keys, sections and steps, the product's part of a load, the
  preset list); through LIB-6 also LIB-2 (attach) and LIB-5 (typing);
  LIB-8 and LIB-10 for comparing what an example leaves (AC-16). The
  examples that name media or record a trace are upgraded and compared
  with LIB-7 and LIB-10 (AC-15).
- Contracts and decisions: `docs/contracts/presets.md` (PR-3, PR-5,
  PR-11 as amended, I-1 to I-3); ADR 0018, 0037;
  `docs/behaviour-changes.md` rows 1600-1, 1600-3, 1600-11.
- Facts: Calc-U-1600 at `b4ea8c8`; every `path:line` below is at that
  commit.

Words of other specs used here: "header key", "section", "step", "idle"
and "preset list" (LIB-6), a block "taken raw" (LIB-6 AC-1), "notice"
(LIB-3 AC-10). Words of this spec: a **bus ROM** is a ROM image that a
preset puts on the system bus of the 60-pin connector, outside any
module; a **bank** is one of the parts of the PC-1600's memory that are
switched in and out; a **BASIC listing** is a BASIC program as text, and
**tokenized BASIC** the same as the machine stores it; **program mode**
is the mode of the machine in which typed lines are stored as a program
(PRO); a **reference run** is a record of what a version 1 preset leaves
behind, made with `b4ea8c8` (AC-16).

## User Stories
- As a user of a Sharp machine, I want a preset to put a BASIC program
  or a machine-code program into the machine it builds, from a file or
  from text in the preset.
- As a user of the PC-1600, I want a preset to mount a folder as the
  host drive.
- As a developer of a ROM or of a program in a module, I want a preset
  to put my ROM image on the bus or into a module, and to carry the
  settings my debugger needs.
- As a user, I want every example and startup preset that Calc-U-1600
  ships to work after the update as it did before.

## Out of Scope
- The grammar, the header keys of the family, the steps, all or nothing,
  the build of the machine and the preset list as such (owner: LIB-6).
- How a program gets into the machine: telling a BASIC listing from
  tokenized BASIC and from machine code, loading, typing in, placing
  machine code and starting it (owner: C1600-2).
- Naming a module or a disk, by name or by file, `medium:` sections and
  the step `saveas` (owner: LIB-7); the step `trace` (LIB-10).
- What the debugger does with the settings of a `debug:` block (owner:
  C1600-3).
- The places, pin-outs and variants of the Sharp machines (owner:
  C1600-1); which hardware fits together (LIB-2; LIB-6 AC-47, AC-52).
- The presets of Calc-U-59 (owner: C59-2).

## Acceptance Criteria

The `program:` section
- [ ] **AC-1** — Given a preset for a Sharp model, then it can have the
  section `program:`, any number of times; each is carried out in its
  place in file order (LIB-6 AC-14, AC-15). Its block is a list of
  these fields, read and checked when the preset is read (LIB-6 AC-6,
  AC-7) (`Core/Preset/PresetFile.cpp:393-537`):

  | Field | Value | Rules |
  |---|---|---|
  | `file:` | a path (LIB-6 AC-1) | exactly one of `file:` and `text:` is required |
  | `text: \|` | a block scalar: the lines of a BASIC program | not together with `typed:`, `address:` or `length:` |
  | `typed:` | `true` or `false`, in any case | only for a file that is a BASIC listing |
  | `address:` | a number up to &FFFF | only for a file that is machine code |
  | `length:` | a number | only for a file that is machine code |

  A field given twice refuses the preset. What `address:` and `length:`
  mean for the placing, and what holds without them, is C1600-2's.

- [ ] **AC-2** — Given `file:`, then the file is read and its kind is
  told when the preset is read, and the preset is refused when the file
  cannot be read, is empty, is of a kind that is not a program, or has a
  field its kind does not take (`typed: true` on tokenized BASIC or on
  machine code; `address:` or `length:` on BASIC)
  (`Core/Preset/PresetFile.cpp:492-534`).
- [ ] **AC-3** — Given a `program:` section with a file whose turn has
  come, then the program is put into the machine by the loader for its
  kind (C1600-2): a BASIC listing or tokenized BASIC is loaded at once, a
  BASIC listing with `typed: true` is typed in, machine code is placed,
  at `address:` when it is given, and started when its file says so.
  Given the loader fails, then the preset fails there with the loader's
  reason (LIB-6 AC-17) (`Core/Preset/PresetRunner.cpp:200-257`).
- [ ] **AC-4** — Given a `program:` section with `text: |`, then its
  lines are typed in as a BASIC program, line by line, by the loader
  that types (C1600-2); the preset has to put the machine into program
  mode before (`- key: mode`), and the load does not do it. Given no
  line was stored, as when the machine was not in program mode, or some
  were not, then the preset fails with a reason that says so and names
  the lines (`Core/PC1600/PC1600BasicTyper.cpp:333-340`).
- [ ] **AC-5** — Given the lines of `text: |`, then they are taken as
  LIB-6 AC-1 says for a block scalar: empty lines and lines that start
  with `#` are left out, and everything else in a line is typed, a ` #`
  included (`Core/Preset/PresetFile.cpp:133-151, 453-470`).

The host drive
- [ ] **AC-6** — Given a preset for the PC-1600 with the header key
  `host-drive:` and a folder as its value (a path, LIB-6 AC-1), then the
  machine is built with the host drive in the place `host-drive`,
  showing that folder as the drive S3: (C1600-1 AC-19; `presets.md`
  PR-11 as amended). Given the value is not a folder that exists, or the
  model is not the PC-1600, then the preset is refused when it is read
  (`Core/Preset/PresetFile.cpp:918-921`,
  `Core/PC1600/PC1600PresetLoader.cpp:105-114`).
- [ ] **AC-7** — Given a frontend that cannot mount a host folder (the
  Web frontend; LIB-4 AC-18), then a preset with `host-drive:` is
  refused when it is read, with that reason.

ROM images
- [ ] **AC-8** — Given the header key `slot-1-rom:` or, on the PC-1600,
  `slot-2-rom:`, with a path, then the module the preset names for that
  slot is built with the bytes of that file as its ROM, read anew at
  every load. Given no module is named for the slot, the key is given
  twice, or the file cannot be read or is not of the size the module's
  ROM has, then the preset is refused when it is read. A module built
  this way is never an instance and is never saved (LIB-7 AC-15) (`Core/Preset/PresetFile.cpp:784-787, 861-873`,
  `Core/Preset/PresetRunner.cpp:21-39`).
- [ ] **AC-9** — Given the header key `bus-rom:`, which has a block that
  is taken raw (LIB-6 AC-1), then the block is a list in YAML, each item
  with `file:` (required) and exactly one of `bank:` (4 to 7; PC-1600
  only) or `address:` (&0000 to &FFFF), and with `address:` optionally
  `me:`, `pv:`, `pu:` (each 0 or 1), which say for which state of the
  LH5801's or LH5803's memory selection (ME) and of its two bank
  flip-flops (PV, PU) the ROM is seen; a key that repeats adds to the
  list. Wherever the key stands in the file, it describes the machine
  that is built (LIB-6 AC-14). Any other shape, and a file that cannot be
  read, is empty or is larger than its bank or than the room from its
  address to &FFFF, refuses the preset when it is read
  (`Core/Preset/PresetFile.cpp:576-642, 922-926`,
  `Core/Preset/PresetBusRomLoader.hpp:17-46`).
- [ ] **AC-10** — Given a preset with `bus-rom:` items, then the machine
  is built with each ROM on the system bus of its 60-pin connector at
  that bank or address, after everything the place keys name; a ROM that
  comes later in the list is found before an earlier one, and before a
  peripheral's ROM, at the same place
  (`Core/Preset/PresetBusRomLoader.hpp:58-62`,
  `Core/tests/pc1600_preset_tests.cpp:935, 1019`; OQ-2).

The `debug:` block
- [ ] **AC-11** — Given the header key `debug:`, at most once, which has
  a block that is taken raw, then the block is a map in YAML with these
  keys: `reset` (`none`, `reset` or `allReset`), `stopOnEntry` (true or
  false), `command` (a text), `listings` and `symbols` (each a list of
  files), `boot` (`debug`), and `program`, a map with `bin` (required),
  `listing`, `source`, `symbols`, `cpu` (`lh5801`, `z80` or `lh5803`),
  `address`, `entry`, `after` (`stopOnEntry`, `call` or `none`),
  `cleanStart`, `bank`, `me`, `pu`, `pv`. It is checked when the preset
  is read, its paths are taken from the preset's folder, and an unknown
  key or a wrong value refuses the preset; what the keys mean is
  C1600-3's (`Core/Debug/AttachKeys.hpp:42-66`,
  `Core/Preset/PresetDebugBlock.hpp:38-128`).
- [ ] **AC-20** — Given the YAML of the two blocks, then it is what
  Calc-U-1600 reads today, no more; empty lines and `#` lines in them
  are skipped as everywhere (LIB-6 AC-1). Its grammar is written into
  the reference chapter of the help from `Core/Yaml.hpp` at the pin
  (LIB-16).
- [ ] **AC-12** — Given a preset with a `debug:` block is loaded, then
  the load does nothing with the block; the debugger asks for it
  (C1600-3) (`Core/Preset/PresetFile.hpp:259-262`).

The product's part of a load
- [ ] **AC-13** — Given LIB-6 AC-53, then this feature delivers what
  Calc-U-1600 tells the loader for the PC-1500, the PC-1500A and the
  PC-1600: when the machine is idle, what is done after the build, and
  the picture of the display. The criteria are LIB-6 AC-45, AC-46 and
  AC-48, and they count for LIB-6's Calc-U-1600 column.

Errors
- [ ] **AC-14** — Given any error of AC-1 to AC-11, then the notice
  gives the line and names the key or field and what was expected, and
  an error inside a raw block starts with `debug:` or `bus-rom:`
  (`Core/Preset/PresetFile.cpp:845-852`; LIB-6 AC-6).

The shipped presets
- [ ] **AC-15** — Given the presets in the source repository that belong
  to Calc-U-1600, then each is in version 2: the 32 under `examples/`,
  those of the tests, of the developer tools, of the developer
  documentation and of the VS Code extension, its templates included
  (ADR 0037; behaviour change 1600-1). In each, only this changes:

  | Version 1 | Version 2 | Why |
  |---|---|---|
  | `format-version: 1` | `format-version: 2` | `presets.md` PR-2 |
  | `plotter: CE-150` | `printer: CE-150` | PR-11 |
  | `- wait: 2` | `- wait: 2s` | PR-5 |
  | `- wait: 0.5` | `- wait: 500ms` | PR-5 |
  | `- key: ent` | `- key: enter`; no other key name changes | LIB-5, decided with its OQ-6 |
  | a header key given twice | the last one only | LIB-6 AC-1 |
  | `floppy:`, `floppy-file:`, `slot-N-file:`, `- saveas:`, `- trace:` | as LIB-7 and LIB-10 write them | PR-8; their specs |

  A `slot-N:` with a module's name stays as it is. The presets that have
  a row of the last line are upgraded when LIB-7 and LIB-10 are built.
  The templates of the VS Code extension, which hold placeholders, are
  upgraded by hand and are not checked (LIB-6 AC-51).

- [ ] **AC-16** — Given each of the 32 examples, when its version 2 file
  is loaded through the harness, then at the end of the load the display
  as text (`lcd`), the BASIC program area, the bytes of every
  machine-code program it placed (LIB-10 `peek`) and the paper (`paper
  marks`; LIB-8 AC-39) are those of its reference run. A reference run
  is recorded before the files are upgraded, with the tools of `b4ea8c8`
  (`pc1500_cli` and `pc1600_cli` with `--preset`, `--lcd-text`,
  `--dump-basic`, `--dump-mem`) from the version 1 file, up to the end
  of its load. The cells of a real-time clock are left out.
- [ ] **AC-17** — Given the preset list of Calc-U-1600 (LIB-6 AC-31),
  then it holds what `examples/bundled.yaml` holds today: as startup
  presets `startup/default-pc1500.pc1500`,
  `startup/default-pc1500a.pc1500a` and `startup/default-pc1600.pc1600`;
  as examples, in the group "BASIC" `basic/hanoi.pc1600`, in the group
  "Plotter" `plotter/lissajou-1500.pc1500a`,
  `plotter/lissajou-1600.pc1600`, `plotter/globus.pc1600`,
  `plotter/biorhythmus_1600.pc1600` and `plotter/ascii.pc1600`, and in
  the group "Machine code" `machine-code/memtest_1500a.pc1500a`
  (`examples/bundled.yaml:12-31`). The Qt6 and the Web frontend bundle
  the same list, each preset with the files it names.
- [ ] **AC-18** — Given the preset list, then the test suite fails
  unless every preset in it has a `preset-name` of at most 18 characters
  and a `description`, is listed once, and shares its name with no other
  listed preset of the same model
  (`Shared/PresetCatalog.cpp:131-147`,
  `Shared/tests/shared_preset_catalog_tests.cpp:37-99`).
- [ ] **AC-19** — Given what the tests of Calc-U-1600 check today about
  these keys and sections, then it holds: `program:` blocks and their
  address forms, the kind following the file, keys and `program:`
  sections in sequence; `slot-N-rom:`; `bus-rom:` in its forms, its
  order and its gating; `host-drive:`; the `debug:` block with its
  paths, unknown keys, numbers, choices and truth values
  (`Core/tests/preset_tests.cpp:480-517, 568-655, 732-850`,
  `Core/tests/pc1600_preset_tests.cpp:846-1019`).

## Edge Cases
- **EC-1** — Given `text: |` with no line below it, then the preset is
  refused when it is read (`Core/Preset/PresetFile.cpp:459`).
- **EC-2** — Given a `program:` section without a block, or with a field
  that is none of AC-1, then the preset is refused when it is read
  (`Core/Preset/PresetFile.cpp:396, 472`).
- **EC-3** — Given two `program:` sections, then the second starts when
  the machine is idle after the first (LIB-6 AC-15); a program the first
  one started and that still runs after an hour does not stop the
  second from starting.
- **EC-4** — Given a `debug:` block with a mistake in a preset that is
  loaded without a debugger, then the preset is refused all the same
  (AC-11), as today; the user guide says that the block is ignored,
  which holds only for a block without a mistake
  (`Core/Preset/PresetFile.cpp:845-852`, `docs/User-Guide.md:891`).
- **EC-5** — Given a preset that was handed over without its folder and
  names a program file, a ROM image or a host drive folder by a path
  that is not absolute, then it is refused when it is read (LIB-6 EC-5).
- **EC-6** — Given `bus-rom:` with `bank:` on a PC-1500 or PC-1500A,
  then the preset is refused when it is read, with a notice that says
  that those models take `address:` (`Core/Preset/PresetFile.cpp:922-926`).

## Technical Requirements (optional)
- These keys and sections are added through the place LIB-6 AC-29 gives;
  the loader itself knows none of them.
- The rest of a preset is not YAML (`presets.md` PR-1).
- A ROM image or a program file a preset names is read at every load and
  never written.

## Open Questions
- [x] **OQ-1** — *Decided as recommended (user, 2026-11-10, batch 5 gate).* Where the loaders are specified. AC-3 and AC-4 need
  them: telling the kind of a program file, loading BASIC, typing a
  program in, placing and starting machine code. The index gives them to
  C1600-2, "as product file kinds of 'open file'", which depends on
  LIB-13 and so on LIB-7; C1600-1 AC-19 says the same. A preset's
  `program:` would then wait for "open file", and so would every
  example. *Recommendation:* C1600-2 becomes the loaders alone
  (dependency: C1600-1), checked through the harness; this feature and
  the Calc-U-1600 column of LIB-13 depend on it; the file kinds of "open
  file" are product data in the LIB-13 spec (rule 2 of the index). The
  index rows C1600-2, C1600-4 and LIB-13 change accordingly.
- [ ] **OQ-2** — What a bus ROM is in the machine (AC-10). The
  descriptors of C1600-1 have no place for it; today it is a card put in
  front of the others on the bus, named after its file in the INSPECTOR
  (`Core/Preset/PresetBusRomLoader.hpp:30-32, 58-62`).
  *Recommendation:* a peripheral on the 60-pin connector that only a
  preset can plug in, defined by the preset as a module definition
  defines a module; it is listed in what is plugged in, and the control
  state offers no control for it. C1600-1 then gets the place and the
  rule for the order.
- [ ] **OQ-3** — `boot: debug` in a `debug:` block makes the debugger
  load the preset so that the machine is built and not started
  (`Qt6/app/debug/DapSession.cpp:432`). LIB-6 builds a machine in one
  step up to key readiness. *Recommendation:* settle it in C1600-3 with
  `pause-on-start` of LIB-10; nothing in this spec depends on it.
- [ ] **OQ-4** — AC-7: a preset with `host-drive:` in the Web frontend.
  Today's behaviour is not recorded. *Recommendation:* refused when it
  is read, as AC-7 says; the one example that uses the key is not in the
  preset list.

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| Product sections for what only a model family has, read by product code under the loader's rules | ADR 0018; `presets.md` PR-3 | 2026-11-10 |
| `program:` has the forms `file:` and `text: \|` on the Sharp models | `presets.md` PR-3 | 2026-11-10 |
| `host-drive:` names the place of the host drive on the PC-1600; the folder is a product kind | `presets.md` PR-11 as amended (user) | 2026-11-10 |
| Presets of version 1 are refused; every shipped preset is upgraded and checked | ADR 0037; behaviour change 1600-1 | 2026-11-10 |
| `printer:` in place of `plotter:`; `wait:` with a unit | `presets.md` PR-5, PR-11; behaviour change 1600-3 | 2026-11-10 |
| `debug:` and `bus-rom:` keep their YAML notation | `presets.md`, facts ("only `debug:` and `bus-rom:` are YAML proper"); LIB-6 AC-1, raw block | 2026-11-10 |
| A host drive folder that does not exist, and a ROM image that cannot be read or does not fit, refuse the preset when it is read, not after the machine was built | Behaviour change 1600-11 (user, batch 5 gate) | 2026-11-10 |
| Proposed in this spec, for the gate: `bus-rom:` and `debug:` are header keys with a block, not sections (AC-9, AC-11) | They describe the machine and the debugger's settings; nothing in them is carried out in file order | 2026-11-10 |
| Proposed in this spec, for the gate: the comparison of AC-16 as the proof that the upgrade changed nothing | ADR 0037 asks for the upgrade, nothing says how it is checked | 2026-11-10 |
