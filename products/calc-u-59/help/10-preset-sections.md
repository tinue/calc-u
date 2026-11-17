# Preset sections of Calc-U-59

The grammar of the preset format (format version 2), its comments, values
and references, and the keys and steps every product has are in the
family's [Reference](../../../library/help/10-reference.md#the-preset-format).
This page lists what a preset of Calc-U-59 holds: its header keys, its
sections, its steps and the key IDs of the TI-58, TI-58C and TI-59. How
to use them, with examples, is in [Presets](06-presets.md); a preset
built up step by step is the [Preset tutorial](06-preset-tutorial.md).

A preset is a text file with the extension `.ti59`, `.ti58` or `.ti58c`.
The extension doesn't choose the model; `model:` does.

```yaml
format-version: 2
model: TI-59
preset-name: Pgm 01
description: Master Library diagnostic, run at regular speed
slot-1: { name: ML }
printer: PC-100C
keys:
  - regular-speed:
  - key: Pgm 0 1 SBR =
```

Every preset builds its machine anew: the model, the module and the
printer the header names, and nothing else, then a cold boot, which
clears all memory, the TI-58C's constant memory included. Then the
sections run in file order. Without a `cue-card:` section, no cue card
is fitted.

## Header keys

| Key | Value |
|---|---|
| `format-version:` | `2` (required) |
| `model:` | `TI-59`, `TI-58` or `TI-58C`, any case (required) |
| `preset-name:` | the name lists and menus show; default: the file name without its extension |
| `description:` | one line for lists, menus and tooltips |
| `timeout:` | the longest wait for the calculator to become idle between two items, in seconds of emulated time; default `3600` |
| `printer:` | `PC-100C`; without it, no printer is attached |
| `slot-1:` | a Solid State ROM module: `{ name: ML }` by its two-letter ID ([Modules](09-modules.md)), or `{ file: … }` (a `.rom.yaml` file); without it, the slot is empty |
| `magnetic-cards:` | TI-59 only: magnetic cards defined inside the preset ([below](#in-file-magnetic-cards)) |

Paths are relative to the preset; `~/` is your home folder.

## Sections

| Section | Value |
|---|---|
| `program:` | a keystroke program: `keycodes: \|` with the program in the preset, or `file:` a [keystroke program file](10-keystroke-programs.md) |
| `data:` | data registers, `NN: value` |
| `partition:` | the partition, as the calculator shows it after `2nd Op 17` |
| `cue-card:` | a stand-alone cue card to fit above the keys |
| `keys:` | a list of steps |

Sections may appear any number of times, and are carried out in file
order. A `program:`, `data:`, `partition:` or `cue-card:` after a `keys:`
section changes the calculator in the middle of the script, as if you
had keyed the change in: only what it lists changes.

### program

```yaml
program:
  keycodes: |
    76 11 42 29 92 76 12 42 30 92   # compact: steps 000-009
    010 43 RCL                      # listing: step number, keycode, mnemonic
    011 00 00
    110 42 STO                      # the step number places the keycode
```

- **Compact** lines are keycodes, each exactly two digits; a line goes
  on at the step after the one written last (000 at the start). Steps of
  `00` are written out.
- **Listing** lines are as the PC-100C lists a program: a step number of
  three or more digits, the keycode, and the mnemonic, which is not read.
  A gap needs no marker.
- Only the listed steps are written; every other step keeps its contents
  (00 after the cold boot). Each `program:` section starts at step 000.
- The highest step is 959 on the TI-59 and 479 on the TI-58 and TI-58C.
- Keycodes are not key IDs: the digit 1 is keycode `01`, and `Pgm` is the
  one step `36`. Key IDs are for `key:` steps ([below](#key-ids)).

### data

```yaml
data:
  00: 3.141592653589793
  05: -1.5e-3
  H01: 7.77E22           # TI-58C only
```

- `NN` is the register number of `STO` and `RCL`, 00 to 99; 60 to 99
  exist on the TI-59 only.
- `H00` to `H03` are the TI-58C's four extra cells beyond its 60
  registers, reached only by the ROM's memory routines; on any other
  model they are an error.
- The value is a decimal number; an exponent is allowed. Integers are
  stored digit for digit, other values rounded to 13 significant digits.
- Only the listed registers are written.

### partition

```yaml
partition: 719.29
```

The number before the dot is the last program step, so `719.29` gives
720 steps; the part after the dot is a note and is ignored. Steps are
rounded up to a multiple of 80, at most 960 on the TI-59 and 480 on the
TI-58 and TI-58C. Until a `partition:` the partition is the model's after
a cold boot: 479.59 on the TI-59, 239.29 on the TI-58 and TI-58C.

### cue-card

```yaml
cue-card:
  title: Roots & Powers
  A: \sqrt x
  B: x^{2}
  "A'": \sum x_{i}
```

A stand-alone cue card, fitted above the top row of keys; its keys are
those of [the cue card block](10-media-files.md#the-cue-card-block). It
does not go onto a magnetic card: a magnetic card has its own cue card.

### In-file magnetic cards

`magnetic-cards:` (TI-59) lists cards defined by the preset, which a
`card:` or `arm:` step uses with `mem: <card-name>`:

```yaml
magnetic-cards:
  - card-name: MyVars
    cue-card:                 # optional
      title: My Vars
    sides:
      0:
        bank: 3               # required, 1-4
        protected: false      # optional; true = written as -n Write
        partition: 479.59     # optional; default: the calculator's when it goes in
        data:                 # registers of bank 3: R30-R59
          30: 1.5
        program: |            # keycodes; bare keycodes start at the bank's first step
          76 11 42 00
```

- Each item has `card-name`, optional `description` and `cue-card`, and
  `sides` with `0`, `1` or both; a side not given is fresh.
- A side replaces its whole bank: what it doesn't list reads as 00.
  Bank 1 holds steps 000–239 and R90–R99, bank 2 steps 240–479 and
  R60–R89, bank 3 steps 480–719 and R30–R59, bank 4 steps 720–959 and
  R00–R29. Contents outside the bank are an error.
- A side written by the calculator stays with the card until the next
  load or model switch; a Reset, a power cycle or a Cold boot keep it.
- `mem:` with a name the preset doesn't define is a fresh card without a
  cue card.
- Such a card is saved to a file only by `saveas:`.

## Steps

The family's steps (`key:`, `wait:`, `skip-wait:`, `let-it-finish:`,
`full-speed:`, `regular-speed:`, `expect:`, `not-expect:`, `screenshot:`,
`trace:`, `elapsed-time:`, `cut-paper:`, `saveas:`) and these:

| Step | Value |
|---|---|
| `card:` | TI-59: push one side of a magnetic card through the reader: `{ file: … }`, `{ name: … }` or `{ mem: … }`, with `side: 0\|1` (default 0). The calculator reads it, or writes it when it waits for a card; the card is fitted after it has passed |
| `arm:` | TI-59: make a magnetic card ready for you to feed by tapping a corner; `{ file: … }`, `{ name: … }` or `{ mem: … }`, without a side |
| `saveas:` | TI-59: `{ card: <card-name>, as: live\|template, name: … }` (or `file:` ending in `.magnetcard.yaml`): save an in-file card with its sides and cue card |
| `expect:`, `not-expect:` | a number, compared with the X register in its full 13 digits, whatever the display format: after `π`, `3.14159265359` passes, `3.141592654` fails |
| `printer-trace:` | `on` or `off`: the PC-100C's TRACE switch; needs `printer: PC-100C`. Without the step, TRACE is off |
| `pause:` | `program` or `cpu`: pause the calculator at that level |
| `pause-on-start:` | `program` (before the first program step), `cpu` (when the ROM leaves its key scan), or `off` |
| `step:` | `{ level: program\|cpu, count: n }` while paused (count default 1, at most 100000) |
| `resume:` | let a paused calculator run on |
| `debug-panel:` | show a tab of the debug panel: `calculator`, `cpu` or `log` ([Debugger](07-debugger.md)); a frontend without the panel ignores it |

- A card named by `file:` or `name:` that does not exist refuses the
  preset; a new card is an in-file card, saved with `saveas:`.
- A machine waiting for a card (after `Write`) counts as idle, so
  `- key: 1 2nd Write` finishes when the calculator waits, and a
  `card:` after it feeds the card.
- `type:` does not exist on these models: keys are pressed with `key:`.
- While the calculator is paused, a step that needs it to run (`key:`,
  `card:`, a `program:` section) waits until it runs again; debugger and
  frontend steps go on.
- A key is held 100 ms of emulated time and until the ROM has read it,
  at every speed, so a held **Adv** feeds the same lines at full and at
  regular speed.

## Key IDs

A `key:` step presses keys by their IDs, separated by spaces, in any
case. A key's ID is the label on the key when that is one plain word,
otherwise the name the PC-100C prints for it; each name with characters
outside ASCII has an ASCII spelling too. On these models a two-digit
number such as `12` is an error; `1 2` is two digit keys.

**Keys**, with their position (row × 10 + column):

| Position | ID | Also | | Position | ID | Also |
|---|---|---|---|---|---|---|
| 11–15 | `A` `B` `C` `D` `E` | | | 61 | `GTO` | |
| 21 | `2nd` | `2ND` | | 62–64 | `7` `8` `9` | |
| 22 | `INV` | | | 65 | `×` | `x`, `*` |
| 23 | `lnx` | `LNX` | | 71 | `SBR` | |
| 24 | `CE` | | | 72–74 | `4` `5` `6` | |
| 25 | `CLR` | | | 75 | `-` | |
| 31 | `LRN` | | | 81 | `RST` | |
| 32 | `x⇄t` | `X⇄T`, `x<>t` | | 82–84 | `1` `2` `3` | |
| 33 | `x²` | `X²`, `x^2` | | 85 | `+` | |
| 34 | `√x` | `√X`, `sqrt` | | 91 | `R/S` | |
| 35 | `1/x` | `1/X` | | 92 | `0` | |
| 41–44 | `SST` `STO` `RCL` `SUM` | | | 93 | `.` | |
| 45 | `yˣ` | `Yˣ`, `y^x` | | 94 | `+/-` | |
| 51–54 | `BST` `EE` `(` `)` | | | 95 | `=` | |
| 55 | `÷` | `/` | | | | |

**Second functions.** The ID presses 2nd, then the key. `2nd <ID>` means
the same, so `Pgm`, `2nd Pgm` and `2nd LRN` all press 2nd, then LRN.

| Key | ID | Also | | Key | ID | Also |
|---|---|---|---|---|---|---|
| A–E | `A'` `B'` `C'` `D'` `E'` | `A′` … `E′` | | GTO | `Pause` | `PAU` |
| lnx | `log` | `LOG` | | 7 | `x=t` | `EQ` |
| CE | `CP` | | | 8 | `Nop` | `NOP` |
| CLR | — (write `2nd CLR`) | | | 9 | `Op` | `OP` |
| LRN | `Pgm` | `PGM` | | × | `Deg` | `DEG` |
| x⇄t | `P/R` | `P→R` | | SBR | `Lbl` | `LBL` |
| x² | `sin` | `SIN` | | 4 | `x≥t` | `GE`, `x>=t` |
| √x | `cos` | `COS` | | 5 | `Σ+` | `∑+`, `sum+` |
| 1/x | `tan` | `TAN` | | 6 | `x̄` | `xbar` |
| SST | `Ins` | `INS` | | - | `Rad` | `RAD` |
| STO | `CMs` | `CMS` | | RST | `STF` | |
| RCL | `Exc` | `EXC` | | 1 | `IFF` | |
| SUM | `Prd` | `PRD` | | 2 | `D.MS` | `DMS` |
| yˣ | `Ind` | `IND` | | 3 | `π` | `pi` |
| BST | `Del` | `DEL` | | + | `Grad` | `GRD` |
| EE | `Eng` | `ENG` | | R/S | `Write` | `WRT` |
| ( | `Fix` | `FIX` | | 0 | `Dsz` | `DSZ` |
| ) | `Int` | `INT` | | . | `Adv` | `ADV` |
| ÷ | `\|x\|` | `IxI` | | +/- | `Prt` | `PRT` |
| | | | | = | `List` | `LST` |

- `St flg` and `If flg` contain a blank, so their IDs are the printer's
  names, `STF` and `IFF`.
- The bare ID `CLR` is the key; its second function is written `2nd CLR`.

**Printer names.** Every name the PC-100C prints for a keycode is
accepted too. Most are the IDs above; the others:

| Keycode | Printer name | Presses |
|---|---|---|
| 20 | `CLR` | `CLR` (the key; its second function is `2nd CLR`) |
| 26, 27 | `2ND`, `INV` | `2nd`, `INV` |
| 62 | `PG*` | `Pgm Ind` |
| 63 | `EX*` | `Exc Ind` |
| 64 | `PD*` | `Prd Ind` |
| 72 | `ST*` | `STO Ind` |
| 73 | `RC*` | `RCL Ind` |
| 74 | `SM*` | `SUM Ind` |
| 83 | `GO*` | `GTO Ind` |
| 84 | `OP*` | `Op Ind` |
| 92 | `RTN` | `INV SBR` |
| 82 | `HIR` | — (no key sequence enters it; an error) |

## Which keyword works on which model

| Keyword | TI-59 | TI-58 | TI-58C |
|---|---|---|---|
| `printer`, `slot-1`, `program`, `data` (00–59), `partition`, `cue-card`, `keys` and the family's steps, `printer-trace`, `debug-panel`, the debugger steps | ✓ | ✓ | ✓ |
| `data` 60–99 | ✓ | | |
| `data` H00–H03 | | | ✓ |
| `magnetic-cards`, `card`, `arm`, `saveas: { card: … }` | ✓ | | |

A keyword that a model does not have refuses the preset when it is read.

## From a 2.3.0 file

A preset written for Calc-U-59 2.3.0 (`MODEL:`, `PROGRAM:`,
`KEYSTROKES:` …) is refused with a hint to upgrade it. By hand:

| 2.3.0 | Version 2 |
|---|---|
| `MODEL: TI-59` (optional) | `format-version: 2` and `model: TI-59` (required) |
| `SOLID-STATE-MODULE: ML` (absent: unchanged) | `slot-1: { name: ML }` (absent: none) |
| `PRINTER: on` (absent: unchanged) | `printer: PC-100C` (absent: none) |
| `PARTITION:`, `PROGRAM:`, `CUECARD:` | `partition:`, `program: { keycodes: … }`, `cue-card:` |
| `REGISTERS:` lines `NN = value` | `data:` entries `NN: value` |
| sections after `KEYSTROKES:` (a reload) | sections in file order |
| `CARD: mem:X` … `END-CARD` | an item of `magnetic-cards:` |
| `KEYSTROKES:` with matrix codes such as `21 31` | `keys:` with `- key: Pgm` (the key IDs above) |
| `Wait: 2s` | `- wait: 2s` |
| `FullSpeed:` / `RegularSpeed:` (default regular) | `- full-speed:` / `- regular-speed:` (default full); `- regular-speed:` first keeps the old speed |
| the last key was waited for until the calculator was idle | the last item is not waited for; `- let-it-finish:` last runs it to its end |
| `Trace: x` / `Trace: Off` | `- trace: x` / `- trace: off` |
| `ZeroElapseTime:` / `ReportElapseTime:` | `- elapsed-time: start` / `report` |
| `InsertCard: X:2` | `- card: { file: X.magnetcard.yaml, side: 1 }` (sides are 0 and 1 now; 2.3.0's 1 and 2) |
| `InsertCard:` / `ArmCard:` on a card that did not exist yet | an in-file card with its `cue-card`, fed with `mem:`, then `- saveas: { card: X, as: live, name: X }` |
| `ArmCard: mem:X` (fed when the program waits) | `- card: { mem: X }` after the step that makes the calculator wait; or `- arm: { mem: X }` for you to feed |
| `PrinterTrace: on`, `CutPaper:`, `DebugPanel: CPU` | `- printer-trace: on`, `- cut-paper:`, `- debug-panel: cpu` |
| `Freeze: Calculator`, `FreezeOnStart: CPU`, `Step: CPU 5`, `Resume:` | `- pause: program`, `- pause-on-start: cpu`, `- step: { level: cpu, count: 5 }`, `- resume:` |
| `CUECARD:` keys `Title`, `Row1`, `Row1Align`, `PencilColor`, … | `title`, `row-1`, `row-1-align`, `pencil-color`, … ([the cue card block](10-media-files.md#the-cue-card-block)); `Template` and `Banks` are dropped |
| a bad line was skipped | the file is refused, with its first error and its line |
| registers 60–99 on a TI-58 or TI-58C were ignored | an error |
