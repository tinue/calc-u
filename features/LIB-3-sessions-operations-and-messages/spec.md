# LIB-3: Sessions, operations and messages

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: LIB-1 (machines, display and sound output), LIB-17 (the
  pacer).
- Contracts and decisions: `docs/contracts/frontend.md` (FE-1, FE-3 to
  FE-6, SP-2, I-1 to I-5, Q-1, Q-2, "Rules that follow from decisions
  already taken"), `docs/contracts/README.md` (L-1, the map of library
  and app layer), `docs/contracts/presets.md` (PR-4, for the speed of an
  operation); ADR 0009, 0014, 0015; `docs/oddities-triage.md`
  (ODD59-25); `docs/behaviour-changes.md` row 59-5.

What the three status columns mean for this feature:
- **Library:** the app session and the machine session exist as library
  parts, with operations, the busy state, the three shapes of a topic,
  commands and host services, shown with a machine built for the tests
  and a frontend that is a test.
- **Calc-U-59, Calc-U-1600:** every frontend of the product talks to a
  library app session by messages only; the product's own session
  classes and, in Calc-U-59, its threaded runner are gone. A product
  column is shown first with the Qt6 frontend and the harness; it is
  complete only together with the rows that host the session in the
  other frontends (C59-5 for Swift, LIB-15 for Web).

Two words used here that the glossary does not have in this sense
(OQ-6): to **suspend** a session is the frontend stopping its machine
while the product is in the background, and to **continue** it is the
opposite; the glossary's "pause" and "resume" are the debugger's.

## User Stories
- As a user, I want the window to stay alive while something long runs
  (a preset, a load, a power cycle): the display and the paper keep
  updating behind "Loading…".
- As a user, I do not want two long things to run into each other: what
  cannot start now does not start.
- As a user, I do not want to hear the noise of a machine racing at full
  speed.
- As the developer, I want one session for every product and frontend,
  so that the glue is not written twice and cannot drift again.
- As the developer of a frontend, I want to be told what changed and
  never have to ask, so that the same frontend code works whether the
  session runs on my thread, on another thread or in a Web Worker.
- As the developer of a product, I want to send what only my product has
  (a cue card, the INSPECTOR views) the same way, without the library
  knowing it.
- As an agent, I want to drive a session without a frontend and get the
  same results.

## Out of Scope
- What the controls around the machine offer: the control state (owner:
  LIB-4).
- How fast a machine runs between two calls of the host (owner:
  LIB-17). This feature tells the pacer the speed and when to start
  afresh (AC-21 to AC-23).
- Turning host keys into key presses, and dropping user keys while busy
  (owner: LIB-5); which surface of a frontend owns a host key
  (`frontend.md` FE-4: the frontends, as today).
- The operations themselves and the commands that start them: model
  switch, reset, cold boot, attach and detach (LIB-18), loading a preset
  (LIB-6), insert and Name & Save (LIB-7), opening a file (LIB-13). This
  feature gives them the way to run.
- The topics of other features: control state (LIB-4), media state
  (LIB-7), paper (LIB-8), run state and debugger topics (LIB-10), "ROMs
  missing" (LIB-12), and every product topic (C59-4 and the product
  rows).
- Hosting the session on a background thread in the Swift frontend and
  the bridge (owner: C59-5); hosting it in a Web Worker (owner: LIB-15).
- The harness's commands and protocol, and a second listener on a
  running session for harness attach (owners: LIB-11, C59-5).
- Files, the save folder and the settings file (LIB-7, LIB-9); the ROM
  lookup (LIB-12).
- "Loading…" as the user sees it (owner: LIB-14, C59-8).
- Moving the Qt6 session off the GUI thread (`frontend.md` FE-3: not in
  the migration).

## Acceptance Criteria

### Library

Sessions and the host
- [ ] **AC-1** — Given a product, when a window is opened, then it gets
  one app session from the library for as long as it is open. The app
  session holds one machine session: a machine, its pacing and its input
  queue (glossary; L-1). Machines come and go inside it (LIB-18).
- [ ] **AC-2** — Given an app session, when its host calls it with the
  current wall time, about 60 times a second, then the session runs the
  machine for that call and sends what changed. Without an operation the
  machine runs at the speed last commanded (AC-21); with one, the
  operation says what runs in this call and at which speed (AC-14).
- [ ] **AC-3** — Given an app session whose host does not call it, then
  the machine does not run.
- [ ] **AC-4** — Given the same commands and host calls in the same
  order, with the same wall times, when the session is hosted once with
  messages delivered at once (the caller's thread) and once with
  messages delivered through a queue (another thread, a Web Worker),
  then the frontend receives the same messages in the same order
  (`frontend.md` I-2).

Topics
- [ ] **AC-5** — Given a state topic, when its state at the end of a
  host call differs from the state last sent, then one message with the
  whole state of the topic is sent; when it does not differ, none is
  sent.
- [ ] **AC-6** — Given a frontend that starts to listen, then it is sent
  every state topic once, so that a frontend that keeps the last message
  of each state topic always has the whole current state and never asks
  back (`frontend.md` FE-1).
- [ ] **AC-7** — Given a stream topic, when something was appended since
  the last message, then a message carries exactly what was appended, in
  order; when the stream starts over, the message carries a reset
  marker.
- [ ] **AC-8** — Given a request, then it gets exactly one reply, with
  the thing asked for or with why it cannot be given (`frontend.md`
  FE-5).
- [ ] **AC-39** — Given the requests of this feature, then they are:
  the machine's emulated time (its tick count, and the same in
  seconds); and what the machine offers now (its capabilities with kind,
  ID and owner, its places and what is plugged in). The harness uses
  them (LIB-11 AC-9).
- [ ] **AC-9** — Given the library's own topics of this feature, then
  they are: the display (state topic: the machine's display output),
  busy (state topic), notices (stream topic) and the log (stream topic).
  Every other generic topic is defined by the feature that owns it, in
  one of the three shapes.
- [ ] **AC-10** — Given a notice, then it is an English sentence for the
  user about something that just happened (an error, a refusal, a
  hint), written below the frontend, and the frontend shows it as it
  is.
- [ ] **AC-11** — Given the log, then it is lines of text for the user
  to read later, written by any library feature and by the product's app
  layer: the lines of a running preset, the report of outdated files,
  developer diagnostics.
- [ ] **AC-12** — Given a product's app layer defines a topic, a command
  or a request of its own, then it travels the same way and in the same
  three shapes as a generic one, the library carries it without reading
  it, and a frontend part of the library that receives a topic it does
  not know ignores it (`frontend.md` FE-6).

Commands
- [ ] **AC-13** — Given a command, then it gets one reply: done, or
  refused with the reason. A command the session does not
  know is refused, and a refused command changes nothing.

Operations and busy
- [ ] **AC-14** — Given an operation is started, then the busy topic
  says so until the operation ends; one part of it runs per call of the
  host, within the same allowance of host time as a call at full speed
  (LIB-17 AC-5), so that the frontend repaints between two calls; and
  the machine runs inside it at full speed unless the
  operation asks for regular speed for a stretch, which is then paced
  like any regular-speed running (`presets.md` PR-4; behaviour change
  59-5). The busy topic also says, for every operation, whether it runs
  at full speed or at regular speed now, so that a frontend shows
  "Loading…" only for the first (LIB-6 AC-22, LIB-14 AC-15).
- [ ] **AC-15** — Given an operation runs, when the display, the paper
  or any other topic changes, then the change is sent at the end of the
  host call in which it happened (`frontend.md` I-5).
- [ ] **AC-16** — Given an operation runs, when a command arrives that
  would start another operation, then it is refused; every other command
  and request is carried out as usual. Starting an operation always goes
  through the session, also for a product command, so the rule holds for
  product commands without the library reading them (`frontend.md` Q-1).
  The commands that end a running operation are the exception: Reset,
  Cold boot and a model switch (LIB-18 AC-16).
- [ ] **AC-17** — Given an operation runs, when it does itself what
  would otherwise be an operation (a preset that cold boots, attaches
  and types), then that is part of the running operation and is not
  refused.
- [ ] **AC-18** — Given an operation ends, then the busy topic says so,
  the reply to the command that started it says whether it succeeded or
  with which message it failed, a failure is also a notice, and pacing
  starts afresh (AC-23).
- [ ] **AC-19** — Given a host without a frontend (the harness, a test),
  when it runs an operation in one go instead of in parts, then the
  operation has the same result, the display output is the same, and
  the machine answers the same calls in the same way from then on, as
  after a run in parts driven by a clock the test supplies.

Sound
- [ ] **AC-20** — Given a frontend that supplies an audio sink, then the
  sink is told the sample rate of the machine's sound output, and the
  sound output at regular speed is handed to it; the sound of a stretch
  at full speed and of an operation is thrown away. Without a sink the
  sound output is thrown away. A host that records instead of playing
  (the harness, LIB-11) is handed all of the sound output, whatever the
  speed (`frontend.md`, responsibilities; today
  Calc-U-1600 produces 48,000 samples per second and starts the sink
  with that rate, `Shared/AudioPump.cpp:41-80`).

Speed and pacing
- [ ] **AC-21** — Given the command to run at full speed or at regular
  speed (a frontend sends it when the display is held and when it is let
  go), then the machine runs so from the next call of the host.
- [ ] **AC-22** — Given a pacing profile whose full speed is paced while
  the machine waits for input, then the machine session tells the pacer
  whether that holds, for each slice (LIB-17 AC-12): the machine reports
  key readiness (LIB-1 AC-42), or one of its media places waits for a
  medium (LIB-7).
- [ ] **AC-23** — Given a session is created, is continued after a
  suspend, ends an operation or gets a new machine, then it tells the
  pacer to start afresh, in the last case with the new machine and its
  pacing profile (LIB-17 AC-13, AC-14).

Suspend
- [ ] **AC-24** — Given the frontend sends suspend, when the host goes
  on calling, then the machine does not run; given it sends continue,
  then the machine runs again. The session never suspends by itself
  (`frontend.md`, rules from decisions already taken).
- [ ] **AC-25** — Given a machine the debugger has paused, when the
  session is suspended and continued, then the machine is still paused;
  given a suspended session, when the debugger resumes, then the machine
  does not run until the session is continued.

Host services
- [ ] **AC-26** — Given a frontend that starts to listen, then it states
  which host services it has, out of: the audio sink; the clipboard, for
  text and for an image; a file picker and a folder picker; handing a
  file to the user to keep (on the web: a download); opening a URL;
  haptics (`frontend.md` Q-2).
- [ ] **AC-27** — Given the session needs a host service, then it asks
  the frontend with one message and gets one answer; it goes on running
  while it waits, and is not busy for that reason. A service the
  frontend does not have is not asked for.

### Calc-U-59
- [ ] **AC-28** — Given the Qt6 frontend of Calc-U-59, then it hosts a
  library app session on the GUI thread; the product has no session
  class and no emulation thread of its own (`frontend.md` FE-3; ADR
  0009). Swift and Web: C59-5, LIB-15.
- [ ] **AC-29** — Given a test frontend that keeps only the last message
  of each state topic and what each stream topic appended, then it holds
  everything the fourteen change flags of today announce
  (`Shared/SessionController.hpp:35-50` at the pin). The topics and
  their owners:

  | Change flag today | Arrives as | Owner |
  |---|---|---|
  | `ModelChanged` | control state | LIB-4 |
  | `DisplayChanged` | display topic | this feature |
  | `CueCardChanged` | product topic | C59-7 |
  | `PrinterChanged`, `PaperCut` | paper (stream topic, reset marker) | LIB-8 |
  | `CardChanged` | media state | LIB-7, C59-7 |
  | `RunStateChanged` | run state; full speed; playback (busy) | LIB-10, this feature |
  | `LiveChanged`, `CpuChanged`, `HeatmapChanged`, `LogChanged`, `AsmChanged`, `ListingChanged` | product topics | C59-4 |
  | `TraceChanged` | see the open point "Traces" in `features/INDEX.md` | LIB-10 |

- [ ] **AC-30** — Given what the frontends do for the session today
  through `SessionController::Platform` (`Shared/SessionController.hpp:57-90`),
  then after the migration: file access for cards and `ti58c.mem` is the
  library's (LIB-7, C59-3, C59-7); the trace location comes from LIB-9;
  the last used model is a setting (LIB-9) and the module ID is no
  longer saved (behaviour change 59-9); `error` is a notice and
  `diagnostic` a line of the log; `showDebugPanel` is a product message
  (C59-4); `runOnUiThread` has no successor.
- [ ] **AC-31** — Given the Swift frontend or the Web frontend goes to
  the background, then it sends suspend, and continue when it comes
  back; the Qt6 frontend sends neither and keeps running (ODD59-25).
- [ ] **AC-32** — Given a long piece of work in Calc-U-59 (loading a
  preset, a power cycle), then it runs as an operation: the busy topic
  says so and AC-15 and AC-16 hold. Today such work runs on the
  emulation thread without a busy signal (`frontend.md`, facts; whether
  this needs a row of its own: OQ-7).
- [ ] **AC-33** — Given Calc-U-59, then no machine has sound and no
  sound is asked for; the key click and haptics are the frontend's own
  (`frontend.md`, fit table).

### Calc-U-1600
- [ ] **AC-34** — Given the Qt6 frontend of Calc-U-1600, then it hosts a
  library app session on the GUI thread; the product has no session
  class of its own. Web: LIB-15.
- [ ] **AC-35** — Given the Qt6 frontend and an operation (a preset, a
  load, a power cycle), then it runs in parts, the busy topic says so
  while it runs, and the display and the paper keep updating; no
  operation blocks the GUI thread until it ends (`frontend.md` SP-2;
  today: `runNow`).
- [ ] **AC-36** — Given the Web frontend's tab is hidden, then it sends
  suspend, and continue when it is shown again; the Qt6 frontend keeps
  running in the background (`docs/inventory/divergence-matrix.md`,
  M17).
- [ ] **AC-37** — Given a Sharp machine at regular speed, then its
  buzzer is heard through the audio sink, and nothing is heard while the
  display is held for full speed or while an operation runs
  (`frontend.md`, facts: "Audio policy").
- [ ] **AC-38** — Given a test frontend as in AC-29, then it holds
  everything today's callbacks announce (`Shared/MachineSession.hpp:69-93`,
  `Shared/AppSession.hpp:62-84` at the pin). The messages and their
  owners:

  | Callback today | Arrives as | Owner |
  |---|---|---|
  | `romMissing` | the message of a product that cannot start (LIB-12 AC-5) | LIB-12 |
  | `notice`, `bayError`, `attachFailed` | notice, or the reply to the command | this feature |
  | `machineReplaced`, `modelSwitched`, `moduleChanged` | control state | LIB-4, LIB-18 |
  | `tapesChanged` | media state | LIB-7 |
  | `presetArmed` | display and control state sent during the operation (AC-15) | LIB-6 |
  | `presetLog` | log | this feature, LIB-6 |
  | `busyChanged` | busy topic | this feature |
  | `hostDirectories` | host services stated by the frontend (AC-26) | this feature |
  | `serialLinksRelinked` | product topic or notice | LIB-2 |
  | `machineAboutToChange`, `runDebuggerSlice` | no message: inside the library | LIB-10, LIB-18 |

## Edge Cases
- **EC-1** — Given a state topic that changes several times within one
  host call, then one message with the last state is sent.
- **EC-2** — Given a state topic that changes and changes back within
  one host call, then no message is sent (AC-5).
- **EC-3** — Given an operation fails halfway, then the busy topic ends,
  the reply and a notice say why, and the session accepts commands
  again.
- **EC-4** — Given suspend is sent while an operation runs, then the
  operation makes no progress until continue, and the busy topic stays
  (OQ-4).
- **EC-5** — Given an audio device that plays at another rate than the
  machine's sound output, then matching the two is the frontend's, as in
  the Calc-U-1600 Web frontend today (`Web/audio-worklet.js:10-13`).
- **EC-6** — Given a request arrives while an operation runs, then it is
  answered as usual (AC-16).

## Technical Requirements (optional)
Invariants checked in review and in the phase 5 walkthroughs:
- A session is never touched by two threads, and nothing crosses its
  boundary but messages (`frontend.md` I-1; ADR 0009).
- Everything a frontend asks of the session is a command or a request;
  it reads no state of the session any other way, and no change flag
  with a getter remains.
- The bridge and the web bindings only translate; no rule lives in them
  (`frontend.md` I-3; ADR 0014).
- Nothing in a generic message names a model-specific type
  (`frontend.md` I-4).
- A product writes no session class of its own; its app layer supplies
  the parts the map in `docs/contracts/README.md` lists (L-1).
- Frontends render; they decide no rule about what the machine offers
  (`frontend.md` FE-2).

## Open Questions
- [ ] **OQ-3** — When a command's effect goes out. A command can arrive
  between two host calls, and a suspended frontend may stop calling
  altogether. *Recommendation:* a command is carried out when it
  arrives, and its reply and the state topics it changed are sent at
  once; what the machine puts out is sent with the host calls. AC-4
  then holds for a given order of commands and calls.
- [ ] **OQ-4** — Suspend while an operation runs (EC-4): today's
  behaviour is not recorded for either product. *Recommendation:* as in
  EC-4.
- [ ] **OQ-5** — What a frontend that starts to listen late is sent of a
  stream topic (AC-6 covers state topics). *Recommendation:* the paper
  and the log from their last reset marker on, so that a panel opened
  later shows all of it; notices not at all, because a notice is about
  the moment.
- [ ] **OQ-6** — "Suspend" and "continue" for the glossary, as defined
  above. `frontend.md` says "pause and resume" for both the background
  and the debugger; the glossary gives those words to the debugger.
- [x] **OQ-7** — *Decided (user, 2026-11-10, batch 5 gate): "Loading…" is added to row 59-5.* Calc-U-59 shows "Loading…" and refuses other operations
  while a preset loads at full speed or a power cycle runs (AC-32).
  Behaviour change 59-5 covers the speed of a preset and the dropped
  keys, not this. *Recommendation:* add it to row 59-5; it is new for a
  Calc-U-59 user, though short.
- [ ] **OQ-8** — The Calc-U-59 topic behind `RunStateChanged` carries
  five things today (freeze, armed freezes, full speed, playback, held
  key), and "live" has no generic meaning. *Recommendation:* run state
  and the armed freezes go to LIB-10 and C59-4; full speed becomes part
  of a small state topic of this feature next to busy; playback is busy;
  the held key is the typing engine's (LIB-5).

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| Sessions are single-threaded, hosted by the frontend, and reached by messages only | ADR 0009, 0015; `frontend.md` I-1 | 2026-11-09 |
| The app session and the machine session are library parts; a product writes no session of its own | `docs/contracts/README.md` L-1 (user) | 2026-11-10 |
| The app session pushes state by topic; a frontend never pulls | `frontend.md` FE-1 | 2026-11-10 |
| Three shapes: state topic, stream topic, request | `frontend.md` FE-5 | 2026-11-10 |
| Product topics, commands and control entries travel the same way; the library carries them unread | `frontend.md` FE-6 | 2026-11-10 |
| Operations always run in parts, one per host call, with live updates; `runNow` goes | `frontend.md` SP-2 (user), I-5 | 2026-11-10 |
| An operation runs at full speed unless it asks for regular speed | `presets.md` PR-4 | 2026-11-10 |
| While an operation runs, commands that would start another are refused; everything else goes through | `frontend.md` Q-1 | 2026-11-10 |
| The Qt6 frontends keep their session on the GUI thread for the migration | `frontend.md` FE-3 | 2026-11-10 |
| Stopping in the background stays per frontend as today; the session decides nothing about the host | `frontend.md`, rules from decisions already taken; ODD59-25 | 2026-11-10 |
| The app session owns the busy state and the audio policy | `frontend.md`, responsibilities (adopted from Calc-U-1600) | 2026-11-10 |
| Proposed in this spec, for the gate: a frontend that starts to listen is sent every state topic once (AC-6) and states its host services (AC-26); a suspend and a debugger pause do not end each other (AC-25); a host service is asked with one message and answered with one, without making the session busy (AC-27) | AC-6 follows from FE-1 ("never asks back"); the others make the seam checkable and no contract states them | 2026-11-10 |
| Every command gets a short reply: done, or refused with the reason | user, 2026-11-10, batch 2 gate | 2026-11-10 |
| Proposed with LIB-6 and LIB-14 and accepted (user, 2026-11-10, batch 5 gate): the busy topic says whether the running operation is at full speed or at regular speed now (AC-14) | `presets.md` PR-4: "Loading…" only while it runs at full speed | 2026-11-10 |
