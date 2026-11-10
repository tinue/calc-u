# One C++ test runner: Calc-U-59's TestHarness

Status: accepted (2026-11-10)

The library, both cores and the app layers use one C++ test runner:
Calc-U-59's `tests/TestHarness.hpp` (a `TEST()` registry with `CHECK` and
`CHECK_EQ`, no dependency), which moves to the library. Calc-U-1600's
tests, about 788 test functions called from one `main()`
(`Core/tests/lh5801_tests.cpp`), are registered into it in the migration;
both are the same assert-and-tally style, so the change is mechanical.
CTest keeps running the test programs. This settles the two hand-rolled
runners the inventory found (`docs/inventory/divergence-matrix.md`, M68,
C18), which ADR 0031-0033 left open.

## Considered options

- A third-party single header (doctest): better failure output and
  filtering, but a vendored dependency and every assertion of both
  products rewritten.
- Each product keeps its runner: no rewrite, but two runners in one
  repository and a third choice for the library.
