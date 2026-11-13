# Modules

A **module** is a peripheral that holds memory (ROM or RAM) and plugs
into a module slot. Its two kinds have a format each:

| Kind | Format | Version | Products |
|---|---|---|---|
| Memory module | §1, module definition `.card.yaml` | 1 | [1600] |
| Solid State ROM module | §2, `.rom.yaml` | 1 | [59] |

A preset puts a module into a slot with `slot-1:` / `slot-2:`, by
`file:` or by `name:`; the name is the file's `module-name` in both
formats ([preset.md §3](preset.md#3-header-keys)). Common rules:
[README.md](README.md).

---

## 1. Module definition

The file that describes one memory module: its memory, how it is
addressed and the hosts it fits. §1.1 is its content model, §1.2
onwards its keys.

A module definition is hand-written and reviewed, so it is YAML with
comments. Integers are decimal (`8192`) or hex with `0x` (`0x2000`).
Unknown keys are an error.

### 1.1 Content model

**The loader connects pins.** Every signal a definition names is one of
the 40 contacts of the 40-pin connector. The loader works in contacts
only. Signal names are a convenience for the author: a file declares one
**terminology**, the pin-name vocabulary of one host, and every name in
it is looked up in that table once, when the file is read. After that
the terminology has no further meaning.

**Compatibility is the author's list.** A file lists the hosts it fits.
The loader makes exactly one check: is the host on the list? It doesn't
check the list against pin tables. A contact the file uses that is not
connected on one of its hosts is not an error: that part of the module
simply never responds there (a **dormant** sub-region), as a real module
would behave. A region that decodes a raw address window (memory-range
match) has no pin to anchor this judgement to; getting its host list
right is up to the author's knowledge of each host's address map.

**A module is a list of memory regions.** Each region is described by
four independent dimensions; none can be inferred from another:

| # | Dimension | Values |
|---|---|---|
| D1 | Addressing | Enable groups OR'd together, each a set of terms AND'd: chip-select lines, an address-line qualifier (a few address lines at fixed levels, standing for on-module decode logic, not a bus-window comparison), or a direct memory-range match. Specified once per region; a banked region nests a second one as its bank window |
| D2 | Content | **Regular** (read/write by the CPU; optionally wired read-only, optionally with a write-protect switch), **ROM** (bytes in the file, never written), **Flash** (optional initial bytes in the file, rewritten only through the chip's command protocol). Once per region, or once per bank range in a banked region |
| D3 | Banking | Unbanked, or banked: several copies of the content behind one addressing, one of them selected |
| D4 | Bank latch (banked only) | **Trigger-based**: a strobe (a pin, or an `OUT` to an I/O port) at which the module samples lines and latches the bank number. **Line-based**: lines read live at each access, no latch |

- There is no "vertical / horizontal" field. Trigger-based is what
  "vertical" denotes, line-based what "horizontal" denotes. Note the
  trap: Sharp's "vertical bank register" (port 28H on the CE-1601M) is
  decoded on the module and latched there, so it is trigger-based.
- **Flash.** A plain CPU write does nothing to the array. The module's
  command decoder must first see the chip's unlock sequence. Then a
  byte-program can only clear bits (`array[addr] &= data`); an erase
  (sector or chip) sets bytes to `0xFF`. A write that doesn't match the
  next expected step returns the decoder to idle; a reset command does so
  on purpose. **The command decoder is independent of the bank latch:** a
  bank-select write never disturbs it, because real firmware interleaves
  the two. Not modelled: the status toggle bits (DQ6/DQ7) and
  autoselect; program and erase take no time.
- **Bytes are in the file.** ROM must carry all its bytes; Flash and
  Regular ranges may carry initial bytes. They never come from an
  external file, except a ROM under development, which comes in through
  the preset (`slot-N-rom:`).
- **Loading a module resets it** to its file: a Regular range to its
  power-up fill or its initial bytes, Flash to its initial bytes or
  erased, ROM to its bytes. No file can opt out.
- **Power-up state.** Regular RAM powers up as `0x00` by default (CMOS
  RAM that lost its supply), Flash as `0xFF`. A file may set it per range.
- **Write-protect switch.** Its position is part of the file. A load
  starts from it; saving with the switch on writes `protected`.
- **PC-1600 module header.** The PC-1600 ROM recognises a ROM or RAM-disk
  module by an 8-byte header at the start of the module's address space
  (ID bytes `43H 16H`, …, a type byte), followed by a jump table. These
  bytes are **content**, in the file's byte blocks at their fixed offset,
  not fields. A region still declares its **module class**, as an
  authoring cross-check against the header bytes. The PC-1500/1500A have
  no such header.

### 1.2 Top-level keys

| Key | Required | Value |
|---|---|---|
| `format-version` | yes | `1` |
| `module-name` | yes | The name tag (`CE-155`, `CE-1600M`), shown in the module picker; what `slot-N: { name: … }` resolves against. Unique within a folder of modules |
| `description` | no | One line (`"8 KB static RAM"`), the picker's tooltip. A saved instance keeps its template's |
| `compatible-hosts` | yes | A non-empty list from `PC-1500`, `PC-1500A`, `PC-1600-Slot-1`, `PC-1600-Slot-2`. The loader's only compatibility check |
| `definition-terminology` | yes | One of the same four. The vocabulary every `chip-select` / `signal` / `line` name in the file is resolved against. Must be in `compatible-hosts` |
| `regions` | yes | A list of one or more regions (§1.3) |
| `battery` | no | `true` for battery-backed hardware. A battery-backed template offers Name & Save, which makes it a medium |
| `template` | no | `true`: a template, never written. Absent or `false`: an instance, autosaved in place. Never written into a saved instance |
| `notes` | no | Free text, ignored by the loader |

The versioning rules are those of [README.md §2](README.md#2-version-name-and-description).

**Terminology → signal names** (besides `A0`… address and `D0`… data
lines):

| Terminology | Signal names |
|---|---|
| `PC-1500` | `Y0`, `Y2`, `S1`, `S2`, `S3`, `S4`, `PU`, `PV` |
| `PC-1500A` | `Y0`, `Y2`, `S3`, `S4`, `S5`, `PU`, `PV` |
| `PC-1600-Slot-1` | `RAM2`, `PVOUT`, `S1`, `S2`, `S3`, `PU`, `PT` |
| `PC-1600-Slot-2` | `RAM1`, `PVOUT`, `K0`, `K1`, `K2`, `PU`, `PT` |

Each name resolves to one contact: `Y0`/`RAM2`/`RAM1` → 4, `S4`/`PVOUT`
→ 5, `S1`/`S3`/`K0` → 16, `S2`/`S4`/`K1` → 17, `S3`/`S5`/`K2` → 18,
`Y2`/`PT` → 19; `PU` → 2 and `PV` → 3 on the PC-1500/1500A (as measured
on a real unit; the Technical Reference Manual prints them swapped),
`PU` → 3 on the PC-1600 slots.

### 1.3 Region

| Key | Required | Value |
|---|---|---|
| `name` | yes | Unique within the file |
| `addressing` | yes | Addressing (§1.4), D1 |
| `content` | yes | Content (§1.5), D2 |
| `banking` | yes | `none`, or a banking map (§1.6), D3/D4 |
| `capacity` | unbanked only | Size in bytes of the region's one copy. Required with `banking: none`; left out when banked (it is `bank-count × bank-size`) |
| `initial-content` | ROM: yes, covering every byte; otherwise optional | Bytes carried in the file (§1.7) |
| `pc1600-module-class` | PC-1600 regions only | `program`, `system`, `ram-disk` or `plain-ram-no-header`: the cross-check against the header bytes. Absent: `plain-ram-no-header`. Left out on PC-1500/1500A regions |

### 1.4 Addressing

Enable groups OR'd together; terms within a group AND'd. The same shape
is used for a region's `addressing` and a banked region's `bank-window`.

```yaml
addressing:
  any-of:                      # OR across groups
    - all-of:                  # AND within a group
        - chip-select: Y0
        - address-bits: { A13: 1, A12: 1, A11: 1 }
      span: 0x0800             # 2 KB, served through A0–A10
      maps-to: 0x0000          # where this slice lands in the region
    - chip-select: S1          # a bare strobe, no decode on this path
      span: 0x0800
      maps-to: 0x0800
```

A spec with one group may give `all-of` directly; a group with one term
may give the bare term.

| Term | Meaning |
|---|---|
| `chip-select: NAME` | That line asserted. Several strobes for one region are separate OR'd groups, not a list |
| `chip-select: [A, B]` | All listed lines asserted at the same access (AND); uncommon |
| `signal: NAME` / `signal: [...]` | As `chip-select`, for a line that isn't a decode strobe (`PVOUT`) |
| `signal-negated: NAME` | That line not asserted, to split a window by a half-select line |
| `address-bits: { An: 0\|1, … }` | Those address lines at those levels, AND'd into the group; the lines not listed are not tested. Doesn't set `span` |
| `memory-range: { from, to }` | A raw address window the module decodes itself, with no chip select in the group. Sets the span to `to − from + 1`; the offset is `address − from` |

| Group key | Default | Meaning |
|---|---|---|
| `span` | required unless the group has a `memory-range` | Size of the slice behind the group; served through the low log2(span) address lines |
| `maps-to` | packed in order, each after the previous | Offset in the region (or, in a `bank-window`, in the current bank) where the slice starts |

The slices must tile `capacity` (unbanked) or `bank-size` (banked)
exactly: no gap, no overlap.

### 1.5 Content

```yaml
content: rom            # bare form: regular | rom; flash needs its protocol
```

```yaml
content:
  kind: regular
  writable: true              # false = wired read-only (not a switch)
  power-up-fill: 0x00         # default 0x00
  write-protect: unprotected  # only for a module with a switch: protected | unprotected
```

```yaml
content:
  kind: flash
  power-up-fill: 0xFF         # default 0xFF; where initial-content leaves gaps
  protocol:
    unlock-sequence:
      - { address: 0x5555, data: 0xAA }
      - { address: 0x2AAA, data: 0x55 }
    byte-program-command: 0xA0
    erase-setup-command:  0x80
    sector-erase-command: 0x30
    chip-erase-command:   0x10
    sector-size: 0x1000
    reset-command: 0xF0
    command-address-mask: 0x7FF   # optional: the low address bits the decoder compares
```

- Every `protocol` key is required except `command-address-mask`
  (default: the full address). `content: flash` without `protocol` is an
  error.
- `rom` has no options; its bytes come from `initial-content`.
- **Split content** in a banked region:

  ```yaml
  content:
    by-bank:
      - { banks: "0-7",  kind: regular, power-up-fill: 0x00 }
      - { banks: "8-15", kind: flash, protocol: { ... } }
  ```

  `banks` is a `"lo-hi"` string or one integer. The ranges partition
  `0 … bank-count − 1` with no gap or overlap.

### 1.6 Banking

```yaml
banking:
  latch:
    type: trigger-based            # trigger-based | line-based
    trigger: { pin: 18 }           # a write-strobe pin, or { io-port: 0x28 }
    sampled-lines: [A0, A1, A2, A3]  # sampled-lines[0] is the least significant bit
    source-domain: address         # address (CE-163) | data (CE-1601M's OUT (28H))
    # line-based instead: lines: [K0, K1, K2], source-domain: address-select
  bank-count: 2                    # banks fitted (may be fewer than the latch allows)
  bank-size: 0x8000
  bank-window:                     # addressing (§1.4), one level deeper
    any-of: [ ... ]
```

A banked region's own `addressing` is only a gate: its terms say
whether an access concerns this region at all, without `span` or
`maps-to`. All slices are mapped in `bank-window`, whose offsets are
into the current bank.

### 1.7 Initial content

```yaml
initial-content:
  fill: 0xFF                 # bytes no block covers; default: the range's power-up-fill
  blocks:
    - bank: 0                # left out for an unbanked region
      offset: 0x0000         # start within the region or bank
      encoding: addressed-hex   # hex | base64 | addressed-hex (README.md §1.1)
      bytes: |
        $0000: 43 16 00 00 ...
```

- Several banks are several blocks with different `bank`. Two blocks
  touching the same byte are an error.
- A `rom` range not covered by blocks is incomplete and rejected; `fill`
  doesn't count for ROM. A `flash` range without blocks is valid
  (erased).
- A PC-1600 module header and jump table are bytes in a block at their
  fixed offset.

### 1.8 Templates, instances and the preset

- `template: true`: never written. Every bundled module is one; a user
  template is listed with them.
- Battery-backed template: Name & Save copies it, with the live contents
  as `initial-content`, into an instance without the `template` key.
- Instance: every write to the module is saved back into its file.
- `slot-N: { name: X }` finds the definition whose `module-name` is X;
  `slot-N: { file: … }` names the file. How a name is found (bundled
  modules first, then the save folder) is the loader's.
- The loader checks the host (the model, or the PC-1600 slot) against
  `compatible-hosts` and refuses a host that is not listed.
- `slot-N-rom:` replaces the bytes of a one-region ROM module
  ([preset.md §3.2](preset.md#32-slot-n-rom-1600)); its definition then
  carries a placeholder (`$0000: FF...`) and still loads alone.

### 1.9 Coverage

The model expresses these modules:

| Module | D1 | D2 | D3 / D4 | Hosts |
|---|---|---|---|---|
| CE-151 | S1 OR S2 | Regular | Unbanked | PC-1500, PC-1500A, PC-1600 Slot 1 |
| CE-155 | (Y0 AND A11–A13 = 111) OR S1 OR S2 OR S3 | Regular | Unbanked | PC-1500, PC-1500A, PC-1600 Slot 1 |
| CE-157 | RAM as CE-151/155; ROM probably a Y2 sub-range, PV-selected | Regular; ROM | ROM unconfirmed | PC-1500, PC-1500A |
| CE-159 | as CE-155 | Regular, write-protect switch | Unbanked | PC-1500, PC-1500A |
| CE-160 | as CE-159 | ROM | Unbanked | PC-1500, PC-1500A |
| CE-161 | Y0, 16 KB | Regular, write-protect switch, battery | Unbanked | all four |
| CE-163 | Y0, 16 KB window | Regular | Trigger: pin 18, `A0` (address) | PC-1500, PC-1500A |
| CE-1638 | Y0, 16 KB window | Regular | Trigger: pin 18, `A0`–`A2`, 8 banks | all four |
| CE-163F | Y0, 16 KB window | Banks 0–7 Regular, 8–15 Flash | Trigger: pin 18, `A0`–`A3`, 16 banks | all four |
| CE-1600M | RAM2 / RAM1 with `PVOUT` as half-select | Regular | Unbanked (the host's own banking) | PC-1600 Slot 1, Slot 2 |
| CE-1601M | RAM1 | Regular | Trigger: `OUT (28H)`, `D0`–`D2` (data); bank window nests `PVOUT` | PC-1600 Slot 2 |
| superRAM | RAM1 | Regular | Trigger: `OUT (28H)`, `D0`–`D3`, 16 banks | PC-1600 Slot 2 |
| CE-1620M | as CE-1600M | ROM | Unbanked | PC-1600 Slot 1, Slot 2 |
| CE-502B | Y0 | ROM: a program module (header at &0000, BASIC from &00C5) | Unbanked | PC-1500, PC-1500A |

CE-157's ROM half and CE-160's exact layout are not known; the model
can express them once they are. A line-based latch is expressible,
though no known module needs one.

---

## 2. Solid State ROM module

A TI-58/58C/59 module holding one ROM of the TMC0540 chip family, such
as the Master Library. One file per module holds its description, its
ROM, and the pre-printed cue card of each library program.

### 2.1 Keys

```yaml
format: solid-state-rom-module
format-version: 1
module-name: ML                         # the catalogue ID
description: Master Library             # the visible name
title: MASTER LIBRARY MODULE -1-        # as printed on the module
menu-sort: 0
chip: TMC0541
notes: |
  TMS0540 SECOND ROM Recorder Initialized
  Datamath Calculator Museum June 2023
  DUT: TMC0541NL ASP7749
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
    row-2: "L.R. INIT: SBR CLR"
    row-2-r: "PRINT: mm STO 00"
    row-2-r-align: right
  - program: 2
    title: "DETERMINANT, MATRIX, & SIMUL. EQ."
    id: ML-02
    "A'": "i; \to x_{i}"
    A: n
```

| Key | Required | Value |
|---|---|---|
| `format` | yes | `solid-state-rom-module` |
| `format-version` | yes | `1` |
| `module-name` | yes | The name tag: the catalogue ID, two upper-case letters (`ML`). What `slot-1: { name: … }` resolves against |
| `description` | yes | The visible name ("Master Library"), shown in the module picker and settings |
| `title` | no | The module's title as printed by TI ("MASTER LIBRARY MODULE -1-") |
| `menu-sort` | no | An integer that orders the modules in a list; equal values keep the loader's order. Default 0 |
| `chip` | no | The ROM chip (`TMC0541`) |
| `notes` | no | Free text: where the dump comes from. Ignored by the loader |
| `rom` | yes | `size`, `encoding` and `bytes` (§2.2) |
| `cue-cards` | no | The pre-printed cue cards, one per library program (§2.3) |

`description` is required here, unlike in the other formats, because it
is the only visible name a module has.

### 2.2 ROM

| Key | Value |
|---|---|
| `size` | The ROM's size in bytes, at most 5000 |
| `encoding` | `addressed-hex`, `hex` or `base64` ([README.md §1.1](README.md#11-byte-blocks)) |
| `bytes` | Covers exactly `0` … `size − 1` |

- A ROM byte holds two BCD digits; it is written as hex, so the digits
  `2` `1` are the byte `21`.

### 2.3 Cue cards

Each item of `cue-cards` is the cue card of one library program: a cue
card block ([cue-card.md](cue-card.md)), a module cue card, plus:

| Key | Required | Value |
|---|---|---|
| `program` | yes | The library program number, 1–99, as selected with `Pgm` |

Two items with the same `program` are an error. Every library program
of the module should have an item, as every program has a cue card on
the real module. A library program without an item has no module cue
card: selecting it takes off any module cue card, and the own card
shows, or nothing.

When the bundled modules are written in this format, every library
program without a pre-printed cue card gets an item that looks like
the stand-in card 2.3.0 showed for it: the module's `description` as
`title` and its `module-name` as `id` (today
`Shared/CueCard.cpp:402-418`).

### 2.4 The bundled modules

| `module-name` | `description` | `chip` |
|---|---|---|
| `ML` | Master Library | TMC0541 |
| `ST` | Applied Statistics | TMC0542 |
| `RE` | Investment | TMC0543 |
| `SY` | Surveying | TMC0544 |
| `NG` | Marine Navigation | TMC0545 |
| `AV` | Aviation | TMC0546 |
| `LE` | Leisure Library | TMC0547 |
| `SA` | Securities Analysis | TMC0548 |
| `BD` | Business Decisions | TMC0549 |
| `MU` | Math/Utilities | TMC0550 |
| `EE` | EE Library | TMC0551 |
| `SE` | Structural Engineering | TMC0553 |
| `AG` | Agriculture | TMC0554 |
| `RP` | RPN Simulator | TMC0555 |
