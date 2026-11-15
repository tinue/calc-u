# Shared architecture: Core, Shared, frontends

Calc-U-1600 is built in three layers, so the desktop app and the
WebAssembly client ([Web-Client.md](Web-Client.md)) run the same emulator
and the same app logic:

```mermaid
flowchart TB
    Qt["frontends/qt6/app/ — desktop GUI<br/>(widgets, dialogs, menus, QAudioSink, DAP server)"]
    Web["frontends/web/ — web client<br/>(React, canvas, IndexedDB, AudioWorklet)"]
    Shared["app/ — app logic, no GUI<br/>AppSession, MachineSession, bays, Pacer, settings, paths, layouts"]
    Core["core/ — the hardware<br/>CPUs, machines, cards, loaders, presets, jobs"]
    Qt --> Shared
    Web --> Shared
    Shared --> Core
```

- **`core/`** is the emulated hardware and what drives it: CPUs, machines,
  cards and connectors, program and preset loaders, the debugger core.
- **`app/`** is everything above Core that is not drawing: the live
  machine and its operations, the module/floppy/tape bays, pacing and audio
  policy, settings keys, file locations, and the layout numbers renderers
  draw with.
- **A frontend** keeps widgets and painting, dialogs, menus, settings
  storage, the audio device, the debugger transport and packaging.

Both libraries are built by [`cmake/CalcU.cmake`](../../cmake/CalcU.cmake)
(`calcu_core`, `calcu_shared`) for the Qt app, the CLIs, the tests and the
WebAssembly check.

## Rules

- **Non-GUI logic goes in `app/`.** A frontend translates types only
  (QString ↔ `std::string`, Qt keys → `shared::HostKey`, pixel buffers →
  QImage) and shows what Shared decides.
- **Portable C++20, standard library only.** No Qt, Cocoa or Emscripten
  headers in Core or Shared. Paths are UTF-8 `std::string`s converted
  through `app/Utf8Path.hpp` (Windows code pages).
- **Single-threaded and never blocking.** Everything runs on the
  frontend's one thread. Long machine work (a boot, a preset, a typed
  program, a power cycle) is a resumable job (below), never a blocking
  loop with a yield hook. There is no threaded mode (unlike Calc-U-59's
  Shared layer); see Decisions.md.
- **Shared never decides where files live.** The frontend hands it the
  host's folders (`calcu::settings::HostDirs`: home, the settings base of its
  platform, on the Web the save folder) and the bundled-resources folders
  (`shared::BundleDirs`), and opens the settings file, `Calc-U-1600.ini`
  (`calcu::settings::SettingsFile` at `settingsFilePath()`, LIB-9); the
  library's `calcu::settings::Paths`, `shared::AppPaths` and
  `shared::Settings` (the declaration of every key, `settingsDecl()`) do the
  rest. No frontend keeps a setting in QSettings or `localStorage` (CTest
  `CalcU1600NoPlatformSettings`).
- **Messages for the user** come back as English sentences (the app has no
  translations).

## The pieces

| File | What it is |
|---|---|
| `AppSession` | The whole emulator behind one frontend: owns `MachineSession`, the bays, the `Pacer` and the `AudioPump`, wires them, runs operations, `tick()` |
| `MachineSession` | The live PC-1500/1500A/1600 as one `calcu::machine::Machine`: model and ROM switches, host keys (`hostKeyDown`) and paste queue, display, audio, memory-module slots, peripherals, tape, host drive, trace, inspector, serial links (see "The machine" below) |
| `ModuleBay`, `FloppyBay`, `TapeBay` | The control bar's slots, disk and tape: catalogues, attach, Name & Save, preset `saveas:`, debounced autosave |
| `PresetCatalog` | The bundled presets of `examples/bundled.yaml` (`defaults`, `examples`) and any preset's short name and description; `examples:` refs resolve through `AppPaths` |
| `Pacer`, `AudioPump` | Frame ticks → emulated time (absolute deadline, turbo); buzzer PCM → an `AudioSink` |
| `Settings`, `AppPaths`, `Utf8Path` | Every settings key with its default (the declaration of `Calc-U-1600.ini`); the save/tapes/bundle folders and by-name save paths |
| `HostKeyMap`, `KeyLayout/`, `LcdLayout`, `PlotterPaper`, `Rgb` | Host keys → calculator keys; key hit-regions and the faceplate fit; LCD geometry, colours and status indicators (`lcd::indicators()`); plotter-paper geometry |
| `MachineModel` | The `Model` and ROM-version enums |
| `ControlState` | What the pickers, menus and panels offer right now: every rule about which control a model has and which peripheral excludes which |
| `MediaLibrary` | The save folder's cards, disks and tapes as items to list, export, import and delete (the web's Saved Media…) |
| `MachineCodeLoad` | Load BASIC Program… / Load Machine Code…: a WAV's files to pick from, then the load and its advice |
| `CardYamlDump` | The INSPECTOR's Card YAML: each slot's module RAM as an `initial-content:` block |
| `DisplayFrame` | One model-agnostic LCD frame a renderer paints from, made from the machine's display output (`toDisplayFrame`, an adapter until M-49) |
| `DirectoryScanCache`, `Text` | A media folder's last parse, reused while its files are unchanged; small string helpers |

## The machine

`MachineSession` owns the live machine in one
`std::unique_ptr<calcu::machine::Machine>` (C1600-1; the library's machine
interface, `library/machine/`). Nothing in `app/` or a frontend names
`PC1500Machine` or `PC1600Machine`: the CTest `CalcU1600MachineIncludeCheck`
(`tools/check_machine_includes.sh`, Frequent and CI) fails the build when a
file in `app/`, `frontends/`, `tools/` or `dev/` includes or names one.

- **Building.** `calcu::machine::build(catalogue, SharpMachineBuilder, request)`
  with a copy of `sharp::sharpCatalogue()` (the models, CE-150, CE-158,
  CE-1600P, host drive) to which the module definitions are added
  (`sharp::addModuleDefinitions`, `moduleDescriptor`). The ROM images come
  through `sharp::modelRomImages` / `peripheralRomImages`
  (`core/Machine/SharpRomImages`, an adapter until M-27). Every rebuild is
  then a `coldBoot()`.
- **Operations and keys.** `reset()`, `coldBoot()`, `powerCycle()` (the
  off / on sequence now runs inside the core, `core/Machine/PowerCycleSequence`),
  then `sharp::legacy::runToPromptJob` (until M-46). Keys are key IDs
  (`keyDown`/`keyUp`; `on` is one of them); the PC-1500's paced keystroke
  queue is the `c1600.key-queue` kind.
- **Peripherals** are `attach`/`detach`/`replace` in the places `slot-1`,
  `slot-2`, `printer`, `interface`, `host-drive`; a change on a running
  machine reaches the bus at the off point of the power cycle that follows.
  The CE-1600P with the CE-158, or a second printer, is refused with a
  sentence.
- **What only a Sharp machine has** is a capability: the product kinds
  `c1600.loader-memory`, `c1600.inspector`, `c1600.ce158-printer-port`,
  `c1600.host-drive-folder`, and the adapter kinds `c1600.plotter-paper`,
  `c1600.trace`, `c1600.tape-deck`, `c1600.floppy-drive`
  (`core/Machine/SharpProductKinds.hpp`); the library's `serial-port`,
  `real-time-clock` and `controls` (the CE-161's write-protect switch).
  What has no seam yet (loaders, preset loaders, the debug target, the
  PC-1600's typing latches, the card in a slot) goes through
  `core/Machine/LegacyCalls.hpp`, each call with the step that removes it.
- **Serial ports and clock.** The session holds one
  `calcu::peripherals::HostLinks` (the pseudo-terminals,
  `calcu1600-rs232c.serial`, `calcu1600-ce158.serial`) and pumps it after
  each slice; the clocks are set with `calcu::peripherals::setClocksFromHost`.
- The machines hold no lock and start no thread (ADR 0009); the host links'
  own reader and writer threads touch only their rings.

## How a frontend drives it

1. Implement `shared::AppSession::Platform`. It is a
   `MachineSession::Listener` (a missing ROM, notices, model switched,
   machine replaced, debugger hooks) plus the app's own events: bay
   errors, preset log and "armed", a failed attach, `busyChanged` (resync
   the whole UI when an operation ends). The bays' part in a rebuild
   (`MachineSession::Bays`) is AppSession's own business.
2. Open the `SettingsFile`, build `Settings` over it, `AppPaths` (with its
   `HostDirs` and `BundleDirs`) and
   the `AppSession`; `attachAudio()` the host's `AudioSink`; call `start()`
   to boot the startup model.
3. Call `tick()` from a ~60 Hz timer. It runs a slice of the running
   operation, or paced emulation (`pacer().setTurbo()` for fast-forward),
   then the per-frame duties (autosave, tape), then plays the buzzer (or
   throws turbo and operation audio away). Repaint from
   `machine().currentDisplay()` and `lcd::indicators()`, plotter points,
   printer output, `tapeStatus()`. A frozen frontend (screenshots) uses
   `runFor()` / `runUntilPasteDone()` instead.
4. Resolve host keys with `resolveHostKey()` and hand them to
   `machine().hostKeyDown()`; release what it asks to with `hostKeyUp()`,
   and send a bare Shift tap to `hostShiftTap()`.
5. Start operations — `loadPresetJob()`, `resetJob()`, `selectModelJob()`,
   `togglePeripheralJob()`, … — with `start(op, done)`; a browser waits for
   `done`. A desktop app may instead `runNow(op, pump)`, which steps the job
   in 10 ms slices and calls `pump` between them (the Qt app keeps painting
   and shows its "Loading…" popup there).

The Qt app does exactly this: `MachineController` owns the `AppSession`
and is its Platform, `EmulationPacer` calls `tick()` from a QTimer,
`SyncOperations` runs operations with `runNow()`, `AudioOutput` is the
QAudioSink, and `MainWindow` keeps only the physical-key map and the
Shift-tap timer.

## Jobs: resumable machine work

[`core/Job.hpp`](../../Core/Job.hpp) turns the blocking sequences into
C++20 coroutines. Wherever the code ran the machine — `machine.runCycles(n)`
— it now says `co_await job::run(machine, n)`. A `job::Job` runs the
coroutine a budget at a time (`step(cycles)`), or straight through
(`job::runToCompletion()`, which every old blocking function name now
calls, so the CLIs and tests didn't change).

A run request is never split by the budget, so a job sliced into frame
ticks makes exactly the machine calls of one run straight through:
`tests/job_tests.cpp` checks five presets for identical step logs,
results, memory, PCs and LCD both ways.

Writing one: every function that (transitively) runs the machine returns
`job::Task<T>` and awaits the ones it calls; take parameters by value
(except the machine and out-pointers) — a Task starts lazily, after the
expression that created it.

## WebAssembly

`tools/build_wasm_check.sh` builds Core + Shared with Emscripten and runs
`tools/wasm_smoke.cpp` under node: both models, three presets run
cooperatively from `tick()`, typed input. `libsharpdx-wasm.a` is the
program loaders' Rust library built for `wasm32-unknown-emscripten`
(`tools/build_sharpdx_wasm.sh`). Native wasm exceptions
(`-fwasm-exceptions`), no threads; there are no host links there (the
library's `NoHostLink.cpp`). The web client builds the same libraries as the `calcu_web`
target ([Web-Client.md](Web-Client.md)).

## Not in Shared yet

- The DAP debugger (`frontends/qt6/app/debug/`): its protocol logic is written
  against QJson and needs a JSON value type in Shared first.
- The CLIs build their own preset environment rather than using
  `AppSession`.

These are rows in [TODO.md](../../TODO.md).
