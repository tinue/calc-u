# Contract: frontend interface, session hosting and messages

Status: draft (2026-11-10). Forks FE-1 to FE-4 are decided (decision
log); open questions remain.

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
- I-4: Nothing in a message names a model-specific type: state is
  described with the descriptor's terms (display kinds, key IDs,
  connectors, capabilities).

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

## Fit table

| | TI-59 (58, 58C) | PC-1500 (1500A) | PC-1600 | PC-1403 (paper) | PC-1350 (paper) |
|---|---|---|---|---|---|
| Frontends | Swift (iOS, macOS), Qt6, Web | Qt6, Web | Qt6, Web | — | — |
| Display topic (M-2) | segment frame, brightness per position, C indicator | dot frame + status-line indicators | dot frame + 16 indicators | dot frame in 24 cells + status segments | dot frame |
| Control state (FE-2) | model; module slot picker (ADR 0025); printer connect; card reader place (TI-59) | model, ROM variant; one slot picker; CE-150, CE-158 | model, ROM variant; two slot pickers; CE-1600P (+ disk picker, CE-1600P ROM variant), CE-150, CE-158 (MODE 1); host drive | model; 11-pin peripheral | model; RAM card picker; 11-pin and 15-pin peripherals |
| Panels | printer paper (PC-100C), debug panel | paper (CE-150), CE-158 printer, debug panel | paper (CE-1600P), CE-158 printer, debug panel | paper (CE-126P) | paper (CE-126P, CE-515P) |
| Sound out | none emulated (key click is a frontend feature) | buzzer PCM | buzzer PCM | buzzer | buzzer |

## Open questions

- Q-1: What the app session does with a command that arrives while an
  operation runs. Facts: Calc-U-1600 refuses a second operation
  (`AppSession::start()` returns false while busy) and runs a nested
  `runNow` straight through (`Shared/AppSession.cpp:94-121`); while busy,
  `tick()` runs operation slices instead of paced emulation (`:136-156`).
  What key input and other commands do while busy is not settled; it is
  taken up with the typing contract (4).
- Q-2: Host services: the minimal list (audio sink, clipboard, file and
  folder pickers, …) is settled with contracts 6 and 7, where file access
  is decided.

## Decision log

| Date | ID | Decision |
|---|---|---|
| 2026-11-10 | FE-1 | The app session pushes the whole state of a topic when it changed; frontends never pull. |
| 2026-11-10 | FE-2 | The rules about which controls a machine offers live in the library, as one generic control state computed from the descriptor and the live machine. |
| 2026-11-10 | FE-3 | The Qt6 frontends keep their session on the GUI thread for the migration; the messages are the same as elsewhere, so moving later changes only the Qt host. |
| 2026-11-10 | FE-4 | The contract requires one owner per host key at a time; which surface owns it is the frontend's. Today's behaviour stays (ODD59-3); the unified input surface stays a backlog item. |
