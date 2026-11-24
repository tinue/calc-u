# FAQ

## Where do preset files live?

Anywhere: they are text files ending in `.ti59`, `.ti58` or `.ti58c`.
On a computer, **File ▸ Load Preset…** opens them (**Settings ▸ Folders**
sets the folder it starts in), and you can drop one
on the window or, on Windows, double-click it. On the iPhone and the iPad
**More ▸ Load Preset…** opens the file picker. See [Presets](06-presets.md).

## Are there examples of presets?

Yes: **Examples ▸** lists the bundled ones, the [preset
tutorial](06-preset-tutorial.md) builds one from scratch, and the
[TI-59 Software Collection](https://calc-u.org/59/software/) has many more
to download.

## I wrote a preset, but the app shows it greyed out. Why?

Almost always a hidden `.txt` ending: some editors save `my-preset.ti59`
as `my-preset.ti59.txt`, and Finder or Explorer hide the ending. The
preset picker accepts only `.ti59`, `.ti58` and `.ti58c`. Show the file
endings (Explorer: "Show file extensions"; Finder: Settings ▸ Advanced ▸
"Show all filename extensions") and rename the file
([Preset tutorial](06-preset-tutorial.md)).

## Where do magnetic cards live?

In the save folder, as `<name>.magnetcard.yaml`: the app's iCloud storage
on the iPhone and the iPad (iCloud Drive ▸ Calc-U-59), or
its local folder when iCloud is off; `Calc-U-59` in your home folder on
macOS, Windows and Linux, unless **Settings ▸ Storage ▸ Save folder** names
another; the browser's storage in the browser. Use the **Crd** buttons to
arm one ([Magnetic cards](04-magnetic-cards.md)).

## Where is the TI-58C's memory kept?

In `ti58c.memory.yaml` in the save folder, next to the cards. It is
written and read by itself when the TI-58C runs without a startup preset,
standing in for the memory the real calculator kept powered
([Constant memory](09-constant-memory.md)).

## How do I label the keys A–E and A′–E′?

Add a `cue-card:` section to a preset and fill in `A` to `E` and `"A'"`
to `"E'"`; a magnetic card carries its own in its card file
([Cue cards](09-cue-cards.md)).

## Why does the module not stay as I chose it?

The module is part of the machine you build, not a setting: Calc-U-59
starts with the Master Library unless a startup preset names another
module. Set a startup preset for the model to start with your module
([Solid State ROM modules](09-modules.md)).

## Can I see what the emulator does inside?

Yes: the debug panel. CALCULATOR shows the program and the registers,
CPU the processor's instructions, RUN LOG the log and the TRACE button
([Debugger](07-debugger.md)).

## How do I get a trace file, and what can I do with it?

In the debug panel's RUN LOG tab, turn TRACE on, do what you want to
record, turn it off. The file (`CALCU59_TRACE.bin`, `CALCU58_TRACE.bin` or
`CALCU58C_TRACE.bin`) is in the trace folder, or offered to download in
the browser. `tools/read_trace.py` of the source repository turns it into
readable disassembly ([Traces](07-debugger.md#traces)).

## Why does switching to the CPU tab reset the heat map?

The CPU tab only counts while it is in view, so the heat map starts over
when you switch to it; it also starts over at a Reset and a model switch.

## How can I step through the reset routine?

In the CPU tab press FREEZE ON START, then **Machine ▸ Cold boot** or
Reset (on the iPhone and the iPad: Cold boot behind the More button, or
the Reset button). The calculator pauses at the first instruction of the
ROM's start-up; STEP walks through it.

## After FREEZE ON START in the CPU tab the key seems lost. Why?

The ROM sees a key press only for a very short time, which has passed
when you resume. Use a trace instead: it records without stopping the
calculator.

## What are the "assembly" examples for?

They are ASM overlays: small programs for the calculator's processor
that the debug panel's ASM OVERLAY box puts over the ROM (Select File,
then Run; Clear removes it). See [ASM overlay](10-asm-overlay.md).

## Is there a faster way to read long printer output?

Yes: Copy or Cut the paper and paste it into a text editor
([The PC-100C printer](05-printer.md)).
