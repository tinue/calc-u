---
name: calc-u-59-shared
description: Use this skill when working on the shared C++ layer of Calc-U-59 (app/, tests/, the product's CMakeLists.txt, tools/goldens/) — the non-GUI logic the Swift app, the Qt6 app and the web client use: file formats, state files, cue cards, the app session's product part (TIAppPart over SessionController, makeAppSession), the Swift frontend's session thread (SessionThread), the emulator runtime (machine session, freeze state, key sequencing, frame sampling), the debugger model (DebugModel), debug snapshots, trace writer, view rules. Also trigger when adding a frontends/swift/Bridge/ wrapper for a Shared module, or when logic is about to be written in frontends/swift/App/*.swift, frontends/qt6/app/ or frontends/web/ that isn't drawing.
user-invocable: true
---

# Calc-U-59 Shared C++ Layer

Architecture, module list, threads, run loop, key input, file flows and
drawing rules: `reference/SharedArchitecture.md`. Don't restate them here.

## Rules

1. **Shared is the home of all non-GUI logic.** If code would parse, encode,
   time, sequence, build a snapshot or compute a layout number in Swift, Qt
   or the web client's JS, it belongs here instead (CLAUDE.md § "Frontend Parity").
2. **C++20, standard library only** (ADR 0005). No Foundation, Qt or
   platform APIs. CMake builds it as C++20, Xcode as gnu++20.
3. **No locations.** Take `std::filesystem::path`, never look up iCloud,
   Documents, bundles or qrc. ROM files come only through the library's ROM
   lookup (LIB-12): a frontend makes a `calcu::roms::Bundle` of its packaging
   and calls `cu59::lookUpRoms` (`app/ProductRoms.hpp`) once at its start,
   then `cu59::makeAppSession` (`app/TIAppPart.hpp`); the app layer then builds and
   loads modules from the `RomSet` and reads no ROM file. Nothing else opens
   a ROM file (CTest `CalcU59NoOwnRomLookup`). Without the full set no
   session is made and the frontend shows the lookup's text.
4. **All builds before every commit** (commands: CLAUDE.md § "Frontend Parity").
   (The Qt app builds as part of the product's CMake build when Qt is installed;
   the web build needs `emcc` — calc-u-59-web skill.)
5. **One thread per session, no other.** The Swift and Qt6 frontends host
   the library's app session (`cu59::makeAppSession`; Qt6 on its GUI thread,
   Swift on `SessionThread`, the only `std::thread` in `app/`, never in the
   Web build). The app layer inside it is single-threaded: no thread, no
   wait, no lock that matters (ADR 0009). Everything a frontend does is a
   command (`cu59::commands`), everything it gets a message (the interim
   `c59.*` topics, `app/TIAppPart.hpp`). Until M-34 the Web frontend still
   runs `SessionController` itself in `Threading::Cooperative`
   (`CooperativePacing`, 2.3.0's batches); `TIAppPart` runs the same
   controller in `Threading::Hosted`. `frontends/web/bindings.cpp` is the
   Web's adapter until then.
6. **Test every module** in `tests/` (the repository's one runner,
   `library/testing/TestHarness.hpp`: `TEST`/`CHECK`/`SKIP_UNLESS`, ADR 0038;
   `TestRoms.hpp` boots machines from `roms/`). Tests run from the product
   directory.
7. **Commit often, never push.**

## Adding a module

1. `app/Foo.{hpp,cpp}` in namespace `cu59`, with a header comment saying
   what it is and where its format/rules are documented. CMake picks up new
   `.cpp` files by glob; the Swift app gets them through
   `CalcU59.xcframework`, which the Xcode build makes first (ADR 0013).
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
5. Add the module to `reference/SharedArchitecture.md`, and a new wrapper to
   `reference/AppArchitecture.md` § "Bridge Layer".

## Porting Swift logic (goldens)

When moving existing Swift logic here, capture its behaviour first:
`tools/goldens/` holds small Swift drivers compiled together with the Swift
original (read from a pinned git commit) that print a golden file into
`tests/golden/`; the C++ test compares against it. See
`tools/goldens/README.md`. Then delete the Swift implementation — no
forwarding shims.
