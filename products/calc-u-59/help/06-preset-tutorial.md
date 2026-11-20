# Writing a preset, step by step

A preset is nothing more than plain text. This tutorial builds one up
from nothing, one part at a time, so you can see exactly what each line
changes. Every step below is also a real file you can download and load
in Calc-U-59 as you go; try each one before moving on to the next. What
every key and section means in full is in [Presets](06-presets.md) and in
the reference, [The preset's TI sections](10-preset-sections.md).

## Before you start

A preset is just a text file, so it is usually easiest to write and edit
it on a computer with a proper text editor, then get the finished file
onto whichever device runs Calc-U-59. With Apple devices, the simplest
way is to keep your presets in the **Calc-U-59** folder of iCloud Drive
on your Mac, the save folder the app uses for magnetic cards, and let
iCloud bring it to your iPhone or iPad; open a preset there with
**Preset** (Load Preset…). On Windows and Linux, keep them in any folder
and set **Settings ▸ Folders ▸ Preset folder** to it; to get them onto a
phone, any folder that syncs to it works.

None of that is needed, though: you can write and edit a preset directly
on an iPhone or iPad in any text editor that saves plain text, with no
computer at all. You can also read this tutorial on the device itself
and download each step's file straight to it; the same goes for every
other preset of the [TI-59 Software Collection](https://calc-u.org/59/software/).

## Watch out for .txt

When you save a preset with a plain text editor, it is easy to end up
with `my-preset.ti59.txt` instead of `my-preset.ti59`. Windows and macOS
both hide known file extensions by default, so the file still looks like
`my-preset.ti59` in Finder, Explorer or the iPhone's file picker.
Calc-U-59's preset picker accepts only `.ti59`, `.ti58` and `.ti58c`, so
a file with a hidden `.txt` tail shows up **greyed out and cannot be
chosen**. If that happens, turn on "Show file extensions" (Windows
Explorer) or "Show all filename extensions" (macOS Finder: Finder ▸
Settings ▸ Advanced) and rename the file, or rename it in a terminal with
`mv my-preset.ti59.txt my-preset.ti59`.

## Step 1: Setup

Every preset starts with the same few lines. `format-version: 2` says
which version of the format the file is written in; a file without it
is refused with a hint. `model`
picks the calculator you get (`TI-59`, `TI-58` or `TI-58C`), `slot-1`
plugs in one of the fourteen Solid State ROM modules by its two-letter
ID, here `ML` for the Master Library, and `printer: PC-100C` attaches the
printer. `preset-name` is the name the preset shows in lists, and
`description` the line under it.

A preset always builds its machine from scratch, from a cold boot: what
the file does not name is not there. Leave out `slot-1` and the slot is
empty; leave out `printer` and there is no printer.

```yaml
# TI-59 Preset File Tutorial — Step 1: setup
#
# This is the smallest useful preset file: it only says which calculator to
# build and what is plugged into it. Load it in Calc-U-59 right now and you
# get a TI-59 with the Master Library module and the PC-100C printer;
# nothing else has been set up yet.

# Every preset of this format starts with its version.
format-version: 2

# Set up the machine: the model, the Solid State ROM module and the printer.
# A preset builds its machine from scratch: leave out slot-1 and there is no
# module, leave out printer and there is no printer.
model: TI-59
preset-name: Tutorial step 1
description: Preset tutorial, step 1 - the machine
slot-1: { name: ML }
printer: PC-100C
```

[Download tutorial-1-setup.ti59](downloads/tutorial-1-setup.ti59)

Load this file and you already have a working calculator: nothing else
in a preset is needed. Everything from here on is added only because
this preset needs it.

## Step 2: Partition

`partition` sets the split between program memory and data registers,
the same trade-off the calculator's own key sequence **2nd Op 17**
controls. The value is written as `steps.registers`: `719.29` means step
719 is the last program step, so 720 steps go to the program and the
remaining 240 steps become 30 data registers (00 to 29) on a TI-59. Only
the number before the dot counts; the part after it is a note for the
reader. Leave the line out and the model's partition after a cold boot
holds: 479.59 on the TI-59.

```yaml
# TI-59 Preset File Tutorial — Step 2: partition
#
# Same as Step 1, with one addition: partition changes the split between
# program memory and data registers.

format-version: 2

# Set up the machine: the model, the Solid State ROM module and the printer.
model: TI-59
preset-name: Tutorial step 2
description: Preset tutorial, step 2 - the partition
slot-1: { name: ML }
printer: PC-100C

# Change the partition of the calculator
partition: 719.29   # 720 steps, 30 data registers (3 2nd Op 17)
```

[Download tutorial-2-partition.ti59](downloads/tutorial-2-partition.ti59)

This step does not change what you see when you load the file; there is
no program or cue card yet. The tutorial's program is short enough for
the default partition too; the line is here to show how it is written.

## Step 3: Cue card

`cue-card` is what turns a blank calculator into a labelled one: a card
in the window above the top row of keys, as the real calculator held a
card in its slot. `title` is the card's headline. `row-1` puts one line of
running text in place of the five labels of the A′–E′ row, handy for a
one-line instruction like this tutorial's "Calculates the sum of 1..N
the slow way"; `row-1-align` sets it left. `A` to `E` label the keys of
the lower row one by one; `\blank` on `D` and `E` makes those places
empty cells into which the label of `C` runs, so "Enter number, then A"
reads across three keys.

A preset's cue card is a plain card, written by hand. A magnetic card
brings its own cue card, which is described with the cards
([Magnetic cards](04-magnetic-cards.md)).

```yaml
# TI-59 Preset File Tutorial — Step 3: cue card
#
# Adds a cue-card section: the card shown once the preset loads, labelling
# the A-E keys the way a real card would.

format-version: 2

# Set up the machine: the model, the Solid State ROM module and the printer.
model: TI-59
preset-name: Tutorial step 3
description: Preset tutorial, step 3 - the cue card
slot-1: { name: ML }
printer: PC-100C

# Change the partition of the calculator
partition: 719.29   # 720 steps, 30 data registers (3 2nd Op 17)

# The cue card that is shown after loading the preset, above the top row of
# keys. row-1 is one line of text in place of the A'-E' labels; A to E label
# the keys below it, and \blank lets the label before it run on.
cue-card:
  title: TI-59 Preset File Tutorial
  row-1: Calculates the sum of 1..N the slow way
  row-1-align: left
  A: Sum 1..N
  C: Enter number, then A
  D: \blank
  E: \blank
```

[Download tutorial-3-cuecard.ti59](downloads/tutorial-3-cuecard.ti59)

Load this file now and the card appears at once. Nothing on it does
anything yet, because there is still no program behind the **A** key.
All the keys of a cue card, the symbols such as `\sqrt` and how the
labels line up are in [Cue cards](09-cue-cards.md).

## Step 4: Program

`program` fills the program memory with keycodes: the codes the
calculator itself shows while you record a program by hand, not the
keys' positions. Under `keycodes: |` each line is either a run of
two-digit keycodes (the compact notation used here, ten steps per line,
going on where the line before stopped) or a listing line as the printer
lists a program, with the step number first. Anything after `#` is a
comment, so the step numbers at the end of the lines are for the reader.
Only the steps listed are written.

This program sums a number down to 1: **A** stores the number you
entered, clears the total, and loops with **Dsz**, adding the counter
into the total until it reaches zero; then it shows the total and stops
with **R/S**.

```yaml
# TI-59 Preset File Tutorial — Step 4: program
#
# Adds a program section: the calculator's program memory, written as
# keycodes. This program sums the numbers from whatever you enter down to
# 1: press a number, then A.

format-version: 2

# Set up the machine: the model, the Solid State ROM module and the printer.
model: TI-59
preset-name: Tutorial step 4
description: Preset tutorial, step 4 - the program
slot-1: { name: ML }
printer: PC-100C

# Change the partition of the calculator
partition: 719.29   # 720 steps, 30 data registers (3 2nd Op 17)

# The cue card that is shown after loading the preset, above the top row of
# keys. row-1 is one line of text in place of the A'-E' labels; A to E label
# the keys below it, and \blank lets the label before it run on.
cue-card:
  title: TI-59 Preset File Tutorial
  row-1: Calculates the sum of 1..N the slow way
  row-1-align: left
  A: Sum 1..N
  C: Enter number, then A
  D: \blank
  E: \blank

# The program memory, as keycodes. The format is flexible: here we write ten
# steps per line (the compact notation) with a comment for the step numbers.
# The loader reads the two-digit keycodes and ignores the comments.
program:
  keycodes: |
    43 00 44 01 97 00 00 00 43 01   # 000-009
    91 76 11 42 00 25 42 01 61 00   # 010-019
    00                              # 020
```

[Download tutorial-4-program.ti59](downloads/tutorial-4-program.ti59)

Load this file and press **5**, then **A**, as the cue card tells you:
the program runs now and shows 15.

## Step 5: Registers

`data` preloads data registers, one `NN: value` per line, as decimal
numbers at the calculator's full precision of 13 digits. The value
here, `3.000000000004` in register 29, is chosen because it is awkward
to key in by hand on the real calculator (you would need **SUM** and a
division to land on it exactly); a preset just states the value. Only
the registers listed are written; the others stay zero.

```yaml
# TI-59 Preset File Tutorial — Step 5: registers
#
# Adds a data section: data registers preloaded with decimal numbers, at
# the full TI-59 precision.

format-version: 2

# Set up the machine: the model, the Solid State ROM module and the printer.
model: TI-59
preset-name: Tutorial step 5
description: Preset tutorial, step 5 - the data registers
slot-1: { name: ML }
printer: PC-100C

# Change the partition of the calculator
partition: 719.29   # 720 steps, 30 data registers (3 2nd Op 17)

# The cue card that is shown after loading the preset, above the top row of
# keys. row-1 is one line of text in place of the A'-E' labels; A to E label
# the keys below it, and \blank lets the label before it run on.
cue-card:
  title: TI-59 Preset File Tutorial
  row-1: Calculates the sum of 1..N the slow way
  row-1-align: left
  A: Sum 1..N
  C: Enter number, then A
  D: \blank
  E: \blank

# The program memory, as keycodes. The format is flexible: here we write ten
# steps per line (the compact notation) with a comment for the step numbers.
# The loader reads the two-digit keycodes and ignores the comments.
program:
  keycodes: |
    43 00 44 01 97 00 00 00 43 01   # 000-009
    91 76 11 42 00 25 42 01 61 00   # 010-019
    00                              # 020

# Register content. The full TI-59 precision is supported. On the real
# hardware the number below cannot be keyed in directly; one would need SUM
# and a division to set it up.
data:
  29: 3.000000000004
```

[Download tutorial-5-registers.ti59](downloads/tutorial-5-registers.ti59)

This step does not change what the program does, since register 29 is
not part of the sum; it is here so that the next step has something
worth recalling with **RCL**. Press **RCL 2 9** and the display shows
`3.`: the display has too few places for the fraction.

To see that the value loaded, open the debug panel and look at the
registers of its CALCULATOR tab ([Debugger](07-debugger.md)). On a Mac,
on Windows and Linux, on an iPad held sideways and on an iPhone Duo
opened flat the panel is beside the calculator; on an iPhone, and on an
iPad held upright, it is a page of its own that you turn to with the
arrow in the corner, once **Settings ▸ Layout ▸ Debug Page in Portrait** is on.

## Step 6: Keys

`keys` is the odd one out: everything above describes the calculator,
but this section acts. It is a list of steps, one per line, each
`- verb: value`, carried out in order once everything above has been set
up. `- key:` presses keys, written by their key IDs: the label on the
key, such as `5`, `A`, `RCL` or `=`, separated by spaces. Key IDs are
not the keycodes of Step 4: the digit 5 is the key ID `5` here and the
keycode `05` in a program. Mixing the two up is the most common mistake
in a preset, and the reference lists both ([The preset's TI
sections](10-preset-sections.md)).

A preset's steps run at full speed unless it says otherwise, so this one
starts with `- regular-speed:` to let you watch the display. Each step
waits until the calculator is idle before the next one starts, so the
sum has finished before the next step. `- wait: 2s` pauses for two
seconds so you can read the result.

```yaml
# TI-59 Preset File Tutorial — complete
#
# This file shows how a preset is put together. The preset tutorial in the
# help (https://calc-u.org/59/, chapter Presets) builds it up step by step.

format-version: 2

# Set up the machine: the model, the Solid State ROM module and the printer.
model: TI-59
preset-name: Preset tutorial
description: Preset tutorial, complete - sums 1..5, then recalls R29
slot-1: { name: ML }
printer: PC-100C

# Change the partition of the calculator
partition: 719.29   # 720 steps, 30 data registers (3 2nd Op 17)

# The cue card that is shown after loading the preset, above the top row of
# keys. row-1 is one line of text in place of the A'-E' labels; A to E label
# the keys below it, and \blank lets the label before it run on.
cue-card:
  title: TI-59 Preset File Tutorial
  row-1: Calculates the sum of 1..N the slow way
  row-1-align: left
  A: Sum 1..N
  C: Enter number, then A
  D: \blank
  E: \blank

# The program memory, as keycodes. The format is flexible: here we write ten
# steps per line (the compact notation) with a comment for the step numbers.
# The loader reads the two-digit keycodes and ignores the comments.
program:
  keycodes: |
    43 00 44 01 97 00 00 00 43 01   # 000-009
    91 76 11 42 00 25 42 01 61 00   # 010-019
    00                              # 020

# Register content. The full TI-59 precision is supported. On the real
# hardware the number below cannot be keyed in directly; one would need SUM
# and a division to set it up.
data:
  29: 3.000000000004

# Keys pressed after everything above has been set up. As a sample we
# calculate the sum of 1..5 with the program above. Keys are written by their
# key IDs, the labels on the keys, not by keycodes: the digit 5 is the key
# ID 5, while in the program above the same digit is the keycode 05.
keys:
  - regular-speed:      # a preset runs at full speed unless it says otherwise
  - key: 5 A            # 5, then A: the program runs and shows 15
  - wait: 2s            # wait 2 seconds, to see the result
  - key: RCL 2 9        # recall register 29
  - wait: 2s            # the display shows only "3.": too few digits for the fraction
  - key: "- 3 ="        # after subtracting 3 we see the fraction
```

[Download tutorial.ti59](downloads/tutorial.ti59)

This is the finished file. Loading it presses **5 A** to run the sum of
Step 4 (15), waits, recalls register 29 of Step 5, waits again, and
subtracts 3, which leaves the fraction: the display shows `4.-12`. It
uses, in order, every part this tutorial added. The last line is in
quotes because a value that starts with `- ` would otherwise be read as
a list.

## Where next

That covers enough to write your own preset from scratch.
[Presets](06-presets.md) describes every part in one place: the magnetic
cards a preset can feed, the steps that check the display, the debugger's
steps. The full grammar, with every key ID, is in the reference ([The
preset's TI sections](10-preset-sections.md)). For more presets to read
and load, many of them long programs from the calculator's day, see the
[TI-59 Software Collection](https://calc-u.org/59/software/).
