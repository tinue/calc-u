# C1600-1 Sharp machines on the machine interface: design
Status: draft for the Part B/C1 boundary review (2026-11-14)

## 1. Scope

One step, M-26. It completes this row and the Calc-U-1600 columns of
LIB-1 and LIB-2 (plan §3, M-26; §5.1).

| Column | Step | Tests from (plan) | Done in (§5.1) |
|---|---|---|---|
| C1600-1 (product row) | M-26 | AC-1–21 (AC-21 per FS 15), EC-1–3 | M-26 |
| LIB-1 1600 | M-26 | LIB-1 AC-34–41 (test cases named in the LIB-1 design, §6 P4, P5) | M-26; the media place clause of AC-40 passes in M-47 (LIB-1 D-1) |
| LIB-2 1600 | M-26, M-31, M-47 | LIB-2 AC-28–34 (test cases are the LIB-2 design's; split assumed in §9 A-6) | M-47 |

- **Waits for** (plan M-26): M-5 (FS 15: a key readiness signal exists,
  AC-21 stands as written), M-8 (the 20 Sharp golden runs and C-7), M-23
  (LIB-2 L, and with it M-18 LIB-1 L), M-4 (C-2: no bound was hit, so
  C1600-1 stands as written and prompt readiness needs nothing per
  mode).
- **Not here** (later steps, so nothing is lost): the typing engine and
  the end of the PC-1500 key queue (LIB-5, M-36); the paper output kind
  (LIB-8 1600, M-45); debug targets and run control on the library
  (LIB-10 1600, M-45); loaders on the loader memory kind (C1600-2, M-44);
  presets, the bus ROMs and running to the prompt (LIB-6, C1600-4,
  LIB-18 1600, M-46); media places for cassettes, floppy disks and
  battery-backed modules (LIB-7 1600, M-47); the ROM lookup (LIB-12
  1600, M-27); the library's sessions (M-31); the frontends on the family
  structure (M-49); the old CLIs and probes go (M-51).
- Paths of the spec are at `b4ea8c8`. In this repository: `Core/` is
  `products/calc-u-1600/core/`, `Shared/` is `app/`, `Qt6/` is
  `frontends/qt6/`, `Web/` is `frontends/web/`. Below, paths without a
  prefix are under `products/calc-u-1600/`.

## 2. Modules and files

No library area is added. The step uses `calcu::machine` (LIB-1) and
`calcu::peripherals` (LIB-2), both in the one target `calcu_library`
(LIB-1 design §2a), which `calcu_core` already links
(`cmake/CalcU.cmake:112-113`).

New, in the core (product style: PascalCase, `core/<area>/`):

| File | Holds |
|---|---|
| `core/PC1500/PC1500InterfaceMachine.hpp/.cpp` | `PC1500InterfaceMachine final : calcu::machine::Machine`; owns one `PC1500Machine` (§3.2); models PC-1500 and PC-1500A |
| `core/PC1600/PC1600InterfaceMachine.hpp/.cpp` | `PC1600InterfaceMachine final : calcu::machine::Machine`; owns one `PC1600Machine` |
| `core/Machine/SharpCatalogue.hpp/.cpp` | `sharpCatalogue()`: the `Catalogue` of LIB-1 AC-34–39, 41, built from the tables below and the module definitions (§3.1) |
| `core/Machine/SharpBuilder.hpp/.cpp` | `SharpMachineBuilder : calcu::machine::MachineBuilder` (§3.1) |
| `core/Machine/SharpKeyIds.hpp/.cpp` | the key IDs of AC-9 and AC-10 and their map to `PC1500Keyboard::Key` and the PC-1600 matrix; `on` separate (AC-11) |
| `core/Machine/SharpDisplayOutput.hpp/.cpp` | `PC1500Display` + `isDisplayOn()` and `PC1600DisplaySnapshot` → `DisplayOutput`; indicator names from `kStatusSymbolNames` (`core/Display/StatusLine.hpp:129-132`) |
| `core/Machine/SharpPlaces.hpp/.cpp` | the places, the core attach and detach call per place and peripheral, the product refusals (LIB-2 AC-29) (§3.4) |
| `core/Machine/PowerCycleSequence.hpp/.cpp` | the power cycle as a sequence over the following advances (§3.3) |
| `core/Machine/SharpProductKinds.hpp/.cpp` | the four product kinds of AC-19 and the adapter kinds of §5 |
| `core/Machine/LegacyCalls.hpp/.cpp` | adapters: core functions on `calcu::machine::Machine&` for the callers whose rows come later (§5) |
| `core/Machine/SharpRomImages.hpp/.cpp` | adapter: `RomImage`s read from the bundled folders through `BundledRoms` (§5) |
| `core/Display/DisplayOutputText.hpp/.cpp` | display as text from a `DotMatrixFrame` and the ROM font read through loader memory access (§3.5) |
| `tests/sharp_machine_tests.cpp` | C1600-1 AC-1–21, EC-1–3 (§6) |
| `tests/descriptor_tests.cpp` | LIB-1 AC-34–41 (LIB-1 design P4, P5) |
| `tools/check_machine_includes.sh` | the include check (Technical Requirements), a CTest of the Frequent group |

Changed:

| File | Change |
|---|---|
| `core/PC1500/PC1500Machine.hpp/.cpp`, `core/PC1600/PC1600Machine.hpp/.cpp` | `m_mutex` and every `lock_guard` go (ADR 0009; Technical Requirements); a key-readiness probe (AC-21); otherwise unchanged, so the core tests of AC-5 run as they are |
| `core/Connector/ExpansionCard.hpp` | `CardBase::powerLoss()` (default: nothing); `core/Connector/SoftwareDefinedCard.hpp` clears its RAM there when its definition has no `battery` (AC-6, behaviour change 1600-16) |
| `core/Job.hpp` | `job::run(calcu::machine::Machine&, Ticks)` next to today's template, calling `advance` |
| `app/MachineSession.hpp/.cpp` | `m_pc1500`, `m_pc1600`, `pc1500()`, `pc1600()`, `withMachine` go; one `std::unique_ptr<calcu::machine::Machine> m_machine`; every dispatch becomes a call on `Machine`, a capability, or a `LegacyCalls` adapter (§4) |
| `app/AppSession.cpp`, `app/FloppyBay.cpp`, `app/MachineCodeLoad.cpp`, `app/ModuleBay.cpp`, `app/CardYamlDump.cpp`, `app/HostKeyMap.cpp` | through `Machine`, capabilities and `LegacyCalls` (§4) |
| `app/DisplayFrame.hpp` | `toDisplayFrame(const DisplayOutput&)` (adapter, §5) |
| `app/MachineModel.hpp` | `modelIdOf`, `modelOf`, `variantIdOf` between the enums and `ModelId`/`VariantId` (adapter, §5) |
| `frontends/qt6/app/MachineController.hpp`, `MainWindow.cpp`, `debug/DebugController.cpp`, `ControlBar.cpp` (comment at 161) | no machine class named; the debugger through `LegacyCalls` |
| `app/tests/shared_*_tests.cpp` | the cases that used `pc1500()`/`pc1600()` go through `Machine` |
| `tests/golden_runs_tests.cpp` | the replay drives `calcu::machine::Machine` (§8) |
| `tools/pc1500_cli.cpp`, `pc1600_cli.cpp`, `pc1600_plotter_probe.cpp`, `pc1600_power_probe.cpp`, `pc1600_uart_probe.cpp`, `CliCommon.hpp`; `dev/loader-matrix/*_loader_matrix.cpp` | onto the interface (§7) |
| `CMakeLists.txt` | the CTest `CalcU1600MachineIncludeCheck` (label `frequent`); `docs/test-groups.md` gets its line |

## 3. Seams and interfaces

### 3.1 Catalogue and building (LIB-1 AC-34–39, 41; C1600-1 AC-1, AC-13, AC-15)

```cpp
// core/Machine/SharpCatalogue.hpp
const calcu::machine::Catalogue& sharpCatalogue();   // built once; module definitions from
                                                      // MemoryCardCatalog added with addPeripheral()
// core/Machine/SharpBuilder.hpp
class SharpMachineBuilder final : public calcu::machine::MachineBuilder {
public:
  calcu::machine::BuildResult build(const calcu::machine::BuildRequest&,
                                    const calcu::machine::Catalogue&) override;
};
```

- Models and variants (LIB-1 AC-34): `PC-1500` {A01, A03, A04; A04},
  `PC-1500A` {A04}, `PC-1600` {New, Old; New}. Tick rates from
  `kPC1500CpuHz` (`core/PC1500/PC1500Clocks.hpp:9`) and `kTStateHz`
  (`core/PC1600/PC1600Clocks.hpp:8`), so the descriptor and the core
  cannot differ (AC-1).
- Peripheral descriptors (LIB-1 AC-39): `CE-150`, `CE-158`, `CE-1600P`
  {New, Old; New} with the CE-1600F described with it, `host-drive`,
  and the nine modules of `frontends/qt6/resources/cards/*.card.yaml`,
  each `fits` from `compatible-hosts` (LIB-1 AC-41). The parsed
  `MemoryCardDefinition` rides as `ProductData`
  (`core/Connector/MemoryCardDefinition.hpp`).
- `build()` makes a `PC1500InterfaceMachine` (with `PC1500Variant` from
  the model) or a `PC1600InterfaceMachine`, loads the `RomImage`s the
  descriptor names (`loadROM`, `loadBank0`, `loadBank3Rom`, … as
  `BundledRoms` does today, `core/Resources/BundledRomCatalog.hpp`),
  then attaches `BuildRequest::peripherals` in request order (AC-17).
  It runs nothing: the caller makes the first operation.
- `BuildRequest::soundRate` other than 48,000 is refused with a reason:
  `PiezoSampler` could take another rate (`core/Audio/PiezoSampler.hpp:44`),
  but both machine classes fix it at `kDefaultSampleRate`
  (`PC1500Machine.hpp:140`, `PC1600Machine.hpp:362`), and AC-14 promises
  48,000 only (D-6).

### 3.2 The two machines (C1600-1 AC-1–14, 20, 21; EC-1, EC-2)

`PC1500InterfaceMachine` and `PC1600InterfaceMachine` implement LIB-1's
`Machine` (LIB-1 design §3.5) over the old class they own, which keeps
its name, its tests and its callers inside the core. The wrapper exists
because the old classes already have a `variant()` that means the
model (`PC1500Variant`), not the ROM variant.

| `Machine` | PC-1500, PC-1500A | PC-1600 | ACs |
|---|---|---|---|
| `advance(n)` | `runCycles(n)`; off or halted tick by tick (`core/PC1500/PC1500Machine.cpp:208-265`) | `runCycles(n)`; LH5803 in T-states (`toTStates`, `PC1600Machine.hpp:109`); off in steps of `kOffSliceTStates` = 256 (`:607`) | AC-1, AC-2, AC-3, EC-2 |
| `tickCount()` | sum of what `advance` returned since build; not reset by any operation | the same | LIB-1 AC-8 |
| `reset()` | `PC1500Machine::reset()` (RTC to 1 January 2000, `PC1500Memory.cpp:56`) | `PC1600Machine::reset()` (clock kept) | AC-6, AC-8 |
| `coldBoot()` | `allReset()`, then `powerLoss()` on each module | `allReset()` (ROM clears the clock), then `powerLoss()` on each module | AC-6, AC-8 |
| `powerCycle()` | starts `PowerCycleSequence` (§3.3) | the same | AC-6, AC-7 |
| `isKeyReady()` | PC in E243–E33D (`WAIT_4_KB`) and bit 0 of 7B0E clear, CPU not off | the last 15 whole frames in a row were ready (`kActiveFramesNeeded`, ~0.25 s), each as `waitForKeyboardScanLoopJob` decides it (`core/PC1600/PC1600BasicTyper.cpp:159-182`): CK0 on, and the SC7852 at the command loop or the matrix read once in the frame; CPU not off (D-3) | AC-21 |
| `keyDown/keyUp(id)` | key IDs of AC-9 through `SharpKeyIds`; `on` → `setOnKeyPressed`; anything else, `ent` and upper case included, `UnknownKey` | key IDs of AC-10; `on` the same | AC-9–12, EC-1 |
| `readDisplay()` | 156 × 7, 14 indicators; `on` = `isDisplayOn()` | 156 × 32, 18 indicators; `on` = `clockEnabled` | AC-13, AC-20, AC-3 |
| `takeSound(out)` | `drainAudio` | `drainAudio` | AC-14 |
| `plugged()`, `capabilities()` | §3.4, §3.6 | §3.4, §3.6 | AC-15–20 |
| `debugTargets()` | `LH5801` | `SC7852`, `LH5803` | LIB-1 AC-36 |

- The PC-1500's key IDs map onto the matrix by a table of its own, not
  `PC1500Keyboard::keyFromName` (which takes `ent` and upper case,
  `core/PC1500/PC1500Keyboard.cpp:32-71`); the core tests keep using
  `keyFromName`.
- `on` is held in the wrapper as a key like the others (AC-11): two
  keys of the matrix and `on` can be down together (AC-12).
- AC-7 needs no code of its own: the ROM's `off` path and the off states
  of the cores stay; the wrapper only reports them (AC-3).

### 3.3 Power cycle (AC-6, AC-7; LIB-1 AC-11)

```cpp
// core/Machine/PowerCycleSequence.hpp
class PowerCycleSequence {             // driven from advance(); no clock, no thread
public:
  void start(calcu::machine::Ticks frame);           // frame = tickRate / 60
  bool active() const;
  calcu::machine::Ticks nextChunk() const;           // ticks until the next phase change
  template <class Core> void onPhaseEnd(Core&);      // presses and releases off/on
};
```

- Phases, today's sequence moved from `app/MachineSession.cpp:244-257`:
  `off` down for `kKeyHoldFrames` = 4 frames, released; at most
  `kPowerOffTimeoutFrames` = 180 frames until the display is off; the
  attaches that wait for the off point (§3.4; D-2) are done; `on` down for
  4 frames, released. The constants move with it (`:38`, `:44`).
- `powerCycle()` returns at once. `advance` runs the sequence in chunks
  that end on its phase changes and still returns whole instructions.
  `isKeyReady()` is false while it runs, and becomes true by the ROM's
  own wait after the `on` key. Running to the prompt stays the
  caller's (LIB-18; until M-46 the adapter `runToPromptJob`, §5).
- On a machine that is already off, `powerCycle()` makes the pending
  change and the machine stays off, as today's `powerCycleAround`
  (`app/MachineSession.cpp:266-268`) does (D-4).
- There is no golden run of today's power cycle (FS 2 result). T0
  records two first, from the code as imported (§8, D-8).

### 3.4 Places and attach (LIB-2 AC-28–31; C1600-1 AC-15–18, EC-3)

| Model | Place | Pin-out | Takes | Core call today |
|---|---|---|---|---|
| PC-1500, PC-1500A | `slot-1` | PC-1500 memory slot | one module | `attachExpansionCard`, `expansionConnector().detach()` |
| | `printer` | 60-pin | chain | `attachCE150`/`detachCE150` |
| | `interface` | 60-pin | chain | `attachCE158`/`detachCE158` |
| PC-1600 | `slot-1`, `slot-2` | Slot 1, Slot 2 | one module each | `attachSlot1Card`/`attachSlot2Card`, `detachSlot1/2` |
| | `printer` | 60-pin | chain | `attachCE150` or `attachCE1600P` |
| | `interface` | 60-pin | chain | `attachCE158` |
| | `host-drive` | 60-pin | chain | `attachHostDrive`, `setHostDriveDirectory` |

- `SharpPlaces` implements LIB-2's protected hooks (`productRefusal`,
  `plug`, `unplug`, `followsNow`) behind the library's
  `canAttach`/`attach`/`detach`/`replace` on both machines (LIB-2 design
  §3.1; reconciled).
  A second printer in `printer` is refused by LIB-2's place rule; the
  CE-1600P with the CE-158 is refused by the core, where today
  `PC1600Machine.cpp:90-91, 265, 290` removes the other one (LIB-2
  AC-29). A module that does not fit by its definition is refused by
  LIB-2's fit rule (EC-3).
- What follows is the pin-out's: a power cycle on every Sharp pin-out
  (LIB-2 AC-28). An attach on a running machine waits for the off point
  of the power cycle that follows, as today's `powerCycleAround`
  (`app/MachineSession.cpp:261-275`) changes the bus while the machine
  is off. `plugged()` and the capabilities show the peripheral at once
  (D-2 = LIB-2 D-4).
- The order on the 60-pin chain is the order of the attaches, kept by
  `CardChain` (`core/Connector/CardChain.hpp:18-33`) (AC-17).
- The CE-1600P variant (AC-18): `replace` with the same place and the
  other variant; the CE-1600F disk and its side are kept by the adapter
  `FloppyDriveAccess` (§5) as `swapCE1600PRom` does today
  (`app/MachineSession.cpp:732-739`). One power cycle, the one `replace`
  answers; today only the detach power-cycles, so one as well (D-5).
- The host drive's ROM is always bundled (`roms.md` Q-1); its folder
  comes with the request as `ProductData` and stays a product kind.

### 3.5 Display as text (AC-4, AC-13)

```cpp
// core/Display/DisplayOutputText.hpp
LcdFont sharpLcdFont(calcu::machine::Machine&);        // ROM font through c1600.loader-memory
LcdText displayText(const calcu::machine::DisplayOutput&, const LcdFont&);   // parseLcdText on the dots
```

Today's text readers read the dots and the ROM font from the old classes
(`core/PC1500/PC1500LcdText.hpp`, `core/PC1600/PC1600LcdText.hpp`;
`core/Display/LcdText.hpp:117`). The same parse on `DotMatrixFrame`
lets the golden replay, the Copy Screen text and the CLIs read the
display through the interface. Where the PC-1600 font is read (bank 6)
must be reachable through loader memory access (§8 risk).

### 3.6 Capabilities (AC-19; LIB-2 AC-32–34)

Product kinds, defined in `core/Machine/SharpProductKinds.hpp`, IDs as
the LIB-1 design §3.7 composes them:

| Kind (`KindId`) | Owner | Offers (what the app layer calls today) | Stays |
|---|---|---|---|
| `c1600.loader-memory` | the main CPU component (`LH5801`, `SC7852`; D-1) | PC-1500: `memory().peek/poke`; PC-1600: `debugPeek`, `pokeMemory`, `mode1`, `programAreaTitle`, `debugWriteInternalRam`, `debugWriteSlotBus`, `debugSlotBusWritable`, `debugSlotImage`, `debugBankState` (`PC1600Machine.hpp:383-474`) | yes; C1600-2 AC-14 reshapes it in M-44 |
| `c1600.inspector` | the machine | `view(inspect::View)`, `menu(bool dumps)`, `slotImage(slot, bankCount)` (`app/MachineSession.cpp:537-561`) | yes |
| `c1600.ce158-printer-port` | CE-158 in `interface` | `drain()` (`drainCE158ParallelOutput`) | yes |
| `c1600.host-drive-folder` | host drive in `host-drive` | `folder()`, `setFolder(path)` | yes |

Generic kinds of LIB-2 (assumed, §9): serial port (PC-1600 RS-232C,
owner the PC-1600's UART component `rs232c` (D-10), `setSerialLink`; CE-158, owner the
peripheral, `setCE158SerialLink`); real-time clock (all three models,
`seedClock`; LIB-2 AC-34); controls (CE-161 write-protect switch,
`ExpansionCard.hpp:159-165`; LIB-2 AC-32). Media place capabilities
(module slots, CE-150 and CE-1600P cassettes, CE-1600F) come with LIB-7
1600 in M-47 (LIB-1 D-1); M-26 lists the media places in the
descriptors only. That is enough for AC-16 (the CE-150's descriptor
lists its cassette, and `plugged()` shows the CE-150), and LIB-1 AC-40's
media place clause passes in M-47.

Threading and ownership: the machine session owns the machine in one
`std::unique_ptr` and calls it from one thread (ADR 0009). After the
locks go, the reader and writer threads of the host link remain the
only threads: the host adapter at the boundary. In M-26 LIB-2 moves
`core/Serial/PtySerialLink.*` into the library as
`calcu::peripherals::PtyHostLink` (same threads, `PtySerialLink.cpp:98-99`
today) and deletes the product's copy (reconciled with LIB-2 design §2).

## 4. Product side (Calc-U-1600)

`app/MachineSession.cpp`, by the groups of the Technical Requirements:

| Group today | After M-26 |
|---|---|
| building a machine and its ROM set (`switchModelJob`, `makePC1600WithRomFallback`, `resetBareForPreset…`) | `calcu::machine::build(sharpCatalogue(), builder, request)`; ROM images through `SharpRomImages` (§5); the Old-ROM fallback stays until M-27 (LIB-12 AC-13) |
| reset level and booting (`resetToPromptJob`) | `reset()` / `coldBoot()`, then `LegacyCalls::runToPromptJob` |
| power state and power cycle (`isMachinePoweredOn`, `powerCycleAround`) | `readDisplay().on`; `powerCycle()` (§3.3), then `runToPromptJob` |
| built-in serial port (`wireNewMachine`) | the serial port capability |
| keys and typing (`machinePressKey`, `setOnKeyPressed`, `tapKey`, `enqueueKey`, paste latches) | `keyDown`/`keyUp`; `on` as a key ID; PC-1500 queue through `c1600.key-queue`; latches through `LegacyCalls::readLatches` (§5) |
| clock rate (`clockHz`) | `descriptor().tickRate` |
| display, text and image (`currentDisplay`, `lcdText`, `currentScreenImage`) | `readDisplay()`, `toDisplayFrame` (§5), `displayText` (§3.5); the image from the dots |
| debugger and INSPECTOR (`inspectorView`, `inspectorMenu`, `debugSlotCardImage`, trace) | `c1600.inspector`; trace through `c1600.trace` (§5) |
| one or two slots (`slotCount`, `slotCard`, `attachSlotCard`, `detachSlots`, `setSlotWriteProtected`) | places and `plugged()`; the switch through the controls capability; the card object through `LegacyCalls::moduleIn` (§5) |
| peripherals only the PC-1600 has, the tape interface | attach and detach in places; tape, plotter paper, floppy through the adapter kinds (§5) |
| the same call on either class (27) | calls on `Machine` |

Deleted: `pc1500()`, `pc1600()`, `hasLiveMachine()` (becomes
`m_machine != nullptr`), both `withMachine` templates, the template
`powerCycle` in the anonymous namespace (moved into the core), the
locks of both machine classes. The other callers: `app/AppSession.cpp`
(preset loaders `:235-257`, loaders `:388-406`) and
`app/MachineCodeLoad.cpp:36, 117` call `LegacyCalls`;
`app/FloppyBay.cpp` uses `FloppyDriveAccess`; `app/ModuleBay.cpp`
(`CardHost` from the model) uses the places; the Qt
`MachineController` hands out `calcu::machine::Machine*`;
`DebugController.cpp:127, 241` call `LegacyCalls::makeDebugTarget` and
`loadProgram`.

Stays: `PC1500Machine`, `PC1600Machine`, the loaders, preset loaders,
typers and debug targets, all inside the core and called with the old
class from core code. `shared::Model` and the ROM version enums stay
behind their adapter (§5).

Calc-U-59: nothing changes.

## 5. Adapters

Each wraps an old call that a later row takes behind its seam. No
adapter is left after M-51 (plan §1; M-51 runs the adapter check).

| Adapter | File | Wraps | Removed in |
|---|---|---|---|
| `SharpRomImages` | `core/Machine/SharpRomImages.*` | `BundledRoms::resolveBundledRomPath` and the cached read, giving `RomImage`s for a `BuildRequest` | M-27 (LIB-12 1600) |
| `c1600.key-queue` kind (`KeyQueue`) | `core/Machine/SharpProductKinds.*` | `PC1500Machine::enqueueKey` and its cadence (`PC1500Machine.cpp:342-377`) | M-36 (LIB-5: the queue goes, AC-32 withdrawn) |
| `LegacyCalls::readLatches` | `core/Machine/LegacyCalls.*` | `pc1600ReadLatches` (`app/MachineSession.cpp:458`) | M-36 (typing support, LIB-5 AC-35) |
| `LegacyCalls::loadBasicProgram`, `loadMachineCode`, `machineCodeLoadState`, `userRam` | same | `::loadBasicProgram`, `loadPC1500/1600MachineCode` (`app/AppSession.cpp:388-406`), `pc1600LoadState`, `pc1500UserRam` (`app/MachineCodeLoad.cpp:36, 117`) | M-44 (C1600-2) |
| `c1600.plotter-paper` kind | `core/Machine/SharpProductKinds.*` | `ce150PlotPoints/Revision`, `ce1600pPlotPoints/Revision`, `clear…Paper`, `drain…Events` | M-45 (LIB-8 1600) |
| `LegacyCalls::makeDebugTarget`, `loadProgram` | `core/Machine/LegacyCalls.*` | `debug::makeDebugTarget(machine)`, `debug::loadProgram(pc1500, pc1600, …)` | M-45 (LIB-10 1600) |
| `c1600.trace` kind | `core/Machine/SharpProductKinds.*` | `beginCpuTrace`, `endCpuTrace`, `cpuTraceBytes`, `cpuTraceActive` | M-45 (LIB-10 1600, AC-42, AC-43) |
| `LegacyCalls::applyPresetJob` | `core/Machine/LegacyCalls.*` | the bare machine and `PC1500/PC1600PresetLoader` (`app/AppSession.cpp:235-257`), bus ROMs (`attachBusRom`) | M-46 (LIB-6 1600, C1600-4) |
| `LegacyCalls::runToPromptJob` | same | `runBootToPromptJob` of both typers | M-46 (LIB-18 1600) |
| `c1600.tape-deck` kind | `core/Machine/SharpProductKinds.*` | `tapePlay`, `tapeRecord`, `tapeEject`, `tapeStatus`, `tapeTakeLastError` | M-47 (LIB-7 1600) |
| `c1600.floppy-drive` kind (`FloppyDriveAccess`) | same | `ce1600fDiskImage`, `…Revision`, `…Eject`, `…HasDisk`, `…LoadImage`, `…Side`, `…SetSide`, `…MotorOn` | M-47 |
| `LegacyCalls::moduleIn` | `core/Machine/LegacyCalls.*` | the `const ExpansionCard*` in a slot for `ModuleBay` (`app/MachineSession.cpp:565-569`) | M-47 |
| `toDisplayFrame` | `app/DisplayFrame.hpp` | today's `shared::DisplayFrame`, built from `DisplayOutput` | M-49 (LIB-14 1600: the frontends paint display output) |
| `modelIdOf`, `modelOf`, `variantIdOf` | `app/MachineModel.hpp` | `shared::Model`, `PC1600RomVersion`, `CE1600PRomVersion`, `modelKey` | M-49: the last users are the frontends and `app/ControlState`, `KeyLayout/FaceplateLayout`, `LcdLayout` (§9 D-7) |
| the CLIs and probes on `LegacyCalls` and the adapter kinds | `tools/*.cpp` | §7 | M-51 (the tools go) |

`LegacyCalls` reaches the old class by `descriptor().model` and a
`static_cast` to the wrapper, inside the core: no RTTI, and no file
outside the core names a machine class.

## 6. Tasks

Tests before code in each task. Test files under `tests/`, in the
`CoreTests` executable, group Frequent, names
`c1600_1_<ac>_<what>`, LIB-1's `lib1_<ac>_…` (LIB-1 design §6).

| Task | What | ACs/ECs | Test (file, case, group) |
|---|---|---|---|
| T0 | Before any core change: record the power cycle of today as two golden runs (§8; D-8) | AC-6 (power cycle row) | `golden-runs/pc1500a-power-cycle.run`, `pc1600-power-cycle.run`, replayed by `test_golden_runs_of_the_sharp_cores`; Frequent |
| T1 | Catalogue and builder | LIB-1 AC-34–39, AC-41; AC-1 (rates) | `descriptor_tests.cpp`: `lib1_ac34_models_variants_rates`, `lib1_ac35_dot_matrix_indicators_sound`, `lib1_ac36_connectors_pinouts_places_debug_targets`, `lib1_ac37_module_slots_and_components`, `lib1_ac38_rom_entries_per_variant`, `lib1_ac39_peripheral_descriptors`, `lib1_ac41_module_fit_table` |
| T2 | Advance and tick count | AC-1, AC-2, AC-3, EC-2 | `sharp_machine_tests.cpp`: `c1600_1_ac1_tick_is_cpu_cycle_or_t_state`, `c1600_1_ac2_whole_instructions_of_the_bus_owner`, `c1600_1_ac3_off_machine_counts_ticks_clock_runs_display_off`, `c1600_1_ec2_off_steps_256_on_1600_tick_by_tick_on_1500` |
| T3 | Operations, `powerLoss()`, power cycle sequence | AC-6, AC-7, AC-8 | `c1600_1_ac6_reset_keeps_memory`, `c1600_1_ac6_power_cycle_keeps_all_memory`, `c1600_1_ac6_cold_boot_empties_memory_and_ce151_ce155`, `c1600_1_ac6_cold_boot_keeps_battery_module`, `c1600_1_ac6_operations_end_at_key_readiness`, `c1600_1_ac7_off_key_then_on_runs_again`, `c1600_1_ac8_rtc_after_reset_and_cold_boot` |
| T4 | Key IDs, `on` | AC-9, AC-10, AC-11, AC-12, EC-1 | `c1600_1_ac9_pc1500_has_65_key_ids`, `c1600_1_ac10_pc1600_has_68_key_ids`, `c1600_1_ac11_on_switches_on_and_breaks`, `c1600_1_ac12_two_keys_and_on_down_together`, `c1600_1_ec1_unknown_key_refused` (`kbii` on PC-1500, `ent`, `A`) |
| T5 | Key readiness | AC-21 | `c1600_1_ac21_pc1500_ready_in_wait_4_kb_with_7b0e_bit0_clear`, `c1600_1_ac21_pc1500_not_ready_while_bit0_set_or_running`, `c1600_1_ac21_pc1600_ready_after_15_ready_frames` (true at the same frame as `waitForKeyboardScanLoopJob` returns on the old class, with a CE-1600P) |
| T6 | Display and sound | AC-13, AC-14 | `c1600_1_ac13_dots_and_indicator_names_per_model`, `c1600_1_ac14_buzzer_48000_mono_same_at_any_slicing` |
| T7 | Places, attach, refusals, chain order, CE-1600P variant | AC-15, AC-17, AC-18, EC-3 | `c1600_1_ac15_each_peripheral_only_through_places`, `c1600_1_ac17_first_peripheral_that_answers_wins_in_attach_order`, `c1600_1_ac18_ce1600p_variant_keeps_memory_and_disk_side`, `c1600_1_ec3_ce1601m_in_slot1_refused` |
| T8 | Capabilities and product kinds | AC-16, AC-19, AC-20 | `c1600_1_ac16_ce150_cassette_listed_on_pc1500` (the media place in the descriptor; LIB-1 D-1), `c1600_1_ac19_product_kinds_with_owners`, `c1600_1_ac20_display_on_slots_cassette_one_way_on_every_model` |
| T9 | Conformance of the running machines | LIB-1 AC-40 | `descriptor_tests.cpp`: `lib1_ac40_machines_conform` (`checkAgainstDescriptor`, every model and variant, each peripheral of AC-39) |
| T10 | Golden replay through the interface | AC-4 | `golden_runs_tests.cpp`: `test_golden_runs_of_the_sharp_cores` (renamed `c1600_1_ac4_golden_runs_through_the_interface`) |
| T11 | Core tests unchanged; app tests on `Machine` | AC-5 | the existing `lh5801_tests`, `lh5803_tests`, `sc7852_tests`, `pc1600_machine_tests`, `pc1600_bank_tests`, `pc1600_keyboard_display_tests`, slot, connector, CE-150, CE-158, CE-1600P, CE-1600F, host drive, module, tape and `piezo_sampler_tests` pass without change; `app/tests/shared_*` moved |
| T12 | `MachineSession`, `AppSession`, bays, Qt and Web callers on `Machine` and the adapters | (behaviour of AC-4, AC-5 in the app) | `app/tests/shared_machine_session_tests.cpp`, `shared_app_session_tests.cpp`, `shared_bay_tests.cpp` as today; the Longer group at the end of Part C1 (screenshot scenarios, Playwright, `tools/dap_smoke.py`) |
| T13 | Include check | Technical Requirements (I-1) | `tools/check_machine_includes.sh` as CTest `CalcU1600MachineIncludeCheck`, Frequent; in CI with the core tests |
| T14 | Tools onto the interface (§7) | plan §1 "Today's tools" | `tools/check_presets.sh` (Frequent) passes; M-4's references made again with the moved CLIs and compared (`tests/reference/v0.8.1-examples/`) |
| T15 | LIB-2 1600 criteria due in M-26 | LIB-2 AC-28–31, AC-34; the M-26 parts of AC-32, AC-33 | the cases of the LIB-2 design §6 P5–P11 (`lib2_ac…`) in `tests/peripherals_tests.cpp` and `app/tests/serial_links_tests.cpp` (reconciled with LIB-2 design) |

Coverage: C1600-1 AC-1–21 and EC-1–3, 24 of 24, in T2–T10 (AC-4 T10,
AC-5 T11). LIB-1 AC-34–41, 8 of 8, in T1 and T9. LIB-2 AC-28–34 are
served by T7, T8 and T15; their cases are the LIB-2 design's.

## 7. Tools and agent knowledge

**Tools** (plan §1; M-26 "Does"). They build machines with
`sharpCatalogue()` and `build()`, advance with `advance`, press with
`keyDown`/`keyUp`, read with `readDisplay`/`displayText`/`takeSound`,
and reach what the interface lacks through the adapter kinds and
`LegacyCalls` until M-51:

| Tool | Moves to |
|---|---|
| `tools/pc1500_cli.cpp`, `tools/pc1600_cli.cpp` (+ `CliCommon.hpp`, `Ce158CliPeer.hpp`) | interface; presets through `LegacyCalls::applyPresetJob` (M-46 moves them to the library's loader); the WAV yield hook (`setYieldHook`, `pc1500_cli.cpp:183`) becomes a drain between `advance` calls |
| `tools/pc1600_plotter_probe.cpp` | interface; paper through `c1600.plotter-paper` |
| `tools/pc1600_power_probe.cpp`, `pc1600_uart_probe.cpp` | interface for build, keys, advance; CPU registers, banks, memory through `c1600.inspector` and `c1600.loader-memory`; `step()` becomes `advance(1)`; still on `calcu_core_probe` (`CMakeLists.txt:104-111`) |
| `dev/loader-matrix/pc1500_loader_matrix.cpp`, `pc1600_loader_matrix.cpp` | interface and `LegacyCalls` (outside the core, so the include check covers them; D-9) |
| `tools/check_presets.sh`, `tools/dap_smoke.py`, `tools/wasm_smoke.cpp` | unchanged; they drive the CLIs, the Qt app and the app layer |

**Renames** (`docs/glossary-mapping.md`, "Code names", in the same
step): Cold boot, Calc-U-1600 `resetJob(allReset = true)` →
`Machine::coldBoot()`; Reset → `Machine::reset()`; Model adds
`calcu::machine::ModelId` (`shared::Model` until M-49); Harness row
unchanged until M-51.

**Agent knowledge that changes in M-26:**
- `products/calc-u-1600/CLAUDE.md`: the app layer reaches the machine
  only through `calcu::machine::Machine`; where the descriptors and the
  adapters live (`core/Machine/`); the include check.
- `products/calc-u-1600/docs/developer/PC1600-Core-Limitations.md`
  (names `PC1600Machine` calls), `Shared-Architecture.md` (the session
  owns a `Machine`; `pc1500()`/`pc1600()` gone), `LCD-Text.md` (text
  from display output, §3.5), `headless/README.md` (the CLIs build from
  the catalogue).
- Memory files of Calc-U-1600 that name the removed calls (`allReset`,
  `withMachine`, `pressKey`, `MachineSession` dispatch):
  `calc-u-1600/project_ce1600p_paper_geometry.md`,
  `project_ce1601m_ramdisk_init_bug.md`,
  `project_ce150_pc1600_lprint_bug.md`,
  `project_fast_basic_binary_loader.md`,
  `project_pc1600_native_basic_and_preset_program.md`,
  `project_pc1600_serial_port.md`,
  `project_pc1600_rtc_power_off_lag_fixed.md`,
  `project_pc1600_ramdisk_geometry_table.md`,
  `project_shared_layer_wasm.md` (found by a search on 2026-11-14;
  searched again in the step).
- Ledger rows made visible: 1600-4 (a module change power-cycles),
  1600-16 (Cold boot empties CE-151, CE-155).
- `docs/background/` plans stay history; the include check skips
  `docs/`.

## 8. Golden runs and risks

**Golden runs.** The 20 runs of `tests/golden-runs/` (PC-1500: switch-on,
A01; PC-1500A: switch-on, basic, beep, error, reset-allreset, CE-150,
CE-151, CE-158; PC-1600: switch-on, basic, beep, error, reset-allreset,
CE-150, CE-158, CE-1600M, CE-1600P, old ROM) replay through the
interface after M-26, each command mapped so:

| Script | Through the interface |
|---|---|
| `model`, `rom`, `attach`, `card` | `BuildRequest` (model, variant, peripherals in script order) |
| `boot` | `build()`, `coldBoot()`, `runToPromptJob` |
| `keys`, `type` | PC-1600: `keyDown`, `advance(0.08 s)`, `keyUp`, `advance(0.08 s)`; PC-1500: `c1600.key-queue`, `advance(0.2 s)` |
| `press`, `release`, `run`, `reset`, `allreset` | `keyDown`/`keyUp`, `advance(tickRate × s)`, `reset()`, `coldBoot()` + `runToPromptJob` |
| display, status, audio, paper | `displayText(readDisplay(), font)`, its indicators, `takeSound`, `c1600.plotter-paper` |
| memory at the end | `c1600.loader-memory` (PC-1500 ME0 0000–FFFF; PC-1600 internal RAM and Z-80 view) |

A difference is a finding, never a new reference.

**Risks.**
- **No reference for the power cycle** (LIB-1 design §8): T0 records one
  from the code as imported, before the core changes, with the
  sequence of `app/MachineSession.cpp:244-257` in the recorder (D-8),
  under the rule "golden runs are recorded before a core is touched".
- **Cold boot and CE-151** (`pc1500a-ce151`): `powerLoss()` clears the
  module before the boot run. If its fresh RAM is not what `powerLoss()`
  writes, the memory sum differs. That would be behaviour change
  1600-16, still a finding to report, not a new reference.
- **Attach at the off point** (§3.4, D-2): if the change happens at
  once instead, a peripheral that the ROM talks to while it switches
  off (CE-150, CE-1600P) may see a different sequence than today.
- **Locks removed**: callers that today rely on a lock from another
  thread. Calc-U-1600 is single-threaded (`docs/background/Decisions.md`,
  "Shared is single-threaded"); the threads of `PtyHostLink` (LIB-2,
  from `PtySerialLink`) touch the link's queues only, to be checked in
  the step.
- **Text through the interface** (§3.5): the PC-1600 font is in bank 6;
  if loader memory access cannot read it without side effects, the
  text of the golden replay differs. Not verified.
- **Emscripten exception mode** (LIB-1 D-6): `calcu_core` builds with
  `-fwasm-exceptions` (`CMakeLists.txt:59`), as `calcu_library` does in
  `build/web`; `calcu_web` links both. Nothing changes here.
- **Run to the prompt stays old until M-46**: M-46 replaces
  `runToPromptJob` by LIB-18's run to key readiness; the tick at which a
  boot ends may move then. That is M-46's golden question.
- **Allocation in `advance`**: the power cycle sequence and the wrapper
  add no allocation per call.

## 9. Decided (best guess)

- **D-1 (was OQ-1; AC-19, C1600-2 AC-14).** Loader memory access is
  owned by the main CPU component (`LH5801`, `SC7852`), which stands for
  the machine itself; LIB-1's `Owner` needs no third form.
- **D-2 (was OQ-2 = LIB-2 OQ-4).** An attach on a running Sharp machine
  shows at once in `plugged()` and the capabilities and reaches the bus
  at the off point of the power cycle that follows, as today's
  `powerCycleAround`.
- **D-3 (was OQ-3; AC-21).** PC-1600 key readiness follows today's code:
  15 ready frames in a row, as `waitForKeyboardScanLoopJob` waits; its
  wait only with a plotter stays the typer's until LIB-5 (M-36).
- **D-4 (was OQ-4; AC-6).** `powerCycle()` on a machine that is off
  makes the change and leaves it off, as today.
- **D-5 (was OQ-5; AC-18).** The CE-1600P variant change is one
  `replace`, so one power cycle; today's detach and attach give one too.
- **D-6 (was OQ-6; AC-14).** A sound rate other than 48,000 is refused at
  build: today's machine classes fix the rate.
- **D-7 (was OQ-7).** The enum adapter goes in M-49 (closed before, LIB-1
  design §5).
- **D-8 (was OQ-8).** T0 records the two power cycle golden runs from the
  code as imported, before the core is touched (CLAUDE.md rule).
- **D-9 (was OQ-9).** The include check covers `app/`, `frontends/`,
  `tools/`, `dev/` and skips `core/`, `tests/`, `docs/`;
  `dev/loader-matrix` moves onto the interface (the draft's proposal).
- **D-10 (was OQ-10 = LIB-2 OQ-11).** Component IDs `rs232c` (PC-1600
  UART) and `rtc` (real-time clock, all three models), as the M-26
  descriptors write them.
- **Not verified.** That `SoftwareDefinedCard` is the class of every
  RAM module (CE-151, CE-155); that the PC-1500's `runCycles` already
  overshoots by less than the last instruction (AC-2); the exact
  component IDs of LIB-1 AC-37 (from the fit table of `machine.md`, as
  the LIB-1 design leaves them).

**Assumed from LIB-2's design (drafted at the same time):**
(Reconciled with the LIB-2 design: A-1 to A-5 hold in the forms below;
A-6 follows LIB-2's split.)
- A-1: `Machine` gets `canAttach(const PeripheralRequest&, const
  Catalogue&) -> std::optional<Refusal>`, `attach(PeripheralRequest,
  const Catalogue&) -> ChangeResult`, `detach(const PlacePath&, const
  PeripheralId&) -> ChangeResult`, `replace(...)`, with
  `ChangeResult::follows` saying what follows (`AfterChange`), and the
  protected hooks the core overrides (LIB-2 design §3.1).
- A-2: A `Refusal` carries a reason a core can set (CE-1600P with
  CE-158, LIB-2 AC-29); the place rule ("takes one") and the fit rule
  (EC-3) are checked by the library before the core is asked.
- A-3: Kinds in `calcu::peripherals`: `SerialPort` (with a host link
  interface the machine session connects; the host link files of AC-33
  are M-31's), `RealTimeClock` (set from the host; LIB-2 AC-22, AC-23),
  `Controls` (named controls with a position; LIB-2 AC-32).
- A-4: A power cycle after an attach is run by the caller (LIB-18;
  today's session until M-46) with `Machine::powerCycle()`; the library
  does not run it inside `attach`.
- A-5: Places on the 60-pin connector are flat (`printer`, `interface`,
  `host-drive`), as P-6 keeps today's implicit connectors implicit; no
  place on a peripheral is needed in M-26.
- A-6: The split of LIB-2 1600, as the LIB-2 design §1 and §6 make it:
  M-26 tests AC-28, 29, 30, 31, 34, of AC-32 the controls capability and
  the blocked write, and of AC-33 the serial port files through the
  product's machine session (LIB-2's adapter "product-held host links");
  M-31 tests AC-33 again through the library's app session; M-47 the
  switch set with a save (AC-32, LIB-7 AC-19).

**Names other designs must match:** `PC1500InterfaceMachine`,
`PC1600InterfaceMachine`, `sharpCatalogue()`, `SharpMachineBuilder`; the
product kinds `c1600.loader-memory`, `c1600.inspector`,
`c1600.ce158-printer-port`, `c1600.host-drive-folder`; the adapter
kinds `c1600.key-queue` (LIB-5, M-36), `c1600.plotter-paper` (LIB-8,
M-45), `c1600.trace` (LIB-10, M-45), `c1600.tape-deck` and
`c1600.floppy-drive` (LIB-7, M-47); `LegacyCalls` functions per §5;
places `slot-1`, `slot-2`, `printer`, `interface`, `host-drive`.
