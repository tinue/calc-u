# LIB-15: Web client shell

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: LIB-3 (the app session and its messages), LIB-9 (the
  settings file and the save folder inside the file system this feature
  supplies).
- Contracts and decisions: `docs/contracts/frontend.md` (FE-7, I-1 to
  I-3), `docs/contracts/settings.md` (ST-3, ST-4), `docs/contracts/roms.md`
  (R-3: the Web frontend always bundles its ROMs); ADR 0009, 0014;
  `docs/behaviour-changes.md` row 59-8.
- Facts: Calc-U-59 at `v2.3.0`, Calc-U-1600 at `b4ea8c8`.

What the three status columns mean for this feature:
- **Library:** the parts every Web frontend shares exist: hosting the
  app session in a Web Worker, the persistent file system with its
  one-writer rule, the installable offline app, and the embedding
  interface, shown with a product made for the tests.
- **Calc-U-59, Calc-U-1600:** the product's Web frontend is built on
  those parts, its own copies are gone, and what its release zip offers
  a host page follows the embedding interface.

## User Stories
- As a visitor of calc-u.org, I want the emulator to run in the page
  without installing anything.
- As a user of a web client, I want my settings and my saved media to be
  there when I come back.
- As a user, I want to install the web client and use it without a
  network.
- As the maintainer of calc-u-site, I want to embed every product the
  same way, from its release zip, so that the site has one embed code.
- As the developer, I want the worker, the storage and the offline code
  written once.

## Out of Scope
- The React components that draw the product: the control bar, menus,
  pickers and panels (owners: LIB-14 and the features whose panels they
  are); the faceplate, layout and product props (the product).
- What the settings file and the save folder hold (LIB-9, LIB-7).
- The messages themselves (owner: LIB-3); this feature carries them
  between the page and the worker.
- Publishing on calc-u.org and the site's pin of a release zip (owner:
  LIB-19).
- The build of a Web frontend: the vendored libraries, the Emscripten
  version (the migration plan; "adding or removing build targets" is a
  non-goal, `Briefing.md` §2).
- Harness attach for a Web frontend (`harness.md` H-4: later).
- What a Web frontend does not have today and does not gain: the DAP
  debugger, serial ports, the host drive, traces in Calc-U-1600, folder
  settings (`docs/inventory/divergence-matrix.md`, M64).

## Acceptance Criteria

### Library

Hosting
- [ ] **AC-1** — Given a Web frontend, then its app session runs in a
  Web Worker; the page and the worker exchange only the messages of
  LIB-3, and the worker calls the session about 60 times a second
  (`frontend.md` I-1, I-2; both products today).
- [ ] **AC-2** — Given the page is hidden, then it sends suspend and the
  worker stops calling the session; given it is shown again, then it
  sends continue and the calls go on (LIB-3 AC-24; both products today:
  Calc-U-59 `Web/src/engine.js:148`, Calc-U-1600
  `Web/src/engine.js:180-181`).
- [ ] **AC-3** — Given the engine cannot start (a file it needs cannot
  be fetched), then the page shows that it could not start and why, and
  does not wait for ever.

Persistent file system
- [ ] **AC-4** — Given a Web frontend, then it supplies the session with
  a file system that is kept in the browser's IndexedDB; the settings
  file and the save folder are in it (LIB-9 AC-15) and are still there,
  with their content, after the page is closed and opened again
  (`settings.md` ST-3).
- [ ] **AC-5** — Given something is written, renamed or deleted in that
  file system, then it is kept without the user doing anything.
- [ ] **AC-6** — Given the same Web frontend is open in two tabs, then
  only the first writes to the kept file system; the second works on a
  copy in memory and tells the user that what is saved there is not kept
  (Calc-U-1600 today, `Web/calc-engine-worker.js:36, 116-124`,
  `Web/src/CalcU1600.jsx:393-397`).
- [ ] **AC-7** — Given a file the session hands to the user to keep (a
  trace, an exported medium, a screenshot), then the Web frontend offers
  it as a download (LIB-3 AC-26).

Installable offline app
- [ ] **AC-8** — Given a Web frontend, then its release zip contains an
  app page that can be installed and then starts and runs every model
  with no network: the engine, the ROMs, the bundled presets and media
  and the product's images are kept by the browser (both products
  today: `Web/app/sw.js`).
- [ ] **AC-9** — Given a newer release is served, then the installed app
  takes its files from the new release as a whole, and the kept files of
  the older one are removed.
- [ ] **AC-10** — Given the app page, then it offers to be installed
  where the browser allows it, and shows a hint on iOS, as today.

Embedding interface
- [ ] **AC-11** — Given a product's release zip for the web, then its
  `dist/` folder works from any path of any site, at the root and below
  it, served over HTTP or HTTPS from the page's own origin
  (`frontend.md` FE-7; both products test this today with a "root" and a
  "subpath" run).
- [ ] **AC-12** — Given `dist/`, then it holds a `MANIFEST.json` with
  the same fields in every product: `name`, `version`, `commit`, and
  `files`, the sha256 of every file by its path (both products today,
  `Web/build.js`).
- [ ] **AC-13** — Given a host page, then it embeds a product by loading
  `calc.js` and `calc.css` and mounting one component, whose name is the
  product's own.
- [ ] **AC-14** — Given the component of any product, then it takes
  these props with the same name and meaning: `width`, `height`,
  `keyboard` (whether host keys reach the machine), `model` (the model
  to start with, by its name), `preset` (a bundled preset to start
  with), `workerUrl`; a product adds props of its own beside them
  (`frontend.md` FE-7).
- [ ] **AC-15** — Given the styles of a Web frontend, then they apply
  only inside the element with the class `calcu-client`, and a host
  page's own styles are not changed by them.
- [ ] **AC-16** — Given the component is rendered on a server, without a
  browser, then it renders without fetching anything and without
  starting a worker (both products test this today: `prerender.spec.js`).

### Calc-U-59
- [ ] **AC-17** — Given the Web frontend of Calc-U-59, then its
  component is `PlayCalculator`, with the props of AC-14 and its own:
  `layout` (`portrait`, `wide`, `auto`) and `settings` (whether the
  settings control is shown) (`Web/src/PlayCalculator.jsx:179-189`,
  `reference/WebClient.md:146-156`; the prop `module`: OQ-2).
- [ ] **AC-18** — Given the Web frontend of Calc-U-59, then its settings
  and its magnetic cards are in the kept file system and survive a page
  load. Today the settings and `ti58c.mem` are `localStorage` keys with
  the prefix `calcu59/`, and cards are held in memory for the page
  session (`Web/src/engine.js:20-40`, `Web/bindings.cpp:54, 225-257`;
  behaviour change 59-8).
- [ ] **AC-19** — Given the Web frontend of Calc-U-59 open in two tabs,
  then AC-6 holds; today it has no such rule.
- [ ] **AC-20** — Given what the page and the worker of Calc-U-59
  exchange today (`Web/calc-engine-worker.js:9-33`,
  `Web/bindings.cpp:404-438, 988-1046`), then after this feature it is
  the messages of LIB-3 and the product's own topics and commands; no
  message type of today's own protocol remains.
- [ ] **AC-21** — Given what the Playwright tests of Calc-U-59 check
  today about the shell, then it still holds: both layouts load without
  an error in the console; the installed app starts with the server gone
  and runs every model, in Chromium and in WebKit; the component renders
  on a server (`Web/tests/play-calculator.spec.js`,
  `Web/tests/offline.spec.js`, `Web/tests/prerender.spec.js`).

### Calc-U-1600
- [ ] **AC-22** — Given the Web frontend of Calc-U-1600, then its
  component is `CalcU1600`, with the props of AC-14
  (`Web/src/CalcU1600.jsx:60`, `docs/developer/Web-Client.md:71-77`).
- [ ] **AC-23** — Given the Web frontend of Calc-U-1600, then its
  settings are in `Calc-U-1600.ini` in the kept file system (LIB-9
  AC-32); today they are `localStorage` keys with the prefix
  `calcu1600/` (`Web/src/engine.js:20-39`). Its saved media stay where
  they are kept today.
- [ ] **AC-24** — Given what the page and the worker of Calc-U-1600
  exchange today (`Web/calc-engine-worker.js:17-23, 76-94`,
  `Web/bindings.cpp:709-960`), then after this feature it is the
  messages of LIB-3 and the product's own topics and commands.
- [ ] **AC-25** — Given what the Playwright tests of Calc-U-1600 check
  today about the shell, then it still holds: each model starts; a name
  given with Name & Save survives a reload; the second tab shows its
  notice; the installed app starts with the server gone; the component
  renders on a server (`Web/tests/play.spec.js:44, 236, 329, 337`,
  `Web/tests/offline.spec.js:40`, `Web/tests/prerender.spec.js:32`).

## Edge Cases
- **EC-1** — Given a browser without a way to tell that another tab is
  open, then the tab treats itself as the only one, as today
  (Calc-U-1600 `Web/calc-engine-worker.js:117`).
- **EC-2** — Given the kept file system cannot be read at the start,
  then the Web frontend starts with an empty one, and a line of the log
  says so (Calc-U-1600 today, `Web/calc-engine-worker.js:149-150`).
- **EC-3** — Given the browser's private mode or a full store, where
  nothing can be kept, then the Web frontend works in memory and tells
  the user as in AC-6.
- **EC-4** — Given a `model` or a `preset` prop that names nothing the
  product has, then the component starts as without it and a notice says
  so.
- **EC-5** — Given the props change after the component was mounted,
  then `model` and `preset` are not read again (Calc-U-1600 today: read
  once at mount).

## Technical Requirements (optional)
- The web bindings only translate between the worker's messages and the
  session; no rule lives in them (`frontend.md` I-3; ADR 0014).
- A Web frontend never fetches a ROM from anywhere but its own `dist/`
  and has no ROM folder (`roms.md` R-3).
- The kept file system holds only files the library and the product's
  app layer write through LIB-9's paths; `localStorage` is not used for
  settings or media.

## Open Questions
- [ ] **OQ-1** — The value of the `model` prop (AC-14). Calc-U-1600
  takes a number, 0, 1 or 2, today. *Recommendation:* the model's name
  as the glossary writes it (`TI-58C`, `PC-1600`), as in presets and
  settings; calc-u-site changes its embed code once.
- [ ] **OQ-2** — The prop `module` of `PlayCalculator`. Today a host
  page can name the Solid State ROM module to start with. Settings no
  longer hold hardware (behaviour change 59-9), and a machine's hardware
  comes from a preset or from the built-in default machine.
  *Recommendation:* drop the prop; a host page that wants another module
  names a preset. Check with calc-u-site whether any page uses it.
- [ ] **OQ-3** — The default of `keyboard`: off in Calc-U-59, on in
  Calc-U-1600 today. *Recommendation:* the same in both, on; a page with
  several embedded machines switches it off.
- [ ] **OQ-4** — `files.json`. Calc-U-1600's `dist/` lists its bundled
  ROMs and presets in a second file that its worker reads; Calc-U-59
  fetches by name. Is one of the two ways part of the embedding
  interface? *Recommendation:* no; it is inside `dist/` and no host page
  reads it. The design picks one way for both.
- [ ] **OQ-5** — The one-writer rule for Calc-U-59 (AC-19) is new there.
  It follows from keeping the cards in the browser. Does it need a row
  in `docs/behaviour-changes.md` next to 59-8? *Recommendation:* add it
  to row 59-8.
- [ ] **OQ-6** — The size limit for a dropped file: Calc-U-1600 ignores
  a drop over 1 MB (`Web/src/CalcU1600.jsx:16`); Calc-U-59 has no limit.
  *Recommendation:* belongs to LIB-13 (opening files); noted here so it
  is not lost.

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| Sessions are single-threaded and reached by messages; the Web frontend hosts its session in a Web Worker | ADR 0009; `frontend.md` I-1, I-2 | 2026-11-09 |
| One written embedding interface: a location-independent `dist/`, `MANIFEST.json` with the same fields, styles under `.calcu-client`, the same prop names for the same things; component names stay per product | `frontend.md` FE-7 (user) | 2026-11-10 |
| The web save folder is persistent IndexedDB in both products; on the web the settings file is a file in it | `settings.md` ST-3, ST-4 | 2026-11-10 |
| The Web frontend always bundles its ROMs | `roms.md` R-3 | 2026-11-10 |
| Suspending in the background stays per frontend as today: the Web frontends suspend when hidden | `frontend.md`, rules from decisions already taken | 2026-11-10 |
| Proposed in this spec, for the gate: the one-writer rule of Calc-U-1600 becomes the rule for every Web frontend (AC-6); a Web frontend that cannot start says so (AC-3) | Persistent storage in two tabs needs a rule; today a failed start of Calc-U-59 leaves "Starting the emulator…" for ever | 2026-11-10 |
