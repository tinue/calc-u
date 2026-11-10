# Contract: preset engine and family preset format

Status: draft (2026-11-10). Forks PR-1 to PR-11 decided (decision log;
PR-11 was added in the phase 4 review); open questions remain.

Inherits: ADR 0016 (a preset always cold boots), 0017 (all-or-nothing
parsing), 0018 (one family preset format, YAML-shaped, product-specific
sections), 0021 (startup presets; three bundled Calc-U-59 startup
presets), 0037 (only the current version is read; older files get a
hint; shipped and hosted files are upgraded; a one-time exception for
the migration release, after which a release reads one version back).
Oddities: ODD59-9 (`#`
anywhere), ODD59-10 (registers written after the program), ODD59-13 (Adv
needs `RegularSpeed:`), Calc-U-1600's preset rules
(`docs/oddities-triage.md`). Builds on `typing.md` (T-1 to T-4),
`peripherals.md` (P-1) and `machine.md` (M-3 key IDs, M-8 places).

## Today (facts)

| | Calc-U-59 (`reference/StateFileFormat.md`) | Calc-U-1600 (`docs/User-Guide.md` §8) |
|---|---|---|
| Files | `.ti59`, `.ti58`, `.ti58c`; 57 in `examples/`, mirrored on calc-u.org (`59/software-files/`, `59/calc/presets/`) | `.pc1500`, `.pc1500a`, `.pc1600`; 32 in `examples/` |
| Version | none (version 1 per Briefing §1.2) | `format-version: 1`, required |
| Shape | line sections `MODEL:`, `PARTITION:`, `PROGRAM:`, `REGISTERS:`, `KEYSTROKES:`, `CUECARD:`, `CARD: mem:…`/`END-CARD`, `SOLID-STATE-MODULE:`, `PRINTER:`; case-insensitive | "YAML-shaped, not YAML": flat `key: value`, `- verb: value` steps, one `text: \|` block; only `debug:` and `bus-rom:` are YAML proper. A real YAML reader "would strip ` #1` from `CLOSE #1` and choke on `PRINT "A: B"`" (`Decisions.md`, Preset format) |
| Order | sections after `KEYSTROKES:` form reloads, in file order | `keys:` and `program:` may repeat, run top to bottom |
| Bulk content | `PROGRAM:` keycodes, bare or step-numbered with labels, up to 960 steps; `REGISTERS:` `NN = value`, `HNN` on the TI-58C | `program:` `file:` (BASIC, tokenized, machine code) or `text: \|` |
| Keys | KEYSTROKES: matrix codes (row × 10 + col), several per line; words on a line are comments | `- key: name`, one per step; `- type:` a line plus ENTER |
| Running | applied live at regular speed, a demo; keys wait for the scan loop; `FullSpeed:`/`RegularSpeed:` | full speed behind "Loading…"; bare `- wait:` waits until idle |
| Steps / directives | `Wait: 2s`, `FullSpeed:`, `RegularSpeed:`, `Trace:`, `ZeroElapseTime:`, `ReportElapseTime:`, `ArmCard:`, `InsertCard:`, `PrinterTrace:`, `CutPaper:`, `DebugPanel:`, `Freeze:`, `FreezeOnStart:`, `Step:`, `Resume:` | `key:`, `type:`, `wait:`, `expect:`, `saveas:`, `screenshot:`, `syncclock:`, `trace:` |
| Hardware | `SOLID-STATE-MODULE: ML`, `PRINTER: on` | `model: PC-1600:new`, `plotter:`, `interface:`, `floppy:`/`floppy-file:`, `slot-1:`/`slot-1-file:`, `slot-1-rom:`, `host-drive:`, `bus-rom:` |
| Naming | CamelCase directives | kebab-case keys; Sharp product names with the hyphen |
| Comments | `#` anywhere (ODD59-9) | `#` line or ` # …` after a value, except `type:` (typed as written) |
| Other | — | `preset-name:`, `description:`, `debug:` block (DAP project settings) |

Calc-U-59's own comparison (`ideas/PresetConvergence.md`) concluded that
two syntaxes were better; ADR 0018 decided one format instead. Its "cheap
convergence" points (wait units `2s`/`500ms`, lowercase spelling, a format
version, one naming style for new keys) and its feature lists are input
to the second batch.

## Responsibilities

- One **preset loader** in the library parses every preset of every
  product completely before anything is built (ADR 0017), refuses an
  older or unversioned file with a hint to upgrade (ADR 0037), then
  builds the machine from a cold boot with the hardware it names (ADR
  0016) and runs its content in file order.
- Products contribute **sections** for what only their model family has
  (ADR 0018), parsed by product code under the loader's rules.
- Every preset a product ships or calc-u.org hosts is in the current
  version and passes the reader before a release (ADR 0017, 0037).
- Calc-U-59 ships three startup presets, one per model (ADR 0021).
- A preset that builds a TI-58C cold boots it like any other machine;
  `ti58c.mem` follows and then holds the new machine's constant memory
  (`capabilities.md`, C-3). No preset key restores it.

## Non-responsibilities

- How a step types keys: the typing engine (`typing.md`).
- Where named media come from and how they are saved: contract 6.
- The `debug:` block's meaning: contract 8.

## Invariants

- I-1: The same preset gives the same machine every time (ADR 0016).
- I-2: A preset either loads completely or not at all (ADR 0017).
- I-3: A preset means the same thing in every product (Briefing §1.5):
  a key or step that exists in two products has one name and one
  meaning.

## Open forks, first batch

### PR-1: Syntax base

**Decided (2026-11-10): option (a), a YAML-shaped subset with a written grammar.**

Options:
- (a) **Recommended:** a documented **YAML-shaped subset**, extended from
  Calc-U-1600's: `key: value` header fields, sections that may repeat
  and run in file order, `- verb: value` steps, `|` block scalars, and
  values that are taken raw where the content needs it (`type:`).
  Parsed by the library's own parser, with a written grammar in the
  format reference. Keeps BASIC text (`CLOSE #1`, `PRINT "A: B"`)
  typeable as written.
- (b) Real YAML (a YAML library, one `run:` list instead of repeated
  sections): any YAML tool can read it, but every BASIC line with `#`,
  `:` or quotes must be quoted or put in a block, and repeated sections
  have to become a list.

### PR-2: Version number and file extensions

**Decided (2026-11-10): option (a), `format-version: 2`, extensions per model.**

Options:
- (a) **Recommended:** the family format starts at `format-version: 2`
  in both products, so every file of either product written before is
  "older" by the same rule (Calc-U-59's are version 1 by Briefing §1.2,
  Calc-U-1600's say 1). The extensions stay per model (`.ti59`, `.ti58`,
  `.ti58c`, `.pc1500`, `.pc1500a`, `.pc1600`): file associations and the
  pickers keep working; `model:` stays required inside.
- (b) Version 1 of the family format, restarting the count: a
  Calc-U-1600 `format-version: 1` file would then look current.
- (c) One family extension (for example `.calcu`): one association per
  product instead of per model, but users lose the model at a glance.

### PR-3: TI bulk content (program steps, registers, cue card)

**Decided (2026-11-10): option (a), block scalars in today's notation.**

Facts: `PROGRAM:` runs up to 960 steps; listings use step numbers,
labels and gap markers; `REGISTERS:` uses `NN = value`. A list item per
step would multiply the file size.

Options:
- (a) **Recommended:** product sections carry their bulk content as `|`
  block scalars in today's notation, parsed by the TI section parser:
  `program:` with `steps: |` (the family's `program:` then has three
  forms, `file:`, `text: |` and `steps: |`, all meaning a program in
  the glossary's sense), `registers: |`, `cue-card: |`, `partition:`.
  Reloads (ODD59-10, registers after the program) become repeated
  `program:` / `registers:` sections in file order, as Calc-U-1600's
  repeated sections already work.
- (b) YAML lists (`- 43`): bloats every file.
- (c) Separate files next to the preset: a preset no longer travels as
  one file (Calc-U-59 loads presets as bytes on iOS and the web,
  `ideas/PresetConvergence.md`).

### PR-4: How a preset runs: speed and visibility

**Decided (2026-11-10): option (a), full speed by default, a speed step for paced playback.**

Facts: Calc-U-59 presets are demos played live at regular speed;
Calc-U-1600 presets run at full speed behind "Loading…". T-4: user keys
are dropped while an operation runs.

Options:
- (a) **Recommended:** building the machine (cold boot, hardware,
  memory contents) always runs at full speed. The steps then run at full
  speed by default; a speed step (today's `RegularSpeed:`) makes the
  following steps run paced and visible, and back. The whole preset is
  one operation: busy until its last step, user keys dropped (T-4); the
  frontend shows "Loading…" only while it runs at full speed.
- (b) Live by default (Calc-U-59): every Calc-U-1600 preset gets slower.
- (c) Always full speed (Calc-U-1600): the Calc-U-59 demos are lost
  (breaks Briefing §1.1).

## Open forks, second batch

### PR-5: Key and wait steps

**Decided (2026-11-10): option (a).** The user's condition, confirmed:
one syntax for every product as far as the hardware allows, so
`- key: mode cl` works on the PC-1600 as `- key: 2nd Pgm` does on the
TI-59.

Facts: Calc-U-59 KEYSTROKES lines carry several matrix codes and words
as comments (`21 31  92 82  71 95`); Calc-U-1600 has `- key: name`, one
key per step, and `- type:` for a line plus ENTER. Calc-U-59 `Wait: 2s`
requires a unit; Calc-U-1600 `- wait: 2` is seconds, a bare `- wait:`
waits until idle. Names: CamelCase (59), kebab-case (1600).

Options:
- (a) **Recommended:** `- key:` takes one or more key IDs (M-3)
  separated by spaces, typed in turn by the typing engine
  (`- key: 2nd Pgm 0 1 SBR =`); matrix codes go. `- wait:` needs a unit
  (`2s`, `500ms`) and counts emulated time; a bare `- wait:` waits until
  idle (both products). `- type:` exists where the model types text.
  Every key and step name is kebab-case and lowercase in the docs
  (`speed: full` / `speed: regular`, `elapsed: zero` / `elapsed: report`).
  `expect:` and `screenshot:` are generic steps, available to every
  product.
- (b) One key per `- key:` step: TI presets with long key sequences grow
  several times.

### PR-6: Comments and raw values (ODD59-9)

**Decided (2026-11-10): option (a).**

Options:
- (a) **Recommended:** Calc-U-1600's rule for the whole format: a `#`
  line, or ` #` after a value, is a comment; raw values (`type:`,
  `expect:`) are taken to the end of the line, `#` included. Inside the
  TI block scalars (`steps:`, `registers:`, `cue-card:`) ` #` starts a
  comment, as today. ODD59-9 (`#` anywhere) goes with the v1 reader.
- (b) `#` anywhere, everywhere: BASIC text with `#` could not be typed.

### PR-7: GUI-control steps

**Decided (2026-11-10): option (a), and Calc-U-1600 supports them too**
(the user's addition): debugger, paper and screenshot steps work in
both products, which makes Calc-U-1600's screenshots (for example for
the user guide) scriptable. This is a new Calc-U-1600 capability, decided
by the user.

Facts: Calc-U-59 presets press the controls around the calculator:
`PrinterTrace:` (the PC-100C TRACE switch), `CutPaper:`, `DebugPanel:`,
`Freeze:`, `FreezeOnStart:`, `Step:`, `Resume:`; screenshot sequences and
UI tests use them (`examples/screentest/test_cdebug.ti59`). Calc-U-1600
has none. Briefing §1.1: no feature may break.

Options:
- (a) **Recommended:** all are kept. Debugger steps get generic names
  (`debug-panel:`, `pause:`, `pause-on-start:`, `step:`, `resume:`),
  their meaning per product is the debugger contract's (8). Paper:
  `cut-paper:` for every paper panel. A peripheral's own controls are
  named with the peripheral (`- switch: PC-100C TRACE on`), following M-3
  (peripheral keys belong to the peripheral).
- (b) Drop them: breaks Calc-U-59's screenshot presets and UI tests.

### PR-8: Media in presets

**Decided (2026-11-10): fit and insert, re-proposed after the user's
comments** (the first proposal, below, is kept for the record):
- `- fit: <card>` (TI only) places a magnetic card or cue card in the
  window above the top row of keys. It affects only the GUI: the card
  labels the keys and its corners can be tapped; the machine sees
  nothing.
- `- insert: <place> …` (generic) brings a medium to a media place. A
  **pass-through** place (the TI card reader) takes the medium through
  and gives it back: `- insert: card-reader 2` is the corner tap on side
  2 of the fitted card, `- insert: card-reader mem:Bank1` feeds a card
  directly without fitting it; playback waits until it has passed. A
  **holding** place (a floppy drive) keeps it: `- insert: floppy
  Formatted A` mounts the disk, ready at once, until `- eject: floppy`.
  The descriptor marks each media place pass-through or holding.
- `- arm: <place> <medium>` lays a medium ready; it is fed when the
  machine reports, through the media capability, that it waits for one
  (the TI card reader on `n Write` / `n INV Write`).
- Sides are generic: 1 and 2 on a magnetic card, A and B on a floppy
  disk.
- A `medium:` section defines a medium in the preset (today's `CARD:
  mem:` … `END-CARD`); it is never saved. `- saveas:` stays.

First proposal:

Facts: Calc-U-59 defines in-memory cards in the preset (`CARD: mem:name`
… `END-CARD`, never saved), feeds cards with `InsertCard:` (the user
pushing a card in; waits until it has passed) and `ArmCard:` (laid ready;
fed when the ROM waits for a card, PC `0x16B2-0x16B7`), and names card
files in the card folder (`Results`, `Results:2`). Calc-U-1600 names media
in the header by name or file (`floppy:`, `floppy-file:`, `slot-1:`,
`slot-1-file:`) and saves with `- saveas:`. ADR 0019: a medium a preset
names by path is never written; its first write makes an instance in
the save folder.

Options:
- (a) **Recommended:** generic media steps over the descriptor's media
  places: `- insert: <place> <medium>` (InsertCard) and `- arm: <place>
  <medium>` (ArmCard; fed when the machine reports that it waits for a
  medium, through the media capability). A medium is named by name (a
  template or a save-folder instance), by `file:` (relative to the
  preset, ADR 0019), or as a medium defined in the preset (`medium:`
  section, today's `CARD: mem:` block), which is never saved. `- saveas:`
  stays. The same steps work for any media place, though using them
  beyond today's features (a floppy swap mid-preset) is not a goal.
- (b) Keep the card steps TI-specific: two vocabularies for one concept.

### PR-9: Hardware keys

**Decided (2026-11-10): option (a); a TI machine without a module is
allowed** (the slot empty, as on the real calculator). Answers Q-2.

Facts: Calc-U-59: `SOLID-STATE-MODULE: ML` (IDs `ML ST RE SY …`),
`PRINTER: on`, both "keep the current one" when absent. Calc-U-1600:
`plotter:`, `interface:`, `floppy:`, `slot-1:`, `slot-2:`, `host-drive:`;
absent means none (ADR 0016).

Options:
- (a) **Recommended:** the keys name the places as the user sees them,
  with Calc-U-1600's keys kept and the TI-59 brought in line:
  `slot-1:` for the module slot (a module by name or ID) and `printer:
  PC-100C`. Absent means none (ADR 0016), so a TI preset without
  `slot-1:` has no module, which no Calc-U-59 GUI offers today (Q-2).
- (b) Product-specific keys (`solid-state-module:`): two names for "the
  module in the slot".

### PR-11: One key for the printer; hardware keys are places

**Decided (2026-11-10, in the review; approved by the user).** Amends
PR-9.

Facts: PR-9 kept Calc-U-1600's `plotter:` and gave the TI-59 `printer:`.
The glossary has one term, Printer, "a peripheral that prints or plots:
PC-100C, CE-150, CE-1600P". I-3 asks for one name per thing. Every
Calc-U-1600 preset is rewritten for version 2 anyway (ADR 0037).

Options:
- (a) **Recommended:** `printer:` in every product (`printer: PC-100C`,
  `printer: CE-150`, `printer: CE-1600P`); `plotter:` goes. The hardware
  keys of a preset are the **places** of the model's descriptor
  (`machine.md`, M-8: `slot-1`, `slot-2`, `printer`, `interface`) and its
  media places (`floppy`), so the loader finds the connector through the
  descriptor and does not know any model. `host-drive:` stays a
  Calc-U-1600 key (a product section, ADR 0018).
- (b) Keep `plotter:` for the Sharp models: two keys for one glossary
  term.

### PR-10: ODD59-13 (Adv needs `RegularSpeed:`)

Resolved by T-1, no fork: Adv repeats are gated by CPU steps, and a
press is now held for emulated time, so one press feeds the same lines
at every speed. The oddity and its rule go.

## Fit table

| | TI-59 (58, 58C) | PC-1500 (1500A) | PC-1600 | PC-1403 (paper) | PC-1350 (paper) |
|---|---|---|---|---|---|
| Extension (PR-2) | `.ti59`, `.ti58`, `.ti58c` | `.pc1500`, `.pc1500a` | `.pc1600` | not named yet (a model extension, by PR-2) | not named yet |
| Hardware keys (PR-9, PR-11) | `slot-1:` (Solid State ROM module), `printer: PC-100C` | `slot-1:`, `printer: CE-150`, `interface: CE-158` | `slot-1:`, `slot-2:`, `printer:`, `interface:`, `floppy:`, `host-drive:` | 11-pin peripheral | RAM card slot, 11-pin and 15-pin peripherals |
| Product sections (PR-3) | `partition:`, `program:` `steps: \|`, `registers: \|`, `cue-card: \|` | `program:` `file:` / `text: \|` | as PC-1500 | as PC-1500 | as PC-1500 |
| `key:` (PR-5) | `- key: 2nd Pgm 0 1 SBR =` | `- key: mode cl` | `- key: mode cl` | key IDs of the model | key IDs of the model |
| `type:` | — (no text typing) | yes | yes (KBII accents) | yes | yes |
| Media places (PR-8) | card reader (TI-59; pass-through, sides 1/2); `fit:` | cassette (holding) | floppy (holding, sides A/B), cassette | cassette (CE-126P) | cassette |
| `arm:` | card reader | — | — | — | — |
| GUI steps (PR-7) | debugger, `cut-paper:`, `- switch: PC-100C TRACE …`, `screenshot:` | debugger, `cut-paper:`, `screenshot:` | debugger, `cut-paper:`, `screenshot:` | same | same |

## Open questions

- Q-1 (phase 6): The upgrade of the 57 Calc-U-59 and 32 Calc-U-1600 example files
  and the calc-u.org copies (ADR 0037) is implementation work in phase 6;
  whether a throwaway script does it is the implementer's choice.
- Q-2 (answered 2026-11-10, PR-9): a TI machine without a module is
  allowed.
- Q-3 (phase 6, with the descriptors): The key IDs of each model (M-3) and their ASCII spellings
  (`x<>t`, `sqrt`, …) are the descriptor's; the preset format uses them as
  they are.

## Decision log

| Date | ID | Decision |
|---|---|---|
| 2026-11-10 | PR-1 | The family format is a documented YAML-shaped subset (header fields, repeatable sections in file order, `- verb: value` steps, `\|` blocks, raw values where needed), read by the library's own parser. |
| 2026-11-10 | PR-2 | `format-version: 2` in both products; extensions stay per model; `model:` required. |
| 2026-11-10 | PR-3 | TI bulk content as `\|` block scalars in today's notation: `program:` with `steps: \|`, `registers: \|`, `cue-card: \|`, `partition:`; reloads are repeated sections in file order. |
| 2026-11-10 | PR-5 | `- key:` takes one or more key IDs in every product (`- key: mode cl`, `- key: 2nd Pgm 0 1`); matrix codes go. `- wait:` needs a unit and counts emulated time; bare `- wait:` waits until idle. kebab-case names; `expect:` and `screenshot:` in every product. |
| 2026-11-10 | PR-6 | `#` line or ` #` after a value is a comment, except in raw values (`type:`, `expect:`); ` #` also comments inside TI blocks. ODD59-9 goes. |
| 2026-11-10 | PR-7 | GUI-control steps kept with generic names (`debug-panel:`, `pause:`, `pause-on-start:`, `step:`, `resume:`, `cut-paper:`, `- switch: PC-100C TRACE on`), supported in both products. |
| 2026-11-10 | PR-8 | `fit:` (TI, GUI only) and generic `insert:` / `eject:` / `arm:` over media places marked pass-through or holding; sides generic; `medium:` sections for preset-defined media; `saveas:` stays. |
| 2026-11-10 | PR-9 | Hardware keys shared (`slot-1:`, `slot-2:`, `printer:`, `plotter:`, `interface:`, `floppy:`, `host-drive:`); absent means none; a TI machine without a module is allowed. |
| 2026-11-10 | PR-10 | ODD59-13 resolved by T-1 (emulated-time holds); no rule needed. |
| 2026-11-10 | PR-11 | `printer:` in every product, `plotter:` goes; a preset's hardware keys are the places and media places of the model's descriptor. Amends PR-9. |
| 2026-11-10 | PR-4 | Building runs at full speed; steps run at full speed by default, a speed step makes the following steps paced and visible. The whole preset is one operation (busy, user keys dropped); "Loading…" only while at full speed. |
| 2026-11-10 | PR-11 (amended in phase 4b, user) | The host drive is a peripheral on the 60-pin connector, as in the code; `host-drive:` names its place on the PC-1600. The folder it shows stays a Calc-U-1600 product kind. |
| 2026-11-10 | PR-5 (amended in phase 4b, user) | The machine takes exact key IDs. In a preset, a TI second-function name (`Pgm`) stands for 2nd and its key; a `2nd` written before it is not pressed twice. |
| 2026-11-10 | I-2 (sharpened in phase 4b, user, 2026-11-10, batch 5 gate) | Everything that needs no running machine is checked before the build, and a build the machine refuses leaves the running machine alone. A preset that fails while it is carried out leaves the machine as it is at that moment. The step-list section is `keys:`; each header key appears once; the first error is reported. |
| 2026-11-10 | PR-8 (completed, user, 2026-11-10, batch 6 gate) | A medium is written in a preset as its name (in double quotes when it holds a space), `file:<path>` or `mem:<name>`, then the side; `slot-N-file:` and `floppy-file:` go; `saveas:` keeps its form. |
| 2026-11-10 | PR-8 (completed, user, 2026-11-10, batch 7 gate) | `fit:` takes a magnetic card (TI-59); a cue card of another kind is fitted by a `cue-card:` section. An inserted card stays at the reader with the card switch closed until the ROM takes it; nothing gives it back by time. |
