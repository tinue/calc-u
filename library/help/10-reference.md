# Reference

The rules every product of the family shares: the preset format, the
settings file, and the terms this help uses. The product's sections,
keys, file formats and keyboard map are in its part:
[Reference of this product](chapter:10).

## The preset format

A preset is written in **preset format version 2**. How presets are
used is in [Presets](06-presets.md).

### Syntax

Presets, and the product's other files, are written in one simplified
form of YAML:

- UTF-8 text; a `\r` before a line end is dropped.
- Mappings by indentation, with spaces only: a **tab anywhere is an
  error**. Write 2 spaces per level.
- Lists of items `- item`; an item is a value, a `[a, b]` or
  `{ key: value }` on one line, or a mapping (`- key: value`, the further
  keys aligned under the first).
- Values bare, `"double-quoted"` or `'single-quoted'`, with no escapes:
  a quoted value cannot hold its own quote character.
- `key: |` starts a block of lines; their common indentation is
  removed.
- `#` starts a comment, as a whole line or after a value with a space
  before it, outside quotes and brackets. The values of `type:` and
  `expect:` are the exception: they run to the end of the line, `#` and
  quotes included.
- Numbers are decimal; `&`, `0x` or `$` before them makes them hex.
  Booleans are `true` and `false`.
- Keywords (keys and fixed values) are lower case with hyphens:
  `format-version`, `full-speed`. Fixed values are read in any case.
  Names of models, variants and peripherals are read in any case; their
  hyphen is required.
- **An unknown key is an error**, at every level.
- Not supported: anchors and aliases, tags, `>` blocks, several
  documents in one file.

A file that does not follow these rules, or the rules below, is refused
as a whole; nothing of it is used.

### Header keys

Header keys describe the preset and its machine. They act on the whole
file wherever they stand, and each appears at most once.

| Key | Value |
|---|---|
| `format-version` | `2`. Required. A file without it or with a lower value was written for an earlier release and is refused; a higher value, for a newer release |
| `model` | Required. A model of the product, optionally followed by `:` and one of its variants: `<model>:<variant>`. Without a variant, the model's default one |
| `preset-name` | The name shown in menus and lists, at most 18 characters, different from the other presets of the same model. Without it, the file name without its extension |
| `description` | One line for lists, menus and tooltips |
| `timeout` | The longest the preset waits for an item to finish, in seconds of emulated time. Default `3600` |
| one key per place | named after the place of the model (`slot-1`, `printer`, …): what is plugged in there. A place no key names stays empty |

The places of each model, the peripherals that fit them and the
product's further header keys are in the product's part.

### References to files and names

Where a key names a module, a medium, a program file or a folder, it
says how, with exactly one of these keys, plus the options its owner
allows (`side:`, …):

| Key | Names |
|---|---|
| `file: <path>` | a file, by its path. A relative path starts at the folder of the preset; `~/` is your home folder |
| `name: <name>` | the item whose name tag is `<name>`, in any case |
| `folder: <path>` | a folder, by its path |

A bare value is an error: `slot-1: { name: Mine }`, not `slot-1: Mine`.

### Sections

Top-level keys that are not header keys are **sections**. They are
carried out from top to bottom, each to its end before the next starts,
and may appear more than once.

| Section | Holds |
|---|---|
| `keys` | a list of steps |
| `program` | a program to put into memory |

A product may add sections of its own; they are in its part. A section
of another product is an error.

### Steps

A step is an item `- verb: value` of a `keys:` section, or `- verb:`
for a verb without a value. Each step starts only when the item before
has finished, unless a `skip-wait:` stands between them.

| Step | Value | What it does |
|---|---|---|
| `key` | key IDs, separated by spaces | presses and releases each key in turn, each when the machine is ready for it. A word that is not a key ID is an error |
| `wait` | a duration with its unit: `2s`, `1.5s`, `500ms` | lets that much emulated time pass after the item before has finished; while the machine is paused, the computer's time |
| `skip-wait` | none | the item before is not waited for. Not the first or the last item |
| `let-it-finish` | none | the machine runs on until it is idle, then the preset ends. Only as the last item |
| `regular-speed`, `full-speed` | none | the items after it run at that speed |
| `expect` | a text, to the end of the line | fails the preset unless the display shows it |
| `not-expect` | a text, to the end of the line | fails the preset if the display shows it |
| `screenshot` | a file name, without `/` or `\` | saves a picture of the display into the save folder; on the Web, as a download |
| `trace` | a file name, without `/` or `\`, or `off` | starts or ends a trace ([Debugger](07-debugger.md#traces)) |
| `elapsed-time` | `start` or `report` | notes the emulated time, or writes the time since then (or since the preset started) to the log as `m:ss.mmm` |
| `cut-paper` | none | cuts the paper of the printer that is plugged in; without one, nothing happens |
| `saveas` | `{ place: …, as: live\|template, name: … }` or `file:` instead of `name:` | Name & Save of the medium or module at a place ([Media](04-media.md)) |
| `pause`, `pause-on-start`, `step`, `resume` | as in [Debugger](07-debugger.md#debugger-steps-in-a-preset) | drive the debugger, where the product defines them |

**When an item has finished:** a `key:` when its last press is done and
the machine is idle; a section when its content is written (machine code
that starts itself, when the code has returned); a `wait:` when its
duration has passed; every other step when it has taken effect. A paused
machine is not idle.

Steps for media and the product's own steps are in its part. A keyword
that a product or model does not have refuses the preset when it is
read.

### Example

```yaml
# A comment
format-version: 2
model: <model>
preset-name: Demo
description: Presses two keys, waits, checks
slot-1: { name: <module> }
keys:
  - regular-speed:
  - key: <key> <key>
  - wait: 500ms
  - full-speed:
  - expect: <text>
  - let-it-finish:
```

## The settings file

The settings file is `<Product>.ini`, an INI file
([Settings](08-settings.md) says where it lies and what each key means):

- UTF-8 without a byte order mark; either line ending.
- `[section]` lines start a section; `key = value` lines give a value.
  The value is the rest of the line after `=`, with the spaces around it
  removed, **without quotes or escapes**.
- A line starting with `#` or `;` is a comment.
- Section and key names are lower case with hyphens, except the keys of
  `[preset]`, which are model names as the product writes them. Names
  are matched in any case.
- A key given twice: the last one counts.
- Booleans are `true` and `false`. A path is absolute; a leading `~`
  stands for your home folder.
- The first line, before any section, is `format-version = 1`; the
  product writes it.

```ini
format-version = 1

[startup]
# The model at start: last (the model used last) or a model's name.
model = last

[trace]
max-file-size-mb = 50
```

## Glossary

The terms this help uses, each with one meaning.

| Term | Meaning |
|---|---|
| Product | One separately released member of the Calc-U family, with its own releases and help |
| Family | All Calc-U products together |
| Model | A real device a product reproduces and you pick |
| Variant | A revision of a model or a peripheral that you can choose and that changes behaviour, mostly a ROM version |
| Machine | One emulated model while it runs, with whatever is plugged into it. Switching the model builds a new machine |
| Place | Where a peripheral or a module plugs into a machine: a module slot, the printer's place, … |
| Peripheral | Emulated hardware that plugs into a machine: a printer, an interface, a module |
| Printer | A peripheral that prints or plots |
| Module | A peripheral that holds memory and plugs into a module slot |
| Module slot | A place of a machine for modules |
| Medium | A removable item that holds your data and can be named and saved: a floppy disk, a cassette, a magnetic card, a battery-backed module |
| Template | A medium bundled with the product, which is never written, or one you made a template |
| Instance | A medium that is not a template; it saves itself in place after each change |
| Name & Save | Giving a template a name and saving it as an instance |
| Insert, Eject | Putting a medium into its place on a machine; taking it out |
| Save folder | The folder where a product keeps your instances and other files |
| Preset | A file that sets up a machine: the model, what is plugged in, programs and scripted input |
| Example | A preset bundled with the product that shows what it can do |
| Startup preset | The preset a model is built from when the product starts with it or you switch to it |
| Startup model | The model the product starts with: a fixed one or the last used |
| Full speed | Running a machine as fast as the computer allows |
| Regular speed | Running a machine at the real device's speed |
| Emulated time | The time a machine has run, counted by the machine itself |
| Idle | A machine that waits for you and is busy with nothing else: at its prompt, or a program waiting for a key |
| Reset | Restarting a machine with its memory kept |
| Cold boot | Starting a machine with all memory empty, as after a long battery removal |
| Power cycle | Switching a real device off and on; what survives depends on the model |
| Key | A key of the emulated machine |
| Host key | A key of your computer's keyboard |
| Keyboard map | Which host key presses which key, per model |
| Typing | Feeding text into a machine as key presses |
| Debugger | Pausing, stepping and breakpoints on a running machine |
| Pause, Resume | The debugger stopping a running machine; letting it run on |
| Trace | A record of every instruction a processor carries out, saved as a trace file |
| Settings | Your preferences for a product, kept in its settings file |
| Help | A product's user guide: this text |
| Shared part | The part of a chapter written once for the whole family |
| Product part | The part of a chapter that belongs to one product |
