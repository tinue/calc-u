# Contract: capabilities and product kinds

Status: draft (2026-11-10), added in the phase 4 review. C-1 and C-3 are
decided; C-2 is proposed and waits for the review gate.

Inherits: ADR 0007 (a small machine interface plus generic capabilities;
"the capabilities and their contracts are written in phase 4"; extensions
named after a product only for functions that are one of a kind), 0006
(the machine is the only seam). Builds on `machine.md` (M-5: the machine
lists the capabilities present now and hands out the typed interface of
each kind; M-7: product kinds), `peripherals.md`, `typing.md` (T-2),
`media.md`, `presets.md` (PR-7, PR-8).

This contract lists the kinds and says in a few lines what each offers.
The typed interface of a kind is written with the feature that builds it
(phase 4b, phase 6), not here.

## Today (facts)

What the app layers ask of their machines today, beyond the machine
interface:

| | Calc-U-59 (`TI59Machine`) | Calc-U-1600 (`PC1500Machine`, `PC1600Machine`) |
|---|---|---|
| Media | `insertCard`, `cardEject`, card data, the ROM waiting for a card (PC `0x16B2-0x16B7`) | `attachSlot1Card`, `attachSlot2Card`, `attachExpansionCard`, `detachSlot…`, `ce1600fHasDisk`, `ce1600fSide`, `ce1600fSetSide`; `contentRevision()` for autosave; `TapeDeck` |
| Paper | printer lines and code lines (`printerLines`, `PrinterCodeLine`, `lineDots`): rows of dots and text; cut | plot points (`ce1600pPlotPoints`, `ce1600pPlotRevision`, `Shared/PlotterPaper`, `PaperKind`): pen strokes in colours; clear, cut; CE-158 printer port: bytes as sent (Save…, Clear) |
| Controls on a peripheral | PRINT, ADV, TRACE of the PC-100C (K-line signals) | write-protect switch of the CE-161 (`hasWriteProtectSwitch`, `setWriteProtected`) |
| Serial | — | `setSerialLink`; PC-1600 RS-232C and CE-158 as host pseudo-terminals |
| Clock | — | real-time clock at the emulated rate, seeded from the host; preset `syncclock:` |
| Typing | scan-loop ranges (`MachineModelInfo`) | command-loop windows, key-matrix reads, SHIFT and KBII latches |
| Power | no off | `isPoweredOff`, `isDisplayOn` |
| Memory access for loaders | RAM written directly by `StateApplier` (program steps, registers, partition) | `memory()` for the BASIC and machine-code loaders |
| One of a kind | ASM overlay, ROM heatmap, constant memory (`ti58c.mem`), cue card of the running library program | host drive (`setHostDriveDirectory`, …), `debugInspect` views, CE-1600P ROM switch |

Sources: `docs/inventory/calc-u-59.md` and `calc-u-1600.md`
(Capabilities, Modelling depth); Calc-U-1600 `Shared/MachineSession.cpp`,
`Shared/AppSession.cpp`, `Core/Connector/ExpansionCard.hpp` at the pin.

## Responsibilities

- A **generic kind** is defined once in the library and handled the same
  way for every product (ADR 0007). The library's pickers, panels, preset
  steps and harness commands are written against generic kinds only.
- A **product kind** is defined by one product, reached through the same
  list of capabilities (M-5, M-7), and used only by that product's app
  layer and frontends. The library transports it and never names it.
- Every capability has an owner: a component, or a peripheral in a place
  (M-5).

## Non-responsibilities

- The typed interfaces themselves (with the features, phase 4b and 6).
- Files: a capability takes and gives content; reading and writing files
  is above the machine (`media.md`, `machine.md`).

## Invariants

- I-1: The library depends on no product kind.
- I-2: A function that two products need is a generic kind, never two
  product kinds with the same job.
- I-3: A capability never reaches the host: no files, no wall time, no
  threads (`machine.md`, non-responsibilities).

## Generic kinds

| Kind | Offers | Used by the library for |
|---|---|---|
| **Media place** | whether it is pass-through or holding; its sides and the current side; insert (content in) and eject (content out); a change counter for autosave; write protection where the medium has it; "waits for a medium" where the ROM does (TI card reader) | pickers, Name & Save and autosave (`media.md`); `insert:`, `eject:`, `arm:` (PR-8) |
| **Paper output** (C-2) | what was printed since a mark, as an append-only stream; cut and clear; the paper's width | the paper panel (ADR 0023), `cut-paper:`, Copy (text and image), harness printout |
| **Controls** | the keys and switches of a peripheral or component that are not machine keys (M-3), each with an ID and its state | `- switch:` (PR-7), the buttons a panel shows, the harness |
| **Typing support** | key readiness, command-prompt readiness, latch states, each where the model has it (`typing.md`, T-2) | the typing engine; pacing (`frontend.md`, sessions and pacing) |
| **Serial port** | bytes in and out, line settings as far as the model has them | the host link (a pseudo-terminal, ADR 0009), harness serial commands |
| **Real-time clock** | reading and setting the clock | `syncclock:`, seeding from the host at build time |

The serial port and the real-time clock are generic by ADR 0007 although
one product has them today; the library does for them only what the rows
say.

## Product kinds (named here, specified with their features)

Not a complete list; each is specified with the feature that needs it.

- Calc-U-59: keystroke program memory (partition, program steps,
  registers, for the preset sections of PR-3); ASM overlay; ROM heatmap;
  the cue card of the running library program; constant memory (C-3).
- Calc-U-1600: memory access for the BASIC and machine-code loaders; host
  drive; the CE-158 printer port's byte output; the INSPECTOR views.

## Forks

### C-1: Which kinds are generic

**Decided (2026-11-10, in the review; the user approved the rule).** The
kinds ADR 0007 names are generic (media, serial port, real-time clock,
printer output). Beyond those, a function starts as a product kind and
becomes a generic kind when a second product needs it; controls and
typing support are generic because both products need them today.

### C-2: Paper output, one kind or several

**Proposed; waits for the review gate.**

Facts: the PC-100C prints rows of dots with their text (20 characters a
line; column timing matters for a line cut short by the printer
interrupt); the CE-150 and CE-1600P draw pen strokes in four colours;
both products have a paper panel with Copy and Cut (`divergence-matrix.md`,
M46). A CE-126P (SC61860, paper machine) prints rows of dots.

Options:
- (a) **Recommended:** one paper output kind whose stream carries one of
  two content forms, named by the owner: **dot rows** (with their text)
  or **pen strokes** (with colour). One paper panel per component set
  renders both forms; Copy and Cut are the same.
- (b) Two kinds, a dot printer and a plotter: two paper panels per
  component set for the same user concept.

### C-3: TI-58C constant memory

**Decided (2026-11-10, user).** A Calc-U-59 product kind.

- `ti58c.mem` always holds the constant memory of the running TI-58C
  machine; it is saved after a change, as today.
- When a TI-58C machine is cold booted (a preset, a startup preset, Cold
  boot), the file follows: it then holds the memory of the machine just
  built.
- Quitting and starting the product counts as a power cycle for the
  TI-58C. With no startup preset set for the TI-58C, the machine is built
  with the constant memory from `ti58c.mem`. With a startup preset set,
  the preset cold boots the machine (ADR 0016) and the file follows. A
  user who wants the TI-58C to keep its memory sets no startup preset for
  it.
- The same holds when the model is switched to the TI-58C: the machine is
  built the same way as at the start (glossary, Startup preset).
- ADR 0021 is amended: no bundled startup preset restores `ti58c.mem`.
- The file is at the root of the save folder (`settings.md`, ST-3) and is
  not a medium (`media.md`, Q-2).

## Fit table

| | TI-59 (58, 58C) | PC-1500 (1500A) | PC-1600 | PC-1403 (paper) | PC-1350 (paper) |
|---|---|---|---|---|---|
| Media places | card reader (TI-59; pass-through, sides 1/2, waits for a medium) | module slot (battery-backed modules); cassette (CE-150) | 2 module slots; floppy (CE-1600F; holding, sides A/B); cassette (CE-1600P) | cassette (CE-126P, CE-124) | RAM card slot; cassette |
| Paper output (C-2) | PC-100C: dot rows | CE-150: pen strokes | CE-1600P, CE-150: pen strokes | CE-126P: dot rows | CE-126P: dot rows; CE-515P: pen strokes |
| Controls | PC-100C PRINT, ADV, TRACE | CE-161 write-protect switch | CE-161 write-protect switch | unknown | unknown |
| Typing support | key readiness | prompt readiness; key readiness to be identified (`typing.md`, Q-3) | key and prompt readiness, latches | unknown (`typing.md`, Q-1) | unknown |
| Serial port | — | CE-158 | RS-232C; CE-158 | — | 15-pin serial |
| Real-time clock | — | yes | yes | unknown | unknown |
| Product kinds | program memory, ASM overlay, heatmap, cue card, constant memory (TI-58C) | loader memory access, CE-158 printer port, INSPECTOR views | same, plus host drive | — | — |

## Open questions

- Q-1 (phase 5 walkthrough): The power state. Calc-U-1600 asks its
  machines `isPoweredOff` and `isDisplayOn`; Calc-U-59 has no off.
  Whether "off" is part of display output, a generic kind or a
  Calc-U-1600 product kind is open.
- Q-2 (phase 4b, with the media feature): Where the readers and writers
  of the media file formats sit (card text, `.card.yaml`, `.floppy.yaml`):
  the machine takes no files, so content crosses the media place in a
  form still to be named.
- Q-3 (phase 4b): The built-in default machine of the TI-58 and TI-58C
  (module, printer). ADR 0021 states it for the TI-59 only.

## Decision log

| Date | ID | Decision |
|---|---|---|
| 2026-11-10 | C-1 | Generic kinds: those ADR 0007 names (media, serial port, real-time clock, printer output), plus controls and typing support. Any other function starts as a product kind and becomes generic when a second product needs it. |
| 2026-11-10 | C-3 | `ti58c.mem` always mirrors the running TI-58C's constant memory and follows a cold boot. A product restart is a power cycle: without a TI-58C startup preset the memory comes back; with one, the preset cold boots it. ADR 0021 amended. |
