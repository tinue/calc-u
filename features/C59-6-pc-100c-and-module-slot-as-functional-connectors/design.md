# C59-6 PC-100C and module slot as functional connectors: design
Status: draft for the Part B/C1 boundary review (2026-11-14)

## 1. Scope

C59-6 is a product row. One step does it all.

| Column | Step | Tests from (plan §3, M-25) | Done in (plan §5.1) |
|---|---|---|---|
| C59-6 | M-25 | AC-1–11, EC-1–3 | M-58 (the last clause of §1's table; D-1) |
| LIB-2 59 (with it) | M-25 | LIB-2 AC-24–27 | M-25 |
| LIB-1 59 (second half) | M-25 | LIB-1 AC-32, AC-33 with the PC-100C and a module plugged in (LIB-1 design §1, task P3) | M-25 |

- M-25 waits for M-8 (FS 8 and FS 9: both results are in), M-23 (LIB-2
  Library) and M-24 (LIB-12 59: module bytes from `roms/<ID>.rom.yaml`).
  It builds on M-19 (C59-1: the TI machine behind
  `calcu::machine::Machine`).
- FS 8 result: without a PC-100C the ROM executes one `PRT.STEP` and no
  `PRT.GO`/`PRT.OUT`/`PRT.FEED` (`docs/walkthroughs/first-steps.md` §8).
  So AC-7 holds trivially, and an absent printer can simply be absent.
  No stand-in busy time is needed.
- FS 9 result: with no module, `2nd Pgm 01` and `SBR =` make the display
  blink `0.` (golden runs `ti59-no-module`, `ti58-no-module`). AC-10
  stands as the code does it today.
- Clauses of the ACs that name a feature built later. By plan §1 such a
  clause passes in that feature's step, and C59-6 becomes Done with the
  last of them, M-58 (D-1; plan §5.1 still shows M-25 and follows).

  | Clause | Feature, step |
  |---|---|
  | AC-1: "paper output", and paper kept by the app session (LIB-8 AC-8, AC-12) | LIB-8 59, M-53 |
  | AC-6 and AC-8: the version-2 preset keys `printer-trace:` and `slot-1: { name: }`, and the refusal of `{ file: }` | LIB-6 59, M-54 |
  | AC-8: the picker shows `description` in `menu-sort` order (LIB-7 AC-8) | LIB-7 59, M-55; C59-8, M-58 |
  | AC-11: the attach/detach command and the power cycle that follows it (LIB-18) | LIB-18 59, M-54 |
  | EC-3: key readiness in typing (LIB-5) | LIB-5 59, M-36 |

  M-25 does each of these with today's means: the paper in the app
  session (code: `SessionController`), the 2.3.0 state-file directives,
  the existing module menus, and today's key-readiness use. The design
  names these as adapters (§5).

## 2. Modules and files

There is no new library code. M-25 uses `calcu::machine` (LIB-1) and
`calcu::peripherals` (LIB-2), both in `calcu_library`. Everything else is product code in
Calc-U-59's style: global namespace in `core/`, `cu59::` in `app/`.

New:

| File | Holds |
|---|---|
| `products/calc-u-59/core/PrinterPinOut.hpp` | the printer connector's functional pin-out: `PrinterPinOut`, `PrinterWord`, `PrinterPad` (§3.1) |
| `products/calc-u-59/core/ModuleSlotPinOut.hpp` | the module slot's functional pin-out: `ModuleSlotPinOut`, `ModuleWord` (§3.2) |
| `products/calc-u-59/core/PC100C.hpp`, `.cpp` | the PC-100C peripheral (§3.3). Moved from `TMC0501`: `PRN_CODE`/`PRN_STR`/`prnCharToCode` (`core/TMC0501.cpp:25-99`), the buffer, the ready gate, the busy count, the pending code line and `flushPendingCodeLine`/`flushPendingIfActive` (`:1243-1267`), the OUT PRT…PRT_FEED handlers (`:758-843`), `kPrinterLineCycles`, `kPrinterFeedCycles`, `kPrinterRowCycles`, `kPrinterRowStartCycles`, `kPrinterColumnCycles` (`core/TMC0501.hpp:246-247, 554-556`). `PrinterCodeLine` stays in `core/TIPrinterTypes.hpp`, where C59-1 moves it in M-19, and `PC100C.hpp` includes it |
| `products/calc-u-59/core/SolidStateRomModule.hpp`, `.cpp` | the Solid State ROM module peripheral (§3.4). Moved from `TMC0501`: `m_libData`, `m_libAddr`, `m_libAddrReadPos`, `m_libAddrWasWriting` (`core/TMC0501.hpp:372-376`), the LIB handlers (`core/TMC0501.cpp:891-933`), `loadLibrary` (`:183-191`) |
| `products/calc-u-59/core/PrinterOutput.hpp` | the product kind `c59.printer-output`, an adapter until LIB-8 59 (§3.5, §5) |
| `products/calc-u-59/tests/peripherals_tests.cpp` | the machine-level tests of §6 (Frequent, in `CalcU59Tests`) |
| `products/calc-u-59/tests/golden-runs/ti58c-printer.run`, `.golden` | recorded from the imported code before the core is touched, as the first commit of M-25 (D-5) |

Changed:

| File | Change |
|---|---|
| `core/TMC0501.hpp`, `.cpp` | Printer and module state removed. `wire(const TiConnectors&)` added. Key reads go through `keyRow(digit)`. The printer, LIB and TST BUSY handlers call the pin-outs (§4) |
| `core/TIMachine.hpp`, `.cpp` (C59-1's `TIMachine`) and `core/TI59Machine.hpp`, `.cpp` (core-internal since M-19) | `loadLibrary`, `setPrinterConnected`, `isPrinterConnected`, `setPrinterTrace`, `printerTrace`, `pressPrinterPrint`/`Adv`, `drainPrinterLines`/`CodeLines`, `printerBufferContent`, `m_printerConnected`, `m_printerTrace` go. Attach/detach/replace for `printer` and `slot-1` come in, with the wiring of the TI-58C's D0 pad to D10 (§4) |
| `core/TICatalogue.hpp`, `.cpp` (C59-1's `tiCatalogue()`, `TIMachineBuilder`) | the PC-100C peripheral descriptor; the builder attaches `BuildRequest::peripherals`; the component depth entries after C59-6 (LIB-1 AC-31) |
| `CMakeLists.txt` (product) | `PC100C.cpp` and `SolidStateRomModule.cpp` in `calcu59_core` (`:42-46`) |
| `app/ModuleCatalog.hpp`, `.cpp` | `allModuleMetadata` reads `description` and `menu-sort` from the 14 module files instead of `cuecards.txt`, reading them in today's `cuecards.txt` order and sorting stably by `menu-sort` (ST before RE, D-4). `moduleDescriptors()` is new (§3.6). `moduleCards` stays on `cuecards.txt` until M-55 (plan §2.3) |
| `app/SessionController.hpp`, `.cpp` | builds with peripherals in the request; `changeModule`/`setPrinterConnected` do attach/detach/replace and then the power cycle; the app session keeps `TRACE` and the paper (§4) |
| `app/EmulatorSession.hpp`, `.cpp` | `loadModule` no longer reaches the core; the session keeps the module bytes it handed in (`moduleImage()`, for the debugger) |
| `app/FrameSampler.cpp` | paper lines from the `c59.printer-output` capability, if one is plugged in (`:17-22`) |
| `app/StateFile`/`SessionController::loadStateFile` (`app/SessionController.cpp:645-668`) | `SOLID-STATE-MODULE:` and `PRINTER:` are applied first, with one power cycle, and only then the state. `TRACE` is off unless `PrinterTrace: on` (§4) |
| `tests/golden_runs_tests.cpp` | `module`, `printer`, `trace`, `paper`, `print` and the printed lines go through the places and capabilities (§8) |
| `tests/core_tests.cpp`, `tests/TestRoms.hpp`, `tests/shared_controller_tests.cpp` | printer and module setup through attach; printer tests renamed to their AC (§6) |
| `tools/headless/capi.cpp`, `ti59.py`, `coverage.py`, `README.md`; `tools/agentic/ti59file.py` | onto attach/detach and the capabilities (§7) |
| `frontends/web/bindings.cpp` (`:850-880`), `frontends/swift/Bridge/TIFileFormats.mm` (`:83-85`), `frontends/swift/App/ROMLoader.swift` (`:35`) | the module list from the module files (AC-8) |
| `frontends/qt6/app/PrinterWidget.cpp:39`, `app/PrinterFont.hpp:2`, `frontends/web/bindings.cpp:659` | include `core/TIPrinterTypes.hpp` (C59-1) for `PrinterCodeLine`, as from M-19; nothing else changes |

## 3. Seams and interfaces

The signal interfaces are the core's (`peripherals.md` P-4). The library
sees only `calcu::machine` and `calcu::peripherals` types.

### 3.1 Printer connector, functional pin-out (`core/PrinterPinOut.hpp`): AC-2, AC-4, TR

```cpp
// Signals of the fit table: Busy (KR), KP, KN, Φ1, D0, D15, IRG, IDLE, EXT, Φ2, D12, I/A.
enum class PrinterWord : uint8_t { Clear, Out, Func, Step, Go, Feed };   // IRG: PRT.CLR … PRT.FEED
enum class PrinterPad  : uint8_t { D0, D12, D15 };                       // the strobes the PC-100C answers on
class PrinterPinOut {
public:
  virtual ~PrinterPinOut() = default;
  virtual void instructionCycle() = 0;                    // Φ1: once per instruction, idle or not (AC-4, TR)
  virtual void word(PrinterWord, uint16_t kr) = 0;        // IRG with KR[10:4] as data
  virtual bool busy() const = 0;                          // Busy, read by TST BUSY
  virtual uint8_t keyLines(PrinterPad) const = 0;         // bit 2 = KP, bit 0 = KN on that strobe
};
```

| Signal (fit table) | What it carries here | Today |
|---|---|---|
| Φ1/Φ2 | `instructionCycle()`, called once at the top of every `TMC0501::step()`. An idle instruction also counts once, although it weighs 4 (spec TR) | `core/TMC0501.cpp:472-478` |
| IRG, EXT, I/A, IDLE | `word()` for the six printer opcodes; the opcode class decides, as today | `:758-843` |
| Busy (KR) | `busy()` | `FLG_BUSY` in `:863-878` |
| KP, KN at D0 | bit 2 at `D0`: printer present | `setPrinterConnected`, `:1287-1298` |
| KP, KN at D12 | bit 2 PRINT, bit 0 ADV | `:1270-1278` |
| KP at D15 | bit 2 TRACE | `:1280-1283` |

The connector knows the host, the peripheral does not (`peripherals.md`
I-1). So the TI machine maps the pads to CPU digits: `D0` → digit 0 on
the TI-59 and TI-58 and digit 10 on the TI-58C, `D12` → 12, `D15` → 15
(AC-2, EC-3; today `core/MachineVariant.hpp:7`, `TMC0501.cpp:1290`).
`FLG_BUSY` stays a CPU flag name only if a trace format needs it (§8).

### 3.2 Module slot, functional pin-out (`core/ModuleSlotPinOut.hpp`): AC-8–10, TR

```cpp
// Signals: VGG, VDD, instruction words, calculating status, external access, Φ2, Φ1, common.
enum class ModuleWord : uint8_t { Fetch, AddressOut, AddressIn, FetchHigh };  // LIB.IN, LIB.PC.OUT, LIB.PC.IN, LIB.HI
class ModuleSlotPinOut {
public:
  virtual ~ModuleSlotPinOut() = default;
  virtual uint16_t word(ModuleWord, uint16_t kr) = 0;     // instruction word in; the EXT it drives out (0 for AddressOut)
  virtual uint16_t address() const = 0;                   // debug view: the CROM address register
  virtual uint8_t addressReadPos() const = 0;             // debug view: digit position (CpuFrame m_libAddrReadPos)
};
```

- The address register belongs to the module, as on the TMC0540. The
  CPU keeps `FLG_EXT_VALID` and `m_libExecPC`. It latches
  `m_libExecPC = slot->address()` before a `Fetch` at `libExecFetchPC()`,
  as today (`core/TMC0501.cpp:898`). `CpuFrame::m_libAddr` and
  `m_libAddrReadPos` (`core/TraceTypes.hpp:41-42`) are filled from the
  debug view.
- Empty slot: `TMC0501` holds `nullptr`, and every module word gives
  EXT = 0 with `FLG_EXT_VALID` set, as today's zero image does. One
  difference: `LIB.PC.IN` reads 0, while today it reads back the digits
  that `LIB.PC.OUT` shifted in. The golden runs `ti59-no-module` and
  `ti58-no-module` rule (D-2): if they differ, the empty slot keeps
  today's address register over an image of zeros, so `LIB.PC.IN` reads
  back as today.

### 3.3 The PC-100C (`core/PC100C.hpp`): AC-1, AC-3–6, EC-2

```cpp
class PC100C final : public PrinterPinOut {          // a core object; the TI machine's plug()/unplug() own it (LIB-2 §3.1)
public:
  static constexpr uint32_t kLineInstructions = 4165, kFeedInstructions = 1370;      // AC-4
  static constexpr uint32_t kRowInstructions = 508, kRowStart = 18, kColumnInstructions = 64;
  explicit PC100C(const calcu::machine::PeripheralRequest&);   // TRACE from request.switches (LIB-2 §3.2 restore, its D-3)
  // PrinterPinOut: as §3.1
  // operations (LIB-2 AC-10): reset/powerCycle/coldBoot clear the buffer and the ready gate, end a pending
  // line at its count (today TMC0501::reset, :175-180), and release PRINT and ADV (today key[] is wiped). TRACE stays.
  void detaching();                                  // EC-2: the pending line ends where the head is
  std::string bufferContent() const;                 // was printerBufferContent(); no caller today
  calcu::peripherals::ControlTable& controls();      // PRINT, ADV keys; TRACE switch (LIB-2 AC-14–16, AC-26)
  PrinterOutput& output();                           // §3.5
};
```

- Capability IDs (LIB-1 §3.7): `printer/controls` and
  `printer/c59.printer-output`. The `ControlTable`'s `OnChange` sets
  the PC-100C's key lines.
- PRINT and ADV are held as long as the control is down (AC-5). The ROM
  sees them only through `keyLines(D12)`. `TRACE` sets KP at D15 while on.

### 3.4 The Solid State ROM module (`core/SolidStateRomModule.hpp`): AC-8–10, EC-1

```cpp
class SolidStateRomModule final : public ModuleSlotPinOut {   // a core object, like the PC-100C
public:
  explicit SolidStateRomModule(std::span<const uint8_t> rom);   // at most 5000 bytes, rest zero (today :183-191)
  // ModuleSlotPinOut: as §3.2, the code of today's :891-933
  // operations: reset/powerCycle/coldBoot clear the address register and its read position (today :149-150)
};
```

The bytes come in with the attach (`PeripheralRequest::roms`, LIB-2
AC-11), in the form M-24 hands them (§9 C-1). If they are missing, the
attach is refused and names `<ID>.rom.yaml` (EC-1, LIB-2 EC-4). The
refusal comes from rule 5 of LIB-2's `canAttach` (`Refusal::missingRoms`,
"The <name> could not be attached: <file> is missing.") before
`plug()` is called.

### 3.5 `c59.printer-output` (`core/PrinterOutput.hpp`), product kind: AC-1, AC-3, EC-2

```cpp
struct PrinterOutput : calcu::machine::Capability {
  static constexpr calcu::machine::KindId kKind{"c59.printer-output"};
  virtual std::vector<std::string> takeLines() = 0;          // today drainPrinterLines
  virtual std::vector<PrinterCodeLine> takeCodeLines() = 0;  // today drainPrinterCodeLines
};
```

This is LIB-8's paper output, held in place until M-53. With no PC-100C
the kind is not listed (AC-1, LIB-1 EC-5).

### 3.6 Catalogue entries: AC-8, LIB-1 AC-32, LIB-2 AC-24

| Descriptor | Content |
|---|---|
| PC-100C (core, static) | id `PC-100C`; `fits` = the printer connector's pin-out; controls `PRINT` Key, `ADV` Key, `TRACE` Switch; no ROM |
| 14 modules (`cu59::moduleDescriptors(files)` in `app/ModuleCatalog.cpp`, added with `Catalogue::addPeripheral`) | id = `module-name`; `name` = `description`; `roms` = one `RomEntry` for `<ID>.rom.yaml` (form: LIB-12); `fits` = the module slot's pin-out; `productData` = `menu-sort` |
| printer and slot pin-outs (C59-1's data in `tiCatalogue()`) | `Functional`, `Takes::One`, `AfterChange::PowerCycle` (LIB-2 AC-24) |

The modules come from the files because AC-8 and M-25 take their names
from there. LIB-1 AC-3/`addPeripheral` covers "read after the catalogue
was built" (D-3).

## 4. Product side (Calc-U-59 only)

**Core**
- `TMC0501` keeps `TiConnectors { PrinterPinOut* printer; uint8_t d0Digit; ModuleSlotPinOut* module; }`,
  set with `wire()` by the TI machine at build, attach and detach.
  It holds no printer state and no module image after this (spec TR).
- `uint8_t keyRow(uint8_t digit)` is `key[digit]` OR the printer's
  `keyLines` at a mapped pad. It replaces `key[digit]` in KEY scan-all,
  test-row and TST BUSY (`core/TMC0501.cpp:594-618, 866-878`).
  Multi-key rejection runs on the combined row, as today, because today
  the printer lines sit in `key[]`.
- `step()`: `if (m_c.printer) m_c.printer->instructionCycle();` in the
  place of the countdown (`:472-478`). TST BUSY uses
  `m_c.printer && m_c.printer->busy()`.
- `reset()` loses its printer and module lines (`:149-150, 175-180`).
  The machine tells the peripherals of each operation (LIB-2 AC-10).
  `TI59Machine::reset`'s re-application of printer and TRACE
  (`core/TI59Machine.cpp:39-45`) goes, because the lines come from the
  PC-100C live.
- `TIMachine` (C59-1) overrides LIB-2's protected `plug()` and
  `unplug()` (LIB-2 design §3.1) for `printer` and `slot-1`. `plug()`
  builds `PC100C` or `SolidStateRomModule` from the request and wires
  it. The library's `followsNow()` default gives the pin-out's
  `PowerCycle` (LIB-2 AC-4, AC-24). There is no `productRefusal`: rule 6
  refuses a second module (LIB-2 AC-25). `unplug()` of the PC-100C calls
  `detaching()` first (EC-2). `plugged()`, `capabilities()` and key
  readiness (C59-1 AC-18; TI-58C rescan with a printer, `kPrinterRescanLo/Hi`
  in `core/TIRomRanges.hpp`, moved there by C59-1 from
  `app/MachineModelInfo.hpp:36-38`, EC-3) read what is wired. C59-1's
  `isKeyReady()` uses the static per-model ranges, which on the TI-58C
  always include the rescan, as today (§9 A-5).
- Peripherals in the `BuildRequest` are attached at build with no power
  cycle (the build is the start).

**App session (code: `SessionController`)**
- `startMachine` puts the PC-100C (if connected) and the module (if any)
  into `BuildRequest::peripherals`. This replaces
  `app/SessionController.cpp:103-135`, including the
  "Solid-state module library was not loaded" log line (AC-10). An empty
  slot is a normal machine.
- `changeModule(id)`, inside `m_session->sync`:
  `replace(slot-1, …)` (LIB-2 AC-35), or `detach` for none, and then
  `powerCycle()` when the answer says so. This replaces
  `selectModule` + `resetMachine` (`:220-245`): row 59-11, the TI-58/59
  lose program and registers; on the TI-58C the constant memory is
  C59-1's/C59-3's.
- `setPrinterConnected(bool)`: attach or detach `printer`. On detach it
  first takes the last output into `m_printerLines` (EC-2), then runs
  `powerCycle()` (`:362-370`; row 59-11). `togglePrinterConnection` no
  longer calls `resetMachine()`, which is 2.3.0's Reset and keeps the
  RAM. The machine's `powerCycle()` loses it, as row 59-11 says. The
  LIB-2 design §5 says the same (D-8).
- `TRACE` belongs to the app session: `m_printerTrace`, off at start,
  never saved. It goes into `PeripheralRequest::switches` of each new
  PC-100C (build, attach; LIB-2 D-3) and is kept
  across reset, power cycle, cold boot, model switch and detach/attach
  (AC-6, row 59-25). `togglePrinterTrace`, `printerTrace()` and
  `applyControl(PrinterTrace)` (`:372-377, 672-675`) set it and the
  switch.
- `pressPrinterPrint`/`pressPrinterAdv` press the controls (`:380-381`).
  The frontends' calls on `SessionController` stay as they are (§5).
- `loadStateFile` (`:645-668`): first the `SOLID-STATE-MODULE:` and
  `PRINTER:` directives, as attach/detach with one power cycle; then
  `applyState`; `TRACE` off; `PrinterTrace: on` in the keystrokes sets
  it. Today the module is swapped after the state is applied, under the
  running machine. Applying it after a power cycle would lose the
  loaded program. The reference runs of M-9 guard this (§8).
- `m_printerLines` (the paper) is kept through detach, attach and model
  switch, as today (AC-1).

**Deleted:** the printer and LIB members of `TMC0501`;
`TI59Machine::loadLibrary`, `setPrinterConnected`, `isPrinterConnected`,
`setPrinterTrace`, `printerTrace`, `pressPrinterPrint`/`Adv`,
`drainPrinterLines`/`CodeLines`, `printerBufferContent`; `m_prnMutex`
(if M-19 has not removed it already); `moduleRomFile`'s use for the ROM
(after M-24, if any is left).

**Stays:** `insertedModuleNumber()` and `libExecPC()` (SCOM and CPU
latch); `ModuleImage`/`DebugModel` (on `EmulatorSession::moduleImage()`);
`moduleCards` and `cuecards.txt` (cue cards, C59-7, M-55).

## 5. Adapters

Removed in M-25: C59-1's `TIPeripheralsAdapter` (`core/TIAdapters.hpp/.cpp`,
reached through `peripheralsOf(calcu::machine::Machine&)`; C59-1 design
§3.4, §5). Reconciled with the C59-1 design:

| Part of `TIPeripheralsAdapter` | Wraps | Its callers moved in M-25 to |
|---|---|---|
| printer calls | `setPrinterConnected`/`isPrinterConnected`, `pressPrinterPrint`/`Adv`, `setPrinterTrace`/`printerTrace`, `drainPrinterLines`, `drainPrinterCodeLines`, `printerBufferContent` | attach/detach, `Controls`, `c59.printer-output`, `PC100C::bufferContent` |
| module image | `loadLibrary` (`loadModuleImage`, used by `app/EmulatorSession`) | `PeripheralRequest` at build, `replace`/`detach` |
| `insertedModuleNumber` | the SCOM read | stays a core call inside `TIMachine` (no caller outside the core today) |
| the users in `tools/headless/capi.cpp` (C59-1 §7 routes them through `peripheralsOf()`) | `h_printer`, `h_trace_sw`, `h_printer_print`, `h_printer_adv`, `h_load_module`, `h_printer_lines`, `h_printer_code_lines`, `h_printer_busy_cycles` | §7 |
| the golden driver's `module`, `printer`, `trace`, `paper`, `print` (C59-1 §8) | the same | `BuildRequest::peripherals`, `Controls`, `c59.printer-output` (§8) |

Added in M-25, removed later:

| Adapter | File | Wraps | Removed in |
|---|---|---|---|
| `c59.printer-output` product kind | `core/PrinterOutput.hpp`, `app/FrameSampler.cpp` | the paper as text and code lines | M-53 (LIB-8 59: paper output kind) |
| app session printer/module calls (the same rows as LIB-2 design §5: printer connect, module change, PC-100C buttons) | `SessionController::setPrinterConnected`, `togglePrinterConnection`, `changeModule`, `selectModule`, `pressPrinterPrint`/`Adv`, `togglePrinterTrace` (and their Qt6, Swift bridge and Web callers) | attach/detach + power cycle, controls | attach/detach: M-54 (LIB-18 59); controls: M-57 (Qt6, Web: LIB-4 59, LIB-14 59) and M-58 (Swift: C59-8, LIB-4 59), as LIB-2 §5 |
| 2.3.0 directives on places | `SessionController::loadStateFile`, `app/StateFile.cpp:381` (`PRINTER:`), `SOLID-STATE-MODULE:`, `PrinterTrace:` | version-2 `printer:`, `slot-1:`, `printer-trace:` | M-54 (LIB-6 59) |
| module list from module files for today's menus | `allModuleMetadata` in `app/ModuleCatalog.cpp`, `TIFileFormats.mm`, `bindings.cpp:880` | the picker | M-55 (LIB-7 59) / M-58 (C59-8) |
| `EmulatorSession::moduleImage()` | the bytes handed in at the attach, for `DebugModel` | the debug target's memory view | M-53 (LIB-10 59) |

## 6. Tasks

Tests come before the code of each task. File: `tests/peripherals_tests.cpp`
unless named otherwise. Executable `CalcU59Tests`. Group Frequent.

| Task | What | ACs/ECs | Test (file, case) |
|---|---|---|---|
| T1 | Pin-out headers; PC-100C and module descriptors; depth entries after C59-6 | AC-8, LIB-1 AC-32, LIB-2 AC-24 | `descriptor_tests.cpp`: `lib1_ac32_pc100c_and_14_modules`, `lib1_ac31_components_and_depth` (updated); here: `c59_6_ac8_fourteen_modules_by_id_description_menu_sort`, `lib2_ac24_places_functional_power_cycle_follows` |
| T2 | `SolidStateRomModule` out of `TMC0501`; empty slot | AC-9, AC-10, LIB-2 AC-25 | `golden_runs_tests.cpp`: `golden_runs_of_the_ti_core` (`ti59-module-ml`, `ti59-module-le`, `ti59-switch-on`, `ti59-no-module`, `ti58-no-module`); `c59_6_ac10_empty_slot_runs_and_blinks_on_pgm_01`; `lib2_ac25_second_module_refused_until_detached`, `lib2_ac25_empty_slot_machine_runs` |
| T3 | Module attach without its ROM | EC-1 | `c59_6_ec1_module_without_rom_refused_file_named` |
| T4 | `PC100C` out of `TMC0501`; `keyRow`; pad wiring | AC-2, AC-3, AC-4, AC-7, EC-3 | `golden_runs_of_the_ti_core` (`ti59-printer`, `ti59-print-with-printer`, `ti59-print-without-printer`, `ti59-module-*`); `core_tests.cpp`: `ti59_/ti58_/ti58c_error_blink_period_matches_hardware` through attach (renamed `c59_6_ac2_…`); `c59_6_ac4_busy_counts_instructions_idle_once`; `c59_6_ac4_printer_interrupt_partial_rows` (was `printer_interrupt_partial_rows`); `c59_6_ec3_ti58c_key_readiness_with_and_without_printer` |
| T5 | Attach/detach of the PC-100C, its capabilities | AC-1, LIB-2 AC-26, LIB-2 AC-27 | `c59_6_ac1_printer_plugged_and_removed`; `lib2_ac26_pc100c_print_adv_trace`, `lib2_ac26_no_pc100c_no_controls`; `lib2_ac27_no_serial_port_no_clock` (the LIB-2 names are LIB-2 design P1–P4) |
| T6 | Controls | AC-5 | `c59_6_ac5_print_key_prints_display` (was `printer_print_key_emits_line`); `c59_6_ac5_adv_feeds_while_held` (more feeds for a longer hold); `c59_6_ac5_trace_prints_while_on` |
| T7 | TRACE in the PC-100C and the app session | AC-6 | `c59_6_ac6_trace_survives_reset_power_cycle_cold_boot`; `shared_controller_tests.cpp`: `c59_6_ac6_trace_kept_by_app_session` (start off, model switch, detach/attach, not saved), `c59_6_ac6_state_file_sets_trace_off_unless_it_sets_it` |
| T8 | Detach mid-line | EC-2 | `c59_6_ec2_detach_mid_line_keeps_rows_then_power_cycle` (the code line ends with the `rowCount` at that count, then the power cycle runs) |
| T9 | Change and power cycle | AC-11 | `c59_6_ac11_module_change_then_power_cycle_like_fresh_build` (ML → LE, `Pgm 01 SBR =` printed as on a machine built with LE); `c59_6_ac11_printer_attach_detach_then_rom_finds_it` (error blink period 0.899 s / 0.449 s after each power cycle) |
| T10 | App session: build request, replace, paper, state files | AC-1, AC-11 (row 59-11) | `shared_controller_tests.cpp`: `c59_6_ac1_paper_kept_across_detach_and_attach`, `c59_6_ac11_change_module_power_cycles` (TI-59 program lost), `controller_model_switch_keeps_printer_…` (updated to `plugged()`), `controller_bad_module_changes_nothing` (kept) |
| T11 | Conformance with peripherals | LIB-1 AC-33 | `descriptor_tests.cpp`: `lib1_ac33_machine_conforms_with_printer_and_module` (every model, ML + PC-100C, and empty) |
| T12 | Tools and knowledge | — | `tools/headless`: `ti59.py` printer and module calls run (manual run of `coverage.py` on one example); review of §7 |

Coverage: C59-6 AC-1–11 (11) and EC-1–3 (3); LIB-2 AC-24–27 (4); LIB-1
AC-32, AC-33 (2). All 20 are in T1–T11. The clauses of §1 that wait for
later features are tested in those features' steps (D-1).

## 7. Tools and agent knowledge

**Tools (plan §1, "Today's tools")**
- `tools/headless/capi.cpp`: `h_printer` → attach/detach `printer`;
  `h_trace_sw`, `h_printer_print`, `h_printer_adv` → `Controls`;
  `h_load_module` → a module in the build request or `replace`;
  `h_printer_lines`/`h_printer_code_lines` → `c59.printer-output`;
  `h_printer_busy_cycles` → `PC100C::kLineInstructions`/`kFeedInstructions`.
  The C ABI stays, so `ti59.py`, `coverage.py` and `tools/agentic/*`
  (`ti59file.py:23, 98`) keep working.
- `ti59.py:56` names ADV "PAPER" (AC-5). Rename it to `ADV` and keep
  `PAPER` as an alias until the harness replaces the tool (M-59).
  `tools/headless/README.md:24` changes with it.

**Agent knowledge (same step, CLAUDE.md "Agent knowledge follows the code")**
- `products/calc-u-59/.claude/skills/calc-u-59-core/SKILL.md`: the
  printer and the module are peripherals in `core/PC100C.*` and
  `core/SolidStateRomModule.*`; `TMC0501` sees their pin-outs; the
  `libExecPC` latch stays in the CPU.
- `products/calc-u-59/reference/CoreArchitecture.md`: "Solid-State
  Library Module" (`:638-720`), "Printer (PC-100C)" (`:722-830`),
  "Printer BUSY" (`:951-`), and the lock table (`:844`, `m_prnMutex`).
  It also says the printer timings were measured on a PC-100A (plan M-25;
  `first-steps.md`, "Not first steps"). `reference/DebugAPI.md:39`
  (`printerBufferContent` → `PC100C::bufferContent`).
  `reference/SharedArchitecture.md:492`.
- `ti59-research`: the PC-100A note (plan M-25). This is a research
  repository, not an old product repository: a local commit there, no
  push (D-6).
- `docs/glossary-mapping.md`, row "Solid State ROM module": add the code
  names `SolidStateRomModule`, `PC100C`. The code names of the Printer
  row stay as they are; no visible label changes.
- `memory/` (product memories, `calc-u-59-memories.md` index): those that
  name `setPrinterConnected`, `loadLibrary` or `m_libData` are updated.
  The implementer greps for them in the step.
- Ledger: rows 59-11 and 59-25 become visible (plan M-25); the release
  notes are M-63's.

## 8. Golden runs and risks

**Guards.** All 17 runs of `products/calc-u-59/tests/golden-runs/`
replay through the places:
- printer: `ti59-printer`, `ti59-print-with-printer`,
  `ti59-print-without-printer`, `ti59-module-ml`, `ti59-module-le`
  (lines, characters, dots, `rows=`/`partial=`);
- slot: `ti59-switch-on`, `ti59-no-module`, `ti58-no-module`, and every
  run without a `module` line, which uses ML by the driver's default
  (`golden_runs_tests.cpp:102`);
- with them `core_tests.cpp` (blink periods, partial rows) and the M-9
  reference runs (`tests/reference_runs_tests.cpp`) for the state-file
  order.

A difference is a finding.

**Driver change.** `printer on`/`module X` before `boot` become
`BuildRequest::peripherals`, so no power cycle is added. `trace`,
`paper` and `print` become `Controls`, and the printed lines come from
`c59.printer-output`. The `.golden` files are not re-recorded.

**Risks**
- Empty slot `LIB.PC.IN` (§3.2): if `ti59-no-module`/`ti58-no-module`
  differ, the ROM read back an address with no module, and the empty
  slot keeps today's zero image (D-2).
- Trace frames: `CpuFrame::m_libAddr` reads 0 with an empty slot (today
  it is the CPU's register). `tests/shared_trace_tests.cpp` and
  `CalcU59TraceReader` use a module, as far as read; to check.
- Order of the events in one instruction: the busy countdown must stay
  before `beginNextStep()`, and `detaching()`/`reset` must flush exactly
  as `flushPendingIfActive` does today, or the `dots` lines move.
- PRINT or ADV held through a reset: today the reset wipes `key[]` and
  releases them; the PC-100C releases them on each operation to keep
  that. No golden run holds a key through a reset.
- Hot path: one null test and one virtual call per instruction with a
  printer, against an inline countdown today. Measured with the Frequent
  timing (6.75 s baseline, plan §1.1); if it shows, cache `busy`.
- State-file order (§4): applying peripherals before the state changes
  when the power cycle happens relative to the load. The 57 reference
  runs decide.
- Swift app: `TIFileFormats.mm` and `ROMLoader.swift` change. It builds
  locally only (plan §1.1). The XCFramework already merges
  `libcalcu_library.a` (LIB-1 §2a).
- No golden run has a TI-58C with a PC-100C (the D10 wiring) today. M-25
  records `ti58c-printer` from the imported code before the core is
  touched (D-5).

## 9. Decided (best guess)

- **D-1 (was OQ-1; plan §1, §5.1).** The clauses of §1's table pass in
  their features' steps (M-36, M-53, M-54, M-55, M-58), and C59-6 is Done
  in M-58 (plan §1); M-25's means are adapters until then. Plan §5.1
  still shows M-25 for C59-6 and follows this (as LIB-1 D-1).
- **D-2 (was OQ-2; AC-10, FS 9).** The empty slot drives nothing (§3.2);
  the no-module golden runs win: if they differ, the slot keeps today's
  zero image with its address register.
- **D-3 (was OQ-3; AC-8).** The 14 descriptors come from the module files
  through `addPeripheral`; the full ROM set is needed at start anyway
  (LIB-12 AC-5).
- **D-4 (was OQ-4; AC-8).** `menu-sort` ties keep today's order, ST
  before RE: the loader reads the files in `cuecards.txt`'s order and
  sorts stably.
- **D-5 (was OQ-5).** `ti58c-printer` is recorded as a golden run from the
  imported code before the core is touched (CLAUDE.md rule).
- **D-6 (was OQ-6).** The PC-100A note is committed to `ti59-research`
  locally in M-25, no push.
- **D-7 (was OQ-7; LIB-1 AC-32).** AC-32's paper is checked at run time
  through the capability (T11); no descriptor field.
- **D-8 (was OQ-8).** Closed before: the app session runs
  `Machine::powerCycle()` after a printer or module change (LIB-2 §5).
- **Not verified:** that M-24 leaves `moduleRomFile` unused, and that
  no trace test runs without a module (§8).

**From C59-1 (M-19), reconciled with its design**
- A-1: `TIMachine final : calcu::machine::Machine` in `core/TIMachine.hpp`,
  over the core-internal `TI59Machine`/`TMC0501`, single-threaded, with
  no mutexes. This design adds the LIB-2 hooks to it.
- A-2: `tiCatalogue()` and `TIMachineBuilder` in `core/TICatalogue.hpp`:
  places `slot-1` and `printer`, a module slot (8 contacts) and a printer
  connector (11 pads), each functional and taking one, and components
  with depth (LIB-1 AC-29, AC-31). M-25 adds the PC-100C, the module
  descriptors and the depth entries after C59-6, as C59-1 §3.2 expects.
  The pin-outs carry `AfterChange::PowerCycle` (LIB-2 AC-24) from M-19.
- A-3: `TIPeripheralsAdapter` (`core/TIAdapters.*`, `peripheralsOf()`)
  keeps the printer calls and the module image reachable until M-25,
  which removes it (§5).
- A-4: `powerCycle()`, `reset()` and `coldBoot()` on `TIMachine` (C59-1
  §3.1): the power cycle loses the TI-58/59 RAM and keeps the TI-58C's
  constant memory.
- A-5: key readiness is `isKeyReady()` over the static ranges of
  `core/TIRomRanges.hpp`, which on the TI-58C always include the printer
  rescan, as today (`app/MachineModelInfo.hpp:36-38`); no input from what
  is wired. T4's `c59_6_ec3_ti58c_key_readiness_with_and_without_printer`
  checks it; a failure is a finding.
- A-6: the golden driver plays through `Machine` (C59-1 §8), and
  `EmulatedTime` turns `run <s>` into instructions (LIB-1 §8).
- A-7: `PrinterCodeLine` in `core/TIPrinterTypes.hpp` (C59-1) is kept by
  this design's `c59.printer-output` until M-53, which C59-1 notes under
  its D-7.

**Taken from LIB-2's draft (M-23; `features/LIB-2-…/design.md`, read at the end of this draft)**
- B-1: `Machine::canAttach/attach/detach/replace` with `const Catalogue&`,
  `ChangeResult { refused, follows }`, `Refusal { reason, missingRoms }`;
  the core overrides `plug()`/`unplug()` (LIB-2 §3.1). No peripheral base
  class in the library: `PC100C` and `SolidStateRomModule` are core
  objects.
- B-2: `calcu::peripherals::Controls` and `ControlTable` with `OnChange`
  and `restore()` (LIB-2 §3.2); `PeripheralRequest::switches` carries
  `TRACE` (LIB-2 D-3).
- B-3: the core tells its peripherals of reset, power cycle and cold
  boot in its own way (LIB-2 §3.1, AC-10); here §3.3, §3.4.
- B-4: LIB-2's test names P1–P4 in `tests/peripherals_tests.cpp`, used
  as they are in §6.

**From LIB-12 (M-24), reconciled with its design**
- C-1: the module file reaches the attach as bundled: raw `.rom.yaml`
  bytes in `RomImage` (LIB-12 §3, its answer to LIB-1 D-4), and the core
  decodes it with LIB-12's `core/ModuleFile`, which also gives
  `module-name`, `description` and `menu-sort` for `moduleDescriptors()`.
  The YAML reader behind `ModuleFile` is `library/formats/`'s, moved
  there from Calc-U-1600's `core/Yaml.hpp` in M-24 (LIB-12 D-4).
