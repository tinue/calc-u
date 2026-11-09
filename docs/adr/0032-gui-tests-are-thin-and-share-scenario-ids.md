# GUI tests are thin and share scenario IDs

Status: accepted (2026-11-09)

Behaviour is tested below the frontends: the app session and the models of
the library's frontend components are tested headless through the
harness. GUI tests per frontend check only wiring, layout and platform
behaviour, each with its native tool: XCUITest for Swift, the Qt
screenshot scenarios plus Qt Test for Qt6, Playwright for the web. The
same scenario has the same name and acceptance criterion ID in every
frontend, so a frontend that lacks it shows up in the AC → task → test
chain.

## Considered options

- One driver for all three frontends through accessibility APIs: one test
  source, but much harder to build and keep reliable.
