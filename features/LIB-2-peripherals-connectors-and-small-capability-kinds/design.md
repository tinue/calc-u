# LIB-2 Peripherals, connectors and small capability kinds: design
Status: approved by the user at the Part B/C1 boundary (2026-11-14)

## 1. Scope

All three columns are worked in Part C1. The Calc-U-1600 column is Done
only in Part D (plan §5.1).

| Column | Step | Tests from (plan §3) | Done in (plan §5.1) |
|---|---|---|---|
| Library | M-23 | AC-1–23, AC-35; EC-1–7 | M-23 |
| Calc-U-59 | M-25 (with C59-6) | AC-24–27 | M-25 |
| Calc-U-1600 | M-26 (with C1600-1), M-31, M-47 | AC-28–34 | M-47 |

- **M-23** changes only `library/` and `docs/test-groups.md`. No product
  code calls the new members until M-25. Waits for M-18 (machine
  interface), M-20 (the folder for serial links, LIB-9), M-22 (the app
  session that gives a host link, LIB-3). The pseudo-terminal tests run on
  macOS and Linux only (plan M-23; AC-18, AC-19, AC-20, EC-7). EC-5 runs
  everywhere.
- **M-25** (C59-6): the PC-100C and the 14 modules plugged in through
  `attach`/`detach`; the PC-100C's keys and switch through the controls
  kind. Waits for M-8 (FS 8, FS 9), M-23, M-24. FS 8: without the PC-100C
  the ROM executes no print instruction, so the printer can leave the
  processor freely. FS 9: an empty `slot-1` blinks `0.` (golden runs
  `ti59-no-module`, `ti58-no-module`). Both results stand, and nothing is
  re-planned.
- **M-26** (C1600-1): every Sharp peripheral through `attach`/`detach`/
  `replace`; the RS-232C port and the CE-158 through the serial port
  kind; the clock through the real-time clock kind; the CE-161 switch
  through the controls kind. M-26 tests AC-28–31 and AC-34 in full. It
  tests AC-32 and AC-33 in part:
  - AC-33: the host links are held by the product's machine session (an
    adapter, §5). They pass to the library's app session in **M-31**
    (LIB-3 1600), where AC-33 is tested again and passes (plan §2.1:
    "AC-33 with LIB-3 1600").
  - AC-32: M-26 tests the controls capability and the blocked writes.
    The save that sets the switch, the power cycle after it, the position
    at attach from the saved file and the media place's report come with
    LIB-7 1600 in **M-47** (LIB-7 AC-19). The column is Done there.
- Not here: the attach and detach commands and running the power cycle as
  an operation (LIB-18, M-35/M-46/M-54), "available now" in the control
  state (LIB-4), `syncclock` and `switch` (LIB-6), media places (LIB-7),
  harness serial commands (a Calc-U-1600 product command, LIB-11/H-2).

## 2. Modules and files

**Library, M-23.** The attach members live on `calcu::machine::Machine`
(LIB-1 §3.5 reserves them), so their code is in `library/machine/` (D-2).
The generic kinds and the host link are a new folder,
`library/peripherals/` (namespace `calcu::peripherals`), part of
`calcu_library` as LIB-1 §2a expects; it uses `machine` only.

| File | Holds |
|---|---|
| `library/machine/Attach.hpp`, `.cpp` (new) | `Refusal`, `ChangeResult`, the generic rules of an attach, the bodies of the four `Machine` members (§3.1) |
| `library/machine/Machine.hpp` (changed) | the four public members and three protected hooks (§3.1) |
| `library/machine/Descriptor.hpp` (changed) | `PeripheralRequest::switches` (§3.1, D-3); the meaning of `PinOutDescriptor`'s fields as comments |
| `library/machine/Catalogue.hpp/.cpp` (changed) | `Catalogue::place(const PeripheralId&, const PlaceName&)` for a place on a peripheral (AC-9; D-11) |
| `library/machine/tests/TestMachines.hpp/.cpp` (changed) | additions for this feature (§6, T2) |
| `library/machine/tests/attach_tests.cpp` (new) | AC-1–11, AC-35, EC-1–4 |
| `library/peripherals/Controls.hpp/.cpp` | kind `Controls`; `ControlTable`, a ready-made implementation for cores (§3.2) |
| `library/peripherals/SerialPort.hpp/.cpp` | kind `SerialPort`; `SerialPortQueues`, a ready-made implementation for cores (§3.3) |
| `library/peripherals/RealTimeClock.hpp/.cpp` | kind `RealTimeClock`; `HostDateTime`, `hostDateTime()`, `setClocksFromHost()` (§3.4) |
| `library/peripherals/HostLink.hpp`, `PtyHostLink.cpp`, `NoHostLink.cpp` | `HostLink`, the pseudo-terminal behind a serial port file; the successor of Calc-U-1600 `core/Serial/PtySerialLink.*` (§3.5) |
| `library/peripherals/HostLinks.hpp/.cpp` | `HostLinks`, the set of host links an app session holds (§3.5) |
| `library/peripherals/tests/*_tests.cpp` | §6; in `calcu_library_tests`, but `pty_tests.cpp` |
| `library/CMakeLists.txt` (changed) | the sources of `library/peripherals/` in `calcu_library`; `CALCU_HAS_PTY`; a second CTest entry `calcu_library_pty_tests` (label `frequent`, registered only where `CALCU_HAS_PTY`), a small program of `pty_tests.cpp` linked with `calcu_library` and `calcu_test_runner` |
| `docs/test-groups.md` (changed) | the new tests and the entry `calcu_library_pty_tests` |

`CALCU_HAS_PTY` is set when `(APPLE OR UNIX) AND NOT EMSCRIPTEN`, which
is today's guard (`products/calc-u-1600/core/Serial/PtySerialLink.cpp:3`).
Without it, `NoHostLink.cpp` is compiled instead of `PtyHostLink.cpp`.

**Calc-U-59, M-25.** C59-6 names the peripheral classes. The files this
feature touches:

| File | Change |
|---|---|
| `products/calc-u-59/core/TIMachine.hpp/.cpp` (C59-1's TI machine) | implements `plug`/`unplug` for `slot-1` and `printer` (C59-6 §4); `setPrinterConnected`, `loadLibrary`, `pressPrinterPrint`, `pressPrinterAdv`, `setPrinterTrace` of `core/TI59Machine.hpp:15, 89-94` go with C59-1's `TIPeripheralsAdapter` (reconciled with C59-1, C59-6 designs) |
| PC-100C peripheral (C59-6's file, in `products/calc-u-59/core/`) | owns a `calcu::peripherals::ControlTable` with `PRINT`, `ADV` (keys), `TRACE` (switch) |
| `products/calc-u-59/app/SessionController.cpp/.hpp` | the printer and module calls become adapters (§5) |
| `products/calc-u-59/tools/headless/capi.cpp`, `ti59.py` | printer buttons through `Controls`, connect through `attach`/`detach` |
| `products/calc-u-59/tests/peripherals_tests.cpp` (new) | AC-24–27 |

**Calc-U-1600, M-26.**

| File | Change |
|---|---|
| `products/calc-u-1600/core/PC1500/PC1500Machine.*`, `core/PC1600/PC1600Machine.*` | implement the hooks; places `slot-1`, `slot-2` (PC-1600), `printer`, `interface`, `host-drive` (PC-1600); the silent detaches at `PC1600Machine.cpp:89-91, 264-265, 289-290` go (AC-29) |
| `products/calc-u-1600/core/Serial/SerialPortCapability.hpp/.cpp` (new) | implements the core's `SerialLink` (`core/Serial/SerialLink.hpp`, the UART's peer, unchanged) over `calcu::peripherals::SerialPortQueues` |
| `products/calc-u-1600/core/Serial/PtySerialLink.*`, `LazySerialLink.hpp` | deleted. The library's `PtyHostLink` and `HostLinks` replace them |
| `products/calc-u-1600/core/HostClock.hpp` | deleted. `seedClockFromHostTime` becomes `calcu::peripherals::setClocksFromHost`; `hostClockOverride` becomes the host-time source of the session (§9, A-4) |
| RTC capability (in the machine classes, C1600-1's file) | `RealTimeClock` over `Upd1990ac::seedFromHost` (PC-1500) and the LU-57813P of `PC1600SubCpu` (PC-1600); read added |
| CE-161 (`core/Connector/SoftwareDefinedCard` with `hasWriteProtectSwitch()`) | a `ControlTable` with the switch `write-protect` (D-9) over `CardBase::setWriteProtected` (`core/Connector/ExpansionCard.hpp:159-165`) |
| `products/calc-u-1600/app/MachineSession.cpp/.hpp`, `AppSession.cpp`, `ModuleBay.cpp` | calls go through the machine interface and the kinds, behind adapters (§5) |
| `products/calc-u-1600/frontends/qt6/app/MachineController.hpp`, `SettingsDialog.cpp`, `EmulationPacer.cpp` | serial status from `HostLinks::status()`; clock through the adapter |
| `products/calc-u-1600/tools/Ce158CliPeer.hpp`, `tools/pc1600_uart_probe.cpp` | onto `PtyHostLink` and the serial port capability |
| `products/calc-u-1600/tests/peripherals_tests.cpp` (new) | AC-28–34 |

## 3. Seams and interfaces

Signature sketches, as in LIB-1 §3. They use LIB-1's types: `Machine`,
`Catalogue`, `PeripheralRequest`, `PlacePath`, `Plugged`,
`PinOutDescriptor`, `AfterChange`, `Capability`, `KindId`, `Owner`,
`CapabilityEntry`, `capabilityAs<K>`, `Ticks`, `TickRate`.

### 3.1 Attach, detach, replace (`machine/Attach.hpp`, `Machine.hpp`)

The pin-out data is LIB-1's (`Descriptor.hpp:216-221`). This feature fixes
what it means (AC-1):

| Field | Meaning |
|---|---|
| `PinOutDescriptor::level` | `ContactLevel` or `Functional`. Read by nobody in the library; descriptive (P-4, I-4) |
| `PinOutDescriptor::takes` | `One`: the connector holds one peripheral over all its places. `Chain`: it holds one per place, all at once (AC-8, P-6) |
| `PinOutDescriptor::after` | what follows an attach or detach on this pin-out while the machine is on (P-5) |
| `PlaceDescriptor::takesAtOnce` | how many the place holds (AC-3, M-8). The attach rule counts per place |
| `PlaceDescriptor::pinOut` | the one pin-out the place presents (AC-2) |
| `PeripheralDescriptor::fits` | a peripheral fits a place when the place's `pinOut` is in this list (AC-2) |

```cpp
namespace calcu::machine {
struct Refusal { std::string reason;                    // an English sentence (spec OQ-1; D-7)
                 std::vector<std::string> missingRoms; }; // EC-4: the files named
struct ChangeResult { std::optional<Refusal> refused;    // set: nothing changed (AC-6)
                      AfterChange follows = AfterChange::Nothing; };  // AC-4, AC-5, AC-35

struct SwitchPosition { ControlId id; bool on; };        // added to PeripheralRequest (D-3)
// struct PeripheralRequest { ...LIB-1's fields...; std::vector<SwitchPosition> switches; };

class Machine {                                          // additions only
public:
  std::optional<Refusal> canAttach(const PeripheralRequest&, const Catalogue&) const;  // AC-6
  ChangeResult attach(PeripheralRequest, const Catalogue&);                             // AC-4, AC-7, EC-1, EC-4
  ChangeResult detach(const PlacePath&, const PeripheralId&);                           // AC-5, EC-2, EC-3
  ChangeResult replace(const PeripheralId& old, PeripheralRequest, const Catalogue&);   // AC-35
protected:
  virtual std::optional<std::string> productRefusal(const PeripheralRequest&,
                                                    const std::optional<PeripheralId>& replacing) const; // AC-29; default: none
  virtual void plug(PeripheralRequest, const PeripheralDescriptor&, TickRate hostRate);  // never fails; default: nothing
  virtual void unplug(const PlacePath&, const PeripheralId&);    // takes the peripherals plugged into it too (EC-3)
  virtual AfterChange followsNow(const PinOutDescriptor&) const; // default: pinOut.after; a switched-off Sharp machine: Nothing
};
}
```

`canAttach` checks, in this order, and stops at the first that fails:
1. the place exists: on the machine's descriptor for a one-step path, or
   on the peripheral plugged in at the path's prefix (AC-9);
2. the peripheral is in the catalogue, also when it was added after the
   catalogue was built (AC-2, `Catalogue::addPeripheral`);
3. it fits the place's pin-out (AC-2, EC-1);
4. the variant belongs to it;
5. every `RomEntry` of the variant has a `RomImage` (EC-4, the same check
   as `calcu::machine::build()`, LIB-1 §3.8);
6. the place holds fewer than `takesAtOnce`, and on a `Takes::One`
   connector no other place on it holds one (AC-3, AC-7);
7. `productRefusal()` (AC-29).

`attach` calls `canAttach`. When `canAttach` says no, `attach` returns
that refusal and changes nothing (AC-6). Otherwise it calls `plug()`
with the machine's tick rate, which is how a peripheral learns the host's
rate (P-2, AC-12), and returns `followsNow()` of the place's pin-out. For
a nested place, that is the pin-out on the peripheral that offers it
(AC-9). `plug()` cannot fail, because everything that can refuse is
checked before it. A core that checks content (a ROM that is not a
CE-158 image, `core/Connector/Ce158Port.hpp:20-28`) does so in
`productRefusal()`.

`detach` with nothing at that place, or another ID there, returns
`{nullopt, Nothing}` and changes nothing (EC-2). Otherwise it calls
`unplug()` once and returns `followsNow()` of that place's pin-out. That
is one answer, also when the peripherals plugged into it go with it
(EC-3, LIB-18 EC-5).

`replace` refuses when `old` is not in the place. It checks the new
request as `canAttach` does, except that `old` does not count in step 6
and is passed to `productRefusal()`. Then it unplugs and plugs, and asks
`followsNow()` once (AC-35).

The library's sentences (spec OQ-1: the library writes the general
cases; D-7):

| Case | Sentence |
|---|---|
| no such place | "The <model> has no place <place>." |
| unknown peripheral | "<id> is not a known peripheral." |
| EC-1 | "The <name> does not fit <place>." |
| variant | "The <name> has no variant <variant>." |
| EC-4 | "The <name> could not be attached: <file> is missing." (wording as today, `app/AppSession.cpp:19-22`) |
| AC-7 | "<place> already holds the <name>." |

AC-10 is the core's job: `reset`/`powerCycle`/`coldBoot` leave `plugged()`
as it was and tell each peripheral of the operation in the core's own way
(P-4). The library states the rule and checks it with the test machines.
Threading (ADR 0009): all four members run on the session's thread,
between two advances. Capability pointers from before an attach, detach
or replace are invalid afterwards (LIB-1 §3.5).

### 3.2 Controls (`peripherals/Controls.hpp`): AC-14–16, EC-6

```cpp
namespace calcu::peripherals {
struct ControlInfo { machine::ControlId id; machine::ControlForm form; bool on; };  // key: on = down; switch: on
enum class ControlCall { Done, UnknownControl, WrongForm };    // refused: nothing changes (EC-6)
class Controls : public machine::Capability {
public:
  static constexpr machine::KindId kKind{"controls"};
  virtual std::vector<ControlInfo> list() const = 0;           // in the descriptor's order (AC-14)
  virtual ControlCall press(const machine::ControlId&) = 0;    // keys, at the current tick count (AC-15)
  virtual ControlCall release(const machine::ControlId&) = 0;
  virtual ControlCall set(const machine::ControlId&, bool on) = 0;  // switches (AC-16)
};
class ControlTable final : public Controls {                   // for cores
public:
  using OnChange = std::function<void(const machine::ControlId&, bool on)>;   // the owner's reaction
  ControlTable(std::vector<machine::ControlDescriptor>, OnChange);
  void restore(const std::vector<machine::SwitchPosition>&);    // at plug: positions from the request (D-3)
};
std::vector<ControlInfo> controlsOf(machine::Machine&, const machine::Owner&);  // "named by owner and ID"
}
```

`WrongForm` (a switch pressed, a key set) is refused like EC-6. The spec
does not name this case. A product kind or a component with controls of
its own is not needed (spec OQ-6: none).

### 3.3 Serial port (`peripherals/SerialPort.hpp`): AC-17, EC-7

```cpp
struct LineSettings { std::optional<std::uint32_t> baud; std::optional<int> bitsPerChar;
                      std::optional<bool> dtr, rts; };          // what today's SerialLink reports (onBaud, setControl)
struct PeerLines { bool cts = true, dsr = true, dcd = true, ri = false; };   // today's SerialLink::Lines
class SerialPort : public machine::Capability {
public:
  static constexpr machine::KindId kKind{"serial-port"};
  virtual std::size_t takeSent(std::span<std::uint8_t> out) = 0;          // oldest first (AC-17)
  virtual std::size_t putReceived(std::span<const std::uint8_t> in) = 0;  // how many it took; the rest stays with the caller
  virtual LineSettings lineSettings() const = 0;
  virtual bool used() const = 0;                 // the machine has used the port since it exists (AC-33; D-5)
  virtual void setPeerLines(const PeerLines&) = 0;
};
class SerialPortQueues final : public SerialPort {   // for cores; no thread, no lock (I-3)
public:
  static constexpr std::size_t kReceiveLimit = 8192;   // today's kRxHighWater (core/Serial/PtySerialLink.hpp:94)
  // core side, called by the UART at its character time:
  void machineSends(std::uint8_t); bool machineReceives(std::uint8_t&);
  void machineSetsLine(const LineSettings&); PeerLines peerLines() const; void markUsed();
};
```

The sent queue has no limit. That matches today's TX ring
(`PtySerialLink.hpp:96-98`), and a machine that sends is never held up
(EC-7). It is drained at every pump (§3.5), or by whoever owns the
machine when there is no host link (D-8).

### 3.4 Real-time clock (`peripherals/RealTimeClock.hpp`): AC-21–23

```cpp
struct ClockFields { std::optional<int> year, month, day, weekday, hour, minute, second; };  // what the chip keeps
struct HostDateTime { int year, month, day, weekday, hour, minute, second, millisecond; };
class RealTimeClock : public machine::Capability {
public:
  static constexpr machine::KindId kKind{"real-time-clock"};
  virtual ClockFields read() const = 0;                          // only the kept fields are set (AC-21)
  virtual void set(const ClockFields&, int millisecond = 0) = 0; // a field the chip lacks is ignored
};
HostDateTime hostDateTime(std::chrono::system_clock::time_point);    // local time; the logic of core/HostClock.hpp
void setClocksFromHost(machine::Machine&, const HostDateTime&);     // every RTC capability (AC-23)
```

The clock advances in the core by ticks only (AC-22), as it does today
(the 1 Hz accumulators in `Upd1990ac` and `PC1600SubCpu`). The
uPD1990AC keeps month, day, weekday, hour, minute and second, and no
year (`core/PC1500/Upd1990ac.hpp:21-24`). The LU-57813P also keeps the
year (`core/PC1600/PC1600Machine.hpp:70-80`). `millisecond` preloads
the accumulator, as today. AC-23 asks whoever builds a machine or runs
an operation to call `setClocksFromHost`, so the session does it (§9,
A-4; LIB-3 A-7).

### 3.5 Host link (`peripherals/HostLink.hpp`, `HostLinks.hpp`): AC-18–20, EC-5, EC-7

```cpp
class HostLink {                       // the pseudo-terminal behind one serial port file
public:
  virtual ~HostLink() = default;
  virtual std::size_t read(std::span<std::uint8_t>) = 0;        // from the program on the user's computer
  virtual void write(std::span<const std::uint8_t>) = 0;        // never blocks; dropped while no program reads (EC-7)
  virtual bool peerOpen() const = 0;                            // → PeerLines::dcd, as today
  virtual std::string path() const = 0;                         // the serial port file, or the raw tty if none
  virtual bool moveTo(const std::string& folder) = 0;           // today's relink()
};
std::unique_ptr<HostLink> openHostLink(const std::string& folder, const std::string& fileName);  // nullptr without CALCU_HAS_PTY

struct HostLinkRule { std::string fileName; bool openAtOnce; };   // product: which port gets which file
using HostLinkRules = std::function<std::optional<HostLinkRule>(const machine::CapabilityEntry&)>;
struct HostLinkStatus { std::string fileName; std::string path; bool open; bool connected; };
class HostLinks {                       // held by the app session (LIB-3), used on its thread only
public:
  HostLinks(HostLinkRules, std::optional<std::string> folder);  // LIB-9 Paths::serialLinkFolder()
  void machineChanged(machine::Machine&);   // after a build, attach, detach, replace: rebind by rule (AC-20)
  void pump(machine::Machine&);             // between two advances: sent → link, link → machine (AC-18)
  void moveTo(std::optional<std::string> folder);
  std::vector<HostLinkStatus> status() const;
};
```

- `PtyHostLink` is today's `PtySerialLink` moved into the library: the
  same reader and writer threads, the same RX ring with
  `kRxHighWater = 8192`, the stable symlink in the folder and the
  fallback when the folder is empty. ADR 0009 allows threads in a host
  adapter: its rings are the only place that synchronizes (LIB-1 §2a).
- `pump` takes every sent byte and writes it. It reads up to
  `kReceiveLimit` minus what still waits and hands it over with
  `putReceived`. It sets `PeerLines{cts, dsr = true, dcd = peerOpen()}`.
  Line settings are never read (AC-18, spec OQ-2: as today).
- A link is created by rule: `openAtOnce` when the port first exists
  (PC-1600 RS-232C), or else when `used()` first turns true (CE-158,
  AC-33). Once created, a link lives as long as the `HostLinks`. When its
  port goes away (a model switch, a detach), the link stays and is
  unbound. It is bound again to the next port the rule gives the same
  file name (AC-20).
- Without `CALCU_HAS_PTY`, `openHostLink` returns nullptr. `pump` then
  drains the sent bytes and drops them, and the machine runs (EC-5).

## 4. Product side

**Calc-U-59 (M-25, with C59-6).**

| Today | After M-25 |
|---|---|
| `TI59Machine::setPrinterConnected` (`core/TI59Machine.cpp:214-221`), re-applied after reset (`:39-45`) | `attach`/`detach` at `printer`. The PC-100C stays plugged in through reset, power cycle and cold boot (AC-10). `followsNow` gives `PowerCycle` (AC-24; ledger 59-11) |
| `TI59Machine::loadLibrary` (`:31-33`), called by `app/EmulatorSession.cpp:45` | `attach` of the module at `slot-1` with its ROM image. A second module is refused by rule 6 (AC-25). An empty slot is a machine without a peripheral there (FS 9) |
| `pressPrinterPrint`, `pressPrinterAdv`, `setPrinterTrace`, `printerTrace` (`:198-212`) | `Controls` of the PC-100C: `PRINT`, `ADV` keys, `TRACE` switch (AC-26). `TRACE` keeps its position through detach and attach, carried in `PeripheralRequest::switches` (C59-6 AC-6; D-3). Today's `setPrinterTrace(false)` on connect (`:220`) goes, as C59-6 AC-6 asks |
| no serial port, no clock | no capability of either kind (AC-27) |

`SessionController` (`app/SessionController.cpp:135, 239-241, 362-381,
657, 674`) keeps its methods as adapters (§5). The frontends' calls are
unchanged in M-25: Qt6 `frontends/qt6/app/PrinterWidget.cpp`,
`EmulatorController.hpp`; Swift `App/Views/PrinterView.swift`,
`App/EmulatorViewModel.swift`, `Bridge/TISessionController.h/.mm`; Web
`frontends/web/bindings.cpp`, `src/Printer.jsx`.

**Calc-U-1600 (M-26, with C1600-1).**

| Today | After M-26 |
|---|---|
| `attachCE150`, `attachCE1600P`, `attachCE158`, `attachHostDrive`, `attachExpansionCard`/`attachSlot1Card`/`attachSlot2Card` and their detaches on the machine classes | `plug`/`unplug` behind `attach`/`detach`/`replace`. `slot-1`, `slot-2`: `Takes::One`, 1 each. 60-pin: `Takes::Chain`, places `printer`, `interface`, `host-drive`, 1 each. All `ContactLevel`, `PowerCycle` (AC-28) |
| one plotter at a time by a silent detach (`PC1600Machine.cpp:264-265`) | `printer` holds one: refused by rule 6 (AC-29). The printer picker becomes a `replace` (LIB-18 AC-10) |
| CE-1600P with CE-158: silent detach (`PC1600Machine.cpp:89-91, 289-290`) | `productRefusal`: "The CE-1600P and the CE-158 cannot be used together." (D-7) (AC-29; D-6) |
| `MachineSession::powerCycleAround` (`app/MachineSession.cpp:261-274`): OFF, change, ON | `attach` gives `PowerCycle`, or `Nothing` while switched off (`followsNow`; LIB-18 AC-33). The caller then runs `powerCycle()`, whose sequence C1600-1 moves into the core (`core/Machine/PowerCycleSequence`). The bus changes at the off point of that power cycle, as today (D-4) |
| `Ce158Port::build(rom, size, kTStateHz)` (`PC1600Machine.cpp:287`) | unchanged: the host's rate reaches the card at plug (AC-31, P-2) |
| `setSerialLink`, `setCE158SerialLink` (`PC1600Machine.hpp:289, 308-317`, `PC1500Machine.hpp:253`) | serial port capabilities. Owner: the PC-1600's UART component (TC8576F; component ID `rs232c`, D-11), and the CE-158 at `interface`. Both are `SerialPortCapability` (§2) |
| `seedClock(...)` on both classes | `RealTimeClock` capability, owner the clock component (ID `rtc`, D-11) (AC-34). `seedClock` stays private to the core |
| `CardBase::setWriteProtected` reached by `MachineSession::setSlotWriteProtected` (`app/MachineSession.cpp:571-576`) | `Controls` of the CE-161 with the switch `write-protect` (AC-32) |

`core/Serial/SerialLink.hpp` stays. It is the UART's peer inside the core,
a pin-out signal in the sense of P-4. `SerialPortCapability` is the only
implementation left.

## 5. Adapters

| Adapter | File | Wraps | Removed in |
|---|---|---|---|
| Printer connect | `products/calc-u-59/app/SessionController.cpp` `setPrinterConnected`, `togglePrinterConnection` | `attach`/`detach` at `printer` + `Machine::powerCycle()` as the power cycle (ledger row 59-11; reconciled with C59-6 design, its D-8) | M-54 (LIB-18 59: the attach command). In M-33/M-34 it moves with `SessionController`'s duties onto the library session |
| Module change | same file, `changeModule` (`:239-241`) | `replace`/`detach` at `slot-1` + `Machine::powerCycle()` | M-54 |
| PC-100C buttons | same file, `pressPrinterPrint`, `pressPrinterAdv`, `togglePrinterTrace` (`:371-381`) | `Controls::press/release/set` | M-57 (Qt6, Web: LIB-4 59 and LIB-14 59) and M-58 (Swift: C59-8, LIB-4 59) (reconciled with C59-6 design) |
| Sharp peripheral calls | `products/calc-u-1600/app/MachineSession.cpp` `attachCE150` … `detachHostDrive`, `attachSlotCard`, `detachSlots`, `reattach` (`:653-740`) | `Machine::attach/detach/replace`, then `Machine::powerCycle()` (C1600-1 §3.3: the sequence moves into the core) and `LegacyCalls::runToPromptJob` (reconciled with C1600-1 design) | M-46 (LIB-18 1600). In M-31 they move to product commands on the library session |
| Write-protect | `MachineSession::setSlotWriteProtected` (`:571-576`), used by `app/ModuleBay.cpp` | `Controls::set(write-protect)` | M-47 (LIB-7 1600, LIB-7 AC-19) |
| Product-held host links | `MachineSession` members `m_serialLink`, `m_ce158SerialLink`, `wireNewMachine`, `refreshSerialLinkDirectory`, `serialLinkStatus`, `ce158SerialLinkStatus` (`app/MachineSession.hpp:268-275, 372-380`) | one `calcu::peripherals::HostLinks`, pumped after each `runActive` slice | M-31 (the library app session holds it, LIB-3 1600) |
| Clock seeding | `MachineSession::seedClockFromHost` (`:223-225`), called by `app/AppSession.cpp:71, 88` and `frontends/qt6/app/EmulationPacer.cpp:29` | `setClocksFromHost(machine, hostDateTime(source()))` | M-31 (the library session does it at build and at the end of every operation) |
| `syncclock` step | `core/Preset/PresetRunner.hpp:177` (`syncClock()`), and `PC1500PresetLoader`/`PC1600PresetLoader` | `setClocksFromHost` | M-46 (LIB-6 1600) |

No adapter is added in M-23.

## 6. Tasks

Library, M-23. Tests are written before the code. `calcu_library_tests`
holds T3–T7. `calcu_library_pty_tests` holds T8 (macOS, Linux). All are
Frequent.

| Task | What | ACs/ECs | Test (file, case) |
|---|---|---|---|
| T1 | CMake: the `library/peripherals/` sources in `calcu_library`, `CALCU_HAS_PTY`, the entry `calcu_library_pty_tests`; entries in `docs/test-groups.md` | (support) | the library builds in every workflow, Emscripten included |
| T2 | Test machines (`library/machine/tests/TestMachines.*`): `test-seg` and `test-dot` with different tick rates; on `test-dot` a 60-pin-like chain connector (`Takes::Chain`, places `a`, `b`, pin-out `test.chain`, `PowerCycle`) and a one-module slot (`Takes::One`); `test-dot` switched off by key `off` answers `Nothing`; a component `rtc` with a `RealTimeClock` keeping all fields but year; `test-hub` (fits `test.chain`, offers place `jack` with pin-out `test.jack`, `Nothing`); `test-uart` (fits both models, a `SerialPortQueues`, sends one byte every 10 ms of emulated time, counts operations); `test-printer` gets keys `feed`, switch `trace`; a product rule refusing `test-printer` with `test-uart` | (support) | exercised below |
| T3 | `Attach.hpp/.cpp`, the `Machine` members | AC-1–11, AC-35, EC-1–4 | `library/machine/tests/attach_tests.cpp`: `ac1_pinout_states_level_takes_and_after`, `ac2_fit_follows_pinout_also_for_added_peripheral`, `ac3_place_takes_its_count_chain_holds_several`, `ac4_attach_lists_peripheral_and_capabilities_and_says_what_follows`, `ac4_switched_off_machine_says_nothing_follows`, `ac5_detach_removes_peripheral_and_capabilities`, `ac6_can_attach_agrees_with_attach_refusal_changes_nothing` (every refusal case of §3.1), `ac7_full_place_refuses`, `ac35_replace_detaches_attaches_asks_once`, `ac8_chain_holds_peripherals_in_their_places`, `ac9_peripheral_place_makes_a_tree_and_its_pinout_decides`, `ac10_operations_keep_peripherals_and_tell_each`, `ac11_attach_reads_no_file` (a request with every byte in memory; review: no file call in `Attach.cpp`), `ec1_unfitting_peripheral_refused_with_reason`, `ec2_detach_of_empty_place_changes_nothing`, `ec3_detach_takes_children_asks_once`, `ec4_missing_rom_named`, `ac29_product_refusal_reaches_caller` (library form of the hook) |
| T4 | Emulated time through a peripheral | AC-12, AC-13 | `peripherals/tests/time_tests.cpp`: `ac12_same_emulated_seconds_on_two_tick_rates` (`test-uart` on both models, one emulated second), `ac13_no_advance_no_output` (50 ms wall time) |
| T5 | `Controls`, `ControlTable`, `controlsOf` | AC-14, AC-15, AC-16, EC-6 | `controls_tests.cpp`: `ac14_controls_listed_with_id_form_state`, `ac15_key_down_up_at_tick_counts`, `ac16_switch_position_seen_and_reported`, `ac16_restore_from_request`, `ec6_unknown_control_refused`, `ec6_wrong_form_refused` |
| T6 | `SerialPort`, `SerialPortQueues` | AC-17, EC-7 | `serial_port_tests.cpp`: `ac17_bytes_out_and_in_in_order`, `ac17_line_settings_as_set`, `ac17_receive_limit_leaves_rest_with_caller`, `ec7_sending_never_waits` (one million bytes, nobody takes them) |
| T7 | `RealTimeClock`, `hostDateTime`, `setClocksFromHost`; `HostLinks` with an injected link factory | AC-21, AC-22, AC-23, EC-5 | `real_time_clock_tests.cpp`: `ac21_read_and_set_kept_fields`, `ac22_clock_moves_by_emulated_time_only`, `ac23_set_from_host_at_build_and_after_operation` (with LIB-3's session and a fixed host-time source, §9 A-4); `host_links_tests.cpp`: `ec5_without_pty_machine_runs_and_bytes_go_nowhere` |
| T8 | `PtyHostLink`, `HostLinks` on a real pseudo-terminal | AC-18, AC-19, AC-20, EC-7 | `peripherals/tests/pty_tests.cpp`: `ac18_bytes_both_ways_moved_between_advances` (the test opens the serial port file at another baud rate than the machine set), `ac19_fixed_name_in_the_folder`, `ac20_file_outlives_port_and_reconnects`, `ec7_no_reader_machine_not_held_up` |

Calc-U-59, M-25. Group Frequent.

| Task | What | ACs | Test (`products/calc-u-59/tests/peripherals_tests.cpp`) |
|---|---|---|---|
| P1 | `printer` and `slot-1` pin-outs and their answer | AC-24 | `lib2_ac24_places_functional_power_cycle_follows` |
| P2 | Module rules | AC-25 | `lib2_ac25_second_module_refused_until_detached`, `lib2_ac25_empty_slot_machine_runs` |
| P3 | PC-100C controls | AC-26 | `lib2_ac26_pc100c_print_adv_trace`, `lib2_ac26_no_pc100c_no_controls` |
| P4 | Kinds absent | AC-27 | `lib2_ac27_no_serial_port_no_clock` (all three models) |

Calc-U-1600, M-26 (and M-31, M-47). Group Frequent, except where marked.

| Task | Step | What | ACs | Test (`products/calc-u-1600/tests/peripherals_tests.cpp` unless named) |
|---|---|---|---|---|
| P5 | M-26 | Places, pin-outs, the answer | AC-28 | `lib2_ac28_slots_one_chain_contact_level_power_cycle` (three models) |
| P6 | M-26 | Refusals | AC-29 | `lib2_ac29_second_printer_refused`, `lib2_ac29_ce1600p_ce158_refused_both_ways` |
| P7 | M-26 | Chain | AC-30 | `lib2_ac30_ce150_and_ce158_together` |
| P8 | M-26 | CE-158 timing across rates | AC-31 | `lib2_ac31_ce158_same_emulated_time_pc1500_pc1600` |
| P9 | M-26, M-47 | CE-161 switch | AC-32 | M-26: `lib2_ac32_ce161_switch_blocks_writes`. M-47: `lib2_ac32_switch_set_by_save_then_power_cycle`, `lib2_ac32_position_at_attach_from_saved_file`, `lib2_ac32_media_place_reports_same_position` |
| P10 | M-26, M-31 | Serial port files | AC-33 | `products/calc-u-1600/app/tests/serial_links_tests.cpp` (product `CALCU_HAS_PTY` entry): M-26 `lib2_ac33_pc1600_rs232c_file`, `lib2_ac33_ce158_file_on_first_use_all_models`, through the product's machine session. M-31: the same cases through the library's app session |
| P11 | M-26 | Clock | AC-34 | `lib2_ac34_rtc_on_three_models`, `lib2_ac34_clock_runs_on_emulated_time`, `lib2_ac34_set_at_build_and_after_operation` (with the fixed host-time source) |

Coverage. Library: 24 ACs (AC-1–23, AC-35) and 7 ECs (EC-1–7) in T3–T8.
Calc-U-59: AC-24–27 in P1–P4. Calc-U-1600: AC-28–34 in P5–P11. That is
35 of 35 ACs and 7 of 7 ECs. AC-29 also has a library test (T3) for the
hook.

## 7. Tools and agent knowledge

- **M-23**: moves no tool, and renames nothing.
- **M-25**: `products/calc-u-59/tools/headless/capi.cpp` and `ti59.py`
  move their printer connect, PRINT, ADV and TRACE onto `attach`/`detach`
  and `Controls` in the same step (plan §1, "Today's tools"). Their
  command names stay. Knowledge: the `calc-u-59-core` skill and
  `reference/CoreArchitecture.md` (printer and modules as peripherals,
  controls; plan M-25); the memory `calc-u-59/real_hardware_timing.md`
  where it names `setPrinterConnected`.
- **M-26**: `tools/Ce158CliPeer.hpp`, `tools/pc1600_uart_probe.cpp` move
  onto `PtyHostLink` and the serial port capability. `pc1500_cli` and
  `pc1600_cli` use `setClocksFromHost` (C1600-1 moves them onto the
  interface). Renames (`docs/glossary-mapping.md`, "Code names", new rows
  in this step): host link and serial port file = `PtySerialLink`,
  `LazySerialLink` → `PtyHostLink`, `HostLinks`; real-time clock =
  `seedClock`, `seedClockFromHostTime` → `RealTimeClock`,
  `setClocksFromHost`. The same step changes
  `products/calc-u-1600/docs/developer/PC1600-Serial-Port.md`,
  `PC1600-Core-Limitations.md`, `Shared-Architecture.md`,
  `products/calc-u-1600/CLAUDE.md` and the memory
  `calc-u-1600/project_pc1600_serial_port.md`.
  `docs/background/plans/RAM-Write-Protect-Plan.md` and
  `Shared-Layer-Plan.md` stay as history.
- `docs/adr/0009` names `PtySerialLink` as history. It is not edited.

## 8. Golden runs and risks

- **Golden runs.** M-23 touches no core, and both products' replays run
  unchanged. M-25: `ti59-printer`, `ti59-print-with-printer`,
  `ti59-print-without-printer`, `ti59-module-ml`, `ti59-module-le`,
  `ti59-no-module`, `ti58-no-module` replay with the peripherals plugged
  in through `attach`. M-26: `pc1500a-ce150`, `pc1500a-ce151`,
  `pc1500a-ce158`, `pc1600-ce150`, `pc1600-ce158`, `pc1600-ce1600m`,
  `pc1600-ce1600p`. A difference is a finding.
- **No reference for the power cycle after an attach.** It was not
  recorded (FS 2 result; LIB-1 §8). On the Sharp machines, today's
  change is made while the machine is off (`powerCycleAround`).
  The core keeps today's order (D-4): the new peripheral reaches the bus
  at the off point of the power cycle. C1600-1 records the Sharp power
  cycle as a golden run from the imported code before the core is
  touched (C1600-1 D-8).
- **Serial byte timing.** Today the UART polls the PTY ring at character
  time. After M-26 the bytes reach the machine once per slice (about
  1/60 s). The order is the same. A burst waits up to one slice, and up
  to 8192 bytes wait in the machine's queue. SharpDataExchange has not
  been tried with this. A manual check with it is part of the M-31 exit.
- **Interface churn.** M-23 adds virtual members to `Machine` with
  defaults, so the TI machine of M-19 builds unchanged. Both products
  rebuild.
- **PTY tests in CI.** They use real pseudo-terminals on ubuntu-24.04
  and macOS. A flaky open or close would hit every merge. They have
  their own CTest entry, so they can move to Longer (ADR 0031) without a
  design change.
- **Emscripten.** `library/peripherals/` builds for the Web without
  `PtyHostLink` (`NoHostLink.cpp`).

## 9. Decided (best guess)

- **D-1 (was OQ-1).** The LIB-1 design reserves the forms of §3.1;
  closed before.
- **D-2 (was OQ-2 = LIB-1 OQ-7).** The attach code lives in
  `library/machine/Attach.*`; with one library target nothing else is
  left to choose (LIB-1 D-7).
- **D-3 (was OQ-3; AC-11, AC-32).** Switch positions travel in a generic
  field, `PeripheralRequest::switches`, as LIB-1 §3.5 reserves: simpler
  than a product payload per peripheral.
- **D-4 (was OQ-4 = C1600-1 OQ-2).** An attach on a running Sharp
  machine shows in `plugged()` and the capabilities at once; the core
  makes the electrical change at the off point of the power cycle that
  follows, as today's `powerCycleAround`.
- **D-5 (was OQ-5; AC-33).** `used()` turns true on the UART's first
  poll, send or status call, as today's `LazySerialLink`.
- **D-6 (was OQ-6; AC-29).** The CE-1600P with the CE-158 is refused, as
  the spec asks; the silent detach goes. A ledger row is added (Calc-U-1600).
- **D-7 (was OQ-7).** The refusal sentences of §3.1 and §4 are taken as
  drafted.
- **D-8 (was OQ-8; EC-7).** The sent queue keeps no limit, as today's TX
  ring; the spec states none, and every session pumps it.
- **D-9 (was OQ-9).** The CE-161 switch's control ID is `write-protect`.
- **D-10 (was OQ-10).** Calc-U-1600 publishes `HostLinks::status()` as a
  product topic from M-31; Settings shows the paths.
- **D-11 (was OQ-11 = C1600-1 OQ-10).** The component IDs are `rs232c`
  (PC-1600 UART, TC8576F) and `rtc` (the clock on all three Sharp
  models); places as C1600-1 §3.4.

**Assumed from LIB-3 (drafted at the same time).**
(Reconciled with the LIB-3 design: A-1, A-3, A-6 hold as drafted; A-2's
`HostLinks` is held by a `SessionPart` and pumped in `beforeCall`
(LIB-3 A-1); A-5's requests are LIB-3's `capabilities` request.)
- A-1: the machine session owns the `Machine` and runs commands only
  between two advances, on one thread. `attach`/`detach`/`replace` are
  called there, inside an operation (LIB-18).
- A-2: the app session holds one `calcu::peripherals::HostLinks` for its
  life, built with the product's `HostLinkRules` and LIB-9's
  `serialLinkFolder()`. It calls `pump(machine)` after every advance
  slice, also inside operations, and `machineChanged(machine)` after a
  build and after every attach, detach and replace.
- A-3: an operation can run "change, then what follows" for LIB-18: it
  reads `ChangeResult::follows`, runs `powerCycle()` and waits for key
  readiness. A refusal ends the operation with `Refusal::reason` as the
  reply and as a notice (LIB-3 AC-13, AC-18).
- A-4 (LIB-3 A-7: `AppSessionSetup::hostTime`; LIB-11 S-7 uses the
  same): the app session has a host-time source: wall-clock local time in
  the products, LIB-11 AC-21's fixed instant in the harness. It calls
  `setClocksFromHost` when a machine is built and at the end of every
  operation (AC-23; LIB-3 AC-18's end of operation).
- A-5: a product topic carries the host links' status (D-10). The
  requests of LIB-3 AC-39 (capabilities, places, plugged in) need
  nothing more from this feature.
- A-6: the session looks capabilities up again after a change, because
  pointers go stale (LIB-1 §3.5).

## Part E simplification E-3 (2026-11-17, branch `es-59-topics`)

`docs/reviews/e-simplification.md` §1 S-3, §2 item 2. `set-control` has one
owner, `PeripheralsPart`: Calc-U-59's second registration in `TIAppPart`
(which the session had kept in place of this one) went, and so did
`TIAppPart::setPrinterControl` and `EmulatorSession::printerControl`.
TRACE's kept position across a detach (C59-6 AC-6) is not a rule of this
part for every switch: LIB-18's `ProductMachines::attaching` (a product hook
called on the request of an attach, as `carried` is at a rebuild) lets
Calc-U-59 put the position it keeps into the PC-100C attached; Calc-U-1600
does not override it, so its CE-161 write-protect switch behaves as before.
The replies of `set-control` in Calc-U-59 are this part's (EC-6): without a
machine "No machine is running.", without a PC-100C "No PC-100C is plugged
in." (Calc-U-59's handler had answered done and remembered TRACE), another
peripheral "No <id> is plugged in.". No frontend sends it then: the paper
panel's controls exist only while the PC-100C owns it.
