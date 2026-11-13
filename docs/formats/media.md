# Media and the constant memory file

A **medium** is a removable item that holds user data and can be named
and saved. Each kind of medium has a format of its own. The
battery-backed module, the fourth kind of medium, is a module definition
([modules.md §1](modules.md#1-module-definition)).

| Medium | Format | Place | Products |
|---|---|---|---|
| Magnetic card | §1, version 3 | the TI-59's card reader (pass-through) | [59: TI-59] |
| Floppy disk | §2, version 2 | the CE-1600F drive of the CE-1600P (holding) | [1600: PC-1600] |
| Cassette tape | §3, version 1 | the cassette interface of a CE-150 or CE-1600P | [1600] |

§4 specifies the TI-58C's constant memory file, which is not a medium.

Common rules: [README.md](README.md).

## Templates and instances

What happens to a medium's file is decided by the file, not by where it
is stored:

- **`template: true`:** a **template**. The product never writes the
  file. Every bundled medium is one; a user may make their own (a user
  template) by putting such a file into the save folder.
- **No `template` key (or `false`):** an **instance**, saved in place
  after each change (autosave).
- **Name & Save** gives a template a name and saves a copy as an
  instance, without the `template` key. A preset's `saveas:` can also
  save a copy as a template ([preset.md §5.2.1](preset.md#521-saveas)).
- The magnetic card has one template, the **fresh** card (§1). It is
  always there and has no file.
- A medium is addressed by its name tag or by its file
  ([README.md §3](README.md#3-addressing-a-resource)).

---

## 1. Magnetic card

The TI-59's removable magnetic strip, with two sides, 0 and 1. A side
holds one bank, a quarter of the TI-59's memory, numbered 1 to 4. Both
sides can hold the same bank number. A card is **fresh** while neither
side has been written.

### 1.1 Keys

```yaml
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
  1:
    # …
```

| Key | Required | Value |
|---|---|---|
| `format` | yes | `ti59-magnetic-card` |
| `format-version` | yes | `3` |
| `card-name` | yes | The name tag. Must not contain `"` or a newline |
| `description` | no | One line about the card |
| `template` | no | `true`: a template |
| `saved` | no | ISO-8601 UTC time of the last write, `YYYY-MM-DDTHH:MM:SSZ`. Informational |
| `cue-card` | no | The card's labels ([cue-card.md](cue-card.md)) |
| `sides` | yes | A mapping with `0`, `1` or both. A side not written yet is left out; an empty `sides` is a fresh card |

Each side:

| Key | Required | Value |
|---|---|---|
| `partition` | yes | The header's partition byte, as the ROM writes it |
| `data-type` | yes | `program` or `data` |
| `bank` | yes | 1–4; shown in that side's corner of the card |
| `protection` | yes | `true` or `false`: the side was written protected (`−n Write`) |
| `checksum` | yes | The header's checksum byte |
| `encoding` | yes | `register-hex` |
| `bytes` | yes | Exactly 30 rows, the bank's registers. The row labels are the bank's register numbers, (bank − 1) × 30 … + 29, and are informational. Byte order: [README.md §1.1](README.md#11-byte-blocks) |

- A damaged file is refused as a whole. A side is never read with parts
  blanked: every side key and all 30 rows are required.
- The ROM checks what it checks on real hardware: the partition on a
  read at idle, the bank number on `n INV Write`.

### 1.2 Card files and in-file cards

A card file holds a side as the ROM writes it: header bytes and raw
registers. A preset's **in-file card** ([preset.md §3.1](preset.md#31-magnetic-cards-59-ti-59))
describes a side by its contents instead (bank, program, data), and is
built into the same bytes when it goes in. A preset saves an in-file
card, with its cue card, by `saveas:` ([preset.md §5.2.1](preset.md#521-saveas));
the user does so with Name & Save. A new card starts as a fresh in-file
card, from the implicit template.

### 1.3 Using a card

- **Arm** (`arm:` in a preset, or the frontend): the card is made ready
  for the user to feed. It is fitted, not read, and the machine doesn't
  detect it. The user feeds it by tapping a corner.
- **Insert** (`card:` in a preset, or the user feeding an armed card):
  one side passes through the reader, which reads it or writes it. After
  it has passed, the card is fitted ([cue-card.md §2](cue-card.md#2-fitting)).
- A reference to a card file that doesn't exist is an error; a new card
  starts fresh and is saved by Name & Save.

### 1.4 Written cards

- A side the machine writes goes into the card's file (an instance), or
  stays in memory for an in-file card.
- A template card is never written; the product makes an instance first
  or refuses the write.
- A card file's cue card is its own `cue-card`; a saved in-file card
  takes the one it was defined with. A card saved without a cue card
  gets one whose title is its card name.
- The corners of a card's cue card show the banks of its written sides;
  they follow from the sides and are not set by hand.

---

## 2. Floppy disk

A disk for the PC-1600's CE-1600F drive, with two sides, 0 and 1, used
one at a time. The file holds the raw bytes of both sides and nothing
else. It knows nothing about files: the filesystem inside a side is
Sharp's, which the CE-1600P ROM reads as on real hardware.

### 2.1 Keys

```yaml
# Calc-U-1600 CE-1600F floppy disk image
format: ce1600f-floppy
format-version: 2
disk-name: "Formatted"
description: "Formatted, both sides empty"
template: true
saved: 2026-09-19T10:17:46Z
sides:
  0:
    encoding: addressed-hex
    bytes: |
      $0000: 00...
      $0200: F2 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
      $0210: 00...
      $0400: F2 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
      $0410: 00...
  1:
    encoding: addressed-hex
    bytes: |
      $0000: 00...
```

| Key | Required | Value |
|---|---|---|
| `format` | yes | `ce1600f-floppy` |
| `format-version` | yes | `2` (version 1, Calc-U-1600 0.8.x, named the sides `a` and `b`) |
| `disk-name` | yes | The name tag; non-empty. Writers double-quote it, so it must not contain `"` or a newline |
| `description` | no | One line, the picker's tooltip. Written double-quoted, or single-quoted when it holds a `"`. Name & Save and autosave keep it |
| `template` | no | `true`: a template. Written only by a preset's `saveas:` as a template; Name & Save never writes it |
| `saved` | no | ISO-8601 UTC time of the last write. Informational |
| `sides` | yes | Exactly the keys `0` and `1` |
| `sides.0`, `sides.1` | yes | Exactly `encoding` and `bytes` |
| `…encoding` | yes | `addressed-hex` |
| `…bytes` | yes | Covers exactly `0x10000` bytes |

### 2.2 Sides and byte order

- Each side is 65536 bytes: 16 tracks × 8 sectors × 512 bytes. The drive
  is single-sided and the disk is turned by hand, so each side is its own
  volume.
- Byte offset within a side = logical sector × 512, where logical sector
  = track × 8 + sector, sectors numbered from 0 (the CE-1600P ROM's
  numbering). The boot sector is at `$0000`, the FAT at `$0200`, its copy
  at `$0400`, the directory at `$0600`–`$0BFF`, the first data cluster at
  `$0C00`.
- Sector ID fields, gaps and other on-track formatting are not stored.

### 2.3 Writing

- Writers emit, in this order: the comment line
  `# Calc-U-1600 CE-1600F floppy disk image`, `format`, `format-version`,
  `disk-name`, `description` (when set), `template` (when true), `saved`,
  `sides`. Indent 2 spaces per level; `addressed-hex` in its canonical
  form ([README.md §1.1](README.md#11-byte-blocks)).
- `saved` is set on every write; the file is always rewritten whole.
- A product may keep an inserted disk in memory and rewrite the file
  after the machine writes to it. A change another tool makes to the file
  while the disk is inserted can then be lost; the disk is ejected before
  its file is edited.

---

## 3. Cassette tape

A tape for the cassette interface of a CE-150 or CE-1600P. The tape
itself is sound, in a `.wav` file. The **tape definition** gives it a
name and a description, like any other medium, and refers to the sound
file, as a preset refers to a `.bas` or `.bin` file.

### 3.1 Keys

```yaml
format: cassette-tape
format-version: 1
tape-name: "Games"
description: "Three games saved with CSAVE"
audio: { file: games.wav }
position: 0
```

| Key | Required | Value |
|---|---|---|
| `format` | yes | `cassette-tape` |
| `format-version` | yes | `1` |
| `tape-name` | yes | The name tag. Must not contain `"` or a newline |
| `description` | no | One line about the tape |
| `template` | no | `true`: a template. Its sound file is never written either |
| `audio` | yes | The sound file, by `file:` only, relative to the tape definition |
| `position` | no | Reserved for future use (where the tape stands, in seconds from the start of the sound). Must be `0`, the default; any other value is an error |

- The sound file is a WAV file of the tape's sound. Its sample format is
  the product's.
- Recording onto an instance writes into its sound file. Name & Save
  copies the definition and the sound file, and writes them side by
  side. A definition written by hand around an existing WAV may name it
  by any relative path.
- Several recordings on one tape are part of the sound, not of this
  format. Which recorder connection a tape is used on belongs to the
  machine, not to the tape.
- A `.wav` file on its own is a program file, not a tape: a preset's
  `program:` loads it into the program area. A tape definition is a
  medium, inserted as a cassette.

---

## 4. Constant memory file

The TI-58C's RAM that keeps its contents while the calculator is off,
together with the cue card fitted at the time. The product writes it and
reads it back when a TI-58C starts. It can't be inserted or ejected, and
there is one per product. Where it is kept is the product's.

A preset doesn't write the file. Its cold boot zeroes the machine's
constant memory, not the file, before the processor starts; the machine
then holds what the ROM's start-up leaves there and what the preset
puts there. When a TI-58C starts from the file, its RAM is loaded from
it before the processor is powered. The file takes the machine's state when it would
anyway: the calculator switched off, the product quit.

### 4.1 Keys

```yaml
format: ti58c-constant-memory
format-version: 1
cue-card:
  title: Loan
  A: n
ram:
  encoding: register-hex
  bytes: |
    R000: 00 00 00 00 00 00 00 25
    R118: 00 00 00 00 00 00 10 00
```

| Key | Required | Value |
|---|---|---|
| `format` | yes | `ti58c-constant-memory` |
| `format-version` | yes | `1` |
| `memory-name` | no | A name tag, for a product that lists constant memory files |
| `description` | no | One line |
| `cue-card` | no | The cue card fitted when the file was written ([cue-card.md](cue-card.md), a stand-alone cue card). Absent: none fitted |
| `ram` | yes | `encoding: register-hex` and `bytes`: the RAM as 120 raw registers, `R000`–`R119`. A register that is not listed is zero; writers leave out registers that are zero. Byte order: [README.md §1.1](README.md#11-byte-blocks) |
