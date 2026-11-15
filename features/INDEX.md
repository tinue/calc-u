# Feature index

The only place where a feature's status lives (`CLAUDE.md`). Phase 4b
(`docs/Process.md`). Sources: `Briefing.md`, `docs/contracts/`,
`docs/adr/`, the inventories.

- **`LIB-n`**: library features. Briefing §7: every product uses the
  library for every one of them; a product that keeps its own
  implementation does not meet the success criterion. LIB-16 and LIB-19
  hold no library code (help chapters, calc-u-site); they are `LIB`
  because both products share them. LIB-16's one piece of code is the
  help tool, a tool of the source repository.
- **`C59-n`, `C1600-n`**: product features: product code with behaviour
  of its own that the migration writes or changes (rule 2 below).
  Product functions not listed here are carried over unchanged (for
  example the host drive, cassette fast-load); they are protected by
  their existing tests and the golden tests.
- Not features: repository layout, CMake, CI, releases, the ROM fetch at
  build time, the test runner and test groups (ADR 0002-0005, 0027-0031,
  0034, 0038). They are steps of the migration plan (phase 6).

## Rules of the cut

1. **One provable outcome per feature.** A feature can be shown done by
   tests at its own seam, without waiting for a feature that depends on
   it.
2. **Library row or product row.** Product *data* that fills a library
   slot (a pacing or typing profile, a keyboard map, settings keys and
   defaults, descriptor lists) is specified in the `LIB` spec, as
   acceptance criteria per product. Product *code with behaviour of its
   own* (a core restructuring, a section parser, a format reader or
   writer, a product kind, a panel) gets a `C59` or `C1600` row.
3. **A capability belongs to its consumer.** A machine row (C59-1,
   C1600-1) covers the machine interface and the descriptors only. The
   implementation of a capability kind in a product is specified with
   the feature that uses it: typing support with LIB-5, paper output
   with LIB-8, media places with LIB-7 (the TI card reader: C59-7),
   debug targets with LIB-10, controls, serial port and real-time clock
   with LIB-2.
4. **Steps and harness commands belong to the feature that owns the
   behaviour.** A preset step and its harness command are one vocabulary
   (`docs/contracts/harness.md`, H-1) and are specified once. LIB-11
   holds the program, the protocol and a first command set; LIB-6 holds
   the step engine and the steps over what it depends on (typing,
   places, controls, clock). A feature that depends on them specifies
   its own steps and commands: media (LIB-7), paper (LIB-8), debugger
   (LIB-10), product commands (the product rows).
5. **Pickers and panels belong to the feature whose behaviour they
   show.** LIB-14 holds the family structure and the shell of the
   component sets, not every component.

## Status

Values: Planned, Specified, In progress, Done.

- A library feature has three status columns. **Library**: the library
  part exists and its tests pass. **Calc-U-59**, **Calc-U-1600**: that
  product uses the library for the feature and its acceptance criteria
  for that product pass. The feature is done when all three are Done
  (Briefing §7). "Specified" (the spec is approved) is set in all three
  at once.
- The unit of work in phase 6 is therefore a feature and a column.
- Rule 1 of the cut holds for a Library column and for a product row.
  A product column of a library feature may be complete only together
  with a product row that builds on the library part (for example LIB-3's
  Calc-U-59 column with C59-5, which hosts the session in the Swift
  frontend); the spec says so where it applies.
- The "Depends on" column lists direct dependencies only. In addition,
  a product column of any library feature needs that product's machine
  row first (C59-1, C1600-1).

## Library features

| ID | Name | Description | Contracts, ADRs | Depends on | Library | Calc-U-59 | Calc-U-1600 |
|---|---|---|---|---|---|---|---|
| LIB-1 | Machine interface and descriptors | One machine interface; model and peripheral descriptors in a catalogue; emulated time in ticks and the emulated-time type; display kinds; key IDs; places; capabilities listed by the machine; product kinds; the descriptor content of each product (the product columns) | machine, capabilities; ADR 0006-0008, 0012 | — | Done | In progress | In progress |
| LIB-2 | Peripherals, connectors and small capability kinds | Attach and detach in places, refusal with a reason, what follows an attach (power cycle), peripherals keyed by pin-out, chains; the generic kinds controls, serial port (with the host link) and real-time clock | peripherals; capabilities C-1; ADR 0007, 0010 | LIB-1 (the host link of a serial port: LIB-3, LIB-9) | Done | Done | In progress |
| LIB-3 | Sessions, operations and messages | Library app and machine sessions; operations sliced with live updates; busy and what is refused while busy; audio policy; pause in the background; topics (state, stream, request), commands, product topics and commands, host services | frontend FE-1, FE-3 to FE-6, SP-2; contracts README L-1; ADR 0009, 0014, 0015 | LIB-1, LIB-17 | Done | In progress | In progress |
| LIB-4 | Control state | The generic control state computed from the descriptors and the live machine: models, variants, places and what fits them ("available now" with a reason), media places, panels; product entries | frontend FE-2, FE-6 | LIB-2, LIB-3 | Specified | Specified | Specified |
| LIB-5 | Typing | Typing engine for host keys, on-screen keys and text; holds in emulated time; readiness through typing support; two keys; Paste Text; typing profile and keyboard map per model; the `key` and `type` steps and commands | typing | LIB-1, LIB-3, LIB-11 | Specified | Specified | Specified |
| LIB-6 | Presets | One loader for the family format version 2 (grammar, all or nothing, older files refused); the machine built from a cold boot with the hardware the preset names; the step engine and the steps over typing, places, controls and clock (`key`, `type`, `wait`, `skip-wait`, `let-it-finish`, `regular-speed`, `full-speed`, `expect`, `screenshot`, `elapsed-time`, `printer-trace`, `syncclock`); the place for product sections; the preset list from the bundle file (startup presets and examples); check of shipped and hosted presets | presets; ADR 0016-0018, 0021, 0037 | LIB-2, LIB-5, LIB-9, LIB-11, LIB-18 | Specified | Specified | Specified |
| LIB-7 | Media and save folder | Templates and instances, Name & Save, autosave, names and collisions, outdated files, preset-named media and in-file magnetic cards (`magnetic-cards:`); `card`, `arm`, `floppy`, `tape`, `eject`, `saveas` as steps and commands; the module and media pickers | media; presets PR-8; ADR 0019, 0020, 0025, 0037 | LIB-4, LIB-6, LIB-9, LIB-11 | Specified | Specified | Specified |
| LIB-8 | Paper output | One paper output kind (marks and a text layer); the paper panel; Copy and Cut; `cut-paper` as step and command | capabilities C-2; ADR 0023 | LIB-3, LIB-6, LIB-11 | Specified | Specified | Specified |
| LIB-9 | Settings and paths | One INI file per product, its place per platform, the save folder per platform, keys and defaults | settings; ADR 0021, 0022 | — | Done | In progress | Done |
| LIB-10 | Debugger core | Debug targets, run control (pause, step, resume, levels, pause on start), breakpoints and watches; the run-control buttons; `debug-panel`, `pause`, `pause-on-start`, `step`, `resume` as steps and commands, and `break`, `peek`, `poke` as commands | debugger | LIB-3, LIB-6, LIB-11 | Specified | Specified | Specified |
| LIB-11 | Harness | `calcu`: one-shot commands and `serve`; the line protocol with `--json`; a first command set that needs only the machine and the session (advance emulated time, raw key down and up, display readout, state of a topic, emulated time, capabilities, sound to a file, quit); the place for product commands; the list of today's tools' functions, each mapped to the feature that delivers it. Attach is built with C59-5 (Swift) and LIB-14 AC-44 (Qt6) | harness; ADR 0026 | LIB-1, LIB-3 | In progress | Specified | Specified |
| LIB-12 | ROM lookup | The bundled ROM files found and checked in one place; a product does not start without its full set. The user's ROM folder and own dumps are designed only (ADR 0035) | roms; ADR 0035 | LIB-1, LIB-3 | In progress | Done | In progress |
| LIB-13 | Opening files | One "open file" command for drop, Open With and pickers; classification; the place for product file kinds, and each product's file kinds as product data (Calc-U-1600's on the loaders of C1600-2) | frontend SP-3 | LIB-6, LIB-7 | Specified | Specified | Specified |
| LIB-14 | Family window structure and component-set shell | The family menu and control-bar structure; the settings dialog; "Loading…", notices, About and Help entries; the Qt6 and React component sets' shell, into which the other features' pickers and panels go (rule 5) | ADR 0023, 0024 | LIB-4, LIB-9, LIB-18 | Specified | Specified | Specified |
| LIB-15 | Web frontend shell | One embedding interface, Web Worker host, persistent IndexedDB save folder, installable offline app | frontend FE-7; settings ST-3 | LIB-3, LIB-9 | In progress | In progress | In progress |
| LIB-16 | Help content | Shared chapters (the Library column) and product chapters in Markdown, in the ten-chapter structure; the help release asset `<Product>-help.zip` | help HL-1 to HL-3; ADR 0036 | — | Specified | Specified | Specified |
| LIB-17 | Pacing | One pacer with a pacing profile per model; regular speed and full speed; lag dropped; the long-run rate stays exact, shown by a test per product before and after the pacer moves | frontend SP-1, I-6; machine M-1 (F-1 to F-3) | LIB-1 | Done | In progress | In progress |
| LIB-18 | Machine control and startup | The commands that change the machine: model and variant switch (a new machine), attach and detach, Reset, Cold boot; the start of a product: startup model, the built-in default machine | machine M-6, I-6; peripherals P-1; roms R-3; ADR 0021, 0024 | LIB-2, LIB-3, LIB-9, LIB-11, LIB-12 | Specified | Specified | Specified |
| LIB-19 | Help and Web frontends on calc-u-site | In the calc-u-site repository: rendering the pinned help asset (the Library column); `/59/` moved to it; the new `/1600/` with the published Calc-U-1600 Web frontend | help HL-3; frontend FE-7; Briefing §4.3 | LIB-15, LIB-16 | Specified | Specified | Specified |

## Product features

| ID | Name | Description | Contracts, ADRs | Depends on | Status |
|---|---|---|---|---|---|
| C59-1 | TI machines on the machine interface | TI-58/58C/59 behind the machine interface: advancing in ticks, reset, power cycle and cold boot, key down and up by key ID, segment display output (the descriptors' content: LIB-1) | machine; ADR 0006, 0012 | LIB-1 | Done |
| C59-2 | TI preset content | Product sections (partition, program steps, registers, cue card) on the keystroke program memory product kind; the 57 examples and the hosted copies upgraded; three startup presets | presets PR-3; ADR 0021 | C59-1, C59-6, LIB-6, LIB-8, LIB-10 | Specified |
| C59-3 | TI-58C constant memory | `ti58c.memory.yaml` holds the constant memory of a TI-58C built from it, never written by a machine a preset built; restored on a start without a startup preset | capabilities C-3 | C59-1, LIB-9, LIB-18 | Specified |
| C59-4 | Calc-U-59 debug panel | CALCULATOR, CPU and LOG tabs, FREEZE and FREEZE ON START, the ROM heatmap, on the library's run control; the ASM overlay: product kind, product file kind of "open file", its format with a version | debugger DB-6; frontend SP-3 | C59-1, LIB-10, LIB-13 | Specified |
| C59-5 | Swift frontend hosting | XCFramework built by CMake, session on a background thread, the bridge as a message boundary, harness attach (localhost TCP) | harness H-3, H-4; ADR 0013-0015, 0033 | LIB-3, LIB-11 | In progress |
| C59-6 | PC-100C and module slot as functional connectors | The PC-100C and the 14 Solid State ROM modules as peripherals in the places `slot-1` and `printer` (their descriptors: LIB-1); both taken out of the `TMC0501` class; a machine without a module; power cycle after an attach (behaviour change 59-11) | peripherals P-1; presets PR-9; ADR 0011 | C59-1, LIB-2 | In progress |
| C59-7 | Magnetic cards | The card reader as a pass-through media place that waits for a medium; the card format's new version with the name field; cards in the save folder, also on the web; `card`, `arm` and the corner taps; the cue card of the running module program as a product kind (behaviour changes 59-1, 59-6, 59-8) | media MD-2; presets PR-8; ADR 0019, 0020 | C59-1, LIB-7 | Specified |
| C59-8 | Swift frontend on the family structure | The family menu and control-bar structure in the Swift frontend (iOS without a menu bar), with the native counterparts of the shared pickers and panels and the same scenario IDs (behaviour changes 59-9, 59-14) | ADR 0023-0025, 0032 | C59-5, LIB-4, LIB-6, LIB-7, LIB-8, LIB-10, LIB-14 | Specified |
| C59-9 | Harness product commands of Calc-U-59 | The functions of today's tools that only Calc-U-59 has, as commands under `ti59`: ROM coverage, timings per printed line and per key, hottest program steps and step cost, keycode listing and assembling | harness H-2; LIB-11 AC-14 | LIB-11, LIB-6, LIB-8, LIB-10, C59-2, C59-6 | Specified |
| C1600-1 | Sharp machines on the machine interface | PC-1500/1500A/1600 behind one interface, no model branches above the machine; variants, advancing in ticks, key down and up by key ID, dot-matrix display output, the peripherals in their places on the 60-pin chain (the descriptors' content: LIB-1) | machine, peripherals P-6; ADR 0006 | LIB-1, LIB-2 | Done |
| C1600-2 | Sharp program files | The loaders on the loader memory access product kind: telling the kind of a program file, loading BASIC, typing a program in, placing and starting machine code, tape fast-load; checked through the harness. Their use as file kinds of "open file" is product data in LIB-13; the two menu items and their dialogs | frontend SP-3 | C1600-1, LIB-5, LIB-11, LIB-14 | Specified |
| C1600-3 | DAP in the app layer | The DAP server out of the Qt frontend, on the library's debug target and run control, without Qt types | debugger DB-4 | LIB-10, LIB-11, C1600-2, C1600-4 | Specified |
| C1600-4 | Sharp preset content | Product sections and keys (`program:` with `file:` and `text:`, `host-drive:`, `bus-rom:`, `slot-N-rom:`, the `debug:` block); the 32 examples upgraded to version 2; its bundle file (`docs/formats/bundle.md`; behaviour changes 1600-1, 1600-3) | presets PR-3, PR-5, PR-11; ADR 0018 | C1600-1, C1600-2, LIB-6, LIB-8, LIB-10 | Specified |
| C1600-5 | Harness product commands of Calc-U-1600 | The functions of today's tools that only Calc-U-1600 has, as commands under `pc1600`: the BASIC dump, INSPECTOR views as text, cassette files, the peer of a serial port (the CE-158's and the PC-1600's own), the host drive | harness H-2; LIB-11 AC-18 | LIB-11, LIB-7, LIB-10 | Specified |

## Open points for the specs

Questions the contracts hand to phase 4b
(`docs/contracts/README.md`), each with the spec that answers it:

| Question | Answered in |
|---|---|
| `frontend.md` Q-3: what "idle" means per model for a bare `- wait:` | LIB-6 |
| `frontend.md` Q-4: what "open file" does with a medium outside the save folder | LIB-13 |
| `capabilities.md` Q-2: where the readers and writers of the media formats sit | LIB-7 (the card format: C59-7) |
| `capabilities.md` Q-3: the built-in default machine of the TI-58 and TI-58C | LIB-18 |

Points this index leaves open:

- **Traces.** `debugger.md` keeps the trace file formats per product,
  and `trace` is a common step and command. Whether the trace writer
  (start, stop, size limit) is library code is not decided. Settled
  (user, batch 6 gate): it is library code in LIB-10; the two file
  formats stay per product and are stated there. No product row.
- **Calculator view.** ADR 0023 does not list the display or the
  on-screen keys among the shared components; the divergence matrix
  suggests merging the hit test (M16) and keeps the display per product
  (M52). Whether rendering a display kind and the hit test go into the
  component sets is to be settled in the LIB-14 spec. Settled (user,
  batch 5 gate): the component sets find the key under a pointer by one
  rule from the product's hit map; drawing the display and the pressed
  keys stays with each product (LIB-14 AC-24).
- **Switches of the Sharp printers.** The CE-150 and the CE-1600P each
  have a switch equivalent to the PC-100C's TRACE switch, which Calc-U-1600
  does not emulate yet (user, 2026-11-13). When it is, it belongs to its
  printer as TRACE does (C59-6 AC-6) and gets a preset step and a default
  (LIB-6 AC-65). Not in the migration.
- **Sharp media formats.** `.card.yaml` stays at version 1;
  `.floppy.yaml` goes to version 2 (behaviour change 1600-19). If moving their readers and writers under
  the media model (`capabilities.md` Q-2) turns out to be more than
  per-product criteria of LIB-7, they get a `C1600` row. Settled (user,
  batch 6 gate): product code behind one seam of LIB-7 (AC-46 there);
  no row.
