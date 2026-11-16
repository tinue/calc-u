---
name: calc-u-59-web
description: Use this skill when working on the Calc-U-59 web client (frontends/web/) — the browser calculator on core/ + app/ compiled to WebAssembly: the React components in frontends/web/src/, the engine worker, frontends/web/bindings.cpp, calc.css, the installable PWA in frontends/web/app/, frontends/web/build.sh and frontends/web/dist/, the Playwright tests in frontends/web/tests/, and the `web` job in `.github/workflows/calc-u-59.yml` (repository root). Also trigger when TIAppPart's commands or topics or TIWebPart change (what the page draws comes from app/TIWebPart), or when the website needs to know how to embed the client.
user-invocable: true
---

# Calc-U-59 Web Client

The third GUI next to the Swift and Qt6 apps: `core/` + `app/` compiled
with Emscripten, the library's app session run single-threaded in the
library's Web Worker (`library/web/`, LIB-15; since M-34); React only draws
and forwards input. Two layouts: wide (as the macOS/Qt window: calculator,
printer, debugger) and portrait (as the iPhone: calculator and printer
pages; the PWA). Orient new GUI work on the Qt app's widget of the same
name (`WebClient.md` § "Layout of `frontends/web/`" maps them).

Layout, build steps, the `dist/` contract, mounting, location independence,
the render rule, styling, the PWA and `MANIFEST.json`:
**`reference/WebClient.md`**. Don't restate them here. Everything that isn't
drawing: `reference/SharedArchitecture.md`. Feature placement:
`reference/FrontendParity.md` (the Web column).

The website (https://calc-u.org/59/, repository `calc-u-site`) only
*embeds* a built `dist/`, imported from a release's `Calc-U-59-web.zip`.
Never edit the calculator there — that copy is generated.

## Rules

1. **No logic in `frontends/web/`.** Parsing, timing, key sequencing, layout numbers,
   cue-card resolution all belong in `app/` (calc-u-59-shared skill) —
   the JS draws what the session sends: `app/TIWebPart` (the `c59.web.*`
   topics, commands and requests), `app/TIAppPart` (`c59.*`) and the
   library's topics, which the component set draws (M-53): the paper panel
   `CalcuPaperPanel` (`src/Paper.jsx`, LIB-8) and the run-control buttons
   `CalcuRunControlButtons` (`src/Debugger.jsx`, LIB-10), on `engine.shell`.
   A trace is handed over with `keep-file` when it ends; the page lists it.
   `bindings.cpp` is only the factory of the session; `src/engine.js` only
   translates the components' calls to commands and requests and the topics
   to slices (as Calc-U-1600's). No message of a protocol of its own (LIB-15
   AC-22), no `localStorage` (LIB-9 AC-23).
2. **Parity** (CLAUDE.md § "Frontend Parity"): a user-visible change to a
   feature the web has needs the matching change in the apps, or a
   `Pending` row in `FrontendParity.md` — and the reverse. Update the table
   either way.
3. **Location-independent** (`WebClient.md` § "Location independence"). Files the worker
   loads go into `dist/bundle/` or `dist/presets/` (`files.json`); other new
   files the PWA needs offline go into `build.js`'s `core` list for the
   library's service worker (build.js fails on a missing one).
4. **Render has no side effects** (`WebClient.md` § "No side effects in render").
5. **`calc.js` exposes `PlayCalculator` only.** The sources are concatenated
   into one closure; a new file goes into `SOURCES` in `build.js`, in load
   order. Its props (`WebClient.md` § "Mounting it") and the stable hooks
   are the contract — changing them is a change for the website too.
6. **The client brings its own styling** (`reference/WebClient.md`). Fonts and
   React are self-hosted in `frontends/web/` (no CDN).
7. **Assets have one source:** ROMs from `roms/` (in `dist/bundle/`: the
   library's worker fetches all 24 at init into `/bundle`, where the
   engine's ROM lookup finds them, LIB-12; the service worker caches them), artwork from
   `frontends/swift/App/Assets.xcassets`, presets from `examples/` (listed in
   `presets.json`), the icon from `frontends/qt6/resources/linux/` (derived from
   `frontends/swift/App/AppIcon.icon`). `build.sh` copies them; never commit copies.
8. **Build and test before committing:** `frontends/web/build.sh && frontends/web/run-tests.sh`.
   Also after any `core/` or `app/` change (CLAUDE.md).
9. **Commit often, never push.**

## Build and test

```bash
frontends/web/build.sh                       # → frontends/web/dist/ (~15 s; engine: CMake target calcu59_web in ../../build/web)
frontends/web/run-tests.sh                   # all tests, at / and under /anything/calc/
frontends/web/run-tests.sh -g "cue card"     # extra args go to playwright test
python3 -m http.server 8000 -d frontends/web/dist   # look at it: http://localhost:8000/
```

The tests (`tests/play-calculator.spec.js`) drive `dist/index.html` and
`dist/app/` through the real engine: arithmetic by bitmap identity of the LED
canvas, the hit test and drag, the physical keyboard, models and the TI-58C
constant memory, settings, cue cards, presets, the card folder, the paper
panel (`LIB-8 AC-28`, `AC-31`), the debugger's run-control buttons
(`LIB-10 AC-34`) and the trace hand-over (`LIB-10 AC-26`), the portrait pages, the service worker's
precache. Key presses after a program stops can extend its last entry, as
on the real calculator — press CLR first. `tests/prerender.spec.js` renders `calc.js`
in Node. Ports 8059 and 8060 (`CALCU_WEB_PORT` moves both).
`tests/session.spec.js` (LIB-15 AC-19, AC-22, EC-4, LIB-3 AC-31) records the
messages and reads the engine through the root element's `calcuEngine`;
`tests/storage.spec.js` (AC-20, AC-21, LIB-9 AC-23) reads IndexedDB
`/persist-calc-u-59`. `tests/pacing.spec.js` (LIB-17 AC-19, Checkpoint) is
skipped unless `CALCU59_WEB_PACING=1`; it reads the session's `ticks`
request through `calcuEngine` for one minute.

The engine's Emscripten flags live in the product's `CMakeLists.txt`
(`calcu59_web`); the tree is configured once — delete `../../build/web`
after an Emscripten upgrade.

CI: job `web` in the repository's `.github/workflows/calc-u-59.yml` (emsdk pinned to the local
`emcc --version`; bump both together). Artifacts: `reference/WebClient.md`.
