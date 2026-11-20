# Settings of Calc-U-59

The settings dialog, the settings file, where it lies, and the keys every
product of the family has are described in
[Settings](../../../library/help/08-settings.md). This page lists what
Calc-U-59 adds: its file's name, the names it gives the family's keys,
and its own keys.

Calc-U-59's settings file is `Calc-U-59.ini`. **Settings…** (in the
application menu on macOS, in the File menu on Windows, Linux and in the
browser's wide layout, the Settings button elsewhere) opens the settings;
a change applies at once. The settings say nothing about the module or
the printer: the control bar and presets choose those, and a startup
preset sets them at a start.

## The dialog

| Section | Setting | Where |
|---|---|---|
| General | **Startup model**: "Last used", TI-59, TI-58, TI-58C; and the version | everywhere |
| Startup presets | one row per model: "None", the bundled startup preset of that model, or (Windows, Linux) a file | everywhere |
| Storage | **Save folder** | Windows, Linux |
| Folders | the **preset folder**, where Load Preset… starts | Windows, Linux |
| Tracing | **Save Location**, where traces go: Local or iCloud, on macOS also Custom Folder (macOS, iPhone, iPad); **Trace folder** (macOS, Windows, Linux); **Maximum file size** in MB (everywhere but the browser's portrait layout) | as named |
| Display | **Segment Style**: "Modernized" or "Classic" | macOS, iPhone, iPad, Windows, Linux |
| Keyboard | **Keyboard Feedback**: "Off", "Haptic" (iPhone, iPad), "Click Sound" | iPhone, iPad; the browser's portrait layout |
| Layout | **Debug Page in Portrait** | iPhone, iPad |

On the iPhone and the iPad the settings are a sheet whose button is
Done. On iOS there is no About; the version is in General.

## Startup presets

`[preset] <model>` of the family has one key per model here:

- `[preset] TI-59`: the startup preset of the TI-59; empty: none.
- `[preset] TI-58`: the startup preset of the TI-58; empty: none.
- `[preset] TI-58C`: the startup preset of the TI-58C; empty: none. A
  TI-58C built from a startup preset starts from a cold boot, and the
  constant memory file is neither read nor written for it
  ([Constant memory](09-constant-memory.md)).

Without a startup preset a model starts with the Master Library module
and no printer.

## Display and keyboard

- `[display] led-style`: how the display draws its segments:
  `modernized` (the default) or `classic`.
- `[keyboard] feedback`: what a key press gives back on a touch screen:
  `off` (the default), `haptic` or `click` (iPhone and iPad; the browser
  in its portrait layout).

## Folders

`[folders] <kind>` of the family has two kinds here. Each is where a
file dialog starts: the folder last used (written by the app), or a fixed
folder given as `fixed:<folder>`.

| Key | Where it applies |
|---|---|
| `[folders] presets` | **File ▸ Load Preset…** (in the dialog: the preset folder) |
| `[folders] asm-overlays` | **Select File** of the ASM overlay ([Debugger](07-debugger.md)) |

## Window

`[window] <key>` of the family has these keys here, all written by the
app:

- `[window] x`, `[window] y`: the window's left and top edge (Qt6).
- `[window] width`, `[window] height`: the window's size (Qt6).
- `[window] calculator-width`, `[window] printer-width`,
  `[window] debug-width`: the widths of the calculator, the paper panel
  and the debug panel (Qt6).
- `[window] portrait-debug-page`: Debug Page in Portrait, `true` or
  `false` (iPhone, iPad); `false` by default.

The Qt6 window comes back at its last normal size and place, never
maximised or full screen.
