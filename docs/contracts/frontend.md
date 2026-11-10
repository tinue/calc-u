# Contract: frontend interface, session hosting and messages

Status: draft (2026-11-10). Forks FE-1 to FE-7 and SP-1 to SP-3 are
decided (decision log; FE-5 to FE-7 and SP-1 to SP-3 were added in the
phase 4 review); open questions remain.

Inherits: ADR 0009 (single-threaded sessions, hosted by the frontend,
messages only), 0014 (ObjC++ bridge translates only), 0015 (Swift session
on a background thread, the bridge is a message boundary), 0023
(component sets per technology), 0024 (one menu and control-bar
structure), 0025 (modules chosen in the control bar). Oddities: ODD59-3,
Calc-U-1600's "busy state in AppSession; UI rules in Shared
(`ControlState`)" (`docs/oddities-triage.md`). Builds on `machine.md`
(M-2 display kinds, M-3 key IDs, M-4 live queries, M-5 capabilities) and
`peripherals.md` (P-1 attach power-cycles).

## Today (facts)

| | Calc-U-59 | Calc-U-1600 |
|---|---|---|
| Frontend interface | `SessionController::Platform`: `runOnUiThread`, card and constant-memory file I/O, `showDebugPanel`, `saveLastUsedModel`, `saveModuleID`, `error`, `diagnostic`, `changed(flags)` (`Shared/SessionController.hpp:57-90`) | `AppSession::Platform` (extends `MachineSession::Listener`): `romMissing`, `notice`, `machineReplaced`, `modelSwitched`, `moduleChanged`, `tapesChanged`, `bayError`, `presetArmed`, `presetLog`, `attachFailed`, `busyChanged`, `hostDirectories` (`Shared/AppSession.hpp:62-84`, `Shared/MachineSession.hpp:69-93`) |
| State out | `changed(flags)` with 14 topic bits (model, display, cue card, printer, card, run state, live, CPU, heatmap, log, ASM, trace, listing, …), then the frontend pulls getters (`SessionController.hpp:36-49`) | Callbacks, then the frontend pulls; after an operation the whole window resyncs from `ControlState` (`Decisions.md`, "The busy state is AppSession's alone") |
| Driving | Threaded runner in the apps; `Threading::Cooperative` with `pump(now)` on the web | Frontend calls `tick(now)` at about 60 Hz; operations run with `start(op, done)` (sliced per tick) or `runNow(op, pump)` (Qt: blocks, pumps its event loop) (`AppSession.hpp:48-54`) |
| Web worker | Commands `call`, `loadState`, `dropFile`, …; the worker posts topic messages: model, display, cueCard, runState, printer, card, live, cpu, … (`Web/calc-engine-worker.js:9-35`) | Command `call`; the worker posts `ready`, `reply`, `notice`, `log`, and the bindings post a display frame when it changed (`Web/bindings.cpp:697-800`) |
| UI rules | In each frontend, from model facts (`MachineModelInfo`) | `ControlState`: what pickers, menus and panels the live machine offers, computed in Shared; frontends only render (`Shared/ControlState.hpp`) |
| Busy | — (operations run on the emulation thread) | `AppSession::busy()` and `Platform::busyChanged` are the only "an operation runs" signal |
| Audio policy | key click only, per frontend | `tick()` owns it: turbo and operation audio are thrown away; the frontend supplies only the sink |
| Messages to the user | `Platform::error(title, message)` | English sentences from Shared; frontends show them as they are |
| Input routing | Keys reach the calculator only while it has focus; debug and printer panels take no calculator keys (ODD59-3); a unified Mac input surface is the target (`TODO.md` § UI) | Frontend maps host key → key name (`shared::resolveHostKey`); `MachineSession::hostKeyDown()` decides how it is typed |

## Responsibilities

- The **app session** is the single owner of a window's state (glossary)
  and the only thing a frontend talks to. It runs on one thread chosen by
  the frontend (ADR 0009): the GUI thread, a background thread (Swift,
  ADR 0015) or a Web Worker.
- The frontend talks to it only by **messages**: commands in, state and
  events out (ADR 0009, 0015). The **frontend interface** (glossary) is
  the set of outgoing messages plus the few host services only a
  frontend can provide (audio sink, clipboard, choosing a file or folder).
- The host drives time: it calls the app session's tick with the current
  wall time at about 60 Hz; the app session paces the machine (regular
  or full speed, `machine.md` F-1, F-2) and emits what changed.
- The app session owns the busy state and the audio policy (adopted from
  Calc-U-1600, `Decisions.md`).
- Messages to the user are English sentences from the app layer (both
  products today); frontends show them as they are.
- Frontends render; they decide no rule about what the machine offers
  (FE-2).

## Non-responsibilities

- Layout and product material (faceplate, hit map) stay per product
  (ADR 0023). Which widget sits where follows ADR 0024.
- Typing strategy (contract 4): the frontend sends host keys, or key IDs
  from an on-screen hit; how they are typed is the app session's.
- File locations and the save folder (contracts 6, 7).
- The debugger's own protocol (DAP) (contract 8).

## Invariants

- I-1: A session is never touched by two threads; nothing crosses the
  boundary but messages (ADR 0009).
- I-2: The same messages serve every frontend; a frontend hosting the
  session on its GUI thread receives them synchronously, the others
  through a queue (Swift) or `postMessage` (web).
- I-3: The bridge and the web bindings only translate (ADR 0014); no
  rule lives in them.
- I-5: While an operation runs, the app session keeps sending what
  changed (display, paper, state) at least once per host tick, so every
  frontend visibly updates behind "Loading…" (SP-2).
- I-6: Pacing keeps the long-run rate exact: emulated time is compared
  with an absolute wall-clock deadline, never with the length of the
  last slice, so no lag builds up (SP-1).
- I-4: Nothing in a **generic** message names a model-specific type:
  state is described with the descriptor's terms (display kinds, key
  IDs, places, generic capability kinds). What only one product has
  travels in **product topics** and product commands (FE-6), which the
  library and the shared component sets carry but never read.

## Open forks

### FE-1: Shape of outgoing state

**Decided (2026-11-10): option (a), push per topic.**

Options:
- (a) **Recommended:** the app session **pushes** state by topic when it
  changed: display frame, control state (FE-2), printer output, media
  state, run state, busy, notices, debug-panel topics, … Each message
  carries the whole state of its topic, so a frontend never asks back.
  This is what both web workers already do (Calc-U-59 posts per topic,
  Calc-U-1600 posts frames).
- (b) Change flags, then the frontend pulls getters (Calc-U-59 native):
  a pull across threads needs locks or a round trip, which ADR 0009 and
  0015 rule out for Swift and the web.
- (c) One callback per event (Calc-U-1600 `Platform`): fine on one
  thread, but each callback is a message anyway once a thread boundary
  is in between.

### FE-2: Where the rules about offered controls live

**Decided (2026-11-10): option (a), in the library, generic.**

Facts: Calc-U-1600's `ControlState` names its peripherals and pickers
(`ce150`, `ce1600p`, `floppyShown`, `pc1600RomMenu`, …).

Options:
- (a) **Recommended:** in the library, generic: one control state
  computed from the descriptor and the live machine (models, variants,
  connectors and what fits them, media places, panels), sent as a topic
  (FE-1). The component sets (ADR 0023) render it; ADR 0024's
  "items a product lacks simply don't appear" follows from it.
- (b) In the app layer per product (Calc-U-1600 today): each product
  writes its own control state type and every component set needs a
  version per product.
- (c) In each frontend: three copies of every rule.

### FE-3: Thread of the Qt6 frontend's session

**Decided (2026-11-10): option (a), the GUI thread for now.**

Facts: both Qt6 frontends host on the GUI thread today; Calc-U-1600's Qt
app blocks in `runNow` and pumps its event loop with a "Loading…" popup
(`calc-u-1600.md`, Threading). Swift moves to a background thread
(ADR 0015); the web runs in a Worker.

Options:
- (a) **Recommended:** the Qt6 frontend keeps its session on the GUI
  thread for the migration; the messages are the same (I-2), so moving
  it later changes only the Qt host code. Least risk; no new threading.
- (b) A background thread, like Swift: no blocking `runNow`, the same
  hosting on every desktop; but a new threaded Qt host for both products
  in the migration.

### FE-4: Input routing (ODD59-3)

**Decided (2026-11-10): option (a), one owner per key; today's behaviour stays.**

Options:
- (a) **Recommended:** the contract requires only that every host key has
  one owner at a time; which surface owns it is the frontend's. Today's
  behaviour of each product stays (Calc-U-59: calculator keys only while
  the calculator has focus; panels take none). The unified input surface
  stays a backlog item (Briefing §2: no new features).
- (b) Build the unified input surface in the migration: a new feature.

### FE-5: Shapes of a topic

**Decided (2026-11-10, in the review; approved by the user).** Refines
FE-1.

Facts: FE-1 says every message carries the whole state of its topic and
a frontend never asks back. Paper, logs and serial output only grow;
Calc-U-59's web bindings already send the paper as "the lines from the
first one that changed" (`Web/bindings.cpp:651`). Both web workers
already answer requests (`reply`), for example for card and trace bytes.

Options:
- (a) **Recommended:** three shapes. A **state topic** carries its whole
  state when it changed (display, control state, run state, busy, media
  state). A **stream topic** carries what was appended since the last
  message, plus a reset marker when it starts over (paper, log, serial
  output, trace list). A **request** gets one reply and is for large
  things asked for rarely (the paper as a 600 dpi image, the bytes of a
  file to export). A frontend still never reads session state directly.
- (b) Whole state only (FE-1 as written): a long paper is sent again
  with every printed line, through a queue (Swift) or `postMessage`.

### FE-6: Product topics, product commands and product control entries

**Decided (2026-11-10, in the review; approved by the user).** Refines
FE-2 and I-4.

Facts: cue cards, the CALCULATOR tab, the ROM heatmap, the ASM overlay
(Calc-U-59) and the host drive, the INSPECTOR views, the CE-158 printer
output (Calc-U-1600) exist in one product. Calc-U-1600's `ControlState`
holds rules no descriptor yields: the disk picker "shown on every
PC-1600 (so the bar doesn't jump), usable only while the CE-1600P … is
attached"; the host drive offered "only on a frontend that has host
directories" (`Shared/ControlState.hpp`).

Options:
- (a) **Recommended:** the same rule as for capabilities (`machine.md`,
  M-7). A product's app layer may define topics and commands of its own;
  they use the same message path and the three shapes of FE-5, and only
  that product's frontends read them. The control state has a generic
  part computed by the library (FE-2: models, variants, places and what
  fits them with "available now" and a reason, media places, panels)
  and may carry product entries added by the product's app layer under
  the same rule (still computed below the frontend, never in it). A
  topic that a second product needs becomes generic.
- (b) Everything generic: the generic messages would grow a field for
  every product's one-of-a-kind function.

### FE-7: The web embedding interface

**Decided (2026-11-10, user): option (a).**

Facts: calc-u-site, another repository, embeds each web client from its
release zip and pins it by sha256. Calc-U-59: `Web/dist/` is
location-independent, a host loads `calc.js` and `calc.css` and mounts
the component `PlayCalculator` (props `layout`, `width`, `keyboard`);
`MANIFEST.json` gives name, version, commit and files. Calc-U-1600: the
component `CalcU1600` (props `width`, `height`, `keyboard`, `model`,
`preset`, `workerUrl`), `files.json`, `MANIFEST.json`; styles of both
under `.calcu-client` (`divergence-matrix.md`, M61).

Options:
- (a) **Recommended:** one embedding interface for the family, written
  down once: a location-independent `dist/`, `MANIFEST.json` with the
  same fields, styles under `.calcu-client`, one component per product
  with the same prop names for the same things (`width`, `height`,
  `keyboard`, `model`, `preset`), product props beside them. The
  component names stay per product.
- (b) Each product keeps its own: the site carries two embed codes that
  drift.

## Sessions, pacing and operations

Added in the phase 4 review: the divergence matrix marked these "merge"
(M10, M12, M17, M44), and no contract covered them. The facts and the
rules that follow from decisions already taken come first, then the
three forks (decided 2026-11-10).

### Today (facts)

| | Calc-U-59 | Calc-U-1600 |
|---|---|---|
| Pacing | 20 ms batches of weighted steps against absolute deadlines; lag over 100 ms restarts the schedule; random jitter on batch ends (ODD59-5) (`Shared/Pacing.hpp`) | `shared::Pacer`: about 16 ms ticks against an absolute deadline; a stall over 0.1 s is dropped (`Shared/Pacer.hpp`) |
| Full speed | holding the display, or `FullSpeed:`; still sleeps in the scan loop or after a hardware poll (ODD59-6) | holding the LCD: as many frames as fit in 14 ms; operations run at full speed |
| Background | Swift and web pause; Qt keeps running (ODD59-25) | web pauses; Qt keeps running |
| Long work | on the emulation thread | coroutine jobs, sliced (`start`) or to completion; Qt blocks in `runNow` and pumps its event loop |
| A file dropped or opened | each frontend decides by name | classified in one place (`Core/DropFile`), by content through libsharpdx |

### Rules that follow from decisions already taken

- The machine session paces: it compares the machine's tick count
  (`machine.md`, M-1) with the wall time the host passes to the tick; lag
  beyond about 0.1 s is dropped (both products).
- ODD59-5 and ODD59-6 stay (`docs/oddities-triage.md`) as pacing
  parameters of the TI machines. The scan-loop sleep reads key readiness
  from typing support (`typing.md`, T-2).
- Holding the display runs at full speed in every product (both today).
- Pausing in the background stays per frontend as today (ODD59-25
  stays): the frontend sends pause and resume; the session decides
  nothing about the host.
- Long work is an operation: a coroutine job (ADR 0009), busy until it
  ends (Q-1).

### SP-1: One pacer, a pacing profile per model

**Decided (2026-11-10, user): option (a)**, on the condition that the
pacing algorithm is carried over correctly. The user's reason: the same
bug was fixed once in each product, an emulation that lagged behind
because the pacing was not averaged. I-6 states the property; a test per
product compares emulated time with wall time over a long run, before
and after the pacer moves to the library.

Options:
- (a) **Recommended:** one pacer in the library. A product supplies a
  **pacing profile** per model, like the typing profile: the slice
  length, the jitter (TI) and whether full speed sleeps while the
  machine reports key readiness (TI).
- (b) Each product keeps its pacer: the same absolute-deadline code
  twice.

### SP-2: How operations run on a GUI-thread host

**Decided (2026-11-10, user): option (a), always sliced, with live
updates.** The user's requirement: the GUI keeps updating while
"Loading…" is in front. Both options can meet it (`runNow` pumps the Qt
event loop between 10 ms slices; a sliced operation sends its topics
after each slice and the window repaints through the normal event loop).
It is now an invariant (I-5) and is checked by a test.

Facts: FE-3 keeps the Qt6 session on the GUI thread; Calc-U-1600's Qt
app runs operations with `runNow` (blocking, pumping the Qt event loop,
"Loading…" popup), its web client with `start` (sliced per tick).

Options:
- (a) **Recommended:** operations always run sliced, one slice per host
  tick, in every frontend; the frontend shows "Loading…" from the busy
  state. `runNow` goes; the harness runs an operation to completion
  because nothing else needs the thread.
- (b) Keep `runNow` for Qt: two ways to run the same operation.

### SP-3: Opening a file

**Decided (2026-11-10, user): option (a).** Today's accepted files stay;
where a platform has no drop today (iOS), none is added.

Options:
- (a) **Recommended:** one "open file" command on the app session for a
  drop, Open With and a file picker that takes any file. The library
  classifies the file (preset, medium, something of the product's); a
  product adds its own file kinds (BASIC and machine-code programs, ASM
  overlays) and their classifier. Calc-U-1600's way, for the family.
- (b) Each frontend decides by file name (Calc-U-59 today).

## Fit table

| | TI-59 (58, 58C) | PC-1500 (1500A) | PC-1600 | PC-1403 (paper) | PC-1350 (paper) |
|---|---|---|---|---|---|
| Frontends | Swift (iOS, macOS), Qt6, Web | Qt6, Web | Qt6, Web | — | — |
| Display topic (M-2) | segment frame, brightness per position, C indicator | dot frame + status-line indicators | dot frame + 16 indicators | dot frame in 24 cells + status segments | dot frame |
| Control state (FE-2) | model; module slot picker (ADR 0025); printer connect; card reader place (TI-59) | model, ROM variant; one slot picker; CE-150, CE-158 | model, ROM variant; two slot pickers; CE-1600P (+ disk picker, CE-1600P ROM variant), CE-150, CE-158 (MODE 1); host drive | model; 11-pin peripheral | model; RAM card picker; 11-pin and 15-pin peripherals |
| Panels | printer paper (PC-100C), debug panel | paper (CE-150), CE-158 printer, debug panel | paper (CE-1600P), CE-158 printer, debug panel | paper (CE-126P) | paper (CE-126P, CE-515P) |
| Sound out | none emulated (key click is a frontend feature) | buzzer PCM | buzzer PCM | buzzer | buzzer |
| Stream topics (FE-5) | paper (PC-100C), log | paper (CE-150), CE-158 printer output | paper (CE-1600P), CE-158 printer output | paper (CE-126P) | paper |
| Product topics (FE-6) | cue card, the debug panel's tabs, heatmap, ASM overlay | INSPECTOR views, CE-158 printer output | same, plus host drive | — | — |
| Pacing profile (SP-1) | 20 ms slices, jitter, scan-loop sleep at full speed | about 16 ms slices | about 16 ms slices | — | — |

## Open questions

- Q-4 (phase 4b, with the media feature): What "open file" does with a
  medium that lies outside the save folder. ADR 0019 allows two ways to
  such a file only (a preset names it, or the user puts it into the save
  folder), so a dropped medium cannot be opened in place.
- Q-3 (phase 4b, with the preset feature): What "idle" means per model
  for a bare `- wait:` (`presets.md`, PR-5 gives it to both products;
  Calc-U-59 has no such step today).
- Q-1 (answered 2026-11-10): commands that would start an operation (load a
  preset, reset, cold boot, model switch, attach or detach, insert) are
  refused while one runs; user keys are dropped (T-4); everything else
  (settings, panels, copy, screenshot) goes through. Background: What the app session does with a command that arrives while an
  operation runs. Facts: Calc-U-1600 refuses a second operation
  (`AppSession::start()` returns false while busy) and runs a nested
  `runNow` straight through (`Shared/AppSession.cpp:94-121`); while busy,
  `tick()` runs operation slices instead of paced emulation (`:136-156`).
  What key input and other commands do while busy is not settled; it is
  taken up with the typing contract (4), which drops user keys while
  busy (`typing.md`, T-4).
- Q-2 (answered 2026-11-10): the host services every frontend provides: an
  audio sink; the clipboard (text and image); file and folder pickers
  (open preset, choose the ROM folder, the save-folder override); export
  or download of a file (web: download); opening a URL (Help); haptics
  where the platform has them. File access in the save folder and the
  settings file are the library's. Background: the minimal list (audio sink, clipboard, file and
  folder pickers, …) is settled with contracts 6 and 7, where file access
  is decided.

## Decision log

| Date | ID | Decision |
|---|---|---|
| 2026-11-10 | FE-1 | The app session pushes the whole state of a topic when it changed; frontends never pull. |
| 2026-11-10 | FE-2 | The rules about which controls a machine offers live in the library, as one generic control state computed from the descriptor and the live machine. |
| 2026-11-10 | FE-3 | The Qt6 frontends keep their session on the GUI thread for the migration; the messages are the same as elsewhere, so moving later changes only the Qt host. |
| 2026-11-10 | FE-4 | The contract requires one owner per host key at a time; which surface owns it is the frontend's. Today's behaviour stays (ODD59-3); the unified input surface stays a backlog item. |
| 2026-11-10 | FE-5 | Three topic shapes: state topics (whole state), stream topics (appended since the last message, with a reset marker) and requests with one reply for large, rare things. Refines FE-1. |
| 2026-11-10 | FE-7 | One written embedding interface for the web clients: location-independent `dist/`, `MANIFEST.json` with the same fields, styles under `.calcu-client`, the same prop names for the same things; component names stay per product. |
| 2026-11-10 | SP-1 | One pacer in the library with a pacing profile per model (slice length, jitter, scan-loop sleep at full speed). The algorithm is carried over unchanged in effect; the long-run rate stays exact (I-6), checked per product. |
| 2026-11-10 | SP-2 | Operations always run sliced, one slice per host tick, in every frontend; `runNow` goes. The session keeps sending updates while an operation runs (I-5). |
| 2026-11-10 | SP-3 | One "open file" command on the app session for drop, Open With and generic pickers; the library classifies presets and media, a product adds its own file kinds. |
| 2026-11-10 | FE-6 | A product's app layer may define topics, commands and control-state entries of its own, on the same message path; only that product's frontends read them. Generic messages stay free of model-specific types (I-4). Refines FE-2. |
