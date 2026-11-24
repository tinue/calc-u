# Web frontend: embedding, worker and storage

A Web frontend as an interface: what a page that embeds it loads and
mounts, what `dist/` holds, the messages between the page and its
worker, the worker's file system and the persistent storage. What a
product's page draws is the product's.

Terms follow `docs/GLOSSARY.md` (Frontend, App session, Topic, Host
service, Save folder, Bundle file).

| Thing | Defined in |
|---|---|
| The props, `bundlePathOf`, the layout warning | `library/web/src/embed.js` |
| The page side: worker start, messages, start state | `library/web/src/shell.js` |
| The directory of `calc.js` (`CALC_BASE`) | `library/web/src/base.js` |
| The host services of a page | `library/web/src/hostServices.js` |
| The sentences of the start and the storage | `library/web/src/sentences.js` |
| The worker: files, storage, the call loop | `library/web/src/worker.js` |
| The persistent file system | `library/web/src/storage.js` |
| The wire form of the messages | `library/web/glue/MessageCodec.hpp` |
| The worker's host of the session; `web.start`, `web.storage`, `web.shell` | `library/web/glue/WorkerHost.hpp`, `.cpp` |
| A product's registration | `library/web/glue/Bind.hpp` (`CALCU_WEB_PRODUCT`) |
| The build: `calc.js`, `calc.css`, `calc-engine-worker.js`, `files.json`, `MANIFEST.json`, scoped CSS | `library/web/build/dist.js` |
| A product's whole build: the engine in `build/web`, `dist/`, `--release` | `library/web/build/product.js` |
| The test servers and the Playwright configuration | `library/web/tools/playwright.sh`, `playwright-config.js` |
| The installed app | `library/web/app/` |
| A product's component | `products/calc-u-59/frontends/web/src/CalcU59.jsx`, `products/calc-u-1600/frontends/web/src/CalcU1600.jsx` |
| A product's build: its values (engine, worker configuration, files) | `products/<product>/frontends/web/build.js` |

---

## 1. Embedding

A host page loads React 18 and ReactDOM as globals, then `calc.css` and,
as a classic `<script src>` (not inlined, bundled or a module),
`calc.js`, and mounts the product's component:

```html
<link rel="stylesheet" href="calc/calc.css">
<script src="react.production.min.js"></script>
<script src="react-dom.production.min.js"></script>
<script src="calc/calc.js"></script>
<script>
  ReactDOM.createRoot(el).render(React.createElement(CalcU1600, { model: "PC-1600" }));
</script>
```

| Product | Global component | Also |
|---|---|---|
| Calc-U-59 | `CalcU59` | `CalcU59.portraitWidth(height)`: the portrait client's width for a height |
| Calc-U-1600 | `CalcU1600` | |

`calc.js` is one closure and adds no other global. It finds the rest of
`dist/` from its own URL (`CALC_BASE`, captured from
`document.currentScript` while it runs), so `dist/` works under any path
of any site, at the root or below it. The page must be served over HTTP
or HTTPS, and `dist/` from the page's own origin (a worker starts only
from there).

Rendering has no side effects: the worker, fetches and listeners start
in effects, so the component renders on a server without a browser.

### 1.1 Props

The shared props, in this order, then the product's own:

| # | Prop | Default | Meaning |
|---|---|---|---|
| 1 | `width` | the parent's width | the component's width in CSS pixels |
| 2 | `keyboard` | `true` | host keys reach the machine while the component has the focus; `false` turns it off |
| 3 | `layout` | the product's default (§1.2) | the product's layout |
| 4 | `preset` | none | a bundled preset to start with: its path inside the bundle (`sum.ti59`, `plotter/globus.pc1600`), with or without the prefix `examples:` |
| 5 | `height` | the parent's height (Calc-U-59: from the width) | the component's height in CSS pixels |
| 6 | `model` | the startup model | the model to start with, by its name as the glossary writes it (`TI-58C`, `PC-1600`) |
| 7 | `workerUrl` | `calc-engine-worker.js` beside `calc.js` | the worker's script, for a page that serves it from another path (the installed app: `app/engine-worker.js`) |
| 8 | the product's own | | Calc-U-59: `settings` (default `true`): whether Settings… is offered; `paperWindow` (default `false`): the full app's paper in its own window (§6). Calc-U-1600: `paperWindow` (the same) |

- Without both `width` and `height` as numbers the component fills its
  parent (Calc-U-1600) or sizes itself from the width (Calc-U-59); a
  faceplate of fixed shape keeps that shape inside.
- `preset`, `model` and `workerUrl` are read once, at mount.
- The component passes `preset` to the session as `examples:<path>`. A
  `model` that names no model is a notice ("There is no model
  "`<name>`"; the startup model is used."); a `preset` the bundle file
  does not list is a notice too. The component then starts as without it.

### 1.2 `layout`

| Product | Values | Default |
|---|---|---|
| Calc-U-59 | `portrait`, `wide`, `auto` | `portrait` |
| Calc-U-1600 | `auto` | `auto` |

Another value draws the default, and the console gets, once per
component and value, the warning (values in JSON quotes):

```
<component>: layout "<value>" is not one of its layouts ("<layout>", "<layout>", …); it is ignored.
```

For example `CalcU1600: layout "wide" is not one of its layouts ("auto"); it is ignored.`

### 1.3 Styles

Every selector of `calc.css` begins with `.calcu-client` (or
`:root .calcu-client`), also inside `@media`, `@supports`, `@container`
and `@layer`; the build fails otherwise (`checkScopedCss`). The
component's root element has the class `calcu-client`, so the styles
apply only inside it and leave the host page's alone.

## 2. `dist/`

The release's Web zip (`<Product>-web-<version>.zip`) holds the contents
of `dist/` at its root.

| Path | What it is | Interface |
|---|---|---|
| `calc.js` | the components, one closure, one global | yes |
| `calc.css` | the styles, scoped to `.calcu-client` | yes |
| `MANIFEST.json` | §2.1 | yes |
| `calc-engine-worker.js` | the product's worker configuration (§4) | no |
| `library-worker.js` | the library's worker | no |
| `audio-worklet.js` | the sound's AudioWorklet | no |
| `wasm/` | the engine: `<wasm>.js`, `<wasm>.wasm` | no |
| `files.json` | §4.1 | no |
| `bundle/` | ROM files, bundled modules and media | no |
| `presets/` | `bundle.yaml` and the bundled presets it lists | no |
| `img/`, `fonts/` | artwork; Calc-U-59's fonts | no |
| `vendor/` | `react.production.min.js`, `react-dom.production.min.js`, for `app/` and `index.html` | no |
| `app/` | the installed app (§6) | no |
| `index.html` | a demo page, also what the tests load | no |

A host page relies only on `calc.js`, `calc.css`, the components' props
and `MANIFEST.json`. Every other name may change with any build.

### 2.1 `MANIFEST.json`

```json
{
  "name": "Calc-U-1600-web",
  "version": "0.9.0",
  "commit": "38aef7f29dc55724a82b0c9d650e42f7bdfb73c4",
  "files": { "app/app.css": "<sha256 hex>", "calc.js": "<sha256 hex>", "…": "…" }
}
```

| Key | Meaning |
|---|---|
| `name` | `Calc-U-59-web`, `Calc-U-1600-web` |
| `version` | the product's version, read from the file and preset `release.yaml` names |
| `commit` | `git rev-parse HEAD` of the build, with `-dirty` when the working tree had changes; `unknown` without git |
| `files` | every file of `dist/` but `MANIFEST.json` and dotfiles, by its `/`-separated path, sorted: the sha256 of its bytes in lower-case hex |

JSON indented by 2, ending in a newline.

## 3. Messages between page and worker

Everything `postMessage` carries is a message of the app session as a
plain object. No message type of a product's own.

| Direction | Form |
|---|---|
| page → worker | `{kind: "listen", args: {services: [name, …]}}` |
| page → worker | `{kind: "shell", args: {mobile, settings, about, …}}`: the page's traits and the product's own args of its shell (§3.1) |
| page → worker | `{kind: "pace"}`: the worker measures its pace and sends it once a second as the state topic `web.pace` `{ticksPerSecond, busyPercent, longestGapMs}` (the page's `?pace=1` line) |
| page → worker | `{kind: "command", name, args, call}` |
| page → worker | `{kind: "request", name, args, call}` |
| page → worker | `{kind: "answer", call, done, reason, value}` |
| worker → page | `{kind: "state", topic, value}` |
| worker → page | `{kind: "stream", topic, reset, value: [item, …]}` |
| worker → page | `{kind: "reply", call, done, reason, value}` |
| worker → page | `{kind: "ask", call, name, value}`: a host service question; `name` the service, `value` its arguments |
| worker → page | `{kind: "sound-start", value: <sample rate>}` |
| worker → page | `{kind: "sound", value: Int16Array}` (mono) |
| worker → page | `{kind: "sound-stop"}` |
| worker → page | `{kind: "app-log", level, text}`: an app log line, already filtered at the level of the session (C++) or of the worker's `?app-log-level=` (the worker's own script, default `warn`); the page writes it to the console and keeps it for Help ▸ Show App Log |

- `call` numbers the page's command or request; the reply carries the
  same number. `call: 0` asks for no reply.
- Values: JSON-like; bytes are `Uint8Array`, whose buffers are
  transferred. A whole JS number is an integer to the session.
- A message the worker cannot decode is answered, when it carries a
  `call`, with `{kind: "reply", done: false, reason: "Unknown message:
  <kind>."}`.

**Sequence.** The page sends `listen` with the host services it offers,
then the command `start`. The worker queues the page's messages until
the session is built.

| Command | Arguments | When |
|---|---|---|
| `start` | `{model, preset}`, both strings, empty when not given | once, after `listen`; a second is refused |
| `suspend` | none | the page is hidden; the worker stops its calls |
| `continue` | none | the page is shown; the worker calls again |

The worker calls the session about 60 times a second (every 1000/60 ms)
with the wall time.

**Host services** a page offers (`hostServices.js`): `audio-sink`,
`clipboard-text`, `clipboard-image`, `file-picker`, `keep-file`,
`open-url`. The answer to `file-picker` is `[{name, bytes}, …]`, or
`done: false, reason: "cancelled"`; the worker writes the bytes to
`/uploads/<name>`, keeps a copy in the file cache (`<mount>/cache/`) and
gives the session the paths.

### 3.1 The library's topics

| Topic | Kind | Value |
|---|---|---|
| `web.start` | state | `{state: "running" \| "failed", failure, detail}`: `detail` (may be empty) goes to the app log only |
| `web.storage` | state | `{mode, sentence}`: §5 |
| `web.shell` | state | `{menus, bar, panels, dialog, nameAndSave}`: the shell's models for the page's last `shell` message |

The shell's models are the library's (`library/shell/`: `menus`, `bar`,
`panels`, the settings `dialog`) over the session's topics and the
`preset-list` reply, in the Value forms of `library/shell/ValueForms.hpp`,
and `nameAndSave` `{place: prompt}`, the Name & Save dialog's prompt
(`shell::nameAndSavePrompt`) for each place of the topic `media`;
the worker computes them anew after each message and call and sends them
when they changed. The `shell` message names the page's traits — `mobile`
(true: the mobile bar, no menu bar), `settings` and `about` (false: no
Settings…, no About; both true when not given) — and the product's own
args (Calc-U-59: `layout`, `version`, `build`); none comes before it.

The page's start state is `starting`, then `running` at the first
`web.start`, or `failed` at `web.start` with `failed`, at an error of
the worker or a message from it that cannot be read. The page shows
"`<Product>` could not start: `<failure>`" (without a failure:
"`<Product>` could not start."), or, when the failure comes once it is
`running`, "`<Product>` stopped: `<failure>`" ("`<Product>` stopped.").
The app log gets one error line, the same sentence, with
the `detail` (where in the worker, the stack) below it. A failure names what went wrong: a file
that could not be fetched (`<path> could not be fetched (<status>)`), the
engine that did not load or start, an abort, an uncaught exception in a
call. A command or request waiting or sent after the failure gets `done: false, reason:
"The product could not start."` (once `running`: `"The product stopped."`).

When a write to the store is refused later, the worker sends
`web.storage` with `unavailable` and the stream item `{sentence}` on the
topic `notices`.

## 4. The worker

A product's `calc-engine-worker.js` (written by the build from the worker
configuration of its `build.js`) sets `self.CALCU_PRODUCT` and loads the
library's worker:

```js
self.CALCU_PRODUCT = { name, wasm, factory, host, mount, lock };
importScripts(`${self.CALC_ROOT || ""}library-worker.js`);
```

| Key | Meaning | Calc-U-59 | Calc-U-1600 |
|---|---|---|---|
| `name` | the product's name | `Calc-U-59` | `Calc-U-1600` |
| `wasm` | the engine's file stem in `wasm/` | `calcu59-core` | `calcu1600-core` |
| `factory` | the engine's module factory | `createCalcU59Module` | `createCalcU1600Module` |
| `host` | the class `CALCU_WEB_PRODUCT` registers | `CalcU59Host` | `CalcU1600Host` |
| `mount` | the persistent file system's mount point | `/persist-calc-u-59` | `/persist` |
| `lock` | the Web Lock of the one writer | `calcu59-storage` | `calcu1600-storage` |

`CALC_ROOT` is the folder of `dist/` relative to the worker; the
installed app's `app/engine-worker.js` sets it to `../`. The worker
resolves `files.json`, `bundle/`, `presets/` and `wasm/` against it.

The host class is built with `{product, dirs: {settingsBase, saveFolder,
home}, storage: {mode, sentence}, post}` and has `failure()`,
`receive(message)` and `call()`.

### 4.1 `files.json`

```json
{ "bundle": ["CE-150.ROM", "…"], "presets": ["bundle.yaml", "…"] }
```

Every file below `dist/bundle/` and `dist/presets/` (dotfiles left out),
by its `/`-separated path inside the folder, sorted. The worker fetches
each and writes it to the same path under `/bundle` or `/presets`; a
fetch that fails is a failed start.

### 4.2 File system

| Path | Holds |
|---|---|
| `/bundle` | the files of `files.json`'s `bundle`: ROMs, bundled modules and media |
| `/presets` | the files of `files.json`'s `presets`: `bundle.yaml` and the bundled presets |
| `/uploads` | files the user picked or dropped; several files of one open as a folder `open-<n>` |
| `<mount>` | the persistent file system (§5) |
| `<mount>/settings/<Product>/<Product>.ini` | the settings file |
| `<mount>/<Product>/` | the save folder |
| `<mount>/cache/` | the file cache: every file the page uploaded, by its name, and the startup presets chosen with "Choose…" (`opening.md` §5, §5a); the worker passes it as `dirs.cache` |

`<mount>` is `/persist-calc-u-59` for Calc-U-59 and `/persist` for
Calc-U-1600. Each product has its own: IDBFS names its IndexedDB
database after the mount point, so two products on one origin never
share a store.

## 5. Persistent storage

| Mode | When | Kept | Sentence |
|---|---|---|---|
| `kept` | this copy holds the lock, or the browser has no Web Locks (plain HTTP), and the read succeeded | every write, rename and delete is synced to IndexedDB | none |
| `second-copy` | another copy holds the lock | read once; changes stay in memory | "`<Product>` is open in another tab or window: what you save here is not kept." |
| `unreadable` | the read from IndexedDB failed | in memory; nothing is written to the store | "`<Product>` could not read its saved files: they are left as they are, and what you save here is not kept." |
| `unavailable` | IndexedDB cannot be opened, or a write was refused (a full store) | in memory | "This browser keeps no files for `<Product>` here: what you save is not kept." |

- **The lock.** `navigator.locks.request(<lock>, {ifAvailable: true})`,
  held for the worker's life. Locks are per origin: two tabs and two
  components on one page count alike.
- The mode is the state topic `web.storage` from the start; its sentence
  also goes once into the notices.

## 6. The installed app

`dist/app/` is an installable offline app: `index.html` mounts the
product's component with `layout: "auto"` and `paperWindow: true` and
starts the worker through `app/engine-worker.js`.

The Web has three kinds of page, each drawn by the same component:

| Kind | Page | Window | Paper in its own window |
|---|---|---|---|
| Full app | `dist/app/` in a desktop browser's tab | desktop (menu bar) | yes: the paper panel's button opens a popup window |
| Demo | the component embedded in another page (the website, `dist/index.html`) | desktop (menu bar) | no: an embed opens no windows of its own |
| Mobile | the installed app, or a browser asking for the mobile site (`?mobile=1` to try it) | mobile (no menu bar) | no | Its service worker (`app/sw.js`, scope `app/`)
precaches the app page, `calc.js`, `calc.css`, the workers, the engine,
the images and every file of `files.json`. Its cache is named
`<id>-app <scope> <hash>` (`<id>`: `calcu59`, `calc-u-1600`); a new
release waits until no window of the app is open, or until a page posts it
`{type: "skip-waiting"}`, then takes over (claiming the open pages, which
reload) and removes the older caches of the same prefix. The page offers
that while a release waits: the banner "A new version is ready." with
Reload. It never touches the persistent file system. An embedded
component is outside its scope.

The page and its `app.css` are the library's (`library/web/app/`); a
product's `app/` holds its `index.html`, with the page's colours as the
custom properties `--calcu-app-background`, `--calcu-app-banner`,
`--calcu-app-banner-border`, `--calcu-app-banner-text`,
`--calcu-app-banner-dim`, `--calcu-app-action`, `--calcu-app-action-text`,
and its `manifest.json`. In a browser tab the page drops the viewport's
`viewport-fit=cover`, so the browser's own bars do not float over it;
installed, it covers the screen and keeps clear of its edges with the
safe-area padding.
