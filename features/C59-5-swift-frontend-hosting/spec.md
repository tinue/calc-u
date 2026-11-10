# C59-5: Swift frontend hosting

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: LIB-3 (the app session and its messages), LIB-11 (the serve
  protocol that attach uses).
- Contracts and decisions: `docs/contracts/frontend.md` (I-1 to I-3,
  FE-3), `docs/contracts/harness.md` (H-3, H-4, I-3); ADR 0003, 0013,
  0014, 0015, 0033; `Briefing.md` §8 (the Swift apps are built locally;
  iOS 18, macOS 15.6).
- Facts: Calc-U-59 at `v2.3.0`; every `path:line` below is at that tag.

## User Stories
- As a user of Calc-U-59 on an iPhone, iPad or Mac, I want the app to
  stay smooth while the machine runs at full speed or a preset loads.
- As the developer, I want the C++ code of the Swift apps built by the
  same build as everything else, so that its flags and its list of files
  exist once.
- As the developer, I want the bridge to be a thin layer that only
  translates, so that no behaviour hides in it.
- As an agent, I want to drive the running app in the iOS simulator and
  on the Mac with the harness, because automating its window often
  fails.
- As the developer of a later Swift frontend for Calc-U-1600, I want to
  reuse this way of hosting.

## Out of Scope
- What the Swift frontend shows: the family menu and control-bar
  structure, pickers and panels in SwiftUI (owner: C59-8).
- The app session and its messages (owner: LIB-3); the commands and the
  protocol of the harness (owner: LIB-11).
- The view data Calc-U-59's app layer supplies to all its frontends (key
  layout, LED geometry, printer geometry, cue-card layout): it stays in
  the app layer and is carried over.
- The version number and where it is defined (ADR 0027; the migration
  plan).
- Signing, notarising, TestFlight and the App Store (`Briefing.md` §8;
  release tooling).
- The XCUITest targets and the screenshot tooling (ADR 0032; C59-8 for
  the scenarios).
- Attach for the Qt6 and Web frontends (`harness.md` H-4: later).

## Acceptance Criteria

One build for the C++ code
- [ ] **AC-1** — Given the source repository, when the root CMake
  project is built for Apple platforms, then it produces one XCFramework
  with the library, the Calc-U-59 core and the Calc-U-59 app layer, for
  macOS, iOS devices and the iOS simulator (ADR 0013).
- [ ] **AC-2** — Given the Xcode project of the Swift frontend, then it
  compiles only Swift and the bridge and links that XCFramework; no file
  of the library, the core or the app layer is compiled by Xcode. Today
  Xcode compiles `Core/` and `Shared/` itself from synchronized folders,
  with its own setting for the C++ standard
  (`Calc-U-59.xcodeproj/project.pbxproj:79-105, 346`).
- [ ] **AC-3** — Given a change in the library, the core or the app
  layer, when the app is built from Xcode, then the change is in the
  app: building the XCFramework is a step of the Xcode build or one
  documented command before it, and a stale XCFramework is not linked
  without a warning.
- [ ] **AC-4** — Given the built apps, then they run on iOS 18 and later
  and macOS 15.6 and later, as today (`project.pbxproj:499-517`).

Session on a background thread
- [ ] **AC-5** — Given the Swift frontend, then its app session runs on
  one background thread of its own, which calls the session about 60
  times a second; the main thread never runs the machine. Today the main
  run loop calls the controller 60 times a second and a separate thread
  per session runs the machine
  (`App/EmulatorViewModel.swift:220-228`,
  `Shared/EmulationRunner.hpp:15-25`).
- [ ] **AC-6** — Given the session thread is busy for a whole call (full
  speed, a part of an operation), then the main thread still takes
  touches and redraws; a key pressed then reaches the session as a
  message and is not lost (ADR 0015).
- [ ] **AC-7** — Given the Swift frontend, then everything it sends to
  the session is a command or a request, and everything it gets is a
  message, both through a queue between the two threads; it reads no
  state of the session directly (`frontend.md` I-1, I-2).
- [ ] **AC-8** — Given the same commands as in the Qt6 frontend, then
  the Swift frontend receives the same messages in the same order
  (LIB-3 AC-4).

The bridge
- [ ] **AC-9** — Given the Objective-C++ bridge, then it only translates
  commands, requests and messages, and the app layer's view data,
  between Swift types and the types of the library and the app layer; it
  decides nothing and keeps no state of the machine (ADR 0014; today's
  rule, `CLAUDE.md:65`).
- [ ] **AC-10** — Given what the Swift frontend does for the session
  today through `TIPlatform` (`Bridge/TISessionController.mm:166-201`:
  queueing work on the main thread, reading and writing card files and
  `ti58c.mem`, the trace location, saving the last model and the module,
  errors, change flags), then after this feature each is either a
  message of LIB-3, a host service (LIB-3 AC-26), or done by the library
  (LIB-3 AC-30); nothing of it remains in the bridge.

Background and foreground
- [ ] **AC-11** — Given the app goes to the background on iOS or macOS,
  then it sends suspend and the session thread stops calling; given it
  becomes active again, then it sends continue (LIB-3 AC-24, AC-31;
  today `App/Calc-U-59App.swift:17-26`).
- [ ] **AC-12** — Given the Mac app is running in a window that is not
  in front, then the machine keeps running at its pace, as today
  (`App/EmulatorViewModel.swift:188-202`).

Harness attach
- [ ] **AC-13** — Given a development or test build of the Swift
  frontend, on macOS or in the iOS simulator, then it opens its app
  session to the serve protocol on a TCP port bound to localhost, and
  `calcu` on the same Mac can attach to it (`harness.md` H-3, H-4; ADR
  0033).
- [ ] **AC-14** — Given `calcu` is attached, then every command of the
  harness works on the running app's session as in `calcu serve`, and
  what a command changes shows in the app's window.
- [ ] **AC-15** — Given `calcu` is attached, then it is a second
  listener of the session: when it starts to listen it is sent every
  state topic once (LIB-3 AC-6), and the app's own frontend goes on
  receiving its messages.
- [ ] **AC-16** — Given a release build (the App Store, TestFlight, the
  notarised DMG), then it contains no attach: it opens no port and does
  not ask for the network (`harness.md` I-3). Today the app has no
  network entitlement at all (`project.pbxproj:470-473`).
- [ ] **AC-17** — Given an attached harness and the app in the
  foreground, when the harness sends `wait`, then emulated time passes
  as the app paces it, at regular speed or at full speed as commanded,
  and the reply comes when it has passed; the harness does not take the
  session away from the wall clock (`harness.md` I-2: "without attach,
  the harness runs in virtual time").

## Edge Cases
- **EC-1** — Given the port for attach is already in use, then the app
  starts all the same, and its log says that attach is not available.
- **EC-2** — Given an attached harness goes away without `quit`, then
  the app goes on as if it had never been attached.
- **EC-3** — Given the app is suspended while a harness is attached,
  then a command waits until the app continues, or is refused with that
  reason (OQ-3).
- **EC-4** — Given a second harness tries to attach while one is
  attached, then it is refused.

## Technical Requirements (optional)
- A session is never touched by two threads (`frontend.md` I-1). The
  core's locks and the threaded runner of today
  (`reference/CoreArchitecture.md:834-850`,
  `Shared/EmulationRunner.hpp:95-99`) have no successor.
- The build location of the Swift apps does not change: they are built
  locally (`Briefing.md` §8).
- Two spikes are noted for phase 5 and come before the design of this
  feature: the session on a background thread, and attach in the iOS
  simulator.
- The Swift frontend's own copies of model facts
  (`App/MachineModel.swift:16-33`) go with LIB-4 and C59-8, not here.

## Open Questions
- [ ] **OQ-1** — How `calcu` finds the app's port (AC-13): a fixed port,
  a port given as a launch argument, or a port the app writes somewhere.
  Several simulators can run at once. *Recommendation:* a launch
  argument names the port, with a fixed default; decide after the
  phase 5 spike.
- [ ] **OQ-2** — What makes a build a development build (AC-13, AC-16):
  the Debug configuration, or a setting of its own? The sandboxed Mac
  app needs the entitlement for incoming connections to listen at all.
  *Recommendation:* the Debug configuration only, with its own
  entitlements file; Release has none.
- [ ] **OQ-3** — Attach while the app is suspended (EC-3).
  *Recommendation:* the command is refused with the reason; a test that
  needs the app keeps it in the foreground.
- [ ] **OQ-4** — Does the attached harness replace the launch arguments
  and the hidden controls the UI tests use today (`--UITesting`, a
  hidden R/S button, an accessibility element for the state of a
  preset, `App/Views/CalculatorView.swift:84-147`)?
  *Recommendation:* yes, once attach works; they are removed with C59-8.
- [ ] **OQ-5** — AC-12: the Mac app holds off App Nap for as long as it
  runs today, while it also suspends when the scene goes to the
  background. Whether macOS ever reports the background for a window
  that is merely covered is not verified. *Recommendation:* verify on
  2.3.0, then word AC-11 and AC-12 exactly.

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| The Swift frontend links an XCFramework built by the root CMake project | ADR 0013, 0003 | 2026-11-09 |
| The bridge translates only | ADR 0014 | 2026-11-09 |
| The Swift frontend hosts its session on a background thread; the bridge is a message boundary | ADR 0015 | 2026-11-09 |
| In development and test builds a frontend can open its app session to the serve protocol; release builds leave it out | ADR 0033; `harness.md` I-3 | 2026-11-09 |
| Attach is built for the Swift frontend (iOS simulator and macOS) in the migration; localhost TCP, line-based text | `harness.md` H-3, H-4 | 2026-11-10 |
| Proposed in this spec, for the gate: an attached harness is a second listener and does not take the session off the wall clock (AC-15, AC-17); one harness at a time (EC-4) | `harness.md` says virtual time holds "without attach" and leaves the attached case open | 2026-11-10 |
