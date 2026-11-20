# Presets

A **preset** is a text file that sets up a machine: the model, what is
plugged in, then programs and scripted input. Load one to get a machine
into a known state in one go: a game ready to play, a program typed in
and started, a test that checks what the display shows. The examples the
product bundles are presets, and so are the startup presets.

The product's own sections, keys and examples are in its part of this
chapter: [Presets of this product](chapter:6). Every key and step is
listed in [the reference](10-reference.md#the-preset-format).

## Loading a preset

- **File ▸ Load Preset…** opens a preset file (Ctrl+O, ⌘O on macOS, in
  the desktop frontends).
- **File ▸ Examples ▸** lists the examples the product bundles, one
  submenu per group. In some frontends Examples is a menu of its own.
- Dropping a preset file on the window, or opening it with the product
  from the computer's file manager, loads it too
  ([Media](04-media.md) describes how a file is opened as what it is).
- In a window without a menu bar, the Preset button loads a preset
  file, and Examples is behind the More button
  ([Getting started](02-getting-started.md#without-a-menu-bar)).

A load always builds a **new machine**: the machine that ran before is
gone, with its memory. The new machine is built from a cold boot with
the model and exactly what the preset names; a place the preset does not
name stays empty. Then the preset's sections are carried out from top to
bottom.

While a preset runs:

- the machine runs at full speed, unless the preset asks for regular
  speed; after half a second "Loading…" is shown;
- your key presses on the machine are dropped, and controls that would
  start something else are disabled;
- Reset, Cold boot and a model switch end the preset at once, without
  an error. This is the way out of a preset that waits for ever.

When the preset is done, the machine runs on at regular speed and is
yours.

## When a preset is refused

The whole file is read and checked before anything is built. If
anything is wrong (an unknown key, a value a key does not take, a key a
model does not have, a peripheral that does not fit its place, a file it
names that cannot be read), nothing is built and the running machine
stays as it is. A notice gives the first error with its line number.

A file without `format-version`, or with another version than 2, is
refused; a file from a newer release is refused as such. A preset
written by an earlier release is converted by hand, as
[the product's part of this chapter](chapter:6) describes.

If a step fails while the preset runs, or does not finish within the
preset's `timeout:`, the preset stops there: the steps after it are not
carried out, the machine stays as it is, and a notice gives the line and
the reason.

## A first preset

```yaml
format-version: 2
model: <model>
preset-name: My setup
description: One line for menus and tooltips
keys:
  - key: <key> <key>
  - wait: 2s
  - expect: <text>
```

- `format-version: 2` and `model:` are required. The model may carry a
  variant after a colon (`<model>:<variant>`); without it the model's
  default variant is built.
- `preset-name:` is the name shown in menus (at most 18 characters);
  without it, the file name without its extension is shown.
- `description:` is one line for lists and tooltips.
- Each place of the model that holds a peripheral or a module has a key
  of its own, named after the place (`printer:`, `slot-1:`, …). Which
  places a model has is in the product's part.
- `keys:` is a section: a list of **steps**, carried out one after the
  other.

The file extension does not choose the model; `model:` does.

## Steps

A step is one line `- verb: value` in a `keys:` section; a step without
a value is written `- verb:`. The most used ones:

| Step | What it does |
|---|---|
| `key: <key> …` | presses and releases each key in turn; key IDs are separated by spaces |
| `wait: 2s` | lets that much time pass (`2s`, `1.5s`, `500ms`; the unit is required) |
| `regular-speed:` / `full-speed:` | the steps after it run at regular or at full speed |
| `expect: <text>` | fails the preset unless the display shows the text |
| `not-expect: <text>` | fails the preset if the display shows the text |
| `screenshot: <file name>` | saves a picture of the display |
| `trace: <file name>` / `trace: off` | starts or ends a trace ([Debugger](07-debugger.md#traces)) |
| `elapsed-time: start` / `report` | measures emulated time and writes it to the log as `m:ss.mmm` |
| `cut-paper:` | cuts the paper of the printer that is plugged in |

Steps for media (inserting a medium, Name & Save) and the product's own
steps are listed in the [reference](10-reference.md#steps) and the
product's part.

## Waiting

A preset waits on its own: **each item starts only when the one before
has finished**. A key press has finished when the machine is idle
again: a key that starts a program finishes when the program stops or
waits for you (an INPUT, a key).
You rarely need `wait:`.

| You want | Write |
|---|---|
| a program to run to its end before the next step | nothing: the next step waits |
| to watch part of the preset happen | `regular-speed:` before it, `full-speed:` after it |
| a pause so a display can be seen, or a running program to go on for a while | `wait: 2s` |
| to press a key while a program still runs (to stop it, say) | `skip-wait:` after the step that started it |
| the last program to run to its end before the preset ends | `let-it-finish:` as the last step |

- The **last step** is carried out and not waited for: the preset ends at
  once, and what the step started runs on at regular speed, under your
  control. A game started by the last key is yours to play.
- `skip-wait:` may not be the first or the last item; `let-it-finish:`
  may only be the last.
- The header key `timeout:` (seconds of emulated time, default 3600) is
  the longest the preset waits for an item to finish. A program that
  never stops either runs as the last item or is followed by
  `skip-wait:`.
- At full speed, `wait:` passes as fast as the computer allows; the time
  is the machine's emulated time either way.

## Checking the display

`expect:` and `not-expect:` turn a preset into a test. They check after
the item before has finished: after the key that runs a program, when
the program has ended. If the check fails, the preset stops and the
notice holds the text and what the display showed. How the display is
compared (a line of text, a number) is the product's: see its part.

## Startup presets

A **startup preset** is the preset a model is built from when the
product starts with that model, or when you switch to that model. Set
one per model in **Settings ▸ Startup presets**: "None", the startup
presets the product bundles for that model, or (in the desktop
frontends) any preset file with "Choose…". It is kept in the settings
file under `[preset]` ([Settings](08-settings.md)).

- Without a startup preset, a model is built as the product's built-in
  default machine.
- If the startup preset cannot be loaded (the file is gone, it is
  cannot be read, it is for another model), the built-in default machine is
  built and a notice says why.
- If it is read without an error but fails while it runs, the machine
  stays as far as the preset built it, and a notice says why.
- Switching the variant of the running model loads no startup preset.

## Examples

The product's examples are bundled with it and listed under **File ▸
Examples ▸**, in groups. They are presets like any other: open one to
see how it is written, copy it, and change it. The product's part lists
them.
