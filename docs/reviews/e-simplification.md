# Part E: the last structural simplification before the releases

Status: done (2026-11-17). The survey (§1), the three large changes (§2)
and the plan (§3) were written and committed (`a080d5c`) before any code
edit; §4 onwards report the result. Questions for the user: §10 (all
answered by the user on 2026-11-17; the answers are under each question).

Baseline: `main` at `5db83d7`; build `build/tests` (RelWithDebInfo, Qt
OFF); Frequent 50/50, 45.2 s wall (`-j8`): `CalcU1600SessionTests` 45.2 s,
`CalcU59Tests` 29.7 s then `CalcU59TraceReader` 15.5 s, `CoreTests` 25.9 s,
the library 6.0 s.

`59/` is `products/calc-u-59/`, `1600/` is `products/calc-u-1600/`.

## 1. Survey (read-only), ranked by payoff

Payoff = lines and concepts removed, then what a third (SC61860) product
no longer has to write. Every item was checked in the code at `5db83d7`.
The full survey notes (four read-only passes: library against its two
consumers; code written twice; leftovers, tests and build; earlier
rejections, SC61860 and public surfaces) are summarised here.

### S-1. A product's session is put together seven times, and its inputs arrive three ways

- **Assemblies.** `1600/app/SharpSession.cpp:9-25` (`makeSharpSession`),
  `59/app/TIAppPart.cpp:348-378` (`makeAppSession`), both
  `frontends/web/bindings.cpp` `WebParts` (1600:30-68, 59:48-106, word for
  word but for the types), both Qt controllers
  (`1600/frontends/qt6/app/MachineController.cpp:24-77`,
  `59/frontends/qt6/app/EmulatorController.cpp:47-108`) and the Swift
  bridge (`59/frontends/swift/Bridge/TIAppSession.mm:425-492`). Each opens
  the settings file from `settingsDecl()`, makes `Paths` and a
  `SettingsPart`, orders the parts, logs "Settings: problem", starts and
  closes.
- **The order of the parts is written by five hosts, two ways.**
  `LibraryParts::sessionParts` (`library/parts/LibraryParts.cpp:63-75`)
  leaves the control part out; Calc-U-1600 puts host parts, control,
  library parts; Calc-U-59 typing, control, host parts, the rest
  (`TIAppPart.cpp:356-363`); the harness its own (`library/harness/Run.cpp:207-208`).
  Calc-U-59's file kinds use `TIDebugPart`, which it places after OpenPart.
- **Folders, trace limit, screenshot folder.** Calc-U-1600 passes `Paths`
  and uses `debugger::traceFolderOf`; Calc-U-59 hand-builds
  `TraceTarget{dir, maxBytes}` in four hosts (Qt, Web, Swift, harness: MB ×
  10⁶, divided back at `TIAppPart.cpp:62`), writes its own trace-folder and
  availability lambdas (`TIAppPart.cpp:47-81`) and builds a second `Paths`
  with `FrontendKind::Swift` on every frontend (`:124-130`). The harness uses
  `FrontendKind::Swift` to mean "the folders as given" (`Run.cpp:141-145`,
  `1600/app/HarnessProduct.cpp:189`) and then patches the debugger
  (`setTraceFolder`/`setTraceLimit`, `Run.cpp:193-205`). The screenshot
  folder is one lambda written four times.
- **`roms-missing`.** The library reads the topic (`shell/Inputs.hpp`,
  `useShell.jsx`) but each product publishes it (`SharpTopics.hpp:126-132`
  `RomsMissingPayload`, `TIAppPart.cpp:203-209`); six callers turn a
  `LookupResult` into `roms` plus a text.
- **Optional parts nobody leaves out.** Typing, presets, paper, debugger and
  media are optional in `LibraryParts` (`LibraryParts.cpp:9-46`); both
  products give all five; only the library test product's "outside a run"
  state omits them. Hanging on it: the harness's "This build reads no
  presets of format 2 yet." (a wrong reply for every build, also through
  `calcu attach`), `harness::SessionParts` (five call sites), Calc-U-59
  null-checking where Calc-U-1600 dereferences, non-const accessors that
  both app parts cast around. Redundant setup fields: `catalogue`
  (= `machines->catalogue()`), `printers` (= the IDs of `papers`),
  `SettingsPartSetup::frontend` (= `paths->hostDirs().frontend`).

### S-2. The harness's product interface: dead members and a skeleton written twice

`HarnessProduct::catalogue()`, `builder()`, `commandWord()` have no
caller; `machines()` one test; `lookUpRoms()` repeats what `prepare()`
did; `RunSetup::partsSetup` serves only the library's test product.
Both products keep a second set of machines for them
(`59/app/HarnessProduct.cpp:70,136`, `1600/app/HarnessProduct.cpp:174-179`).
The two `HarnessProduct.cpp` write the same `prepare()` (end the run,
build the part on the run's settings, the folders, the screenshot lambda,
virtual time), `libraryParts()`, `sessionParts()`; `words`/`wordsOf`,
`fromSession`/`linesReply` are written in both command files and four
times in the library's harness.

### S-3. Calc-U-59 keeps 2.3.0's dirty tracking, a second page-form copy of its topics, and its own `set-control`

- `TIAppPart`'s `Change` bits, `setChangeObserver`, `TISnapshot` serials
  (`TIAppPart.hpp:86-127, 247-279`, `publish()` at `TIAppPart.cpp:310-338`)
  drive `TIWebPart`'s `c59.web.display`, `c59.web.cue-card`,
  `c59.web.run-state`, `c59.web.printer` (`TIWebPart.cpp:62-241`). C1 kept
  them because comparing by value would rescan the card list and trace
  folder on each call and because M-53…M-58 would replace the topics; both
  reasons are gone (the card reader is LIB-7's, the trace LIB-10's; the
  steps ran). `c59.web.printer` is `c59.printer`'s value; `c59.web.run-state`
  restates the library's `run-state` and `held-keys`.
- Calc-U-59 registers `set-control` a second time (`TIAppPart.cpp:218-225`)
  and `AppSession` silently keeps the last registration
  (`library/sessions/AppSession.cpp:136`): LIB-2's handler never runs in
  Calc-U-59. The reason is C59-6 AC-6 (TRACE keeps its position across
  detach and attach).

### S-4. The frontends' host glue, written per product

- **Qt6.** The settings bootstrap (`AppPaths.cpp`, ~50 lines each, the same
  but for the declaration and the macOS folder); opening a file (drag,
  drop with size and the way's limit, the macOS FileOpen event:
  `1600/.../FileOpenFilter.hpp` vs `59/.../main.cpp` `App::event`); the
  `asks:` round trip (`1600 TapeControl::saveTape`, `59 MainWindow::armFreshCard`
  with a literal `"asks: "` and `mid(6)`).
- **Web pages.** The shell product request and merge, Load Preset, `open`,
  the drop handler, a word-for-word `SavedMedia` wrapper, the slot picker,
  `calcImage`/`sizeCanvas`, `hitMapOf`, the `asks:` prefix four times in JS
  (once without the space, `shellModel.js:37`); Calc-U-59 writes its own
  `createC59Shell` because `useShell` takes no host services. D rejected
  this because Calc-U-59's shape was M-56/M-57's design; that has run.
- **Web build and tests.** The two `build.js` run the same steps (a third
  copy in the library's test product); both `run-tests.sh` embed an HTTP
  server in a heredoc beside `library/web/tools/serve.py`, which says it
  came from them; `serveDist`, the prerender sandbox and (four times in
  Calc-U-59) `gotoReady` repeat the library's test helpers.
- **Build scripts.** `build_and_run.sh` + `tools/build_app.sh`/`build_qt.sh`,
  `tests/rom_missing_build.sh`, the help CTest pairs.

### S-5. Names outside the glossary in code

- `namespace shared` (the old name of the app layer) in Calc-U-1600:
  about 610 uses in 146 files, and the `shared_*_tests.cpp` file names of
  both products (glossary-mapping "Shared" → App layer).
- The Swift frontend's concept names `TIFreezeOwner`, `freezeOwner`,
  `unfreeze()`, `loadStateFile`, `.stateFile` (the labels FREEZE and RESUME
  stay; the concepts are Pause, run level, preset).
- Calc-U-1600's `CardBase`, `ExpansionCard`, `SoftwareDefinedCard`,
  `BatteryCardInstance`, `MemoryCardDefinition` (about 80 files): neither
  renamed nor declared kept; the mapping names no target names, and the
  public suffix `.card.yaml` is tied to them (§ questions).

### S-6. Stale agent knowledge and developer documents

`docs/glossary-mapping.md` still reads as if the migration were to come,
names `Shared/`, has Calc-U-59's M-55 cells in Calc-U-1600's column and
stale 1600 names (`setTurbo`, `startup/modelPreference`). Memory files and
one research skill give recipes for removed tools (`pc1500_cli`,
`tools/headless`); developer documents describe gone classes
(`TIDebugPanelAdapter`, `PresetFile::traceHook`, `HostKeyMap`,
`bundled.yaml`); the Calc-U-1600 `CLAUDE.md` says "Core, Shared,
frontends" and "adapters live in core/Machine/"; the skill
`calc-u-59-shared` and two "Shared architecture" documents; 73 `Shared/`
path comments in Calc-U-59; 26 "until M-n" comments; Calc-U-1600's 24
historic plans (331 old-term hits) without a banner; `TODO.md` paths of
gone files; the Process phase row.

### S-7. Small library helpers written many times

"The first capability of kind K": `sharp::firstOf` (34 uses), `cu59::kindOf`
(52, with a `const_cast`), ten more loops in the library;
`Machine::capability()` is non-const, so six `const_cast`s. Splitting
argument words (six copies), session reply → harness reply (three), UTF-8
text ↔ `std::filesystem::path` (about 20 private copies) and "read a whole
file" loops (14), `lowerAscii` (four in the library after D, three in the
products), TI's `FileKind::takes` three times with a private `endsWith`.

### S-8. Dead hooks, test-only wrappers, a second file-access interface

- No user: `typing::PauseProbe` (only the harness test product sets it),
  `qt6::SessionHost::setServiceHandler`/`answer`, `PlaceKeyRule::byName`,
  the mutable `OpenPart::setup()` (for the upload folder, which belongs in
  the setup), `PaperPart`'s nullable control, `TIAppPart::error`,
  `webPayload` (= `valuePayload`).
- Product settings wrappers only tests call (1600 `Settings.hpp`: eleven;
  59 `Settings.hpp`: six; 59 Qt `AppSettings.hpp`: eight of nine).
- Calc-U-59's `FileIO`/`plainFileIO` and the adapter `FileIOAccess` beside
  LIB-7's `media::FileAccess` (same contract).
- Library CTest entries off `calcu_add_test` (the C1 reason, no working
  directory, is gone: the function takes one now); the library test
  products' own `textArg`/`intArg` beside `sessions/Args.hpp`.

### S-9. Release inputs and their notes name the old repositories

`calc-u-1600.yml:23,507` and `1600/release.yaml:49` point at a missing
`.github/macos-signing.md` (it is `1600/.github/macos-signing.md`);
`build.yml` (the old workflow name) in seven places; `Web/` paths in both
`release.yaml` comments. (`1600/release.yaml`'s examples zip goes in M-61,
ledger 1600-21: Part F, not touched here.)

### S-10. The two workflows

Calc-U-1600 has five hand-written platform jobs (764 lines) where
Calc-U-59 uses matrices; the `library` job is the same job in both
workflows. ReleaseIt reads only the workflow file name and the artifact
names (`workflow_dispatch: {}`, no inputs).

### S-11. Tests and build

- Frequent: Calc-U-1600's `CalcU1600SessionTests` alone is 45 s of the 60 s
  (wall); Calc-U-59's two programs run one after the other (29.7 s + 15.5 s).
- One 318 KB fixture twice (`1600/frontends/web/tests/fixtures/pc1500_tape.wav`
  = `1600/tests/fixtures/tape/pc1500_tape.wav`); `find_package(Python3)`
  four times; `POSITION_INDEPENDENT_CODE` for `ti59headless` (gone in M-59);
  `59/work/calendar/` with two 2.3.0-format presets the reader refuses;
  three faceplate PNGs byte-identical in `1600/source-material/`.

### S-12. Seen and fine (kept)

`ModelRules` hooks (each has a user); `ShellProduct` fields (each used);
`KindDecl`, `HostLinkRules`, `ProductMachines::carried`/`addRoms` (fit
tables name SC61860 users); `CellGrouping`, `AfterChange::Nothing`, the
paper text layer, per-target space widths (SC61860 fit tables);
`DebuggerSetup::runControlSteps` (library tests mix both kinds);
`PanelArea::addPanel` (LIB-14 AC-23/AC-42); Web attach generality (harness
fit table "web later"); Calc-U-59's `SessionThread` (one threaded host);
the product data files (`*ShellProduct`, `*Printers`, `*ProductMachines`,
`*DebuggerSetup`, `*PresetRules`, `*PresetKeys`, `*Typing`,
`*TraceRecorder`, `PacingProfiles`, `*FileKinds`); the display painters and
chrome colours (each product's look).

## 2. The three largest changes if effort were free

1. **One assembly of a product's session, from the library (S-1, S-2;
   both products).** `LibraryPartsSetup` takes the host's `Paths` and the
   ROM `LookupResult`; the library derives the trace setup, the screenshot
   folder and the `roms-missing` topic (StartupPart publishes it); the five
   parts are always made and returned by reference; `LibraryParts` gives
   the full part list in the one order, control part included, with slots
   for the host's and the product's parts; a `FrontendKind` for "the
   folders as given" replaces the harness's use of `Swift`; one host-side
   builder for the settings file, `Paths`, `SettingsPart` and the log line
   that the seven assemblies call; the harness product interface loses its
   dead members and its products share one skeleton.
2. **Calc-U-59's topics by value, one form per fact, one owner of
   `set-control` (S-3).** Topics compared by value; no change bits, no
   observer, no serials; the page reads the library's `run-state` and
   `held-keys` and `c59.printer` itself; `TIWebPart` keeps only what is
   the page's own (the LED cell bytes, the cue card's draw list, the
   faceplate); `set-control` is LIB-2's alone, TRACE's kept position
   through a product hook, and a second registration of a command name is
   refused. The names and value shapes of `c59.led`, `c59.cue-card`,
   `c59.printer` stay (they are readable through `calcu state`: E-2 in §
   questions).
3. **The frontends' host glue in the component sets (S-4; both
   products).** `library/qt6`: the settings bootstrap, file opening (drag,
   drop, FileOpen event), the `asks:` round trip; `library/web`: the shell
   product hook, open/drop helpers, `SavedMedia`, the slot picker,
   `calcImage`/`sizeCanvas`, one `asks:` constant and helper, `useShell`
   with host services; one product build in `library/web/build/dist.js`,
   `serve.py` for both `run-tests.sh`, shared Playwright helpers. Each
   product's own visible rules (which files a drop takes, the macOS folder)
   are kept as they are today: nothing visible changes.

## 3. Plan: independent commits, one short branch each

| # | Branch | Does one thing | After |
|---|---|---|---|
| E-1 | `es-assembly` | Large change 1, library and app-layer half: setup inputs, parts always made, one order, `roms-missing`, the harness interface and skeleton (S-1, S-2) | |
| E-2 | `es-host-session` | Large change 1, host half: one builder for the seven assemblies (S-1) | E-1 |
| E-3 | `es-59-topics` | Large change 2 (S-3) | E-2 |
| E-4 | `es-qt-glue` | Large change 3, Qt6 (S-4) | E-2 |
| E-5 | `es-web-glue` | Large change 3, the Web pages (S-4) | E-3 |
| E-6 | `es-web-build` | Large change 3, Web build and test scaffolding, build scripts (S-4) | |
| E-7 | `es-helpers` | Library helpers (S-7) | E-5 |
| E-8 | `es-dead` | Dead hooks, test-only wrappers, `FileIO` (S-8) | E-7 |
| E-9 | `es-names` | Names outside the glossary in code (S-5, without the card classes) | E-8 |
| E-10 | `es-ci` | The workflows and the release inputs' notes (S-9, S-10) | |
| E-11 | `es-tests-build` | Tests and build (S-11) | E-9 |
| E-12 | `es-knowledge` | Agent knowledge and developer documents (S-6) | E-11 |

Each: build, Frequent (golden replays included), merge `--no-ff`; the
local Linux CI after the merge (web jobs where a Web frontend or
`library/web/` changed); the Swift app (macOS build, UnitTests) in the
main checkout after a merge that reached the Swift bridge. At the end: the
Longer group locally for both products, the Checkpoint screenshot
comparisons, `tools/help check` for both products, the glossary grep over
`docs/inventory/agent-knowledge.md`, then CI.

## 4. Changes made

Every step: build, Frequent green (golden replays included), merged
`--no-ff`; after the merge on `main`: Frequent, the Qt tree's tests, the
Swift app (macOS build, `CalcU59SwiftUnitTests`, `CalcU59UIRegression`)
where the bridge was reached, and the local Linux CI (base jobs, `web-59`,
`web-1600`). E-1 to E-9 and E-11, E-12 were done by one implementing agent
each in a worktree; the merges, checks and two fixes by me. "All" is
`git diff --shortstat` of the merge; "code" leaves out Markdown and
pictures.

| # | Change and reason | Before → after | All | Code |
|---|---|---|---|---|
| E-10 | The workflows and the release inputs' notes (S-9, S-10) | Calc-U-1600's two Linux and two Windows jobs are two matrices (job names, artifact names and contents unchanged, step lists compared job by job); the `library` job is one reusable workflow `library.yml` both call; `macos-signing.md` references point at `products/calc-u-1600/.github/`; `build.yml` and `Web/` in notes replaced. `calc-u-1600.yml` 764 → 611 lines, `calc-u-59.yml` 298 → 279, `library.yml` 32 | +133 −268 | +126 −263 |
| E-6 | Web build and test scaffolding, build scripts (S-4) | One `buildProduct()` in `library/web/build/dist.js` for both products and the library's test product (version through `release.yaml` as the help tool and ReleaseIt read it, same strings); both `run-tests.sh` on `library/web/tools/serve.py`; `serveDist` and the prerender sandbox once (`library/web/tests/dist-helpers.js`); one `gotoReady` in Calc-U-59; root `tools/run_qt.sh <59\|1600>` replaces four product scripts; `calcu_add_help_tests()`. Both `dist/` byte-identical before and after | +392 −563 | +325 −529 |
| E-1 | **Large change 1**, library half (S-1, S-2) | `LibraryPartsSetup` takes the host's `Paths` and the ROM `LookupResult`; the library derives the trace folder and limit, the screenshot folder, the media places, the catalogue and the printers; `FrontendKind::Given` replaces the harness's use of `Swift`; StartupPart publishes `roms-missing`; the five parts are always made (references; `harness::SessionParts` and "This build reads no presets of format 2 yet." gone); `sessionParts(SessionSlots)` gives the whole list in one order (Calc-U-59's TypingPart now after the control part, its `TIDebugPart` a file-kind part; no golden or test result changed); `HarnessProduct` loses `catalogue`, `builder`, `commandWord`, `machines`, `lookUpRoms`; `harness::words()`, `fromSessionReply()` once. Two per-product fields keep Calc-U-59's behaviour (§9, Q-1, Q-2) | +897 −860 | +710 −846 |
| E-2 | **Large change 1**, host half (S-1, S-4) | `library/shell/HostSession` (`HostSettings`, `makeHostSession`) used by all seven assemblies; `library/qt6/HostFolders` (platform folders, the one settings file, the macOS base as a parameter) and `ProductHost<Part>` (session, `SessionHost`, start, teardown, `ask`); `WebStart::placesDirs()`. Settings file locations, log lines and part order unchanged | +634 −341 | +541 −328 |
| E-7 | Library helpers once (S-7) | `machine::first<K>` and a const `capability()` (product `firstOf`/`kindOf` and six `const_cast`s gone); a new area `library/files` (`toPath`, `fromPath`, `readText`, `readBytes`) replaces about 20 private copies and 1600's `Utf8Path.hpp`; one `lowerAscii` (but `settings`, LIB-1 §2a); a default `FileKind::takes` | +388 −393 | +346 −387 |
| E-4 | **Large change 3**, Qt6 (S-4) | `library/qt6/FileOpening` (drag, would-take with size, read to the way's limit, the macOS FileOpen filter, start file), `AskedSend` (the `asks:` round trip, `media::kAsks`), `PacingProbe` (the measurement; each product keeps its printed lines and bound); 1600's `FileOpenFilter.hpp`, 59's `App` and two controller methods gone. 7 new library tests | +623 −291 | +532 −286 |
| E-3 | **Large change 2** (S-3) | Calc-U-59's topics compared by value: `Change` bits, observer, `TISnapshot` serials, `publish()` gone; `TIWebPart` derives its page forms from `c59.led`/`c59.cue-card`; `c59.web.printer` and `c59.web.run-state` gone (the page reads `c59.printer`, `run-state`, `busy`, `held-keys`); `set-control` is LIB-2's alone, TRACE's kept position through `ProductMachines::attaching`; `AppSession` refuses a command declared twice (it found a hidden duplicate `wait-idle` in a library test). Names and values of `c59.led`, `c59.cue-card`, `c59.printer` unchanged; 57 TI reference runs: 0 differ | +344 −376 | +222 −356 |
| — | Fix: a Calc-U-59 Web test race (`lib14_ac28` read the menu before it was drawn; local CI `web-59` failed now and then since before this work) | `expect.poll` | +4 −4 | +4 −4 |
| — | Fix: Calc-U-1600 Qt6 use-after-free at window teardown (ASan: `HostKeys::clear` from `~QWidget`'s `focusChanged`; intermittent SIGTRAP of `CalcU1600Shots.05-peripherals`; since Part D P-8, `0c9fe60`) | the connection dropped first in `~MainWindow` | +5 −1 | +5 −1 |
| E-8 | Dead hooks, test-only wrappers, a second file interface (S-8) | Gone: `typing::PauseProbe`, `SessionHost::setServiceHandler`/`answer`, `PlaceKeyRule::byName`, the mutable `OpenPart::setup()` (upload folder now a setup field), `PaperPart`'s nullable control, 16 + about 20 product settings calls only tests used, 8 Qt wrappers, Calc-U-59's `FileIO`/`plainFileIO`/`FileIOAccess` (`TIAppSetup::files` is LIB-7's `FileAccess`; Swift's `CoordinatedFiles` implements it); the library's CTest entries on `calcu_add_test` (`ctest -N` identical); test `textArg`/`intArg` on `sessions/Args.hpp`. Kept: `KeyObserver` (a cited test seam), `SharpRoms` in the core (the core's module scan needs it) | +431 −572 | +303 −554 |
| E-5 | **Large change 3**, Web pages (S-4) | `useShellProduct`, `useFaceplateMaps`, `calcuOpenFiles`, `calcuFileDrop` (each product's drop rule kept), `ShellSavedMedia`, `ShellSlotPicker`, `calcuMediaPlace`, one `CALCU_ASKS` with `calcuAskedQuestion`/`calcuSendAsking`, `library/web/src/canvas.js` (`calcImage`, `sizeCanvas`), `useShell(…, services)` (`createC59Shell` gone). 17 page states compared before and after: 0 pixels and byte-identical DOM | +392 −269 | +280 −260 |
| E-9 | Names outside the glossary in code (S-5) | Calc-U-1600's app-layer namespace `shared` → `cu1600` (589 lines, 139 files); 11 `shared_*_tests.cpp` renamed by what they test; Swift `TIFreezeOwner` → `TIRunLevel`, `freezeOwner` → `pausedLevel`, `unfreeze()` → `resume()`, `loadStateFile` → `loadPreset`; about 60 `Shared/` path comments, old labels and removed tools in comments; `POSITION_INDEPENDENT_CODE` for the gone `ti59headless` removed (every build checked). Labels, accessibility IDs unchanged | +772 −773 | +717 −719 |
| E-11 | Tests and build (S-11) | `CalcU1600SessionTests` split by folder (`CalcU1600AppSessionTests`, 61 of its 183 tests); `CalcU59TraceReader` 15 s → 0.1 s (CTest spent the time taking in 4 MB of `--json` output; it now goes to `/dev/null`, failures still show); one copy of the 318 KB tape fixture; `find_package(Python3)` once; `59/work/` (2.3.0 experiment, two presets the reader refuses) deleted. `TEST` counts unchanged (1121 / 622). Frequent 45.2 s → 31.4 s wall | +32 −1599 | +25 −1593 |
| E-12 | Agent knowledge and developer documents (S-6) | `glossary-mapping.md` as history, columns fixed, rows done or kept, the card classes open; skill `calc-u-59-shared` → `calc-u-59-app`; `SharedArchitecture.md` → `AppLayer.md`, `Shared-Architecture.md` → `App-Layer.md` (every link); stale classes and paths in developer docs and both `TODO.md`; a banner on Calc-U-1600's 25 historic plans (the inventory's open question answered); 15 memory files (recipes for removed tools → `calcu`, internals of gone classes dropped, findings kept); `docs/Process.md` phase row | +213 −173 | +14 −12 |

Total since `5db83d7`: +5 474 −6 432 (all), +3 934 −5 840 in `library/`
and `products/` without Markdown and pictures (of the deletions, about
1 500 lines are the deleted 2.3.0 experiment outputs of E-11).

## 5. The three large changes, and what came of them

1. **One assembly of a product's session: done (E-1, E-2).** The library
   builds a product's parts from the host's places and the ROM lookup,
   gives them in one order and publishes `roms-missing`; one host-side
   builder serves the seven hosts (two Web engines, two Qt controllers,
   the Swift bridge, both app layers' factories). Calc-U-59's app layer
   now takes the same inputs as Calc-U-1600's. Not unified: Calc-U-59 keeps
   its trace folder and limit fixed at the start and its Web `screenshot:`
   step writing into the save folder (two setup fields; nothing visible
   changes; Q-1, Q-2). E-2 added more lines than it removed: seven
   hand-written assemblies became one builder and two Qt host classes whose
   comments are new text.
2. **Calc-U-59's topics by value, one owner of `set-control`: done
   (E-3).** Calc-U-59 has Calc-U-1600's shape: topics hold values, the Web
   part draws from them. Not done: the PC-100C's controls in the control
   state's paper entry (it would change `control-state`'s value, which
   `calcu state` reads); the frontends keep building that list.
3. **The frontends' host glue in the component sets: done (E-4, E-5,
   E-6).** Each product's visible rules stay where they differ today
   (window raising and drop acceptance in Qt6, the drop rule on the Web:
   Q-4). Not merged: the three modal frames (their DOM and CSS differ; a
   merge changes pixels or the tests' selectors) and `hitMapOf` (the two
   faceplate maps have different shapes).

## 6. The candidates rejected in C1 and D, looked at again

| Candidate (where rejected) | Now |
|---|---|
| Calc-U-59's change flags, observer, `TISnapshot` serials (C1) | Done (E-3): the scans that made a by-value compare costly left with LIB-7/LIB-10 |
| One page-form topic set (C1, D) | Done in part (E-3): `c59.web.printer`, `c59.web.run-state` gone; the LED cell bytes and the cue card's draw list stay (layout numbers, `app/` by the product's rule) |
| A library `engine.js` skeleton (C1, D) | Done (M-49/M-57 and E-5, E-6) |
| Products on the library's host services (C1, D) | Done for opening and dropping (E-4, E-5). Two small rests not done: Calc-U-1600 `painters.js` writes the clipboard itself, Calc-U-59 `Debugger.jsx` has a raw file input |
| Calc-U-59 on the part-owned `start` and `roms-missing` (C1) | `start` done in M-54; `roms-missing` done (E-1) |
| Notices with a packed title (C1) | Gone (M-58; the last function in E-3) |
| `PtyHostLink` without threads (C1) | Still rejected: no measurement says it pays |
| F23/F25 test merges (C1) | Still rejected: the product tests assert product facts |
| Shared `textArg`/`runFor` in library test targets (C1) | `textArg`/`intArg` done (E-8); `runFor` still rejected (three different clocks) |
| `/tmp` paths in preset tests (C1) | Gone (M-46) |
| The 59 rig's own clock, 1600's call-counting pump (C1) | Still rejected: the reference runs' 1 ms; the pump counts calls |
| The library's CTest entries off `calcu_add_test` (C1) | Done (E-8): the function takes a working directory now |
| `AppSession::context()` (C1) | Kept: host log lines and tests use it |
| No-menu-bar shell form, value-form fields, storage banner (D) | Gone (M-57, M-58) |
| `shellModel.js` computed in the worker (D) | Still rejected: a LIB-14 D-2 change with pointer-latency questions |
| Calc-U-1600 `Dialogs.jsx`, drop and open handling, focus reclaim (D) | Drop and open done (E-4, E-5); the modal frames still rejected, now for pixels and selectors, not for an API on one user |
| `AudioSink`/`SoundFeed`, `SerialScriptsPart`, Sharp's serial seam (D) | Still rejected: the second user is an SC61860 product (§11) |
| Porting the ~28 core test files off the old typers (D) | Still rejected: C1600-2 AC-7's oracle and the golden runs' boot path |
| CoreTests from a restored state; `LegacyExpression.hpp` (D) | Still rejected; Frequent has headroom now (E-11) |
| `typing::PauseProbe`, `KeyObserver`, the optional loader, `wait-idle` mapping (D) | `PauseProbe` gone (E-8); `KeyObserver` kept as a test seam (LIB-5's qa-report cites its tests); optional parts and the refusal gone (E-1: every SC61860 fit table gives a machine all five parts) |
| `TypingProduct::typesText()`, `runControlSteps` per product, the duplicate Sharp preset tests (D) | Still rejected, reasons unchanged |
| Rename `calcu_core`/`calcu_shared` (D) | Still rejected (a workflow change for a name); the namespace was renamed (E-9) |
| `tools/check_no_old_tools.cmake` (D) | Kept: a cheap guard |
| `c1600.upload`, `c1600.lcd-text` (D) | Kept; whether test-only commands may be reachable through the embed handle is Q-6 |
| The library's test rigs on `LibraryParts` (D) | Still rejected: they build part subsets on purpose |

## 7. Candidates rejected this time

| Candidate | Reason |
|---|---|
| A shared `HarnessProduct` skeleton (survey S-2) | After E-1 the two `prepare()` share about six lines; a template base adds a concept without removing lines |
| The PC-100C's controls in `control-state` (S-3) | Changes the value `calcu state control-state` reads; Swift draws its own buttons anyway |
| One modal frame on the Web; one `hitMapOf` | Pixels and the tests' selectors; the two faceplate maps differ in shape |
| A C++ helper registering `<prefix>.shell-product` | About 25 lines; left out to keep E-5 free of C++ while E-8 ran |
| One `rom_missing_build.sh` driver | About 8 shared lines once ROM globs, environment names and 59's harness check are parameters |
| A composite action for Qt and ROMs in the workflows | The Qt install differs per product; the ROM fetch is one line |
| `SharpRoms` from the core to `app/` | The core's module folder scan needs `fullRomSet()` |
| `settings::lowerAscii` onto `machine::lowerAscii` | `settings` may not include `machine` (LIB-1 §2a) |
| Calc-U-1600's core reads on UTF-8 paths | They take native narrow paths; Windows would change |
| The shared help text (the settings-file table twice) | User-visible help text changes |
| "Waiting for a medium is idle" in `IdleWatch` (S-14 of the notes) | Could change when a Calc-U-1600 preset sees idle |
| One segment-to-text rule (`positionText`, `tiPositionText`) | The `expect` refusal's "shows" text would change |
| `SessionThread` into the library | One threaded host |
| Renaming Calc-U-1600's card classes | Tied to the public `.card.yaml` suffix and names the mapping never gave: Q-5 |
| `extra_assets` (examples zip) out of `1600/release.yaml` | M-61, Part F |
| Moving the Sharp cassette code into the library | SC61860 work, after the releases (Briefing §6; §11) |

## 8. Departures from approved `design.md` files

Each design that changed has a section "Part E simplification E-n" naming
the change: C59-5 (E-8), C59-6 (E-3), LIB-1 (E-1, E-2, E-7, E-8), LIB-2
(E-3), LIB-3 (E-1, E-2, E-3), LIB-5 (E-8), LIB-6 (E-1, E-8), LIB-7 (E-4,
E-5, E-8), LIB-8 (E-3), LIB-9 (E-1, E-2, E-8), LIB-10 (E-1), LIB-11 (E-1),
LIB-12 (E-1, E-8), LIB-13 (E-4, E-5, E-7, E-8), LIB-14 (E-1, E-2, E-4,
E-5, E-8), LIB-15 (E-3, E-5; E-6 as a departure bullet), LIB-18 (E-1, E-3).
The main ones:
- LIB-1 §2a: a new area `library/files` (any area may include it); the
  host-side builder in `library/shell` (not `parts`, which may not include
  `shell`).
- LIB-3: `AppSession` refuses a command declared twice; topics of
  Calc-U-59 compare by value.
- LIB-9: `FrontendKind::Given`; `HostDirs` carries the trace and screenshot
  folders; `LibraryPartsSetup::placesAtStart` and `screenshotFolder` (Q-1,
  Q-2).
- LIB-11: the product interface's members; `prepare()` returns the lookup.
- LIB-18: `ProductMachines::attaching` (TRACE's kept position).
- LIB-15 §3.5: the version through `release.yaml`.

No spec, format document, `features/INDEX.md` row or ledger row changed
(`git diff 5db83d7` over them is empty).

## 9. Visible behaviour

Nothing a user of a frontend sees changed. Checked: the 33 pictures of
Calc-U-1600's Qt6 scenarios (`CalcU1600Shots.*`) and the 4 App Store shots
that produce a picture are pixel-identical to the baseline taken on
`main` before any change; E-5's 17 Web page states are pixel- and
DOM-identical; both `dist/` byte-identical (E-6). Changes a harness or
edge-case user can see, kept and put as questions: `set-control` without a
PC-100C (Q-3); the Swift frontend's harness attach now has presets and
media (before: refused with a wrong sentence); a file macOS hands over
while Calc-U-59's Qt6 starts is no longer dropped (LIB-13 AC-7).

## 10. Questions for the user

**Kept per product in this pass (one rule, or keep?)**
- **Q-1 (LIB-9 AC-17, LIB-6 D-13).** The spec says a preset's `screenshot:`
  step on the Web offers a download (Calc-U-1600 does); Calc-U-59's Web
  writes it into the save folder. Follow the spec (a ledger row) or keep?
  **Answered (user, 2026-11-17):** follow the spec: Calc-U-59's Web offers
  the download too; a ledger row.
- **Q-2 (LIB-10, LIB-9).** Calc-U-1600 reads the trace folder and limit at
  each trace and checks that the folder can be written ("cannot write to
  the folder …"); Calc-U-59 keeps the folders of its start ("The trace
  folder cannot be written: <reason>"). One rule?
  **Answered (user, 2026-11-17):** one rule: both read the folder and the
  limit at each trace start (LIB-14 AC-11) and refuse with "The trace
  folder cannot be written: <reason>."; a ledger row per product. Checked:
  Swift macOS is sandboxed (`ENABLE_APP_SANDBOX`, user-selected files);
  the open panel grants the chosen folder for the run, the bookmark
  (LIB-9 AC-34) only carries it to the next launch, so the session takes
  the folder from the frontend at each trace; the Qt6 `.dmg` builds are
  Developer ID + hardened runtime, not sandboxed, and need no bookmark.
- **Q-3 (LIB-2, C59-6).** `set-control` without a PC-100C plugged in is
  now LIB-2's refusal ("No PC-100C is plugged in."); it was "done". No GUI
  can send it. Accept?
  **Answered (user, 2026-11-17):** accept the refusal (a Calc-U-59 ledger
  row). On the hardware the PC-100C's switches work without a calculator,
  but a preset that needs the printer plugs it in first.
- **Q-4 (LIB-13 AC-6, AC-14, AC-21).** Qt6: Calc-U-1600 raises its window
  on a drop and on Open With and accepts the drop's action; Calc-U-59 does
  not. Web: Calc-U-1600 opens a drop of exactly one file and ignores more;
  Calc-U-59 opens the first. One rule?
  **Answered (user, 2026-11-17):** Calc-U-1600's rule for both: raise the
  window on a drop and on Open With, accept the drop's action, the Web
  opens a drop of exactly one file only; a Calc-U-59 ledger row.

**Names and public surfaces that freeze at the release**
- **Q-5.** Calc-U-1600's `CardBase`, `ExpansionCard`, `SoftwareDefinedCard`,
  `BatteryCardInstance`, `MemoryCardDefinition` and the suffix `.card.yaml`
  (the glossary avoids "card"): rename (and to what), or keep as
  core-internal names?
  **Answered (user, 2026-11-17):** the module definition becomes
  `.module.yaml` with `format: memory-module`; `.card.yaml` is not read
  (0.9.0 reads no old formats; no exception), so ledger row 1600-1's
  "module definitions stay readable" changes; every shipped preset and
  example is updated. The C++ class names stay core-internal.
- **Q-6. Surfaces that look wrong** (details in the survey notes):
  - harness command words `ti59`/`pc1600` (machine names; `pc1600` also
    runs the PC-1500) beside the prefixes `c59.`/`c1600.` and the DAP's
    `calcu1600/`;
    **Answered (user, 2026-11-17):** one word per product everywhere:
    `c59`, `c1600` (a future Calc-U-1403: `c1403`): `c59 asm`,
    `c1600 dump-basic`, DAP `c1600/load`; the dotted prefixes stay; the one-shot selector too: `--product c59|c1600`;
  - Calc-U-59's debug-panel topics without a prefix (`program-listing`, …);
  - `ti59 registers` (data registers) beside the common `registers` (CPU);
  - `docs/contracts/harness.md` H-2 promises a common `dump`, H-1 names
    `load`/`insert`/`fit`;
  - three spellings per medium (`cassette-tape` / `cassette` / `.tape.yaml`;
    `.magnetcard.yaml`);
    **Answered (user, 2026-11-17):** "cassette" names the medium
    everywhere: kind, step, eject word and `format: cassette` (no machine
    prefix; a CE-126P tape would get its own format), `.cassette.yaml`,
    `cassette-name`, the cassette folder, `c1600 cassette record|status`;
    the place is the `cassette-recorder` (CE-125 manual: "microcassette
    recorder", "tape counter"; PC-1403 manual: "cassette recorder"); the
    glossary's "Cassette tape" and "Place" change with a decision-log row;
  - no `format:` key in the preset and the module definition; no version
    in the `calcu` line protocol; ASM overlay version "2.0";
  - `lcd` for the TI's LED display; camelCase `debug:` keys (`allReset`);
    **Answered (user, 2026-11-17):** `lcd` becomes `display-text`
    (`c1600.lcd-text` → `c1600.display-text`); the `debug:` keys stay
    camelCase, the debugger's own words shared with `launch.json` (a note
    in the format document); the VS Code extension's debug type becomes
    `c1600` (with Q2's rule);
  - INI: Calc-U-1600's `[window]` has 2 keys, Calc-U-59's 8; `[folders]
    other` declared but unused; `[trace] max-file-size-mb` (LIB-9 AC-11:
    0 means 50, the declaration has min 1; the unit is MiB);
  - hex prefixes `0x`/`$`/`&` across the media writers; `saved` and the
    leading comment line only on some media;
  - the Web global `PlayCalculator` (Calc-U-1600's is `CalcU1600`) and its
    `portraitWidth`; test-only commands (`c1600.upload`, `c1600.lcd-text`,
    `c59.web.key-button-*`) reachable through the embed handle; the site
    hook `[data-panel=printer]` → `[data-panel=paper]` (calc-u-site updated?);
    **Answered (user, 2026-11-17):** components `CalcU59` and `CalcU1600`
    with the shared props (`width`, `keyboard`, `layout`, `preset`, then
    product extras); test-only commands not in a release build's handle;
    calc-u-site adjusted in M-62;
  - internal paths and "M-47" in Calc-U-1600's shipped help
    (`09-module-definitions.md`); the `<!-- follows: -->` lines in the asset;
  - `c59.led`, `c59.cue-card`, `c59.printer`: part of the frozen `calcu`
    protocol in their current shape?
    **Answered (user, 2026-11-17):** `c59.cue-card` stays; `c59.printer`
    goes (its `trace` is the PC-100C control in `control-state`, `connected`
    its place in `capabilities`); `c59.led` becomes `c59.calc-indicator`
    (the frame through `display` if its payload can carry it). Also:
    `ti59 registers` becomes `c59 data-registers`; the other small items
    of Q-6 as proposed in the session (topics `c59.`-prefixed, the
    contract corrected, writers `0x`, `saved` and the comment line on
    every medium, `[folders] other` removed, AC-11 for the trace size,
    the help's internal references removed; `[window]`, the protocol
    without a version and ASM 2.0 kept).
- **Q-7 (preset.md §9.3).** The card format 3 text says the rows' bytes are
  "in reverse order"; the codec writes them in order (checked against a
  ROM-written card). Correct the document?
  **Answered (user, 2026-11-17):** correct it ("copied unchanged, in the
  same order"), check the other rows of §9.3 against the codec and add a
  worked example from a real card.
- **Q-8 (ADR 0001).** The ADR puts Sharp BASIC, SDE and the cassette tape
  in the library; they are in Calc-U-1600. Move them with the third
  product's first step (an ADR note), or now?
  **Answered (user, 2026-11-17):** with the third product's first step; a
  note on ADR 0001 says they stay in Calc-U-1600 until a second product
  needs them.

**Findings (no golden or reference run differed)**
- Golden replays of both products equal after every merge; the 57 TI
  reference runs "0 of 57 differ" (the accepted differences unchanged);
  the 32 Sharp reference runs pass.
- F-1: Calc-U-1600 Qt6 use-after-free at teardown (since D P-8), fixed
  (`570de53`).
- F-2: Calc-U-59 Web test race `lib14_ac28`, fixed (`b83d50e`); it was the
  `web-59` local CI failures seen since Part E.
- F-3: a library attach test had silently replaced LIB-6's `wait-idle`
  since E-1; found by E-3's duplicate check, removed.
- F-4: `CardScenarioTests.test_c59_7_ac12_card_read_by_its_corner` fails
  once the simulator holds many cards (40 left by earlier runs; a lazily
  built list row off screen does not exist for XCUITest); it failed alone
  at `5db83d7` too and passed after the app was removed from the
  simulator. The test should remove its cards or scroll.
- F-5: the App Store screenshot plan fails 4 of 9 shots on `main` since
  before this work (`testScreenshotCPUInspector`: two `confirm-cold-boot`
  buttons; `CalcDebugger` ×2 and `Glyphs`: `AppHelpers.swift:91`);
  `ipad-01` writes no picture. Needed before the App Store release (M-64).
  **Fixed (2026-11-17, branch `fix-f5-screenshots`), no app change:**
  (1) one dialog: iOS 27 nests the dialog's button in a system button
  with the same identifier, so the query takes `.firstMatch` (as C59-8
  AC-5's test does); (2) `CalcDebugger` and `Glyphs` load presets that end
  paused (preset.md §5.6), and the bare `wait` on a paused machine waits
  until it is resumed (§6.2; `calcu serve` refuses instead, having no wall
  clock to wait on), so `loadPreset(…, endsPaused: true)` reads the
  `run-state` topic until it says paused; (3) M-56 renamed
  `testScreenshotAssembly` to `testC59_4_AC16AsmOverlay` and the runner,
  still asking for the old name, ran no test and passed: it names each test
  in full now and fails a shot without a PNG or of the wrong pixel size.
  All 9 shots pass with the sizes of the runner's header and were looked
  at. The iPhone Duo shot (the tenth, taken when the Duo is open) came out
  black once, while the Duo was reported folded partway through the run;
  the size check now rejects such a picture. Re-run with the Duo opened
  flat: passes, 2853 × 2007. Seen on it (not fixed, cosmetic): the narrow
  debug column wraps its header words mid-word ("DE-BUG", "RESUM E",
  "F.STAR T").
- F-6: `CalcU1600Shots.*` fail when the display sleeps ("screencapture
  failed"); run the Longer group under `caffeinate -d`.
- F-7: one local CI `core-1600` failure (GCC 13) and one `CalcU1600DapSmokeQt`
  failure under heavy load (load average up to 186 while agents ran); both
  passed on the re-run; the first's output was not kept.
- F-8: `products/calc-u-1600/tools/run_tests.sh` runs only `CoreTests`
  (since D P-10 the session tests are other programs).
- Outside the repository (not edited): `Sharp1500-1600-Ref` skill line 128
  (`headless/pc1500_cli`), calc-u-site skill `site-59` line 104
  (`StateFileFormat.md`), two memory notes of calc-u-site and
  SharpDataExchange.

## 11. Adding a PC-1403 or PC-1350: what `library/` would need

Read against the fit tables of `docs/contracts/`; nothing was changed for it.
1. **The Sharp cassette as library code.** Both models need a cassette with
   WAV; `TapeDeck`, `WavFile`, the tape kind and its `.tape.yaml` codec
   (and probably `PiezoSampler`) are Calc-U-1600 code, and the control
   bar's tape deck is the product kind `c1600.tape-deck`. A third product
   needs them moved and one library kind (counter, motor). ADR 0001 also
   names Sharp BASIC and SDE (Q-8).
2. **Display cells.** `DotMatrixGeometry::cells` is declared but neither
   the frame nor the `display` topic carries it, and conformance does not
   check it (LIB-1 AC-16 says the output comes in cells); `lcd` reads cells
   from a product `GlyphFont` of at most 8×8. Additions only if a frontend
   draws the gaps.
3. **Key readiness.** `Machine::isKeyReady()` is required by typing, the
   machine control, presets and the debugger's pause-on-start; for SC61860
   it is unknown (typing.md Q-1). The fallbacks exist
   (`ProductRules::readyNow`, the profile's `StartRule`); the debugger fit
   table says no pause-on-start, the library gives every target one.
4. **Small:** `typing::NamedKey` has F1–F6 only; `settings::Folder` only
   if CE-140F disks get their own folder; break and watch lines print
   addresses at 16 bits (`DebugCommands.cpp`), a reply text to settle
   before it freezes; `ci/linux/jobs.sh` turns the other product off by
   name, so a third product would build in every job.
5. **Generic already:** key IDs, media kinds, preset words and suffixes,
   ROM lookup, serial line settings, pacing per model, paper width and
   form, bar pickers, debugger spaces, registers and bank windows.
   Library generality that only SC61860 uses and that stays:
   `CellGrouping`, `AfterChange` per pin-out, the paper text layer, space
   widths per target, places owned by a peripheral.
6. The contracts disagree in places (the CE-140F and CE-124 on which
   model; a PC-1350 cell that refers to itself; the banked ROM missing in
   `debugger.md`; Briefing §1.4 against §1.6).

## 12. End of part, and line counts

- **Frequent** on `7b9b050`: 51 of 51, 31.4 s wall (`-j8`, quiet; 45.2 s
  at `5db83d7`): `CalcU1600SessionTests` 39.9 s of CPU beside
  `CalcU1600AppSessionTests` 11.2 s, `CoreTests` 14.2 s, `CalcU59Tests`
  8.7 s, `CalcU59TraceReader` 6.2 s, the library 4.2 s.
- **Longer** (`build/groups`, both `dist/` built first, without `-j`):
  30 of 34 at the first run; the four failures were `CalcU59UIRegression`
  (F-4) and three `CalcU1600Shots` (F-6, display asleep); all four passed
  on the re-run under `caffeinate -d` (after the simulator's app was
  reset). Every reference run, Playwright suite, both ROM-missing builds,
  the WASM smoke, `CalcU1600HarnessTests`, `CalcU1600MachineStateTests`,
  both DAP smokes, `CalcU1600QtTests`, `CalcU59SwiftUnitTests` and
  `CalcU59NoAttachInRelease` passed.
- **Screenshots (Checkpoint):** §9.
- **`tools/help check`:** both pass (Calc-U-59 732, Calc-U-1600 928 glossary
  "Avoid" warnings, as before).
- **Glossary grep** of every element of `docs/inventory/agent-knowledge.md`
  (and Calc-U-1600's 25 historic plans and the memory) for the old terms of
  `glossary-mapping.md`: 370 history in the historic plans (they carry a
  banner now), 66 history notes, 64 false positives, 32 flagged and
  reviewed by hand: none current inside the repository and the memory; the
  rest outside the repository (§10).
- **Local Linux CI** (GCC 13) on `7b9b050`: every job green (`library`, `core-59`, `core-1600`, `qt-59`, `qt-1600`, `help-59`, `help-1600`, `web-59`, `web-1600`); after every merge of this pass as well, but for F-2 and F-7.
- **GitHub CI** on `1fd29db`, both workflows green on every job: Calc-U-1600 44111986721 (core tests, `library / library` from the reusable workflow, help, Web, Linux x86_64 and aarch64, Windows x86_64 and arm64 from the new matrices with their old job names, macOS arm64 signed and notarised); Calc-U-59 44111994920 (core tests, `library / library`, help, Web, Linux and Windows per arch).

Line counts by the method of C1 and D (`.cpp .hpp .h .swift .js .jsx .mm
.py CMakeLists.txt .cmake`, tracked files, without `node_modules`, `dist`,
`build`, `vendor`; `.jsx` counted, so the D column differs a little from
D's report):

| | Import (`057a781`) | After C1 (`d5b5914`) | After D (`9e15777`) | Part E start (`5db83d7`) | Now |
|---|---|---|---|---|---|
| `library/` | 0 | 21 688 | 64 270 | 65 647 | 66 675 |
| `library/` without tests | 0 | 11 308 | 34 381 | 35 098 | 35 979 |
| Calc-U-1600 `app/` (without tests) | 5 470 | 6 327 | 9 967 | 9 971 | 9 811 |
| Calc-U-1600 `app/tests` | 1 610 | 1 740 | 2 585 | 2 614 | 2 614 |
| Calc-U-1600 `core/` | 33 051 | 34 210 | 30 209 | 30 209 | 30 203 |
| Calc-U-1600 `tests/` | 23 657 | 24 223 | 30 220 | 30 220 | 30 227 |
| Calc-U-1600 Qt6 frontend | 8 893 | 9 219 | 6 629 | 6 542 | 6 391 |
| Calc-U-1600 Web frontend | 3 671 | 2 572 | 2 750 | 2 749 | 2 538 |
| Calc-U-59 `app/` | 8 519 | 10 470 | 10 462 | 11 141 | 10 709 |
| Calc-U-59 `core/` | 3 306 | 4 937 | 4 979 | 6 533 | 6 524 |
| Calc-U-59 `tests/` | 3 538 | 7 226 | 8 154 | 14 669 | 14 703 |
| Calc-U-59 Qt6 frontend | 3 362 | 3 684 | 3 710 | 4 519 | 4 336 |
| Calc-U-59 Swift frontend | 6 888 | 7 882 | 7 920 | 9 893 | 9 899 |
| Calc-U-59 Web frontend | 3 938 | 2 978 | 3 029 | 3 422 | 3 202 |

Since the start of this pass the app layers and frontends of both
products lost about 1 350 lines and the library gained about 1 030 (880
outside its tests): what both products wrote is now written once, with
its tests; the rest of the reduction is in the build and test scaffolding
(the Web build, the workflows) that these counts do not show.

Not done here, needs the user: the worktrees and branches of this pass's
agents under `.claude/worktrees/` (the removal was refused by the
permission check; all are merged) and the two older ones
(`worktree-agent-a89862d74f216bacf`, `m26-tests`).
