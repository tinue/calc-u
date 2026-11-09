# One root CMake project with options per product and frontend

Status: accepted (2026-11-09)

All C++ builds go through one CMake project at the repository root, with
options that choose products and frontends (for example
`CALCU_PRODUCT_59`, `CALCU_FRONTEND_QT`). The library and the cores build
without Qt, so core and library tests need no Qt (Calc-U-59's way; today
Calc-U-1600's core tests need Qt because its root adds `Qt6/`). The web
frontends of both products build through CMake with the Emscripten
toolchain, replacing Calc-U-59's `em++` script. One entry point serves
local builds, IDEs, the harness and CI. How the Swift frontend gets the
C++ code (today Xcode compiles `Core/` and `Shared/` directly) is a
separate decision.

## Considered options

- One CMake project per product that pulls in `library/`: rejected; two
  entry points, and changes to the library would not be built for both
  products by default.
