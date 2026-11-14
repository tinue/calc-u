# LIB-15 Web frontend shell: design
Status: approved by the user at the Part B/C1 boundary (2026-11-14)

## 1. Scope

All three columns are worked in Part C1.

| Column | Step | Tests from (plan §3) | Done in (plan §5.1) |
|---|---|---|---|
| Library | M-29 | AC-1–18; EC-1–5 | M-29 |
| Calc-U-1600 | M-31 (with LIB-3 1600, LIB-17 1600) | AC-24–28 | M-46 (AC-16's bundle-file clause, D-4; plan §5.1 shows M-31) |
| Calc-U-59 | M-34 (with the session part of LIB-3 59) | AC-19–23; also LIB-3 AC-31, LIB-9 AC-23 | M-55 |

- M-29 waits for M-20 (LIB-9 L: `SettingsFile`, `Paths`, `HostDirs`),
  M-22 (LIB-3 L: the app session and its messages) and M-12 (both Web
  frontends build through the root project in `build/web`). It changes
  `library/` and the two `web` CI jobs only. No product code uses the
  shell until M-31.
- M-31 waits for M-29 and M-30 (LIB-9 1600: the INI file already in
  `/persist`). The Qt6 frontend and the harness move first (LIB-3 1600),
  then the Web frontend moves onto the shell.
- M-34 waits for M-29 and M-33 (Calc-U-59's Qt6 and Swift frontends on
  the library session; `SessionController` kept only for the Web). From
  M-33 to M-34 the Web keeps the old `SessionController` stack with
  LIB-17's `CooperativePacing`, and M-34 removes both (LIB-3 D-1, the old
  stack stays one step; reconciled with LIB-3, LIB-17).
- **AC-20 in parts** (plan M-34, §5.1). M-34 tests the settings part (the
  INI file in the persistent file system) and keeps the TI-58C memory as
  today's `ti58c.mem`, now a file in the save folder (LIB-3 59's `TIFileAccess`, §4),
  so that today's Playwright check (C59-3 AC-14) stays green. M-54 makes
  it `ti58c.memory.yaml` (C59-3) and M-55 puts the cards in the save
  folder (C59-7, LIB-7 59). The Calc-U-59 column is Done in M-55.
- No criterion of this spec lands outside these three steps. LIB-19
  (M-62, M-65) uses the embedding interface. It adds no criterion here.
- No first step of `docs/walkthroughs/first-steps.md` concerns this
  feature.

## 2. Files

### Library (M-29): `library/web/`

JavaScript in `library/web/src/` (classic scripts, concatenated into a
product's `calc.js` closure, so nothing of the library reaches the host
page's globals: AC-15). C++ glue in `library/web/glue/`, namespace
`calcu::web`: the one exception to the one library target (LIB-1 §2a),
because each Web engine compiles it itself. Tests in `library/web/tests/`.

| File | Holds |
|---|---|
| `library/web/CMakeLists.txt` | Under Emscripten: the INTERFACE target `calcu_web_glue` (its sources are compiled by each consumer, §3.6) and the test product `calcu_web_test_product`. Natively: the CTest entries `LibraryWebNodeTests` (`frequent`, when `node` is found) and, with `CALCU_TESTS_LONGER`, `LibraryWebPlaywright` (`longer`). Both are entries of their own beside `calcu_library_tests` because they run Node and browsers, not C++ |
| `library/web/glue/WorkerHost.hpp/.cpp` | `calcu::web::WorkerHost`: owns one app session in the worker, is its listener (a `calcu::sessions::MessageSink`), host call, message in and out (§3.6) |
| `library/web/glue/MessageCodec.hpp/.cpp` | a LIB-3 `Message` ↔ a JS object, through `Payload::toValue()` and `Value`; the sound messages (`SoundStart`, `SoundSamples`, `SoundStop`) as `Int16Array` (§3.6). Replaces Calc-U-1600's `WebAudioSink` (`bindings.cpp:133-191`) |
| `library/web/glue/Bind.hpp` | `CALCU_WEB_PRODUCT(name, factory)`: the embind registration a product's `bindings.cpp` makes |
| `library/web/src/base.js` | `CALC_BASE` (from both `src/engine.js:14-18`): every URL relative to `calc.js` (AC-13) |
| `library/web/src/shell.js` | page side: `createShell()` starts the worker, keeps the last state of each state topic and what each stream topic appended, suspend/continue on `visibilitychange`, the start state (§3.1) |
| `library/web/src/hostServices.js` | the page's host services: clipboard text and image, file picker, download, open URL (AC-4, AC-9) |
| `library/web/src/audio.js`, `library/web/src/audio-worklet.js` | the audio sink on the page side: AudioWorklet ring buffer, unlocked on the first gesture (from Calc-U-1600 `src/engine.js:52-93`, `audio-worklet.js`) |
| `library/web/src/embed.js` | `embedProps()`, `useFillParent()` (AC-16, EC-4, EC-5), `useTopic()` (React 18 `useSyncExternalStore`, as `useEngine` in both `src/engine.js`) |
| `library/web/src/sentences.js` | the sentences of AC-3, AC-8, EC-2, EC-3, the same for every product (TR) |
| `library/web/src/worker.js` | worker side: loads the product's WASM, loads `files.json` into the file system, the persistent file system, the 60 Hz loop (§3.2) |
| `library/web/src/storage.js` | worker side: the one-writer lock, the IDBFS mount, the four storage modes (§3.3) |
| `library/web/app/app.jsx` | the installable app page: fills the viewport, install button and iOS hint (from both `app/app.jsx`), parametrised by the product's component and sentence (AC-12) |
| `library/web/app/sw.js` | the service worker template (§3.4; AC-10, AC-11) |
| `library/web/app/engine-worker.js` | the app's entry to the worker (both `app/engine-worker.js`, identical today) |
| `library/web/build/dist.js` | Node, used by each product's `build.js`: `compileCalcJs`, `writeFilesList`, `stampServiceWorker`, `writeManifest`, `checkScopedCss` (§3.5) |
| `library/web/tools/serve.py` | the two test servers (root and `/anything/calc/`), from both `run-tests.sh` (identical but for the `.wasm` type) |
| `library/web/tests/product/` | the product made for the tests: `TestProduct.cpp` (LIB-1's `TestMachines`, LIB-3's sessions, two product topics, a product command that writes a file), `src/CalcUTest.jsx` (component `CalcUTest`), `calc.css`, `manifest.json`, `build.sh`, `build.js`. Built into `build/web/library/web/test-dist/<name>/`, twice (product names `Test-A`, `Test-B`, AC-7) and in two releases (AC-11) |
| `library/web/tests/*.spec.js`, `library/web/tests/playwright.config.js`, `library/web/tests/run-tests.sh`, `library/web/package.json` | Playwright, Chromium and WebKit (§6); `@playwright/test` as both products pin it (`^1.48.0`) |
| `library/web/tests/build.test.js` | `node --test`: MANIFEST, files list, service worker stamp, CSS scope check |
| `library/CMakeLists.txt` (changed) | `add_subdirectory(web)` |
| `.github/workflows/calc-u-59.yml`, `calc-u-1600.yml` (changed) | the `web` job also builds the test product and runs `library/web/tests/run-tests.sh` (D-8) |
| `docs/test-groups.md` (changed) | the two library entries |

### Calc-U-1600 (M-31): `products/calc-u-1600/frontends/web/`

| File | Change |
|---|---|
| `bindings.cpp` | reduced to the product factory and `CALCU_WEB_PRODUCT`. `WebController` (lines 242-905), `WebAudioSink` (137-191) and the embind surface (909-960) go. What it builds for the page today goes two ways. The state it announces (`postUi`, the lamps, the machine) arrives as LIB-3 1600's interim product topics of `SharpAppPart` (`c1600.control-state`, `c1600.machine`, `c1600.tapes`), encoded by `toValue()`. The page-only data (`faceplate`, `lcdConstants`) becomes product requests (adapter W16-1, §5) |
| `calc-engine-worker.js` | a product entry of a few lines: the product's worker config (§3.2), then `importScripts` of the library worker. The protocol of lines 17-29 goes |
| `src/engine.js` | deleted; the components use `createShell()` and `useTopic()`. `localStorage` is already gone in M-30 (LIB-9) |
| `src/CalcU1600.jsx` and the other components | every `engine.call(method)` / `engine.request(method)` becomes a command or request of LIB-3 or of the product (names: LIB-3 1600's design); props through `embedProps()`; the banner of lines 393-397 shows the library's storage sentence; `fatal` (lines 411-418) shows the start state |
| `audio-worklet.js` | deleted (library) |
| `app/app.jsx`, `app/sw.js`, `app/engine-worker.js` | deleted; `build.sh` copies the library's. `app/manifest.json`, `app/index.html`, `app/app.css` stay (product names, icons) |
| `build.js` | calls `library/web/build/dist.js`; its own steps 3-5 (lines 96-131) go; `files.json` is written by the library function |
| `build.sh` | copies `library/web/app/*` and `library/web/src/audio-worklet.js`; the M-12 configure and `cmake --build --target calcu_web` stay |
| `run-tests.sh`, `playwright.config.js` | `run-tests.sh` starts `library/web/tools/serve.py`; the config is unchanged |
| `index.html` | `?model=` takes the model's name (OQ-1 of the spec) |
| `calc.css` | every selector under `.calcu-client` (AC-17; today e.g. `.calcu-menubar`, line 73) |
| `tests/play.spec.js`, `offline.spec.js`, `prerender.spec.js` | the hooks (`data-ready`, `calcuEngine.request`) are rewritten on the shell; the checks of AC-28 stay; new: `audio.spec.js` (AC-26) |
| `products/calc-u-1600/CMakeLists.txt` | `calcu_web` links `calcu_library` and `calcu_web_glue`; link options unchanged |

### Calc-U-59 (M-34): `products/calc-u-59/frontends/web/`

| File | Change |
|---|---|
| `bindings.cpp` | reduced to the product factory and `CALCU_WEB_PRODUCT`. `WebController` over `SessionController`, `startupPlan`, `staticData`, `ledGeometry` as free functions, `store()` and the `Platform` overrides (lines 364-400) go. What `drain()` posts (lines 404-438) arrives as the display topic and LIB-3 59's interim topics of `TIAppPart` (`c59.model`, `c59.printer`, `c59.card-reader`, `c59.run-state`, `c59.trace`, `c59.debug.*`, `c59.cue-card`). The page-only data (`staticData`, `ledGeometry`, `mobileRules`) becomes product requests (adapter W59-1) |
| `calc-engine-worker.js` | product entry as for Calc-U-1600; `readRomFile` (sync XHR, lines 51-60), the `startupPlan` fetch and the message types of lines 9-33 go |
| `src/engine.js` | deleted; `STORAGE_PREFIX` `calcu59/` and every `localStorage` use go (LIB-9 AC-23) |
| `src/PlayCalculator.jsx` and the other components | props through `embedProps()`; `keyboard` on by default; `module` goes (AC-19); the `preset` prop goes into the start request instead of `fetchBesideCalc` + `loadState` (lines 200-203); `status` "Starting the emulator…" comes from the start state (AC-3) |
| `app/*`, `build.js`, `build.sh`, `run-tests.sh` | as for Calc-U-1600; `build.js` steps 3-4 (lines 81-117) go; `dist/files.json` is new |
| `index.html` | `?module=` goes |
| `calc.css` | every selector under `.calcu-client` (today e.g. `.calcu-device`, `.calcu-wide`, lines 42-63); the `@font-face` family gets a name of its own (§8) |
| `tests/play-calculator.spec.js`, `offline.spec.js`, `prerender.spec.js` | hooks rewritten; the checks of AC-23 stay; "Settings: startup model and module are kept" (line 199) keeps the model only (row 59-9); new: `storage.spec.js` (AC-20 settings part, AC-21) |
| `products/calc-u-59/CMakeLists.txt` | `calcu59_web` links `calcu_web_glue` (it links `calcu_library` through the core since M-19, with `-fwasm-exceptions`, LIB-1 D-6), and adds `-lidbfs.js` and `-sEXPORTED_RUNTIME_METHODS=FS,IDBFS` (lines 82-84) |
| `products/calc-u-59/app/SessionController.*`, `tests/FakePlatform.hpp`, `tests/shared_controller_tests.cpp`, `tests/shared_cooperative_tests.cpp` | deleted (plan M-34); their checks are ported to the library session's tests or replaced by them (LIB-3 59's design) |

## 3. Seams and interfaces

### 3.1 Page side (`shell.js`, `embed.js`): AC-1–4, AC-9, AC-13, AC-16, AC-18, EC-4, EC-5

```js
// Created in an effect only, never during a render (AC-18).
createShell({
  workerUrl,            // prop, or CALC_BASE + "calc-engine-worker.js" (AC-13, AC-16)
  start: { model, preset },   // read once at mount (EC-5)
  services,             // default: hostServices.web() — the list of AC-4
}) → {
  getState(), subscribe(listener),        // for useTopic()
  send(command, args) → Promise<reply>,   // LIB-3 command: one reply (LIB-3 AC-13)
  request(name, args) → Promise<answer>,  // LIB-3 request (LIB-3 AC-8)
  frames,                                 // the display topic, for a canvas, outside React
  unlockAudio(), destroy(),
}
// state: { start: "starting" | "running" | "failed", failure, topics: {name → last state},
//          streams: {name → appended since the last reset marker} }

embedProps(props) → { width, height, keyboard /*default true*/, model, preset, workerUrl, rest }
useFillParent(ref, {width, height}) → {width, height}   // without both: the parent's size (AC-16)
useTopic(shell, name, select)
```

- **Wire.** Everything `postMessage` carries is a LIB-3 message in the
  JS form of §3.6: a command, request, reply, state topic, stream topic,
  host service question or answer, the "listen" message with the host
  services, suspend and continue. No message type of its own (AC-1,
  AC-22, AC-27). The start parameters go in the first command,
  `start` (D-1, §9 N3).
- **Start state (AC-3).** `failed` is set by the page on
  `worker.onerror`, on `worker.onmessageerror`, and when the worker sends
  the library's state topic `web.start` with `failed` and the reason (a
  file it could not fetch with its path and status, the WASM that did not
  compile or start, an abort). The sentence is from `sentences.js`. The
  product's component renders the start state (today: Calc-U-1600's
  `fatal` dialog, Calc-U-59's `status` line).
- **Suspend (AC-2).** `visibilitychange`: hidden → the `suspend`
  command, and the worker stops its loop; shown → `continue`, and the loop
  starts again (both products today: `src/engine.js:148` /
  `:180-181`). The order matters: Calc-U-59 calls `suspend()` before
  stopping (`calc-engine-worker.js:118`); the worker sends the command to
  the session first, then stops calling.
- **Host services (AC-4, AC-9).** `hostServices.web()` states LIB-3's
  `AudioSink`, `ClipboardText`, `ClipboardImage`, `FilePicker`,
  `KeepFile` (the download), `OpenUrl`; no `FolderPicker`, no `Haptics`.
  A `ServiceAsk` from the session gets one `ServiceAnswer`: the file picker
  with `{name, bytes}` or "cancelled" (from `CalcU1600.jsx:19-38`); a
  download through a Blob URL (both `downloadBytes`); `open-url` with
  `window.open(url, "_blank", "noopener")`; clipboard through
  `navigator.clipboard`. A picked file's bytes go back in the answer; the
  worker writes them to `/uploads/<name>` and passes the path (as
  Calc-U-1600's `upload`, `bindings.cpp:319-324`).
- **Props (AC-16, EC-4, EC-5).** `embedProps` only names and defaults
  the props. Whether a `model` or a `preset` names something is decided
  by the session's start (LIB-18 AC-5; until M-35 by the product's app
  layer), which answers with a notice and starts as without it (EC-4).
  The shell never reads `start` again (EC-5).

### 3.2 Worker side (`worker.js`): AC-1–3, AC-10, AC-14

```js
// products/<p>/frontends/web/calc-engine-worker.js
self.CALCU_PRODUCT = {
  name: "Calc-U-1600",                 // as the glossary writes it
  wasm: "calcu1600-core",              // dist/wasm/<wasm>.js, .wasm
  factory: "createCalcU1600Module",
  host: "CalcU1600Host",               // the class CALCU_WEB_PRODUCT registers
  mount: "/persist",                   // §3.3
  lock: "calcu1600-storage",
};
importScripts(`${self.CALC_ROOT || ""}library-worker.js`);
```

1. `fetch("files.json")`; every file it lists is fetched and written to
   `/bundle` or `/presets` (as Calc-U-1600 today,
   `calc-engine-worker.js:102-112, 130-142`). Any fetch that fails is a
   failed start (AC-3; LIB-12 EC-3). For Calc-U-59 this replaces the
   synchronous `readRomFile` and the startup plan: everything is there
   before the session starts (OQ-6 of the spec, below).
2. The persistent file system (§3.3), in parallel with step 1.
3. `new Module[host]({ dirs, storage })`, then the "listen" message with
   the page's host services, then the queued messages (as both workers'
   `pending` today).
4. The loop: `setInterval(call, 1000 / 60)`; `call` passes the wall time
   (AC-1). `suspend` and `continue` stop and start it (AC-2).
5. A C++ abort or an uncaught exception in `call` ends the loop and sends
   `web.start` = `failed` (from Calc-U-1600's `errorText`, lines 48-56).

### 3.3 Persistent file system (`storage.js`): AC-5–8, EC-1–3

| Mode | When | Mount | Sentence |
|---|---|---|---|
| `kept` | the lock is held, or there is no lock API (EC-1), and the read succeeded | IDBFS, `autoPersist: true` | none |
| `second-copy` | the lock is held by another copy (AC-8) | IDBFS, `autoPersist: false`, read once | AC-8 |
| `unreadable` | `syncfs(true)` failed (EC-2) | memory; nothing is written to the store | EC-2 |
| `unavailable` | IndexedDB cannot be opened, or a write fails because the store is full (EC-3) | memory | EC-3 |

- **One writer (AC-8).** `navigator.locks.request(lock, {ifAvailable:
  true})`, held for the worker's life (Calc-U-1600 today,
  `calc-engine-worker.js:116-124`). Locks are per origin, so two tabs and
  two components on one page are covered alike. Without `navigator.locks`
  (plain HTTP, EC-1) every copy is `kept`.
- **One store per product (AC-7).** Emscripten's IDBFS names its database
  after the mount point. Two products that both mount `/persist` would
  share one database, and a sync of one would remove the other's files.
  So each product has its own mount point: Calc-U-1600 keeps `/persist`
  (AC-25: the same store and place), Calc-U-59 gets `/persist-calc-u-59`.
  The lock names follow (`calcu1600-storage` as today, `calcu59-storage`).
- **Layout inside the mount** (LIB-9's `HostDirs` for `FrontendKind::Web`):
  `settingsBase = <mount>/settings`, so the settings file is
  `<mount>/settings/<Product>/<Product>.ini`, and
  `saveFolder = <mount>/<Product>`. For Calc-U-1600 this is
  `/persist/Calc-U-1600`, today's save folder (`AppPaths.cpp:61-66`,
  `bindings.cpp:52`). D-7.
- **Not written over (EC-2).** The mount starts with `autoPersist: false`;
  it is switched on only after a successful read in mode `kept`.
- **A change is kept (AC-6).** `autoPersist` queues a sync after each
  write, rename and delete. The test checks the second.
- The storage mode is the library's state topic `web.storage`
  (`{mode, sentence}`), sent at "listen" like every state topic (LIB-3
  AC-6). The sentence also goes once into the notices.

### 3.4 Installable offline app (`app/`): AC-10–12

- `sw.js`: `CACHE_PREFIX = "<product id>-app " + registration.scope + " "`,
  the name stamped with a hash of the precached files (both products
  today). The list is the core files plus every file of `files.json`
  (the ROMs, the bundled presets and media) and the product's images.
- **Update (AC-11).** Today both call `skipWaiting()` and
  `clients.claim()` on every install. A new release then takes over a
  running page, and a file the page fetches later comes from the new
  release. The template calls them only on the first install. Otherwise
  the new worker waits until no window of the app is open, and its
  `activate` removes the older caches of the same prefix. The fetch
  handler answers from its own `CACHE_NAME` only (today: `caches.match`
  over every cache). The persistent file system is IndexedDB and is never
  touched by the service worker. A user-visible change for both
  products (D-2; ledger rows to add).
- `app.jsx`: the shared `useInstallPrompt` and `InstallBanner` (both
  products' copies agree but for the wording), with
  `{Component, installSentence}` from the product (AC-12).

### 3.5 Build (`build/dist.js`): AC-13–15, AC-17

```js
compileCalcJs({ librarySources, productSources, version, out })  // one closure, one global (AC-15)
writeFilesList(dist, { bundle: "bundle", presets: "presets" })   // dist/files.json (spec OQ-6)
stampServiceWorker(dist)                                         // as both build.js today
writeManifest(dist, { name, version })                           // { name, version, commit, files } (AC-14)
writeLibraryWorker(dist)        // src/worker.js + storage.js + sentences.js → dist/library-worker.js (§3.2)
checkScopedCss(cssText) → [selectors outside .calcu-client]      // AC-17
```

- **`files.json` (spec OQ-6: one way for both).** Both products list
  their bundled files in `dist/files.json`, which the worker reads. It is
  not part of the embedding interface (AC-14). Calc-U-59's fetch by name
  goes.
- `MANIFEST.json` `name` stays as today (`Calc-U-59-web`,
  `Calc-U-1600-web`). The fields are the same in both.
- The product's `build.js` keeps its own version source (Xcode
  `MARKETING_VERSION`, Qt `APP_VERSION`) and its preset list until LIB-6
  (`presets.json`, `examples/bundled.yaml`).

### 3.6 C++ glue (`calcu::web`): AC-1, AC-2, AC-4, AC-22, AC-26, AC-27

```cpp
namespace calcu::web {

struct WebStart {                       // what the worker passes in (§3.2)
    calcu::settings::HostDirs dirs;     // FrontendKind::Web, settingsBase, saveFolder (§3.3)
    std::string bundleDir = "/bundle", presetsDir = "/presets", uploadsDir = "/uploads";
    StorageMode storage;                // kept | second-copy | unreadable | unavailable
};
using AppSessionFactory =
    std::function<std::unique_ptr<calcu::sessions::AppSession>(const WebStart&)>;  // Hosting::Paced

class WorkerHost : public calcu::sessions::MessageSink {   // one per worker; single-threaded (ADR 0009)
public:
    WorkerHost(AppSessionFactory, emscripten::val start);  // then session.listen(*this, services from the page)
    void receive(emscripten::val message);   // command → send(), request → ask(), service answer → answer()
    void call();                             // session.call(WallTime from emscripten_get_now())
    void deliver(const calcu::sessions::Message&) override;  // encode → postMessage, at once, in order
};

// MessageCodec
emscripten::val encode(const calcu::sessions::Message&);   // StateMessage, StreamMessage (reset), ReplyMessage,
                                                           // ServiceAsk, SoundStart/Samples/Stop; payload via toValue()
emscripten::val toJs(const calcu::sessions::Value&);       // bytes → Uint8Array (transferred)
calcu::sessions::Value fromJs(emscripten::val);            // command and request arguments, service answers

}  // namespace calcu::web

// products/<p>/frontends/web/bindings.cpp, all that is left there:
CALCU_WEB_PRODUCT(CalcU1600Host, &shared::makeWebAppSession)
```

- The glue only translates (I-3, ADR 0014): it reads no topic, decides no
  rule, keeps no state but the session. The session's
  messages go out at once, in order; `postMessage` is the queue of I-2
  (LIB-3 AC-4).
- **Sound.** The page states `AudioSink`, so the session sends this
  listener `SoundStart{rate}`, then `SoundSamples` at regular speed only
  (LIB-3 design §3.6). The codec posts them as they come, and `audio.js`
  plays them. What is heard is the session's decision (LIB-3 AC-20,
  AC-37); the glue only carries it. Matching the device rate is
  `audio-worklet.js`'s (LIB-3 EC-5).
- A command name the session does not know is refused by the session
  (LIB-3 AC-13). The glue refuses only a message it cannot decode, with a
  reply.
- **Exception modes.** `calcu_web_glue` is an INTERFACE library with
  `target_sources(... INTERFACE ...)`, so its sources are compiled inside
  each engine (`calcu59_web`, `calcu_web`) with that engine's flags. Both
  engines use `-fwasm-exceptions` from M-19 on, as does `calcu_library`
  in `build/web` (LIB-1 D-6); the test product does too. Product code
  behind a library interface still catches at its boundary (LIB-1 §2a:
  no exceptions across a library interface).

## 4. Product side

**Calc-U-1600 (M-31).** `calc-engine-worker.js` becomes the product
entry (§3.2) and `WebController` goes. Its parts go to:
- the app session, through LIB-3's commands and requests;
- `shared::makeWebAppSession` (a new `products/calc-u-1600/app/WebAppSession.cpp`):
  `HostDirs`, the `SettingsFile` from M-30, the session with the product's
  topics, and the start (today's `startupJob`, `bindings.cpp:236-240`,
  which takes a model name now, OQ-1 of the spec);
- `SharpAppPart`'s interim topics (LIB-3 1600's design) and W16-1.

`m_app->pacer().setTurbo` becomes the speed command (LIB-17 design).
Deleted: `WebController`, `WebAudioSink`, `src/engine.js`, the own
service worker, app page and worker entry, `audio-worklet.js`, steps
3-5 of `build.js`. Stays: the components (LIB-14 changes them in M-49),
`MAX_DROP_BYTES` (LIB-13), the Playwright hooks (rewritten), the M-12
build in `build/web`, the Emscripten version (6.0.13 in CI).

**Calc-U-59 (M-34).** The Web frontend hosts the library's app session,
as Qt6 and Swift do since M-33. `SessionController` goes with its last
users, and with it the `CooperativePacing` adapter (LIB-17 design §5)
and LIB-9's `WebIniMirror` (A-5). The settings file is in
`/persist-calc-u-59/settings/Calc-U-59/Calc-U-59.ini` (LIB-9 AC-23).
`makeWebAppSession` in a new `products/calc-u-59/app/WebAppSession.cpp`
does what `startupPlan` and the `WebController` constructor do today
(`bindings.cpp`, `Module.startupPlan`, `calc-engine-worker.js:132-136`),
without `module` (AC-19; row 59-9). The constant memory and the cards go
through LIB-3 59's `TIFileAccess` (M-33) on the Web's save folder, so
`ti58c.mem` is a file in the persistent file system until M-54 (C59-3),
and the cards are files there until M-55 (C59-7; D-9). Traces stay in
MEMFS `/traces` (LIB-9 A-3, until M-53). Deleted: as for Calc-U-1600,
plus `readRomFile`, `startupPlan`, `staticData` and `ledGeometry` as
bindings (they become product requests in W59-1), and the `module` prop.
Stays: the Emscripten version (6.0.12 in CI).

## 5. Adapters

| Adapter | File | Wraps | Removed in |
|---|---|---|---|
| W16-1 page data requests (Calc-U-1600) | `products/calc-u-1600/app/WebPageData.hpp/.cpp` | `WebController::faceplate`, `lcdConstants` (`bindings.cpp:959-960`) as product requests `c1600.faceplate`, `c1600.lcd-constants`. They are static product data the Web page draws with today; no other frontend asks for them | M-49 (LIB-14 1600: the component set draws from descriptor data) |
| W59-1 page data requests (Calc-U-59) | `products/calc-u-59/app/WebPageData.hpp/.cpp` | `staticData`, `ledGeometry`, `mobileRules` (`bindings.cpp:1043-1046`, the `mobileRules` method) as product requests `c59.static-data`, `c59.led-geometry`, `c59.mobile-rules` | M-57 (LIB-14 59) |

The interim product topics that carry today's change flags and callbacks
are LIB-3's (`SharpAppPart`, `TIAppPart`; LIB-3 design §5). The Web
frontends read them like the Qt6 frontends do, so this feature adds none.
`TIFileAccess` (cards, constant memory) is LIB-3 59's. The Web uses it
from M-34 on.

Removed by this feature: Calc-U-59 `SessionController` (M-34), LIB-17's
`CooperativePacing` (M-34; LIB-3 D-1), LIB-9's A-5
`WebIniMirror` (M-34). M-29 adds
no adapter. W16-1 and W59-1 are listed for M-59's adapter check.

## 6. Tasks

Tests before code. Group: Playwright entries are Longer (ADR 0031,
`docs/test-groups.md`); `node --test` is Frequent. `L/` =
`library/web/tests/`; `P16/` = `products/calc-u-1600/frontends/web/tests/`;
`P59/` = `products/calc-u-59/frontends/web/tests/`. Test names start with
the criterion.

| Task | What | ACs/ECs served | Test (file, case name, group) |
|---|---|---|---|
| T1 | test product, `serve.py`, two builds and two releases | (all of L) | `L/run-tests.sh` builds it; Longer |
| T2 | `WorkerHost`, `MessageCodec`, `worker.js` loop | AC-1 | `L/hosting.spec.js`: `ac1 the session runs in a worker and the page gets only LIB-3 messages`, `ac1 about 60 host calls a second`; Longer |
| T3 | suspend and continue | AC-2 | `L/hosting.spec.js`: `ac2 hidden stops the calls, shown continues them`; Longer |
| T4 | start state and failures | AC-3 | `L/hosting.spec.js`: `ac3 a missing bundled file`, `ac3 a broken wasm`, `ac3 a worker script error`; Longer |
| T5 | host services | AC-4, AC-9 | `L/hosting.spec.js`: `ac4 states its services and no folder picker`, `ac9 a handed file is a download`, `ac4 file picker, clipboard text, open url answered once`; Longer |
| T6 | `storage.js`, IDBFS mount, layout | AC-5, AC-6 | `L/storage.spec.js`: `ac5 settings and save folder survive close and open`, `ac6 write rename delete found after one second`; Longer |
| T7 | one store per product | AC-7 | `L/storage.spec.js`: `ac7 Test-A and Test-B on one origin see only their own`; Longer |
| T8 | one writer | AC-8, EC-1 | `L/storage.spec.js`: `ac8 second tab works in memory and says so`, `ac8 twice on one page`, `ec1 plain http every copy writes` (Chromium, host mapped to a non-local name); Longer |
| T9 | storage modes `unreadable`, `unavailable` | EC-2, EC-3 | `L/storage.spec.js`: `ec2 unreadable store is not written over` (IndexedDB stubbed to fail the read), `ec3 no store works in memory and says so`; Longer |
| T10 | `sw.js`, `app.jsx` | AC-10, AC-12 | `L/offline.spec.js`: `ac10 installed app starts and runs with the server gone`, `ac12 install button, iOS hint`; Longer, Chromium and WebKit |
| T11 | update rule | AC-11 | `L/offline.spec.js`: `ac11 the new release from the next start, all files, old cache gone, store untouched`; Longer |
| T12 | `CALC_BASE`, relative URLs | AC-13 | every `L/*.spec.js` runs in the projects `root` and `subpath`; Longer |
| T13 | `dist.js` | AC-14, AC-15, AC-17 | `L/build.test.js`: `ac14 manifest fields and hashes`, `ac15 one global, nothing else`, `ac17 scope check finds an unscoped selector`; Frequent. `L/embed.spec.js`: `ac15 host page with React 18 global mounts CalcUTest`, `ac17 host styles unchanged`; Longer |
| T14 | `embedProps`, `useFillParent` | AC-16, EC-4, EC-5 | `L/embed.spec.js`: `ac16 props and defaults`, `ac16 fills its parent`, `ec4 unknown model and preset give a notice`, `ec5 changed props are not read again`; Longer |
| T15 | prerender | AC-18 | `L/prerender.spec.js`: `ac18 renders on a server without fetch or worker`; Longer |
| T16 | Calc-U-1600 on the shell | AC-24, AC-27 | `P16/play.spec.js`: `ac24 CalcU1600 takes the AC-16 props` (with `?model=PC-1500`), `ac27 no message of the old protocol` (the worker's incoming types checked against the LIB-3 shapes); Longer |
| T17 | 1600 settings and media | AC-25 | `P16/play.spec.js`: `ac25 settings in Calc-U-1600.ini`, `ac25 media saved by 0.8.1 still listed` (IndexedDB `/persist` seeded as 0.8.1 writes it); Longer |
| T18 | 1600 sound | AC-26 | `P16/audio.spec.js`: `ac26 buzzer samples at regular speed, none at full speed or in an operation` (the product's test hook counts samples); Longer |
| T19 | 1600 regression | AC-28 | `P16/play.spec.js` cases at lines 44, 236, 329, 337; `offline.spec.js:40`; `prerender.spec.js:32`, kept with new hooks; Longer |
| T20 | Calc-U-59 on the shell | AC-19, AC-22 | `P59/play-calculator.spec.js`: `ac19 PlayCalculator props, layout and settings, no module`, `ac22 no message of the old protocol`; Longer |
| T21 | 59 persistent file system | AC-20 (settings part; `ti58c.mem` through `TIFileAccess`), LIB-9 AC-23 | `P59/storage.spec.js`: `ac20 settings survive a page load`, `lib9_ac23 no calcu59 key read or written`, and line 181's constant-memory test kept; Longer. Cards: C59-7's tests in M-55; `ti58c.memory.yaml`: C59-3's in M-54 |
| T22 | 59 one writer, suspend | AC-21, LIB-3 AC-31 | `P59/storage.spec.js`: `ac21 a second tab says it cannot save`; `P59/play-calculator.spec.js`: `lib3_ac31 hidden page suspends`; Longer |
| T23 | 59 regression | AC-23 | `play-calculator.spec.js:71` (both layouts), `:471-491` (example offline), `offline.spec.js:47` (Chromium, WebKit), `prerender.spec.js:33`, kept with new hooks; Longer |

Every criterion assigned to the C1 steps (AC-1–28, EC-1–5: 33) is in at
least one task. AC-20's card and `ti58c.memory.yaml` parts are tested in
M-55 and M-54.

## 7. Tools and agent knowledge

- **Today's tools.** None uses the Web frontends. `tools/build_wasm_check.sh`
  and `tools/wasm_smoke.cpp` (Calc-U-1600) build in `build/web` and stay
  (LIB-17 moves `runEmulatedSeconds` in M-31).
- **M-29:** `docs/test-groups.md` (two library entries). No product
  knowledge yet (plan: none).
- **M-31:** `products/calc-u-1600/docs/developer/Web-Client.md` (the
  worker protocol, `files.json`, props: `model` by name, `localStorage`
  gone, the store and its sentence), `docs/developer/Building.md` (the
  web build uses `library/web/`), `products/calc-u-1600/CLAUDE.md` (web
  section), the memory `calc-u-1600/project_shared_layer_wasm.md` (names
  `AppSession` and the web GUI). Renames: `WebController` goes, with no
  successor in the product. `AppSession` → app session, as LIB-3's step.
- **M-34:** the `calc-u-59-web` skill
  (`products/calc-u-59/.claude/skills/calc-u-59-web/SKILL.md`: protocol,
  `localStorage`, `module`), `products/calc-u-59/reference/WebClient.md`
  (props, storage, `dist/` contents, lines 23-25 and 133-176),
  `products/calc-u-59/CLAUDE.md` (web section); renames
  `SessionController` → app session (`docs/glossary-mapping.md`, row "App
  session"); ledger row 59-8 (the one-writer rule) becomes visible.

## 8. Golden runs and risks

- **Golden runs.** No step of this feature touches a core. Both products'
  replays (`products/calc-u-59/tests/golden_runs_tests.cpp`,
  `products/calc-u-1600/tests/golden_runs_tests.cpp`) run unchanged in
  Frequent. In M-34 the Web loses `SessionController`, on which M-9's 57
  TI reference runs were taken. They are compared again in M-54 on the
  library session. Until then the Playwright suites are the guard.
- **IDBFS database name** (§3.3): if a product mounted `/persist` beside
  Calc-U-1600 on calc-u.org, a sync could delete the other product's
  files. T7 guards it.
- **Update rule** (§3.4): an installed app with a window always open
  stays on the old release until it is closed. The cost is one more
  start. This is the reading of AC-11 (D-2).
- **CSS today fails AC-17.** Both `calc.css` have selectors outside
  `.calcu-client`. The scope check fails the build once it is on (M-31,
  M-34). Calc-U-59's `@font-face` "Archivo" is document-wide, and a host
  page that names the same family would get it. The design names the
  family `Calc-U Archivo`. Dialogs drawn outside the root element would
  lose their styles: to check in each product (not verified).
- **Preloading Calc-U-59's files** (§3.2): every ROM and module file is
  fetched at start (`roms/calculator` 96 KB, `roms/solid-state` 292 KB
  on disk). This is more than today's start set, but there is no
  synchronous XHR.
- **Emscripten versions.** CI pins 6.0.12 (59) and 6.0.13 (1600); local
  builds use Homebrew 7.0 (commit `bb46dd5`). Not verified:
  `autoPersist` and toggling it after a read (§3.3) in 6.0.12. If the
  toggle does not take, the worker remounts after the read.
- **Not verified:** how IDBFS reports a failed `autoPersist` write (EC-3,
  a full store mid-session); Web Locks in the WebKit that Playwright
  ships (Safari has had them since 15.4).

## 9. Decided (best guess)

- **D-1 (was OQ-1; AC-1, AC-3).** The start parameters, the start failure
  and the storage mode travel in LIB-3's shapes as a command and topics
  this feature owns (`start`, `web.start`, `web.storage`); LIB-3 adds no
  message kind.
- **D-2 (was OQ-2; AC-11).** As drafted: a new release takes over from
  the next start, not over a running page. User-visible for both
  products: ledger rows to add.
- **D-3 (was OQ-3; EC-3).** Only a store that fails is detected (open
  fails, a write is refused); private mode as such cannot be told by any
  browser API.
- **D-4 (was OQ-4; AC-16, EC-4).** Plan §1 applies: the bundle-file clause
  passes in LIB-6's steps (M-46 for Calc-U-1600, M-54 for Calc-U-59), and
  LIB-15 1600 becomes Done in M-46 (plan §5.1 still shows M-31 and
  follows). Until then the product's app layer checks against today's
  lists (`examples/bundled.yaml`, `presets.json`), as today. Calc-U-59 is
  Done in M-55 already.
- **D-5 (was OQ-5; AC-17).** The `@font-face` family is renamed
  `Calc-U Archivo` (§8); not visible to the user.
- **D-6 (was OQ-6 = LIB-1 OQ-6).** Both engines use `-fwasm-exceptions`;
  Calc-U-59's Web moves in M-19 (LIB-1 D-6). One `build/web` tree, one
  `calcu_library`; the glue stays an INTERFACE target (§3.6).
- **D-7 (was OQ-7 = LIB-9 OQ-6).** Closed: one mount per product
  (`/persist`, `/persist-calc-u-59`), `settingsBase = <mount>/settings`,
  `saveFolder = <mount>/<Product>` (§3.3).
- **D-8 (was OQ-8 = LIB-1 OQ-5).** No third workflow (ADR 0004): the
  library's Web tests run in each product workflow's `web` job, which has
  emsdk and Playwright, so the shell is tested with both Emscripten
  versions; the `library` job runs `calcu_library_tests` and
  `LibraryWebNodeTests`.
- **D-9 (was OQ-9; AC-20, row 59-8).** The Web uses `TIFileAccess` from
  M-34, so cards survive a page load from M-34, before M-55; no release
  lies between, and rows 59-6 and 59-8 already describe it.
- **Not verified:** the items at the end of §8.

**What this design takes from the LIB-3 design
(`features/LIB-3-sessions-operations-and-messages/design.md`, read on
2026-11-14), and what it decides beside it.**

Taken as drafted:
- `AppSession::listen(MessageSink&, HostServices)`, `call(WallTime)`,
  `send`, `ask`, `answer`; `Hosting::Paced` for the Web (LIB-3 §3.7-3.8).
- `Message` = `StateMessage | StreamMessage | ReplyMessage | ServiceAsk |
  SoundStart | SoundSamples | SoundStop`, delivered in order on the
  session's thread. The queue is the host's: here `postMessage` (§3.2).
- `Payload::toValue()` and `Value` (JSON-like, with bytes): the codec
  translates every payload, product payloads too, without reading them
  (LIB-3 §3.1).
- `HostService` names: `AudioSink`, `ClipboardText`, `ClipboardImage`,
  `FilePicker`, `FolderPicker`, `KeepFile`, `OpenUrl`, `Haptics`.
- The commands `suspend` and `continue`.
- The interim product topics of `SharpAppPart` and `TIAppPart`, and
  `TIFileAccess` (LIB-3 §5).

Decided here (best guess; LIB-3 needs no change for them):
- **N1.** The JS wire form is LIB-15's, as LIB-3 defines no wire form:
  `{kind: "state"|"stream"|"reply"|"ask"|"sound-start"|"sound"|
  "sound-stop", topic, reset, value, call, done, reason}` and, page to
  worker, `{kind: "command"|"request"|"answer"|"listen", name, args,
  call}`.
- **N2.** Topic names owned by LIB-15: `web.start`, `web.storage`, generic
  topics of another feature (LIB-3 AC-9). `WorkerHost` declares them in
  the session when it builds it, like any feature's topics; `web.start`
  = `failed` before the session exists is sent by the worker in the same
  shape (D-1).
- **N3.** The `start` command, with an optional model name and an
  optional bundled preset path (LIB-18 AC-5). Before M-35 it belongs to
  the product's part (`SharpAppPart`, `TIAppPart`).
- **N4.** Argument and answer shapes of the host services: `KeepFile`
  `{name, bytes, mediaType}`; `FilePicker` `{accept, multiple}` →
  `[{name, bytes}]` (the glue writes them to `/uploads` and answers with
  the paths); `ClipboardImage` as PNG bytes (today Calc-U-1600 makes the
  PNG in the page, `CalcU1600.jsx:41-57`); `OpenUrl` `{url}`.
- **N5.** The display payload's `toValue()` gives its frame as `bytes`,
  so that the worker can transfer it at 60 Hz without a copy into JS
  arrays.
