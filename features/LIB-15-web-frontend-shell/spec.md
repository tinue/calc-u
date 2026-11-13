# LIB-15: Web frontend shell

<!-- spec.md is the stable CONTRACT: WHAT, not HOW. It changes only
     deliberately, when the requirements change, and stays unchanged
     during implementation. Design lives in design.md, QA results in
     qa-report.md. No status or dates here: status lives only in
     features/INDEX.md. -->

## Dependencies
- Features: LIB-3 (the app session and its messages; "suspend" and
  "continue" as defined there), LIB-9 (the settings file and the save
  folder inside the file system this feature supplies).
- Contracts and decisions: `docs/contracts/frontend.md` (FE-7, I-1 to
  I-3, Q-2), `docs/contracts/settings.md` (ST-3, ST-4),
  `docs/contracts/roms.md` (R-3: the Web frontend always bundles its
  ROMs); ADR 0009, 0014; `docs/behaviour-changes.md` row 59-8.
- Facts: Calc-U-59 at `v2.3.0`, Calc-U-1600 at `b4ea8c8`.

What the three status columns mean for this feature:
- **Library:** the parts every Web frontend shares exist: hosting the
  app session in a Web Worker, the persistent file system with its
  one-writer rule, the installable offline app, and the embedding
  interface, shown with a product made for the tests.
- **Calc-U-59, Calc-U-1600:** the product's Web frontend is built on
  those parts, its own copies are gone, and what its release zip offers
  a host page follows the embedding interface.

Two words used here: the **persistent file system** is the files of one
product that the browser keeps for the user (settings, saved media); the
**offline cache** is the copy of the product's own files that lets the
installed app start without a network. They are separate, and an update
of the second never touches the first.

## User Stories
- As a visitor of calc-u.org, I want the emulator to run in the page
  without installing anything.
- As a user of a Web frontend, I want my settings and my saved media to
  be there when I come back.
- As a user, I want to install the Web frontend as an app and use it
  without a network.
- As the maintainer of calc-u-site, I want to embed every product the
  same way, from its release zip, so that the site has one embed code.
- As the developer, I want the worker, the storage and the offline code
  written once.

## Out of Scope
- The React components that draw the product: the control bar, menus,
  pickers and panels (owners: LIB-14 and the features whose panels they
  are); the faceplate, layout and product props (the product).
- What the settings file and the save folder hold (LIB-9, LIB-7;
  `ti58c.memory.yaml`: C59-3).
- The messages themselves (owner: LIB-3); this feature carries them
  between the page and the worker.
- Publishing on calc-u.org and the site's pin of a release zip (owner:
  LIB-19).
- The build of a Web frontend: the vendored libraries, the Emscripten
  version (the migration plan; "adding or removing build targets" is a
  non-goal, `Briefing.md` §2).
- Harness attach for a Web frontend (`harness.md` H-4: later), and the
  hooks the products' Playwright tests use to read a page
  (`data-ready`, the page's request call); they stay per product.
- The size limit for a dropped file (owner: LIB-13; Calc-U-1600 ignores
  a drop over 1 MB today, `Web/src/CalcU1600.jsx:16`).
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
- [ ] **AC-3** — Given the worker cannot start (a file it needs cannot
  be fetched, the worker reports an error, or its compiled code does not
  start), then the page itself shows that the product could not start
  and why; it does not go on showing that it is starting.
- [ ] **AC-4** — Given a Web frontend, then it states these host
  services to its session (LIB-3 AC-26): the clipboard for text and for
  an image, a file picker, handing a file to the user as a download,
  opening a URL, and an audio sink; it states no folder picker.

Persistent file system
- [ ] **AC-5** — Given a Web frontend, then it supplies the session with
  a persistent file system in the browser's IndexedDB; the settings file
  and the save folder are in it (LIB-9 AC-15) and are still there, with
  their content, after the page is closed and opened again
  (`settings.md` ST-3).
- [ ] **AC-6** — Given something is written, renamed or deleted in the
  persistent file system, then a reload of the page one second later
  finds the change.
- [ ] **AC-7** — Given two products served from one origin (calc-u.org
  serves both), then each has a persistent file system of its own, and
  neither sees the other's files.
- [ ] **AC-8** — Given the same product is open twice in one browser, in
  two tabs or twice on one page, and the page is served over HTTPS or
  from localhost, then only the first writes to the persistent file
  system; the second works on a copy in memory, tells the user that what
  is saved there is not kept, and stays so until it is loaded again
  (Calc-U-1600 today, `Web/calc-engine-worker.js:36, 116-124`,
  `Web/src/CalcU1600.jsx:393-397`).
- [ ] **AC-9** — Given a file the session hands to the user to keep (a
  trace, an exported medium, a screenshot), then the Web frontend offers
  it as a download.

Installable offline app
- [ ] **AC-10** — Given a Web frontend served over HTTPS or from
  localhost, then its release zip contains an app page that can be
  installed and then starts and runs every model with no network: the
  product's code, the ROMs, the bundled presets and media and the
  product's images are in the offline cache (both products today:
  `Web/app/sw.js`).
- [ ] **AC-11** — Given a newer release is served, then the installed
  app uses the files of the new release, all of them together, from its
  next start on, and the offline cache of the older release is removed;
  the persistent file system is not touched.
- [ ] **AC-12** — Given the app page, then it offers to be installed
  where the browser allows it, and shows a hint on iOS, as today.

Embedding interface
- [ ] **AC-13** — Given a product's release zip for the web, then its
  `dist/` folder works from any path of any site, at the root and below
  it, served over HTTP or HTTPS from the page's own origin
  (`frontend.md` FE-7; both products test this today with a "root" and a
  "subpath" run).
- [ ] **AC-14** — Given `dist/`, then it holds a `MANIFEST.json` with
  the same fields in every product: `name`, `version`, `commit`, and
  `files`, the sha256 of every file by its path (both products today,
  `Web/build.js`). No other file of `dist/` is part of the embedding
  interface; a list of bundled files that a worker reads is the
  design's, one way for both products (OQ-6).
- [ ] **AC-15** — Given a host page that has loaded React 18 as a
  global, then it embeds a product by loading `calc.css` and, as a
  classic script, `calc.js`, which defines one global component whose
  name is the product's own, and by mounting that component (both
  products today: Calc-U-59 `reference/WebClient.md:133-176`,
  Calc-U-1600 `docs/developer/Web-Client.md:54-90`).
- [ ] **AC-16** — Given the component of any product, then it takes
  these props with the same name and meaning: `width` and `height` (the
  size of the component in CSS pixels), `keyboard` (whether host keys
  reach the machine), `model` (the model to start with), `preset` (a
  bundled preset to start with), and `workerUrl` (where the worker's
  script is, for a page that serves it from another path); a product
  adds props of its own beside them. `model` takes the model's name as
  the glossary writes it (`TI-58C`, `PC-1600`); `preset`, and the play
  page's `?preset=` alike, takes a bundled preset's path inside the
  bundle (`sum.ti59`, `plotter/globus.pc1600`), its bundle path without
  the prefix `examples:`, and a path the bundle file
  does not list (`docs/formats/bundle.md`) is refused with a notice and
  the component starts as without the prop; `keyboard` is on by
  default; without `width` and `height` the component fills its parent,
  and a product whose faceplate has a fixed shape keeps that shape
  inside it (`frontend.md` FE-7; OQ-1, OQ-3).
- [ ] **AC-17** — Given the styles of a Web frontend, then they apply
  only inside the element with the class `calcu-client`, and a host
  page's own styles are not changed by them.
- [ ] **AC-18** — Given the component is rendered on a server, without a
  browser, then it renders without fetching anything and without
  starting a worker (both products test this today: `prerender.spec.js`).

### Calc-U-59
- [ ] **AC-19** — Given the Web frontend of Calc-U-59, then its
  component is `PlayCalculator`, with the props of AC-16 and its own:
  `layout` (`portrait`, `wide`, `auto`) and `settings` (whether the
  settings control is shown) (`Web/src/PlayCalculator.jsx:179-189`,
  `reference/WebClient.md:146-156`). The prop `module` of today goes; a
  host page that wants another module names a preset (OQ-2).
- [ ] **AC-20** — Given the Web frontend of Calc-U-59, then its
  settings, its magnetic cards and `ti58c.memory.yaml` are in the persistent
  file system and survive a page load. Today the settings and
  `ti58c.mem` are `localStorage` keys with the prefix `calcu59/`, and
  cards are held in memory for the page session
  (`Web/src/engine.js:20-40`, `Web/bindings.cpp:54, 225-257`; behaviour
  change 59-8).
- [ ] **AC-21** — Given the Web frontend of Calc-U-59 open twice, then
  AC-8 holds; today it has no such rule (OQ-5).
- [ ] **AC-22** — Given what the page and the worker of Calc-U-59
  exchange today (`Web/calc-engine-worker.js:9-33`,
  `Web/bindings.cpp:404-438, 988-1046`), then after this feature it is
  the messages of LIB-3 and the product's own topics and commands; no
  message type of today's own protocol remains.
- [ ] **AC-23** — Given what the Playwright tests of Calc-U-59 check
  today about the shell, then it still holds: both layouts load without
  an error in the console; the installed app starts with the server gone
  and runs every model and loads a bundled example, in Chromium and in
  WebKit; the component renders
  on a server (`Web/tests/play-calculator.spec.js`,
  `Web/tests/offline.spec.js`, `Web/tests/prerender.spec.js`;
  `Web/tests/play-calculator.spec.js:471-491` for the example).

### Calc-U-1600
- [ ] **AC-24** — Given the Web frontend of Calc-U-1600, then its
  component is `CalcU1600`, with the props of AC-16
  (`Web/src/CalcU1600.jsx:60`, `docs/developer/Web-Client.md:71-77`).
- [ ] **AC-25** — Given the Web frontend of Calc-U-1600, then its
  settings are in `Calc-U-1600.ini` in the persistent file system (LIB-9
  AC-32); today they are `localStorage` keys with the prefix
  `calcu1600/` (`Web/src/engine.js:20-39`). The media a user saved with
  the Web frontend of 0.8.x are still there: the store and the place in
  it stay the same; the settings start afresh (OQ-4).
- [ ] **AC-26** — Given the Web frontend of Calc-U-1600 and a Sharp
  machine at regular speed, then its buzzer is heard, and nothing is
  heard at full speed or while an operation runs (LIB-3 AC-37; today the
  worker posts the samples to the page, `Web/bindings.cpp:144-174`).
- [ ] **AC-27** — Given what the page and the worker of Calc-U-1600
  exchange today (`Web/calc-engine-worker.js:17-23, 76-94`,
  `Web/bindings.cpp:709-960`), then after this feature it is the
  messages of LIB-3 and the product's own topics and commands.
- [ ] **AC-28** — Given what the Playwright tests of Calc-U-1600 check
  today about the shell, then it still holds: each model starts; a name
  given with Name & Save survives a reload; the second tab shows its
  notice; the installed app starts with the server gone; the component
  renders on a server (`Web/tests/play.spec.js:44, 236, 329, 337`,
  `Web/tests/offline.spec.js:40`, `Web/tests/prerender.spec.js:32`).

## Edge Cases
- **EC-1** — Given a page served over plain HTTP, or a browser without a
  way to tell that the product is open twice, then every copy treats
  itself as the only one, as today (Calc-U-1600
  `Web/calc-engine-worker.js:117`), and the app page cannot be
  installed.
- **EC-2** — Given the persistent file system cannot be read at the
  start, then the Web frontend works in memory, tells the user that
  nothing is kept, and does not write over what it could not read.
- **EC-3** — Given the browser's private mode or a full store, where
  nothing can be kept, then the Web frontend works in memory and tells
  the user as in AC-8.
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
- The persistent file system holds only files the library and the
  product's app layer write through LIB-9's paths; `localStorage` is not
  used for settings, media or `ti58c.memory.yaml`.
- The sentences a Web frontend shows about storage (AC-8, EC-2, EC-3)
  come from the library, the same for every product.

## Open Questions
- [x] **OQ-1** — *Decided as recommended (user, 2026-11-10, batch 3 gate).* The value of the `model` prop (AC-16). Calc-U-1600
  takes a number, 0, 1 or 2, today. *Recommendation:* the model's name
  as the glossary writes it (`TI-58C`, `PC-1600`), as in presets and
  settings; calc-u-site changes its embed code once.
- [x] **OQ-2** — *Decided as recommended (user, 2026-11-10, batch 3 gate).* The prop `module` of `PlayCalculator`. Today a host
  page can name the Solid State ROM module to start with. Settings no
  longer hold hardware (behaviour change 59-9), and a machine's hardware
  comes from a preset or from the built-in default machine.
  *Recommendation:* drop the prop; a host page that wants another module
  names a preset. Check with calc-u-site whether any page uses it.
- [x] **OQ-3** — *Decided as recommended (user, 2026-11-10, batch 3 gate).* Defaults that differ today. `keyboard` is off in
  Calc-U-59 and on in Calc-U-1600. Without `height`, Calc-U-59 takes it
  from the width, and Calc-U-1600 takes its parent's size.
  *Recommendation:* `keyboard` on in both; without `width` and `height`
  the component fills its parent, and a product whose faceplate has a
  fixed shape keeps that shape inside it.
- [x] **OQ-4** — *Decided as recommended (user, 2026-11-10, batch 3 gate).* The media a user saved in the Calc-U-1600 Web frontend
  of 0.8.x (AC-25). Calc-U-1600 needs no backwards compatibility
  (`Briefing.md` §1.2), and module definitions and floppy disks stay
  readable while their formats do not change (behaviour change 1600-1).
  *Recommendation:* keep the same store and the same place in it, so
  that those media are still there; the settings start afresh.
- [x] **OQ-5** — *Decided as recommended (user, 2026-11-10, batch 3 gate).* The one-writer rule for Calc-U-59 (AC-21) is new there.
  It follows from keeping the cards in the browser. *Recommendation:*
  add it to row 59-8 of `docs/behaviour-changes.md`.
- [x] **OQ-6** — *Decided as recommended (user, 2026-11-10, batch 3 gate).* `files.json`. Calc-U-1600's `dist/` lists its bundled
  ROMs and presets in a second file that its worker reads; Calc-U-59
  fetches by name. *Recommendation:* neither is part of the embedding
  interface, because no host page reads it; the design picks one way
  for both.

## Decision Log

| Decision | Rationale | Date |
|----------|-----------|------|
| Sessions are single-threaded and reached by messages; the Web frontend hosts its session in a Web Worker | ADR 0009; `frontend.md` I-1, I-2 | 2026-11-09 |
| One written embedding interface: a location-independent `dist/`, `MANIFEST.json` with the same fields, styles under `.calcu-client`, the same prop names for the same things; component names stay per product | `frontend.md` FE-7 (user) | 2026-11-10 |
| The web save folder is persistent IndexedDB in both products; on the web the settings file is a file in it | `settings.md` ST-3, ST-4 | 2026-11-10 |
| The Web frontend always bundles its ROMs | `roms.md` R-3 | 2026-11-10 |
| Suspending in the background stays per frontend as today: the Web frontends suspend when hidden | `frontend.md`, rules from decisions already taken | 2026-11-10 |
| Proposed in this spec and accepted (user, 2026-11-11): the one-writer rule of Calc-U-1600 becomes the rule for every Web frontend, per product, with no later takeover (AC-7, AC-8); a change is kept within one second (AC-6); a Web frontend that cannot start says so itself (AC-3); an unreadable persistent file system is not written over (EC-2); `workerUrl` is part of the embedding interface (AC-16); an unknown `model` or `preset` gives a notice (EC-4) | Persistent storage opened twice needs a rule; today a failed start of Calc-U-59 leaves "Starting the emulator…" for ever; both products have `workerUrl` today although FE-7 does not list it; no source gives EC-2 and EC-4 | 2026-11-10 |
| `model` takes the model's name; `keyboard` is on by default in both products; without `width` and `height` the component fills its parent; the prop `module` of `PlayCalculator` goes; media saved with the Calc-U-1600 Web frontend of 0.8.x stay; the one-writer rule is added to ledger row 59-8 | user, 2026-11-10, batch 3 gate | 2026-11-10 |
| The row is named "Web frontend shell" | user, 2026-11-11; the glossary's term is Web frontend | 2026-11-11 |
| The installed app without the server also loads a bundled example (AC-23) | user, 2026-11-13 (B-23 of `docs/walkthroughs/presets.md` §5.2); today's Playwright test checks it | 2026-11-13 |
| `preset` and `?preset=` take a bundled preset's path inside the bundle; an unlisted path is refused with a notice (AC-16) | user, 2026-11-13 (B-27 (c) of `docs/walkthroughs/presets.md` §5.2); a name tag can repeat across models and change, a path stays stable for links | 2026-11-13 |
| Aligned with `docs/formats/` after the Q&A round: `ti58c.memory.yaml` | user rulings of 2026-11-13 (`docs/walkthroughs/presets.md` §5.2, §5.4); `docs/formats/` | 2026-11-13 |
