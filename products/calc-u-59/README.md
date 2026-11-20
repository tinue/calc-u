# Calc-U-59

A TI-58, TI-58C, and TI-59 hardware emulator with an integrated debugger for macOS, iOS, Windows and Linux, running each calculator's original ROM.

## Purpose

Calc-U-59 started as a research tool and still has that focus at its core: it accurately emulates the TMC0501 processor shared by the TI-58, TI-58C, and TI-59, and lets you step through ROM code, inspect registers, trace printer output, load raw machine-state files, and study the quirks of the original hardware.

It has also grown into a capable calculator in its own right. If you just want a faithful TI-58/58C/59 on your Mac, iPhone, iPad or Windows/Linux PC and don't care about the debugger, that works fine too — on iPhone the debug panel is disabled by default, so the app presents as a classic calculator out of the box.

A third audience is people who have found their own old TI-58 or TI-59 programs — whether on magnetic cards, in printed listings, or from memory — and want to revive them: run them again, understand what they actually do at the machine level, or improve them with the benefit of a full debugger.

Whether you want a classic calculator that just works, or you want to run programs under a debugger and reproduce behavior from original hardware down to the instruction level, you are in the right place.

## User Guide

**For installation and day-to-day operation — including a debug-panel walkthrough — use the website:**

### → https://calc-u.org/59/

The user guide is the help in [`help/`](help/) (Calc-U-59's chapters) and [`library/help/`](../../library/help/) (the chapters both Calc-U products share), Markdown that each release carries as `Calc-U-59-help.zip` and calc-u.org/59 renders. Everything else here is developer documentation: architecture and APIs for people building or extending the emulator (including AI agents doing so).

Pre-built macOS DMGs, Windows installers (x64, ARM64) and Linux AppImages (x86_64, aarch64) are on this repository's [Releases page](../../releases); the iPhone and iPad app is on the App Store.

## Features

- Cycle-accurate TMC0501 CPU emulation
- Two native apps and a web client on one C++ app layer: Swift for macOS, iOS and iPadOS (with iCloud card sync), Qt6 for Windows and Linux, and the browser (`frontends/web/`, the engine compiled to WebAssembly)
- Integrated debugger with register inspection and program memory dump
- [Debug API](docs/developer/Debug-API.md) for scripted access to CPU state, breakpoints, instruction tracing, and disassembly
- Printer trace mode (functional simulation — see limitation below)
- Presets (`.ti59`, `.ti58`, `.ti58c`, [preset format 2](../../docs/formats/preset.md)) that set up a machine, its program, registers and cue card, and play keys
- Card reader emulation and the Solid State Software modules, with cue cards
- The web client, playable at [calc-u.org/59/play](https://calc-u.org/59/play/) — no install required

## Current Limitations

**Printer:** The PC-100C thermal printer is simulated functionally: the emulator intercepts the ROM's print commands, keeps the printer busy for a line's print time (enough for the [printer interrupt](rom/printerinterrupt.md)), and renders characters with a dot-matrix font calibrated against physical hardware. For a line cut short, the head's column-by-column firing and each dot's heating time are modeled, measured on a real PC-100A; the paper mechanics are not.

**Debugger:** Live view and inspection, and stepping both calculator-level and CPU-level instructions work. The core has a breakpoint API used by the harness's `break` (`docs/developer/Debug-API.md` § "Breakpoints"), but no GUI has a UI to set one. Also missing: keypress latch while single-stepping and conditional freezes (e.g. on a register change). The API surface will change as it matures.

## Building

The code is layered: `core/` emulates the hardware, `app/` (C++20, standard library only) holds all non-drawing logic, and the three GUIs only draw — see [`docs/developer/App-Layer.md`](docs/developer/App-Layer.md) and [`docs/developer/Frontend-Parity.md`](docs/developer/Frontend-Parity.md).

**Swift app (macOS, iOS, iPadOS):** open `frontends/swift/Calc-U-59.xcodeproj` in Xcode, or `xcodebuild -project frontends/swift/Calc-U-59.xcodeproj -scheme Calc-U-59 -destination platform=macOS build`. The project carries the author's Apple Developer Team ID and bundle identifier (`ch.erzberger.calcu59`). To build it yourself, change the team on the target's *Signing & Capabilities* tab to your own; Xcode updates the bundle identifier. The Simulator needs no changes. For iCloud card sync on a real device, also register a new iCloud container and update the two container identifiers in `Calc-U-59.entitlements`.

**C++ build, tests and Qt6 app:** the source repository's root CMake project compiles `core/`, `app/`, the C++ test suite and, when Qt6 is found, the Qt app (`-DCALCU_FRONTEND_QT=OFF` skips it, `-DCALCU_PRODUCT_1600=OFF` leaves out Calc-U-1600). From this directory:

```bash
../../tools/run_tests.sh calc-u-59            # every Frequent test of the product, in ../../build/tests
../../tools/run_tests.sh calc-u-59 <text>     # the TESTs whose name contains <text>
```

`../../tools/run_qt.sh 59 --build` builds the Qt app into the repository's `build/app-59` tree; `../../tools/run_qt.sh 59 [file.ti59]` builds and starts it. On macOS, Qt comes from Homebrew and the Qt build is for development only. The Windows and Linux packages are built by CI (the repository's `.github/workflows/calc-u-59.yml`), which also lists the packages each platform needs.

**Web client:** `frontends/web/build.sh` compiles `core/` and `app/` with Emscripten (`emcc`, e.g. `brew install emscripten`) and builds a self-contained `frontends/web/dist/` that the website embeds; `frontends/web/run-tests.sh` runs its Playwright tests against it. CI builds it too and uploads it zipped. See [`docs/developer/Web-Client.md`](docs/developer/Web-Client.md).

See [`docs/developer/New-GUI-Guide.md`](docs/developer/New-GUI-Guide.md) if you're considering a GUI for yet another platform.

## AI Programming Assistant

The file [`prompt/ti59-agent-prompt.md`](prompt/ti59-agent-prompt.md) is a system prompt that turns a capable LLM (Claude, GPT-4o, etc.) into a TI-59 expert. Paste the entire file as the system prompt, then ask the model to write, explain, or debug TI-59 programs. It outputs ready-to-load `.ti59` presets.

**What it covers:** complete key-code encoding, all instructions (conditionals, DSZ, flags, indirect addressing, subroutines), the AOS operator-precedence system, alphanumeric printing via Op 00–06, the full PC-100C character table, and the choices for a `.ti59` file, such as when to use preset registers. The file format itself is [`docs/formats/preset.md`](../../docs/formats/preset.md), which the model gets beside the prompt, so the two cannot drift apart.

**How to use it:**

1. Open your LLM interface, set `prompt/ti59-agent-prompt.md` as the system prompt, and attach (or paste) `docs/formats/preset.md` from the repository root.
2. Ask for a program in plain language, e.g. *"Write a program that sums 1 to N"* or *"Print a sine table from 0° to 90° in 5° steps"*.
3. The model returns a `.ti59` file. Save it and load it in the emulator.

**Token cost:** The prompt and the format are large (about 40 KB and 45 KB, roughly 22 000 tokens together). Every conversation turn carries this overhead. Use a model with a generous context window and be aware that costs add up quickly in long sessions.

## Changelog

See [CHANGELOG.md](CHANGELOG.md) for version history and release notes. This covers the apps and the web client (`core/`, `app/`, `frontends/swift/App/`, `frontends/swift/Bridge/`, `frontends/qt6/`, `frontends/web/`); the website is a separate repository, [`calc-u-site`](https://github.com/tinue/calc-u-site), with its own git history and no version numbers of its own, so website-only changes aren't logged here.

## License

Copyright (C) 2026 Martin Erzberger. Calc-U-59 is free software, licensed
under the [GNU General Public License, version 3](../../LICENSE) (GPLv3); see
[NOTICE](NOTICE) for the full copyright notice.

The Windows and Linux app is built on [Qt6](https://www.qt.io/), used under
the LGPLv3 — see [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md) for Qt and
for the `roms/` ROM images, which are third-party copyrighted material not
covered by this project's license.

## Acknowledgements

Special thanks to:

- **hrastprogrammer** ([hrastprogrammer.com](https://www.hrastprogrammer.com)) for the first emulator core that made this work possible.
- **Hynek Sladký** ([hsl.wz.cz/ti_59.htm](https://hsl.wz.cz/ti_59.htm) — currently offline, [archived 2024-08-18](https://web.archive.org/web/20240818211248/https://hsl.wz.cz/ti_59.htm)) for the initial ROM dump of the TI-59.

## Appendix: Documentation Index

This repository's documentation is for developers (and AI agents working on
the code) — not end users. Every document here, grouped by location, with a
one-line abstract. Start here before writing new documentation — several of
these already cover ground a new document might otherwise duplicate.

End-user documentation (installing, operating, debug-panel walkthrough) is
**not** indexed here — it lives entirely on the website linked above and
isn't duplicated in this repository.

### Project root

| Document | Abstract |
|---|---|
| [`README.md`](README.md) | This file — project overview, features, limitations, and this index. |
| [`CLAUDE.md`](CLAUDE.md) | Skill-routing table and pointers to the architecture reference docs, for AI coding agents working in this repo. |
| [`CHANGELOG.md`](CHANGELOG.md) | Version history, one heading per release. |
| [`THIRD-PARTY-NOTICES.md`](THIRD-PARTY-NOTICES.md) | Qt6 (LGPLv3) notice for the Windows/Linux app; React and Archivo notices for the web client; status of the `roms/` ROM images. |
| [`TODO.md`](TODO.md) | Unsorted backlog of ideas, bugs, and observations across the core, settings, and UI. |
| [`release.yaml`](release.yaml) | Release configuration for ReleaseIt: the Mac DMG built locally, the Windows/Linux packages and the web client's zip from CI, the iOS App Store upload. After a release build, `bin/install-release.sh` puts the Mac app into /Applications. |
| [`PRIVACY.md`](PRIVACY.md) | App Store privacy policy — Calc-U-59 collects no data and makes no network connections. |
| [`LICENSE`](../../LICENSE) | GNU General Public License, version 3 (GPLv3). |
| [`NOTICE`](NOTICE) | Copyright notice for the GPLv3 license. |

### `docs/developer/` — developer documentation

[`docs/developer/README.md`](docs/developer/README.md) indexes the architecture reference: the core, the app layer and the Swift frontend, the debug API, frontend parity, the web client, the harness, idle detection and the porting guide.

### `.claude/skills/` — AI agent skills

Trigger conditions and workflows for Claude Code when working in specific areas of the codebase. These are agent-only — a human developer should read `docs/developer/` instead; skills are kept short and point there rather than restating content.

| Document | Abstract |
|---|---|
| [`calc-u-59-core`](.claude/skills/calc-u-59-core/SKILL.md) | Working on the C++ emulation core (`core/`): CPU model, ROM disassembly, mnemonic workflow, trace infrastructure. |
| [`calc-u-59-app`](.claude/skills/calc-u-59-app/SKILL.md) | Working on the app layer (`app/`, `tests/`, root CMake): rules, adding a module and its Bridge wrapper, golden-file ports. |
| [`calc-u-59-qt`](.claude/skills/calc-u-59-qt/SKILL.md) | Working on the Qt6 app (`frontends/qt6/`, CI workflow): ship scope (Windows/Linux), class map against the Swift views, threading, build and packaging. |
| [`calc-u-59-web`](.claude/skills/calc-u-59-web/SKILL.md) | Working on the web client (`frontends/web/`): rules (no logic, location independence, no side effects in render, own styling), build and tests; points to `docs/developer/Web-Client.md`. |
| [`pre-release-cleanup`](.claude/skills/pre-release-cleanup/SKILL.md) | User-invoked release audit: parity, simplification, docs, changelog, screenshots, builds. |
| [`calc-u-59-swift`](.claude/skills/calc-u-59-swift/SKILL.md) | Working on the Swift/SwiftUI app (`frontends/swift/App/`, `frontends/swift/Bridge/`): view model, bridge, display, keyboard, debug panels, printer, card reader, settings. |
| [`calc-u-59-docs`](.claude/skills/calc-u-59-docs/SKILL.md) | Working on documentation (the help in `help/`, `docs/developer/`, root markdown files); states the help-vs-developer documentation split, the help tool, and the release documentation update workflow. |
| [`calc-u-59-presets`](.claude/skills/calc-u-59-presets/SKILL.md) | Pointer to `docs/formats/preset.md` plus the mistakes that come up most in presets of format 2 (keycodes vs. key IDs, second functions, full speed, the last item) and how to check one with `calcu`. |
| [`calc-u-59-uitesting`](.claude/skills/calc-u-59-uitesting/SKILL.md) | Writing/debugging XCUITest UI tests (`frontends/swift/Calc-U-59UITests/`): file picker navigation, accessibility identifiers, orientation. |

### `rom/` — ROM listings and guides

| Document | Abstract |
|---|---|
| [`rom/TI59-ROM-Guide.md`](rom/TI59-ROM-Guide.md), [`rom/TI58C-ROM-Guide.md`](rom/TI58C-ROM-Guide.md) | How each ROM works, as a guide to the annotated listings (`*-commented.asm`, `*-labels.asm`, `*-keycode-programs.asm`). |
| [`rom/fastmode.md`](rom/fastmode.md) | Fast mode: what it is, the rules for fast-mode programs, the ways in, and how the hack works. |
| [`rom/printerinterrupt.md`](rom/printerinterrupt.md) | The printer interrupt (keycode 1F): how to use it, how the code is made, and how it works. |
| [`rom/traces/README.md`](rom/traces/README.md) | The CPU traces the ROM annotations cite, and [`rom/traces/sbr444-quirk-analysis.md`](rom/traces/sbr444-quirk-analysis.md), the trace-level analysis of the `SBR 444`/R-S firmware quirk chain in the article's printer-interrupt sequence (overview: `rom/printerinterrupt.md`). |

### `tools/` — developer tools

| Document | Abstract |
|---|---|

### `prompt/` — LLM system prompt

| Document | Abstract |
|---|---|
| [`prompt/ti59-agent-prompt.md`](prompt/ti59-agent-prompt.md) | Drop-in, self-contained system prompt turning any capable LLM into a TI-59 programming assistant that outputs ready-to-load `.ti59` presets. The file format is not restated: the model gets `docs/formats/preset.md` beside it. |

### `examples/` — example and debug presets

| Document | Abstract |
|---|---|
| [`examples/calendar/CALENDAR_PROGRAMS.md`](examples/calendar/CALENDAR_PROGRAMS.md) | History of the 1978–1981 calendar-printer competition and a timing comparison of the `calendar-0*.ti59` programs. |
| [`examples/assembly/README.md`](examples/assembly/README.md) | Format of the assembly examples derived from Hynek Sladký's HW programming guide. |
| [`examples/debug/readme.md`](examples/debug/readme.md) | Notes that this subfolder holds debugging/regression presets, kept separate to keep the main examples folder clean. |

### The user guide (not indexed here)

The help (`help/`, `help/help.yaml` lists its files chapter by chapter; checked and built by `tools/help` at the repository root, `tools/README.md`) is not enumerated here. The website that renders it lives in its own repository, [`calc-u-site`](https://github.com/tinue/calc-u-site), with its own internal (build/design-system) documentation.
