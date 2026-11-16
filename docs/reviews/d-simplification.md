# Part D: structural simplification before the boundary to Part E

Status: done (2026-11-16). The survey (§1) and plan (§3) were written and
committed (`350bc61`) before any code edit; §4 to §11 report the result.

Baseline: `main` at `818374d`; build `build/tests` (RelWithDebInfo, Qt
OFF); Frequent 41/41, 57.3 s wall (`CoreTests` 57.3 s of its 60 s).

`59/` is `products/calc-u-59/`, `1600/` is `products/calc-u-1600/`.

## 1. Survey (read-only), ranked by payoff

Payoff = what Calc-U-59 does not have to write or work around in Part E,
then lines and concepts removed. Every item was checked in the code.

### S-1. The library's parts are wired by hand, five times (and a sixth in M-54)

A product's app layer builds about ten library parts and wires them:
`PresetsPart` and `StartupPart` need each other (the loader and the
machine control), so the control is handed over after construction
(`PresetsPart::setControl`, `PaperPart::setControl`,
`MediaPart::setControl`), the steps are added to the vocabulary
(`addPaperStep`, `addDebuggerSteps`, `addMediaToPresets`), the openers
are built over the presets and media parts, the parts go into the
session in an order that seven designs each state for their part
(LIB-6 §3.8, LIB-7 §3.7, LIB-8 §10.3, LIB-10 §10, LIB-13 §506, LIB-18
§3.5, C1600-3 §3.3). The same wiring is in:

| Where | |
|---|---|
| `1600/app/SharpAppPart.cpp:66-180` | the product's composition root |
| `library/harness/Run.cpp:178-266` | the harness's "own parts" path (the test product, Calc-U-59's adapter) |
| `library/presets/tests/PresetRig.hpp:127-137`, `media/tests/MediaFixture.hpp:114-122`, `debugger/tests/DebugRig.hpp:66-75` | three test rigs |

Calc-U-59's `TIAppPart` would be the sixth copy (M-54 to M-57). Around it:
`PresetsPart` has `setTyping` (no caller), five `!rules || !catalogue`
guards that never fire, an `ownElapsed_` fallback never used, and
`PresetLoader::load` takes the control a second way; the harness's
`HarnessProduct` has eleven per-part virtuals for the own path, and
Calc-U-1600's overrides of eight of them (`mediaKinds`, `moduleListing`,
`presetExtensions`, `fileKinds`, `controlSetup`, `presetBundle`,
`typingPart`, `paperPart`) are dead since its run always takes the
app-layer path; `Run.cpp` and `attach/AttachedHarness.cpp` list the same
eight `add*Commands` (a test keeps them equal).

### S-2. A pass-through media place forgets the fitted card (Sharp-shaped LIB-7)

The library treats the TI-59's card reader like a holding place
(`library/media/MediaPart.cpp`):
- `media.feed` erases the armed card (`:133-141`), so after one corner
  tap a second is refused ("Nothing is armed"); C59-7 AC-12 says the card
  "stays fitted afterwards", AC-17 and AC-25 likewise; Fit Fresh Card
  then side 0 then side 1 is impossible;
- the armed copy of the content goes stale after a write (`givenBack`
  updates only the binding, `:579-592`);
- `openFile` (`:1067`) and `choose` (`:1184`) feed side 0 into a
  pass-through place instead of fitting the card (LIB-13 AC-4, AC-12,
  C59-7 AC-11, AC-14); the library test of LIB-13 AC-4 passes only because
  its stand-in `opening/tests/TestOpeners.hpp:124,136` skips the reader;
- a feed while a card passes ejects it (`:677`); `arm` is not refused
  while a card passes (C59-7 AC-16); the topic has no "fitted" or
  "passing" to draw AC-11 to AC-13 from.

Sharp has no pass-through place, so nothing of Calc-U-1600 changes.

### S-3. Calc-U-1600's core carries code only its tests use

- The old typers `core/PC1500/PC1500BasicTyper.*` (363 lines),
  `core/PC1600/PC1600BasicTyper.*` (497), the job forwarder
  `core/Job.hpp` (40) and `PowerCycleSequence`'s test accessors: no
  `app/`, `frontends/` or `tools/` file includes them; about 28 core test
  files do (the golden runs use them as their reference typer, C1600-2
  AC-7 as its oracle), and `calcu_core` compiles them into every build
  (Qt, Web, WASM). C1600-2's design said `typeBasicProgramText(Job)` goes
  in M-44; it is still there (a departure no qa-report lists).
- The trace into a `FILE*` (`PC1500TraceFile` file mode, the four
  `begin/endCpuTrace…` of both machines): one test, whose comment names
  "the CLIs' preset loader" (gone in M-51).
- A second media resolver: `NamedFileCatalog.hpp` (`namedFileName`,
  `resolveNamedFile`), `FloppyImageFile.hpp:180-237` (catalogue entries,
  `resolveFloppyByName`, `readFloppyFile`), `MemoryCardCatalog.hpp:76-98`;
  only tests use them, and they pin rules LIB-7 replaced (two files of one
  name refused, `/` turned into `-`); `currentIso8601Timestamp()` default
  arguments read the wall clock where the library always passes the time.
- A second PNG encoder `core/Display/PngWriter.hpp` (184 lines) and
  `writeLcdScreenshotPng` (no caller); the product already uses
  `formats::PngWriter`.
- `core/Job.hpp`'s template `job::run(Machine&, cycles)` and
  `tests/job_tests.cpp`: re-test the library's `Task` through the
  forwarder.

### S-4. The trace recorder's budget rule is the product's (LIB-10)

`library/debugger/TraceSession.hpp:42-57` hands each recorder the rule of
AC-24/AC-43 (stop at the budget, keep room for the end record);
`1600/app/SharpTraceRecorder.cpp` (69 lines) implements it with its own
flag, `library/debugger/tests/TestDebugParts.hpp:60-99` again. The TI
core drains a ring as Sharp's does (`59/core/TIAdapters.hpp:87-120`), so
M-53 would write a fourth copy.

### S-5. Generic frontend code in Calc-U-1600 that Calc-U-59 needs again

- Shell data to page data: `1600/app/SharpShellProduct.cpp:86-215`
  (`shellProductValue`, `dialogSpecValue`, the hit map's value form)
  turns library types into what `library/web/components/shellModel.js`
  reads; `TIWebPart` would write it again (LIB-14 D-32: written once in
  C++).
- Host keys to `host-key-down`: `1600/frontends/qt6/app/HostKeyEvents.*`
  and `CalcU1600.jsx:29-50` (LIB-5 AC-7 is the family's rule; Calc-U-59
  has a third and fourth copy with a different modifier rule).
- `1600/app/StartFile.*` (the file named at the start; LIB-13 AC-14 asks
  the same of Calc-U-59, whose Qt6 `main.cpp:76-88` has its own loop).
- The React and Babel vendor files, byte-identical in both products'
  `frontends/web/vendor/` (3.2 MB each); the library's Web tests read
  Calc-U-1600's copy.
- Dead: 18 of 22 wrappers of `1600/frontends/qt6/app/AppSettings.hpp`;
  `shared::Settings`'s open-folder helpers (production goes through
  `SettingsPart`).

### S-6. The help tool reads only one way of versioning

`tools/helptool/manifest.py:75-103` refuses a `release.yaml` version
preset other than `cmake`; Calc-U-59's is `xcode` (MARKETING_VERSION), so
M-60's exit check would fail; LIB-16 D-9 says the version is read "as
ReleaseIt reads it". The shared chapter list is in three places
(`CHAPTERS` in `manifest.py`, `library/help/help.yaml`, a check that they
agree). Otherwise the tool is generic (chapters, links, `names:`,
settings keys); calc-u-site `/59/` converted to Markdown fits it.

### S-7. Both products' printer references in the library's paper tests

`library/paper/tests/fixtures/ce150.marks` is byte for byte
`1600/tests/reference/v0.8.1-examples/plotter_ce150_demo.pc1500a.CE-150.paper-marks`,
`pc100c.marks` is Calc-U-59's `diag.ti59.paper-marks`;
`PaperFixtures.cpp:10-43` copies the CE-150, CE-1600P values of
`core/Connector/PlotterPaperOutput.hpp` and a PC-100C description "until
M-53 confirms it". (The "17 000 lines" of `library/paper/tests` are the
PNG fixtures counted by `wc -l`; the code is 1 250 lines.)

### S-8. Names and lookups written several times

- Key names to presses: `typing/TypingPart.cpp:61-74`,
  `presets/LibraryKeywords.cpp:37-51, 85-109`, `harness/FirstCommands.cpp:41-54`,
  each with its own `lower()`; models, peripherals, variants by name
  without regard to case: `presets/Reader.cpp`, `startup/StartupPart.cpp:30-53`.
- Three private `scalarText` beside `formats::scalarText`.
- The expect hook `ProductRules::takesRegex()` cannot say "a number"
  (C59-2, LIB-6 AC-40, AC-68): a TI `expect: abc` would pass the reader
  and fail while running (all or nothing, ADR 0017).
- A header key's peripherals go read → `keep` → `productHeader` →
  `amendPlan` with `dynamic_cast`; `HeaderKeyDef::afterOtherHeaderKeys` has
  no user.
- The step-count limit is checked by the harness (`DebugCommands.cpp:191-205`)
  besides `DebuggerSteps.cpp:30`; peek and poke repeat the bank-window
  checks (`DebuggerPart.cpp:323-340, 457-472`); `DebuggerSetup::runControlSteps`
  is a per-model function that is constant per product;
  `setTraceFolder/setTraceLimit` store each value twice.

### S-9. Dead code, leftovers and stale text (Calc-U-1600 and library)

- No caller: `SharpProductMachines::ModuleCarry`/`setModuleCarry`
  ("until M-47"); `SharpAppPart::presetEntries`; `endsWithIgnoringCase`;
  `TapeDeck::loadForPlayBytes`; `PC1600Machine::loadLH5803RomFile`,
  `ce1600pCard`; `Ce158Card::uartStatus`; `HostDirectoryDrive::fcbSize`;
  `LH5801::setFlagZ`; `SerialPort::receivedWaiting`; `Shell::menuModel`,
  `aboutDialog`; `HitMap::setMap`; `typing::frames()`;
  `ReadContext::pluggedAny`; `PresetsPart`'s `cause == "startup"`;
  `RunContext::display()`'s fallback; the null-catalogue path of
  `computeControlState`.
- Sharp marker rules listing TI keywords the reader refuses first
  (`SharpPresetRules.cpp:110-114`).
- Stale comments and documents: `LegacyCalls`, `JobForward`, the CLIs,
  "Reserved for LIB-10", `core/HostClock.hpp`, `LazySerialLink`,
  `PtySerialLink`, "adapter A-4", "until M-52"
  (`SharpPresetRules.hpp:41`), `Shared/`/`Core/` paths; `1600/docs/developer/Shared-Architecture.md`;
  `docs/test-groups.md` (files of CoreTests gone or missing, the CLIs
  under "Not in a group"); `1600/TODO.md` cleanup backlog items 1 to 4
  (every class named is gone); root and product `CLAUDE.md` on
  `core/Job.hpp`.
- CMake: `TOOLS_ROOT` never read, a dead `core/tests/` filter,
  `calcu_add_core()` with one caller, `find_package(Python3)` twice, four
  near-identical test-program blocks; `.gitignore` with the old `Core/`
  case (Linux sees a fetched `libsharpdx-linux.a` as untracked).
- Four `TEST` names twice in CoreTests (`CoreTests <name>` runs both).

### S-10. CoreTests at 57 to 59 s of its 60 s

183 tests in 19 files on `SessionFixture` take 33 s (a fresh session and
cold boot per test); 44 tests ≥ 0.3 s take 30 s. CoreTests is one
program, so ctest cannot spread it.

### S-11. Seen and fine (kept)

`DebugTarget`, `RunControl`, levels (TI's program level, 32 HOLDs, the
50000 count) fit TMC0501; `IdleRule`, the engine's loop, `MachinePlan`'s
content and `BuildOrigin` fit C59-2 and C59-3; `KindDecl`, codecs,
in-file media, `ModuleListing` fit card format 3 and the module picker;
`PaperDescription` fits the PC-100C; `FileKind` fits the keystroke file
and the ASM overlay; `HitMap`, `NoticeQueue`, `LoadingRule`,
`SettingsPart`, `WorkerHost` are generic; the DAP stays product code
(`docs/contracts/debugger.md` DB-4: no TI DAP); `ServeAddOn` is
spec-defined; no Sharp knowledge in the library's code beyond comments.

## 2. The three largest changes if effort were free

1. **One assembly of the library's parts (S-1; fits Calc-U-59).** A
   library area that builds the library's parts of a product's app
   session from one product description, wires them (machine control
   first, handed to every part at construction; the steps; the openers)
   and gives them in the one order. `SharpAppPart`, the harness's own
   path and the rigs use it; `TIAppPart` takes it in M-54 instead of
   writing it a sixth time; the harness's per-part virtuals become one.
2. **The pass-through media place made right (S-2; fits Calc-U-59).**
   The fitted card is the binding; a feed inserts a side and keeps it;
   opening or choosing fits; the topic says fitted and passing; the
   library tests of LIB-13 AC-4 run on the real `MediaPart`.
3. **Calc-U-1600's core holds what the products run (S-3).** The old
   typers and the job forwarder leave `core/` for the tests' support
   (they stay the golden runs' and AC-7's reference); the `FILE*` trace,
   the second media resolver and PNG encoder go.

## 3. Plan: independent commits, one short branch each

| # | Branch | Does one thing | From |
|---|---|---|---|
| P-1 | `ds-dead` | S-9: dead code, stale text, CMake leftovers, `.gitignore`, test names | |
| P-2 | `ds-assembly` | large change 1 (S-1) | |
| P-3 | `ds-pass-through` | large change 2 (S-2) | |
| P-4 | `ds-core` | large change 3 (S-3) | |
| P-5 | `ds-trace-records` | S-4: the library owns the trace budget | |
| P-6 | `ds-names` | S-8: names, lookups, the expect hook, debugger duplicates | |
| P-7 | `ds-paper-refs` | S-7: the Sharp printers' references to Calc-U-1600 | |
| P-8 | `ds-frontend` | S-5: shell value forms, host keys, start file, vendor files to the library | |
| P-9 | `ds-help` | S-6: the help tool reads every version preset; one chapter list | |
| P-10 | `ds-test-split` | S-10: Calc-U-1600's session tests a program of their own | |

Each: build, Frequent (golden replays included), merge `--no-ff`; the
local Linux CI after the merge (web jobs for P-8). At the end: the Longer
group locally for both products, then CI.


## 4. Changes made

Every change: build (`build/tests`; the Qt tree `build/groups` where a
change reached Qt), Frequent green, golden replays unchanged, merged
`--no-ff`; the local Linux CI ran on `ea1c032` and on `7160c26`, all jobs
green. Lines are `git diff --shortstat` of each merge (code, tests and
documents together). P-2, P-10 and P-1 were done by me; P-3 to P-9 by one
implementing agent each in a worktree (the plan's way of running steps).

| # | Change and reason | Before → after | Lines |
|---|---|---|---|
| P-1 | Dead code and stale text (S-9) | 16 functions and flags without a caller gone (`ModuleCarry`, `presetEntries`, `loadForPlayBytes`, `loadLH5803RomFile`, `ce1600pCard`, `uartStatus`, `fcbSize`, `setFlagZ`, `receivedWaiting`, `Shell::menuModel`/`aboutDialog`, `HitMap::setMap`, `typing::frames`, `pluggedAny`, `afterOtherHeaderKeys`, `endsWithIgnoringCase`); Sharp's marker rules no longer list the TI keywords the reader refuses first; CMake: one `calcu1600_test_program()` for the five test programs, `calcu_core` inline, a dead variable, filter and second `find_package` gone; `.gitignore` names `core/` in its case; four duplicate `TEST` names renamed; comments, `TODO.md` backlog items on gone classes, `Shared-Architecture.md`, `test-groups.md` | +79 −252 |
| P-2 | **Large change 1** (S-1): one assembly of the library's parts | `library/parts/LibraryParts` builds the library's session parts from one setup, makes LIB-18's machine control first and hands it to presets, startup, paper and media at construction, adds the paper, debugger and media steps, builds the openers and gives the parts in the one order (§5). `SharpAppPart` keeps only its data and its own parts; every harness product returns a `LibraryParts` (the run's settings and folders from `RunSetup::partsSetup`). Gone: `setControl` of presets, paper and media, `PresetsPart::setTyping`, five guards that never fired, `RunContext`'s fallback elapsed base, `StartupPart`'s own control, the harness's own-parts path, `AppLayerParts`, `RunParts`, eleven per-part `HarnessProduct` members, the second command list of attach (`addServeCommands`) and `presetRules()` as the harness's "has presets" | +541 −509, follow-up `3ac02a8` |
| P-3 | **Large change 2** (S-2): the pass-through media place | For a pass-through place the binding is the fitted card: `arm` fits, `feed` inserts a side and the card stays fitted, the reader's give-back updates it, opening and choosing fit instead of feeding, arm/choose/open/feed are refused while a card passes, the topic says `holds` (fitted) and `passing`; AC-27's line for a fitted card that never passed. LIB-13 AC-4's pass-through test runs on the real `MediaPart`; the stand-in no longer skips the reader | +366 −86 |
| P-4 | **Large change 3** (S-3): Calc-U-1600's core holds what the products run | The old typers, the job forwarder and the raw plot points moved to `tests/support/` (compiled into the tests only); the `FILE*` trace mode and the four `begin/endCpuTrace…` of both machines, the second media resolver (`NamedFileCatalog`, floppy and card catalogue entries and name resolvers), the wall-clock default arguments, the second PNG encoder and `tests/job_tests.cpp` gone. `core/` 32 293 → 30 832 production lines | +266 −965 |
| P-5 | The library owns the trace budget (S-4) | A recorder is a byte source (`begin`, `take(out, budget)` whole records, `endBytes`, `end`); `TraceSession` decides what goes into the file and keeps the end record's room. `SharpTraceRecorder` is a pass-through; the step-count limit is one constant held by the session command; one bank-window helper for peek and poke; the trace folder and limit stored once. Trace bytes identical to `7160c26` (300 ms and 1 s traces, two traces ended by the 1 MB limit) | +159 −135 |
| P-6 | Names, lookups, the expect hook, header peripherals (S-8) | `typing/KeyNames` (key names to presses, once, for the `key` command, the `key:` step and the harness's `key-down`), `machine/Names` (`lowerAscii`, `modelNamed`, `peripheralNamed`, `variantNamed`; seven `lower()` copies and two lookups gone), two private `scalarText` copies gone, `ModelRules::checkExpect` replaces `takesRegex()`, a header key adds its peripherals to the plan itself (`ReadContext::addPeripheral`, `LoadPlan::headerPeripherals`), so `amendPlan`, `amendSharpPlan` and the casts go; one read helper for `load`/`loadWithin`; the null-catalogue path of the control state gone. The 32 Sharp reference runs compared before and after: equal | +486 −333 |
| P-7 | The Sharp printers' references to Calc-U-1600 (S-7) | The CE-150 and CE-1600P reference pictures (byte-identical PNGs, `git mv`) are Calc-U-1600's `lib8_ac16_reference_pictures_of_the_plotters` on the product's real descriptions; the library's copied CE-150 marks and plotter descriptions gone. The PC-100C's stays in the library until M-53 (Calc-U-59 has no paper description yet) | +150 −59 |
| P-8 | Generic frontend code to the library (S-5) | `shell/ValueForms` (the shell product, dialog spec and hit map as the page reads them; Values byte-identical), `qt6/HostKeys` and `web/components/hostKeys.js` (host keys to `host-key-down`, held keys, focus lost), `opening/StartFile`, one `library/web/vendor/` (both products' copies gone; Calc-U-59's `dist/` identical); 21 `AppSettings` wrappers and ten `shared::Settings` folder helpers without a production caller gone | +664 −781 |
| P-9 | The help tool reads every version preset; one chapter list (S-6) | `version()` reads ReleaseIt's four presets as ReleaseIt does (`xcode`: the first `MARKETING_VERSION`; Calc-U-59 reads 3.0.0); the chapter list only in `library/help/help.yaml` (`CHAPTERS` and the agreement check gone). Calc-U-1600's help asset byte-identical | +194 −101 |
| P-10 | Calc-U-1600's session tests a program of their own (S-10) | `CalcU1600SessionTests`: the test files that use `SessionFixture`, picked by CMake; ctest runs it beside `CoreTests`. Frequent wall time 57 s → 32 to 36 s on a quiet machine | +24 −3 |

Total since `818374d` (code, tests and documents): +3 173 −3 227; code
and tests alone (`library`, `products`, no Markdown, no pictures):
+2 361 −2 941.

## 5. The three large changes, and what came of them

1. **One assembly of the library's parts: done (P-2).** The order of the
   parts, which seven designs stated for their own part, is in one place
   (`library/parts/LibraryParts.hpp`): ControlPart first (the host's own
   parts after it), then typing, peripherals, presets, startup, media, the
   product's parts its file kinds use, open, paper, debugger, then the
   product's own. Calc-U-1600's order is unchanged. The harness's
   own-parts path, planned to go in M-59, went now: Calc-U-59's adapter
   and the library's test product build a `LibraryParts` in `prepare()`
   like Calc-U-1600 does. Calc-U-59's adapter session now also has
   LIB-18's ordering of its parts (typing first), with the same set; its
   tests pass. In M-54 `TIAppPart` fills a `LibraryPartsSetup` with its
   data instead of building and wiring the parts. Not converted: the
   library's four test rigs, which build part subsets on purpose (one
   reads with another catalogue, AC-52); they take the machine control at
   construction now and lost their `setControl` calls.
2. **The pass-through media place made right: done (P-3).** M-55 builds
   the card reader on a place that keeps the card fitted across passes, as
   C59-7 AC-11 to AC-17 and AC-25 describe. Two spec readings are listed in
   §10 (eject while a card passes; AC-27 at a variant switch). Left for
   M-55: the bank numbers of AC-11 per side (the product reads them from
   the fitted content), and a passing card ejected with a whole written
   bank (AC-10) reported through `takeGivenBack()`.
3. **Calc-U-1600's core holds what the products run: done (P-4).** The old
   typers stay the golden runs' and C1600-2 AC-7's reference, now as test
   support. Rejected part: porting the ~28 core test files off the old
   typers (§7).

## 6. The candidates rejected in C1, looked at again

| C1 candidate | Now |
|---|---|
| One page-form topic set per product (`SharpWebPart` 631, `TIWebPart` 663 lines) | Done for Calc-U-1600 by M-49 (`SharpWebPart` 137 lines, real product requests). `TIWebPart` waits for M-53 to M-57, which replace its interim topics with their owners'; still rejected for Calc-U-59 |
| A library `engine.js` skeleton | Calc-U-1600's page is on LIB-14's shell (M-49); Calc-U-59's moves there in M-57. Still rejected (Part E) |
| Products on the library's host services | Calc-U-1600 done (M-48, M-49); Calc-U-59 in M-56, M-57. Still rejected (Part E) |
| Calc-U-59 on the part-owned `start` and `roms-missing` | LIB-18 59 is M-54; the harness adapter now has LIB-18's part through `LibraryParts`. The app layer still rejected (Part E) |
| Notices without a packed title in Calc-U-59 | C59-8 (M-58); user-visible in today's alerts. Still rejected |
| `PtyHostLink` without threads | No measurement says it pays. Still rejected |
| F23/F25 test merges | The product tests assert product facts. Still rejected |
| Shared `textArg`/`runFor` in three library test targets | Still about ten lines against new include paths. Still rejected |
| `/tmp` paths in ~8 preset tests | Gone: none left (M-46 rewrote those tests) |
| C1 "not merged": the 59 rig's own clock, 1600's pump, the library's CTest entries off `calcu_add_test` | Unchanged; the reasons hold (the reference runs' 1 ms; the pump counts calls; no working directory) |

## 7. Candidates rejected this time

| Candidate | Reason |
|---|---|
| A "no menu bar" form of the shell (More button, Cold boot that asks first, no About on iOS) and labelled choices in the settings dialog (TI AC-29, AC-30; C59-8 AC-5, AC-7, AC-11) | Specified Part E work with no consumer before M-57/M-58 (LIB-14 D-15 puts the More button in M-57); building it now is a feature. Listed in §9 for the M-57 and M-58 designs |
| The fields the shell's value form drops (`aboutLinks`, `version`, `build`; `copyScreen` always true) | Calc-U-1600's page shows what it shows today; TI AC-26 and AC-32 need them in M-57. In §9 |
| `shellModel.js` computed in the worker (no JS mirror of `library/shell`) | A change of LIB-14 D-2, with pointer-latency questions; too large for this pass |
| The storage banner from the library's sentence | Changes Calc-U-1600's wording (a ledger row); no new user-visible change here. Question Q-4 |
| Calc-U-1600's `Dialogs.jsx`, drop and open handling, focus reclaim and the Qt `addRow` hooks into the library | Their Calc-U-59 shape (card prompts, the debug panel) is M-56/M-57's design; moving them now would fix an API on one user |
| `AudioSink`/`SoundFeed` and `SerialScriptsPart` into the library | No Part E user (the TI has no sound and no serial port); the second user is an SC61860 product |
| Sharp's serial port behind one seam (`SerialLink` + `SerialPortCapability`) | Two golden runs (`pc1500a-ce158`, `pc1600-ce158`) rest on a UART without a link; no Part E user |
| Porting the ~28 core test files off the old typers | 1 to 3 thousand lines of test edits, the golden runs' boot path and C1600-2 AC-7's "against the old typer" oracle would need a ruling; the typers left production instead (P-4) |
| CoreTests from a restored machine state, or tape round trips and long scripted runs to Longer | Changes what the tests check; the program split (P-10) brought the wall time down instead |
| `tests/LegacyExpression.hpp` (264 lines, the DAP expressions' oracle) replaced by fixed values | Less differential coverage for no structural gain |
| `typing::PauseProbe`, `KeyObserver`, `StartupSetup`'s optional loader, the harness's `wait-idle` mapping | Calc-U-59's adapters use them until M-53/M-54 (fixed item: the adapters stay) |
| `TypingProduct::typesText()` | Not derivable from the profile without a machine: Calc-U-59's `type` would answer "No machine is running." instead of "Unknown command: type." |
| `DebuggerSetup::runControlSteps` per product instead of per model | The library's tests use one product with both kinds of model (AC-19) |
| The three duplicate tests of `sharp_presets_tests.cpp` | LIB-6's qa-report row AC-50 names them for Calc-U-1600 |
| `c1600.upload`, `c1600.lcd-text` | Named by the LIB-13, LIB-7 and C1600-2 designs / used by the page's tests |
| Rename `calcu_core`/`calcu_shared` to `calcu1600_*` | A workflow change for a name only |
| `tools/check_no_old_tools.cmake` | The plan's adapter check; kept as a guard |
| `products/calc-u-1600/tools/delactions.sh` (deletes all Actions runs of "this repo", now the monorepo) | The user's tool: Q-5 |
| The library's test rigs on `LibraryParts` | They build part subsets on purpose (§5) |
| The Sharp-shaped shared help chapter 2 ("Typing text", menus every product has) | Help text a user reads; M-60 writes Calc-U-59's chapters and shares the rest then. In §9 |

## 8. Departures from approved `design.md` files

Each design that changed has a short section naming the change.
- **LIB-1** §2a: a new area `library/parts/` (its includes; included by
  `harness` and the products only).
- **LIB-11** D.2, D.3: the per-part members, `appLayerParts()`,
  `runParts()` and the run's own parts superseded by `libraryParts()` and
  `RunSetup::partsSetup()`; `addServeCommands()`; the preset commands are
  refused when the run has no vocabulary.
- **LIB-6** §3.8, §10.9a, D-33, the `amendPlan` lines: `PresetsSetup`
  takes the control at construction, no `setControl`/`setTyping`;
  `checkExpect` replaces `takesRegex`; header peripherals through
  `ReadContext::addPeripheral`.
- **LIB-7** D-22 (seven points: feed keeps the card fitted, no stale
  copy, choose and open-file fit, refusals while a card passes, eject,
  topic `holds`/`passing` instead of `armed`, AC-27's timing), §3.6 to
  §3.8, §6; `MediaSetup::control` instead of `setControl`.
- **LIB-8** §2, §6 T5, §10.7, §10.14: the plotters' reference pictures are
  Calc-U-1600's; `ac17` draws on a widened TP-4; no `PaperPart::setControl`.
- **LIB-10** §3.7, T2, §10.7, §11: the recorder is a byte source; one
  step-count limit (the session command's refusal of a count below 1 now
  reads "step takes a count from 1 to 100000."; no frontend showed the
  old sentence); one bank-window helper.
- **LIB-13** §3.4, L7, L12, the StartFile row: pass-through means fitted;
  `StartFile` in `library/opening/`.
- **LIB-14** D.5: the value forms in `library/shell/ValueForms`.
- **LIB-5**, **LIB-15**: the host-key rows (`library/qt6/HostKeys`,
  `hostKeys.js`), `library/web/vendor/`; LIB-5 P-6 section (`KeyNames`).
- **LIB-16** §3.1, §3.3, C1, D-9, L1, §12: every ReleaseIt version preset;
  the chapter list in `help.yaml` only (each chapter has `follows:`).
- **LIB-18**, **LIB-4**: P-6 sections (`machine/Names`, no null catalogue);
  `StartupSetup` takes the machine control.
- **C1600-2**: `typeBasicProgramText(Job)` outlived M-44 and is test
  support now (qa-report deviation 9).
- **C1600-4** §11, D-2 superseded: header peripherals in plan order
  without `amendSharpPlan`.

## 9. Part E designs affected

There are no separate Part E design files yet; the Calc-U-59 parts of the
C2 designs are their drafts. What each needs:
- **M-53 (LIB-8, LIB-10 59):** the TI trace recorder is a byte source over
  `drainCpuFrames` (LIB-10 §11); the PC-100C's reference picture moves to
  Calc-U-59 on its real description (LIB-8 §10.14); `typing::PauseProbe`
  goes with the TI freeze.
- **M-54 (LIB-6, C59-2, LIB-18, C59-3):** `TIAppPart` fills a
  `parts::LibraryPartsSetup` (rules, bundle, typing, papers, debugger,
  kinds, file kinds, control rules) instead of building parts; the TI
  rules implement `checkExpect` (a number; a regular expression refused,
  AC-40, AC-68) and say whether a non-number is refused when read.
- **M-55 (LIB-7 59, C59-7):** the card reader on the fitted model
  (LIB-7 D-22); AC-11's bank numbers from the fitted content; AC-10 through
  `takeGivenBack()`.
- **M-56 (LIB-13 59, C59-4):** the start file is `opening::startFile`
  (Calc-U-1600's rule: the last argument not starting with `-` that names
  a file); Calc-U-59 skips option values (`--grab x`) and checks no file:
  Q-1.
- **M-57 (LIB-4, LIB-14 59):** the shell value forms are the library's
  (`shell::toValue`); they need `aboutLinks`, `version` and a real
  `copyScreen` for TI AC-26, AC-32; the More button and the labelled
  choices are to be designed into `MenuModel`/`BarModel`/`DialogRow` (one
  model for Web portrait and iOS); host keys through `qt6::HostKeys` and
  `hostKeys.js` (Calc-U-59 drops keys held with a modifier today; LIB-5
  AC-7 decides).
- **M-58 (C59-8):** the same shell model questions for Swift;
  `SettingsPart` assumes the settings file on the session's thread, where
  C59-5 D-3 keeps Swift's on the main thread.
- **M-59 (C59-9, LIB-11 59):** the adapter check finds no harness
  own-parts path any more; Calc-U-59's entry gives the app layer's
  `LibraryParts` instead of its adapter's.
- **M-60 (LIB-16 59):** the tool reads Calc-U-59's version (3.0.0); the
  shared chapter 2 needs its Sharp-only parts moved to Calc-U-1600's part.

## 10. Questions for the user

- **Q-1 (LIB-13 AC-14 vs AC-21).** The library's start file follows
  Calc-U-1600 (an existing file, not an option). Calc-U-59's Qt6 takes the
  last argument that is not an option value, without checking it. Should
  Calc-U-59 take the library's rule in M-56 (a small visible change for a
  path that does not exist), or keep its own?
- **Q-2 (C59-7, LIB-7 D-22).** Two readings P-3 chose: Eject Card while a
  card passes takes it out of the reader and it stays fitted (C59-7 AC-25:
  "taken out as by Eject Card" while "what is fitted stays fitted"); and
  AC-27's log line is not written at a variant switch, which carries the
  fitted card over. Agreed?
- **Q-3 (help).** The shared chapter 2 describes Paste Text and the menus
  of "every product"; Calc-U-59 has no Paste Text and no menu bar on iOS.
  Move those parts to Calc-U-1600's chapter in M-60?
- **Q-4.** Calc-U-1600's Web storage banner says "open in another tab" for
  every mode but "kept", also when storage is unreadable or unavailable;
  the library has the right sentence. Fix it (a ledger row)?
- **Q-5.** `products/calc-u-1600/tools/delactions.sh` deletes the Actions
  run history of the repository it runs in, now the monorepo. Remove it?
- **Golden and reference runs:** no golden replay differed after any
  commit; the 32 Sharp reference runs (compared at the idle machine) and
  the 57 TI reference runs: §11.
- No spec change is wanted for any change made.

## 11. End of part, and line counts

- **Frequent** on `1ad3da9`: 42 of 42, 31.6 s wall (`-j8`, quiet
  machine; it was 57.3 s at `818374d`): `CalcU1600SessionTests` 31.6 s
  beside `CoreTests` 25.8 s, `CalcU59Tests` 7.9 s, the library 5.9 s.
- **Longer** (`build/groups`, both Web `dist/` built first, without
  `-j`): 32 of 33 in 670 s. All 32 Sharp reference runs
  (`CalcU1600ExampleReferences`, compared at the idle machine, C1600-4
  AC-16) and the 57 TI reference runs (`CalcU59ReferenceRuns`, with the
  accepted differences of LIB-5's M-36 list unchanged) pass; the
  Playwright runs of the library and both Web frontends, both ROM-missing
  builds, the WASM smoke, `CalcU1600HarnessTests`,
  `CalcU1600MachineStateTests`, both DAP smokes, `CalcU1600QtTests` and all
  sixteen `CalcU1600Shots` pass. `CalcU59UIRegression` (XCUITest on the
  simulator) failed after 33 s and passed unchanged on the rerun (163 s);
  the first run's log was overwritten by the rerun, so its cause is not
  known (Swift was not touched by this work beyond the library underneath
  it).
- **Local Linux CI** (GCC 13): all default jobs and `web-59`, `web-1600`
  green on the final state. A first run failed `core-1600` with a link
  error and then a crash of `CoreTests`; a fresh volume passed. The cause
  was the tool's copy: `rsync -a` kept the source files' times, so a file
  changed after an earlier run's build looked older than its stale object.
  `ci/linux/run.sh` now copies by content and gives a copied file a fresh
  time (`--checksum --no-times`).
- **GitHub CI**: GITHUB_CI_PLACEHOLDER

Line counts by C1's method (`.cpp .hpp .h .swift .js .mm .py
CMakeLists.txt .cmake`, without `node_modules`, `dist`, `build` and
`vendor`):

| | C1 after | Part D before (`818374d`) | After |
|---|---|---|---|
| `library/` | 21 706 | 62 079 | 62 997 |
| `library/` without tests | 11 761 | 33 138 | 33 662 |
| Calc-U-1600 `app/` (without `app/tests`) | 6 327 | 10 254 | 9 967 |
| Calc-U-1600 `app/tests` | 1 740 | 2 616 | 2 585 |
| Calc-U-1600 `core/` (without tests) | — | 31 690 | 30 209 |
| Calc-U-1600 `tests/` | 24 223 | 29 364 | 30 220 |
| Calc-U-59 `app/` | 10 470 | 10 456 | 10 462 |
| Calc-U-59 `tests/` | 7 226 | 8 154 | 8 154 |

The library grew by what moved into it (`LibraryParts`, the value forms,
host keys, the start file, key names and name lookups, and their tests);
Calc-U-1600's app layer and core shrank; its `tests/` grew by the old
typers that left the core (about 900 lines). Since the C1 report the
library's growth is C2 and D (the nine C2 columns and the Calc-U-1600
columns' library parts).
