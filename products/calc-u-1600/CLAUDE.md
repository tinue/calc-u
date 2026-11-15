# Calc-U-1600

Calc-U-1600 lives in `products/calc-u-1600/` of the source repository tinue/calc-u (ADR 0002). Work happens on short branches merged into `main` (ADR 0029); a release is tagged `calc-u-1600-vX.Y.Z` (ADR 0028). The repository's `CLAUDE.md` holds the current phase and its rules; they come first.

Before removing, simplifying or "fixing" code in a review or cleanup, read `docs/background/Decisions.md`. It lists deliberate choices that look wrong at first sight. When a recurring review question gets settled, add it there.

## Test builds

All C++ builds go through the repository's root CMake project (ADR 0003; options `CALCU_PRODUCT_59`, `CALCU_PRODUCT_1600`, `CALCU_FRONTEND_QT`, `CALCU_TESTS`), the Emscripten ones too (`build/web`); this directory does not configure on its own. Run the test suite optimized: `tools/run_tests.sh` (`-O1`, the repository's `build/tests` tree) or the `CoreTests` target of any root tree (`-O1` in Debug). When chasing a bug — a debugger, stepping, inspecting variables — first build a full-debug tree and work there, leaving the other trees as they are:

```sh
cmake -S ../.. -B ../../build/debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCALCU_FRONTEND_QT=OFF -DCALCU_TESTS_DEBUG=ON
cmake --build ../../build/debug --target CoreTests
```

A test file registers its tests with `TEST(name)` of the repository's one runner, `library/testing/TestHarness.hpp` (ADR 0038; `CHECK`, `CHECK_EQ`, `SKIP_UNLESS` for a missing ROM); there is no list of tests to keep. `tools/run_tests.sh <substring>` runs the tests whose name contains it. Every CTest entry is in a test group (ADR 0031; `docs/test-groups.md` at the repository root): a tree has the Frequent group (`CoreTests`, `CalcU1600CheckPresets`; ≤ 60 s), and with `-DCALCU_TESTS_LONGER=ON` / `-DCALCU_TESTS_CHECKPOINT=ON` also the Longer and Checkpoint ones (`ctest -L longer`).

## Harness

`calcu` is the family's harness (LIB-11, `features/LIB-11-harness/` at the repository root): the product without a window, on the library's app session, in emulated time only, so two runs give the same replies. Build the target `calcu` of a root tree (`cmake --build ../../build/tests --target calcu`; the program is `build/tests/library/harness/cli/calcu`). It finds the ROMs in `roms/` through the product's lookup (`sharp::lookUpRoms`), the module definitions in `frontends/qt6/resources/cards/`, and starts the startup model (the PC-1500A under the default settings) cold booted, at key readiness (LIB-18).

- One-shot: `calcu --product 1600 wait 2s then lcd then ticks` (commands parted by `then`; exit status 0, 1 at the first refusal, 2 when it cannot start).
- `calcu --product 1600 serve`: one command per line on standard input, one reply per line (`done`, `done <n>` and n lines, `refused: <reason>`); `--json` gives one object `{"done","result","reason"}` per reply. From Python: `library/harness/python/calcu_serve.py`.
- First commands: `wait <2s|500ms>`, `key-down`/`key-up <key ID>`, `lcd` (rows of text, `indicators:`, `not text:`; no cursor cell yet), `display`, `state <topic>`, `capabilities`, `ticks`, `audio <file.wav>`, `quit` (`calcu --help`).
- Machine control (LIB-18): `model <PC-1500|PC-1500A|PC-1600>`, `variant <A01|A03|A04|New|Old>`, `attach <place> <peripheral>[:<variant>]` (`attach printer CE-1600P:old`), `detach <place>`, `reset`, `cold-boot`, `power-cycle`; each replies when the machine is ready for keys again. No settings or save folder of yours is touched unless `--settings`, `--save-folder`, `--output-folder` name them.
- Typing (LIB-5): `key <name> ...` (key IDs and `break` for `on`, any case; replies when the last key is up) and `type <text>` (the text and ENTER; replies when ENTER is up). Neither waits until the machine is idle: `wait` does.
- Not yet: `pc1600 …` commands, presets, media, memory dumps (later features). Until M-51, `pc1500_cli`, `pc1600_cli` and the probes stay the tools for those.

## Core, Shared, frontends

Three layers (docs/developer/Shared-Architecture.md): `core/` is the hardware, `app/` all app logic that isn't drawing, `frontends/qt6/app/` the desktop GUI, `frontends/web/` the WebAssembly browser GUI.

- Non-GUI logic goes in `app/` (or `core/`), never in `frontends/qt6/app/`; Qt classes translate types and show what Shared decides.
- Above `core/`, the machine is only a `calcu::machine::Machine` (C1600-1): built with `calcu::machine::build(sharp::sharpCatalogue(), SharpMachineBuilder, request)`; Sharp-only functions are capabilities (`c1600.*` kinds, `core/Machine/SharpProductKinds.hpp`); what has no seam yet goes through `core/Machine/LegacyCalls.hpp` (adapters, each with the step that removes it). Every ROM image comes from the one ROM lookup at the start (`sharp::lookUpRoms`, `core/Machine/SharpRoms.hpp`, LIB-12) and its `calcu::roms::RomSet`; nothing else reads a ROM file (the CTest `CalcU1600NoOwnRomLookup`; the one exception is the host-drive ROM, `sharp::readHostDriveRom`). Descriptors, kinds and adapters live in `core/Machine/`. Never include or name `PC1500Machine`/`PC1600Machine` in `app/`, `frontends/`, `tools/` or `dev/`: the CTest `CalcU1600MachineIncludeCheck` (`tools/check_machine_includes.sh`, Frequent and CI) fails.
- Core and Shared: portable C++20, standard library only, single-threaded, never blocking; the machines hold no lock (ADR 0009). Long machine work is a resumable job (`core/Job.hpp`: `co_await job::run(machine, n)`, the library's `calcu::sessions::Task`); coroutine parameters by value.
- Every frontend hosts the library's app session (`library/sessions/`, LIB-3) with the product's part `app/SharpAppPart` (the machine, the bays, the Sharp operations as `c1600.*` commands, `app/SharpTopics.hpp`) and talks to it by messages; pacing is the library's (LIB-17, `app/PacingProfiles`). The product writes no session class of its own (CTest `CalcU1600NoOwnSession`), and an operation never runs to its end on a frontend's thread: it runs in parts, one per host call. `app/SharpSession` (`makeSharpSession`) builds the session for a native host, which talks to it through the library's `calcu::sessions::Listener` (Qt6 `MachineController`, the tests' `app/tests/SessionFixture.hpp`, `tools/wasm_smoke`); the Web engine builds the session in `frontends/web/bindings.cpp` for the library's worker, with `app/SharpWebPart` for what the page draws. Keys and typing go through the library's typing engine (LIB-5; the profiles and keyboard maps in `app/SharpTyping`, the frontends send `host-key-down`, `screen-key-down`, `paste`, ...); the debugger reaches the machine through the adapter of `app/SharpMachineParts` until M-45.
- A change to `core/` or `app/` passes `tools/run_tests.sh` and `tools/build_wasm_check.sh`, and the Qt app builds; a change the web client sees also passes `frontends/web/build.sh` + `frontends/web/run-tests.sh`.

