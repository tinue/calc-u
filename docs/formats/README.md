# Calc-U file formats

One specification of the preset and media formats of Calc-U-59 and
Calc-U-1600. It says **what** each file holds and means, including the
runtime behaviour that is part of that meaning (speed, waiting). It does
not say how a product implements a format, or whether it implements every
part of it: which keywords a product's loader carries out is decided per
product.

Terms follow `docs/GLOSSARY.md`.

## Files of this specification

| File | Contents |
|---|---|
| [README.md](README.md) | This index and the rules every format shares: syntax, byte blocks, version, name and description, addressing a resource, errors, platform markers |
| [preset.md](preset.md) | The preset format, with the chapter on full speed, and the keystroke program file |
| [media.md](media.md) | The media: magnetic card, floppy disk, cassette tape; and the TI-58C's constant memory file |
| [modules.md](modules.md) | The modules: module definition (memory module) and Solid State ROM module |
| [cue-card.md](cue-card.md) | The cue card block, shared by presets, magnetic cards, Solid State ROM modules and the constant memory file |

## Formats at a glance

| Format | Kind | Suffix | `format:` | `format-version` | Name key | Products |
|---|---|---|---|---|---|---|
| Preset | — | `.ti59` `.ti58` `.ti58c` `.pc1500` `.pc1500a` `.pc1600` | — | 2 | `preset-name` | both |
| Magnetic card | medium | `.magnetcard.yaml` | `ti59-magnetic-card` | 3 | `card-name` | Calc-U-59 (TI-59) |
| Floppy disk | medium | `.floppy.yaml` | `ce1600f-floppy` | 2 | `disk-name` | Calc-U-1600 (PC-1600) |
| Cassette tape | medium | `.tape.yaml`, with a `.wav` | `cassette-tape` | 1 | `tape-name` | Calc-U-1600 |
| Constant memory | — | `ti58c.memory.yaml` | `ti58c-constant-memory` | 1 | `memory-name` | Calc-U-59 (TI-58C) |
| Module definition | module (memory module) | `.card.yaml` | — | 1 | `module-name` | Calc-U-1600 |
| Keystroke program | — | `.keystrokes.yaml` | `ti-keystroke-program` | 1 | `program-name` | Calc-U-59 |
| Solid State ROM module | module | `.rom.yaml` | `solid-state-rom-module` | 1 | `module-name` | Calc-U-59 |

Each medium has a format of its own; media are not merged into one
format.

---

## 1. Syntax: simplified YAML

Every format is written in one YAML subset, not general YAML:

- UTF-8 text. Lines end in `\n`; a `\r` before it is dropped.
- Block mappings by indentation, spaces only. A tab anywhere is an
  error. Writers use 2 spaces per level.
- Block sequences `- item`. An item is a scalar, a flow collection, or a
  mapping (`- key: value`, with further keys aligned under the first).
- Flow sequences `[a, b, c]` and flow mappings `{ key: value, key: value }`.
- Scalars bare, `"double-quoted"` or `'single-quoted'`, with **no escape
  processing**: a quoted value cannot contain its own quote character.
- `key: |` literal block scalars, common indentation stripped.
- `#` starts a comment as a whole line, or as ` #` after a value,
  outside quotes and outside `[]` / `{}`. Exception: the payload of a
  preset's `type:` and `expect:` steps runs verbatim to the end of the
  line ([preset.md §5.1](preset.md#51-input)).
- Not supported: anchors and aliases, tags, `>` folded scalars, several
  documents, complex keys, flow scalars over several lines.

**Keywords** (keys and fixed values) are kebab-case and lower-case:
`format-version`, `cue-card`, `full-speed`, `solid-state-card`. Values
the user picks (names, texts, key IDs) keep their case unless a format
says otherwise. The exception is the preset's `debug:` block
([preset.md §3.4](preset.md#34-debug-1600)): its keys and values are camelCase
(`stopOnEntry`, `cleanStart`, `allReset`), as in the debug launch
configuration they mirror.

**Numbers.** `&`, `0x` or `$` makes a number hex; without one it is
decimal. In the module definition, a number is decimal or hex with `0x`.

**Booleans** are `true` or `false`.

**Unknown keys** are an error at every level of every format.

### 1.1 Byte blocks

Bytes are carried in a `bytes: |` block scalar, next to an `encoding:`
key that says how they are written. Each format says which encodings it
allows and which range the block covers.

| Encoding | Lines |
|---|---|
| `hex` | Hex byte pairs; whitespace and newlines are ignored |
| `base64` | Base64; whitespace and newlines are ignored |
| `addressed-hex` | `$XXXX: <1–16 hex byte pairs>` places those bytes from that address. `$XXXX: XX...`, the only token on its line, repeats byte `XX` up to the next line's address, or to the end of the block's range for the last line. Lines in address order cover the block's range exactly: no gap, no overlap. A run stands for one byte value only |
| `register-hex` | `Rnnn: <8 hex byte pairs>`: one TI-58/59 register of 16 digits, `nnn` its decimal number. The format says which numbers are allowed. Number the register's digits 0–15 as the machine's RAM holds them; byte k (k = 0–7) holds digit 14 − 2k in its high half and digit 15 − 2k in its low half |

`addressed-hex` is made so that a memory dump pastes straight in. Its
**canonical form**, which writers emit so that files diff cleanly: rows
of 16 bytes, upper-case hex, `$` and a 4-digit upper-case address, one
space between bytes and two before the 9th; a row of 16 equal bytes
starts a run `$XXXX: XX...`, which absorbs every following whole row of
the same byte.

## 2. Version, name and description

Every format has these keys at the top level:

| Key | Required | Meaning |
|---|---|---|
| `format-version` | yes | The integer version of the format the file is written in |
| `<kind>-name` | yes, except in presets and the constant memory file | The **name tag**: the short name shown in GUIs, pickers and drop-down lists, and the name a `name:` reference resolves against (§3). The key is named after the kind: `preset-name`, `card-name`, `disk-name`, `tape-name`, `memory-name`, `module-name`, `program-name` |
| `description` | no, except in the Solid State ROM module | One line about the item, for a tooltip or a longer list entry |

- A reader rejects a file without `format-version`, or with a version it
  does not know, rather than guess. It checks the version before any
  other key, so a newer file reports "unsupported version", not "unknown
  key".
- Any change a reader of version n would misread needs version n + 1.
  A format whose requirements stay the same keeps its number, even when
  its specification is rewritten: a new edition of this specification
  is no reason for a new version.
- When a new version of a format becomes available, a release reads the
  previous version and silently upgrades the file in place to the
  current one (ADR 0037). The first release of each product from the
  source repository is the one exception: it reads only the current
  version, and older files are upgraded by hand (ADR 0037, Scope).
- A preset without `preset-name` is named by its file stem (`hanoi` for
  `hanoi.pc1600`). A `preset-name` is short (18 characters at most) and
  differs from the other presets of the same model. These two rules
  apply only to a `preset-name` written in the file; a file stem is
  shown as it is, whatever its length, and may repeat.

## 3. Addressing a resource

A **resource** is anything loadable that a file refers to: a module, a
medium, a program file, a ROM file, a folder. A reference always says
**how** it addresses the resource. A bare value is an error:

```yaml
floppy: Formatted                # error: is "Formatted" a file or a name?
floppy: { name: Formatted }      # the floppy disk whose disk-name is "Formatted"
floppy: { file: disks/work.floppy.yaml, side: 1 }
```

A reference is a mapping, in block or flow form, with **exactly one** of
these keys, plus the options its owner allows (`side:` and so on):

| Key | Addresses | Resolved by |
|---|---|---|
| `file: <path>` | One file, by its path | The path. A relative path is relative to the folder of the file that holds the reference; `~/` is the user's home folder |
| `name: <name>` | The resource whose name tag equals `<name>` | The loader. How a name is found is not part of the format; it is specified with the loader |
| `mem: <name>` | A resource defined inside the same file (an in-file magnetic card) | The file itself |
| `folder: <path>` | A folder, by its path (only `host-drive:`) | As `file:` |

- Where a reference is defined, it says which of these keys it accepts.
  `slot-1-rom:`, for example, accepts `file:` only.
- Names are **case preserving**: a name keeps the case it was written
  in and is shown so, but two names that differ only in upper and lower
  case are the same name, whatever the platform's file system does. This
  holds for every name tag (a lookup by `name:`, a save under a name,
  the rule that a `preset-name` differs from the others) and for the
  names that Sharp or TI gave their products (`CE-1600P`, `PC-100C`).

## 4. Errors

- A file that does not parse, has an unknown key, or breaks a rule of its
  format is **rejected as a whole**. No part of it is used.
- In a preset, the **first failing step ends the preset**. No later step
  runs.

## 5. Platform markers

Every keyword says where it applies:

- **[both]**: defined for both products.
- **[59]**, **[1600]**: one product only. A model in parentheses narrows
  it further: **[59: TI-59]**, **[1600: PC-1600]**.
- **[undefined]**: the meaning is clear, but the syntax would be specific
  to one platform and is not defined yet. A file that uses it is
  rejected until a later version of the format defines it.

A keyword marked for one product is an error in a preset for the other
product's models. Within one product, a keyword used with a model it
doesn't apply to is an error too (for example `slot-2:` on a PC-1500).
