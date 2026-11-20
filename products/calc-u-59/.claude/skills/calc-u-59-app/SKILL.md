---
name: calc-u-59-app
description: Use this skill when working on the app layer of Calc-U-59 (the C++ app logic) (app/, tests/, the product's CMakeLists.txt, tools/goldens/) — the non-GUI logic the Swift app, the Qt6 app and the web client use: file formats, presets (the TI rules and keywords of the library's loader: TIPresetRules, TIPresetKeys, Keycodes, the c59 commands), the TI-58C constant memory file (ConstantMemoryKeeper), magnetic cards on the library's media part (TIMagneticCardKind, TIMagneticCardsKey, the card reader place of the core), cue cards and module cue cards (ModuleCueCards), the app session's product part (TIAppPart, makeAppSession, the library's parts it owns, TIProductMachines; TIWebPart for the Web page), the Swift frontend's session thread (SessionThread), the emulator runtime (machine session, key sequencing, frame sampling), the TI's side of the library's paper (TIPrinters) and debugger (TILevelRules, TITraceRecorder, TIDebuggerSetup), the debug panel (TIDebugPart: its product topics, commands and entries; the ASM overlay reader AsmOverlay), the file kinds of the library's open (TIFileKinds), the debugger model (DebugModel), debug snapshots, the trace records (TraceWriter), view rules. Also trigger when adding a frontends/swift/Bridge/ wrapper for an app-layer module, or when logic is about to be written in frontends/swift/App/*.swift, frontends/qt6/app/ or frontends/web/ that isn't drawing.
user-invocable: true
---

# Calc-U-59 App Layer

Architecture, module list, threads, run loop, key input, file flows and
drawing rules: `reference/AppLayer.md`. Don't restate them here.

## Rules

1. **The app layer is the home of all non-GUI logic.** If code would parse, encode,
   time, sequence, build a snapshot or compute a layout number in Swift, Qt
   or the web client's JS, it belongs here instead (CLAUDE.md § "Frontend Parity").
2. **C++20, standard library only**. No Foundation, Qt or
   platform APIs. CMake builds it as C++20, Xcode as gnu++20.
3. **No locations.** Take `std::filesystem::path`, never look up iCloud,
   Documents, bundles or qrc. ROM files come only through the library's ROM
   lookup: a frontend makes a `calcu::roms::Bundle` of its packaging
   and calls `cu59::lookUpRoms` (`app/ProductRoms.hpp`) once at its start,
   then `cu59::makeAppSession` (`app/TIAppPart.hpp`); the library's machine control
   then builds every machine and loads modules from the `RomSet` and reads
   no ROM file. Nothing else opens
   a ROM file (CTest `CalcU59NoOwnRomLookup`). Without the full set no
   session is made and the frontend shows the lookup's text.
4. **All builds before every commit** (commands: CLAUDE.md § "Frontend Parity").
   (The Qt app builds as part of the product's CMake build when Qt is installed;
   the web build needs `emcc` — calc-u-59-web skill.)
5. **One thread per session, no other.** Every frontend hosts the library's
   app session (`cu59::makeAppSession`; Qt6 on its GUI thread, Swift on
   `SessionThread`, the only `std::thread` in `app/`, never in the Web
   build; the Web in the library's Web Worker through
   `frontends/web/bindings.cpp`). The app layer inside it is
   single-threaded: no thread, no wait, no lock. Everything a
   frontend does is a command (`cu59::commands`, the Web's `c59.web.*`),
   everything it gets a message (the product topics `c59.calc-indicator`,
   `c59.cue-card`, `app/TIAppPart.hpp`, compared by value; the PC-100C's
   controls and TRACE are the library's `control-state` `controls`:
   set them after a flow, the session sends what changed; the Web page's
   `c59.web.*`, `app/TIWebPart.hpp`, derived from those values). A command
   name has one owner: the session refuses a second declaration
   (`set-control` is the library's; TRACE's kept position goes into an attached
   PC-100C through `TIProductMachines::attaching`).
   What Calc-U-59 gives the library's shell (the menus' product items, the About
   texts and links, the settings dialog per frontend and layout, the
   faceplate's hit map) is `app/TIShellProduct`, one source for every
   frontend (Swift through `Bridge/TIShell`, its settings
   per system `ShellFrontend::SwiftMac`/`SwiftIOS`); a new menu item or
   settings row goes there. Without the
   full ROM set the session is made with the lookup's result
   (`TIAppSetup::roms`) and no machine; the library's StartupPart publishes
   `roms-missing`. The places (save folder, traces, screenshots, `~/`)
   are the host's `calcu::settings::Paths` (`TIAppSetup::paths`).
   `TIAppPart` owns the library's parts of its session
   (`parts::LibraryParts`; `sessionParts()`): typing, peripherals,
   presets on `TIPresetRules` and the bundle of `examples/`
   (`examples/bundle.yaml`), the start, model switch, Reset, Cold boot,
   attach and detach on `TIProductMachines`, the library's `PaperPart`,
   `DebuggerPart`, `MediaPart` (the magnetic card, `app/TIMagneticCardKind`;
   the TI-59's place `card-reader`; slot-1's module listing,
   `tiModuleListing`; the save folder through the host's `media::FileAccess`, `TIAppSetup::files`;
   a card file opened goes through the open part to the library media part), `OpenPart` with the file kinds of
   `app/TIFileKinds`; then `ProgramMemoryPart` (`c59.partition`,
   `c59.program`, `c59.data-registers`), `TIDebugPart` (the debug panel's six
   product topics, its `c59.*` commands and the control state's product entries)
   and the part. Every file a frontend is handed goes to `open` (a drag:
   `open-would-take`). A frontend starts, switches,
   resets and loads with the library's commands (`start`, `model`, `reset`,
   `cold-boot`, `attach`, `detach`, `load-preset`), never with product
   commands; the machine is the machine session's, and the part follows it
   at `machineChanged` (a new `EmulatorSession`). No freeze state, no
   printer lines, no trace writer and no preset reader in `app/`
   (`reference/AppLayer.md`). Tests drive them
   on the session (`tests/SessionRig.hpp`: `SessionRig`, and `PartRig` with
   the constant memory file in memory; `tests/CardRig.hpp` for cards, with a
   save folder of its own; `tests/AppRig.hpp` for the start and the
   constant memory; `tests/TIPresetRig.hpp` for a preset read and the
   harness entry in virtual time; on the library's `TestFrontend` and
   `Pump`, `library/testing/`).
6. **Test every module** in `tests/` (the repository's one runner,
   `library/testing/TestHarness.hpp`: `TEST`/`CHECK`/`SKIP_UNLESS`;
   `TestRoms.hpp` boots machines from `roms/`). Tests run from the product
   directory.
7. **Commit often, never push.**

## Adding a module

1. `app/Foo.{hpp,cpp}` in namespace `cu59`, with a header comment saying
   what it is and where its format/rules are documented. CMake picks up new
   `.cpp` files by glob; the Swift app gets them through
   `CalcU59.xcframework`, which the Xcode build makes first.
2. A test in `tests/`.
3. Swift access: usually through the app session: a `c59.*` command or a
   field of an interim topic in `app/TIAppPart.*`, translated by
   `frontends/swift/Bridge/TIAppSession`; a pure module (a view rule, a
   format) gets an ObjC++ wrapper in
   `frontends/swift/Bridge/` (type translation only), imported from
   `frontends/swift/Bridge/Calc-U-59-Bridging-Header.h`. C++-only helpers live in
   `frontends/swift/Bridge/TIFileFormats+Cpp.h` and `frontends/swift/Bridge/BridgeConvert.h` (struct/string
   conversions); never import those from the bridging header.
4. Qt access: call it directly.
5. Add the module to `reference/AppLayer.md`, and a new wrapper to
   `reference/AppArchitecture.md` § "Bridge Layer".

## Porting Swift logic (goldens)

When moving existing Swift logic here, capture its behaviour first:
`tools/goldens/` holds small Swift drivers compiled together with the Swift
original (read from a pinned git commit) that print a golden file into
`tests/golden/`; the C++ test compares against it. See
`tools/goldens/README.md`. Then delete the Swift implementation — no
forwarding shims.
