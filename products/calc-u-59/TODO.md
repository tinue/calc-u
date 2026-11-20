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
- **Unified Mac input surface** — the target design: on macOS the whole window is one keyboard surface, every key has exactly one meaning, and focus stops being a concept. The three panels only remain separate because iPhone portrait swipes between them, which is a layout concern, not an input one. `KeyboardView`'s focus scoping (`PhysicalKeyboardModifier`) is a step toward this, with a rough edge: clicking the debug panel silently stops keys reaching the calculator, with no indicator — there is no focus ring, because indicating focus is not where this is heading.
- **Keyboard support** for debugging panels — the calculator itself is done (macOS app, Qt6 app and the website's `/59/play/`, see `docs/developer/App-Layer.md` § "Physical Keyboard Mapping"); the debug panels and the printer were deliberately left out. On macOS all three panels are visible at once, so Space (R/S vs. STEP) and ⌘C/⌘X (printer tape vs. selected debugger text) have no unambiguous owner. Resolved by the unified surface above: one meaning per key settles the ownership question instead of arbitrating it per panel.
- **Keyboard support on iPadOS** — an iPad with a hardware keyboard is a plausible case the current macOS-only gating excludes. The map and the view-model machinery are already platform-neutral; only `KeyboardView`'s `#if os(macOS)` and a focus story for the portrait page navigation would need work.

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
- **Frozen** - Visually indicate when the calculator is frozen in one of the debuggers (mostly useful on iPhone, where only one panel is visible at a time)

## Far out (if ever)
- **Additional calculator models** Such as TI-57
