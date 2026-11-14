# C59-1 TI machines on the machine interface: design
Status: approved by the user at the Part B/C1 boundary (2026-11-14)

## 1. Scope

| Column | Step | Tests from | Done in (plan §5.1) |
|---|---|---|---|
| Product row C59-1 (59) | M-19 | C59-1 AC-1–19, EC-1–4 | M-19 |
| LIB-1 Calc-U-59 column, M-19 part | M-19 | LIB-1 AC-26–31, AC-33 without peripherals (LIB-1 design §1, P1, P2) | M-25 (with C59-6: AC-32, AC-33 with peripherals) |

- Waits for M-18 (LIB-1 L), M-8 (FS 2 golden runs, FS 7 Reset and the
  partition), M-3 (FS 5 key readiness, FS 11 TI-58C constant memory).
  FS results used here: idle is not a state of its own (key readiness
  only); 2.3.0's Reset loses the partition (row 59-22 stands); 2.3.0
  starts a TI-58C with the start-up first and the RAM after (row 59-15
  stands, M-54).
- Every C59-1 criterion is tested in M-19. Nothing of C59-1 lands later.
  Criteria that rest on later rows hold here at the machine and become
  visible to the user in those rows: AC-11 (row 59-13, the key sequencer
  side: LIB-5, M-36), AC-6's constant memory at build (C59-3, M-54,
  row 59-15).
- Not here: the TI catalogue's peripherals (C59-6, M-25), attach/detach
  (LIB-2), the media place's typed interface (LIB-7/C59-7), the debug
  target interface (LIB-10), sessions and pacing (LIB-3, LIB-17).

## 2. Modules and files

No new library area. Product code, named in the product's style (core
classes in the global namespace with a `TI` prefix, app code in `cu59`).
New core files stay flat in `core/`, because
`frontends/swift/build_xcframework.sh:67-75` copies only `core/*.hpp`.

New:

| File | Holds |
|---|---|
| `products/calc-u-59/core/TIMachine.hpp/.cpp` | `TIMachine final : calcu::machine::Machine` (§3.1); owns a `TI59Machine`, the monotonic tick base, the Reset restore, the adapters' objects |
| `products/calc-u-59/core/TICatalogue.hpp/.cpp` | `tiCatalogue()` (the three machine descriptors), `TIMachineBuilder`, `TIConstantMemory : ProductData` (§3.2) |
| `products/calc-u-59/core/TIKeys.hpp/.cpp` | the 45 keys: key ID, other spellings (`docs/formats/preset.md` §8.1), row, column; K-line bits `{0,1,2,3,5,6}` from `app/KeyMatrix.hpp:12` |
| `products/calc-u-59/core/TISegments.hpp` | the glyph rules of `app/SevenSegment.hpp:15-52` (`kDigitSegments`, minus, degree, quote, apostrophe, `segmentsFor`) and `toDisplayOutput(const DisplaySnapshot&)` |
| `products/calc-u-59/core/TIRomRanges.hpp` | `PcRange` and the per-model `scanLoop`, `idleAndKeyDetect`, `cardWait` of `app/MachineModelInfo.hpp:23-105`, now core-internal |
| `products/calc-u-59/core/TIRomText.hpp/.cpp` | Datamath text decoding of the ROM and CONST-K files, moved from `app/RomLoader.cpp` (LIB-1 D-4) |
| `products/calc-u-59/core/TIPrinterTypes.hpp` | `PrinterCodeLine`, moved out of `core/TMC0501.hpp:22-27` so the C59-6 adapter header need not include `TMC0501.hpp` |
| `products/calc-u-59/core/TIAdapters.hpp/.cpp` | the core-side adapters of §5 and their accessors |
| `products/calc-u-59/core/TIMachineTesting.hpp` | test-only reads inside the core: key lines, raw RAM, PC (Technical Requirements: "read by the test inside the core") |
| `products/calc-u-59/app/MachineGuard.hpp` | the app-layer lock adapter of §5 (D-1) |
| `products/calc-u-59/app/KeyIds.hpp` | matrix code (state files, presets) ↔ `KeyId`, from the descriptor; replaces `app/KeyMatrix.hpp` |
| `products/calc-u-59/tests/machine_tests.cpp` | C59-1 AC-1–3, 6–19, EC-1–4 |
| `products/calc-u-59/tests/descriptor_tests.cpp` | LIB-1 AC-26–31, 33 (LIB-1 design P1, P2) |
| `products/calc-u-59/tests/check_core_seam.sh` | AC-17 (CTest, Frequent) |

Changed (main ones):

| File | Change |
|---|---|
| `core/TI59Machine.hpp/.cpp` | `m_keyMutex` and every `lock_guard` go; becomes core-internal (only `TIMachine` and the adapters use it) |
| `core/TMC0501.hpp/.cpp` | `m_displayMutex`, `m_prnMutex`, `m_traceMutex` go; the atomics (`m_traceFlags`, `m_cSteps`, `m_pollSteps`, `m_elapsedTicks`) become plain members; `StepBoundary` moves to `TraceTypes.hpp` (alias kept) |
| `core/MachineVariant.hpp` | `resonatorHz` becomes descriptor data; `instructionHz` goes; the enum stays core-internal |
| `app/MachineModelInfo.hpp` | no `core/` include; `toVariant`, `instructionHz`, the three `PcRange`s go; `Model` stays as the app's enum with `modelId(Model)` / `modelFrom(ModelId)`; `romFiles`/`constFiles` stay (LIB-1's adapter, M-24); `hasConstantMemory`, `hasLargeMemory`, `extraRegisterCount`, `maxStep()` stay as file-format facts |
| `app/EmulatorSession.*` | owns `std::unique_ptr<calcu::machine::Machine>` from `calcu::machine::build()`; `machine()` returns `calcu::machine::Machine&`; `sharedMachine()` goes; adapters reached by §3.4 |
| `app/StateApplier.*` | `softReset` → `Machine::reset()`, `cleanReset` → `Machine::coldBoot()`; `applyState`, `applyReload`, `powerOn` on `Machine&` plus the C59-2 / C59-3 adapters (§5) |
| `app/Pacing.*`, `app/EmulationRunner.*` | `runBatch` calls `advance()` once per batch; carry and target in ticks (×32, same random sequence); stop and polled-hardware come from adapters |
| `app/FreezeController.*` | implements `TIStepObserver` (§3.4); its scan-loop test uses the `inScanLoop` flag the core passes |
| `app/KeySequencer.*` | `Host::pc()` + `m_scanLoop` → `Host::isKeyReady()`; keys as `KeyId` |
| `app/FrameSampler.*`, `app/DisplayPov.*` | read `DisplayOutput`; seconds from `elapsed()` differences; `LedFrame::digits/ctrl` → `segments[12]` |
| `app/SevenSegment.hpp` | keeps `LedCell`/`ledCell` and `kSegmentsC` (drawing); `ledCell(i, segments, level, calcOpacity)` |
| `app/RomLoader.*` | reads the five files into `RomImage`s; module images stay here (C59-6, LIB-12) |
| `app/SessionController.cpp`, `app/DebugModel.*`, `app/DebugSnapshots.*`, `app/DebugLog.*`, `app/AsmOverlay.*`, `app/TraceWriter.*`, `app/RomHeatmap.*`, `app/CardFile.cpp`, `app/ConstantMemory.hpp` | move onto `Machine` and the §5 adapters; no `core/TI59Machine.hpp`, `TMC0501.hpp` or `MachineVariant.hpp` include |
| `frontends/qt6/app/LedRenderer.cpp:71`, `frontends/web/bindings.cpp:483-487`, `frontends/swift/Bridge/TITypes.h`, `TISessionController.mm:237-241`, `TISharedTables.h/.mm`, `App/EmulatorViewModel.swift:387-389`, `App/Views/LEDDisplayView.swift:28` | the display frame carries segments instead of digit/control nibbles |
| `frontends/qt6/CMakeLists.txt:72-77, 126` | the new core sources in `CORE_SOURCES`; link `calcu_library` (the Qt target compiles core sources itself and links no library today) |
| `frontends/swift/build_xcframework.sh:49-54, 67-75` | the XCFramework rule of LIB-1 design §2a, first needed here: merge `libcalcu_library.a` into `libCalcU59.a` and copy `library/**/*.hpp` |
| `products/calc-u-59/CMakeLists.txt` | new core sources; `calcu59_core` uses `calcu::machine` through `calcu_library` (linked today); `Threads::Threads` stays on `calcu59_shared` only (the runner, until M-33); under Emscripten `-fexceptions` becomes `-fwasm-exceptions` on `calcu59_core`, `calcu59_shared` and `calcu59_web` (compile and link, lines 76-86, and the comment above them; LIB-1 design §2a, D-6); CTest `CalcU59CoreSeam`, `CalcU59HeadlessSmoke` |
| `tests/core_tests.cpp`, `tests/golden_runs_tests.cpp`, `tests/TestRoms.hpp`, `tests/shared_*_tests.cpp`, `tests/reference_runs_tests.cpp` | through `Machine` and the adapters (§6) |
| `tools/headless/capi.cpp`, `ti59.py`, `README.md`; `tools/agentic/ti59file.py` | §7 |

## 3. Seams and interfaces

### 3.1 The TI machine (`core/TIMachine.hpp`)

```cpp
class TIMachine final : public calcu::machine::Machine {
public:
  TIMachine(const calcu::machine::MachineDescriptor&, MachineVariant, TIRoms decoded,
            std::shared_ptr<const TIConstantMemory> content);           // AC-6 build order, AC-7
  const MachineDescriptor& descriptor() const override;                 // LIB-1 AC-26–31
  std::optional<VariantId> variant() const override { return {}; }      // LIB-1 AC-26: no variant
  Ticks advance(Ticks atLeast) override;                                // AC-1, AC-2, EC-1
  Ticks tickCount() const override;                                     // AC-3
  void reset() override; void powerCycle() override; void coldBoot() override;   // AC-6, AC-8, AC-19
  bool isKeyReady() const override;                                     // AC-18
  KeyCall keyDown(const KeyId&) override; KeyCall keyUp(const KeyId&) override;  // AC-9–12, EC-3
  DisplayOutput readDisplay() override;                                 // AC-13–15, EC-2
  std::size_t takeSound(std::span<std::int16_t>) override { return 0; } // AC-16
  Plugged plugged() const override;                                     // M-19: nothing plugged (C59-6: M-25)
  std::vector<CapabilityEntry> capabilities() const override;           // none in M-19; the card reader from M-55 (LIB-1 D-1)
  Capability* capability(const CapabilityId&) override;
  std::vector<DebugTargetId> debugTargets() const override;             // {"TMC0501"}
private:
  TI59Machine m_hw;  Ticks m_ticksBeforeReset = 0;
  std::optional<SavedMemory> m_restore;           // AC-19: RAM and partition until key readiness
  /* adapter objects of §5 */
};
```

- **Ticks (AC-1, AC-2, AC-3, EC-1).** `tickCount() = m_ticksBeforeReset +
  32 × m_hw.elapsedTicks()`. Before every CPU reset, the current count
  goes into `m_ticksBeforeReset`, so the count never goes back, and the
  debug steps and the ASM overlay (which step the CPU too) count without
  a second path. `advance(n)`: `n == 0` returns 0. Otherwise it calls
  `m_hw.step()` until `tickCount()` has grown by at least `n`, and returns
  the growth. The step weight stays as it is (`TMC0501::getStepWeight`,
  `core/TMC0501.cpp:283-285`: 4 in IDLE, never on the TI-58C), so an
  instruction is 32 or 128 ticks and the overshoot is at most 127.
- **After each instruction** the advance does three things, in this
  order. It finishes a pending Reset restore (below). It records the
  polled-hardware bit for the LIB-17 adapter. If a run-control hook is
  set (§3.4), it checks breakpoints and the observer, and stops early.
  Without a hook it does no work beyond one pointer test.
- **Operations (AC-6, AC-8, AC-19).** None runs instructions. Each
  returns at once (LIB-1 AC-11), and the start-up runs in the advances
  that follow.

  | | TI-59, TI-58 | TI-58C |
  |---|---|---|
  | `reset()` | save all 120 RAM registers and the partition SCOM nibbles; CPU reset (`TI59Machine::reset`, which re-presses the card switch, `core/TI59Machine.cpp:39-45`); `m_restore` set | as `powerCycle()` |
  | `powerCycle()` | CPU reset; the RAM is left to the ROM's start-up, which clears it (as 2.3.0's core reset, golden `core-reset`) | CPU reset, RAM kept (constant memory, H00–H03 included) |
  | `coldBoot()` | RAM zeroed, then CPU reset | RAM zeroed, then CPU reset |

  The CPU reset clears the key matrix (AC-8). While `m_restore` is set,
  the first instruction that ends in the scan loop writes the saved RAM
  and partition back and clears it. Only then can `isKeyReady()` be true
  (AC-19). The partition is put back the way `setPartitionProgramRegs`
  does (`core/TI59Machine.cpp:118-131`); see D-5.
- **Build (AC-6 order, AC-7).** The constructor sets up the model, zeroes
  the RAM and SCOM, loads `content` into the RAM if one is handed (a
  TI-58C only), and resets the CPU. The new machine is therefore one that
  was just cold booted, with the same flags (the one-flag difference of
  `app/EmulatorSession.cpp:16-35` goes).
- **Key readiness (AC-18).** `isKeyReady() = !m_restore &&
  scanLoop(model).contains(pc)`, with the ranges of
  `app/MachineModelInfo.hpp:86-103`, moved to `core/TIRomRanges.hpp`.
  These include the printer's confirmation scan, as 2.3.0 has them (FS 5).
- **Keys (AC-9–12, EC-3).** `TIKeys` maps a `KeyId` to its row and
  column. `keyDown` sets `m_hw.pressKey(kBit[col], row)`. An unknown ID
  returns `KeyCall::UnknownKey` and changes nothing. The other spellings
  are not IDs: the machine refuses them, and presets, the harness and the
  tools resolve them (LIB-1 D-2). Two keys stay down
  together, because the core already ORs bits into `key[]`
  (`core/TMC0501.cpp`, `pressKey`).
- **Display (AC-13–15, EC-2).** `readDisplay()` calls
  `m_hw.takeDisplay()` and `toDisplayOutput()`. It gives 12 positions,
  index 0 rightmost. `lit` is `segmentsFor(digit, ctrl, suppressed)`
  over A–G, with DP lit when its duty is above 0. Position brightness is
  `digitDuty`, and DP's own brightness is `dpDuty` (D-4). The indicator
  `C` is separate from position 11, with `on = calcIndicator > 0` and
  `brightness = calcIndicator / 16` (D-4). `on` is always true. EC-2
  already holds: with no step in between, `takeDisplay` returns the last
  snapshot (`core/TMC0501.cpp`, `if (pollSteps == 0) return m_lastDisplay`).
- **Threading.** No lock, no atomic, no thread in the core (ADR 0009;
  the four mutexes of `reference/CoreArchitecture.md:834-850` go). The
  caller serialises (§5, `MachineGuard`).

### 3.2 Catalogue and builder (`core/TICatalogue.hpp`): LIB-1 AC-26–31

```cpp
const calcu::machine::Catalogue& tiCatalogue();         // TI-59, TI-58, TI-58C; peripherals from M-25
class TIMachineBuilder final : public calcu::machine::MachineBuilder {
  BuildResult build(const BuildRequest&, const Catalogue&) override;   // decodes RomImages (TIRomText), makes TIMachine
};
struct TIConstantMemory final : calcu::machine::ProductData { std::array<std::uint8_t, 120 * 16> ram; };  // C59-3 AC-2 hands it
```

Descriptor content per LIB-1 AC-26–31:
- tick rates 455,000 / 384,000 / 394,000 (`core/MachineVariant.hpp:29-36`);
- a segment display of 12 positions, layout A–G plus DP (DP has its
  own brightness), and indicator `C` with brightness; `hasSound = false`;
- the ROM entries of `roms/calculator/files.txt` (LIB-12 gives the form
  and md5);
- the 45 `KeyDescriptor`s of `TIKeys`, in row order;
- connectors module slot (8 contacts) and printer connector (11 pads),
  functional, each taking one, with `AfterChange::PowerCycle` (LIB-2
  AC-24, ledger row 59-11; reconciled with C59-6); places `slot-1` and
  `printer`;
- media place `card-reader` (pass-through, sides 0 and 1) on the TI-59
  only;
- debug target `TMC0501`;
- components with their depth (AC-31; ADR 0012). Today
  (`reference/CoreArchitecture.md` §Component Overview, `machine.md` fit
  table, "Depth today"): `TMC0501` separate; SCOM, CROM (module), card
  reader and printer interface folded into `TMC0501`; ROM and RAM
  separate; module slot and printer connector functional. After C59-6
  (M-25), the module and the printer no longer fold into `TMC0501`. The
  descriptor's depth entries change there.

### 3.3 App-side key IDs (`app/KeyIds.hpp`): AC-10

```cpp
std::optional<calcu::machine::KeyId> keyIdAt(std::uint8_t matrixCode);  // r×10+c → descriptor key (r-1)*5+(c-1)
std::optional<std::uint8_t> matrixCodeOf(const calcu::machine::KeyId&);
```
Matrix codes are product data (state files, `preset.md` §8.1
"Position"), not a core type. The key matrix itself (K-line bits, digit
slots) stays in the core.

### 3.4 Reaching the adapters

```cpp
// core/TIAdapters.hpp — included by app/, frontends' bridge, tools/headless
TIPeripheralsAdapter*  peripheralsOf(calcu::machine::Machine&);   // nullptr if not a TIMachine
TICardReaderAdapter*   cardReaderOf(calcu::machine::Machine&);
TIMemoryAdapter*       memoryOf(calcu::machine::Machine&);
TIDebugTargetAdapter*  debugTargetOf(calcu::machine::Machine&);
TIDebugPanelAdapter*   debugPanelOf(calcu::machine::Machine&);
TIPacingHints*         pacingHintsOf(calcu::machine::Machine&);
struct TIStepObserver { virtual bool stopAfter(std::uint16_t pc, bool stepBoundary, bool inScanLoop) = 0; };
enum class TIStop { None, Breakpoint, Observer };
```
The accessors check `descriptor().family` and `static_cast`, because
library code needs no RTTI (LIB-1 §2a). The adapters are not
capabilities, so they never appear in `capabilities()`, LIB-1 AC-21, or
the conformance check.

## 4. Product side (Calc-U-59)

| Today | After M-19 |
|---|---|
| `TI59Machine` as the public facade (`core/TI59Machine.hpp`) | core-internal; `TIMachine` is the only class above the core, as `calcu::machine::Machine` |
| `EmulatorSession(Model, ResourceReader, module, …)` builds `TI59Machine`, `loadROM`, `loadConstants`, `loadLibrary` (`app/EmulatorSession.cpp:20-45`) | `calcu::machine::build(tiCatalogue(), builder, {modelId, roms})`; the module image through `TIPeripheralsAdapter::loadModuleImage` (C59-6) |
| `softReset`, `cleanReset` (`app/StateApplier.cpp:95-115`), 300,000 steps | `Machine::reset()`, `coldBoot()`; over at key readiness (AC-6) |
| TI-58C start `powerOn`: start-up, then saved RAM (`:89-93`) | unchanged order, in the C59-3 adapter (row 59-15 waits for M-54) |
| TI-58C Reset reads `ti58c.mem` (`app/SessionController.cpp:186, 258-266`) | the machine keeps its RAM (C59-1 spec OQ-6, decided) |
| `pressMatrix`/`releaseMatrix` (`app/KeyMatrix.hpp`) | `keyDown(keyIdAt(code))`; `KeyMatrix.hpp` deleted |
| `takeDisplay()` → `DisplaySnapshot` in `FrameSampler`, `DisplayPov` | `readDisplay()`; POV unchanged in the app layer (C59-1 spec OQ-4, decided); seconds = Δ`tickCount()` / tick rate (equal to today's `pollTicks / instructionHz`) |
| `ModelInfo::scanLoop` in `FreezeController`, `KeySequencer`, `EmulatorSession::takeQueuedCard` (`app/EmulatorSession.cpp:316`) | `isKeyReady()`, or the observer's `inScanLoop` |
| `ModelInfo::idleAndKeyDetect` + `kPolledHardware` in `shouldSleep` | `TIPacingHints` (LIB-17, M-33) |
| `ModelInfo::cardWait` + `isWaitingForCard` | `TICardReaderAdapter::isInCardWait()` (C59-7; LIB-17's `TICardWaitAdapter` builds on it) |
| `runBatch` steps one by one, checks `FreezeController` and bit 31 (`app/Pacing.cpp:5-23`) | one `advance(target)`; per-instruction checks inside it through `TIDebugTargetAdapter::setRunControl(observer)`; `takeStop()` after. **Settles LIB-17 D-7: M-19 does it.** |
| `elapsedTicks()` in `EmulatorSession::elapsedReport`, `zeroElapseTime` | `tickCount()`, `descriptor().tickRate` |
| the four mutexes | none in the core; `MachineGuard` in the app until M-33 |

Deleted: `app/KeyMatrix.hpp`; `MachineVariant` and `instructionHz`
outside the core; `TI59Machine::pressKey/releaseKey/takeDisplay/reset/elapsedTicks`
as public calls above the core.

Stays: `Model` (app enum), `ModelInfo`'s file-format facts, the
threaded `EmulationRunner` and `KeySequencer` lanes (until M-33/M-34),
`DisplayPov`, and the drawing rule of `ledCell`, by which the C shows in
cell 11 when it is the brighter one. So the frontends show at least
what they show today (decision log 2026-11-10).

## 5. Adapters

Each adapter wraps old calls and exposes no member for model, advance,
ticks, the three operations, keys or display, which go through
`Machine` only (AC-17). The core types still reachable through the
adapter headers (`CpuFrame`, `DebugEvent`, `TIStepBoundary`,
`PrinterCodeLine`) go when their adapter goes (D-7).

| Adapter | File | Wraps | Removed in |
|---|---|---|---|
| `TIPeripheralsAdapter` | `core/TIAdapters.*` | `loadLibrary` (module image), `setPrinterConnected`/`isPrinterConnected`, `pressPrinterPrint`/`Adv`, `setPrinterTrace`/`printerTrace`, `drainPrinterLines`, `drainPrinterCodeLines`, `printerBufferContent`, `insertedModuleNumber` | M-25 (C59-6) |
| `TICardReaderAdapter` | `core/TIAdapters.*` | `insertCard`, `cardEject`, `isCardPresent`, `cardMode`, `isWaitingForCard`, `isInCardWait()` (`cardWait` range), `libExecPC` (cue card of the running module program) | M-55 (C59-7) |
| `TIMemoryAdapter` | `core/TIAdapters.*` | `writeProgram`, `writeProgramStep`, `readProgramStep`, `writeDataRegister`, `readDataReg`, `dataRegCount`, `partitionProgramRegs`, `setPartitionProgramRegs`, `copyRAMReg`, `writeRAMReg`, `serialiseRAM`, `deserialiseRAM`; `runInstructions(n)` for `StateApplier`'s 300,000-instruction start-up (n × `advance(1)`, so the count stays exact) | M-54 (C59-2 and C59-3; the debug panel's reads move onto the keystroke program memory kind there) |
| TI-58C start of 2.3.0 | `app/StateApplier.cpp` `powerOn` on `Machine&` + `TIMemoryAdapter` | start-up first, then the RAM of `ti58c.mem` | M-54 (C59-3 AC-2: `TIConstantMemory` at build; row 59-15) |
| `TIDebugTargetAdapter` | `core/TIAdapters.*` | `stepInstruction` (with the HOLD loop of `app/EmulatorSession.cpp:187-191`), `stepUntilNextKeycode`, `beginNextStep`, `snapshotCPU`, `lastStepBoundary`, `pc`, `setTraceFlags`/`traceFlags`, `drainCpuFrames`/`readCpuFrames`, breakpoints, `setRunControl(TIStepObserver*)`/`takeStop()`, `disassemble`, `readROMKeycode`, `decodeBCD` | M-53 (LIB-10 59); `FreezeController` moves onto LIB-10's run control there |
| `TIDebugPanelAdapter` | `core/TIAdapters.*` | `loadDebugOverlay`, `clearDebugOverlay`, `runDebugOverlay`, `setDebugLevel`, `drainDebugEvents` | M-56 (C59-4) |
| `TIPacingHints` | `core/TIAdapters.*` | polled-hardware since the last advance (`kPolledHardware`), `inIdleAndKeyDetect()` for `shouldSleep` | M-33 (LIB-17 59; D-2) |
| `MachineGuard` | `app/MachineGuard.hpp` | one `std::mutex` that the runner takes per `advance` and per debug step, and that every UI-thread or sequencer-thread call on the machine or an adapter takes per call. Callbacks (`freezeFired`, `batchDone`) run outside it. With `Threading::Cooperative` (Web) it is never contended. | M-33 for the Qt6 and Swift paths (LIB-3 59 / C59-5: the session on one thread); its last user, the Web's cooperative `EmulatorSession`, goes in M-34 (LIB-15 59; the old Web stack stays one step, LIB-3 D-1) — so at the latest M-34; D-1 |
| ROM file names from `ModelInfo` | `app/MachineModelInfo.hpp` | LIB-1 design's adapter (named there) | M-24 |

Plan §1 "no adapter is left at the end": each row above has its removal
step. M-59 checks it.

## 6. Tasks

Tests are in `products/calc-u-59/tests/`, in the executable
`CalcU59Tests`, group Frequent, and need the fetched ROMs as the core
tests do today. Test names: `c59_1_acN_…` / `c59_1_ecN_…`, and
`lib1_acN_…` as in the LIB-1 design. Tests are written before the code
of their task.

| Task | What | ACs/ECs | Test (file, case, group) |
|---|---|---|---|
| T1 | Before any core change: the expected values that cannot come from the goldens. The AC-1 weights on each model (an active and an IDLE instruction), and the FS 7 partition values for AC-6/19, recorded from the code as imported | (inputs to T4, T6) | notes in the test file's header, with the 2.3.0 commit |
| T2 | `TICatalogue`, `TIKeys`, `TIRomRanges`, `TIRomText`, `TIMachineBuilder` | LIB-1 AC-26–31; AC-9, AC-12, AC-16 (descriptor part) | `descriptor_tests.cpp`: `lib1_ac26_models_and_tick_rates`, `lib1_ac27_segment_display_and_no_sound`, `lib1_ac28_rom_entries_per_model`, `lib1_ac29_keys_connectors_places_debug_target`, `lib1_ac30_card_reader_on_ti59_only`, `lib1_ac31_components_and_depth`; `machine_tests.cpp`: `c59_1_ac9_45_key_ids_by_row_with_spellings` (and no `x2`), `c59_1_ac12_no_printer_keys` |
| T3 | `TIMachine` skeleton: build, `advance`, `tickCount` | AC-1, AC-2, AC-3, EC-1, AC-7 | `c59_1_ac1_instruction_ticks_per_model` (32 active; 128 IDLE on TI-59/58; 32 always on TI-58C), `c59_1_ac2_overshoot_below_one_instruction` (n = 1…10,000 and a fixed pseudo-random set; return = growth), `c59_1_ac3_tick_count_goes_on_across_operations`, `c59_1_ec1_idle_advance_by_32_runs_one_of_128`, `c59_1_ac7_new_machine_equals_cold_booted` (RAM, SCOM and display at key readiness) |
| T4 | Operations, Reset restore, build order | AC-6, AC-8, AC-19 | `c59_1_ac6_reset_keeps_ram_and_partition_ti59_ti58`, `c59_1_ac6_reset_equals_power_cycle_ti58c`, `c59_1_ac6_power_cycle_loses_memory_ti59_ti58`, `c59_1_ac6_power_cycle_keeps_constant_memory_ti58c` (H00–H03 included), `c59_1_ac6_cold_boot_from_zeroed_memory`, `c59_1_ac6_constant_memory_in_place_before_start_up` (TI-58C built with `TIConstantMemory`: partition `319.19` and `Fix 2` come back, FS 11), `c59_1_ac8_no_key_down_and_switched_on_display_after_each_operation`, `c59_1_ac19_restored_before_key_readiness` (`isKeyReady()` false until RAM and partition are back) |
| T5 | Key readiness | AC-18 | `c59_1_ac18_key_ready_in_scan_loop_each_model_with_and_without_printer`, `c59_1_ac18_not_key_ready_while_a_program_runs` (`Lbl A GTO A`, with `Pause`, with `Prt`, 2 s each, FS 5 cases) |
| T6 | Keys | AC-10, AC-11, EC-3 | `c59_1_ac10_each_key_id_presses_its_row_and_column` (all 45, key lines through `TIMachineTesting`, against today's matrix code r×10+c), `c59_1_ac11_two_keys_down_both_in_the_matrix`, `c59_1_ec3_unknown_key_refused_state_unchanged` (`x2`, `PRINT`, `""`) |
| T7 | Display output | AC-13, AC-14, AC-15, AC-16, EC-2 | `c59_1_ac13_twelve_positions_rightmost_first_segments_and_brightness`, `c59_1_ac14_digit_minus_degree_quote_apostrophe_blank_and_zero_suppression` (from today's `del_in_keycode_rom_shows_control_nibble_glyphs`), `c59_1_ac15_c_indicator_apart_from_leftmost_position` (running: positions dark, C on; waiting: the number, C off; C and minus in one output where the ROM drives both), `c59_1_ac16_no_sound_display_always_on`, `c59_1_ec2_two_reads_without_advance_are_equal` |
| T8 | Card switch on the readerless models | EC-4 | `c59_1_ec4_ti58_ti58c_find_their_card_switch_line`: after build and each operation, key readiness is reached, and the TI-58's idle pass goes through the card veto 0x0459–0x045B (PCs through `TIDebugTargetAdapter`), as today (`core/TI59Machine.cpp:14-23`) |
| T9 | `core_tests.cpp` through the interface | AC-5 | the AC-5 tests rewritten on `Machine`: `c59_1_ac5_boot_reaches_key_readiness_all_models`, `c59_1_ac5_two_plus_three_equals_five`, `c59_1_ac5_control_nibble_glyphs`, `c59_1_ac5_error_blink_period_ti59`/`_ti58`/`_ti58c`, `c59_1_ac5_display_duty_normalised`, `c59_1_ac5_ti58c_keeps_memory`, `c59_1_ac5_start_up_clears_memory_except_ti58c` (both of today's reset-routine tests). The rest of the 17 (ROM files, module parse, card, printer, breakpoint bit) go through the adapters, with names unchanged |
| T10 | Golden replay through the interface (§8) | AC-4 | `golden_runs_tests.cpp`: `c59_1_ac4_golden_runs_through_the_machine_interface` (17 runs) |
| T11 | Mutexes out, `MachineGuard`, adapters, app layer, frontends' display frame, `runBatch` on `advance` | AC-17 (with T12) | the existing `shared_*_tests.cpp`, `reference_runs_tests.cpp` (57 reference runs) and Playwright pass unchanged in what they assert. New: `shared_runtime_tests.cpp` `c59_1_runbatch_stops_at_freeze_inside_advance` (FREEZE ON START lands on the same instruction as `runBatch` today) |
| T12 | The seam check | AC-17 | `check_core_seam.sh` as CTest `CalcU59CoreSeam` (Frequent). Outside `core/` and `tests/`, it fails on an include of `TI59Machine.hpp`, `TMC0501.hpp`, `MachineVariant.hpp`, `RAM.hpp`, `ROM.hpp`, `TIMachine.hpp`, `TIKeys.hpp`, `TIRomRanges.hpp` or `TIMachineTesting.hpp` (`"../core/…"` and `<calcu59/core/…>`). It also fails when `TIAdapters.hpp` declares `pressKey`, `releaseKey`, `takeDisplay`, `reset`, `elapsedTicks`, `step`/`stepN` outside `TIDebugTargetAdapter`, or `variant` |
| T13 | LIB-1 conformance, no peripherals | LIB-1 AC-33 | `descriptor_tests.cpp`: `lib1_ac33_machine_conforms_without_peripherals` (`checkAgainstDescriptor`, three models; the media place clause passes in M-55, LIB-1 D-1) |
| T14 | Headless tools on the interface (§7) | (plan §1, tools) | CTest `CalcU59HeadlessSmoke` (Frequent, if Python3): `ti59.py 5 + 7 =` against the CMake-built `ti59headless` (`TI59HEADLESS_LIB`), display `12` |

Coverage: C59-1 AC-1–19 (19) and EC-1–4 (4) in T2–T10 and T12; LIB-1
AC-26–31 and AC-33 without peripherals (7) in T2 and T13. That makes 30
of the 30 assigned to M-19. LIB-1 AC-32, and AC-33 with peripherals,
are tested in M-25.

## 7. Tools and agent knowledge

**Tools moved in M-19 (plan §1).**
- `tools/headless/capi.cpp`: `h_new(model)` builds through
  `calcu::machine::build(tiCatalogue(), …)`, and `h_load_model_roms`
  hands the `RomImage`s. New: `h_key_down(name)`/`h_key_up(name)`
  resolve a name through the descriptor's ID and spellings, and return
  -1 for an unknown one (EC-3). They replace `h_press(kbit, digit)`.
  `h_stepn(n)` becomes n × `advance(1)`, which keeps instruction counts
  exact. `h_elapsed` becomes `tickCount()/32`: weighted steps as before,
  but no longer zeroed by a reset (AC-3). `h_instruction_hz` becomes
  tick rate / 32. `h_reset` becomes `powerCycle()` (today's core reset).
  `h_display` returns 12 positions of segments, DP and brightness from
  `readDisplay()`. Records, snapshot, disassembly, RAM, program, card
  and printer calls go through the §5 adapters. The adapters are linked
  into the same `ti59headless` target.
- `tools/headless/ti59.py`: `KEYS`/`SECOND` keep the 45 IDs of AC-9 and
  their spellings (`x<>t`, `x^2`, `sqrt`, `y^x`, `/`, `*`), and `x2`
  goes (AC-9). `KBITS` goes. `display()` decodes text from segments.
  `IDLE_LO/HI` and `in_idle()` read `h_idle_range` as today
  (`TIPacingHints`'s range, LIB-17 adapter). `README.md` gets the key
  names and the API.
- `tools/agentic/ti59file.py` (`dark_time`, `LIB.h_display`): reads the
  new `h_display`. The other `LIB.h_*` calls it makes (`h_stepn`,
  `h_ram_write`, `h_trace_sw`, `h_printer_busy_cycles`) keep their
  signatures.
- `tools/check_parity.sh`, `tools/read_trace.py`: unchanged (trace
  format unchanged).

**Agent knowledge in the same step** (CLAUDE.md "Agent knowledge follows
the code"; plan M-19):
- `products/calc-u-59/.claude/skills/calc-u-59-core/SKILL.md`: "Rules for
  Core changes". `TI59Machine` as the only public interface becomes
  `calcu::machine::Machine` (`TIMachine`) plus the named adapters of
  `core/TIAdapters.hpp`, each with its removal step. The `m_keyMutex`
  rule becomes "the core holds no lock and no atomic; the app's
  `MachineGuard` serialises until M-33".
- `products/calc-u-59/reference/CoreArchitecture.md`: Component Overview
  (`TIMachine`, catalogue, adapters), "Threading Model" (lines 834-850)
  rewritten, ticks (×32, monotonic), Reset/Power cycle/Cold boot, key
  IDs, display output.
- `products/calc-u-59/reference/SharedArchitecture.md` (Threads table,
  display path, key input coordinate systems), `DebugAPI.md`,
  `FrontendParity.md`: the names they cite (`TI59Machine`,
  `takeDisplay`, `softReset`).
- `products/calc-u-59/.claude/skills/calc-u-59-shared/SKILL.md`, if it
  names the moved calls.
- `docs/glossary-mapping.md` "Code names": Machine → `TIMachine`; Model
  → `ModelId` (descriptor), with `cu59::Model` kept as the app enum and
  `MachineVariant` core-internal; Reset/Cold boot → `Machine::reset()` /
  `coldBoot()` (instead of `softReset`/`cleanReset`); Power cycle →
  `Machine::powerCycle()`.
- Product memories that name the old calls
  (`memory/calc-u-59/live_debug_panel.md`, `solid_state_pc_display.md`,
  `memwr_memrd_implementation.md`): the current names added; history
  kept as quoted.
- Ledger: row 59-22 becomes visible (Reset keeps the partition). For
  row 59-13, only the machine side (AC-11) is visible here. The user
  sees it with LIB-5 in M-36 (plan §5.2).

## 8. Golden runs and risks

**Golden runs.** The 17 runs of `products/calc-u-59/tests/golden-runs/`
guard the step. Their `.run` and `.golden` files stay unchanged. The
replay maps each script line onto `Machine`, plus the adapters for what
`Machine` does not have yet:

| Script | Through |
|---|---|
| `model`, `module`, `printer` | `build()`; `TIPeripheralsAdapter` |
| `boot`, `idle` | 100 × `advance(1)` until `isKeyReady()` (today: `stepN(100)` until the scan loop, `tests/TestRoms.hpp:36-43`) |
| `keys`, `press`, `release` | `keyDown`/`keyUp` with the IDs of AC-9 (the scripts' `x2`, `sqrt`, `/`, `*` resolved by the replay's own table); hold = `uint32(rate/32 × 0.1)` × `advance(1)`, exactly today's instruction count |
| `run s` | `uint32(rate/32 × s)` × `advance(1)` |
| `app-reset` | `reset()`; on TI-59/58 then advance until `isKeyReady()` (2.3.0 ran the start-up inside `softReset`; on the TI-58C it did not) |
| `app-coldboot`, `core-reset` | `coldBoot()`, `powerCycle()` |
| `app-poweron`, `save-ram`, `partition` | the C59-3 / C59-2 adapters |
| `trace`, `paper`, `print`, `card-*` | `TIPeripheralsAdapter`, `TICardReaderAdapter` |
| display per line | `readDisplay()`, compared in segment space. The golden's `<ctrl><digit>` becomes `segmentsFor`, `--` means brightness 0, and `.` means DP brightness above 0 |
| `t=`, `pc=` | `t` as `(tickCount() − count at the last operation)/32`; `pc` through `TIDebugTargetAdapter`. After an `app-reset` on TI-59/58, `t` is compared as the difference to that line, because AC-6 replaces 2.3.0's 300,000 instructions |
| end: `= ram`, `= partition` | `TIMemoryAdapter` (inside the core) |

Expected differences, which are not findings: the two
`*-reset-partition` runs after their `app-reset` (row 59-22): their
comparison stops at that line, and T4 checks the kept partition (D-3).
Any other difference is a finding.

**Risks.**
- *The lock moves up.* Removing the four mutexes while the runner
  thread and the sequencer lanes still exist is a race unless every
  path takes `MachineGuard`. A missed path does not fail the tests.
  Mitigation: the adapters and `Machine` are reached only through
  `EmulatorSession`, which takes the guard. A ThreadSanitizer run of
  `CalcU59Tests` and the Qt app's smoke run goes into the step's
  qa-report.
- *Reset end moment.* The TI-59/58 Reset now ends at key readiness
  (about 2,100 weighted steps of start-up at regular speed, golden
  `ti59-fill-reset-coldboot` `boot` after `core-reset`). Before, it ended
  after 300,000 steps run at once. The display can be dark for the
  start-up after Reset, as it already is after Clean today. AC-6 states
  this as intended. Not a ledger row.
- *Restore at the first scan-loop instruction* (AC-19). 2.3.0 restored
  after 300,000 steps. If the ROM writes RAM after its first entry into
  the scan loop, the restore is too early. T4 and the golden replay show
  it (D-5).
- *Builds.* The Qt target compiles the core sources itself
  (`frontends/qt6/CMakeLists.txt:72-77`) and now links `calcu_library`.
  The XCFramework starts to merge `libcalcu_library.a` and copy the
  library headers (LIB-1 §2a). The Web engine moves to
  `-fwasm-exceptions` (LIB-1 D-6): app/ still reports ROM and file errors
  with exceptions, so the Web smoke and Playwright runs of this step
  check that those errors still reach the user. All three are first
  touched here.
- *Frequent budget.* The replay and `StateApplier`'s start-up use
  `advance(1)` per instruction (a virtual call). Frequent measured
  6.75 s (plan §1.1), so a few million calls fit. To be measured in T10.
- *Display frame churn.* Three frontends change their LED drawing input
  from nibbles to segments. Playwright, the XCUITests (Longer) and the
  Qt screenshot runs guard what is shown.

## 9. Decided (best guess)

- **D-1 (was OQ-1; ADR 0009).** A temporary app-layer lock
  `cu59::MachineGuard` serialises the three threads until M-33 (Qt6,
  Swift) and M-34 (Web); "no successor" holds for the machine and the
  core. Fewer changes in M-19 and no change to display sampling.
- **D-2 (was OQ-2).** `idleAndKeyDetect` and the polled-hardware bit stay
  behind `TIPacingHints` until M-33; full speed stays as in 2.3.0.
- **D-3 (was OQ-3; AC-4).** The two `*-reset-partition` runs are compared
  up to their `app-reset` line, and T4 checks the kept partition (no
  sidecar files). `t` and `pc` are compared as in §8, because the golden
  files carry them. Amended by the user, 2026-11-14: in
  `ti59-fill-reset-coldboot` the `pc` after its `app-reset` is not
  compared (scan-loop phase after a Reset at key readiness).
- **D-4 (was OQ-4; AC-13, AC-15).** Brightness is today's normalised
  duty (steady = 1.0), as AC-5 keeps it; C's brightness is
  `calcIndicator / 16`. Today's behaviour.
- **D-5 (was OQ-5; AC-19).** The restore writes what
  `setPartitionProgramRegs` writes; if T4 shows wrong register addressing
  after a Reset, it also takes SCOM[13][3..5], reported as a finding.
- **D-6 (was OQ-6; LIB-1 AC-7).** With a run-control hook set, `advance`
  may return early; the stop is read from
  `TIDebugTargetAdapter::takeStop()` until LIB-10 (M-53) defines the
  report.
- **D-7 (was OQ-7).** Only C59-1's own core types (display snapshot,
  model list, instruction rate) go in M-19; `CpuFrame`, `DebugEvent`,
  the step boundary and `PrinterCodeLine` go with their adapters' rows.
- **Assumed from other designs.**
  - LIB-1: `Machine`, `build()`, `Catalogue`, `checkAgainstDescriptor()`
    as in its §3. LIB-1 D-1: `capabilities()` lists no card reader
    before M-55. LIB-1 D-2: `keyDown` takes the ID only, and the tools
    resolve spellings. LIB-1 D-4: raw ROM bytes, so the decoding moves
    into the core. LIB-1 D-6: `-fwasm-exceptions` from this step.
  - LIB-17: `TICardWaitAdapter` builds on `TICardReaderAdapter::isWaitingForCard`
    and `isInCardWait()`. `Pacing.*` and `EmulationRunner.*` go in M-33,
    with `TIPacingHints` and `MachineGuard`. LIB-17 D-7: M-19 moves
    `runBatch` onto `advance`.
  - LIB-3 59 / C59-5 (M-33): the session on one thread, which removes
    `MachineGuard`.
  - LIB-12 (M-24, reconciled): answers LIB-1 D-4 the same way (raw
    bytes in `RomImage`; the product reader checks, the core decodes).
    Its TI reader uses `core/TIRomText.*` of this design.
  - C59-6 (M-25, reconciled): rewritten in this design's names
    (`TIMachine`, `tiCatalogue()`, `TIMachineBuilder`, `peripheralsOf()`,
    `TIPeripheralsAdapter` removed in M-25). Two items settled between
    the two designs:
    - `PrinterCodeLine` stays in `core/TIPrinterTypes.hpp` after M-25,
      because C59-6's product kind `c59.printer-output` hands code lines
      until LIB-8 59 (M-53). Under D-7 it therefore goes in M-53, not
      with `TIPeripheralsAdapter`.
    - C59-6 EC-3 (TI-58C key readiness with and without a printer)
      assumed a key readiness that reads what is wired. This design's
      `isKeyReady()` uses the static per-model ranges of
      `core/TIRomRanges.hpp`, which on the TI-58C always include the
      printer rescan (`kPrinterRescanLo/Hi`). Whether that is enough
      without a printer is checked by C59-6's
      `c59_6_ec3_ti58c_key_readiness_with_and_without_printer`; if not,
      M-25 adds a wired-printer input to the ranges.
  - C59-6, C59-7, C59-2/C59-3, LIB-10, C59-4: each takes over the adapter
    of §5 with its name and removes it in the step named.
- **Not verified.** `runDebugInjectedProgram` counts its steps into
  `elapsedTicks` (needed for AC-3 through the ASM overlay). No app path
  reads the display twice per frame (one reader, C59-1 spec OQ-7). A frame
  with C and the leftmost minus both lit exists in the ROM's normal
  displays (T7 searches the error blink of a negative result).
