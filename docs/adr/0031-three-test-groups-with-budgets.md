# Three test groups with time budgets

Status: accepted (2026-11-09)

Tests are grouped by how often they run (Briefing §1.10), each with a
budget and a place:

| Group | Contents | Budget | Runs |
|---|---|---|---|
| Frequent | library, core and app-layer unit tests; harness-driven golden tests | ≤ 60 s per product, locally | on every change, locally and in CI |
| Longer | web Playwright, Qt screenshot scenarios run as checks, XCUITest UIRegression on one simulator, DAP smoke test | ≤ 10 min | locally before merging into `main`; Playwright also in CI |
| Checkpoint | the full App Store screenshot set, the User Guide screenshots, the macOS XCUITests | none | before a release |

A test that outgrows its group's budget moves up a group or is split.
Measured on 2026-11-09 (M4 Pro): Frequent today takes 6.75 s (Calc-U-59)
and 37 s (Calc-U-1600).
