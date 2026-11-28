# Web Client (`frontends/web/`)

The web client is the third GUI on `core/` + `app/`, next to the Swift
app (`frontends/swift/App/`) and the Qt6 app (`frontends/qt6/`): the library's app session with
`cu59::TIAppPart`, as every frontend hosts it, compiled to WebAssembly and run
single-threaded in a Web Worker on the library's Web shell (`library/web/`),
with React components that only draw what it sends and
forward input to it. It has the
apps' features (`Frontend-Parity.md`, the Web column) in two layouts:

- **wide** — calculator | printer | debugger, as the desktop window, the
  Qt `MainWindow`;
- **portrait** — one page at a time, calculator or printer, switched with
  corner chevrons, as the iPhone layout; no debugger, no traces.

The installable PWA (`app/`) picks between them by the window's size with
the library's view rule (`shell::view`, in JS `calcuShellView`): the room
beside the faceplate that takes the printer column gives wide, anything
else portrait.

Apart from the layout, the page is desktop or mobile (the library's
`calcuShellMobile`, `library/web/components/useShell.jsx`): mobile for
the installed PWA and for a browser that asks for the mobile site, else
desktop. Desktop has the menu bar, mobile none; the control bar ends with
More on both (Load Preset… and Settings… in it).

Deliberately different from the apps: the LED is Classic only; the files
it keeps are in the browser's IndexedDB, the persistent file system of the
library's shell (database `/persist-calc-u-59`): the
settings file `settings/Calc-U-59/Calc-U-59.ini` and the save
folder `Calc-U-59/` with the magnetic cards (`<name>.magnetcard.yaml`, the
library's media part: File ▸ Import Media… (also in More) brings a card file in,
Saved Media… downloads or deletes one) and
`ti58c.memory.yaml`; nothing is in `localStorage`. Only the
first copy of the client in a browser writes them; a second tab (or a
second client on one page) works in memory and says so. There is no trace
folder: a trace (the TRACE button's or a preset's)
is kept in the worker's memory by the library's trace session and handed to
the page with the host service `keep-file` when it ends,
which the page offers as a download and lists in the RUN LOG tab (it keeps the
bytes; the worker keeps no copy). Like the apps, it starts with
the PC-100C connected.

It is built into a self-contained folder, `frontends/web/dist/`, that does not know
where it is hosted. The website (https://calc-u.org/59/, repository
`Calc-U-Site`) embeds that folder as `59/calc/` (a release asset of this
repository, or a local `frontends/web/dist/` for testing) and mounts the calculator
from it. This document is the contract between the two.

```
library/web/src/* + frontends/web/src/* ──(Babel, in Node)──▶ calc.js: CalcU59        ─┐ session messages (postMessage)
                                                                             ▼
calc-engine-worker.js → library-worker.js ── wasm/calcu59-core.{js,wasm} ◀── bindings.cpp + app/ + core/ + library (emcc)
```

## Layout of `frontends/web/`

| Path | Role (Qt counterpart) |
|---|---|
| `src/page.jsx` | The page's host services for the library's `useShell()` (`createC59Traces`: the library's services, the traces handed over with `keep-file` kept), the `preset` prop as its bundle path, the RUN LOG tab's view of `log`, the page's context; decides nothing (`EmulatorController`). The components send commands and requests by name and read topics with `useTopic` |
| `src/painters.js` | Canvas painting: the LED (`LedRenderer`), the cue card's draw list (`CueCardRenderer`); the images and canvas sizes through the library's `calcImage` and `sizeCanvas` (`library/web/src/canvas.js`) |
| `src/CalculatorFace.jsx` | Artwork, LED and cue card overlays, pressed-key effect (`held-keys`); the pointer is the React set's `useShellFaceplate` on `c59.faceplate-map` (a press on a fitted card's corner: `c59.tap-card`); transparent key buttons for assistive technology (`KeyboardWidget`) |
| `src/Dialogs.jsx` | Card picker on the `media` topic (Arm Card, Arm Fresh Card; the replace question by the library's `calcuAskedQuestion`) in the library's `ShellModal` (`CardPickerDialog`); File ▸ Saved Media… is the library's `ShellSavedMedia` |
| `src/Paper.jsx` | The PC-100C's pane: the component set's paper panel `CalcuPaperPanel` (`library/web/components/`), owner and its place from the control state's panel, which draws the library's `paper PC-100C` from its marks; PRINT, ADV and TRACE are the panel's controls, which it takes from the `control-state`'s `controls` (their positions, labels and short labels; the short ones where the row is compact, `calcuShellPaperRowCompact`) and sends as the library's `set-control`, Copy and Cut its `paper.copy` / `paper.cut` (Qt6: `calcu::qt6::PaperPanel`). The PC-100C is attached by its button "PC-100C" at the left of the panel's header (the panel model's `places`) |
| `src/Debugger.jsx` | CALCULATOR / CPU / RUN LOG tabs, heatmap, ASM overlay (`debug/*`) on the debug part's product topics `c59.calculator-debug`, `c59.program-listing`, `c59.cpu-debug`, `c59.heatmap`, `c59.asm-overlay`, `c59.debug-view` and the library's `log`; Run and the CALCULATOR tab follow the `control-state` entries `c59.asm-run`, `c59.calculator-tab` (TRACE can always be pressed); FREEZE, STEP and RESUME of each tab are the component set's `CalcuRunControlButtons` for the levels `program` and `cpu` on the library's `run-state`, FREEZE ON START / ARMED its `pause-on-start`, TRACE the library's `trace` |
| `src/CalcU59.jsx` | The component `CalcU59`, put together from the React set's shell: the menu bar (desktop; none on mobile), the shell's control bar with More at its right edge with the module picker (the library's `ShellSlotPicker`) and the card controls (Arm Card and Arm Fresh Card choose the product items of the same names), the shell's dialogs, the storage banner, "ROMs missing"; the product's data and faceplate maps through the library's `useShellProduct` and `useFaceplateMaps`; the two layouts, focus and physical keyboard (the library's `calcuHostKeys`), drag and drop (the library's `open {way: drop, upload: true}`; the library's `calcuFileDrop`: a drop of exactly one file; `calcuOpenFiles`), keyboard feedback (`[keyboard] feedback`) (`MainWindow`) |
| `bindings.cpp` | The factory of the app session (`CALCU_WEB_PRODUCT(CalcU59Host, …)`): the ROM lookup over `/bundle` (without the full set: a session without a machine and the topic `roms-missing`), the settings file, the shell's part, the library's `SettingsPart` and ControlPart (with the debug panel's product entries), `TIAppPart::sessionParts()` (the library's parts with the TI rules, the bundle of `/presets`; no trace folder: the hand-over) and `TIWebPart`; the library's `WorkerHost` translates the messages |
| `calc.css` | Tokens, fonts and every rule the client needs |
| `fonts/` | Archivo (WOFF2, SIL OFL 1.1), for the cue card, as the family `Calc-U Archivo` |
| `app/` | The installable PWA's own files: `index.html` (with the page's colours), `manifest.json`; its page script, `app.css`, service worker and worker entry are the library's (`library/web/app/`) |
| `index.html` | Demo and test page, sized to the window: `?layout=wide|portrait`, `?mobile=0|1` (the library's override), also `width`, `height`, `model` (by its name), `preset` (a path of `examples/bundle.yaml`, sent as the bundle path `examples:<path>` with the library's `start`) |
| `examples/bundle.yaml` (the product's) | What File ▸ Examples ▸ offers: the request `preset-list` gives its examples in their groups (`docs/formats/bundle.md`). `build.js` copies exactly the files it lists, and the file itself, to `dist/presets/` |
| `library/web/vendor/` (the repository's) | React 18 and ReactDOM (shipped), Babel (build only), ReactDOMServer (tests only); one copy for both products |
| `build.sh`, `build.js` | The build (below): `build.js` names what is the product's (the engine, the worker's configuration with the persistent mount `/persist-calc-u-59` and the lock `calcu59-storage`, the files, the sources), the library's `library/web/build/product.js` does the steps |
| `tests/`, `run-tests.sh`, `playwright.config.js`, `package.json` | Playwright tests (`tests/helpers.js`: `gotoReady`); the servers and the shared configuration are the library's (`library/web/tools/playwright.sh`, `playwright-config.js`) |

### What the session sends

Everything that crosses `postMessage` is a session message in the library's
JS form (commands, requests, replies, state and stream topics;
`docs/formats/session-protocol.md`).
`app/TIWebPart` sends the `c59.web.*` topics, already in the form the page
draws, each derived from a product topic's value when it changed: LED cells of `c59.calc-indicator` (`ledCell`, as float32 bytes),
the cue card of `c59.cue-card` as a draw list laid out by `cueCardGeometry`
and friends for the size the page reports. The page reads the control
state's `controls` (the PC-100C's PRINT, ADV, TRACE), the library's `run-state` (FREEZE ON START, ARMED), `busy` (the
display's full-speed hold, `data-full-speed`) and `held-keys` (the key drawn
pressed) itself. The settings are
the library's `settings` topic and `set-setting` (the shell's dialog). The
cards are the library's `media` topic (the place `card-reader`: the fitted
card, the side passing, its picker; the saved cards by kind) and its
`media.*` commands: Arm Card `media.choose {place: card-reader, entry:
saved:<file>}`, Arm Fresh Card `media.start-new {place: card-reader,
template: "implicit:Fresh card", name}` (a reply `asks: …` is the replace
question, sent again with `confirmed`), Eject Card `media.eject`, a corner
`media.feed`, `media.import`, `media.export`, `media.delete`; the bar's
module picker is the `slot-1` picker. The debug panel's data are the product topics
of `app/TIDebugPart`, whose values are the page's forms:
rows formatted by `DebugText`, heatmap steps (`heatStep`) as bytes. A file
dropped on the calculator or picked by Load Preset… (several at once:
one folder, `/uploads`) is the library's `open` with `upload: true`; the session
decides what it is. The product's data for the shell are the request
`c59.shell-product {version, build}` (`app/TIShellProduct`: the help
address, the About texts and links, the uploads' accept list) with the
chrome; the faceplate is
`c59.faceplate-map {model}` (the hit map in pixels of the 2064 × 4075 base
image, the card area, each key's rectangle, the card's corners); the
heatmap's grid `c59.heatmap-grid`; the LED shapes
`c59.led-geometry`, the debug
tabs' headers `c59.debug-headers` (app/'s `debugHeader`: title, buttons
left to right, wide and narrow labels, colours, `narrowBelow`)
(`app/TIWebPart`). The start is the shell's `start` command with
the `model` and `preset` props. The paper, the run state and the trace are
the library's topics (`papers`, `paper PC-100C`, `paper-copies`,
`run-state`, `trace`), which the component set's panel and buttons
read from the shell. The menus, the bar, the More menu and the settings
dialog are the library's C++ models, which the worker serves as the topic
`web.shell` (`docs/formats/web-embed.md` §3.1) for the page's `shell
{mobile, settings, about, version, build, buildTime, host, layout}`
(`TIWebPart::shellSetup`: the settings of the layout).
The pointer is the shell's hit rule: the widened key rects,
distance 0, the display's full-speed hold; a key is fixed when the pointer
goes down (a drag does not move the press to another key).

## Build

```bash
frontends/web/build.sh          # → frontends/web/dist/ (gitignored): tested and zipped for a release
frontends/web/run-tests.sh      # Playwright against dist/ on ports 8059 and 8060; build first
```

`build.sh` needs `emcmake`/`cmake` (Homebrew `emscripten`; CI pins one release,
the release the build images run, in the repository's
`.github/workflows/calc-u-59.yml`, job `web`, as for Calc-U-1600),
`ninja` and `node`. It runs `build.js`, which hands the product's values to
the library's Web build (`library/web/build/product.js`, the same steps for
Calc-U-1600). `CALCU_WEB_DIST` names another output folder,
`CALCU59_ROMS_DIR` another ROM folder (`tests/rom_missing_build.sh`). The
build

1. builds the CMake target `calcu59_web` (`core/` + `app/` + `bindings.cpp`)
   in the repository root's `build/web` tree, configured once with
   `emcmake` (the flags are in the product's `CMakeLists.txt`):
   single-threaded (a static host can't be relied on to send the COOP/COEP
   headers `SharedArrayBuffer` needs), with `-fwasm-exceptions` (app/
   reports file errors with exceptions), `MODULARIZE` as
   `createCalcU59Module`, `FS` and `IDBFS` exported, the library's glue
   (`calcu_web_glue`) linked, with the CMake option `CALCU_WEB_TEST_AIDS`
   on (off with `--release`); Calc-U-59's engine has no test-only command,
   so both give the same dist/, and CI zips the one its tests ran against;
2. copies the full set of ROM files (the 24 of `roms/fetch_roms.sh`; the
   module files hold the cue cards) from `roms/` to `dist/bundle/`, and the artwork (`ti59_base`,
   `ti58_base` and the three cue-card templates) from `frontends/swift/App/Assets.xcassets`
   to `dist/img/`;
3. copies the static files, React, the font and the app icon (derived from
   `frontends/swift/App/AppIcon.icon` by `tools/gen_qt_icons.sh`), and
   writes `calc-engine-worker.js` from the worker's configuration;
4. runs the library's product build (`library/web/build/dist.js`
   `buildProduct`, which the library's test product uses too): it compiles the library's page side and `src/*` into one `calc.js` (in
   the order of `build.js`'s `sources`, with the version stamped in) and the library's app page
   with `CalcU59` into `app/app.js`, fails on a selector of
   `calc.css` outside `.calcu-client`, copies the library's
   worker (`library-worker.js`), service worker and app worker entry, the
   presets from `examples/`, writes `files.json` (what the worker loads),
   stamps the service worker with a hash of what it precaches (and fails if
   a precached file is missing), and writes `MANIFEST.json`.

Re-run it whenever `core/`, `app/`, `roms/` or anything in `frontends/web/` changes.
CI uploads `dist/` zipped as `Calc-U-59-web-<version>.zip` (artifact
`Calc-U-59-web`); the zip holds the contents of `dist/` at its root.
`release` (`release.yaml`) attaches it to each release as
`Calc-U-59-web.zip` — the file the website's `59/.seo/fetch-web-client.js`
downloads and pins.

## The `dist/` contract

What a host page loads and mounts, what `dist/` holds, the shared props,
location independence, rendering without side effects and the scoped
styles are the family's: `docs/formats/web-embed.md` at the repository
root. The global is `CalcU59` (with `CalcU59.portraitWidth(height)`).
What is Calc-U-59's own:

- `layout`: `"portrait"` (the default: calculator and printer pages),
  `"wide"` (calculator, printer, debugger side by side) or `"auto"` (by
  the window's size, the PWA's choice: needs `width` and `height`).
- `keyboard`: the keys of `App-Layer.md` § "Physical Keyboard Mapping".
- `width` in wide: the calculator artwork takes 40 % (at most 420 px
  without `height`, never below 400 px), the printer 220–320 px, the
  debugger the rest (at least 220 px).
- `height`: without it the height follows the artwork's aspect plus the
  50 px bar; with it the artwork fits inside (the PWA passes `#root`'s
  content box).
- `model`: `"TI-59"`, `"TI-58"` or `"TI-58C"`; without it the stored
  setting.
- `settings` (default `true`), the product's own prop: offer Settings…
  (the File menu and the More menu).
- `paperWindow` (default `false`), a shared prop: with `layout="auto"`
  only (the fixed layouts have no view that gives the paper's room to the
  debugger) on a desktop page, the paper panel's button that opens the
  paper in a window of its own (`useCalcuPaperWindow`,
  `library/web/components/PaperWindow.jsx`). The app page passes `true`;
  an embedding page leaves it out.

There is no `module` prop: a host page that wants
another module names a preset that sets it.

The client has a minimum size, as the apps' windows do: the narrowest
artwork whose control bar still fits (400 px wide, 264 px portrait), plus
the printer's and debugger's minimums in wide — 862 × 856 px wide,
280 × 587 px portrait. Given less, it keeps that size (it never shrinks as
a flex item) and the host scrolls, as `index.html` does.

The client sizes itself from these: it has no `scale`. Dialogs (cards,
settings, About, notices) open inside its own box, so it needs no room outside it.

A file of `files.json` that cannot be fetched is a failed start
("Calc-U-59 could not start: …"). A missing or bad ROM file is the shell's
"ROMs missing" (`[data-calcu=roms-missing]`, the lookup's text in place of
the calculator; the menu bar's Help and About work).

Stable hooks a host's tests may rely on: the root element's classes
`calcu-client calcu-device` with `data-layout="wide|portrait"` (its
`calcuShell` property is the product's own test hook), the artwork canvas
`canvas.calcu-keyboard`, the LED `canvas.calcu-led`, the cue card
`canvas.cuecard` (its `aria-label` holds the card's text), a fitted card's
corners as the buttons `Feed card side 1` and `Feed card side 2`, the key buttons
named by their face labels (`2nd`, `−`, `R/S`, …) with `aria-pressed`, the
panels `[data-panel=paper]` and `[data-panel=debug]`, the card controls by
name (`Arm Card` and `Arm Fresh Card` (both read "Crd"),
`Eject Card`), the card dialog `card-picker` with its list `Saved cards`, and the shell's
elements by their `data-calcu` names: `menu-bar`,
`menu-file`, `item-<id>`, `control-bar`, `bar-model`, `bar-place-printer`,
`media-picker-slot-1`, `bar-more`, `more-item-<id>`, `settings-dialog`, `about-dialog`, `notice`,
`loading`, `storage-banner`, `roms-missing`.

`run-tests.sh` runs every test twice (two servers of
`library/web/tools/serve.py`, started by `library/web/tools/playwright.sh`):
against a server whose root is `dist/`, and against one that serves
`dist/` only under `/anything/calc/` and answers 404 everywhere else, so a
root-relative URL fails the second run. `tests/prerender.spec.js` renders
`calc.js` in Node with the vendored ReactDOMServer (the library's sandbox,
`library/web/tests/dist-helpers.js` `prerender`) and checks that it leaks
no globals besides `CalcU59`.

`calc.css` refers to custom properties only: the chrome of app/'s
`TIChrome` (colours `--calcu-<name>`, the panel header and button-bar
heights, the inset, the pill's shape, the bar's height and gap), which
`c59.shell-product` carries and `CalcU59` sets on its root
(`calcuChromeStyle`, `library/web/components/useShellProduct.jsx`); until the
engine answers they fall back to neutral system colours (the common ones
and the shell's menus, dialogs, banner and paper in the library's
`shell.css`, the product's own in `calc.css`). `CalcU59` takes
the bar's height and the printer column's width range from the same
chrome. The font family is `Calc-U Archivo`, a name no host page uses.
Sizes that follow the width are inline styles.

### The PWA (`app/`)

`app/index.html` mounts `CalcU59` with `layout="auto"`, the keyboard
and an install banner. It follows the window with the library's view rule
(`calcuShellView` on app/'s chrome, `TIChrome`; the bar's labels by the
library's `calcuShellBarFit`, measured with and without them): the
faceplate as large as the window allows; where the room left beside it is
at least `panelsMinWidth`, calculator and printer side by side, with the
debugger where that room also takes `debuggerMinWidth` and the window is
`debuggerMinHeight` tall (not a phone in landscape); else the pages (a
phone in portrait, a narrow window). The manifest's orientation is
`portrait`, the faceplate's own: Android keeps the installed app upright,
Safari ignores it, and the layout follows the window there. A browser can't
see a fold, so a half-open foldable counts as its whole window. WebKit in a Home Screen app returns from landscape
with the document scrolled by the status bar's height but drawn unscrolled, so
touches land too low; the library's `app.jsx` resets the scroll. Only an installed app in the
iOS simulator or on a device shows it, not Playwright. `#root` is fixed to the viewport and a
`ResizeObserver` sizes the calculator to its content box (inside the safe
area), which also catches insets arriving late; the status bar is opaque,
because with black-translucent WebKit's standalone viewport left it in or
out from one launch to the next. Its service worker's scope is `app/`. It precaches the shell,
`calc.js`, `calc.css`, the worker, the engine, the artwork, every model's
ROM files, every Solid State module and every preset, so the installed app
does all of it offline after the first launch. A browser asks a service worker
for a worker's script only if that URL is in its scope, so the app starts the
engine through `app/engine-worker.js`, which loads `../calc-engine-worker.js`;
an embedded client starts that file directly, and stays outside this service
worker even on an origin where the app is installed, so it never gets a cached
engine older than its `calc.js`. `tests/offline.spec.js` starts the app with the
server gone (its own server, `dist-helpers.js` `serveDist`), in Chromium and WebKit: `context.setOffline()` isn't enough, as the
browser's HTTP cache then answers. The library's `stampServiceWorker` adds
everything of `files.json` (the ROMs, the presets) to the list; the worker
loads them all at its start. The cache is named
`calcu59-app <scope> <hash>`: a new build is a new cache, used from the
app's next start, and the old one of the same scope is
deleted on activation — other caches on the origin (the host's, another
copy of the client) are left alone. The service worker never touches the
persistent file system (IndexedDB).

### `MANIFEST.json`

The form is `docs/formats/web-embed.md` §2.1. Its `version` is
`MARKETING_VERSION` from the Xcode project (the only version; read through
`release.yaml` as ReleaseIt and the help tool read it).
