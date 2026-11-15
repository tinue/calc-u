---
name: calc-u-59-swift
description: Use this skill when working on the Swift/SwiftUI code of Calc-U-59 (frontends/swift/App/, frontends/swift/App/Views/, frontends/swift/Bridge/). Covers EmulatorViewModel, the ObjC++ bridge, display, keyboard, debug panels, printer, card reader, settings, and state file I/O. Trigger on any task that touches frontends/swift/App/*.swift, frontends/swift/App/Views/*.swift, frontends/swift/Bridge/, or the Xcode project.
user-invocable: true
---

# Calc-U-59 Swift GUI

This skill covers all work inside `frontends/swift/App/`, `frontends/swift/App/Views/`, and `frontends/swift/Bridge/`.

- App structure, files, threading, bridge: `reference/AppArchitecture.md`.
- Everything that isn't drawing (run loop, keys, files, debugger, drawing
  rules): `reference/SharedArchitecture.md` and the calc-u-59-shared skill.
- Freeze / step and the debug snapshots: `reference/DebugAPI.md`.
- TI-59 background: the calc-u-59-core skill's Domain Primer. SwiftUI
  practice: the swiftui-pro skill.

## Rules

1. **No non-GUI logic in Swift.** Parsing, file formats, timing, key
   sequencing, snapshot building and the numbers the views draw with belong
   in `app/`, reached through `EmulatorViewModel` → `TIAppSession` (the app
   session) or a `frontends/swift/Bridge/` wrapper. Views are thin bindings
   over `EmulatorViewModel`.
   **The session runs on its own thread** (`cu59::SessionThread`, C59-5):
   the bridge posts commands (`c59.*`, `app/TIAppPart.hpp`) and hands each
   batch of messages to the main thread; `EmulatorViewModel.apply` copies
   what a batch changed. Nothing in Swift reads the session or runs the
   machine; a result comes back as a reply (a completion block) or a topic.
   A new value the views need is a field of an interim topic payload in
   `TIAppPart` plus a getter in `TIAppSession` that reads the last message.
   **Harness attach (Debug, macOS and simulator only):** the app opens its
   session on 127.0.0.1, port `--attach-port N`, else 59059; `calcu attach
   [--port N]` drives it (`library/harness/attach/`; one port per simulator).
   Release and device builds have no attach (`CALCU_ATTACH` off;
   `Calc-U-59-Debug.entitlements` is the Debug configuration's only).
   `check_no_attach.sh` checks it: `--sources` in Frequent, `--binary` as a
   build phase of every Release and device build, the full build check in
   Longer. The XCFramework phase must keep its declared output, or a Release
   build after a Debug one links the Debug XCFramework (C59-5 qa-report).
   The Mac app's App Nap activity is `.latencyCritical`: without it a covered
   window loses emulated time (AC-9).
2. **Parity.** A user-visible change here needs the matching Qt change, and
   the web change if the feature is in the web's scope, on the same branch —
   or a `Pending (Qt)` / `Pending (Web)` row in `reference/FrontendParity.md`.
   Update the table either way (CLAUDE.md § "Frontend Parity").
3. **Bridge wrappers translate types only.** Per-frame work that walks
   registers or program memory runs in C++ and crosses the bridge once as a
   finished result — never 100 bridge calls per 60 Hz frame.
4. **Guard `@Observable` writes** (`reference/AppArchitecture.md` § "60 Hz refresh").
5. **Keep the Dynamic Type cap.** `CalculatorView.body` limits Dynamic Type
   to `.small ... .large`. Wider sizes break the layout (the TI-59 card bar
   first).
6. **Compile, don't run.** Never launch the simulator or the built Mac app;
   ask the user to, saying exactly what to look for.
7. **Commit often, never push.**
8. **Cite sources.** A hardware spec or external document gets its https URL
   (or local reference file) in the comment or doc.

## Build Verification

Every change needs a macOS build:

```bash
xcodebuild -project frontends/swift/Calc-U-59.xcodeproj -scheme "Calc-U-59" \
  -destination "platform=macOS" build
```

Build for iOS only when a task or test explicitly needs it, and only on a
simulator from `bin/simulators.yaml`. `bin/simulators status` lists them with
their UDIDs; never hard-code a model name from memory.

```bash
bin/simulators status    # pick a device, e.g. the iPhone 18 Pro Max
xcodebuild -project frontends/swift/Calc-U-59.xcodeproj -scheme "Calc-U-59" \
  -destination "platform=iOS Simulator,id=<UDID>" build
```

The Xcode build first builds `CalcU59.xcframework` (the library, `core/`,
`app/`) through CMake: it needs CMake and Ninja, and Xcode compiles only
Swift and the bridge (`reference/AppArchitecture.md` § "Build").

A change to `core/` or `app/` also needs the CMake build and tests
(calc-u-59-shared skill).

Tests of the bridge and the session thread from Swift: the unit-test target
`Calc-U-59Tests` (plan `UnitTests`, calc-u-59-uitesting skill). After a build
that failed because the XCFramework was missing, Xcode may keep saying so
for one test plan: delete `Build/Intermediates.noindex/XCBuildData` in the
derived data.
