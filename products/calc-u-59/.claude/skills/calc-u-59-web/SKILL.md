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
drawing: `reference/AppLayer.md`. Feature placement:
`reference/FrontendParity.md` (the Web column).

The website (https://calc-u.org/59/, repository `calc-u-site`) only
*embeds* a built `dist/`, imported from a release's `Calc-U-59-web.zip`.
Never edit the calculator there — that copy is generated.

## Rules

1. **No logic in `frontends/web/`.** Parsing, timing, key sequencing, layout numbers,
   cue-card resolution all belong in `app/` (calc-u-59-app skill) —
   the JS draws what the session sends: `app/TIWebPart` (the `c59.web.*`
   topics, commands and requests), `app/TIAppPart` (`c59.*`) and the
   library's topics, which the component set draws (M-53): the paper panel
   `CalcuPaperPanel` (`src/Paper.jsx`, LIB-8) and the run-control buttons
   `CalcuRunControlButtons` (`src/Debugger.jsx`, LIB-10), on the shell.
   Since M-57 (LIB-14 59) the page is put together from the React set's
   shell (`library/web/components/`): `useCalcuShell` with the layout's
   traits (wide: the menu bar `ShellMenuBar`, File / Machine / Help, no Edit;
   portrait: no menu bar, the bar's Reset, Preset, Settings and More,
   `ShellMoreMenu`), `ShellControlBar` in each calculator column (the model
   picker, LIB-7's `CalcuMediaPicker` for `slot-1`, the button "PC-100C",
   C59-7's card controls as the media controls of `magnetic-card`),
   `ShellDialogs` (settings, About, notices, "Loading…"),
   `ShellStorageBanner`, `ShellRomsMissing`, and `useShellFaceplate` on the
   hit map of `c59.faceplate-map` (a press on a fitted card's corner is the
   product's first: `c59.tap-card`). The product's data are the request
   `c59.shell-product {layout, version}` (`app/TIShellProduct`: items,
   About texts and links, the settings spec of the layout); a new menu
   item goes into `TIShellProduct` (the menu model), not into the page.
   The page's glue is the library's (Part E, E-5; `library/web/components/
   useShellProduct.jsx`, `ShellMedia.jsx`, `library/web/src/canvas.js`):
   `useShellProduct` (the request, its fallback `C59_NO_PRODUCT`, the merge
   with the page's acts and Load Preset…), `useFaceplateMaps`,
   `calcuOpenFiles`, `calcuFileDrop` (the family's rule, LIB-13 AC-25: a
   drop of exactly one file is opened, more are ignored), `ShellSavedMedia`,
   `ShellSlotPicker` (label "Solid State Module"), `calcImage` (the file
   name with `.png`) and `sizeCanvas`, `calcuAskedQuestion` (the card
   picker's replace question), `calcuMediaPlace`; the shell is LIB-15's
   `useShell` with the page's host services (`createC59Traces`).
   `src/engine.js`, `src/ControlBar.jsx`, the page's own preset picker,
   Settings and About, `c59.static-data`, `c59.web.model`,
   `c59.web.settings` and the page's pointer commands went in M-57;
   `c59.web.printer` and `c59.web.run-state` in Part E (E-3): the page
   reads the library's `run-state`, `busy` and `held-keys`, and the
   PC-100C's controls from the control state's `controls` (`src/Paper.jsx`;
   `c59.printer` went in release prep 3).
   A trace is handed over with `keep-file` when it ends; the page lists it.
   The debug panel draws the product topics of `app/TIDebugPart` (C59-4,
   M-56): `c59.calculator-debug`, `c59.program-listing` (the whole listing while
   paused), `c59.cpu-debug`, `c59.heatmap`, `asm-overlay`, `c59.debug-view` (the tab a
   `debug-panel:` step asked for: shown when its `requests` grows; the log
   level), whose values are already the page's forms; the LOG tab keeps the
   last 2000 lines of LIB-3's `log` stream (clear empties the view only); the
   buttons send `c59.dump`, `c59.log-level`, `c59.heatmap-reset`,
   `c59.asm-load` / `-run` / `-clear`; Run, TRACE and the CALCULATOR tab
   follow the `control-state` product entries `c59.asm-run`, `c59.trace`,
   `c59.calculator-tab` (`bindings.cpp` builds the session with the library's
   `calcu::shell::makeHostSession`: LIB-4's ControlPart and LIB-14's
   SettingsPart; without the full set of ROM files the session starts
   without a machine and publishes `roms-missing`). A drop and
   Load Preset… (several files) are LIB-13's
   `open {files, way: drop|dialog, upload: true}`: the session decides
   what a file is, a refusal is its notice (the shell's notice dialog).
   The magnetic cards are the library's `media` topic and `media.*`
   commands (M-55, LIB-7 59, C59-7; `src/Dialogs.jsx`'s card picker, opened
   by the product items Arm Card… / Arm Fresh Card… or the bar's card
   controls; File ▸ Import Media… and Saved Media…, the component set's
   `ShellSavedMedia`); `c59.web.card` and the
   `c59.web.*-card` commands are gone; the corners stay `c59.tap-card`
   (the face's pointer) and `media.feed` (assistive buttons).
   `bindings.cpp` is only the factory of the session; the components send
   commands and requests by name and read the topics with `useTopic`
   (`src/page.jsx`: the host services that keep the traces handed over
   (`createC59Traces`, for `useShell`), the LOG view). No message of a protocol of its own (LIB-15
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
5. **`calc.js` exposes `CalcU59` only** (`src/CalcU59.jsx`; 2.3.0's
   `PlayCalculator` went in release prep 4). The sources are concatenated
   into one closure; a new file goes into `sources` in `build.js` (the
   product's data for the library's `buildProduct`), in load order. Its props (`WebClient.md` § "Mounting it": the family's
   shared ones from the library's `embedProps`, in the order `width`,
   `keyboard`, `layout`, `preset`, `height`, `model`, `workerUrl`, then
   `settings`; LIB-15 AC-29) and the stable hooks
   are the contract — changing them is a change for the website too.
6. **The client brings its own styling** (`reference/WebClient.md`). Fonts and
   React are self-hosted in `frontends/web/` (no CDN).
7. **Assets have one source:** ROMs from `roms/` (in `dist/bundle/`: the
   library's worker fetches all 24 at init into `/bundle`, where the
   engine's ROM lookup finds them, LIB-12; the service worker caches them), artwork from
   `frontends/swift/App/Assets.xcassets`, presets from `examples/` (exactly the files
   `examples/bundle.yaml` lists, into `dist/presets/`; the shell's
   Examples ▸ and the startup preset rows ask `preset-list`), the icon from `frontends/qt6/resources/linux/` (derived from
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

`tests/shell.spec.js` (M-57) carries the scenarios of LIB-14 59 under the
IDs of their Qt6 twins (`lib14_ac24` to `lib14_ac33`, `lib7_ac36 file menu
…`); elements by their `data-calcu` names.
The tests (`tests/play-calculator.spec.js`) drive `dist/index.html` and
`dist/app/` through the real engine: arithmetic by bitmap identity of the LED
canvas, the hit test and drag, the physical keyboard, models and the TI-58C
constant memory, settings, cue cards, presets, the magnetic cards (C59-7,
LIB-7 59: Arm Card, Arm Fresh Card, the scenarios `C59-7 AC-12` and
`C59-7 AC-15`, Import…, Saved Media…), the paper
panel (`LIB-8 AC-28`, `AC-31`), the debugger's run-control buttons
(`LIB-10 AC-34`) and the trace hand-over (`LIB-10 AC-26`), the portrait pages, the service worker's
precache. Key presses after a program stops can extend its last entry, as
on the real calculator — press CLR first. `tests/prerender.spec.js` renders `calc.js`
in Node. Ports 8059 and 8060 (`CALCU_WEB_PORT` moves both; the servers are
`library/web/tools/serve.py`). Shared: `tests/helpers.js` (`gotoReady`), and
the library's `library/web/tests/dist-helpers.js` (`serveDist` for the
offline test, `prerender`).
`tests/session.spec.js` (LIB-15 AC-19, AC-22, EC-4, LIB-3 AC-31) records the
messages and reads the shell through the root element's `calcuShell`;
`tests/storage.spec.js` (AC-20, AC-21, LIB-9 AC-23) reads IndexedDB
`/persist-calc-u-59`. `tests/pacing.spec.js` (LIB-17 AC-19, Checkpoint) is
skipped unless `CALCU59_WEB_PACING=1`; it reads the session's `ticks`
request through `calcuShell` for one minute.

The engine's Emscripten flags live in the product's `CMakeLists.txt`
(`calcu59_web`); the tree is configured once — delete `../../build/web`
after an Emscripten upgrade.

CI: job `web` in the repository's `.github/workflows/calc-u-59.yml` (emsdk pinned to the local
`emcc --version`; bump both together). Artifacts: `reference/WebClient.md`.
