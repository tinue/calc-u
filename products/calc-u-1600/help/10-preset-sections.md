# Preset sections of Calc-U-1600

The grammar of the preset format (format version 2), its comments, values
and references, and the keys and steps every product has are in the
family's [Reference](../../../library/help/10-reference.md). This page
lists what a preset of Calc-U-1600 holds: its header keys, its sections
and its steps, with the YAML of `bus-rom:` and `debug:`. How to use them is
in [Presets](06-presets.md).

## Header keys

| Key | Value |
|---|---|
| `format-version:` | `2` (required) |
| `model:` | `PC-1500[:A01\|A03\|A04]` (default A04), `PC-1500A`, `PC-1600[:new\|old]` (default new); any case (required) |
| `preset-name:` | the short name a list shows once the preset is chosen; default: the file name without its extension |
| `description:` | one line for lists, menus and tooltips |
| `timeout:` | the longest wait for the computer to become idle between two items, in seconds of emulated time; default `3600` |
| `printer:` | `CE-150`, `CE-1600P[:new\|old]` (default new; PC-1600) |
| `interface:` | `CE-158` |
| `slot-1:`, `slot-2:` | a module by `{ name: … }` (its module name) or `{ file: … }` (a `.module.yaml` file); `slot-2:` on the PC-1600 only |
| `slot-1-rom:`, `slot-2-rom:` | `{ file: … }`: a ROM file whose bytes replace the ROM of the module in that slot ([below](#slot-n-rom)) |
| `host-drive:` | `{ folder: … }`: the folder of drive `S3:` (PC-1600; [The host drive](09-host-drive.md)) |
| `bus-rom:` | ROM files on the 60-pin connector ([below](#bus-rom)) |
| `debug:` | the debugger's set-up of a project preset ([below](#debug)) |

Paths are relative to the preset; `~/` is your home folder.

## Sections

| Section | Value |
|---|---|
| `program:` | `file:` a BASIC listing (`.bas`), tokenized BASIC (`.bbin`), a cassette WAV, or machine code with or without a header; with `address:` (needed for headerless machine code), `length:`, or `typed: true` (type a listing through the ROM's editor). Or `text: \|` with the listing in the preset |
| `keys:` | a list of steps |

Both may appear any number of times, and are carried out in file order.

## Steps

`key:`, `type:`, `wait:`, `skip-wait:`, `let-it-finish:`, `full-speed:`,
`regular-speed:`, `expect:`, `not-expect:`, `screenshot:`, `sync-clock:`,
`trace:`, `elapsed-time:`, `cut-paper:`, and the media steps `floppy:`,
`cassette:`, `eject:`, `saveas:` ([Presets](06-presets.md#media-steps)). The
debugger steps `pause:`, `pause-on-start:`, `step:` and `resume:` are not
available on these models.

| Step | Value |
|---|---|
| `floppy:` | `{ name: … }` or `{ file: … }`, with `side: 0\|1` (default 0); PC-1600 with the CE-1600P |
| `cassette:` | `{ name: … }` or `{ file: … }` (a `.cassette.yaml`); with the CE-150 or the CE-1600P |
| `eject:` | `floppy` or `cassette` |
| `saveas:` | `place:` (`slot-1`, `slot-2`, `floppy`, `cassette`), `as:` (`live` or `template`), `protected: true` (CE-161 only), and `name:` or `file:` |
| `expect:`, `not-expect:` | text in one line of the display; `\\` for a backslash, `\xHH` for a character outside ASCII; `/…/` a regular expression |
| `sync-clock:` | sets the computer's clock from your computer's |

**Key IDs:** a single letter, digit, or `+ - * / = ( ) .`; `enter`,
`space`, `cl`, `mode`, `shift`, `def`, `sml`, `rcl`, `rsv`, `off`, `up`,
`down`, `left`, `right`, `f1`–`f6`, and `on` (or `break`). `ctrl`, `kbii`
and `bs` (PC-1600) can't be pressed from a preset.

## slot-N-rom

While a ROM module is being developed, its bytes come from the build:

```yaml
slot-1: { file: module.module.yaml }     # decode, banking, the ROM's size
slot-1-rom: { file: build/module.bin } # what the ROM holds
```

- It needs a module in the same slot, a ROM module of one region
  ([module definition format](10-module-definition-format.md)).
- The file is exactly the region's size, and it is read each time the
  preset is loaded.
- Such a module is never saved: no autosave, no Name & Save, and a
  `saveas:` of that slot fails.

## bus-rom

A list of ROM files on the 60-pin connector, for developing a ROM
extension. A bus ROM shadows a built-in ROM at the same place.

```yaml
bus-rom:
  - file: build/ext.bin
    bank: 7          # PC-1600 system bus, page B, bank 4–7
  - file: ext1500.bin
    address: &8000   # PC-1500 connector, or the PC-1600's LH5803 side
    me: 1            # optional: ME1 (default ME0)
    pv: 1            # optional: only while PV is 0 / 1
    pu: 0            # optional: only while PU is 0 / 1
```

Each item has `file` and either `bank` (4–7, PC-1600 only) or `address`
(&0000–&FFFF). `me`, `pv` and `pu` (0 or 1) go with `address` only. A
`bus-rom:` given twice refuses the preset, as any header key.

## debug

What a debug session loads, builds and starts on the computer this preset
sets up ([Debugger](07-debugger.md)). Loading the preset for use, not for
debugging, does nothing with the block beyond reading it.

```yaml
debug:
  program:
    bin: build/memtest.bin
    listing: build/memtest.rst
    cpu: lh5801
    address: &7C01
    after: stopOnEntry
  symbols: [build/memtest.sym]
```

| Key | Meaning |
|---|---|
| `program` | The program to load: `bin`, `listing`, `source` (if the main source isn't `<listing>.asm`), `symbols`, `cpu` (`lh5801`, `z80` or `lh5803`), `address`, `entry`, `after` (`stopOnEntry`, `call` or `none`; default `none`), `cleanStart` (default `true`), and the bank qualifiers `bank`, `me`, `pu`, `pv` |
| `listings` | Listings of code that isn't loaded (ROM): a path, or `{ path, source, cpu, bank, me, pu, pv }` |
| `symbols` | Symbol tables: a path, or `{ path, cpu, bank, me, pu, pv }` |
| `command` | A BASIC line typed to start the code: in place of the program's `CALL`, or, without a program, once the computer is set up |
| `boot` | `debug`: run the power-on under the debugger (the preset's hardware only; no `program` or `command`) |
| `reset` | `none` (default), `reset` or `allReset`: a Reset or a Cold boot that stops at the reset vector; the start-up runs from there under the debugger |
| `stopOnEntry` | Stop right after the debugger attaches (after a reset: before the first instruction) |

Numbers (`address`, `entry`, `bank`, `me`, `pu`, `pv`) follow the number
rule (`&`, `0x` or `$` for hex); an `entry` may also be a symbol.
