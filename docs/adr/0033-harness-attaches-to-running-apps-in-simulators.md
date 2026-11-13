# The harness attaches to running apps in simulators

Status: accepted (2026-11-09); amended 2026-11-10
(`docs/contracts/harness.md`, H-4): every frontend **may** offer attach;
the migration builds it for the Swift frontend only, Qt6 and the web
follow when a need shows. Amended 2026-11-13 (user, B-22 of
`docs/walkthroughs/presets.md`): the Qt6 frontend gets attach in the
migration; its screenshot scenarios are its client (LIB-14 AC-44).

In development and test builds, every frontend can open its app session to
the harness protocol (the serve mode of ADR 0026): over the local network
for the Swift app in an iOS simulator started through Xcode's Device Hub,
over a WebSocket for a Web frontend in a browser. An agent then presses
keys, loads presets and reads LCD text, the debug panel and media state
directly, and takes a screenshot only when it needs one. Tests and
iterative bug fixing in simulators thus no longer depend on UI automation
(XCUITest, WebDriver), which often fails there, or on screenshots passed
by hand. Release and App Store builds leave it out; real iPhones and iPads
are not a target.

## Consequences

- The concrete mechanism in the iOS simulator, and whether the Xcode
  plugin the user installed helps, is a phase 5 spike.
- UI automation remains only for what needs a real tap or gesture (layout,
  touch).
- Installation and smoke tests on Windows and Linux are not part of this
  project (Briefing §1.10); with harness attach they would later be a
  short script.
