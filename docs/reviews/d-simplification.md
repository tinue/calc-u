# Part D: structural simplification before the boundary to Part E

Status: survey and plan (§1 to §3), written before any code edit; §4 on
report the result.

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
