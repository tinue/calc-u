# TO DO
Unsorted collection of ideas, bugs, observations etc.

## Core emulation
- **Keep memory on model switch** - A switch starts a new machine, so the RAM is lost (except the TI-58C's constant memory). Needs memory kept per model.
- **Emulation speed control** — a multiplier to run the emulator faster or slower than real time.
- **Rework display scanning** - Simple scanning on DIGIT 0 vs. scan per digit; Still open issues with the latter
    - **Split TMC 0501 and SCOM** - Take this to the extreme, and support out of phase DIGIT / KEY shenanigans

## Settings
- **Enable/disable persistency** - For TI-58/59 (TI-58C always is persistent by design)

## UI
- **Keyboard support** for debugging panels — the calculator itself is done (the Qt6 app and the website's `/59/play/`, see `docs/developer/App-Layer.md` § "Physical Keyboard Mapping"); the debug panels and the printer were deliberately left out. On the desktop all three panels are visible at once, so Space (R/S vs. STEP) and Ctrl+C/Ctrl+X (printer tape vs. selected debugger text) have no unambiguous owner.
- **Keyboard support on iPadOS** — an iPad with a hardware keyboard is a plausible case. The map is platform-neutral (`app/TITyping`); the Swift app's `KeyboardView` and a focus story for the portrait page navigation would need work.

## Debugger
### CPU Debugger
- **Break points** The run control has PC breakpoints on the debug target `TMC0501` (`docs/developer/Debug-API.md` § "Breakpoints"), set only through the session's `break` (the harness's `break TMC0501 <address>`); the core's own `TI59Machine::addBreakpoint` is used only by the tests. Remaining: a UI in the GUIs (through the app layer, `app/`), then conditional triggers (register content, IDLE-RUN change, and more).
- **Latch keyboard entries** Support catching a keypress in single step mode: while the debugger is frozen, a key press is lost (the ROM accepts a key only when two scans one display sweep apart see it).

### Calculator Debugger
- **Break points** Support break points (e.g. STO content, t content, PGM step) — unlike the CPU Debugger, nothing exists here yet, not even PC-level.
- **Latch keyboard entries** Support catching a keypress in single step mode.

## Various
- **Event callbacks** — push notifications for display updates and register changes, replacing the current polling model.
- **Card stacking in the GUI** — presets feed cards (`card:`, `arm:`; `docs/formats/preset.md` §5.2); what remains is letting a user queue a stack by hand.
- **Card sound** — play a recorded card pass, with the emulator's pass timed to match (a real pull-through takes 1.155 s, the emulator's 0.43 s).
- **REST API** — scripted control of a running app. Headless runs (the harness `calcu --product c59`, `docs/developer/Harness.md`) and GUI decoupling (`app/`, `docs/developer/New-GUI-Guide.md`) are done.
- **Help pictures by scenarios** — make the help's pictures by scripted scenarios, as Calc-U-1600 does.
- **Frozen** - Visually indicate when the calculator is frozen in one of the debuggers (mostly useful on iPhone, where only one panel is visible at a time)

## Far out (if ever)
- **Additional calculator models** Such as TI-57
