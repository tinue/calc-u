# C59-4 Calc-U-59 debug panel: design
Status: approved by the user at the Part D/E boundary (2026-11-16)

## 1. Scope

C59-4 is a product row: one column, one step.

| Column | Step | Tests from (plan §3) | Done in (plan §5.1) |
|---|---|---|---|
| Calc-U-59 | M-56 (with LIB-13 59) | AC-1–26, EC-1–5 | M-65 (AC-20's hosted copies; AC-24's Qt6 scenarios in M-57, §6) |

Every criterion becomes a test in M-56. Two parts pass later:
- **AC-20's hosted copies.** calc-u.org hosts four of the overlays
  (`calc-u-site` `59/software-files/assembly/`, listed at
  `59/Pages.jsx:1947-1950`; `docs/inventory/calc-u-site.md:48, 263, 286-287`).
  They are upgraded with the site (LIB-19, M-65), as plan §5.1 says.
  `test_stopwatch.asm` is not hosted.
- **AC-24's Qt6 scenarios.** Calc-U-59's Qt6 frontend has no GUI test
  program today. Its two scenarios are written as attach clients with
  LIB-14's scenarios (LIB-14 AC-44, M-57). The Web and Swift ones are
  written in M-56. (Plan note P-1.)

The steps this design uses:
- M-53 (LIB-10 59, `features/LIB-10-debugger-core/design.md` §12): the TI
  debug target `TMC0501`, `TILevelRules`, the run control as the TI freeze,
  `RunControlButtons` per tab, the trace session with `TITraceRecorder`,
  and the adapters L-E3 (the panel's reads on `TIDebugPanelAdapter`) and
  L-E4 (`debug-panel:` to `c59.show-debug-panel`), which M-56 removes.
- M-54 (LIB-6 59, LIB-18 59): `TIAppPart` on `parts::LibraryParts`, LIB-4's
  `ControlPart` with the product's control rules, the format 2 step
  `debug-panel:` (LIB-10 AC-32) and the build origin of a new machine.
- M-55 (C59-7): nothing of the panel. M-56 comes after it in the plan's
  order.
- LIB-13 59, same step (`features/LIB-13-opening-files/design.md` §10):
  `.asm` as a file kind of `open`.

Not here: pausing, stepping, pause-on-start and the trace as such
(LIB-10); the TI debug target (LIB-10 AC-28); where the panel sits in each
window (LIB-14 AC-33, C59-8 AC-15); the harness commands for coverage and
timings (C59-9). The Swift panel's views are here (spec, Out of Scope),
on today's Swift views; the Swift window's structure is C59-8 (M-58).

Paths in the spec are at `v2.3.0` (`Core/`, `Shared/`, `Qt6/`, `Web/`,
`App/`). Here they are in today's tree under `products/calc-u-59/`: `core/`,
`app/`, `frontends/qt6/`, `frontends/web/`, `frontends/swift/`, at
`7bdd51c`, with what M-53 changes as §12 of LIB-10's design says.

## 2. Modules and files

All under `products/calc-u-59/` unless named.

### Core

| File | Change |
|---|---|
| `core/AsmOverlayKind.hpp/.cpp` (new; product kind `c59.asm-overlay`, owner TMC0501) | `load(words) → bool`, `clear()`, `loaded()`, `enter(maxInstructions) → {entered, instructions}`, `running()` (§3.2). Over `ROM::loadOverlay`/`clearOverlay` (`core/ROM.hpp:11-36`, `core/ROM.cpp:24-37`) and `TMC0501::runDebugInjectedProgram` (`core/TMC0501.cpp:349-387`, COND set at `:376`) |
| `core/RomHeatmapKind.hpp/.cpp` (new; product kind `c59.rom-heatmap`, owner TMC0501) | counts per ROM address 0000 to 13FF, counted in the core's step while switched on; `counts()`, `reset()`, `setOn(bool)` (§3.3, D-4) |
| `core/CalculatorStateKind.hpp/.cpp` (new; product kind `c59.calculator-state`, owner TMC0501) | the frame latched at the last program step boundary with the module's latch (today `TIStepBoundary`, `core/TraceTypes.hpp:64-69`), and the bytes of the ROM's own programs (today `readROMKeycode`) (§3.4, D-3) |
| `core/MachineLogKind.hpp/.cpp` (new; product kind `c59.machine-log`, owner TMC0501) | the machine's own log lines (`DebugEvent`, `core/TraceTypes.hpp:53-56`) with their level, and the level set (§3.5) |
| `core/TIAdapters.hpp/.cpp` | `TIDebugPanelAdapter` (`:122-134`) goes, with M-53's L-E3 reads; `TIAdapters.hpp` then holds `TICardReaderAdapter`'s successor's leftovers only, or goes if M-55 emptied it |
| `core/TIMachine.cpp` | offers the four kinds; the overlay is removed at a cold boot (AC-26, §3.8) |

### App layer

| File | Change |
|---|---|
| `app/AsmOverlay.hpp/.cpp` | the reader of format 2.0 (AC-14, AC-15; §3.7) replaces `parseAsmWords` (`:13-47`); `runAsmOverlay` (`:51-59`) goes into the debug part's Run (§3.8) |
| `app/TIDebugPart.hpp/.cpp` (new) | `cu59::TIDebugPart : calcu::sessions::SessionPart`: the product topics of AC-25, the panel's commands, Run and Clear, the dumps, the log level, the heatmap reset (§3.1). It takes over from `TIAppPart` the debugger flows of `app/TIAppPart.hpp:379-414` and `app/TIAppPartFlows.cpp:725-921` that M-53 left (the snapshots, heatmap, log, dumps, overlay) |
| `app/TIAsmFileKind.hpp/.cpp` (new; listed in `app/TIFileKinds`, LIB-13 §10.4) | `AsmOverlayKind : calcu::opening::FileKind`, name `asm-overlay`, `.asm` (AC-19) |
| `app/TIDebugControlRules.hpp/.cpp` (new) | `calcu::control::ControlRules` for the product entries of AC-25: `asm-run`, `trace`, `calculator-tab`, each with its reason (§3.9) |
| `app/DebugModel.hpp/.cpp`, `DebugSnapshots.hpp/.cpp`, `DebugLog.hpp/.cpp`, `DebugText.*`, `RomHeatmap.hpp/.cpp` | read through the debug target and the kinds of §2 instead of `debugTargetOf`/`debugPanelOf` (`DebugSnapshots.cpp:96-161`, `DebugLog.cpp:69`); `RomHeatmap::ingest` from the frame ring goes (the core counts); the colours and steps (`RomHeatmap.hpp:11-25`) stay for the frontends |
| `app/TIAppPart.hpp/.cpp`, `TIAppPartFlows.cpp` | the debugger flows move to `TIDebugPart`; the interim topics `c59.debug.live`, `.cpu`, `.heatmap`, `.log`, `.asm`, `c59.show-debug-panel` (`app/TIAppPart.hpp:79-83`) and the commands `c59.clear-heatmap` … `c59.clear-asm` (`:241-247`) go; the `Change` bits `LiveChanged` … `ListingChanged` go |
| `app/TIWebPart.cpp:93, 347-348` and the `c59.web.live`, `.cpu`, `.heatmap`, `.log`, `.asm`, `.debug-font` topics | go; the page reads the product topics of AC-25 (the Web's own forms, where a payload is too large, are the debug part's request `debug-text`, D-7) |
| `app/HarnessProduct.cpp` | the `TIDebugPart` among the harness's parts; the commands of AC-22 under `ti59` (§3.10) |
| `app/TIDebuggerSetup.cpp` (M-53) | `showPanel` sets the topic `debug-view` (AC-2; L-E4 goes) |

### Frontends

| File | Change |
|---|---|
| `frontends/qt6/app/debug/DebugPanel.cpp:32-85` | the tabs from `debug-view` and the entry `calculator-tab`; the CALCULATOR tab disabled while an overlay runs (AC-18) |
| `frontends/qt6/app/debug/LiveDebugWidget.*`, `CpuInspectorWidget.*`, `RomHeatmapWidget.*`, `AsmOverlayBox.*`, `LogWidget.*` | read the product topics; the buttons send the debug part's commands; Run and TRACE enabled by the product entries |
| `frontends/qt6/app/EmulatorController.*:87-150, 293-328` | the debug relays become the product topics' |
| `frontends/qt6/app/MainWindow.cpp:43, 101-108` | Select File reads the file and sends `c59.asm-load {name, text}` |
| `frontends/web/src/Debugger.jsx:40-325`, `engine.js:50-72, 168-196` | the same, on the product topics; the ASM upload sends `c59.asm-load` |
| `frontends/swift/App/Views/DebugView.swift`, `LiveDebugView.swift`, `CPUInspectorView.swift`, `App/EmulatorViewModel.swift:93-129, 358-390, 542-548`, `Bridge/TIAppSession.h:241-284`, `.mm:597-662` | the bridge mirrors the product topics and sends the debug part's commands; `btn-step` for the CALCULATOR tab's STEP (AC-24) |

### Examples and tests

| File | Holds |
|---|---|
| `examples/assembly/Decoder.asm`, `DPT.asm`, `simple_count.asm`, `stopwatch.asm`, `examples/screentest/test_stopwatch.asm` | upgraded to 2.0 (AC-20; §3.7) |
| `examples/assembly/README.md` | describes format 2.0 and points to the help's reference chapter |
| `tests/debug_panel_tests.cpp` (new) | AC-1–13, AC-16–19, AC-21–23, AC-25, AC-26, EC-1–5; the cases of `tests/shared_debug_tests.cpp:184-287` and `part_flows_tests.cpp:194` move here |
| `tests/asm_overlay_tests.cpp` (new) | AC-14, AC-15, AC-20 |
| `frontends/web/tests/play-calculator.spec.js:461-469` | AC-24 (§6) |
| `frontends/swift/Calc-U-59UITests/ScreenshotTests.swift:126-176` | AC-24 |

## 3. Seams and interfaces

Sketches. Single-threaded on the session's thread; nothing thrown across
an interface (LIB-1 §2a).

### 3.1 The debug part and its topics (AC-1, AC-25)

`TIDebugPart` is a session part of the TI app session, after LIB-10's
`DebuggerPart` (the product's own parts follow the library's,
`library/parts/LibraryParts.hpp:40-47`). It holds what the panel shows,
computed below the frontends (TR; `frontend.md` FE-6).

| Topic (state) | Payload | Today |
|---|---|---|
| `calculator-debug` | AC-3: the data registers not zero (with H00 to H03 on the TI-58C), the partition, the steps around the current one and its mark (next or running), the pending operations' registers, T and X, the flags, the display format and the angle mode, the program's source, SCOM, the return addresses | `c59.debug.live` (`TIDebugLiveState`, `app/TIAppPart.hpp:143-151`) |
| `program-listing` | AC-4: the listing of the program that runs, its source and the current index; empty while the machine runs | `TIDebugLiveState::listing` |
| `cpu-debug` | AC-6: the instructions with address, text and registers before; the newest 32 while running, up to 1024 and the next while paused | `c59.debug.cpu` |
| `heatmap` | AC-8: the counts of 0000 to 13FF | `c59.debug.heatmap` |
| `asm-overlay` | the file's name, the number of instructions, the status line, whether the overlay runs | `c59.debug.asm` |
| `debug-view` | the tab `debug-panel:` asked for last: `calculator`, `cpu` or `log` | the stream `c59.show-debug-panel` |

- The topic names are the spec's (AC-25), as given there. The harness
  reads them with `state` (D-1).
- **What is computed when.** `calculator-debug` and `cpu-debug` are
  computed only while a frontend shows that tab, as today
  (`c59.live-debug`, `c59.cpu-debug` stay as the commands that say so,
  `app/TIAppPart.hpp:227-228`); the harness's `state` computes them once
  on demand. `program-listing` is built when the run state turns paused,
  and emptied when it runs (AC-4). The rates of today stay: the frame's 60
  Hz for the CALCULATOR snapshot, 10 Hz for the heatmap, 1 Hz for the log
  text (`app/TIAppPart.hpp:466-469`).
- The log itself is LIB-3's `log` topic (AC-1, AC-9); the LOG tab keeps its
  last 2000 lines in the frontend's view (`app/DebugLog.hpp:13-15`). The
  run state and the trace are LIB-10's `run-state` and `trace`.

### 3.2 The ASM overlay as a product kind (AC-13)

```cpp
namespace cu59 {
struct AsmOverlay : calcu::machine::Capability {             // kind "c59.asm-overlay", "TMC0501/c59.asm-overlay"
    static constexpr std::size_t kMaxInstructions = 2048;    // 1800 to 1FFF (core/ROM.hpp:11-13)
    virtual bool load(const std::vector<std::uint16_t>& words) = 0;   // false: more than 2048
    virtual void clear() = 0;
    virtual std::size_t loaded() const = 0;
    struct Entry { bool entered; std::uint32_t instructions; };
    virtual Entry enter(std::uint32_t maxInstructions) = 0;  // COND set, EXT/PREG asserted (TMC0501.cpp:355-387)
    virtual bool running() const = 0;                         // from enter until a reset, cold boot or clear
};
}
```

`running()` is the core's: true from a successful `enter` until the next
`Machine::reset()`, `coldBoot()` or `clear()`. A FREEZE does not end it
(AC-18; ledger 59-21; today `app/TIAppPartFlows.cpp:758-764` clears it).

### 3.3 The heatmap (AC-8)

The kind `c59.rom-heatmap` counts, while it is on, each instruction the
processor carries out at a ROM address from 0000 to 13FF
(`RomHeatmap::kCodeWords`, `app/RomHeatmap.hpp:28`). It is switched on
while the CPU tab is shown, and is set to zero by:
- the command `ti59 heatmap reset` / `c59.heatmap-reset`, which the tab's
  CLR sends and a frontend sends when the panel comes into view;
- a Reset (`machineRestarted`) and a model switch (a new machine starts at
  zero).

It is Calc-U-59's own count, apart from LIB-10's `profile`, which the
harness switches and reads (AC-8). An instruction is counted once (AC-23,
`tests/shared_debug_tests.cpp:220`): the core counts as it carries it out,
so no ring is read twice.

### 3.4 The CALCULATOR and CPU tabs (AC-3, AC-4, AC-6)

- **Through the debug target (AC-3).** The data registers, SCOM, the
  program steps and the partition are read through `TMC0501`'s spaces
  `ram`, `scom` and `program` (LIB-10 §12.4); the registers T and X and
  the pending operations through its registers and `scom`. The state "at
  the last program step boundary" while the machine runs is the frame
  latched there, which `c59.calculator-state` gives (D-3), in the
  target's register names. While paused at a boundary, the target's
  registers are that state.
- **Where the program comes from (AC-4).** The calculator's memory
  (`program`), the ROM's own programs (`c59.calculator-state`'s bytes, today
  `readROMKeycode`, `app/DebugSnapshots.cpp:96`) or the module's image
  (`app/ModuleImage.*`, from the module file as today).
- **The CPU tab (AC-6)** reads the target's history (the newest first):
  32 while running, up to 1024 while paused, and the next instruction from
  `nextAddress` and `decode` (`app/DebugSnapshots.cpp:108-153`).

### 3.5 The LOG tab (AC-9 to AC-12)

- **Dumps (AC-10).** `c59.dump {what: scom|memory|prog|vars}` writes the
  lines of `dumpScom`, `dumpMemory`, `dumpProg`, `dumpVars`
  (`app/DebugLog.hpp:36-45`, today's layout) into LIB-3's log; the harness's
  `ti59 dump …` replies with the same lines (AC-22).
- **Levels (AC-11).** `c59.log-level {level: off|info|debug}` sets the
  level of `c59.machine-log`; the button cycles through the three and shows
  the one that holds, read from the field `logLevel` of `debug-view`
  (D-6). The machine's lines go into LIB-3's log at their level;
  the product's own lines (a Reset, a preset's steps, the dumps) at every
  level, as today (`app/TIAppPartFlows.cpp:784-806`).
- **The view (AC-9).** The tab shows the last 2000 lines of the log, its
  text can be copied, and its clear button empties the tab's view only.
- **TRACE (AC-12).** The button sends LIB-10's `trace {button: true}` and
  `trace {off: true}`; it shows the `trace` topic, whoever started the
  trace; the product entry `trace` says whether it can be used (§3.9). In
  the Web the tab lists the traces that ended (LIB-10 §12, D-25).

### 3.6 The buttons of the CALCULATOR and CPU tabs (AC-5, AC-7, EC-3)

FREEZE, STEP and RESUME are LIB-10's run-control buttons (M-53, LIB-10
§12.3) for `program` and `cpu`. FREEZE ON START sends `pause-on-start
{level}`, then reads ARMED; ARMED sends `pause-on-start {off}`. FREEZE and
FREEZE ON START can be used only while the machine runs and that tab's
start is not armed (AC-5): the run state's `armed` field. A tab's STEP
cannot be used while the other level owns the pause (EC-3, `ButtonRule`).
Labels and short forms ("F.START", "DEBUG") as today (AC-21).

### 3.7 The ASM overlay format 2.0 (AC-14, AC-15, AC-19, AC-20)

`cu59::readAsmOverlay(std::string_view text) → std::variant<std::vector<std::uint16_t>, std::string /*sentence*/>`,
by the rules of AC-14. The refusals of AC-15, one sentence each (D-5):

| Case | Sentence |
|---|---|
| outdated (no first line `Calc-U-59 ASM 2.0`, or another version) | "`<name>` is an ASM overlay in an older format. This release reads the ASM overlay format version 2.0, described in the help's reference chapter." |
| no `HEX:` line | "`<name>` has no `HEX:` line." |
| nothing after it | "`<name>` has no instructions after `HEX:`." |
| a run not a multiple of four digits | "`<name>`, line `<n>`: `<run>` is not a multiple of four hexadecimal digits." |
| an instruction above 1FFF | "`<name>`, line `<n>`: `<value>` is above 1FFF." |
| anything else after `HEX:` | "`<name>`, line `<n>`: `<word>` is neither hexadecimal digits nor a comment." |
| more than 2048 instructions | "`<name>` holds `<n>` instructions; at most 2048 fit from 1800 to 1FFF." |

- **Select File (AC-16)** reads the file in the frontend and sends
  `c59.asm-load {name, text}`; a refused file is a notice and the
  instructions loaded before stay (EC-2). A file read is put over the ROM
  at once.
- **The file kind (AC-19).** `AsmOverlayKind` takes a name ending in
  `.asm`, any case, and does what `c59.asm-load` does. Opened before the
  first machine is built, it is kept and opened afterwards (EC-5; LIB-13
  AC-7).
- **The five files (AC-20).** Each gets the first line `Calc-U-59 ASM 2.0`;
  what was before `HEX:` stays for the reader; after `HEX:` every comment
  starts with `;`; a run of fewer than four digits or a word after `HEX:`
  is turned into a comment where it was meant as one. Each must give the
  same instructions as 2.3.0's reader gave it (`tests/shared_debug_tests.cpp:238-259`),
  and the comments that name a key, a tool or a format that changed are
  corrected. `examples/assembly/README.md` describes 2.0.

### 3.8 Run, Clear and the overlay's life (AC-16 to AC-18, AC-26, EC-1)

- **Run** (`c59.asm-run`; the product entry `asm-run`, §3.9) can be used
  when instructions are loaded, while the machine runs or is paused, not
  while an operation runs. It is a short operation:
  1. The machine goes to the next program step boundary (LIB-10's
     `program` rule, at most 50000 weighted cycles).
  2. `enter(8192)`: the machine is made to go on at 1800 with COND set.
  3. Not entered: the status "ASM entry failed after n step(s)." and a
     notice with the same words.
  4. Entered: if pause-on-start `cpu` is armed, the run control pauses the
     machine at 1800 with the reason pause-on-start (the event of AC-17,
     through `TILevelRules::startFiresAfter` on the overlay's entry mark,
     D-8); otherwise the overlay goes on. The status reads "ASM entered at
     0x1800 (n step(s))." as today.
- **Run without instructions (EC-1):** the status "No ASM overlay loaded."
  and nothing else.
- **Clear** (`c59.asm-clear`) takes the instructions away, and resets the
  machine when they were running (today `app/TIAppPartFlows.cpp:911-921`).
- **While it runs (AC-18):** the `calculator-tab` entry says no, with the
  reason; the panel shows the CPU tab; FREEZE, STEP and RESUME of the CPU
  tab work on its instructions, and it still runs afterwards (§3.2).
- **Reset** ends the run; the instructions stay over the ROM and Run enters
  them again (AC-18). **Cold boot** and **a new machine** (a preset, a model
  switch) remove the overlay: its run ends, and Run has nothing to run
  (AC-26; D-2). Not kept when the product quits.

### 3.9 The product entries (AC-25)

`TIDebugControlRules` adds to LIB-4's control state (LIB-4 AC-14):

| Entry | Can be used | Reason when not |
|---|---|---|
| `asm-run` | instructions are loaded and no operation runs | "No ASM overlay loaded." / LIB-4's busy reason |
| `trace` | a trace folder can be written, or the host keeps traces (the Web) | "The trace folder cannot be written." |
| `calculator-tab` | no overlay runs | "An ASM overlay runs." |

### 3.10 The harness commands (AC-22)

Under `ti59` (C59-9 owns the word; these are this feature's):

| Command | Reply |
|---|---|
| `ti59 asm load <file>` | done, with the number of instructions; refused with §3.7's sentence |
| `ti59 asm run` | done, with the status line; refused when nothing is loaded |
| `ti59 asm clear` | done |
| `ti59 dump scom\|memory\|prog\|vars` | the dump's lines |
| `ti59 log-level off\|info\|debug` | done |
| `ti59 heatmap reset` | done |

No preset step for any of them (AC-22).

### 3.11 The frontends (AC-1, AC-21, EC-4)

- **Qt6** keeps its panel (`frontends/qt6/app/debug/`); **Web** its
  `Debugger.jsx`; **Swift** its `DebugView`, `LiveDebugView`,
  `CPUInspectorView` (TR: the panel is per frontend). Each draws the
  product topics; none computes what it shows.
- **The panel's place** (AC-25's last sentence): Qt6 the splitter's third
  pane (`frontends/qt6/app/MainWindow.cpp:27-38`), the Web's wide layout
  (`PlayCalculator.jsx:147-148`), Swift where its layout has room
  (`CalculatorView.swift:238, 261, 453`); unchanged until LIB-14 59 and
  C59-8.
- **EC-4.** A frontend that does not show the panel keeps the last
  `debug-view` and shows that tab when the panel is shown; since
  `debug-view` is a state topic, a frontend that starts to listen gets it
  too (today `Debugger.jsx:285-289`).

## 4. Product side: what is replaced

| Today | After M-56 |
|---|---|
| `TIDebugPanelAdapter` (`core/TIAdapters.hpp:122-134`) and M-53's L-E3 reads | the product kinds `c59.asm-overlay`, `c59.rom-heatmap`, `c59.calculator-state`, `c59.machine-log` and the debug target |
| the heatmap from the frame ring (`app/RomHeatmap.hpp:1-40`, `DebugModel::tick`) | counted in the core (§3.3) |
| eight interim topics and the `c59.web.*` debug topics | the six product topics of AC-25 |
| `parseAsmWords` (lenient: runs under four digits passed over, values cut to 13 bits) | the 2.0 reader (AC-14, AC-15) |
| an overlay kept through a cold boot, a model switch and a preset (`app/TIAppPartFlows.cpp:73-77, 89-96`); a FREEZE ends its run (`:758-764`) | AC-18, AC-26 |
| `.asm` only by the box's dialogs | also a file kind of `open` (AC-19) |
| no harness commands for the panel | AC-22 |

## 5. Adapters

Removed in M-56:

| Adapter | File | Since |
|---|---|---|
| `TIDebugPanelAdapter` with L-E3 | `core/TIAdapters.*` | C59-1 §5; LIB-10 §12.7 |
| L-E4 `debug-panel:` to `c59.show-debug-panel` | `app/TIDebuggerSetup.cpp` | LIB-10 §12.7 |
| the interim topics `c59.debug.*`, `c59.show-debug-panel` | `app/TIAppPart.*` | LIB-3 59 §5 |
| `.asm` refused by the drop rule (LIB-13 AC-13) | `app/TIAppPartFlows.cpp:599-618` | LIB-13 §5 |

None is added.

## 6. Tasks

Tests before code in each task. Unless named, cases are in
`tests/debug_panel_tests.cpp`, in `CalcU59Tests` (Frequent), on
`tests/SessionRig.hpp` with the real ROMs (`SKIP_UNLESS`). Web cases in
`CalcU59WebPlaywright` (Longer), titled with the ID (ADR 0032). Case names
`c59_4_acN_…` / `c59_4_ecN_…`.

| Task | What | ACs/ECs | Test (file: cases) |
|---|---|---|---|
| T1 | The topics | AC-1, AC-25 | `c59_4_ac25_six_product_topics_read_by_state`, `_entries_asm_run_trace_calculator_tab_with_reasons`, `_cpu_and_calculator_computed_while_shown`; `c59_4_ac1_panel_fed_by_product_and_generic_topics` |
| T2 | `debug-panel:` | AC-2, EC-4 | `c59_4_ac2_view_shown`, `_calculator_passed_over_while_overlay_runs`; `c59_4_ec4_view_kept_until_shown` (a late listener gets it) |
| T3 | CALCULATOR tab | AC-3, AC-4 | `c59_4_ac3_shows_registers_partition_steps_flags_source_scom_returns` (the case of `shared_debug_tests.cpp:184`), `_ti58c_shows_h00_to_h03`, `_read_through_the_target`; `c59_4_ac4_listing_while_paused_memory_rom_module`, `_current_step_marked` |
| T4 | Buttons | AC-5, AC-7, EC-3 | `c59_4_ac5_freeze_on_start_arms_then_armed_disarms`, `_freeze_and_start_only_while_running_and_not_armed`; `c59_4_ac7_cpu_tab_buttons_for_cpu`; `c59_4_ec3_step_of_the_other_level_cannot_be_used_preset_step_is_carried_out` |
| T5 | CPU tab | AC-6 | `c59_4_ac6_newest_32_running_up_to_1024_and_next_paused` |
| T6 | Heatmap | AC-8 | `c59_4_ac8_counts_each_rom_address_once` (the case of `shared_debug_tests.cpp:220`), `_reset_by_command_reset_and_model_switch`, `_apart_from_the_profile` |
| T7 | LOG tab | AC-9, AC-10, AC-11, AC-12 | `c59_4_ac9_last_2000_lines_in_pages` (`shared_debug_tests.cpp:270`), `_clear_empties_the_view_not_the_log`; `c59_4_ac10_dumps_in_todays_layout` (`:287`); `c59_4_ac11_levels_off_info_debug`, `_debug_logs_each_register_and_memory_write`, `_product_lines_at_every_level`; `c59_4_ac12_trace_button_starts_and_ends_the_button_trace`, `_shows_a_trace_whoever_started_it`, `_cannot_be_used_without_a_writable_folder` |
| T8 | Overlay kind | AC-13 | `c59_4_ac13_up_to_2048_over_1800_to_1fff`, `_taken_away`, `_entered_at_1800` |
| T9 | Format 2.0 | AC-14, AC-15 | `asm_overlay_tests.cpp`: `c59_4_ac14_first_line_version`, `_prose_before_hex_not_read`, `_semicolon_comments`, `_runs_of_four_eight_twelve`, `_0x_prefix_either_case`, `_crlf`, `_first_at_1800`; `c59_4_ac15_` one case per row of §3.7's table |
| T10 | The box | AC-16, EC-1, EC-2 | `c59_4_ac16_select_file_loads_at_once`, `_run_only_with_instructions`, `_clear_resets_when_running`; `c59_4_ec1_run_without_instructions_status`; `c59_4_ec2_refused_file_keeps_loaded` |
| T11 | Run | AC-17 | `c59_4_ac17_enters_at_1800_with_cond` (`DPT.asm`), `_fails_after_8192_with_status_and_notice`, `_armed_cpu_start_pauses_at_1800`, `_while_paused_not_while_operation` |
| T12 | The overlay's life | AC-18, AC-26 | `c59_4_ac18_calculator_tab_closed_cpu_shown`, `_freeze_step_resume_inside_the_overlay_it_still_runs`, `_reset_ends_run_keeps_instructions_run_again`; `c59_4_ac26_cold_boot_removes`, `_new_machine_by_preset_and_model_switch_removes` (replacing `part_flows_tests.cpp:194` `part_asm_overlay`) |
| T13 | Opening | AC-19, EC-5 | `c59_4_ac19_asm_opened_as_by_select_file` (`open` by drop, start and harness); `c59_4_ec5_opened_before_first_machine_kept` |
| T14 | The five files | AC-20 | `asm_overlay_tests.cpp`: `c59_4_ac20_five_overlays_in_2_0_give_2_3_0s_instructions` (the instructions of 2.3.0's reader, recorded once before the reader changes as `tests/fixtures/asm/*.words`), `_dpt_runs_with_cond` |
| T15 | Every frontend | AC-21 | Web: `C59-4 AC-21 the three tabs, labels and short forms`; Swift: `Calc-U-59UITests` `testC59_4_AC21TabsAndLabels` (Longer); Qt6: checked by hand in M-56, the attach scenario in M-57 |
| T16 | Harness | AC-22 | `tests/harness_tests.cpp`: `c59_4_ac22_asm_load_run_clear`, `_dump_replies_lines`, `_log_level`, `_heatmap_reset`, `_no_preset_steps_for_them` |
| T17 | Today's tests | AC-23 | the cases of `shared_debug_tests.cpp:184-287` named in T3, T6, T7, T9; `asm_overlay_parser` without its two lenient cases |
| T18 | GUI scenarios | AC-24 | Web: `play-calculator.spec.js:461` renamed `C59-4 AC-5 FREEZE, STEP and RESUME on the CALCULATOR tab` (it now presses STEP and checks the next step), new `C59-4 AC-16 an overlay is chosen and run and the CPU tab shows it`. Swift: `ScreenshotTests.swift:126-176` as `testC59_4_AC16AsmOverlay`, new `testC59_4_AC5CalculatorFreezeStepResume` (`btn-step`). Qt6: both as attach scenarios in M-57 (P-1) |
| R1 | Reference runs | §8 | the 57 reference runs, the golden runs |

Coverage: AC-1 to AC-26 and EC-1 to EC-5 have a task; AC-20's hosted
copies pass in M-65, AC-24's Qt6 scenarios in M-57.

## 7. Tools and agent knowledge

**Tools.** The harness gets the commands of §3.10. `tools/headless/` reads
no panel data (its frames go through LIB-10's L-E3 until M-56; from M-56
through the debug target's history). `tools/agentic/asm59.py` (stays a
script on the harness, plan M-59) writes overlays: it writes format 2.0
from M-56.

**Agent knowledge changed in M-56.**

| File | Change |
|---|---|
| `reference/DebugAPI.md:12-16, 63, 123, 162-211, 257, 281, 329-378` | the panel's data as product topics; the ASM overlay format 2.0; `DebugModel`, `RomHeatmap`, `DebugLog`, `AsmOverlay` as they are after M-56 |
| `reference/SharedArchitecture.md:50-56, 101, 127, 526, 533` | `TIDebugPart`, the product kinds |
| `reference/FrontendParity.md:66-75` | the panel's rows on the topics |
| `reference/AppArchitecture.md:129`, `NewGUIGuide.md:64` | `RomHeatmap` |
| skills `calc-u-59-shared` (`SKILL.md:3`, the debugger model), `calc-u-59-qt` (`:68-70`, `AsmOverlayBox`, `RomHeatmapWidget`), `calc-u-59-web`, `calc-u-59-swift`, `calc-u-59-uitesting` (the renamed UITests, `btn-step`) | the topics and commands |
| `docs/glossary-mapping.md` | rows "Heatmap", "Dump" (glossary terms) and "ASM overlay" with the code names after M-56 |
| `docs/test-groups.md` | `debug_panel_tests.cpp`, `asm_overlay_tests.cpp`, the renamed Web and Swift cases |

The ASM overlay format is described in the help's reference chapter with
LIB-16 59 (M-60, LIB-16 AC-10); until then `examples/assembly/README.md`
describes it.

**Ledger.** Rows 59-1 (the ASM format 2.0, an overlay removed by a cold
boot or a new machine, `.asm` opened like any file) and 59-21 become
visible in M-56.

## 8. Golden runs, reference runs and risks

- **Golden runs** do not use the overlay or the panel; the heatmap counts
  in the core's step only while it is on. Every `.golden` must replay
  unchanged.
- **Reference runs** (57). The debug examples (`debug_*`,
  `screentest_test_cdebug`) show the panel through KEYSTROKES `DebugPanel:`
  (format 2 `debug-panel:` from M-54); their `.txt` files hold no panel
  data and must stay as they are.
- **The five overlays' instructions** are recorded with today's reader
  before it changes (T14); the 2.0 files must give the same words. A
  difference is a finding.
- **Risk: the heatmap's cost.** Counting in the core's step is one
  increment per instruction while on, cheaper than today's ring read. Off
  while the CPU tab is hidden.
- **Risk: the overlay's run at a Reset.** Today `beginReset` clears the run
  flag in the app; after M-56 it is the core's (§3.2). If the core cannot
  tell a Reset from the overlay's own jumps, the debug part ends the run at
  `machineRestarted`.
- **Risk: the Web payloads.** `cpu-debug` with 1024 entries and their
  registers is large for every change; the Web gets a compact form through
  a request (D-7).

## 9. Decided (best guess)

- **D-1 (AC-25, topic names).** The spec's names as given, without a
  product prefix: they are the contract the harness reads (`state
  calculator-debug`).
- **D-2 (AC-18 against AC-26).** A Reset ends the overlay's run and keeps
  its instructions (AC-18); a Cold boot and a new machine remove them
  (AC-26). AC-18's "given the model is switched, they are put over the new
  machine's ROM" is replaced by AC-26, the later ruling (spec decision log,
  2026-11-13, B-18; ledger 59-1).
- **D-3 (AC-3, "through the debug target").** Everything the generic
  target gives is read through it. The frame latched at the last program
  step boundary, and the bytes of the ROM's own programs, are not a debug
  target's (the target gives the state between the next two instructions,
  LIB-10 AC-3), so they are the product kind `c59.calculator-state`.
- **D-4 (AC-8, the heatmap's count).** Counted in the core per instruction
  carried out ("taken through the debug target as the profile is": by the
  addresses the target carries out), as a product kind (`capabilities.md`,
  product kinds: ROM heatmap). Not LIB-10's `Profile`, which the harness
  switches.
- **D-5 (AC-15, sentences).** The sentences of §3.7, each with the file's
  name and, after `HEX:`, the line.
- **D-6 (AC-11, where the level shows).** The level is a field `logLevel`
  of the topic `debug-view`: one small state topic for the panel's own
  settings, rather than a seventh topic the spec does not name.
- **D-7 (the Web's forms).** Where today's `c59.web.*` topics carry a
  compact form (`app/TIWebPart.cpp`), the page asks the debug part's
  request `debug-text {topic}` for it; the topics stay the contract.
- **D-8 (AC-17, the start at 1800).** The entry into the overlay is an
  event of `TILevelRules`: the mark "entered at 1800" set by the overlay
  kind on the first instruction there, on which an armed `cpu` start
  fires.
- **D-9 (AC-20, the upgrade).** A run of fewer than four digits or a word
  after `HEX:` in today's files becomes a `;` comment where it was text; no
  instruction changes (T14).
- **D-10 (Select File).** The frontend reads the file and sends its text,
  as today (`c59.load-asm {text, name}`, renamed `c59.asm-load`), since the
  Web has no path.

## 10. Plan notes

- **P-1 (§5.1, C59-4 row).** "Tested in: M-56 (AC-20's hosted overlays
  M-65)" becomes "M-56 (AC-20's hosted overlays M-65; AC-24's Qt6
  scenarios M-57)". Plan M-57, "Tests from": add C59-4 AC-24 (Qt6).
- **P-2 (M-56 "Waits for").** Add M-54 (`LibraryParts` and LIB-4's control
  state in the TI session; the format 2 `debug-panel:`).
- **P-3 (M-56 "Knowledge, ledger").** Add `reference/SharedArchitecture.md`,
  `AppArchitecture.md`, `NewGUIGuide.md`, the `calc-u-59-shared`,
  `-swift` and `-uitesting` skills, `examples/assembly/README.md` and
  `tools/agentic/asm59.py`.

## 11. Questions for the user

None that split this design.
