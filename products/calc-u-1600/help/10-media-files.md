# Media files

Calc-U-1600 has three kinds of media: the battery-backed module, the
floppy disk and the cassette tape. Each is a file you can keep, copy,
import and export. How media are used (templates and instances, Name &
Save, inserting and ejecting) is in the [shared Media chapter](../../../library/help/04-media.md)
and in [Media](04-media.md); this page gives the files themselves.

| Medium | File | Version | Its place |
|---|---|---|---|
| Battery-backed module | `.card.yaml` | 1 | a module slot |
| Floppy disk | `.floppy.yaml` | 2 | the CE-1600F drive of the CE-1600P (PC-1600) |
| Cassette tape | `.tape.yaml` with a `.wav` | 1 | the cassette interface of a CE-150 or CE-1600P |

Common to all three:

- A file with `template: true` is a **template**: the product never
  writes it. The bundled media are templates. A file without that key is
  an **instance**, saved in place after each change.
- Name & Save copies a template (or an instance) into a new instance
  under the name you give. A preset's `saveas:` step can save a copy as
  a template instead ([Presets](06-presets.md)).
- Instances lie in the save folder; tapes lie in the tape folder,
  `Tapes` in the save folder unless the settings name another one
  ([Settings](08-settings.md)).
- A floppy disk written by Calc-U-1600 0.8 (version 1) is refused with a
  hint until you update it by hand: set `format-version: 2` and rename
  the sides `a:` and `b:` to `0:` and `1:`. Module definitions of 0.8 are
  read as they are.
- A name with a character a file name cannot hold (`/`, `:`) is refused.

## Battery-backed module

A battery-backed module is a module definition with `battery: true`
([Module definition format](10-module-definition-format.md)). Its saved
instance is the same kind of file, a `.card.yaml`, written from the
template it came from:

- everything but the content, the name and the template mark is kept as
  the template has it: the addressing, the hosts, the description;
- `module-name:` is the name you gave in Name & Save;
- the region's `initial-content:` holds what the module's memory held,
  as `addressed-hex`;
- there is no `template:` line (unless a preset's `saveas:` saved it as
  a template);
- for a module with a write-protect switch (the CE-161), `write-protect:`
  holds the switch's position;
- a comment at the top says from which module the instance was made and
  when.

A preset plugs a saved instance in by its name, `slot-1: { name: … }`,
like a bundled module: the bundled modules are searched first, then the
save folder.

## Floppy disk

A disk for the PC-1600's CE-1600F drive, with two sides, 0 and 1, used
one at a time. The file holds the raw bytes of both sides and nothing
else: the files on a side are kept in the disk's own file system, which
the CE-1600P's ROM reads and writes as on real hardware.

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
| `format-version` | yes | `2` (version 1, of Calc-U-1600 0.8, named the sides `a` and `b`) |
| `disk-name` | yes | The name tag, shown in the disk picker; not empty, without `"` or a line break |
| `description` | no | One line, the picker's tooltip. Name & Save and every later write keep it |
| `template` | no | `true`: a template, never written |
| `saved` | no | The time of the last write (UTC, ISO 8601); for information |
| `sides` | yes | Exactly the keys `0` and `1` |
| `sides.0`, `sides.1` | yes | Exactly `encoding: addressed-hex` and `bytes` |
| `…bytes` | yes | The side's 65536 bytes, every byte covered |

**A side.** Each side is 65536 bytes: 16 tracks of 8 sectors of 512
bytes. The drive reads one side, and you turn the disk to use the other,
so each side is a volume of its own. A byte's offset in a side is the
logical sector × 512, where the logical sector is track × 8 + sector,
sectors counted from 0. The boot sector is at `$0000`, the FAT at
`$0200`, its copy at `$0400`, the directory at `$0600` to `$0BFF`, and
the first data cluster at `$0C00`.

**`addressed-hex`.** Each line is either `$XXXX:` followed by up to 16
bytes in hex, placed from that offset on, or `$XXXX: XX...` alone, which
repeats the byte `XX` from that offset up to the next line's offset (or
to the end of the side). The lines cover the side without gap or
overlap.

**Writing.** The product rewrites the whole file after the machine wrote
to the disk, with `saved` set to the time of the write. While a disk is
in the drive, a change another program makes to its file can be lost:
eject the disk before you edit its file.

The bundled templates are **Blank** (unformatted: `INIT` it before use)
and **Formatted** (both sides formatted and empty).

## Tape definition

A tape for the cassette interface of a CE-150 or a CE-1600P. The tape
itself is sound, in a `.wav` file; the tape definition gives it a name
and a description, like any other medium, and names the sound file.

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
| `tape-name` | yes | The name tag, shown in the tape picker; without `"` or a line break |
| `description` | no | One line about the tape |
| `template` | no | `true`: a template; its sound file is never written either |
| `audio` | yes | The sound file, by `file:` only, relative to the tape definition |
| `position` | no | Reserved for later use; must be `0`, the default |

- Recording onto an instance writes into its sound file, as a mono
  16-bit WAV at 48000 Hz, each time the motor stops and when the tape is
  ejected. A write that fails is shown as a notice.
- Name & Save copies the definition and its sound file into the tape
  folder, side by side.
- To use a WAV you already have as a tape, write a tape definition next
  to it that names it; the definition may name it by any relative path.
- Several recordings on one tape are part of the sound, not of the
  definition.
- A `.wav` on its own is not a tape but a program file: opened or
  dropped, it is loaded as a program ([BASIC programs](09-basic-programs.md)).

The bundled template is **Empty**, a blank cassette.
