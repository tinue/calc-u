# Media files

Calc-U-59 keeps three kinds of file besides presets: the magnetic card
of the TI-59, the TI-58C's constant memory file, and the Solid State ROM
module. All three hold a cue card block, described at the end of this
page. How cards are used (arming, feeding, Name & Save) is in
[Magnetic cards](04-magnetic-cards.md) and the shared
[Media chapter](../../../library/help/04-media.md); the constant memory in
[Constant memory](09-constant-memory.md); the modules in
[Modules](09-modules.md). This page gives the files themselves.

| File | Suffix | `format:` | Version | Where it lies |
|---|---|---|---|---|
| Magnetic card | `.magnetcard.yaml` | `ti59-magnetic-card` | 3 | the save folder, or where you opened it from |
| Constant memory file | `ti58c.memory.yaml` | `ti58c-constant-memory` | 1 | the save folder, one per product |
| Solid State ROM module | `.rom.yaml` | `solid-state-rom-module` | 1 | with the ROMs; the 14 modules are part of the ROM set |

All of them follow the family's YAML rules
([Reference](../../../library/help/10-reference.md#syntax)). A damaged
file is refused as a whole and the product says so with a notice; a
file of an older format is not read.

## Magnetic card

A TI-59 magnetic card has two sides, 0 and 1. A side holds one bank, a
quarter of the TI-59's memory, numbered 1 to 4; both sides may hold the
same bank. A card is **fresh** while neither side has been written.

```yaml
# Calc-U-59 magnetic card
format: ti59-magnetic-card
format-version: 3
card-name: "My Program"
description: "Bank 1 and 2 of the plotter program"
saved: 2026-11-12T10:00:00Z
cue-card:
  title: My Program
  A: run
sides:
  0:
    partition: 0x1C
    data-type: program
    bank: 1
    protection: false
    checksum: 0x02
    encoding: register-hex
    bytes: |
      R000: 00 29 43 59 40 58 21 67
      # … 30 rows in all
```

| Key | Required | Value |
|---|---|---|
| `format` | yes | `ti59-magnetic-card` |
| `format-version` | yes | `3` |
| `card-name` | yes | The card's name, which the card dialogs list; no `"` and no line break |
| `description` | no | One line about the card |
| `template` | no | `true`: a template, which is never written |
| `saved` | no | The time of the last write, `YYYY-MM-DDTHH:MM:SSZ` (UTC); for your information |
| `cue-card` | no | The card's labels ([below](#the-cue-card-block)) |
| `sides` | yes | `0`, `1` or both; a side not written yet is left out, and an empty `sides` is a fresh card |

Each side:

| Key | Value |
|---|---|
| `partition` | the header's partition byte, as the ROM writes it |
| `data-type` | `program` or `data` |
| `bank` | 1–4; shown in that side's corner of the card |
| `protection` | `true` or `false`: the side was written protected (`−n Write`) |
| `checksum` | the header's checksum byte |
| `encoding` | `register-hex` |
| `bytes` | exactly 30 rows, the bank's registers; the row labels are informational |

- A card made in Calc-U-59 is saved as `<card-name>.magnetcard.yaml` in
  the save folder.
- A write rewrites the file whole. It replaces only the side that was
  written, keeps the name, the description, the template mark and the
  cue card, and sets `saved`. Comments in the file are not kept; the
  file starts with the comment line `# Calc-U-59 magnetic card`.
- A card saved without a cue card shows one titled with its name.
- The corners show the banks of the written sides; they follow from the
  sides and are not set by hand.
- The calculator checks what the real one checks: the partition when a
  card is read at idle, the bank number on `n INV Write`.

## Constant memory file

The TI-58C's memory that keeps its contents while the calculator is off,
with the cue card fitted at the time. Calc-U-59 writes it and reads it
back when a TI-58C starts without a startup preset; it cannot be opened
or dropped (it is refused with a notice).

```yaml
# Calc-U-59 TI-58C constant memory
format: ti58c-constant-memory
format-version: 1
saved: 2026-11-12T10:00:00Z
cue-card:
  title: Loan
  A: n
ram:
  encoding: register-hex
  bytes: |
    R000: 00 00 00 00 00 00 00 25
    R059: 00 00 00 00 00 00 10 00
```

| Key | Required | Value |
|---|---|---|
| `format` | yes | `ti58c-constant-memory` |
| `format-version` | yes | `1` |
| `memory-name` | no | A name |
| `description` | no | One line |
| `saved` | no | The time of the last write, `YYYY-MM-DDTHH:MM:SSZ` (UTC); for your information |
| `cue-card` | no | The stand-alone cue card fitted when the file was written; absent: none |
| `ram` | yes | `encoding: register-hex` and `bytes`: the 64 raw registers `R000`–`R063`; H00 to H03 are `R060` to `R063`. A register not listed is zero |

When the file is written, and when a preset leaves it alone, is in
[Constant memory](09-constant-memory.md).

## Solid State ROM module

One file per module holds its names, its ROM and the pre-printed cue
card of each library program. The 14 modules come with the ROM set; you
do not need to write one, but a preset can name one by `slot-1: { file: … }`.

```yaml
format: solid-state-rom-module
format-version: 1
module-name: ML
description: Master Library
title: MASTER LIBRARY MODULE -1-
menu-sort: 0
chip: TMC0541
rom:
  size: 5000
  encoding: addressed-hex
  bytes: |
    $0000: 25 00 00 54 02 43 11 41  14 15 15 82 17 01 19 51
    # …
cue-cards:
  - program: 1
    title: MASTER LIBRARY DIAGNOSTIC
    id: ML-01
    row-1: "DIAGNOSTIC: SBR ="
    row-1-align: center
```

| Key | Required | Value |
|---|---|---|
| `format` | yes | `solid-state-rom-module` |
| `format-version` | yes | `1` |
| `module-name` | yes | The two-letter ID (`ML`) that `slot-1: { name: … }` names |
| `description` | yes | The name the module picker shows ("Master Library") |
| `title` | no | The title printed on the module |
| `menu-sort` | no | Orders the modules in a list; default 0 |
| `chip` | no | The ROM chip |
| `notes` | no | Free text, not read |
| `rom` | yes | `size` (at most 5000 bytes), `encoding` (`addressed-hex`, `hex` or `base64`), `bytes` |
| `cue-cards` | no | One cue card block per library program, each with `program:` (1–99, as `Pgm` selects it) |

A library program without an item in `cue-cards` shows the stand-in card
titled with the module's `description`. A module file cannot be opened or
dropped on the window; it is refused with a notice.

## The cue card block

A cue card labels the user-defined keys A–E and A′–E′ above the top row
of keys. The same block stands in a preset's `cue-card:`, an in-file
card, a magnetic card file, the constant memory file and a module's
`cue-cards:`. The kind follows from where it stands: a stand-alone cue
card (preset, constant memory file), a magnetic card (TI-59), or a
module cue card, pre-printed in gold. More on what is shown when is in
[Cue cards](09-cue-cards.md).

| Key | Kinds | Value |
|---|---|---|
| `title` | all | The headline |
| `id` | module | The module and program ID right of the title, e.g. `ML-02` |
| `row-1` | all | Text in place of the A′–E′ label row |
| `row-2`, `row-2-r` | all | Text in place of the A–E label row; `row-2-r` stands right of `row-2` on the same line |
| `row-1-align` | all | `left`, `center` (default) or `right` |
| `row-2-align`, `row-2-r-align` | all | `left` (default), `center` or `right` |
| `style` | all | `none` (default) or `button`: a box around the row text |
| `pencil-color` | stand-alone, magnetic | `black` (default), `pencil` or `sharpie`, for the whole card |
| `A` … `E` | all | Labels of the A–E row |
| `A'` … `E'` | all | Labels of the A′–E′ row, written quoted: `"A'": text` |

- A key that doesn't apply to the kind is an error, and so is an unknown
  key.
- A label row shows only when its row text is empty.
- A label `\blank` is an empty cell that the label before it spans into;
  an empty label is an empty cell nothing spans into.
- A text is written in quotes when it contains `: ` or ` #`, or starts
  with `[`, `{`, `"` or `'`. Quotes have no escapes.

**Symbols.** The title, the id, the rows and every label turn these
tokens into the symbols real cards use (case matters):

| Group | Tokens |
|---|---|
| Greek | `\lambda` λ, `\Lambda` Λ, `\sigma` σ, `\Sigma` Σ, `\mu` μ, `\theta` θ, `\Theta` Θ, `\alpha` α, `\beta` β, `\epsilon` ε, `\Epsilon` Ε, `\Delta` Δ, `\pi` π, `\Pi` Π |
| Overlined | `\xbar` x̄, `\ybar` ȳ, `\zbar` z̄, `\nbar` n̄, `\mbar` m̄ |
| Arrows | `\leftrightarrows` ⇄, `\leftrightarrow` ↔, `\updownarrow` ↕, `\leftarrow` ←, `\to` → |
| Signs | `\sqrt` √, `\cbrt` ∛, `\fourthroot` ∜, `\triangle` △, `\angle` ∠, `\inf` ∞, `\sum` ∑, `\product` ∏, `\integral` ∫, `\approx` ≈, `\neq` ≠, `\leq` ≤, `\geq` ≥ |
| Layout | `\blank`: an empty cell (above) |
| Superscript | `^{-1}` ⁻¹, `^{-2}` ⁻², `^{-3}` ⁻³, `^{-}` ⁻, `^{+}` ⁺, `^{*}` ˣ, `^{n}` ⁿ, `^{T}` ᵀ, `^{x}` / `^{X}` ˣ, `^{y}` / `^{Y}` ʸ, `^{z}` / `^{Z}` ᶻ, `^{0}` … `^{9}` ⁰ … ⁹ |
| Subscript | `_{-1}` ₋₁, `_{-}` ₋, `_{+}` ₊, `_{0}` … `_{9}` ₀ … ₉, `_{i}` ᵢ, `_{j}` ⱼ, `_{a}` ₐ, `_{e}` ₑ, `_{o}` ₒ, `_{x}` / `_{X}` ₓ, `_{y}` / `_{Y}` ᵧ, `_{n}` ₙ, `_{m}` ₘ, `_{k}` ₖ |

## From 2.3.0

Calc-U-59 3.0 no longer reads, lists or changes 2.3.0's card files
(`.U59`, every layout) or its constant memory file `ti58c.mem`; they
stay where they are, and you convert them by hand:

| 2.3.0 | Now |
|---|---|
| a card file `Calc-U-59 Card 2.0`, ending with `END-CARD` | a magnetic card of format 3 (`.magnetcard.yaml`); `card-name` is the old file name without its extension; no end line |
| the card's `CUECARD:` block | `cue-card:` (its keys below) |
| `SIDE: 1` / `SIDE: 2` | `sides:` `0:` / `1:` |
| `HEADER:` keys `Partition`, `DataType`, `Bank`, `Protection`, `Checksum` | the side's `partition`, `data-type`, `bank`, `protection`, `checksum`, every one needed |
| `Partition: 18`, `Checksum: 03` (hex without a prefix) | `partition: 0x18`, `checksum: 0x03`: without `0x` the number is read as decimal |
| `Protection: yes` / `no` | `protection: true` / `false` |
| `DATA:` rows `R###: …` | `encoding: register-hex` and `bytes:`, the 30 rows copied unchanged, in the same order |
| card files 1.0 and the binary layouts | not converted |
| `ti58c.mem` (cue card, `R###:` rows) | `ti58c.memory.yaml`, format 1, the rows copied unchanged |
| `CUECARD:` keys `Title`, `ID`, `Style`, `Row1`, `Row2`, `Row2R`, `Row1Align`, `Row2Align`, `Row2RAlign`, `PencilColor` | `title`, `id`, `style`, `row-1`, `row-2`, `row-2-r`, `row-1-align`, `row-2-align`, `row-2-r-align`, `pencil-color` |
| `CUECARD:` keys `Template`, `Banks` | dropped: the kind follows from where the block stands, the corners from the sides |
| `CUECARD:` unknown keys and `IDAlign` were ignored | an error |

For example, the first side of a card 2.3.0 wrote (rows R002 to R029 left
out):

```text
SIDE: 1
HEADER:
Partition: 18
DataType: program
Bank: 1
Protection: no
Checksum: 03

DATA:
R000: 30 19 75 49 90 90 90 39
R001: 23 49 80 90 90 39 69 22
```

becomes, in `sides:` of the new card:

```yaml
  0:
    partition: 0x18
    data-type: program
    bank: 1
    protection: false
    checksum: 0x03
    encoding: register-hex
    bytes: |
      R000: 30 19 75 49 90 90 90 39
      R001: 23 49 80 90 90 39 69 22
```

The full rules of every format are in the source repository:
[media.md](https://github.com/tinue/calc-u/blob/main/docs/formats/media.md),
[modules.md](https://github.com/tinue/calc-u/blob/main/docs/formats/modules.md),
[cue-card.md](https://github.com/tinue/calc-u/blob/main/docs/formats/cue-card.md).
