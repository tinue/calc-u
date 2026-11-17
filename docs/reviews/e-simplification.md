# Part E: the last structural simplification before the releases

Status: survey and plan (2026-11-17). The survey (§1), the three large
changes (§2) and the plan (§3) are written and committed before any code
edit; §4 onwards report the result.

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
