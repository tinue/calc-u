# Module definition format

A **module definition** (`.module.yaml`) describes one memory module: its
memory, how it is addressed and the hosts it fits. The bundled memory
modules are module definitions, and so is a battery-backed module you
saved with Name & Save. [Module definitions](09-module-definitions.md)
describes *what* a definition expresses and *why*; this page is *how* it
is written, in its current version, `format-version: 1`.

## 0. The file

- **YAML**, UTF-8, one module per file, extension `.module.yaml` (a
  `.card.yaml` of Calc-U-1600 0.8 is not read: rename it to
  `.module.yaml`; `format: memory-module` may be added). A
  definition is written and reviewed by hand, so it has comments (`#`
  starts one anywhere).
- Integers are decimal (`8192`) or hex with `0x` (`0x2000`); hex is usual
  for addresses and byte values.
- An unknown key is an error, not ignored: a misspelled key must not load
  silently.
- The bundled modules are the bundled templates that come with the
  product ([§7](#7-worked-examples) lists them); your own definitions and saved instances lie in the save folder, where
  the module pickers list them with the bundled ones.

---

## 1. Top-level keys

| Key | Required | Value |
|---|---|---|
| `format` | no | `memory-module`. Calc-U-1600 writes it into every module it saves; a file without it is read too, one with another value is refused. |
| `format-version` | yes | Integer; this document is version **1**. |
| `module-name` | yes | Short identifier for the module (e.g. `CE-155`, `CE-1600M`). Shown as it is in the module picker, and what a preset's `slot-N: { name: … }` resolves against (§8). Unique within a folder of modules. |
| `description` | no | One line about the module (e.g. `"8 KB static RAM"`), the picker's tooltip for it. A saved instance keeps its template's. |
| `compatible-hosts` | yes | List, non-empty, drawn from `PC-1500`, `PC-1500A`, `PC-1600-Slot-1`, `PC-1600-Slot-2`. The loader's **only** compatibility check: a host not on this list is declined; a host on it gets no further test. |
| `definition-terminology` | yes | Exactly one of the same four identifiers. Names the pin-name vocabulary every `chip-select` / `signal` / `line` field in the file is resolved against, once, at parse time. Must appear in `compatible-hosts`. |
| `regions` | yes | List of one or more region maps (§2). |
| `battery` | no | `true` for battery-backed hardware. A battery-backed template offers Name & Save, which makes the module a medium ([Media files](10-media-files.md#battery-backed-module)). |
| `template` | no | `true` marks a read-only **template** (§8): the product never writes the file, wherever it lies. Absent or `false` = an instance, autosaved in place. Name & Save and autosave never write it; a preset's `saveas: { as: template }` writes `template: true`. |
| `saved` | no | The time of the last write (UTC, ISO 8601), written into every saved instance; for information. |
| `notes` | no | Free text; ignored by the loader. |

The reset-on-load behaviour is a loader invariant,
not a field — no key can opt out of it.

**Versioning.** A reader rejects a file without `format-version`, or with
a version it does not know, rather than guess; the version is checked
before the other keys. Any change a version-1 reader would misread
requires a new version number. A saved instance keeps the line of the file
it was saved from.

### Terminology → signal names

The name in a `chip-select` / `signal` / `line` field must be legal for
the declared `definition-terminology`:

| Terminology | Legal signal names (besides `A0`… address / `D0`… data lines) |
|---|---|
| `PC-1500` | `Y0`, `Y2`, `S1`, `S2`, `S3`, `S4`, `PU`, `PV` |
| `PC-1500A` | `Y0`, `Y2`, `S3`, `S4`, `S5`, `PU`, `PV` |
| `PC-1600-Slot-1` | `RAM2`, `PVOUT`, `S1`, `S2`, `S3`, `PU`, `PT` |
| `PC-1600-Slot-2` | `RAM1`, `PVOUT`, `K0`, `K1`, `K2`, `PU`, `PT` |

Each name
resolves, once at parse time, to one physical 40-pin contact
(`Y0`/`RAM2`/`RAM1` → pin 4, `S4`/`PVOUT` → pin 5, `S1`/`S3`/`K0` → pin
16, `S2`/`S4`/`K1` → pin 17, `S3`/`S5`/`K2` → pin 18, `Y2`/`PT` → pin 19;
`PU` → pin 2 and `PV` → pin 3 on the PC-1500/1500A, as measured on a real
unit (the TRM prints them swapped), `PU` → pin 3 on the PC-1600 slots).

Whether a name resolves to a live pin or a dormant one on a *particular*
compatible host is not checked here — that is the author's dormant
sub-region judgement.

---

## 2. Region map

| Key | Required | Value |
|---|---|---|
| `name` | yes | Identifier, unique within the file. |
| `addressing` | yes | Addressing spec (§3) — dimension D1. |
| `content` | yes | Content spec (§4) — dimension D2. |
| `banking` | yes | `none`, or a banking map (§5) — dimensions D3/D4. |
| `capacity` | unbanked only | Backing-store size in bytes for the one copy of the region. Required when `banking: none`; omitted when banked (there it is `bank-count * bank-size`). |
| `initial-content` | ROM: yes (must cover every byte) · Flash/Regular: optional | Byte content carried in the file (§6). Allowed for any content kind, no restriction — a Regular (RAM) range can power up pre-loaded exactly like a Flash range's initial image. |
| `pc1600-module-class` | PC-1600 regions only | `program` \| `system` \| `ram-disk` \| `plain-ram-no-header` — authoring cross-check against the header bytes in `initial-content`. Omit on PC-1500/1500A regions. |

---

## 3. Addressing spec

An addressing spec is **enable groups OR'd together**; each group is
**terms AND'd together**. It appears in three places, all the same shape:
a region's `addressing`, a banked region's `bank-window`, and (in
principle) deeper nestings.

```yaml
addressing:
  any-of:                      # OR across groups
    - all-of:                  # AND within a group
        - chip-select: Y0
        - address-bits: { A13: 1, A12: 1, A11: 1 }   # the on-module decoder's select inputs
      span: 0x0800             # 2 KB — served via the low 11 address lines (A0 to A10)
      maps-to: 0x0000          # where this slice lands in the backing store
    - chip-select: S1          # bare dedicated strobe, no address decode on this path
      span: 0x0800
      maps-to: 0x0800
    - chip-select: S2
      span: 0x0800
      maps-to: 0x1000
```

Shorthand: a single-group spec may drop `any-of` and give `all-of`
directly; a single-term group may drop `all-of` and give the bare term.

### Group terms

| Term | Meaning |
|---|---|
| `chip-select: NAME` | Named line asserted. Friendly alias for `signal`. The common case: one dedicated strobe standing for one slice of storage, with no address decode on that path. Several such strobes for one region are separate OR'd groups (one per `- ` entry under `any-of`), **not** a list. |
| `chip-select: [A, B, C]` | All listed lines asserted at the same access (an AND). Uncommon — the host's 2 KB strobes are mutually exclusive, so this is only for a strobe that is qualified by another live signal. |
| `signal: NAME` / `signal: [..]` | Same as `chip-select`; use when the line isn't a decode strobe (e.g. `PVOUT`). |
| `signal-negated: NAME` | Line explicitly **not** asserted. Needed to split a window by a half-select line (`PVOUT` low vs. high). |
| `address-bits: { An: 0\|1, … }` | Specific address lines required at specific levels, AND'd into the group. Models an on-module decoder that gates on a **few** address lines alongside a chip select — *not* a full address-bus comparison. CE-155's `Y0` chip: `{ A13: 1, A12: 1, A11: 1 }` (the "`AD11–AD13 = 111`" of [PC-1500 address decoding](https://github.com/tinue/Sharp1500-1600-Ref/blob/main/PC-1500/Memory-Architecture/PC-1500-Address-Decoding.md) §3.2). Lines not listed are not tested. Does **not** set `span` — the low, un-listed lines are the offset. |
| `memory-range: { from, to }` | A raw address window the module decodes itself, with **no** chip select in the group. Fixes `span` from `to − from + 1`; the offset within the slice is `address − from`, so the window need be neither a power of two nor aligned. Flagged distinctly because the author's host-compatibility judgement has no pin to anchor to. |

### Group keys

| Key | Default | Meaning |
|---|---|---|
| `span` | required unless the group has a `memory-range` | Size in bytes of the storage slice behind this group. The module serves it using the low `log2(span)` address lines as the offset (a `memory-range` group uses `address − from` instead) (`0x0800` ⇒ A0 to A10); it does **not** decode any higher line except those an `address-bits` term names. |
| `maps-to` | groups packed in declaration order, each starting where the previous ended | Byte offset in the region's backing store (or, inside `bank-window`, the current bank's slice) that the low end of this group's slice maps to. |

The groups' slices must tile `capacity` (unbanked) or `bank-size`
(banked) exactly — no gap, no overlap.

---

## 4. Content spec

### Whole-region, one kind

```yaml
content: rom            # bare string form: regular | rom | flash (defaults for each kind)
```

```yaml
content:
  kind: regular
  writable: true              # false = wired read-only (not a togglable switch)
  power-up-fill: 0x00         # per-range power-up byte; default 0x00 (CMOS RAM after a power loss)
  write-protect: unprotected  # optional: the module has a write-protect switch (CE-161); omit if not.
                              # protected | unprotected = the switch position, part of the module's file.
                              # Protected refuses every write, host pokes included. Name & Save with
                              # "Write-protect" / `saveas: { …, protected: true }` saves it as `protected`.
```

```yaml
content:
  kind: flash
  power-up-fill: 0xFF         # default 0xFF (erased); applies only where `initial-content` leaves gaps
  protocol:                   # chip/firmware properties, not inferred
    unlock-sequence:
      - { address: 0x5555, data: 0xAA }
      - { address: 0x2AAA, data: 0x55 }
    byte-program-command: 0xA0
    erase-setup-command:  0x80
    sector-erase-command: 0x30
    chip-erase-command:   0x10
    sector-size: 0x1000
    reset-command: 0xF0
    command-address-mask: 0x7FF   # optional; default = full address, no masking.
                                   # How many low address bits the command
                                   # decoder actually looks at before comparing
                                   # against an unlock-sequence/command address --
                                   # some flash chips (recovered from a real
                                   # CE-163F firmware disassembly) decode fewer
                                   # bits than the full bus, so an unlock write's
                                   # upper address bits don't have to match.
```

Every field above is required except `command-address-mask` — there is no
sensible default for a chip/firmware property, so `content: flash`
as a bare string (no `protocol:`) is a load error, not a "use the defaults"
shorthand.

`rom` takes no options beyond the implicit read-only-forever rule; its
bytes come from `initial-content` (§6), which is mandatory for ROM.

### Split content in a banked region

```yaml
content:
  by-bank:
    - { banks: "0-7",  kind: regular, power-up-fill: 0x00 }
    - { banks: "8-15", kind: flash, protocol: { ... } }
```

`banks` is a `"lo-hi"` string or a single integer. Ranges must partition
`0 .. bank-count-1` with no gap or overlap. A single-kind banked region
just uses the whole-region form above.

---

## 5. Banking map

```yaml
banking:
  latch:
    type: trigger-based            # trigger-based | line-based
    # --- trigger-based ---
    trigger: { pin: 18 }           # a memory-write strobe pin (CE-163: S3),
                                   # OR { io-port: 0x28 } for an OUT (n) write (CE-1601M)
    sampled-lines: [A0, A1, A2, A3]  # bits latched on the trigger; sampled-lines[0] is the LSB
    source-domain: address         # address → bits of the address bus (CE-163)
                                   # data    → bits of the written byte (CE-1601M's OUT (28H))
    # --- line-based (instead of trigger/sampled-lines) ---
    # lines: [K0, K1, K2]          # the bank number read live off these lines
    # source-domain: address-select
  bank-count: 2                    # live banks (may be < what the latch width allows)
  bank-size: 0x8000               # bytes per bank
  bank-window:                     # addressing spec (§3), nested one level
    any-of: [ ... ]
```

A banked region's top-level `addressing` (§2) is a **pure gate** — "is
this access one my banking applies to at all" — so it carries only
terms, no `span` / `maps-to`. All slice mapping happens in `bank-window`,
which carries its own full addressing spec so a second, independent
sub-range (CE-1601M / *superRAM* `PVOUT`) can be expressed as
"selected-bank AND half-select line", one level deeper. Inside
`bank-window`, `span` / `maps-to` offsets are into the current bank's
`bank-size` slice.

Both confirmed banked modules are **trigger-based** and differ only in
`source-domain`: CE-163's `{ pin: 18 }` strobe samples the **address**
bus; CE-1601M's `{ io-port: 0x28 }` write samples the **data** bus. Both
latch the decoded bank internally until the next trigger.

Descriptive "Vertical / Horizontal" is **not** a field:
line-based = Horizontal, trigger-based = Vertical.

---

## 6. `initial-content`

Bytes fixed by the definition — all of ROM's content (required), or a
Flash or Regular (RAM) range's *initial* array (optional; absent ⇒ the
range powers up as its `power-up-fill` byte). There is no
content-kind restriction: a Regular range takes `initial-content` exactly
like a Flash range does — CE-163F's RAM banks and Flash banks are both
populated this way when dumped and pasted back in (§9). Never an external
file: a ROM under development comes in through the preset instead (§8,
`slot-N-rom:`).

```yaml
initial-content:
  fill: 0xFF                 # bytes no block covers; default is the bank's
                              # own power-up-fill (content §4), not always 0xFF
  blocks:
    - bank: 0                # omit entirely for an unbanked region
      offset: 0x0000         # start offset within the region / bank slice
      encoding: hex          # hex | base64 | addressed-hex
      bytes: |
        43 16 00 00 ...       # whitespace/newlines ignored for hex
    - bank: 1
      offset: 0x0000
      encoding: addressed-hex
      bytes: |
        $0000: FF...          # the whole bank slice, one byte value
```

- **Multi-bank** content is just multiple `blocks` with different `bank:`
  keys — one file populates every bank. Two blocks touching
  the same bytes is an error.
- A region/bank-range marked `rom` with no covering block is incomplete
  and is rejected at load. A `flash` range with none is valid.
- For a PC-1600 header-bearing module the 8-byte header and jump table
  are just bytes in a block at their fixed offset — there are
  no separate header fields.

### `encoding: addressed-hex`

A project-specific dialect designed so a memory dump can be pasted
straight into a `bytes: |` block with no reformatting — the INSPECTOR's
**Module YAML** button ([Debugger](07-debugger.md)) writes exactly this. Lines are
either an explicit row or a same-byte run:

```yaml
bytes: |
  $0000: 00...
  $0050: 00 00 00 00 00 00 11 F1  8A 26 45 33 2C 58 40 12
  $0060: F0 97 22 42 41 4E 4B 20  22 3B F1 6F 26 45 32 40
  $0070: 13 F0 97 22 56 32 30 22  40 FF FF FF FF FF FF FF
  $0080: FF...
  $3FF0: AA...
```

- `$XXXX: <up to 16 space-separated hex byte pairs>` places those literal
  bytes starting at that address.
- `$XXXX: XX...` (the only token on its line) means "byte `XX` repeats from this address up to (but not
  including) the next line's address" — or up to the end of the block's
  span for the last line. This is exact, not a guess: because lines must
  partition the block's address range with no gap or overlap, a run's
  length is always computable from the surrounding addresses, unlike an
  elided hex dump (which drops a run with no record of which byte value it
  was).
- A run only ever stands for one uniform byte value — content that
  alternates between two fill values (e.g. `00`/`FF` rows) is written out
  in full, the same as an ordinary row.

### Encodings

`addressed-hex`, `hex` and `base64`; any other encoding is an error.

---

## 7. Worked examples

| File | Module | Shows |
|---|---|---|
| `ce151.module.yaml` | CE-151, 4 KB | Two bare-strobe `span` groups (`S1`, `S2`), one 2 KB chip each, no on-module decode (Service Manual schematic 5-3); PC-1500/1500A and PC-1600 Slot 1. |
| `ce161.module.yaml` | CE-161, 16 KB | One bare `Y0` group spanning the whole 16 KB window; battery-backed with a `write-protect:` switch ([CE-161 program module](https://github.com/tinue/calc-u/blob/main/products/calc-u-1600/docs/developer/CE-161-Program-Module.md)); PC-1500/1500A and both PC-1600 slots. |
| `ce155.module.yaml` | CE-155, 8 KB | one `address-bits` group (the on-module `AD11–AD13` decoder) OR'd with three bare-strobe `span` groups; an author-declared host list covering PC-1500 and PC-1600 Slot 1. |
| `ce1600m.module.yaml` | CE-1600M, 32 KB | `PVOUT` folded into addressing as a half-select line — unbanked despite two physical halves; one Slot-1-terminology file declared for both PC-1600 slots (`RAM2` resolves to the pin-4 enable on both). |
| `ce1601m.module.yaml` | CE-1601M, 64 KB | Trigger-based vertical banking — `trigger: { io-port: 0x28 }`, `source-domain: data` (the byte written by `OUT (28H)`); `bank-window` nesting the `PVOUT` half-select. |
| `superram.module.yaml` | superRAM, 512 KB | The CE-1601M mechanism with a 4-bit (`D0`–`D3`) `OUT (28H)` latch and all 16 × 32 KB vertical banks fitted; PC-1600 Slot 2 only. |
| `ce1638.module.yaml` | CE-1638, 128 KB | A single-kind banked region — trigger-based, address-domain, 8 banks sampling `A0`-`A2`. |
| `CE-502B.module.yaml` (comes with the ROMs) | CE-502B, 16 KB ROM | `content: rom` with one `addressed-hex` block covering the whole region: a Sharp program module (ROM header at &0000, BASIC program from &00C5) on Y0. |
| `ce163f.module.yaml` | CE-163F, 256 KB | `content: by-bank:` (§4) splitting one 16-bank region into Regular (0-7) and Flash (8-15) content, and a `flash` `protocol:` block including `command-address-mask` for its real low-11-bit command decode quirk. |

---

## 8. Plugging a module in: `slot-1:`, `slot-2:` and `slot-N-rom:` in a preset

A preset plugs a module into a slot by name or by file (the preset's
other keys: [Preset sections](10-preset-sections.md)):

```yaml
# PC-1500 / PC-1500A: one slot
slot-1: { name: CE-155 }

# PC-1600: two slots
slot-1: { name: CE-1600M }
slot-2: { file: cards/prototype.module.yaml }   # relative to the preset
```

- `name:` finds the definition whose `module-name` is that name: the
  bundled modules first, then the save folder, so a preset can name a
  module you saved (`slot-2: { name: CE-1601M - Programs }`; the file's
  name is not consulted). A bundled module hides a saved one of the same
  name. No match is an error, and so are two files in one folder that
  claim the same name.
- `file:` names a definition directly, relative to the preset.

**A ROM from a file: `slot-N-rom:`.** While a ROM module is being
developed, its bytes come from the build, not from the module definition:

```yaml
slot-1: { file: module.module.yaml }      # decode, banking, the ROM's size
slot-1-rom: { file: build/module.bin }  # what the ROM holds
```

The module definition stays complete: its ROM region holds a placeholder
(`$0000: FF...`, §6) and loads on its own. `slot-N-rom:` replaces those
bytes, and only those:

- it needs a module in the same slot;
- the module must be a ROM module of one region (`content: rom`, no
  `by-bank:` split with RAM or Flash);
- the file must be exactly the region's size, or the preset fails;
- the file is read each time the preset is loaded, so a fresh start in
  the debugger picks up a rebuilt ROM ([Debugger](07-debugger.md));
- such a module is never saved: no autosave, no Name & Save, and a
  `saveas:` of its slot is refused.

**Templates and instances.** What happens to a module's file is decided
by the file itself, not by where it lies:

- `template: true`: a **template**, never written. Every bundled module is
  one, and a template of your own in the save folder is listed with the
  bundled modules. A battery-backed template (`battery: true`) offers
  **Name & Save**, which copies it, with the module's memory as
  `initial-content:`, into an instance in the save folder.
- No `template` key: an **instance**. Every write to the module is saved
  back into its file, wherever it was loaded from. A preset's
  `saveas: { place: slot-1, as: live, name: … }` saves a copy as an
  instance, `as: template` as a template.

Either way the host (the model, or the PC-1600's slot) must be in
`compatible-hosts`, or the module is refused. The slot pickers list the
modules whose `compatible-hosts` cover the model and slot, by
`module-name`.

## 9. What is supported

The parser reads the whole format and validates it; the product
supports:

- **Content:** Regular, Flash and ROM, single-kind or `by-bank`-split
  across a banked region's banks. A `rom`
  range ignores every write — the machine's CPU, the debugger's poke and the loaders alike — and its `initial-content` must cover every
  byte (`fill:` doesn't count). `initial-content` is supported via
  `encoding: addressed-hex | hex | base64` (§6); a preset's `slot-N-rom:`
  replaces a ROM module's bytes (§8).
- **Banking:** Unbanked, and **trigger-based** Banked (both a memory-write
  strobe `{ pin: N }` sampling the address bus and an `{ io-port: 0xNN }`
  write sampling the data bus). `line-based` is a hard load error.
- On the PC-1600, `pc1600-module-class: plain-ram-no-header` (or the field
  absent) needs nothing further; the header-bearing classes need an
  `initial-content` block covering the header/jump-table offset.
