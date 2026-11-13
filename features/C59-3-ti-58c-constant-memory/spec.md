# C59-3: TI-58C constant memory

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: C59-1 (the TI-58C behind the machine interface, and what it
  keeps through a reset, a power cycle and a cold boot), LIB-9 (the save
  folder), LIB-18 (when a TI-58C is built), LIB-3 (notices, the log,
  suspend), LIB-15 (the persistent file system of the Web frontend).
- Contracts and decisions: `docs/contracts/capabilities.md` (C-3; the
  product kinds of Calc-U-59; I-3), `docs/contracts/settings.md` (ST-3:
  the file is at the root of the save folder), `docs/contracts/media.md`
  (Q-2: the file is not a medium); ADR 0021 as amended, 0037;
  `docs/behaviour-changes.md` rows 59-1, 59-12, 59-15.
- Facts: Calc-U-59 at `v2.3.0`; every `path:line` below is at that tag.

In this spec "the calculator" is the real device; the running thing is
"the machine" (glossary). "The fitted cue card" is the cue card that is
in the window above the keys at the moment, of either type a TI-58C can
have: the blank one with the labels the user or a preset wrote on it,
or the pre-printed one of a module. A TI-58C has no magnetic cards, so
its cue card never has bank numbers (glossary, Cue card;
the cue card as such: C59-7).

## User Stories
- As a user of the TI-58C, I want my program and my registers to be
  there when I start the product again, as the real calculator keeps
  them when it is switched off.
- As a user, I want a preset or a startup preset to give me exactly the
  machine it describes, not a mix with what was there before.
- As a user, I want to start a TI-58C from scratch when I ask for it.
- As a user of the Swift frontend on several devices, I want the
  constant memory to follow me through iCloud, as today.

## Out of Scope
- What the TI-58C keeps through a reset, a power cycle and a cold boot
  inside the machine (owner: C59-1, AC-6 there).
- When a TI-58C is built, and from what (owner: LIB-18, AC-19 there).
  This feature supplies the memory and takes it back.
- Where the save folder is on each platform (owner: LIB-9), and the
  persistent file system of the Web frontend (LIB-15).
- Program steps and registers written by a preset (owner: C59-2).
- The cue card as such: its templates, its drawing, `fit`, and what
  Reset does to cue cards on the other models (owner: C59-7). This
  feature only stores the fitted cue card with the memory.
- A constant memory for any other model, and more than one stored
  memory (new functions).

## Acceptance Criteria

The product kind
- [ ] **AC-1** — Given a TI-58C machine, then it lists a product kind
  "constant memory", which this feature defines (`capabilities.md`,
  product kinds; C-3): through it the content of the constant memory can
  be read, with a counter that changes whenever the content changes. A
  TI-59 or TI-58 lists none.
- [ ] **AC-2** — Given a TI-58C is built, then it can be handed a
  constant memory at its build; the machine then comes up as a real
  TI-58C that is switched on again: the ROM's start-up runs on that
  memory, and the program, the registers and what the ROM keeps in its
  own registers (the partition, the display format) are as they were
  left: the memory is in place before the processor starts, and the ROM
  does the rest (`rom/TI58C-ROM-Guide.md`; user, batch 5 gate;
  behaviour change 59-15).

The file
- [ ] **AC-3** — Given Calc-U-59, then the constant memory is kept in
  the file `ti58c.mem` at the root of the save folder (LIB-9 AC-12,
  AC-16); it is not a medium and is in no picker (`settings.md` ST-3;
  `media.md` Q-2).
- [ ] **AC-4** — Given `ti58c.mem`, then it is a text file that states
  its format version and holds the whole constant memory as C59-1 AC-6
  defines it, the registers H00 to H03 included, leaving out the
  registers that are zero, and the fitted cue card if there is one, as
  it is, its template included (OQ-3; today it is written as a blank
  cue card), in today's layout with a first line that states the version
  (`Shared/ConstantMemory.cpp:9-66`).
- [ ] **AC-5** — Given a TI-58C built from the file runs (AC-7, AC-10)
  and its constant memory or the
  fitted cue card changes, then `ti58c.mem` holds the new content at the
  latest half a second after the change, counted in the wall time the
  host passes to the session (LIB-3 AC-2), and is not written when
  nothing changed (`Shared/SessionController.cpp:247-256, 285-286`).
- [ ] **AC-6** — Given a TI-58C built from the file runs (AC-10), then
  `ti58c.mem` is brought up to
  date at once, without waiting for AC-5: before the machine is replaced
  by another, when the session is suspended (LIB-3 AC-24; in the Web
  frontend when the page is hidden, LIB-15 AC-2), and when the product
  quits (`Shared/SessionController.cpp:79, 94, 210`).

When the memory comes back
- [ ] **AC-7** — Given no startup preset is set for the TI-58C, when a
  TI-58C is built at the start of the product or at a model switch
  (LIB-18 AC-19), then it is handed the content of `ti58c.mem` (AC-2),
  and the cue card of the file is fitted again.
- [ ] **AC-8** — Given a TI-58C is built from a preset or from a startup
  preset, then it is handed no constant memory: its memory is zeroed
  before the processor starts, and the machine holds what the ROM's
  start-up leaves in it (C59-1 AC-6; LIB-18 AC-19), and the fitted cue card is the one the preset
  gives, or none (`capabilities.md` C-3; behaviour change 59-12).
- [ ] **AC-9** — Given Cold boot on a running TI-58C, then its constant
  memory is zeroed before the processor starts again and holds what the
  ROM's start-up leaves in it afterwards (C59-1 AC-6).
- [ ] **AC-10** — Given a TI-58C built from a preset or a startup preset
  (AC-8), then it never writes `ti58c.mem`, neither after a change nor
  when it is replaced, suspended or the product quits: the file keeps
  the constant memory of the last TI-58C built from it, which the next
  one built from it gets back (AC-7). AC-5 and AC-6 hold only for a
  TI-58C built from the file. Given Cold boot on a TI-58C built from the
  file (AC-9), then the file follows: at the latest half a second after
  the machine reports key readiness it holds the empty memory and no
  cue card (`docs/formats/media.md` §4).
- [ ] **AC-11** — Given a TI-58C is reset or power-cycled (a Reset, a
  module change, the PC-100C attached or detached; LIB-18 AC-21, AC-22),
  then its constant memory stays in the machine (C59-1 AC-6),
  `ti58c.mem` is not read, and the fitted cue card stays; everything
  else is as after switching on: a running program has stopped and a
  module program that was selected no longer is (C59-1 AC-6; user,
  batch 5 gate). Today Reset reads the file back
  (`Shared/SessionController.cpp:186-187`).
- [ ] **AC-12** — Given the product runs a TI-59 or a TI-58, then
  `ti58c.mem` is neither read nor written, and it still holds what the
  last TI-58C left.

Every frontend
- [ ] **AC-13** — Given Swift, Qt6 and Web, then AC-3 to AC-12 hold in
  each, with the save folder of that frontend (LIB-9 AC-13 to AC-15).
  Today the Web frontend keeps the memory in a `localStorage` key
  (`Web/bindings.cpp:53, 366-370`; LIB-15 AC-20).
- [ ] **AC-14** — Given what the tests of Calc-U-59 check today, then it
  holds: the content survives a round trip through the file, registers
  that are zero are left out, a model switch away from the TI-58C and
  back keeps the memory, and the Web frontend keeps it across a page
  load (`tests/shared_formats_tests.cpp:375-392`,
  `Web/tests/play-calculator.spec.js:181-196`).

## Edge Cases
- **EC-1** — Given there is no `ti58c.mem`, then a TI-58C is built with
  an empty constant memory and no cue card fitted, and the file is
  written at the first change.
- **EC-2** — Given `ti58c.mem` is in an older format version or states
  none (a file of Calc-U-59 2.3.0), then it is not read: the TI-58C is
  built with an empty constant memory, a notice says that the file is
  outdated and where the format is described, and the file is replaced
  at the first change (ADR 0037; behaviour change 59-1).
- **EC-3** — Given `ti58c.mem` states the current version but cannot be
  read (a damaged line), then nothing of it is used, as today, and a
  notice says so; today nothing is said
  (`Shared/ConstantMemory.cpp:9-66`).
- **EC-4** — Given a write of `ti58c.mem` fails, then the machine runs
  on, a notice says so once per start of the product, and the next
  change tries again. Today a failed write is silent
  (`App/EmulatorViewModel.swift:500-505`,
  `Qt6/app/EmulatorController.cpp:27-32`).
- **EC-5** — Given a file with no register lines, then it is a valid
  empty constant memory.
- **EC-6** — Given the startup preset that is set for the TI-58C cannot
  be loaded, so that the built-in default machine is built (LIB-18
  EC-1), then that machine is handed the content of `ti58c.mem` as in
  AC-7.
- **EC-7** — Given a preset or a startup preset fails while it is
  carried out (LIB-18 EC-6), then the machine it left writes no file
  either (AC-10).
- **EC-8** — Given the Web frontend of the first release from this
  repository and a constant memory that 2.3.0 kept in `localStorage`,
  then it is not read (LIB-9 AC-23) and nothing is said; the TI-58C
  starts with an empty constant memory (behaviour change 59-1).

## Technical Requirements (optional)
- The constant memory crosses the machine as content, not as a file:
  the machine reads and writes no file (`capabilities.md` I-3).
- `ti58c.mem` is an own format (ADR 0037), described in the help's
  reference chapter with the other formats.
- A running product never reads `ti58c.mem` again after the build of
  its TI-58C. Where two devices share the save folder through iCloud,
  the file last written wins and nothing is merged, as today.

## Open Questions
- [x] **OQ-1** — *Decided as recommended (user, 2026-11-10, batch 5 gate).* The layout of the new version of `ti58c.mem` (AC-4).
  Today: an optional `CUECARD:` block, a line of dashes with "Registers
  (raw values)", then `Rnnn:` and eight hex bytes per register that is
  not zero, the registers H00 to H03 among them as R060 to R063; no
  version. *Recommendation:* the same layout with a first line that
  states the version, so that today's reader and writer are carried
  over.
- [x] **OQ-2** — *Decided (user, 2026-11-10, batch 5 gate): the real calculator keeps its RAM powered, so the memory is what comes back; it is in place before the processor starts, and the ROM restores the partition and the rest. AC-2 stands; row 59-15.* What comes back at a start (AC-2). Read from the code,
  not run: today the ROM's start-up runs first and the saved registers
  are written afterwards, so the partition and the display format, which
  the ROM rebuilds from one of its own registers at power-on, would come
  from an empty register at a start, and from the saved one after a
  Reset (`Shared/StateApplier.cpp:89-93`; `rom/TI58C-ROM-Guide.md`).
  *Recommendation:* the target is AC-2, the real calculator's behaviour.
  The first task verifies 2.3.0 with a TI-58C that is filled, quit and
  started again; if today differs, AC-2 is a visible fix and gets a row
  in `docs/behaviour-changes.md`.
- [x] **OQ-3** — *Decided (user, 2026-11-10, batch 5 gate): the file remembers the cue card that is fitted and restores it, as it is. A TI-58C has no magnetic cards, so no bank numbers arise.* The fitted cue card in `ti58c.mem` (AC-4). Today it is
  written with its template set to the blank cue card and its bank
  numbers dropped, whatever it was (`Shared/ConstantMemory.cpp:9-30`).
  *Recommendation:* write it as it is.
- [x] **OQ-4** — *Decided as recommended (user, 2026-11-10, batch 5 gate).* EC-3 and EC-4 add a notice where today nothing is said.
  *Recommendation:* add both, with one row in
  `docs/behaviour-changes.md`; a user who loses the constant memory
  should be told why.
- [x] **OQ-5** — *Decided as recommended (user, 2026-11-10, batch 5 gate).* EC-6: a startup preset is set for the TI-58C but cannot
  be loaded. `capabilities.md` C-3 says "with a startup preset set, the
  preset cold boots the machine"; here no preset runs. An empty machine
  would then overwrite the file (AC-10). *Recommendation:* the memory
  comes back as without a startup preset, so that a missing preset file
  does not cost the user the constant memory.

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| The constant memory is a Calc-U-59 product kind | `capabilities.md` C-3 | 2026-11-10 |
| `ti58c.mem` always holds the constant memory of the running TI-58C and follows a cold boot | `capabilities.md` C-3 (user) | 2026-11-10 |
| A restart of the product is a power cycle for the TI-58C: without a startup preset the memory comes back; with one, the preset cold boots it | `capabilities.md` C-3; ADR 0021 as amended; behaviour change 59-12 | 2026-11-10 |
| The file is at the root of the save folder and is not a medium | `settings.md` ST-3; `media.md` Q-2 | 2026-11-10 |
| A `ti58c.mem` of 2.3.0 is refused with a hint; the TI-58C then starts with an empty constant memory | ADR 0037; behaviour change 59-1 | 2026-11-10 |
| Reset on the TI-58C is a power cycle | Glossary, Reset; C59-1 AC-6 | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 5 gate): on a Reset or power cycle the memory stays in the machine and the file is not read (AC-11); the same was decided as C59-1 OQ-6 | The machine keeps the memory itself | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 5 gate): the half second of AC-5 and AC-10 and the moments of AC-6, taken from today; a file with no register lines is valid (EC-5, today's behaviour) | They make the criteria checkable | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 5 gate): the fitted cue card comes back with the memory and stays through a Reset of the TI-58C (AC-7, AC-11); today both happen because the file is read back | Today's visible behaviour kept | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-10, batch 5 gate): the constant memory that the Web frontend of 2.3.0 kept in `localStorage` is not read and nothing is said (EC-8) | LIB-9 AC-23; row 59-1 covers the loss | 2026-11-10 |
| Reset in the emulator keeps the memory and the cue card that is fitted, and loses everything else | user, 2026-11-10, batch 5 gate | 2026-11-10 |
| The constant memory is in place before the processor starts | user, 2026-11-10, batch 5 gate | 2026-11-10 |
| `ti58c.mem` also remembers the fitted cue card, whichever type it is | user, 2026-11-10, batch 5 gate | 2026-11-10 |
| A TI-58C built from a preset or a startup preset never writes the constant memory file; only one built from the file keeps it up to date (half a second after a change, before it is replaced, on suspend, on quit); a Cold boot of such a machine empties the file (AC-5, AC-6, AC-10, EC-7). Replaces "the file follows" for presets (`capabilities.md` C-3) | user, 2026-11-13 (question F-8 of `docs/walkthroughs/presets.md` §5.4); `docs/formats/media.md` §4 | 2026-11-13 |
| "Empty" constant memory after a preset build or a Cold boot is zeroed memory before the processor starts, then what the ROM's start-up leaves (AC-8, AC-9) | user, 2026-11-13 (B-28 of `docs/walkthroughs/presets.md` §5.2): "memory gets cleared *before* the CPU is let loose, in order to simulate real hardware": initialize the model, load the RAM from `ti58c.memory.yaml`, power the CPU; the start-up writes its signature into H00 (`rom/TI58C-ROM-Guide.md:165-176`); first step C-6 confirms the cells | 2026-11-13 |
