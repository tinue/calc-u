# LIB-15: QA report

Results per acceptance criterion and edge case of `spec.md`, per column.

## Library (M-29, 2026-11-15)

Built and run on an M4 Pro, macOS 27.2, Homebrew Emscripten 7.0, Node 26,
Playwright 1.65.0 (Chromium and WebKit as it ships them). The code is
`library/web/`: the page side in `src/` (concatenated into a product's
`calc.js` closure), the worker side `src/worker.js`, `src/storage.js`,
`src/sentences.js` (into `dist/library-worker.js`), the app page and
service worker in `app/`, the build functions in `build/dist.js`, the
test server `tools/serve.py`, and the C++ glue in `glue/` (namespace
`calcu::web`, the INTERFACE target `calcu_web_glue`). The tests use the
product made for the tests, `library/web/tests/product/` (LIB-1's test
machines `test-seg` and `test-dot`, LIB-3's app session, LIB-9's settings
file, the product part `tp.`), built by `tests/product/build.sh` into
`build/web/library/web/test-dist/` as `Test-A` and `Test-B` (release
1.0.0) and `Test-A-2` (`Test-A`, release 2.0.0).

Result of the runs:
- `LibraryWebNodeTests` (Frequent, `node --test library/web/tests/build.test.js`):
  4 tests passed, 0.27 s.
- `LibraryWebPlaywright` (Longer, `library/web/tests/run-tests.sh`): 76
  cases, 74 passed, 2 skipped (`ec1` runs in the Chromium root project
  only), 0 failed, 35 s plus the test product's build. Projects: `root`
  and `subpath` (Chromium, `dist/` at a server root and below
  `/anything/calc/`), `webkit` (WebKit at the root), `offline-chromium`,
  `offline-webkit`, `prerender` (Node).
- The whole Frequent group passed in the RelWithDebInfo tree (14 entries,
  38.9 s), the golden replays of both products among them, unchanged
  (`CalcU59Tests` 228 tests, `CoreTests` 850 tests, 0 skipped).
- Both products' Web engines (`calcu59_web`, `calcu_web`) still build in
  `build/web` beside `calcu_web_test_product`.
- CI was not run in this step; the `web` job of both product workflows
  now also runs the library's Playwright suite with its own Emscripten
  (6.0.12, 6.0.13; design D-8).

Test files are `library/web/tests/`; "all projects" means `root`,
`subpath` and `webkit` (the page tests) or both offline projects.

| ID | Result | Test case (file) |
|---|---|---|
| AC-1 | Pass | `ac1 the session runs in a worker and the page gets only LIB-3 messages` (every message both ways recorded on the page: only `listen`, `command`, `request`, `answer` out, only `state`, `stream`, `reply`, `ask`, `sound*` in), `ac1 about 60 host calls a second` (the session's own count of host calls over 2 s: between 40 and 75 a second) (`hosting.spec.js`, all projects) |
| AC-2 | Pass | `ac2 hidden stops the calls, shown continues them` (`hosting.spec.js`, all projects) |
| AC-3 | Pass | `ac3 a missing bundled file` (a file of `files.json` answered 404: "Test-A could not start: bundle/demo.txt could not be fetched (404)"), `ac3 a broken wasm`, `ac3 a worker script error` (a `workerUrl` whose script throws); in each, the "Starting" line is gone (`hosting.spec.js`, all projects) |
| AC-4 | Pass | `ac4 states its services and no folder picker` (the session's own list of what the page stated), `ac4 file picker, clipboard text, open url answered once` (a picked file reaches the session as `/uploads/pick.txt`; a new tab; the clipboard in Chromium only, where Playwright grants clipboard access; each ask answered exactly once) (`hosting.spec.js`, all projects) |
| AC-5 | Pass | `ac5 settings and save folder survive close and open` (a save-folder file and `[startup] model` in `/persist-test-a/settings/Test-A/Test-A.ini`, read by the next page's start) (`storage.spec.js`, all projects) |
| AC-6 | Pass | `ac6 write rename delete found after one second` (`storage.spec.js`, all projects) |
| AC-7 | Pass | `ac7 Test-A and Test-B on one origin see only their own` (`storage.spec.js`, all projects) |
| AC-8 | Pass | `ac8 second tab works in memory and says so` (the sentence as banner and notice; read once, writes kept in memory; still a second copy after the first tab closed; nothing of it in the store afterwards), `ac8 twice on one page` (`storage.spec.js`, all projects, WebKit included) |
| AC-9 | Pass | `ac9 a handed file is a download` (name and content) (`hosting.spec.js`, all projects) |
| AC-10 | Pass | `ac10 installed app starts and runs with the server gone` (both models run, a bundled preset and a bundled file load from the cache) (`offline.spec.js`, Chromium and WebKit) |
| AC-11 | Pass | `ac11 the new release from the next start, all files, old cache gone, store untouched` (with the new release waiting, a reload stays on 1.0.0, `calc.js` and the worker's bundled `version.txt` alike; the next start has 2.0.0 for both, one cache, a different one, and the file written under 1.0.0) (`offline.spec.js`, Chromium and WebKit) |
| AC-12 | Pass | `ac12 install button, iOS hint` (Chromium: the button on the browser's install offer, simulated with a `beforeinstallprompt` event, as no headless browser offers an install; WebKit: no button without it, the hint with an iPad user agent) (`offline.spec.js`) |
| AC-13 | Pass | every page test runs in `root` and `subpath`; the offline tests serve `dist/` below `/calc/` |
| AC-14 | Pass | `ac14 manifest fields and hashes` (`build.test.js`, Frequent); the test product's `MANIFEST.json` is written by the same function |
| AC-15 | Pass | `ac15 one global, nothing else` (`build.test.js`, Frequent), `ac15 host page with React 18 global mounts CalcUTest` (`embed.spec.js`, all projects) |
| AC-16 | Pass | `ac16 props and defaults` (`width`, `height`, `keyboard` on by default and off, `model` by name, `preset` by its path in the bundle, `workerUrl`), `ac16 fills its parent` (`embed.spec.js`, all projects). The bundle-file clause stays with LIB-6 (design D-4): the test product checks a preset against the bundled files it has, and `ec4` checks that a path with the `examples:` prefix is refused |
| AC-17 | Pass | `ac17 scope check finds an unscoped selector` (`build.test.js`, Frequent; the test product's build fails on one), `ac17 host styles unchanged` (`embed.spec.js`, all projects) |
| AC-18 | Pass | `ac18 renders on a server without fetch or worker` (`prerender.spec.js`) |
| EC-1 | Pass | `ec1 plain http every copy writes` (Chromium, the test server under a host name mapped to 127.0.0.1, so not a secure context: two tabs both keep their files, no notice; no service worker on the app page) (`storage.spec.js`, `root` only) |
| EC-2 | Pass | `ec2 unreadable store is not written over` (a worker entry that makes IndexedDB's read fail; what was kept is still there afterwards, what was written in that run is not) (`storage.spec.js`, all projects) |
| EC-3 | Pass | `ec3 no store works in memory and says so` (a worker entry in which IndexedDB cannot be opened) (`storage.spec.js`, all projects). A write refused later (a full store mid-session) switches the mode to `unavailable` and says so (`storage.js`); that path is not tested, no browser here lets a test fill the store (design §8, not verified) |
| EC-4 | Pass | `ec4 unknown model and preset give a notice` (`embed.spec.js`, all projects) |
| EC-5 | Pass | `ec5 changed props are not read again` (`embed.spec.js`, all projects) |

### Decided in the step (spec → today's behaviour → simpler structure)

- **The factory's result.** A product's factory returns
  `calcu::web::WebSession {keep, session}` instead of a bare
  `std::unique_ptr<AppSession>` (design §3.6): the session holds its
  parts by pointer, and the product's parts and settings must outlive it.
  `WebStart` also carries the product's name, the storage sentence and the
  shell's own part, which the factory lists first.
- **Call ids.** The session replies to a command inside `send()`, before
  its call id is known to the caller. The page numbers its own calls; the
  glue holds the session's messages for the length of one `send()` or
  `ask()` and gives each reply the page's number (also an operation's
  later reply). Nothing is reordered.
- **Wire details beside design §9 N1.** An ask carries the host service's
  name in `name` ("file-picker", "keep-file", …); `sound-start` carries
  its rate in `value`. The clipboard-text service copies `{text}` and,
  without a text, answers with the clipboard's `{text}` (N4 names no
  shape for it).
- **The storage toggle (design §3.3, §8).** Instead of switching
  `autoPersist` after the read or remounting, `storage.js` replaces IDBFS's
  persist queue with its own: shut until the read succeeded in mode
  `kept`, and it notices a refused write. One mount, one read.
- **A refused write mid-session** is sent by the worker as the
  `web.storage` state and a notice in the session's shapes (like
  `web.start` = `failed`, design N2), since the session does not see it.
- **The sentences** (`src/sentences.js`): AC-8 follows Calc-U-1600's
  today ("… is open in another tab or window: what you save here is not
  kept."); EC-2, EC-3 and the failed start are new wording.
- **Vendored React and Babel.** The test product and `build.test.js` use
  the copies in `products/calc-u-1600/frontends/web/vendor/` (identical
  in both products); the library has none of its own until M-31/M-34.
- **Playwright.** `library/web/package-lock.json` pins 1.65.0 as
  Calc-U-1600 does; the Calc-U-59 job installs the library's browsers
  besides its own (1.64.1).

### Open for the user

- Design D-2 asks for ledger rows for the update rule (a new release from
  the next start, not over a running page). M-29 names no ledger row and
  no product uses the shell yet; the rows belong with M-31 (Calc-U-1600)
  and M-34 (Calc-U-59).

## Calc-U-1600 (M-31, 2026-11-15)

The Calc-U-1600 Web frontend runs on the library's shell: the product's
`calc-engine-worker.js` is the configuration of the library's worker,
`frontends/web/bindings.cpp` is the factory of the app session
(`CALCU_WEB_PRODUCT(CalcU1600Host, …)`); `WebController`, `WebAudioSink`,
the product's app page, service worker, worker entry and audio worklet are
gone (the library's are used), and `build.js` uses
`library/web/build/dist.js`. What the page draws comes as product topics,
commands and requests of `app/SharpWebPart` (`c1600.ui`, `c1600.lcd`,
`c1600.lamps`, `c1600.paper`, `c1600.printer`; W16-1 `c1600.faceplate`,
`c1600.lcd-constants`).

| AC | Result | Evidence |
|---|---|---|
| AC-24 | Pass | `frontends/web/tests/session.spec.js` `ac24` (`?model=PC-1500`); `index.html` takes the model's name |
| AC-25 | Pass | `session.spec.js` `ac25`: `/persist/settings/Calc-U-1600/Calc-U-1600.ini` and `/persist/Calc-U-1600/Kept.card.yaml` in IndexedDB `/persist` (0.8.x's store and place), listed after a reload; `play.spec.js` keeps the no-`localStorage` check |
| AC-26 | Pass | `session.spec.js` `ac26`: sound messages at regular speed, none at full speed, none while a reset runs |
| AC-27 | Pass | `session.spec.js` `ac27`: page → worker only `listen`, `command`, `request`, `answer`; worker → page only LIB-3 kinds |
| AC-28 | Pass | `play.spec.js`, `offline.spec.js`, `prerender.spec.js`, `rom-missing.spec.js` kept: 52 passed, 2 skipped (Chromium root and sub-path, offline in Chromium and WebKit) |

Decided in the step:
- **`src/engine.js` stays, as a translator** over `createShell()` (the
  components' `engine.call("pickModel", 2)` → `c1600.pick-model`, the
  topics → the slices they read), instead of rewriting every component on
  `useTopic()` before LIB-14 1600 (M-49) rewrites them anyway. It decides
  nothing.
- **The page is ready once the start operation is over** (keys during an
  operation are dropped, ledger 1600-9); `play.spec.js`'s `pickModel` waits
  for the end of the operation, not only for the busy indicator.
- **`calc.css` is scoped** (every selector under `.calcu-client`, AC-17);
  `build.js` fails on an unscoped one.
- AC-16's bundle-file clause stays with M-46 (design D-4); the column is
  Done there.
