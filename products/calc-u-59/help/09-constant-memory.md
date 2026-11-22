# Constant memory

The TI-58C keeps its memory when it is switched off: the program, the
data registers, the partition and the display format survive, as on the
real calculator, whose static memory stayed powered. Calc-U-59 keeps
this **constant memory** in a file, `ti58c.memory.yaml`, at the top of
the save folder, together with the cue card that was fitted. The TI-59
and TI-58 have no constant memory.

## When the memory comes back

| You | The TI-58C |
|---|---|
| start Calc-U-59 on the TI-58C, or switch to it, **without a startup preset** for it | comes up with the memory of the file, as a real TI-58C switched on again, its cue card fitted again |
| start it, or switch to it, **with a startup preset**, or load a preset | starts from a cold boot, with what the preset gives; the file is not read |
| Reset, choose a module, attach or detach the PC-100C | keeps its memory in the calculator; the file is not read |
| Cold boot | its memory is zeroed, as after the batteries were out for a long time |

## When the file is written

The file follows a TI-58C that was built from it: at the latest half a
second after its memory or its cue card changes, and at once before the
calculator is replaced, when the app goes to the background or the
browser page is hidden, and when Calc-U-59 quits. A Cold boot of such a
TI-58C writes the zeroed memory to the file too.

A TI-58C built from a preset or a startup preset **never writes the
file**. So loading an example keeps your own constant memory for the
next time you switch to the TI-58C without a preset.

While a TI-59 or TI-58 runs, the file is neither read nor written; it
holds what the last TI-58C left.

## Where it lies

`ti58c.memory.yaml` lies in the save folder: in the app's iCloud
storage on the iPhone and the iPad, `Calc-U-59` in your home
folder on macOS, Windows and Linux, the browser's storage for the page in the
browser. It is not a medium: no picker lists it, and opening or dropping
it is refused with a notice. A damaged file, or a write that fails, is
reported with a notice. The format is in [Media files](10-media-files.md).
