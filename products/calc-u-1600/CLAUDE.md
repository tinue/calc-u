# Calc-U-1600

Calc-U-1600 lives in `products/calc-u-1600/` of the source repository tinue/calc-u (ADR 0002). Work happens on short branches merged into `main` (ADR 0029); a release is tagged `calc-u-1600-vX.Y.Z` (ADR 0028). The repository's `CLAUDE.md` holds the current phase and its rules; they come first.

Before removing, simplifying or "fixing" code in a review or cleanup, read `docs/background/Decisions.md`. It lists deliberate choices that look wrong at first sight. When a recurring review question gets settled, add it there.

## Test builds

All C++ builds go through the repository's root CMake project (ADR 0003; options `CALCU_PRODUCT_59`, `CALCU_PRODUCT_1600`, `CALCU_FRONTEND_QT`); this directory configures on its own only under Emscripten, until the Web frontends move to the root (M-12). Run the test suite optimized: `tools/run_tests.sh` (`-O1`, the repository's `build/tests` tree) or the `CoreTests` target of any root tree (`-O1` in Debug). When chasing a bug — a debugger, stepping, inspecting variables — first build a full-debug tree and work there, leaving the other trees as they are:

```sh
cmake -S ../.. -B ../../build/debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCALCU_FRONTEND_QT=OFF -DCALCU_TESTS_DEBUG=ON
cmake --build ../../build/debug --target CoreTests
```

## Core, Shared, frontends

Three layers (docs/developer/Shared-Architecture.md): `core/` is the hardware, `app/` all app logic that isn't drawing, `frontends/qt6/app/` the desktop GUI, `frontends/web/` the WebAssembly browser GUI.

- Non-GUI logic goes in `app/` (or `core/`), never in `frontends/qt6/app/`; Qt classes translate types and show what Shared decides.
- Core and Shared: portable C++20, standard library only, single-threaded, never blocking. Long machine work is a resumable job (`core/Job.hpp`: `co_await job::run(machine, n)`); coroutine parameters by value.
- A change to `core/` or `app/` passes `tools/run_tests.sh` and `tools/build_wasm_check.sh`, and the Qt app builds; a change the web client sees also passes `frontends/web/build.sh` + `frontends/web/run-tests.sh`.

