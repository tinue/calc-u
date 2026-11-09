# Each product has its own version; the library has none

Status: accepted (2026-11-09)

Each product keeps its own version line (Calc-U-59 2.x, Calc-U-1600 0.x
towards 1.0) and is released on its own: work on one product and release
it for a while without touching the other (Briefing §6). The library has
no version of its own: it is part of the source repository, only products
in this repository use it, and every product release records the commit
it was built from.

## Considered options

- A SemVer version for the library: useful only if something outside this
  repository used it.
