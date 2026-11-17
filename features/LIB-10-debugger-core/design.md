# LIB-10 Debugger core: design
Status: Library column approved at the Part C1/C2 boundary (2026-11-15); Calc-U-1600 column approved at the Part C2/D boundary (2026-11-16)

## 1. Scope

Part C2 works the Library column only. The product columns are worked
in Part D (Calc-U-1600) and Part E (Calc-U-59). §4 says what lands
where, so that the seams of §3 fit them. Their tasks are designed with
those parts.

| Column | Step | Tests from (plan §3) | Done in (plan §5.1) |
|---|---|---|---|
| Library | M-40 | AC-1–26, AC-45–51; EC-1–9 | M-40 |
| Calc-U-1600 | M-45 (with LIB-8 1600) | AC-36–44, AC-52 (AC-38 per FS 15) | M-46 |
| Calc-U-59 | M-53 (with LIB-8 59) | AC-28–35 | M-53, or M-58 if AC-34's Swift part needs C59-8's panels (D-12) |

M-40 waits for M-3 (C-1 of FS 5: the pause-on-start of
`test_cdebug.ti59` lands inside the key's hold on the CPU level, after
it on the calculator level; first-steps §5) and M-38 (LIB-6 L: the step
place). It also builds on:
- M-18 (LIB-1 L): `Machine::debugTargets()` and the descriptor's list.
  LIB-1 §3.5 reserves `debugTarget(DebugTargetId)` and leaves "how the
  stop is reported" to this feature;
- M-22 (LIB-3 L): `MachineSession::setGate(AdvanceGate*)` and
  `setPaused(bool)`, both reserved for this feature; `SessionPart`; the
  outbox; `HostService::KeepFile`;
- M-20 (LIB-9 L): `Paths::folder(Folder::Traces)`, `Paths::ensure`,
  `traceMaxFileSizeMb()`;
- M-23 (LIB-2 L): `peripherals::setClocksFromHost` (AC-16);
- M-28 (LIB-11 L): the command registry, `HarnessHost`, `wait`;
- M-29 (LIB-15 L): the Web shell, its test product, `KeepFile` as a
  download (AC-26);
- M-35 (LIB-18 L): the Reset and Cold boot commands (AC-15, AC-46);
- M-36 (LIB-5): the `Deferred` outcome, `SessionContext::currentCall()`
  and `reply()`, and the harness's advance until a deferred reply (§3.11);
- M-39 (LIB-8 L): the Qt6 component set `library/qt6/`, `calcu_qt6`,
  `calcu_qt6_tests`, and the React folder `library/web/components/` with
  `COMPONENT_SOURCES` (D-9). The plan's "Waits for" of M-40 should name
  M-39.

M-40 changes `library/` only. No product calls the new code before
M-45. Each product keeps its own gate (`SharpDebuggerGate`,
`TIFreezeGate`) until its column (§5).

**Clauses that name later features.** None of the Library criteria
passes later than M-40:
- AC-18 names "the component sets". LIB-14 builds their shells in M-43.
  The three buttons are components of their own and are tested without
  a shell in M-40 (§3.10, D-9).
- AC-22 names LIB-9's trace folder (M-20) and LIB-6's end of a load
  (M-38). Both exist before M-40.
- AC-26 names LIB-15 AC-9 (M-29). It is tested with LIB-15's test
  product.

## 2. Modules and files

### Library, M-40

The new area `library/debugger/`, namespace `calcu::debugger`, in
`calcu_library` (LIB-1 §2a). It includes `machine`, `pacing`,
`sessions`, `settings`, `formats`, `peripherals` (for AC-16's clocks
only) and `presets` (the step place of LIB-6). `harness` may include it
(§2a: any area but `web`). The interface a machine implements goes into
`machine`, since a debug target is part of what a machine exposes
(ADR 0006). So `machine` stays on the standard library only.

| File | Holds |
|---|---|
| `library/machine/DebugTarget.hpp` (new) | `RegisterInfo`, `SpaceInfo`, `BankWindow`, `BankSelect`, `QualifierField`, `TargetOffers`, `TargetInfo`, `CellRead`, `HistoryEntry`, `Decoded`, `Flow`, `DebugTarget`, `InstructionEvent`, `InstructionObserver`, `Checks`, `DebugStop`, `Debugging` (§3.1, §3.2) |
| `library/machine/Machine.hpp` (changed) | `virtual Debugging* debugging()` with default `nullptr`; `DebugTarget* debugTarget(const DebugTargetId&)`, non-virtual, through it (the name LIB-1 §3.5 reserved) |
| `library/machine/Conformance.hpp/.cpp` (changed) | `checkDebugging(Machine&)`: one target per listed ID, in the descriptor's order; each `info()` well-formed (§3.1); reading every register and the first and last cell of every space changes no tick count and no other cell |
| `library/machine/tests/TestCpu.hpp/.cpp` (new) | the test processor of §6 behind `test-seg`'s `test-cpu` and `test-dot`'s `main` and `inner`, switched on by the build option `TestProgram` (as LIB-17's `TestSegTiming`, LIB-1 D-8) |
| `library/machine/tests/TestMachines.hpp/.cpp` (changed) | `TestMachine::debugging()` returns the `TestCpu` set when the option is given, `nullptr` otherwise, so the other areas' tests stay as they are |
| `library/debugger/Levels.hpp/.cpp` | `LevelName`, `kCpu`, `kProgram`, `RestartKind`, `LevelRules`, `CpuOnlyRules` (§3.3) |
| `library/debugger/Breakpoints.hpp/.cpp` | `Setter`, `BreakpointId`, `Breakpoint`, `Watch`, `HitTest`, `BreakpointTable` (§3.5) |
| `library/debugger/RunControl.hpp/.cpp` | `RunControl`: the state machine of §3.4, the step over and out of Calc-U-1600's `core/Debug/RunControl.cpp:107-157` without source lines |
| `library/debugger/RunState.hpp/.cpp` | the payloads `RunState`, `TraceState`; the topic, command and request names (§3.6) |
| `library/debugger/TraceSession.hpp/.cpp` | `TraceRecorder`, `TraceFile`, `TraceSession` (§3.7) |
| `library/debugger/Profile.hpp/.cpp` | `Profile`: counts per target and address (AC-20 `profile`) |
| `library/debugger/Symbols.hpp/.cpp` | `SymbolReader`, `SymbolTable` (AC-51) |
| `library/debugger/DebuggerPart.hpp/.cpp` | `DebuggerSetup`, `DebuggerPart`: a `sessions::SessionPart` and the machine session's `sessions::AdvanceGate`; the session commands and requests of §3.6 |
| `library/debugger/DebuggerSteps.hpp/.cpp` | the preset steps of AC-19 on LIB-6's step place (§3.8) |
| `library/debugger/ButtonRule.hpp/.cpp` | `ButtonRule`: which of the three buttons can be used, from the run state; the hold repeat of 250 ms (AC-18, §3.10) |
| `library/debugger/tests/*.cpp`, `TestDebugParts.hpp` | §6; `TestDebugParts.hpp` holds the test product's `LevelRules` with a `program` level, its `TraceRecorder` and a `SymbolReader` for a table format made for the tests |
| `library/harness/DebugCommands.cpp` (new), `Commands.hpp` (changed) | `addDebugCommands(CommandRegistry&)`: the commands of AC-20, AC-50, AC-51 (§3.9) |
| `library/harness/FirstCommands.cpp` (changed) | `wait` ends at a pause and gives the run state (AC-21; LIB-11 AC-9) |
| `library/harness/tests/TestProduct.*` (changed) | the test product's parts include a `DebuggerPart` with the test rules of `TestDebugParts.hpp` |
| `library/sessions/SessionPart.hpp`, `AppSession.cpp` (changed) | the hook `SessionPart::machineRestarted` and `sessions::Restart` (§3.11; in the list of C2's LIB-3 additions, LIB-18 §8). The `Deferred` outcome, `currentCall()` and `reply(...)` are LIB-5's (M-36) |
| `library/startup/MachineControl.cpp` (changed; LIB-18, M-35) | after `Machine::reset()` and `Machine::coldBoot()`, and before its run to key readiness, calls `machineRestarted` on every part (§3.11) |
| `library/qt6/RunControlButtons.hpp/.cpp` (new; in `calcu_qt6`, created in M-39, D-9) | `calcu::qt6::RunControlButtons`: a `QWidget` with three buttons for one level, fed from a `run-state` payload, sending `pause`, `step`, `resume` |
| `library/qt6/tests/run_control_buttons_tests.cpp` | AC-18 for Qt6, offscreen |
| `library/web/components/RunControlButtons.jsx` (new; in `COMPONENT_SOURCES`) | the React component `CalcuRunControlButtons`, the same rule (§3.10) |
| `library/web/tests/run-control.spec.js` (new), `tests/product/src/CalcUTest.jsx` (changed) | AC-18 and AC-26 for React, on LIB-15's test product |
| `library/CMakeLists.txt`, `library/qt6/CMakeLists.txt` (changed) | the sources of `library/debugger/` and their tests; the buttons and their test in `calcu_qt6` and `calcu_qt6_tests` (D-9) |
| `docs/test-groups.md` (changed) | the new files and entries |

### Products

Nothing in M-40. M-45 and M-53 are outlined in §4.

## 3. Seams and interfaces

Signature sketches. Everything runs on the session's thread. It holds
no lock, starts no thread and throws nothing across its interface
(`debugger.md` I-4; ADR 0009; LIB-1 §2a). The library names no
processor and no model (TR). Register and cell values are
`std::uint64_t`: the widest values of either product today are the
TI's registers of 16 digits, 64 bits (D-1).

### 3.1 The debug target (AC-1 to AC-4, AC-48, AC-49)

```cpp
namespace calcu::machine {

struct RegisterInfo { std::string name; int bits; bool writable; };        // 1..64 bits
struct BankWindow   { std::uint64_t first, count; std::uint32_t banks, vbanks; };   // vbanks 0: none
struct SpaceInfo {
  std::string name;                  // "rom", "me0", "me1-written"
  int cellBits;                      // 1..64
  std::uint64_t size;                // cells
  bool writtenView = false;          // AC-48: "<space>-written", read-only, a cell may hold none
  std::vector<BankWindow> banked;    // AC-49: the ranges reached by bank (and vertical bank)
};
struct QualifierField { std::string name; std::vector<std::string> values; };   // "bank": "0".."7"; "pv": "0","1"
struct TargetOffers {
  bool watches = false, stepOverOut = false, decode = false;
  std::size_t history = 0;                         // entries kept; 0: no history (AC-4)
  std::vector<QualifierField> qualifier;           // empty: none
};
struct TargetInfo {
  DebugTargetId name;                              // as the descriptor names it (LIB-1 AC-36)
  std::vector<RegisterInfo> registers;
  std::vector<SpaceInfo> spaces;
  std::size_t codeSpace;                           // index into spaces: what a breakpoint's address means
  TargetOffers offers;
};

struct BankSelect { std::optional<std::uint32_t> bank, vbank; };   // none: as the processor sees it now
struct CellRead { enum Kind { Value, Unreadable, NoneWritten } kind; std::uint64_t value = 0; };
struct HistoryEntry { std::uint64_t address; std::vector<std::uint64_t> registers; std::optional<std::string> text; };
struct Decoded { std::string text; std::uint32_t cells; };
enum class Flow { Other, Call, Return };           // what step over and out need (AC-8)

class DebugTarget {
public:
  virtual const TargetInfo& info() const = 0;
  virtual std::uint64_t nextAddress() const = 0;                          // AC-3
  virtual std::uint64_t readRegister(std::size_t) const = 0;              // AC-2, pre-execution (I-1)
  virtual bool writeRegister(std::size_t, std::uint64_t) = 0;             // false: refused, nothing changed
  virtual CellRead peek(std::size_t space, std::uint64_t, BankSelect) const = 0;   // no effect on the machine (I-2)
  virtual bool poke(std::size_t space, std::uint64_t, std::uint64_t, BankSelect) = 0;
  virtual std::vector<HistoryEntry> history(std::size_t count) const { return {}; }  // newest first (AC-4)
  virtual std::optional<Decoded> decode(std::uint64_t address) const { return {}; }
  virtual std::vector<std::string> qualifierNow() const { return {}; }    // a value per field (AC-1)
  virtual bool qualifierMatches(const std::vector<std::string>& wanted, std::uint64_t address) const { return true; }
  virtual Flow flowAt(std::uint64_t) const { return Flow::Other; }        // stepOverOut only
  virtual std::uint64_t callDepth() const { return 0; }                   // grows with a call; stepOverOut only
  virtual std::uint64_t retired() const = 0;                              // instructions carried out
  virtual void setBreakpoints(std::vector<std::uint64_t> addresses) = 0;  // checked only while Checks say so
  virtual bool setWatches(std::vector<WatchRange>) { return false; }      // false: no watches (AC-11)
protected:
  ~DebugTarget() = default;
};
struct WatchRange { std::size_t space; std::uint64_t first, count; bool read, write; };
}
```

- A wanted qualifier holds `""` for a field left free. The target
  checks it, so the library needs no knowledge of banks, PU, PV or ME
  (Calc-U-1600 `BankKey`, `DebugTarget::bankMatches`).
- `callDepth()` grows with a call and shrinks with a return. A target
  whose stack grows downwards gives it from its stack pointer. The
  library only compares two values (D-2).
- `peek` and `poke` with a `BankSelect` reach a bank whichever is
  switched in. A range that leaves its `BankWindow` is refused by the
  library before the target is asked (AC-49). Out of the space: EC-9.

### 3.2 The machine's side: checks, stops, the observer (AC-5, AC-10, AC-11, AC-14, AC-39)

```cpp
namespace calcu::machine {
struct InstructionEvent { std::size_t target; std::uint64_t address, nextAddress; std::uint32_t marks; };
class InstructionObserver {                       // the run control
public: virtual bool stopAfter(const InstructionEvent&) = 0;   // true: the advance ends after it
protected: ~InstructionObserver() = default;
};
struct Checks { bool breakpoints = false, watches = false; InstructionObserver* observer = nullptr; };
struct WatchHit { std::size_t space; std::uint64_t address; bool write; };
struct DebugStop {
  enum Kind { Breakpoint, Watch, Observer } kind;
  std::size_t target;
  std::uint64_t address;                          // Breakpoint: where the target stands
  std::vector<WatchHit> hits;                     // Watch: every hit of the instruction (EC-4)
};
class Debugging {
public:
  virtual std::size_t targetCount() const = 0;
  virtual DebugTarget& target(std::size_t) = 0;   // in the descriptor's order
  virtual std::size_t current() const = 0;        // the target that carries out the next instruction (AC-7, AC-14)
  virtual void setChecks(const Checks&) = 0;      // read by advance(); all off: the fast path
  virtual std::optional<DebugStop> takeStop() = 0;   // why the last advance ended early; clears it
  virtual void passOnce(std::size_t target) = 0;  // its breakpoint where it stands is passed once (AC-10, AC-39)
protected:
  ~Debugging() = default;
};
}
```

- `Machine::advance(atLeast)` keeps its signature (LIB-1 §3.5). While
  `Checks` are set, it stops early: before an instruction at a
  breakpoint of its target, after an instruction that hit a watch, or
  after an instruction for which the observer said stop. It returns the
  ticks run, fewer than asked for then (AC-5, LIB-1 AC-7). The stop is
  read with `takeStop()`.
- `marks` carries a product's facts about the instruction that the
  library passes on unread. The TI machine sets one for a program step
  boundary and one for the key-readiness scan loop (today's
  `TIStepObserver::stopAfter(pc, stepBoundary, inScanLoop)`). The
  product's `LevelRules` read them (§3.3).
- Fetching an instruction is no access for a watch (AC-11). A watch
  that is hit pauses after the instruction. A breakpoint pauses before
  it.
- A machine with several targets calls the observer for each
  instruction of each, with its index. `passOnce(t)` lets only target
  `t` pass the breakpoint it stands on, and drops a pass any other target
  had (AC-39; today's `DebugTarget::resumeFromStop`,
  `products/calc-u-1600/core/Debug/DebugTarget.cpp:156-169`). The run
  control calls it with `current()`.

### 3.3 Levels: the product's rules (AC-6, AC-7, AC-9, AC-29, AC-30, AC-38)

```cpp
namespace calcu::debugger {
using LevelName = machine::Id<struct LevelTag>;
inline const LevelName kCpu{"cpu"}, kProgram{"program"};
enum class RestartKind { Reset, ColdBoot, Build };

class LevelRules {
public:
  virtual bool hasProgramLevel() const { return false; }
  // A pause of the level lands at once, or after the instruction for which this says so.
  virtual bool pauseLandsAtOnce(LevelName) const { return true; }
  virtual bool pauseLandsAfter(LevelName, const machine::InstructionEvent&) { return true; }
  virtual void pauseAsked(LevelName, machine::Machine&) {}               // e.g. the TI's count of 50000 starts
  // One step of `program` has been carried out after this instruction.
  virtual bool programStepDoneAfter(const machine::InstructionEvent&) { return true; }
  // A `cpu` step carries out an instruction that stays at its address again, up to this many times.
  virtual int cpuStepRepeats() const { return 0; }                       // the TI: 32 (AC-29)
  // Pause-on-start.
  virtual void startArmed(LevelName, machine::Machine&);                 // default: remembers key readiness
  virtual bool startFiresAfter(LevelName, const machine::InstructionEvent&, const machine::Machine&);
  virtual bool startFiresAtRestart(LevelName, RestartKind) const { return false; }
protected:
  ~LevelRules() = default;
};
class CpuOnlyRules final : public LevelRules {};   // the Sharp machines' and the test machines' default
}
```

- The default `startFiresAfter` for `cpu` fires on the first
  instruction after which `Machine::isKeyReady()` is false, once it was
  seen true since arming (AC-38: the processor leaving key readiness).
  It asks `isKeyReady()` only while a start is armed. Without
  `hasProgramLevel()`, `program` is refused (AC-19's read check, AC-6).
- The TI rules (M-53) carry today's `FreezeController::checkStep`
  (`products/calc-u-59/app/FreezeController.hpp`), the 50000-instruction
  landing (spec AC-29: `Core/TI59Machine.hpp:124-129` at the pin), the 32 repeats and the restart
  rule of AC-30.

### 3.4 The run control (AC-5 to AC-16, AC-45 to AC-47; EC-1, EC-2, EC-4 to EC-7)

`RunControl` is owned by the `DebuggerPart`, one per app session. It
exists whether or not anything uses it (AC-5). It is the machine
session's `AdvanceGate`. Its state is one of `Running`,
`PausePending{level, calls}`, `Paused{level, reason, target, hits}`,
`Stepping{level, kind, target, left, call}`. Besides it keeps the armed
level of pause-on-start, the flag of pause-at-start, and the profile.
`MachineSession::paused()` is true exactly in `Paused`.

| # | Rule | Serves |
|---|---|---|
| G1 | **The gate.** `advance(m, atLeast, stoppedEarly)`: if `m.debugging()` is null, a plain advance. Else set `Checks`: breakpoints and watches on while some are set and no operation runs (`SessionContext::busy()`); the observer on in `PausePending` and `Stepping`, while a start is armed, and while the profile is on. Then `m.advance`, then `takeStop()` and the rules below. `stoppedEarly` is true when the machine is paused afterwards. While a trace runs the advance is cut into pieces of at most 1/60 s of emulated time, and the trace is pumped after each (§3.7) | AC-5, AC-10, AC-11, AC-13, I-3 |
| G2 | **Pause** `level`: in `Paused`, done at once, nothing changes. Else, if the rules land it at once, `Paused{level, pause}`. Otherwise `PausePending`: the observer asks `pauseLandsAfter`, and the reply comes when it lands. A pause disarms pause-on-start. A pause during `Stepping` ends the step there: the step's reply goes out with the run state, then the pause's | AC-6, AC-9, EC-1, EC-6 |
| G3 | **Resume**: in `Running`, done, nothing changes. Else `passOnce(current)`, `MachineSession::setPaused(false)` (pacing starts afresh, LIB-3 AC-23), `peripherals::setClocksFromHost` with the session's host time. Whichever level owned the pause | AC-6, AC-16, EC-2 |
| G4 | **Step** `level, count, target?`: refused unless `Paused`. Then `passOnce`, `Stepping`, the machine runs and the run state says running. `cpu`: one step is one instruction of the target (the named one, else `current()`), counted on the observer's events of that target, the other targets running meanwhile; an instruction whose `nextAddress` is its own address is repeated up to `cpuStepRepeats()` times. `program`: `programStepDoneAfter`. After `count` steps: `Paused{level, step}`, so the level of the step owns the pause, whichever owned it before | AC-7, AC-45, AC-39, EC-5 |
| G5 | **Step over / out** (target offers them): over a `Flow::Call` instruction, the step ends at the first instruction of that target after which `callDepth()` is back at or below its value before; out, after a `Flow::Return` instruction of that target that leaves `callDepth()` below its value at the start. Otherwise one instruction. A breakpoint or watch inside pauses | AC-8 |
| G6 | **Breakpoint stop**: the breakpoints of that target at that address whose qualifier `qualifierMatches` and whose test (AC-12) says stop. None: `passOnce(target)`, and the advance goes on within the same gate call. Some: `Paused{cpu, breakpoint, target, ids}` | AC-10, AC-12, AC-47 |
| G7 | **Watch stop**: as G6 for the watches whose range holds a hit and whose kind matches. The run state names the watch of the lowest address hit; the machine pauses once | AC-11, AC-12, EC-4 |
| G8 | **Pause-on-start** `level` or `off`: arming one level disarms the other; `off` disarms both; `startArmed` is told. Fires by `startFiresAfter` or, at a restart, by `startFiresAtRestart`: `Paused{level, pause-on-start}`, disarmed. Fires during an operation too | AC-9, AC-13 |
| G9 | **Pause-at-start**: after the next restart of any kind (`machineRestarted`, or a new machine), `Paused{cpu, entry}` before the first instruction; once. Separate from G8 | AC-46 |
| G10 | **Restart** (Reset, Cold boot) while paused or stepping: `Running` (a step's reply goes out done, with the run state); an armed start stays armed | AC-15, EC-6 |
| G11 | **New machine** (`machineChanged`): `Running`, nothing armed (G9 then applies), every pending reply goes out done; the table sets its breakpoints and watches again on the targets of the same name and drops the rest (§3.5); a trace ends (§3.7) | AC-15, EC-6 |
| G12 | **Operations**: while one runs, G1 leaves breakpoints and watches off; pause, step, resume and pause-on-start are carried out from a preset's own steps or from outside. A paused machine holds a running operation: the session does not run it (`MachineSession::runs()`). A preset that ends paused leaves it paused | AC-13, EC-7 |
| G13 | **Several targets**: a pause stops the whole machine; `current()` is named in the run state | AC-14 |
| G14 | **Suspend**: the session's suspend and the pause are two flags (LIB-3 §3.7). Continue while paused leaves it paused | AC-16 |

The pause, step and resume of G2 to G4 come from the session commands
of §3.6, from the preset steps (§3.8), from the harness (§3.9) and from
the buttons (§3.10). They are the same calls.

### 3.5 Breakpoints and watches (AC-10 to AC-12, AC-15, AC-47; EC-3)

```cpp
namespace calcu::debugger {
using Setter = std::string;                 // "harness", "dap"; the session command names it
using BreakpointId = std::uint32_t;         // given by the table, never reused in a session
using HitTest = std::function<bool(const machine::DebugTarget&)>;   // true: pause (AC-12)
struct Breakpoint { BreakpointId id; Setter setter; machine::DebugTargetId target;
                    std::uint64_t address; std::vector<std::string> qualifier; HitTest test; };
struct Watch { BreakpointId id; Setter setter; machine::DebugTargetId target; machine::WatchRange range; HitTest test; };
class BreakpointTable {
public:
  std::variant<BreakpointId, std::string> addBreakpoint(Setter, const machine::Debugging&, machine::DebugTargetId,
                                                        std::uint64_t, std::vector<std::string>, HitTest = {});
  std::variant<BreakpointId, std::string> addWatch(Setter, const machine::Debugging&, machine::DebugTargetId,
                                                   machine::WatchRange, HitTest = {});   // EC-3: past the end refused
  void remove(const Setter&, std::optional<BreakpointId> = {});   // only the setter's own (AC-47)
  std::vector<Breakpoint> breakpoints(const Setter* = nullptr) const;
  void apply(machine::Debugging&);          // per target: setBreakpoints, setWatches
  void rebind(machine::Debugging*);         // a new machine: same target names kept, the rest dropped (AC-15)
};
}
```

- A test is the setter's (AC-12). The library sets none. C1600-3 builds
  conditions, hit counts and log messages on it (M-50).
- `setBreakpoints` gets every address of the target, of every setter,
  without qualifiers. The run control sorts the hits out (G6).

### 3.6 The run state, the trace state, the session's names (AC-17, AC-25)

```cpp
struct RunState : sessions::Payload {        // kind "run-state"
  bool paused;
  std::optional<LevelName> owner;            // when paused
  enum class Reason { Pause, Step, Breakpoint, Watch, PauseOnStart, Entry };
  std::optional<Reason> reason;
  std::optional<machine::DebugTargetId> target;   // the current target (AC-14)
  struct Hit { BreakpointId id; Setter setter; };
  std::vector<Hit> hits;                     // a breakpoint or watch stop
  std::optional<LevelName> armed;            // pause-on-start
  bool pauseAtStart;
};
struct TraceState : sessions::Payload { bool running; std::string fileName; };   // kind "trace"
```

`Stepping` and `PausePending` are sent as `paused = false` (AC-7). The
session's names (`library/debugger/RunState.hpp`), a command unless
marked as a request:

| Name | Arguments | Reply |
|---|---|---|
| state topic `run-state` | — | `RunState` |
| state topic `trace` | — | `TraceState` |
| `pause` | `{level}` | `Deferred`, then done with `RunState` |
| `resume` | — | done with `RunState` |
| `step` | `{level, count, target?, kind: in\|over\|out}` | refused when not paused; else `Deferred`, then done with `RunState` |
| `pause-on-start` | `{level}` or `{off: true}` | done |
| `pause-at-start` | — | done |
| `break` | `{setter, target, address, qualifier?}` | done with the ID, or refused |
| `break-off` | `{setter, id?}` | done |
| `watch` | `{setter, target, space, address, count, access}` | done with the ID, or refused |
| `watch-off` | `{setter, id?}` | done |
| `set-register`, `poke` | as AC-20 | done, or refused (at the first cell) |
| `trace` | `{file}`, `{button: true}` or `{off: true}` | done, or refused with the reason |
| `profile` | `{on: bool}` | done |
| `symbols` | `{file, target?}` | done, or refused |
| requests `targets`, `registers`, `peek`, `history`, `disassemble`, `breakpoints`, `watches`, `profile-report` | as AC-20 | the values as a payload |

A level or a target the machine does not have is refused with the
reason (AC-6). Without a machine every name but the topics is refused
with LIB-3's "No machine is running.".

### 3.7 The trace session (AC-22 to AC-26; OQ-1, OQ-3)

```cpp
class TraceFile {                          // the library's: a file in the trace folder, or memory (Web)
public: void write(std::span<const std::byte>); std::uint64_t bytes() const;
};
class TraceRecorder {                      // the product's record source, one per machine (ADR 0037)
public:
  virtual void begin(std::vector<std::uint8_t>& out) = 0;   // turns on the core's recording; appends header, session start
  // Appends whole records of what ran since the last call, at most `budget`
  // bytes; false: more were waiting than the budget took.
  virtual bool take(std::vector<std::uint8_t>& out, std::uint64_t budget) = 0;
  virtual std::uint64_t endBytes() const = 0;   // the size of the session's end record
  // Appends the records still waiting, then the end record (the last
  // endBytes() bytes); turns the core's recording off.
  virtual void end(std::vector<std::uint8_t>& out) = 0;
  virtual std::string buttonFileName() const = 0;   // AC-23: CALCU59_TRACE.bin, TRACE.bin
  virtual void machineGone() {}            // the machine was replaced: the core is not touched again
};
```

The recorder only says what the bytes are; `TraceSession` writes them
and alone keeps T2's limit, so a product writes no budget rule (§11,
Part D simplification).

| # | Rule | Serves |
|---|---|---|
| T1 | A start, by `trace:`, the command or a button: a name with `/` or `\` is refused. One trace at a time: a running one ends first. The place is `Paths::ensure(Folder::Traces)`; an existing file of that name is replaced. Where `Paths` gives no trace folder and a listener offers `KeepFile`, the file is kept in memory. Otherwise, and when the folder cannot be written, the start fails with the reason | AC-22, AC-23, EC-8 |
| T2 | The gate pumps after each piece (G1): `take` with a budget of the limit minus `bytes()` minus `endBytes()`, its bytes into the file. The limit is `[trace] max-file-size-mb` (LIB-9 AC-11) MiB. When `take` says false, the trace ends: of what `end` gives, only the last `endBytes()` bytes (the end record) go into the file; and a notice says "The trace <name> reached the size limit of <n> MB and ended." | AC-24, AC-43 |
| T3 | Ends: `trace: off`; the end of the load whose `trace:` step started it, whatever its result (the step marks the trace as the running operation's; LIB-3's `SessionPart::operationEnded` ends it); a new machine, with a log line "The trace <name> ended: a new machine was built."; T2. Each writes `end()` and closes the file | AC-22, AC-24 |
| T4 | A trace that ended in memory is handed over with `askHost(KeepFile, {name, bytes, mediaType: "application/octet-stream"})` (LIB-15 N4) | AC-26 |
| T5 | `trace` topic: running, the file name, whoever started it | AC-25 |

The file's content is the product's (TR). Neither format changes
(AC-33, AC-42).

### 3.8 The preset steps (AC-19)

`DebuggerSteps` adds six `presets::StepDef`s to LIB-6's `Vocabulary`
(LIB-6 design §3.3; spec AC-29). Each is checked when the preset is
read (LIB-6 AC-6), and each `Item` has `needsRunningMachine() = false`
and `after() = TakenEffect`, except `pause:` and `step:`, which are
`Own`: their `carryOut` runs until the pause has landed. All six have
`command = false`: the harness has commands of its own with other
rules (§3.9: `step` refused when not paused). `debug-panel:` has
`frontend = true` (LIB-6 AC-63).

| Step | Read check | Carried out |
|---|---|---|
| `pause:` | a level the model has | G2. Finished when the pause has landed (LIB-6 AC-56) |
| `resume:` | no value | G3 |
| `step:` | `{level, count}`, the count 1 to 100000, 1 without | G4, finished when paused again. Not paused: a log line "step: ignored, the machine is not paused", and the next item (OQ-4) |
| `pause-on-start:` | a level, or `off` | G8 |
| `debug-panel:` | a view the product names | a frontend step (LIB-6 AC-63): appended to the product's stream that tells its frontend; the harness passes it over |
| `trace:` | a file name without `/` or `\`, or `off` | T1, T3; a start that fails fails the step (LIB-6 AC-17) |

- Which steps a model has is the product's (LIB-6 AC-62): `DebuggerSetup`
  names, per model, whether the four run-control steps and
  `debug-panel:` are defined, and the views. On the Sharp models they
  are [undefined] and refuse the preset; `trace:` is defined in both.
- None needs a running or idle machine before it is carried out
  (LIB-6 AC-19).

### 3.9 The harness commands (AC-20, AC-21, AC-50, AC-51; EC-9)

`addDebugCommands` adds AC-20's commands at the top level. Each line is
turned into the session command or request of §3.6 with the setter
`harness`, so `break off` and `watch off` take only the harness's own
(AC-47). Rules:
- Numbers are read as decimal, `0x…` or `$…`. Addresses and values are
  written as `0x` and as many hex digits as the cell or register width
  needs (D-4). A name loaded by `symbols` stands wherever an address of
  that target is taken (AC-51); `history` and `disassemble` show
  `name` or `name+offset` (D-5).
- `peek … bank=<b> vbank=<v> file=<file>`: the cells as bytes (one byte
  per started 8 bits of a cell, little-endian) into the file, relative
  to the run folder; the reply gives the count (AC-50).
- `expect-register` and `expect-memory` are refused with the value found
  (AC-50). LIB-11 then ends a one-shot run with the exit status of a
  refused command.
- `pause`, `step`, `step-over`, `step-out`: the harness has no wall
  clock (LIB-11 AC-6). For a `Deferred` reply it advances the session as
  LIB-5 §3.5 does for `key` (pieces of at most 1/60 s of emulated time
  until the reply comes, at most one hour), then refuses with "The
  machine did not pause." (D-6).
- `wait <duration>` (AC-21): `advanceUnpaced` ends early when the gate
  pauses the machine (G1). The reply gives the time that passed and the
  run state. Paused when given: at once, no time passed.
- `profile report [<count>]`: the `count` (default 20, D-7) most
  frequent addresses with target, address, symbol and count, then the
  number of different addresses.
- `targets` lists AC-1's statement of each target as one block per
  target.

### 3.10 The run-control buttons (AC-18; DB-6)

`ButtonRule` (C++) and `RunControlButtons.jsx` hold one rule:
- pause can be used while `run-state.paused` is false;
- step and resume while it is true and `owner` is the buttons' level;
- step fires on press and then every 250 ms while held, until release
  or until step cannot be used (Calc-U-59
  `App/Views/HoldRepeatButton.swift:11-12`).

The Qt6 widget and the React component take the level, their labels and
nothing else; the product places them (DB-6). The rule's cases are one
table of the tests, `library/debugger/tests/button_cases.json`. The C++
and the Playwright tests read it, so the two component sets cannot
drift apart. The Swift frontend uses the same table in C59-8 (M-58).

### 3.11 Changes to the sessions area (LIB-3 code)

- **Late replies.** LIB-5 adds the `CommandOutcome` alternative
  `Deferred{}` in M-36 (LIB-5 §3.5, D-4). The handler takes
  `ctx.currentCall()` and later calls `ctx.reply(call, ...)` with done,
  reason and payload, once. Until then no reply goes out (LIB-3 AC-8
  still holds: one reply per command). The attach end already waits for
  a late reply (`HarnessHost::kPendingReply`).
- **Restarts.** `SessionPart::machineRestarted(SessionContext&,
  sessions::Restart)` with `enum class Restart { Reset, ColdBoot }` in
  `sessions`, empty by default; added in M-40. LIB-18's `MachineControl`
  calls it after the machine's Reset or Cold boot, before its run to key
  readiness (G9, G10, AC-30's restart rule; LIB-18 §3.4). A
  pause-at-start then holds that run (LIB-18's operation waits while
  paused). The debugger's `RestartKind` (§3.3) is the session's `Restart`
  plus `Build` for a new machine.
  Until LIB-18's product columns (M-46, M-54) the products call it from
  their own reset paths (§5).

## 4. Product side

### M-40

None. The products keep `SharpDebuggerGate` and `TIFreezeGate`. Their
machines return `nullptr` from `debugging()`, and their sessions do not
hold a `DebuggerPart`. The harness entries of both products are
unchanged, so `pause`, `break` and the like refuse with "The machine
offers no debug target." for them until their columns.

### Calc-U-1600, M-45 (Done in M-46)

| Today | After M-45 |
|---|---|
| `debug::DebugTarget`, `PC1500DebugTarget`, `PC1600DebugTarget` (`core/Debug/DebugTarget.hpp`, `MachineDebugTargets.*`, `CpuViews.hpp`, `CpuRegisters.*`) | `machine::Debugging` and one `machine::DebugTarget` per CPU in `core/Debug/` over the same views. Registers of `CpuRegisters.cpp`; spaces `me0`, `me1`, `me1-written`; the SC7852's `memory`, `io`, `io-written` (AC-52); `BankWindow`s from C1600-1's memory map; qualifier fields `bank`, `me`, `pu`, `pv` from `BankKey`; history of 32 |
| `debug::RunControl` (`core/Debug/RunControl.*`) | instruction stepping and over/out move to the library (G4, G5). What stays is the source-line stepping of C1600-3, on the library's run control (M-50) |
| `BreakpointTable` with conditions, hit counts, log messages | the DAP's table over `HitTest` (AC-12; C1600-3, M-50) |
| `SharpDebuggerGate`, `SharpDebugger` (`app/SharpMachineParts.*`), `DebugController::runDebuggerSlice` | gone. `SharpAppPart::sessionParts()` returns `{&m_peripherals, &m_debugger, this}` with a `DebuggerPart` and `CpuOnlyRules` (or Sharp rules for AC-38 if FS 15 needs them) |
| `c1600.trace` kind, `beginTrace`/`endTrace` (`app/SharpAppPart.hpp:190-193`), the INSPECTOR's TRACE button and its limit (`Qt6/app/DebugPanel.cpp:251-272`) | `SharpTraceRecorder : TraceRecorder` over the core's recording into a buffer; the button sends `trace {button: true}` (`TRACE.bin`) |
| the title "Paused (debugger)" (`Qt6/app/MainWindow.cpp:251-255`) | from `run-state`; Machine ▸ Resume in both frontends (AC-41, with LIB-14 1600 in M-49 for the Web's menu) |

The Qt `DebugController` and `DapSession` stay in the Qt frontend and
talk to the library's run control through the session's commands with
the setter `dap` until C1600-3 moves them (M-50; adapter of §5).

### Calc-U-59, M-53 (Done in M-53 or M-58)

| Today | After M-53 |
|---|---|
| `TIDebugTargetAdapter` (`core/TIAdapters.hpp:86-120`) | `machine::Debugging` with one target `TMC0501` in `core/`: today's processor frame as registers; spaces ROM (13-bit words), RAM registers (16 digits), SCOM, program steps; history and decoding; no watches, no step over or out (AC-28). `marks`: step boundary, scan loop |
| `FreezeController`, `FreezeOwner`, `DebugState`, `EmulatorSession::freeze`/`stepKeycode`/`stepInstruction` | `cu59::TILevelRules : LevelRules` (AC-29, AC-30); the owner is the run state's level (`program` = CALCULATOR, `cpu` = CPU) |
| `TIFreezeGate` (`TIAppPart::Hooks::advance`, `EmulatorSession::advanceHosted`) | gone; `DebuggerPart` is the gate. Breakpoints of the harness pause the app (AC-31) |
| `TraceWriter` ×2, `drainTrace`, `c59.trace`, `c59.run-state` (LIB-3 §5 interim topics) | `TITraceRecorder` on `TraceWriter` without the key and card records (AC-33); the library's `trace` and `run-state` topics |
| KEYSTROKES `Freeze:`, `FreezeOnStart:`, `Step:`, `Resume:`, `DebugPanel:`, `Trace:` | read by LIB-6 59 (M-54) as the steps of §3.8 (AC-32). In M-53, before M-54, the old directives call the same session commands |
| FREEZE, STEP, RESUME of the debug panel (Qt6, Web) | `RunControlButtons` per tab (AC-34). Swift: C59-8 (M-58) on `button_cases.json` |
| Web MEMFS `/traces` (LIB-9 A-3) | T1, T4 |

## 5. Adapters

| Adapter | File | Wraps | Removed in |
|---|---|---|---|
| `SharpDebuggerGate` (existing, LIB-3 §5) | `products/calc-u-1600/app/SharpMachineParts.*` | `runDebuggerSlice`, `machineAboutToChange` towards the Qt `DebugController` | M-45 |
| `LegacyCalls::makeDebugTarget` (existing, C1600-1 §5) | `products/calc-u-1600/core/Machine/LegacyCalls.*` | `debug::makeDebugTarget(machine)` | M-45 (its DAP users move to `machine::Debugging` through the adapter below) |
| `c1600.trace` kind (existing, C1600-1 §5) | `core/Machine/SharpProductKinds.*` | `beginCpuTrace`, `endCpuTrace`, `cpuTraceBytes`, `cpuTraceActive` | M-45 (`SharpTraceRecorder`) |
| `DapRunControlBridge` (new in M-45) | `products/calc-u-1600/frontends/qt6/app/debug/` | `DapSession`'s calls on `debug::RunControl` and `debug::DebugTarget` mapped onto the session commands (setter `dap`) and `machine::DebugTarget`; source-line stepping kept on top | M-50 (C1600-3) |
| `TIFreezeGate` (existing, LIB-3 §5) | `products/calc-u-59/app/TIAppPartInternal.hpp`, `EmulatorSession::advanceHosted` | `FreezeController`'s checks inside the advance | M-53 |
| `TIDebugTargetAdapter` (existing, C59-1 §5) | `products/calc-u-59/core/TIAdapters.*` | the debugger's calls on the TI core; also used by `tools/headless/capi.cpp` | M-53; `capi.cpp` moves onto `machine::Debugging` in the same step (plan §1, today's tools) |
| interim topics `c59.run-state`, `c59.trace` (existing, LIB-3 §5) | `TIAppPart` | freeze and trace state | M-53 (the held key of `c59.run-state` goes with LIB-5, M-36) |
| KEYSTROKES debugger directives (new in M-53) | `TIAppPartFlows.cpp` (`K::Freeze`, `K::FreezeOnStart`, `K::Step`, `K::Resume`, `K::DebugPanel`) | the old directives sent as the session commands of §3.6 | M-54 (LIB-6 59 reads the steps of §3.8) |
| restart call (new in M-45 / M-53) | `SharpAppPart`, `TIAppPart` reset paths | `machineRestarted` from the products' own Reset and Cold boot (§3.11) | M-46 / M-54 (LIB-18's product columns) |
| LIB-9 A-3 (existing) | `products/calc-u-1600/app/AppPaths.cpp`, `products/calc-u-59/frontends/web/bindings.cpp` | today's Web trace places | M-45 / M-53 |

M-40 adds no adapter in a product.

## 6. Tasks

Library, M-40. Tests before the code of each task. The files are in
`library/debugger/tests/` unless named otherwise, in
`calcu_library_tests`, and every case is Frequent unless named
otherwise.

**The test machines** (LIB-1 AC-24). `TestCpu` (`library/machine/tests/TestCpu.*`),
switched on by the build option `TestProgram{program, data}`:
- an 8-bit accumulator processor with registers `a`, `x` (8 bits,
  writable), `pc`, `sp` (16 bits, `pc` read-only) and flag `z` (1 bit);
  instructions `nop`, `ld`, `st`, `in`, `out`, `jmp`, `jz`, `call`,
  `ret`, `wait` (stays at its address while a flag is clear), with the
  ticks of the test model's instructions;
- spaces `memory` (256 cells, 8 bits; cells 0x80 to 0xBF banked: 4
  banks, bank 3 with 2 vertical banks; cells 0xF0 to 0xFF ROM, writes
  refused) and `io` (16 cells: 0 to 7 write-only, 8 to 15 a device
  whose read is unreadable), with `io-written` (AC-48);
- a qualifier field `bank` with values `0` to `3`; decoding; a history
  of 8; watches; step over and out;
- `test-seg` gets one such target (`test-cpu`), `test-dot` two (`main`,
  `inner`) that share the bus by turns, so AC-14 and AC-39's library
  form are tested;
- a mark bit for "program step boundary" at every `nop`, which the test
  rules of `TestDebugParts.hpp` use for their `program` level.

The session tests use `library/testing/`'s `TestFrontend` and `ScriptedClock`, with
a test product part holding a `DebuggerPart` with `TestDebugParts`.

| Task | What | ACs/ECs | Test (file: cases) |
|---|---|---|---|
| L1 | `DebugTarget.hpp`, `TestCpu`, conformance | AC-1, AC-2, AC-3, AC-4 | `machine/tests/debug_target_tests.cpp`: `ac1_one_target_per_processor_named_as_descriptor`, `ac1_target_states_registers_spaces_code_space_offers_qualifier_now`, `ac2_registers_read_written_where_writable`, `ac2_peek_no_effect_unreadable_device_cell`, `ac2_refused_poke_changes_nothing`, `ac3_next_address_and_pre_execution_state`, `ac4_history_newest_first_with_registers_before`, `conformance_check_debugging` |
| L2 | `-written` view, banks | AC-48, AC-49, EC-9 | `machine/tests/debug_target_tests.cpp`: `ac48_written_view_last_value_or_none_no_effect`, `ac49_every_bank_and_vbank_by_name_no_switch`, `ac49_range_across_banks_refused`, `ec9_peek_poke_outside_space_refused` |
| L3 | Gate, pause, resume, run state | AC-5, AC-6, AC-14, AC-16, AC-17, EC-1, EC-2 | `run_control_tests.cpp`: `ac5_session_has_one_run_control_unused`, `ac5_paused_no_emulated_time_display_kept`, `ac5_stopped_advance_gives_ticks_run`, `ac6_pause_lands_where_rules_say_reply_then`, `ac6_resume_whichever_level_owned`, `ac6_unknown_level_refused`, `ac14_pause_stops_all_targets_names_current`, `ac16_resume_sets_clocks_and_starts_afresh`, `ac16_paused_survives_suspend_and_continue`, `ac17_run_state_fields_for_each_reason`, `ec1_pause_while_paused_done_nothing_changes`, `ec2_resume_while_running_done` |
| L4 | Steps and owner | AC-7, AC-45, EC-5, EC-6 | `run_control_tests.cpp`: `ac7_cpu_step_one_instruction_of_named_or_current_target`, `ac7_cpu_step_repeats_waiting_instruction_by_rule`, `ac7_program_step_by_rule`, `ac7_running_while_stepping`, `ac7_step_refused_when_not_paused`, `ac7_hit_ends_steps`, `ac45_other_level_step_takes_the_pause`, `ec5_step_from_breakpoint_carries_it_out`, `ec6_large_count_ended_by_pause_reset_new_machine` |
| L5 | Over and out | AC-8 | `run_control_tests.cpp`: `ac8_step_over_call_runs_routine`, `ac8_step_out_runs_to_return`, `ac8_breakpoint_inside_still_pauses`, `ac8_refused_without_offer` (port of Calc-U-1600 `run_control_tests.cpp:159`) |
| L6 | Pause-on-start, pause-at-start | AC-9, AC-46 | `start_tests.cpp`: `ac9_armed_fires_at_rule_event_once`, `ac9_arming_one_disarms_other_off_both_pause_both`, `ac9_default_cpu_rule_leaves_key_readiness`, `ac46_paused_before_first_instruction_after_reset_cold_boot_build`, `ac46_once_and_separate_from_pause_on_start` |
| L7 | Breakpoints | AC-10, AC-12, AC-47 | `breakpoint_tests.cpp`: `ac10_pauses_before_instruction`, `ac10_qualifier_narrows`, `ac10_resume_and_step_pass_it_once`, `ac10_two_targets_only_the_one_to_run_passes`, `ac12_test_says_go_on`, `ac47_ids_owners_remove_only_own`, `ac47_hit_names_ids_and_owners` (ports of Calc-U-1600 `debug_target_tests.cpp:111-193, 464`) |
| L8 | Watches | AC-11, EC-3, EC-4 | `breakpoint_tests.cpp`: `ac11_read_write_both_pause_after_instruction`, `ac11_fetch_is_no_access`, `ac11_refused_without_offer`, `ec3_past_end_of_space_refused`, `ec4_several_hits_pause_once_lowest_address_named` (ports of `debug_target_tests.cpp:269-301`) |
| L9 | Operations | AC-13, EC-7 | `run_control_tests.cpp`: `ac13_no_breakpoint_or_watch_in_operation`, `ac13_commanded_pause_step_resume_in_operation`, `ac13_armed_start_fires_in_operation` (a test preset that arms and fires inside its load, the library form of `test_cdebug.ti59`, with a key held when it fires, C-1), `ec7_preset_ends_paused_stays_paused` |
| L10 | Restart, new machine | AC-15 | `run_control_tests.cpp`: `ac15_reset_and_cold_boot_while_paused_run`, `ac15_armed_stays_across_reset`, `ac15_new_machine_not_paused_nothing_armed`, `ac15_breakpoints_rebound_by_target_name_rest_dropped` |
| L11 | Trace session | AC-22, AC-23, AC-24, AC-25, EC-8 | `trace_tests.cpp`: `ac22_trace_records_every_instruction_until_off`, `ac22_ends_with_its_preset_or_new_machine`, `ac22_replaces_file_one_at_a_time`, `ac22_slash_refused_at_read`, `ac22_cannot_make_file_step_fails`, `ac23_button_name_is_products`, `ac24_limit_ends_complete_file_notice`, `ac24_new_machine_log_line`, `ac25_trace_topic_whoever_started`, `ec8_no_writable_folder_step_fails_with_reason` |
| L12 | Web hand-over | AC-26 | `trace_tests.cpp`: `ac26_ended_trace_in_memory_handed_with_keep_file`; `library/web/tests/run-control.spec.js`: `ac26 an ended trace is a download` (Longer, in `LibraryWebPlaywright`) |
| L13 | Preset steps | AC-19 | `steps_tests.cpp`: `ac19_each_step_does_what_its_ac_says`, `ac19_level_view_count_out_of_range_refused_at_read`, `ac19_step_not_paused_logs_and_goes_on`, `ac19_undefined_steps_refused_for_a_model`, `ac19_trace_defined_for_every_model`, `ac19_no_step_waits_for_a_running_machine`, `ac19_debug_panel_is_a_frontend_step` |
| L14 | Harness commands | AC-20, AC-21 | `library/harness/tests/debug_commands_tests.cpp`: `ac20_targets_registers_set_register`, `ac20_peek_poke_marks_unreadable_refuses_at_first`, `ac20_break_watch_on_off_list_own_only`, `ac20_history_disassemble_with_text`, `ac20_profile_counts_and_report`, `ac20_step_over_out_refused_without_offer`, `ac20_pause_at_start`, `ac20_state_run_state_and_trace`, `ac20_step_not_paused_refused`, `ac21_wait_ends_at_pause_gives_time_and_state`, `ac21_wait_while_paused_ends_at_once` |
| L15 | More harness commands | AC-50, AC-51 | `library/harness/tests/debug_commands_tests.cpp`: `ac50_peek_into_file_gives_count`, `ac50_expect_register_refused_with_value`, `ac50_expect_memory_refused_at_first_difference`, `ac50_refused_expect_ends_one_shot_nonzero` (CLI case in `cli_cases.cmake`), `ac51_symbol_names_as_addresses_and_in_output`, `ac51_unknown_name_refused` |
| L16 | Buttons | AC-18 | `button_rule_tests.cpp`: `ac18_rule_cases_from_table`, `ac18_step_repeats_four_a_second_while_held`; `library/qt6/tests/run_control_buttons_tests.cpp`: `ac18_qt6_buttons_follow_run_state_and_send_commands` (offscreen); `library/web/tests/run-control.spec.js`: `ac18 react buttons follow the run state` (Longer) |

Coverage, Library: 32 of 32 ACs (AC-1–26, AC-45–51) and 9 of 9 ECs.

## 7. Tools and agent knowledge

**Tools.** In M-40 the harness gains AC-20's commands for the test
product only. Today's tools stay as they are: `tools/headless/`
(Calc-U-59; `capi.cpp` moves onto `machine::Debugging` in M-53, §5),
`pc1500_cli` and `pc1600_cli` (until M-51). Both `tools/read_trace.py`
stay scripts and read the unchanged formats (AC-33, AC-42).
`tools/dap_smoke.py` runs unchanged until M-50.

**Agent knowledge** that changes with the code:

| Step | Files |
|---|---|
| M-40 | `docs/test-groups.md` (the new library files; `calcu_qt6_tests`; the two Playwright cases); `docs/glossary-mapping.md`: the "Pause" row names `calcu::debugger::RunControl` as the library's side. No glossary term is new: breakpoint, watch, history, level, run control, pause and trace are there |
| M-45 | `products/calc-u-1600/docs/developer/Debugger-Internals.md` (`SharpDebuggerGate`, `RunControl` lines 43 on), `Shared-Architecture.md` (row `SharpMachineParts`); the Calc-U-1600 memory files on the debugger and traces; `docs/glossary-mapping.md` "Pause" row (`paused`, `DebugController`) |
| M-53 | `products/calc-u-59/reference/DebugAPI.md`, `reference/SharedArchitecture.md:104`, `reference/CoreArchitecture.md`; the skills `calc-u-59-core` (trace infrastructure), `calc-u-59-shared` (freeze state, `DebugModel`, trace writer), `calc-u-59-uitesting` (traces, plan M-53); `docs/glossary-mapping.md` "Pause" row (`freeze`, `FreezeController`, `FreezeOwner` go); the Calc-U-59 memory files that name `FreezeController` or `TraceWriter` |

## 8. Golden runs and risks

- **Golden runs.** M-40 touches no core and no product. The golden runs
  of both products cannot change. In M-45 and M-53 the debug targets
  wrap the cores. With all checks off, `advance` takes the path it takes
  today. A golden run that differs then is a finding.
- **Cost per instruction.** The observer is a virtual call per
  instruction while it is on (G1). The TI core pays it today whenever
  `setRunControl` is set. The Sharp cores today check breakpoints inside
  the CPU and never call out, so their per-instruction observer comes
  only with stepping, an armed start or the profile. M-45 measures a
  PC-1600 at full speed with a breakpoint set against 0.8.1.
- **Interface churn.** `Machine::debugging()` has a default, so M-40
  rebuilds both products' machines without a change (LIB-1 §8).
- **Deferred replies in virtual time.** A preset loaded through the
  harness that pauses itself and then needs the machine (a `key:` after
  `pause:`) cannot finish inside `send()`. LIB-3 ends it with "The
  operation did not finish." (`AppSession.cpp:305-307`). LIB-6 AC-19
  says such a step waits for a resume, which the harness cannot give
  inside one command. This design keeps LIB-3's ending. The reply names
  the paused machine (D-8). For LIB-6's design to confirm.
- **The parallel LIB-6 and LIB-18 designs.** §3.8 uses LIB-6's draft
  (`StepDef`, `Vocabulary`, `Item`, `After`, `needsRunningMachine`,
  `frontend`). T3 needs no signal of LIB-6: the end of the load is the
  end of its operation (`operationEnded`). §3.11 adds one call to
  LIB-18's `MachineControl`. Both drafts' risk "an operation on a paused
  machine never ends in virtual time" is answered by D-8. A pause-at-start
  inside LIB-18's Reset operation holds it before key readiness; in the
  harness that Reset then ends as D-8 says.
- **The Qt6 component set's place** (D-9) is shared with LIB-8's paper
  panel (M-39), LIB-7's pickers (M-41) and LIB-14's shell (M-43). M-39
  creates the folder and the target; M-40 adds to them.
- **The size limit.** The Sharp core writes its trace file itself today
  and checks the size from outside. T2's budget asks the recorder for
  whole records within it. So M-45 turns the core's recording into a
  buffer that `SharpTraceRecorder` drains; 0.8.1 overshoots by up to a
  frame.

## 9. Decided (best guess)

- **D-1 (AC-1, DB-1).** Register and cell values are 64-bit unsigned;
  wider ones are refused by `checkDebugging`. Both products fit. DB-1's
  "any width" holds up to 64 bits.
- **D-2 (AC-8).** Step over and out need only `Flow` at an address and a
  `callDepth()` that grows with calls. The direction of a stack stays in
  the target, so the library names no processor.
- **D-3 (AC-5, AC-10).** The stop is reported by
  `Debugging::takeStop()` after an `advance` that ended early.
  `advance` keeps its signature (LIB-1 §3.5).
- **D-4 (AC-20).** Numbers in: decimal, `0x…`, `$…`. Out: `0x` with
  the digits of the width. Not in the spec; today's tools print hex.
- **D-5 (AC-51).** `symbols` takes the product's readers; the library's
  tests use a table format of their own. A name shows as `name` or
  `name+offset`. Which assembler outputs each product reads is its
  column's (Calc-U-1600 listings, M-45; the TI has none today, so
  Calc-U-59 binds none unless C59-4's ASM overlay format counts, M-56).
- **D-6 (AC-6, AC-7, AC-20).** In the harness a pause or step that does
  not land at once runs the machine until it lands, at most 3600 s of
  emulated time (the preset's default `timeout:`, LIB-6 AC-57), then is
  refused with "The machine did not pause.".
- **D-7 (AC-20).** `profile report` lists 20 addresses by default.
- **D-8 (AC-13, LIB-6 AC-19).** A load through the harness that pauses
  and then needs the machine ends as LIB-3 ends it, "The operation did
  not finish.", with the run state in the reply (§8).
- **D-9 (AC-18; TR).** The run-control buttons are a component of each
  set, built in M-40 before LIB-14's shells. The Qt6 component set lives
  in `library/qt6/`, namespace `calcu::qt6`, in its own static target
  `calcu_qt6` (Qt6::Widgets), built only where Qt6 is found; M-39 (LIB-8
  D-1) creates it as the fourth exception in LIB-1 §2a's list, since
  `calcu_library` has no Qt and is linked into Swift and Wasm builds.
  The React component goes to `library/web/components/`, in
  `COMPONENT_SOURCES` (LIB-14 D-16, D-17).
- **D-10 (AC-22, AC-26).** A trace on a host without a trace folder is
  kept in memory when a listener offers `KeepFile`, and fails otherwise.
  So Calc-U-1600's Web, which states no trace hand-over in its
  `DebuggerSetup`, fails `trace:` (AC-40, EC-8), and Calc-U-59's Web
  gets the download (AC-26).
- **D-11 (AC-24).** The notice reads "The trace <name> reached the size
  limit of <n> MB and ended."; the log line "The trace <name> ended: a
  new machine was built.". Not part of the contract; LIB-3's sentences
  set the tone.
- **D-12 (AC-34).** The Calc-U-59 column is Done in M-53 if the Swift
  debug panel can use the rule table of §3.10 without C59-8's panels;
  otherwise in M-58, as plan §5.1 allows.
- **D-13 (AC-47).** Setters are named strings: `harness`, `dap`. A
  preset's steps set no breakpoints, so a preset needs no setter.
- **D-14 (AC-17).** While a step runs or a pause has not landed, the run
  state says running (`paused = false`); no third state is sent.
- **Open point (AC-38, FS 15).** Whether the default `cpu` start rule
  (key readiness left after it was seen) is what FS 15 found on the
  PC-1500 and PC-1500A, or whether Sharp rules are needed, is settled
  in M-45 from the research of M-5.

## 10. Part D: the Calc-U-1600 column

Status of this section: approved at the Part C2/D boundary (2026-11-16)
(2026-11-16). §1 to §9 above are the approved Library column and are not
changed. Where M-40's code differs from them, the LIB-10 qa-report's
"Deviations" is the record, and this section builds on the code. §4's
outline for Calc-U-1600 and §5's Calc-U-1600 rows are replaced by this
section; where it departs from them, it says so (D-15, D-16).

### 10.1 Scope

| Column | Step | Tests from (plan §3) | Done in (plan §5.1) |
|---|---|---|---|
| Calc-U-1600 | M-45 (with LIB-8 1600) | AC-36–44, AC-52 (AC-38 per FS 15) | M-46 |

- M-45 waits for M-40 (the library's debugger), M-31 (the product on the
  library's sessions), M-5 (FS 15: key readiness on the PC-1500) and
  M-44 (C1600-2). It builds on `main` after M-43.
- "Done in M-46": three clauses name format 2 presets, which Calc-U-1600
  reads from M-46 (LIB-6 1600): AC-40's "the preset steps `pause:`,
  `pause-on-start:`, `step:`, `resume:` and `debug-panel:` are refused
  when a preset is read" and "`trace:` fails in the Web frontend"; and
  AC-43 for a preset's `trace:` step. In M-45 the version 1 presets' trace
  key runs through the library's trace session by an adapter (L-A3), so
  AC-43 is tested on version 1 in M-45 and on format 2 in M-46. Every
  other criterion passes in M-45.
- The edge cases EC-1 to EC-9 belong to the Library column (M-40).
- Not here: the DAP server, its source lines, listings bound to loads,
  conditions, hit counts, log messages and expressions (C1600-3, M-50;
  `features/C1600-3-dap-in-the-app-layer/design.md`). M-45 builds two
  pieces of C1600-3 early, without the server (D-15). The INSPECTOR
  stays as it is; only its TRACE button changes (AC-23).
- The open point at the end of §9 (AC-38) is settled here: D-17.

### 10.2 What the code is today (main, `60e5b09`)

- The gate: `app/SharpMachineParts.hpp:14-37`, `.cpp:11-19`
  (`SharpDebugger`, `SharpDebuggerGate`); `SharpAppPart::setDebugger`
  (`app/SharpAppPart.hpp:116`); `m_ms->setGate(&m_gate)`
  (`SharpAppPart.cpp:189`); `machineReplaced`, `machineAboutToChange`
  (`:629, 697`). `MachineController` is the `SharpDebugger`
  (`frontends/qt6/app/MachineController.cpp:38-39, 250-263`).
- The core's debugger: `core/Debug/DebugTarget.hpp/.cpp` (threads,
  registers by name, `peek`/`poke` per thread and space, `bankAt`,
  `bankState`, `BankKey`, history, breakpoints, watches, `arm`,
  `runMachine`, `stepMachine`, `resumeFromStop`, `reset`),
  `MachineDebugTargets.hpp/.cpp` (`PC1500DebugTarget`, `PC1600DebugTarget`,
  `makeDebugTarget`), `CpuViews.hpp`, `CpuRegisters.*`, `RunControl.*`
  (frame slices, line and instruction steps, over and out),
  `BreakpointTable.*` (the client's breakpoints with conditions, hit
  conditions and log messages). In the CPUs: `core/CPU/BreakpointSet.hpp`,
  `WatchSet.hpp`, `DebugStop.hpp`, `HistoryRing.hpp` (32 entries).
- `LegacyCalls::makeDebugTarget` and `loadProgram(…, const
  debug::DebugTarget&)` (`core/Machine/LegacyCalls.hpp:44-47`).
- The Qt debugger: `frontends/qt6/app/debug/DebugController.*`
  (`runSlice`, `createTarget`, `resetMachine`, `machineAboutToChange`,
  `machineReplaced`, `DebugController.cpp:122-303`), `DapSession.*`,
  `DapServer.*`. The title "Calc-U-1600 — Paused (debugger)"
  (`MainWindow.cpp:240-241`).
- Traces: the adapter kind `c1600.trace` (`SharpProductKinds.hpp:88-98`,
  offered at `PC1500InterfaceMachine.cpp:52-59, 156` and
  `PC1600InterfaceMachine.cpp:61-67, 248`); `SharpAppPart::beginTrace`,
  `endTrace`, `traceBytes`, `traceActive` (`SharpAppPart.cpp:823-845`).
  The cores write the file themselves (`PC1500Machine::beginCpuTrace`,
  `endCpuTrace`, `pumpTraceFile`, `PC1500Machine.cpp:235-262`;
  `PC1600Machine.cpp:667-690`; `PC1500TraceFile.*`), draining the CPUs'
  rings every 256 instructions. The version 1 preset key `trace` opens the
  file in the core's runner (`core/Preset/PresetRunner.cpp:110-131`) and
  closes it at the end (`:316-323`, `TraceCloser`). The INSPECTOR's TRACE
  button and its limit of `maxMB × 1'000'000` bytes
  (`frontends/qt6/app/DebugPanel.cpp:239-272`). The Web's trace place is
  LIB-9 A-3 (`app/AppPaths.cpp:100-101`).
- The library after M-40 (qa-report, "Deviations"): restarts are told
  through `SessionContext::tellRestarted`, carried out by LIB-18's
  `StartupPart`; run-control commands on a machine whose `debugging()` is
  `nullptr` are refused "The machine offers no debug target."; the
  harness forms `step <level> [<count>]`; `TraceRecorder::machineGone()`;
  `DebuggerSetup` with `TraceSetup`, the symbol readers, the per-model
  steps and the panel's views.

### 10.3 Modules and files (M-45)

| File | Change |
|---|---|
| `core/Debug/SharpDebugging.hpp/.cpp` (new) | `sharp::SharpDebugging : calcu::machine::Debugging` for each model, and `SharpDebugTarget : calcu::machine::DebugTarget`, one per CPU, over today's `CpuView`s and the machines' memory access (§10.4, §10.5). They take over what `PC1500DebugTarget` and `PC1600DebugTarget` did |
| `core/Debug/DebugTarget.hpp/.cpp`, `MachineDebugTargets.hpp/.cpp` | deleted. `Register`, `BankField`, `HistoryEntry`, `CpuKind`, `flagNames` and `statusFlags`, which the disassembler, the expressions and the DAP use, move to `core/Debug/CpuRegisters.hpp` |
| `core/Debug/RunControl.hpp/.cpp` | deleted: instruction steps, over and out are the library's (G4, G5); source-line steps move to `app/dap/DapStepping` (D-15) |
| `core/Debug/BreakpointTable.hpp/.cpp` | moved to `app/dap/DapBreakpoints.hpp/.cpp` on the library's `BreakpointTable` with the setter `dap` and `HitTest` (D-15) |
| `core/CPU/LH5801/*`, `core/CPU/SC7852/*`, `core/PC1600/PC1600Memory.*` | the `-written` views (AC-52): the value last written to each ME1 cell that reads back unreadable, and to each `io` port of the SC7852, kept since the machine was built; reading them has no effect |
| `core/Machine/SharpMachine.hpp/.cpp` | `debugging()` returns the model's `SharpDebugging`. `advance(atLeast)` honours the `Checks` (§10.5) |
| `core/PC1500/PC1500InterfaceMachine.*`, `core/PC1600/PC1600InterfaceMachine.*` | own the `SharpDebugging`; the `Trace` adapter goes; the product kind of the trace records is offered instead (§10.7) |
| `core/Machine/SharpProductKinds.hpp` | `CpuTrace` (`c1600.trace`) goes. New product kind `sharp::TraceRecords` (`c1600.trace-records`, owner: the main CPU, which stays): `begin()`, `take(std::vector<std::uint8_t>&, budget) → bool`, `endRecord()`, `end()` (D-18) |
| `core/PC1500/PC1500TraceFile.*`, `PC1500Machine.*`, `PC1600Machine.*` | the trace writer writes whole records into a byte buffer instead of a `FILE*`; `beginCpuTrace(FILE*, …)` and `pumpTraceFile` become `beginTraceRecords()` and the drain into the buffer. The format and its version 3 do not change (AC-42) |
| `core/Machine/LegacyCalls.hpp/.cpp` | `makeDebugTarget` goes. `loadProgram` takes `const calcu::machine::Debugging&` until it goes in M-50 (C1600-3, on C1600-2's loader) |
| `core/Preset/PresetRunner.*`, `LegacyCalls::applyPresetJob` | the `trace` key calls a `PresetTraceFn` the app hands in, instead of opening a file (L-A3); `TraceCloser` goes (the operation's end ends the trace, T3) |
| `app/SharpTraceRecorder.hpp/.cpp` (new) | `shared::SharpTraceRecorder : calcu::debugger::TraceRecorder` on `c1600.trace-records`; `buttonFileName()` is `TRACE.bin` (AC-42) |
| `app/SharpDebuggerSetup.hpp/.cpp` (new) | `shared::sharpDebuggerSetup(const AppPaths&, const Settings&, bool hasTraceFolder)`: `CpuOnlyRules` (D-17); `TraceSetup` with `SharpTraceRecorder`, `traceFolderOf(paths)` where the host has a trace folder, `[trace] max-file-size-mb`, `handOver = false` (LIB-10 D-10: the Web fails `trace:`, AC-40); `runControlSteps` false for the three models (AC-19); no panel views; the symbol readers of §10.8 |
| `app/SharpSymbolReaders.hpp/.cpp` (new) | `calcu::debugger::SymbolReader`s for sdas `.lst`/`.rst`, zasm `.lst` and a `.SYMBOLS:` table, over `core/Debug/Listing/` (LIB-10 D-5; AC-51 for Calc-U-1600) |
| `app/SharpMachineParts.hpp/.cpp` | deleted (`SharpDebugger`, `SharpDebuggerGate`) |
| `app/SharpAppPart.hpp/.cpp` | a member `calcu::debugger::DebuggerPart m_debugger{sharpDebuggerSetup(…)}` among `sessionParts()` (LIB-8 §10.3 gives the order); `setDebugger`, `m_gate`, `beginTrace`, `endTrace`, `traceBytes`, `traceActive` and the debugger calls in `installMachine`/`discardMachine` go; `resetJob`, `resetToPromptJob` and `selectModelJob` tell `tellRestarted` (L-A2); the version 1 preset's trace through `m_debugger.traceSession()` (L-A3) |
| `app/HarnessProduct.cpp` | a `DebuggerPart` on `sharpDebuggerSetup(…)` among `sessionParts()`, returned by `debuggerPart()`, so `targets`, `pause`, `break`, `watch`, `trace`, `symbols` and the rest of AC-20 work for Calc-U-1600 |
| `frontends/web/bindings.cpp`, `app/AppPaths.cpp` | the Web builds `SharpAppPart` with `hasTraceFolder = false`, so its `DebuggerPart` has no trace folder; `app/AppPaths.cpp:100-101` (LIB-9 A-3) goes |
| `app/dap/DapBreakpoints.hpp/.cpp`, `app/dap/DapStepping.hpp/.cpp`, `app/dap/DapTargets.hpp/.cpp` (new; C1600-3's folder, D-15) | the client's breakpoints and watches on the library's table (setter `dap`); source-line steps on the library's `step` with a test (D-16); threads, frames, registers by name with their halves and flags, banks and memory references over `machine::DebugTarget`. No Qt type |
| `frontends/qt6/app/debug/DebugController.*`, `DapSession.*` | call `app/dap/` and the `DebuggerPart` instead of `debug::DebugTarget`, `debug::RunControl` and `debug::BreakpointTable`; `runSlice`, `createTarget`, `machineAboutToChange` and `machineReplaced` go; `pausedChanged` comes from the `run-state` topic. They stay in the Qt frontend until M-50 (L-A1) |
| `frontends/qt6/app/MachineController.*` | no longer a `SharpDebugger`; `runDebuggerSlice` goes; it relays `run-state` and `trace` to `MainWindow` and `DebugPanel` |
| `frontends/qt6/app/MainWindow.cpp` | the title "Calc-U-1600 — Paused (debugger)" while `run-state` says paused, whoever paused it; Machine ▸ Resume, enabled while paused, sends `resume` (AC-41; L-A4) |
| `frontends/qt6/app/DebugPanel.cpp` | the TRACE button sends `trace {button: true}` and `trace {off: true}`, and shows its state from the `trace` topic; its own limit and its INSPECTOR lines (`:239-272`) go: the library ends the trace and gives the notice (AC-23, AC-24, AC-43) |
| `frontends/web/src/MenuBar.jsx`, `engine.js` | Machine ▸ Resume, enabled while `run-state` says paused (AC-41; L-A4) |
| `library/debugger/RunControl.hpp/.cpp` (changed) | `step` takes an optional `StepTest until`: after each counted step the run control asks it, and goes on while it says so, up to the count (D-16) |
| `tests/debug_target_tests.cpp`, `tests/run_control_tests.cpp`, `tests/presetloader_trace_tests.cpp`, `tests/debugger_tests.cpp` (new), `tests/dap_layer_tests.cpp` (new), `tests/harness_tests.cpp` | §10.10 |
| `tests/gui/lib10-ac41.shots.yaml` (new) | the Qt6 check scenario `CalcU1600Shots.lib10-ac41` (Longer) |
| `frontends/web/tests/play.spec.js` | §10.10 |

### 10.4 The Sharp debug targets (AC-36, AC-37, AC-52)

| | LH5801 (PC-1500, PC-1500A) | SC7852 (PC-1600, the Z-80) | LH5803 (PC-1600) |
|---|---|---|---|
| Name (C1600-1 descriptor, `SharpCatalogue.cpp:93, 132`) | `LH5801` | `SC7852` | `LH5803` |
| Registers | today's (`CpuRegisters.cpp:34-227`), each with its width (8, 16, 1) and whether it is written | the same | the same |
| Spaces | `me0`, `me1`, `me1-written` | `memory`, `io`, `io-written` | `me0`, `me1`, `me1-written` |
| Code space (a breakpoint's address) | `me0` | `memory` | `me0` |
| Offers | watches, step over and out, decoding, history 32 | the same | the same |
| Qualifier fields | `me`, `pu`, `pv` | `bank` (`0` to `7`) | `me`, `pu`, `pv` |

- **Cells.** Every space has 8-bit cells. `me0`, `me1` and `memory` have
  65536 cells. `io` has as many as the SC7852's I/O map has, from
  C1600-1's memory map (`features/C1600-1-sharp-machines-on-the-machine-interface/design.md`
  and the research skill `PC1500`); this design restates no number.
- **Reading and writing (AC-37)**, as today
  (`MachineDebugTargets.cpp:68-105`): ROM, cells where nothing is and
  I/O refuse a write; `me1` is never written; a cell of `me1` whose
  reading would disturb a device reads as unreadable; on the PC-1600 the
  LH5803's memory is written only where it is RAM. `io` is read only
  where reading has no effect, and is never written through the target.
- **`-written` views (AC-48, AC-52).** For the cells that read back as
  unreadable, the value last written by the processor since the machine
  was built, or none.
- **Banks (AC-49, AC-52).** The `BankWindow`s are the banked ranges of
  C1600-1's memory map: the PC-1600's banks, those of the modules in its
  slots, and the vertical banks of a module in Slot 2. `peek` and `poke`
  with `bank=`/`vbank=` go through the cores' own bank access, as the
  INSPECTOR's slot images do (`InspectorViews::slotImage`), and switch no
  bank.
- **Qualifier.** `qualifierMatches` is today's `bankMatches`
  (`DebugTarget::bankMatches`, `BankKey`). `qualifierNow` gives the bank
  of the page the program counter is in (SC7852), and `me`, `pu`, `pv`
  (LH580x).
- **History.** 32 entries per CPU (`LH5801.hpp:189`), newest first, each
  with the registers before it and its text from the disassembler.
- **Flow and depth (AC-8).** `flowAt` from the disassembler's kind of
  instruction (call, vector call, return); `callDepth` is `0x10000 − sp`
  (both stacks grow downwards), so a VMJ whose inline parameters move the
  return address is stepped over as today (`RunControl.cpp:107-157`).

### 10.5 How a Sharp machine advances under the run control (AC-5, AC-39)

- **All checks off** (no breakpoint, no watch, no observer): `advance`
  takes today's path. The golden runs see no change (§10.11).
- **Breakpoints and watches** on: the CPUs' own lists are armed
  (`BreakpointSet`, `WatchSet`), as `DebugTarget::arm(true)` did inside
  `runSlice`. The core runs as today and ends the advance at the latched
  stop (`DebugStop`). `takeStop()` turns the latch into a `DebugStop`.
  While an operation runs the library leaves them off (G1), which is
  today's "armed only inside `runSlice()`" rule
  (`docs/developer/Debugger-Internals.md`, "Arming").
- **Observer** on (a step, a pause that waits, an armed start, the
  profile): the machine runs one machine step at a time, as
  `stepMachine()` did (the bus owner carries out one instruction, or idles
  one halted tick), and calls the observer after each instruction with
  the index of the CPU that carried it out.
- **Two CPUs (AC-39, AC-14).** `current()` is the bus owner
  (`PC1600DebugTarget::busOwner`). `passOnce(t)` is today's skip-once of
  the CPU `t` alone (`DebugTarget::resumeFromStop`,
  `debug_target_tests.cpp:464`).

### 10.6 Levels (AC-38)

`cpu` only. A pause lands before the next instruction. `pause-on-start
cpu` (a harness command; the step is refused on the Sharp models, AC-19)
fires on the first instruction after which `Machine::isKeyReady()` is
false, once it was seen true: the library's default rule. FS 15 found key
readiness on the PC-1500 and PC-1500A (`docs/walkthroughs/first-steps.md`
§15), and `SharpMachine::isKeyReady()` already models it on all three
models since M-36 (LIB-5 AC-31). So `CpuOnlyRules` suffice (D-17).

### 10.7 Traces (AC-22–25, AC-42, AC-43)

- **One trace session.** The library's `TraceSession` starts, limits and
  ends every trace: the step and the command (`trace <file>`), the
  INSPECTOR's button (`TRACE.bin`, AC-23), and the version 1 preset key
  through L-A3. One trace at a time; a new machine ends it with a log line;
  the limit ends it with the library's notice (LIB-10 D-11).
- **The records are the core's.** `SharpTraceRecorder::begin` gives the
  header and the session start and turns the core's recording on; `take`
  gives whole records from the core's buffer, at most the budget; `end`
  gives what waits and the session end (since P-5, §11: the library's
  `TraceSession` writes them and keeps the limit). The bytes are those of today's file: header
  `PC15` and version 3; the records of both CPUs of a PC-1600 in one file;
  gap records where a ring overflowed (`PC1500TraceFile.cpp:9-144`).
  `tools/read_trace.py` reads them unchanged.
- **The limit** is `[trace] max-file-size-mb` MiB (LIB-10 T2), for every
  trace (AC-43). Today the button's limit counts 1'000'000 bytes per MB and
  a preset's trace has none (`DebugPanel.cpp:251-252`).
- **Where.** Qt6 and the harness: the trace folder (LIB-9). Web: no trace
  folder and no hand-over, so a trace fails with the reason (AC-40, EC-8).

### 10.8 The run control in the hosts (AC-40, AC-41)

- **Qt6.** `SharpAppPart`'s `DebuggerPart` is the machine session's
  gate whether or not a DAP client is attached. Pause, step and resume
  reach it from the DAP (setter `dap`), from the harness attached to a
  development build (M-49 wires attach for Calc-U-1600; until then the
  session commands from a test host) and from Machine ▸ Resume.
- **The harness.** `calcu --product 1600` has the `DebuggerPart`, so
  every command of AC-20, AC-50 and AC-51 works on the Sharp targets;
  `symbols` reads the Sharp listings (§10.3).
- **Web.** The same part in the worker; Machine ▸ Resume in the page.
  The page has no harness, so the Playwright case sends `pause` and
  `resume` as session commands (D-19).
- **The DAP in M-45 to M-50.** The Qt `DapSession` keeps its requests and
  its JSON, and reaches the machine only through `app/dap/` and the
  `DebuggerPart`: breakpoints and watches with the setter `dap` and their
  tests (conditions, hit counts, log messages), line steps by D-16,
  threads and memory by `machine::DebugTarget`. `stopped` events come from
  the `run-state` changes. `tools/dap_smoke.py` passes against the Qt app
  as before (§10.11). C1600-3 (M-50) then moves the server, the session
  and the queue out of the Qt frontend.

### 10.9 Adapters

Removed in M-45 (LIB-10 §5's Calc-U-1600 rows):

| Adapter | Where |
|---|---|
| `SharpDebuggerGate`, `SharpDebugger` | `app/SharpMachineParts.*`, `MachineController::runDebuggerSlice` |
| `LegacyCalls::makeDebugTarget` | `core/Machine/LegacyCalls.*` |
| `c1600.trace` (`CpuTrace`) | `core/Machine/SharpProductKinds.hpp`, the interface machines |
| LIB-9 A-3 (the Web's trace place) | `app/AppPaths.cpp:100-101` |

New in M-45, each with the step that removes it:

| Adapter | Where | Wraps | Removed in |
|---|---|---|---|
| L-A1 Qt DAP | `frontends/qt6/app/debug/DapServer.*`, `DapSession.*`, `DebugController.*` | the server, its requests in `QJsonObject` and the message queue, now on `app/dap/` and the `DebuggerPart` | M-50 (C1600-3) |
| L-A2 restart call | `SharpAppPart::resetJob`, `resetToPromptJob`, the DAP's reset | `tellRestarted(Reset/ColdBoot)` from the product's own Reset and Cold boot (LIB-10 §3.11) | M-46 (LIB-18 1600: `StartupPart::restart` tells it) |
| L-A3 preset trace hook | `PresetRunner`'s `trace` key, `LegacyCalls::applyPresetJob`, `SharpAppPart::loadPresetJob` | a version 1 preset's trace started and ended through `TraceSession::start(…, ofOperation = true)` | M-46 (LIB-6 1600: the step `trace:`) |
| L-A4 Resume and title | `MainWindow.cpp`, `MenuBar.jsx` | Machine ▸ Resume and the title in today's menus and window | M-49 (LIB-14 1600: the shell's Machine menu, LIB-14 AC-1) |
| `LegacyCalls::loadProgram` (changed signature) | `core/Machine/LegacyCalls.*` | Build & Load on the old loader | M-50 (C1600-3 on C1600-2's loader) |

LIB-10 §5 named a `DapRunControlBridge` in `frontends/qt6/app/debug/`
for M-45 to M-50. D-15 puts its content in `app/dap/` instead, as code
of C1600-3 that M-50 keeps; the adapter is L-A1 only.

### 10.10 Tasks (AC → task → test)

Cases in `products/calc-u-1600/tests/` are in `CoreTests` (Frequent)
unless named otherwise. Session cases use `app/tests/SessionFixture.hpp`
with `SharpAppPart`. Tests come before the code of each task. Names are
`lib10_acN_…`; the ported cases keep today's names after the prefix.

| Task | What | AC | Test (file: cases; group) |
|---|---|---|---|
| D16-1 | `SharpDebugging`, targets | AC-36 | `debug_target_tests.cpp`: `lib10_ac36_pc1500_and_pc1500a_have_the_lh5801`, `lib10_ac36_pc1600_has_the_sc7852_and_the_lh5803`, `lib10_ac36_registers_spaces_offers_history_32`, `lib10_ac36_qualifier_fields_bank_me_pu_pv`, `lib10_ac36_conformance_on_every_model` (`checkDebugging` on PC-1500 A01/A03/A04, PC-1500A, PC-1600 New/Old); Frequent |
| D16-2 | Reading and writing | AC-37 | `debug_target_tests.cpp`: `lib10_ac37_rom_empty_cells_and_io_refuse_a_write`, `lib10_ac37_me1_never_written_device_cells_unreadable`, `lib10_ac37_lh5803_written_only_in_ram`, `lib10_ac37_refused_poke_changes_nothing`; Frequent |
| D16-3 | Banks, `io`, `-written` | AC-52 | `debug_target_tests.cpp`: `lib10_ac52_sc7852_io_space`, `lib10_ac52_written_views_of_me1_and_io`, `lib10_ac52_every_bank_of_the_pc1600_and_its_modules`, `lib10_ac52_vertical_banks_of_a_slot_2_module`; `harness_tests.cpp`: `lib10_ac52_harness_peek_bank_and_vbank`; Frequent |
| D16-4 | The advance with checks | AC-39, AC-5 | `debugger_tests.cpp`: `lib10_ac39_step_of_one_target_other_runs_with_the_bus`, `lib10_ac39_only_the_target_about_to_run_passes_its_breakpoint`, `lib10_checks_off_take_the_old_path` (tick counts equal with and without the part); Frequent |
| D16-5 | Level `cpu`, pause-on-start | AC-38 | `debugger_tests.cpp`: `lib10_ac38_cpu_only_pause_lands_before_the_next_instruction`, `lib10_ac38_program_level_refused`, `lib10_ac38_pause_on_start_fires_leaving_key_readiness_pc1500`, `_pc1500a`, `_pc1600`; Frequent |
| D16-6 | The run control in the hosts | AC-40 | `harness_tests.cpp`: `lib10_ac40_harness_pause_step_resume`, `lib10_ac40_harness_breakpoint_and_watch`; session: `lib10_ac40_run_control_without_a_debugger` (Qt6's part, no DAP client); Web: `play.spec.js` `lib10_ac40 run control pauses and resumes the web machine`; Frequent, Longer. In M-46: `lib10_ac40_debugger_steps_refused_at_read`, `lib10_ac40_trace_step_fails_on_the_web` (format 2) |
| D16-7 | Title and Resume | AC-41 | Qt6: `CalcU1600Shots.lib10-ac41` (a session `pause`, `expect-title: "Calc-U-1600 — Paused (debugger)"`, `action: Machine > Resume`, `expect-title: "Calc-U-1600"`); Web: `play.spec.js` `lib10_ac41 machine resume`; Longer |
| D16-8 | Trace format | AC-42, AC-23 | `presetloader_trace_tests.cpp` ported to the trace session: `lib10_ac42_trace_step_produces_wellformed_file`, `_left_open_is_closed_at_the_end`, `_z80_frame_shares_one_file_with_lh5801_frame`, `_pc1600_trace_step_produces_wellformed_file`, `_pc1600_left_open_is_closed`; `debugger_tests.cpp`: `lib10_ac42_bytes_equal_the_recorded_reference` (§10.11), `lib10_ac23_button_trace_is_trace_bin`; CTest `CalcU1600ReadTrace` (new: the harness writes a short PC-1600 trace, `tools/read_trace.py` reads it; Frequent if under 2 s, else Longer) |
| D16-9 | The size limit | AC-43, AC-24 | `debugger_tests.cpp`: `lib10_ac43_preset_trace_ends_at_the_limit_with_a_notice` (version 1 through L-A3; again on format 2 in M-46), `lib10_ac43_button_trace_ends_at_the_limit_with_a_notice`; Frequent |
| D16-10 | Today's tests ported | AC-44 | `debug_target_tests.cpp:111-613` onto `machine::DebugTarget` and the run control (`lib10_ac44_z80_breakpoint_and_skip_once`, `_not_between_prefix_and_opcode`, `_skip_is_tied_to_its_address`, `_keeps_ei_shadow`, `_lh5801_skip_once`, `_breakpoint_set`, `_trace_flags_leave_breakpoints_alone`, `_reset_clears_breakpoint_state`, `_lh5801_watches_skip_fetches`, `_z80_watches`, `_pc1500_target`, `_pc1600_target`, `_pc1600_lh5803_thread`, `_pc1600_skip_only_for_the_running_cpu`, `_machine_step_latches_stops`, `_cpu_views`, `_register_reads`, `_watch_set`); `run_control_tests.cpp:152-235` (`lib10_ac44_step_over_a_call`, `_data_breakpoint_and_pause`); Frequent. The cases with source lines, conditions, hit counts, log messages and expressions (`run_control_tests.cpp:115-150, 171-211, 236-290`; `debug_target_tests.cpp:84, 637-732`) move to `tests/dap_layer_tests.cpp` as `c1600_3_ac23_…` (C1600-3 AC-23, D-15) |
| D16-11 | `step` with a test | (library, D-16) | `library/debugger/tests/run_control_tests.cpp`: `step_until_goes_on_while_the_test_says_so`, `step_until_ends_at_a_breakpoint`; `calcu_library_tests`, Frequent |
| D16-12 | The DAP on the new layer | (C1600-3 AC-20 at its old host) | `CalcU1600DapSmoke` (Longer): `tools/dap_smoke.py --app` against the Qt app, every scenario passing as at the pin |
| R-1 | Golden and reference runs, speed | §10.11 | `golden_runs_tests.cpp` (Frequent); `lib10_speed_pc1600_full_speed_with_a_breakpoint` (prints, for the qa-report; Longer) |

Coverage: AC-36 to AC-44 and AC-52, 10 of 10; AC-40's and AC-43's format
2 parts in M-46.

### 10.11 Golden and reference runs

- **Before the core is touched**, M-45 records one reference trace with
  today's file path: a PC-1600 of the golden run `pc1600-basic` traced for
  its first 200000 instructions, and a PC-1500A for 100000, into
  `tests/golden-runs/pc1600-basic.trace`, `pc1500a-basic.trace`. After the
  change, D16-8 compares the trace session's file with them byte for
  byte. A difference is a finding.
- **Golden runs** (M-8): with all checks off the cores run as today, so
  every `.golden` must replay unchanged.
- **Reference runs** of the 32 examples (M-4): compared at the idle
  machine in M-46.
- **Speed.** LIB-10 §8 asks for a PC-1600 at full speed with a breakpoint
  set against 0.8.1: R-1 measures it; the qa-report gives both numbers.

### 10.12 Agent knowledge (in M-45)

- `products/calc-u-1600/CLAUDE.md`: "the debugger reaches the machine
  through the adapter of `app/SharpMachineParts` until M-45" becomes "the
  debugger is the library's `DebuggerPart` (LIB-10); the DAP's layer is in
  `app/dap/`, its server in the Qt frontend until M-50"; the harness's
  "Not yet" list loses the debugger.
- `products/calc-u-1600/docs/developer/Debugger-Internals.md`: "Run
  control" (no `runSlice()`), "Where the code is" (`core/Debug/SharpDebugging`,
  `app/dap/`, `app/SharpTraceRecorder`; `DebugTarget`, `RunControl`,
  `BreakpointTable` gone), "Hooks elsewhere" (no `SharpDebuggerGate`),
  "Arming" (the library's rule G1).
- `products/calc-u-1600/docs/developer/Shared-Architecture.md` (the
  `SharpMachineParts` row; the adapter kind `c1600.trace` replaced by the
  product kind `c1600.trace-records`); `docs/Debugger.md` (the trace's
  size limit and its notice).
- `docs/glossary-mapping.md`, row "Pause": `paused` (`DebugController`) →
  the `run-state` topic of `calcu::debugger::RunControl`.
- `docs/test-groups.md`: the new and renamed cases, `CalcU1600ReadTrace`,
  `CalcU1600Shots.lib10-ac41`.
- Memory: `calc-u-1600/project_dap_debugger_plan.md` ("CPU-owned
  breakpoint enable", `CpuViews + MachineDebugTarget<Machine>`,
  `frontends/qt6/app/debug` as the place: now the library's run control,
  `SharpDebugging`, `app/dap/`) and its line in `calc-u-1600-memories.md`.
- No skill: Calc-U-1600 has none.

### 10.13 Ledger rows

- 1600-14, visible in M-45: the size limit for a preset's trace (version
  1 now, format 2 in M-46), the notice when it ends a trace, the run
  control in both frontends, Machine ▸ Resume.
- Proposed addition to 1600-14: "The limit counts MiB (today the
  INSPECTOR's limit counts 1'000'000 bytes per MB)."
- 1600-14's "Copied to clipboard" is LIB-8's (§10.9 there).

### 10.14 Risks

- **Speed of the observer path.** A step, an armed start and the profile
  run the machine one instruction at a time with a virtual call each.
  Today's line steps also go one machine step at a time
  (`RunControl::slice`, at most 20000 per frame), so a step is no slower;
  the profile is new. R-1 measures the breakpoint case.
- **The skip-once of two CPUs.** Today's skip lives in the CPUs and in
  `resumeFromStop`. If `passOnce` cannot be told per CPU through the
  latch, D16-4 shows it; the fallback is a step of the target over its
  breakpoint before the resume.
- **The trace buffer.** Records now wait in a buffer between pumps (at
  most 1/60 s of emulated time, G1). A PC-1600 at full speed writes some
  megabytes per second, so the buffer holds a few hundred kilobytes.
- **Qt DAP on new ground.** `DapSession` changes its calls in M-45 and
  moves in M-50. `CalcU1600DapSmoke` (Longer) runs in M-45; the open
  finding of `docs/test-groups.md` (no `stopped` after the first `stepIn`,
  timing-dependent) may show again: if it does, it is the old finding,
  checked against the pin.

### 10.15 Decided (best guess)

- **D-15 (the DAP's layer is built in `app/dap/` in M-45).** The DAP's
  breakpoints with their tests, its line steps and its view of threads
  and memory are needed the day `debug::RunControl` and
  `debug::BreakpointTable` go. They are written once, without Qt, in
  C1600-3's folder, with C1600-3's design (approved at the same
  boundary) and the tests of C1600-3 AC-23. M-50 then moves only the
  server, the requests and the queue. LIB-10 §5's `DapRunControlBridge`
  in the Qt frontend is not built; see the question below.
- **D-16 (a step with a test).** `RunControl::step` takes an optional
  `StepTest` (`std::function<bool(const machine::DebugTarget&)>`, true:
  go on). A line step is `step(cpu, a large count, thread, in|over, until
  the source line changes)`, so it runs in the gate's pieces and ends at a
  breakpoint, as today. A library change of a few lines with its own
  tests (D16-11).
- **D-17 (AC-38, the open point of §9).** `CpuOnlyRules`: the default
  rule is what FS 15 found; no Sharp rules.
- **D-18 (the trace records are a product kind).** `c1600.trace-records`
  stays a product kind, owned by the main CPU, as the INSPECTOR's is: the
  file's content is the product's (ADR 0037), and the app reaches the core
  only through the machine's capabilities (C1600-1).
- **D-19 (AC-40, "work in both").** Read as: in the harness of
  Calc-U-1600, and in the Qt6 and Web sessions; the Web has no harness,
  so its case sends the same session commands from the page.
- **D-20 (Banks scope, for C1600-3).** The SC7852's qualifier has one
  field `bank`; the DAP's Banks scope shows the bank of each of the four
  pages by asking `qualifierMatches` at each page's start, as today
  (`PC1600DebugTarget::bankState`).

### 10.16 Plan notes

- Plan M-45, "Does": add "the DAP's breakpoints and line steps in
  `app/dap/` (C1600-3, D-15)" and "Machine ▸ Resume in both frontends".
  "Tests from": add C1600-3 AC-23 (its ported cases, D16-10).
- Plan §5.1, LIB-10's Calc-U-1600 row: unchanged (M-45 / M-46). LIB-10
  §4 put the Web's Resume with LIB-14 1600 in M-49; L-A4 does it in M-45,
  so AC-41 passes in M-45 and "Done in M-46" holds.
- Plan M-45, "Knowledge, ledger": add the product's `CLAUDE.md`,
  `Shared-Architecture.md`, `docs/Debugger.md` and the memory file
  `project_dap_debugger_plan`.

### 10.17 Questions for the user

1. **Q-1 (D-15).** LIB-10 §5 planned a throwaway `DapRunControlBridge` in
   the Qt frontend for M-45 to M-50. This design builds the DAP's
   breakpoints and line steps once in `app/dap/` in M-45 instead (C1600-3
   code, a step early), so M-50 only moves the server. Agreed?
   Recommendation: yes.
   Answered (user, 2026-11-16, with the approval of the Part D designs): yes.

## 11. Part D simplification (P-5, 2026-11-16)

Departures from the text above (`docs/reviews/d-simplification.md` S-4
and S-8's debugger bullet). No spec change, no user-visible change.

- **The trace budget is the library's (S-4).** §3.7's `TraceRecorder`
  wrote into the `TraceFile` and kept the limit itself: `pump(file,
  budget)`, and on the limit an `end` that wrote only the end record.
  Calc-U-1600's recorder, the library tests' recorder and the Web test
  product's each wrote that rule, and Calc-U-59 (M-53) would have written
  it again. Now the recorder is a record source that appends to a byte
  buffer: `begin(out)`, `take(out, budget)`, `endBytes()`, `end(out)`
  (the records still waiting, then the end record), `buttonFileName()`,
  `machineGone()`. `TraceSession` writes the bytes, computes T2's budget
  and, when the limit ended the trace, keeps only the last `endBytes()`
  bytes of what `end` gives. The files are byte for byte as before
  (AC-42's references, AC-43's limit). Calc-U-59's recorder (M-53) is a
  source over `drainCpuFrames` with no budget rule of its own.
- **One step-count limit (S-8).** `kMaxStepCount` (`RunState.hpp`,
  100000, AC-19) is the one constant. The session command `step` refuses a
  count outside 1 to 100000 with "step takes a count from 1 to 100000."
  (it refused only a count below 1, with other words, which no frontend
  sends: the buttons and the DAP step by 1); the harness no longer checks
  the range itself, only that the count is a number, in the same words.
  The preset step `step:` keeps its check when the preset is read (LIB-6
  AC-6) on the same constant.
- **Peek and poke (S-8)** check the bank and vertical bank (AC-49) in one
  helper of `DebuggerPart.cpp`; the refusals are unchanged.
- **The trace folder and limit (S-8)** live only in the `TraceSession`:
  `DebuggerPart` hands `DebuggerSetup::trace` over at construction, and
  `setTraceFolder`/`setTraceLimit` only forward to it.
- **Kept: `DebuggerSetup::runControlSteps` stays per model.** AC-19 makes
  it a product property in the products (Calc-U-1600: none of its models;
  Calc-U-59: every model), but the library's tests use one product with
  both kinds of model (`test-dot` stands for a model without the steps,
  `test-seg` has them; `DebugRig`, the harness's `TestProduct`), so a
  per-product flag would need a second test product.

## 12. Part E: the Calc-U-59 column

Status of this section: approved by the user at the Part D/E boundary (2026-11-16).
§1 to §11 above are approved and are not changed. This section replaces
§4's Calc-U-59 outline and §5's Calc-U-59 rows. Where it departs from them,
it says so. It also settles D-12. Paths are under `products/calc-u-59/`
unless named, at `7bdd51c`.

### 12.1 Scope

| Column | Step | Tests from (plan §3) | Done in (plan §5.1) |
|---|---|---|---|
| Calc-U-59 | M-53 (with LIB-8 59) | AC-28–35 | M-53 (D-12 settled: AC-34's Swift part passes in M-53, D-27) |

- M-53 waits for M-40 (the library's debugger, in particular §11's
  trace-record source), M-34 (the product on the library's sessions in
  every frontend), M-36 (LIB-5: the typing part's pause) and M-25. It
  builds on `main` after the Part D simplification.
- AC-32 names the format 2 steps `pause:`, `pause-on-start:`, `step:`,
  `resume:` and `debug-panel:`, which Calc-U-59 reads from M-54 (LIB-6 59).
  In M-53 the 2.3.0 KEYSTROKES directives run through the session's
  run-control commands by an adapter (L-E1), so AC-32 is tested on them in
  M-53 and again, as written, on format 2 in M-54. The column stays Done
  in M-53: the mapping AC-32 states passes there. Plan §5.1's "M-53 / M-53"
  holds.
- The edge cases EC-1 to EC-9 are the Library column's (M-40).
- Not here: the debug panel's data, its tabs, the heatmap, the dumps, the
  log levels and the ASM overlay (C59-4, M-56,
  `features/C59-4-calc-u-59-debug-panel/design.md`). M-53 changes only how
  FREEZE, STEP, RESUME, FREEZE ON START and TRACE reach the machine. It
  does not change what the tabs show.

### 12.2 What the code is today (`7bdd51c`)

- **The core's debugger calls.** `TIDebugTargetAdapter`
  (`core/TIAdapters.hpp:86-120`) offers single steps, the step to the next
  program byte (`stepUntilNextKeycode`, 50000 weighted cycles,
  `core/TI59Machine.hpp:120-126`, `.cpp:216-225`), `beginNextStep`,
  `snapshotCPU`, `lastStepBoundary`, trace flags and the frame ring
  (`drainCpuFrames`, `readCpuFrames`), core breakpoints and the run-control
  hook `TIStepObserver` (`:34-41`). `TIMachine::advance`
  (`core/TIMachine.cpp:36-52`) asks the hook after every instruction with
  the next PC, whether that is a step boundary, and whether it is in the
  scan loop. The frame is `CpuFrame` (`core/TraceTypes.hpp:24-46`). The
  debug target's ID is `TMC0501` (`core/TICatalogue.cpp:92`).
- **The freeze.**
  - `FreezeController` (`app/FreezeController.hpp:7-52`) holds the armed
    FREEZE ON START of both tabs and the rule "left the scan loop after it
    was seen, or jumped to 0000".
  - `EmulatorSession` holds the freeze state, the halt flag and the queued
    steps (`app/EmulatorSession.hpp:47-66, 168-226`;
    `app/EmulatorSession.cpp:118-158` (`runQueuedSteps`, `advanceHosted`),
    `:169-265` (freeze, resume, arm, step)).
  - The machine session's gate `TIFreezeGate` and the typing part's pause
    probe `TIFreezeProbe` are `app/TIAppPartInternal.hpp:13-49`.
  - `TIAppPart`'s debugger flows are `app/TIAppPart.hpp:379-399`, its
    commands `:227-247` (`c59.freeze` … `c59.step-instruction`,
    `c59.manual-trace`), and its interim topics `c59.run-state`
    (`TIRunState`, `:131-137`) and `c59.trace` (`TITraceState`,
    `:138-142`).
- **Traces.** There are two `TraceWriter`s per machine, the manual one and
  the scripted one (`app/TraceWriter.hpp:18-60`;
  `app/EmulatorSession.cpp:273-350`). Each writes the version 2 format with
  the header `TI59` (`app/TraceWriter.cpp:20-22, 72`), and the manual one
  also writes the key and card records (`:18, 154-157`;
  `TIKeyTraceObserver`, `app/EmulatorSession.hpp:81-90`). The file names
  are `CALCU59_TRACE.bin`, `CALCU58_TRACE.bin` and `CALCU58C_TRACE.bin`
  (`app/TraceWriter.cpp:44-50`). The trace place is `TraceTarget`
  (`app/TIAppPart.hpp:63-67`): Qt6 `AppPaths.cpp:88`, Swift
  `AppSettings.swift:141-175`, and the Web's MEMFS `/traces`
  (`frontends/web/bindings.cpp:56, 63, 66`, LIB-9 A-3) with
  `c59.web.trace-ready`, `c59.web.traces` and `c59.web.trace-bytes`
  (`app/TIWebPart.cpp:244-249, 352-359, 406-415`).
- **KEYSTROKES** `Freeze:`, `FreezeOnStart:`, `Step:`, `Resume:` and
  `DebugPanel:` go through `TIAppPart::applyControl`
  (`app/TIAppPartFlows.cpp:656-692`), and `Trace:` goes through
  `EmulatorSession::scriptedTrace` (`app/TIKeystrokePlayback.cpp:87`,
  `app/EmulatorSession.cpp:336-350`).
- **Readers of the adapter besides the debugger**: the panel's snapshots
  (`app/DebugSnapshots.cpp:96-161`), the log's dumps
  (`app/DebugLog.cpp:69`), the overlay's entry (`app/AsmOverlay.cpp:53-57`),
  the headless tool (`tools/headless/capi.cpp:65, 275`), and the tests'
  helper (`tests/TestRoms.hpp:57-59`).
- **Frontends.**
  - Qt6: the buttons of `frontends/qt6/app/debug/LiveDebugWidget.cpp:69-82`
    and `CpuInspectorWidget.cpp:76-87`, `HoldRepeatButton.hpp:7-22`, the
    TRACE button `LogWidget.cpp:49-51, 72`, and the topics read in
    `EmulatorController.cpp:280-291, 312-316`.
  - Web: `frontends/web/src/Debugger.jsx:25-61` (`RepeatButton`,
    `FreezeHeader`), `:260-275` (TRACE, the list of ended traces), and
    `engine.js:50-72, 122-124, 152-155, 183-196`.
  - Swift: `App/Views/LiveDebugView.swift:55-91`,
    `CPUInspectorView.swift:307-338`, `HoldRepeatButton.swift:6-53`,
    `DebugView.swift:143-161`; `EmulatorViewModel.swift:93-129, 358-390`;
    and the bridge `TIAppSession.h:241-284`, `TIAppSession.mm:597-662`.
- **The harness** gives Calc-U-59 no debugger
  (`app/HarnessProduct.cpp:66-75`), so `pause`, `break` and the rest
  refuse with "The machine offers no debug target." (§4, M-40).

### 12.3 Modules and files (M-53)

| File | Change |
|---|---|
| `core/TIDebugging.hpp/.cpp` (new) | `TIDebugging : calcu::machine::Debugging` with one `TIDebugTarget : calcu::machine::DebugTarget`, `TMC0501` (§12.4). It owns the run-control hook that `TIDebugTargetAdapter::setRunControl` set, and turns `TIMachine::advance`'s per-instruction call into an `InstructionEvent` with the marks of §12.4 |
| `core/TIMachine.hpp/.cpp` | `debugging()` returns the machine's `TIDebugging`. `advance` asks the library's observer (`InstructionObserver`) instead of `TIStepObserver`, and arms the core's breakpoints from the `Checks` (as the Sharp machines do, design §10.5) |
| `core/TIAdapters.hpp/.cpp` | `TIDebugTargetAdapter`, `TIStepObserver` and `TIStop` go. `TIDebugPanelAdapter` gains, until M-56, the reads only the panel needs (L-E3): `snapshotCPU`, `lastStepBoundary`, `readCpuFrames`, `readROMKeycode` |
| `core/TraceRecords.hpp` (new; product kind `c59.trace-records`, owner TMC0501) | `begin()`, `take(std::vector<CpuFrame>&, max, lost&)`, `end()`: the frame ring for the trace recorder, as `drainCpuFrames` with the trace flags set while a trace runs (D-24) |
| `app/TILevelRules.hpp/.cpp` (new) | `cu59::TILevelRules : calcu::debugger::LevelRules` (AC-29, AC-30; §12.5) |
| `app/TITraceRecorder.hpp/.cpp` (new) | `cu59::TITraceRecorder : calcu::debugger::TraceRecorder` on `c59.trace-records`. It writes today's `TraceWriter` records into the session's buffer, without the key and card records (AC-33). `buttonFileName()` is `TraceWriter::baseFileName(model)` |
| `app/TraceWriter.hpp/.cpp` | Becomes the encoder of records into a byte buffer (header, session start and end, an instruction, a gap). The `FILE*`, the size limit, `writeKeyDown`, `writeKeyUp`, `writeCardInsert` and `writeCardEject` go: the library's `TraceSession` writes the file and keeps the limit (§11) |
| `app/TIDebuggerSetup.hpp/.cpp` (new) | `cu59::tiDebuggerSetup(…)`: the `TILevelRules`; the `TraceSetup` with `TITraceRecorder`, the trace folder from `TraceTarget`, `[trace] max-file-size-mb`, and `handOver` true on the Web (AC-26, LIB-10 D-10); `runControlSteps` true for the three models; `panelViews` `calculator`, `cpu`, `log`; `showPanel` sends the interim `c59.show-debug-panel` (L-E4); no symbol readers (D-5: the TI has none) |
| `app/FreezeController.hpp/.cpp` | Deleted (its rule is `TILevelRules`'s, §12.5) |
| `app/EmulatorSession.hpp/.cpp` | These go: the halt flag, `m_steps` and `runQueuedSteps`, `advanceHosted`, `DebugState`, `FrozenViewReason`, freeze, resume, arm, disarm, `stepKeycode`, `stepInstruction`, the two `TraceWriter`s, `drainTrace`, `setTraceLocation`, `scriptedTrace`, `TIKeyTraceObserver` and `TraceConsumers`. What stays: the playback and `Events`/`later()`, until their owners (`HANDOVER.md`); the trace flags for the CPU tab (`setCpuPanelVisible`) |
| `app/TIAppPart.hpp/.cpp`, `TIAppPartFlows.cpp`, `TIAppPartInternal.hpp` | `TIFreezeGate` (the `advance` of `Hooks`) and `FreezeProbe` go; the `DebuggerPart` is the gate. These commands go: `c59.freeze`, `resume`, `arm-pc-change`, `arm-scan-loop-exit`, `disarm-*`, `step-keycode`, `step-instruction`, `manual-trace`. `c59.trace` goes. `TIRunState` keeps `fullSpeed` and `keystrokesPlaying` only (L-E5). KEYSTROKES `Freeze:`, `FreezeOnStart:`, `Step:`, `Resume:` and `Trace:` become the library's commands (L-E1). The two resets and the model switch tell `tellRestarted` (L-E2). The panel's snapshots are rebuilt when `run-state` changes, instead of on `frozenViewReady` |
| `app/TIAppPart.cpp` `makeAppSession` | The session owns a `calcu::debugger::DebuggerPart` on `tiDebuggerSetup(…)`, after the `PaperPart` (LIB-8 D-23) |
| `app/HarnessProduct.cpp:66-75` | `s.debugger = tiDebuggerSetup(…)`, so `targets`, `registers`, `peek`, `break`, `pause`, `step`, `trace`, `history`, `disassemble` and `profile` work for Calc-U-59 (AC-31) |
| `app/TIWebPart.cpp:244-249, 352-359, 406-415`, `frontends/web/bindings.cpp:56, 63, 66` | `c59.web.trace-ready`, `c59.web.trace-bytes` and the MEMFS `/traces` go (LIB-9 A-3). The library hands an ended trace over with `KeepFile` (AC-26), which `hostServices.js` downloads. The page keeps the list of ended traces itself (`c59.web.traces` goes) |
| `frontends/qt6/app/debug/LiveDebugWidget.cpp:69-82`, `CpuInspectorWidget.cpp:76-87`, `HoldRepeatButton.*` | FREEZE, STEP and RESUME are a `calcu::qt6::RunControlButtons` per tab, for `program` and `cpu` (AC-34). The labels FREEZE, STEP and RESUME are set by the product (AC-18: "their labels … are the product's"). FREEZE ON START and ARMED send `pause-on-start {level}` and `pause-on-start {off}`. `HoldRepeatButton` goes |
| `frontends/qt6/app/debug/LogWidget.cpp:49-51, 72` | TRACE sends `trace {button: true}` and `trace {off: true}` and shows the `trace` topic |
| `frontends/qt6/app/EmulatorController.*` | The debugger methods of `:87-134` become `run-state` and `trace` relays and the commands above |
| `frontends/web/src/Debugger.jsx:25-61, 260-275`, `engine.js` | `CalcuRunControlButtons` per tab. FREEZE ON START and ARMED, TRACE and the trace list are kept from the `KeepFile` downloads |
| `frontends/swift/Bridge/TIAppSession.h/.mm:597-662`, `App/EmulatorViewModel.swift:93-129, 358-390`, `App/Views/LiveDebugView.swift:55-91`, `CPUInspectorView.swift:307-338`, `DebugView.swift:143-161` | The bridge mirrors `run-state` and `trace`, sends `pause`, `step`, `resume`, `pause-on-start`, `trace`, and wraps `calcu::debugger::ButtonRule` (D-27). The Swift buttons follow the rule; `HoldRepeatButton` repeats at 250 ms |
| `tools/headless/capi.cpp:65, 275` | Steps and disassembles through `TIDebugging` and reads frames through `TIDebugPanelAdapter` (L-E3), until M-59 |
| `tests/…` | §12.7 |

### 12.4 The TI debug target (AC-28)

| | TMC0501 (TI-59, TI-58, TI-58C) |
|---|---|
| Name | `TMC0501` (`core/TICatalogue.cpp:92`) |
| Registers | the fields of today's processor frame (`CpuFrame`, `core/TraceTypes.hpp:24-46`): `A`, `B`, `C`, `D`, `E` (16 digits each, 64 bits), `KR`, `SR`, `fA`, `fB`, `flags` (16 bits), `R5`, `digit` (8 bits), `EXT`, `PREG`, `pc` (16 bits) as they are there; read only (D-21) |
| Spaces | `rom` (13-bit words, 0000 to 1FFF with the overlay's range), `ram` (16-digit registers, as many as the model has, `TIMemoryAdapter::ramRegCount`), `scom` (16 digits × 16), `program` (8-bit steps, the model's program steps, C59-2 AC-3) |
| Code space | `rom` |
| Writable | the cells of `ram` and `program`, through the core's writes of `TIMemoryAdapter` (`core/TIAdapters.hpp:63-84`); `rom`, `scom` and the registers refuse a write |
| Offers | a history (the core's frame ring, the newest first, as `readCpuFrames`), decoding (`TMC0501::disassemble`); no watches, no step over or out, no qualifier |

- **Marks** of `InstructionEvent`: bit 0, the next instruction is a
  program step boundary (`TI59Machine::kStepBoundary`); bit 1, it is in the
  keyboard scan loop; bits 8 to 15, the instruction's weight (1 active, 4
  IDLE, as `stepUntilNextKeycode` counts). The library passes them on
  unread (§3.2).
- **Between two instructions (AC-3).** A pause of `program` lands at a
  boundary; `TIDebugging` then does what `beginNextStep()` did after a step
  today, so that the next instruction and the registers read are those of
  the next program step.

### 12.5 Levels (AC-29, AC-30)

`TILevelRules` (`hasProgramLevel` true):
- **Pause, both levels** (`pauseLandsAtOnce` false): it lands at the next
  instruction whose mark says "boundary", or when the weights counted
  since `pauseAsked` reach 50000. Inside an ASM overlay no boundary comes,
  so the count decides, as `core/TI59Machine.cpp:216-225` does today.
- **One `program` step** (`programStepDoneAfter`): the same rule, a
  boundary or 50000. One `cpu` step: one instruction; `cpuStepRepeats()` is
  32 (AC-29).
- **Pause-on-start `program`**: it fires at the next boundary.
  **Pause-on-start `cpu`**: it fires on the first instruction after which
  the scan-loop mark is clear, once the mark was seen set since arming, or
  when the next address is 0000 while it was not 0000 at arming. This is
  `FreezeController::checkStep` (`app/FreezeController.cpp:29-37`) as a rule.
- **At a restart** (`startFiresAtRestart`): false. The ROM's start-up
  after a Reset or a Cold boot runs in the advances that follow (C59-1
  AC-6), so a `cpu` start fires there by the 0000 rule on every model.
  This is ledger row 59-34. AC-30's sentence that "a Reset of the TI-59 or
  TI-58 does not fire `cpu`" reads the other way: Q-2 (§12.13).

### 12.6 Traces (AC-33)

- **One trace session.** The library's `TraceSession` starts, limits and
  ends every trace: the TRACE button (`trace {button: true}`, the file of
  `buttonFileName()`), the harness command, and KEYSTROKES `Trace:` through
  L-E1 (`ofOperation` true). It runs one trace at a time. A new machine
  ends a trace with a log line, and the limit ends it with the library's
  notice (ledger 59-19).
- **The records.** `TITraceRecorder::begin` gives today's 16-byte header
  (`TI59`, version 2, the model) and the session start, and switches the
  core's trace flags on through `c59.trace-records`. `take` turns the
  ring's frames into instruction records and a lost count into a gap
  record. `end` gives what waits, then the session end. Key and card
  records are never written (AC-33). `tools/read_trace.py` reads the files
  unchanged.
- **The limit** is `[trace] max-file-size-mb` MiB for every trace (LIB-10
  T2). Today the default is 50'000'000 bytes
  (`app/EmulatorSession.hpp`, `m_traceMaxBytes`); the setting's own value
  is LIB-9's.
- **Where.** Qt6, Swift and the harness use the trace folder. The Web has
  no trace folder: `handOver` keeps the trace in memory and hands it over
  with `KeepFile` when it ends (AC-26). A Swift trace location that cannot
  be written fails the trace with the reason (EC-8). The TRACE button's
  "cannot be used" is C59-4 AC-12's (M-56).

### 12.7 Adapters

Removed in M-53 (§5's Calc-U-59 rows):

| Adapter | Where |
|---|---|
| `TIFreezeGate`, `TIFreezeProbe` | `app/TIAppPartInternal.hpp:25-49`, `EmulatorSession::advanceHosted` |
| `TIDebugTargetAdapter` | `core/TIAdapters.*`; its users `tools/headless/capi.cpp`, `tests/TestRoms.hpp:57` |
| interim topics `c59.trace`; `c59.run-state`'s debugger fields | `TIAppPart` |
| LIB-9 A-3 (the Web's trace place) | `frontends/web/bindings.cpp:56, 63, 66` |

`typing::PauseProbe` (LIB-5 §5) was Calc-U-59's only probe
(`docs/reviews/d-simplification.md` §9). With the TI freeze gone, the
typing part asks the machine session's pause, which the run control sets
(LIB-3 `setPaused`). If no other user of `PauseProbe` is left, M-53 removes
it from `library/typing/`.

New in M-53, each with the step that removes it:

| Adapter | Where | Wraps | Removed in |
|---|---|---|---|
| L-E1 KEYSTROKES directives | `TIAppPart::applyControl`, `EmulatorSession`'s playback (`Trace:`) | `Freeze:`, `FreezeOnStart:`, `Step:`, `Resume:` and `Trace:` sent as the session's `pause`, `pause-on-start`, `step`, `resume` and `trace` (`ofOperation`) | M-54 (LIB-6 59: the steps of §3.8) |
| L-E2 restart call | `TIAppPart::resetMachine`, `cleanResetMachine`, `startMachine` | `tellRestarted(Reset/ColdBoot)`, and the build for a model switch (§3.11) | M-54 (LIB-18 59) |
| L-E3 panel reads | `TIDebugPanelAdapter` (`snapshotCPU`, `lastStepBoundary`, `readCpuFrames`, `readROMKeycode`) | what the CALCULATOR and CPU tabs, the dumps and the overlay's entry read that the generic target does not give | M-56 (C59-4) |
| L-E4 `debug-panel:` view | `c59.show-debug-panel` sent by `showPanel` | the library's frontend step to today's stream | M-56 (C59-4: the product topic `debug-view`) |
| L-E5 slim `c59.run-state` | `TIRunState` (`fullSpeed`, `keystrokesPlaying`) | the two fields that are no debugger state | M-54 (playback) and M-57 (full speed in the control state) |

### 12.8 Tasks (AC → task → test)

Unless named otherwise, cases in `products/calc-u-59/tests/` are in
`CalcU59Tests` (Frequent). Session cases use `tests/SessionRig.hpp` and
the real ROMs (`SKIP_UNLESS`). The tests of each task come before its
code. Names are `lib10_acN_…`; ported cases keep today's names after the
prefix.

| Task | What | AC | Test (file: cases; group) |
|---|---|---|---|
| E10-1 | `TIDebugging`, the target | AC-28 | `debug_target_tests.cpp` (new): `lib10_ac28_one_target_tmc0501_on_every_model`, `_registers_of_the_processor_frame`, `_spaces_rom_ram_scom_program`, `_ram_and_program_written_rom_scom_refused`, `_history_and_decoding_no_watches_no_over_out`, `_conformance_on_every_model` (`checkDebugging` on the TI-59, TI-58 and TI-58C) |
| E10-2 | Levels | AC-29 | `debugger_tests.cpp` (new): `lib10_ac29_pause_lands_at_the_next_boundary`, `_waiting_for_a_key_lands_after_50000`, `_in_an_overlay_lands_after_50000`, `_program_step_is_one_program_byte_as_sst`, `_program_step_through_rom_and_module_programs`, `_cpu_step_repeats_a_hold_up_to_32` |
| E10-3 | Pause-on-start | AC-30 | `debugger_tests.cpp`: `lib10_ac30_program_fires_at_the_next_boundary`, `_cpu_fires_leaving_key_readiness`, `_cpu_fires_at_0000_after_reset_and_cold_boot` (row 59-34; Q-2), `_arming_one_disarms_the_other`; ported `shared_runtime_tests.cpp:80` `freeze_on_scan_loop_exit_fires_on_key`, `:122` `freeze_on_pc_change_and_mutual_exclusion`; `part_flows_tests.cpp:144` `part_cpu_freeze_on_start_sees_reset_and_clean_reset` |
| E10-4 | Breakpoints in the app | AC-31 | `harness_tests.cpp`: `lib10_ac31_harness_breakpoint_pauses_the_ti`; session: `lib10_ac31_app_session_pauses_at_a_breakpoint` (a breakpoint set by the setter `harness` in a `PartRig`) |
| E10-5 | The directives | AC-32 | session: `lib10_ac32_freeze_calculator_is_pause_program`, `_freeze_cpu_is_pause_cpu`, `_freeze_on_start_is_pause_on_start`, `_step_and_resume`, `_debug_panel_calculator_cpu_log` (through L-E1 on 2.3.0 presets; on format 2 in M-54); reference run `screentest_test_cdebug.ti59` (`reference_runs_tests.cpp:152`) unchanged |
| E10-6 | Traces | AC-33 | `trace_tests.cpp` (new; ports `shared_trace_tests.cpp:18, 70`): `lib10_ac33_header_ti59_version_2_and_model`, `_records_session_instruction_gap`, `_no_key_or_card_records`, `_button_names_by_model`, `_limit_ends_with_a_notice`, `_bytes_equal_the_recorded_reference` (§12.9); CTest `CalcU59ReadTrace` (new: the harness writes a short TI-59 trace and `tools/read_trace.py` reads it; Frequent if under 2 s, else Longer) |
| E10-7 | The buttons | AC-34 | Qt6: `frontends/qt6/tests/debug_buttons_tests.cpp` (`CalcU59QtTests`, offscreen): `lib10_ac34_qt6_calculator_and_cpu_buttons_follow_run_state`. Web: `play-calculator.spec.js` `LIB-10 AC-34 FREEZE STEP RESUME of both tabs follow the run state` (Longer). Swift: `Calc-U-59Tests/RunControlTests.swift` (new) `testLib10AC34ButtonsFollowTheRule` (the bridge's rule over the rows of `library/debugger/tests/button_cases.json`) |
| E10-8 | Today's tests | AC-35 | ported onto the run control: `shared_runtime_tests.cpp:295` `session_queued_steps_run_after_the_host_call`, `:330` `session_freeze_fires_through_listener`, `:353` `session_steps_only_when_frozen_and_arms_exclude`, `:390` `c59_1_runbatch_stops_at_freeze_inside_advance`; `shared_debug_tests.cpp:78` `step_shows_the_next_step`, `:103` `running_view_shows_only_step_boundaries`; `part_flows_tests.cpp:106` `part_model_switch_keeps_printer_and_log_resets_freeze`, `:167` `part_resume_before_the_frozen_view_drops_it`; each as `lib10_ac35_<today's name>`. `shared_runtime_tests.cpp:379` `trace_flags_for_consumers` goes with `TraceConsumers` |
| E10-9 | The Web hand-over | AC-26 (product), AC-33 | Web: `play-calculator.spec.js:471` renamed `LIB-10 AC-26 a trace is downloaded when switched off` (still `CALCU59_TRACE.bin`) |
| R-1 | Golden runs, reference runs, speed | §12.9 | `golden_runs_tests.cpp`, `reference_runs_tests.cpp` (Frequent, Longer) |

Coverage: AC-28 to AC-35, 8 of 8, all in M-53. AC-32 is tested again on
format 2 in M-54.

### 12.9 Golden and reference runs

- **Before the core is touched**, M-53 records one reference trace on
  today's path: a TI-59 running `tests/golden-runs/ti59-calculation.run`
  with the manual trace on, as `tests/golden-runs/ti59-calculation.trace`.
  E10-6 compares the trace session's file with it byte for byte, after the
  reference's key and card records are dropped (AC-33). A difference is a
  finding.
- **Golden runs.** While no observer is set, `TIMachine::advance` takes
  today's path, so every `.golden` must replay unchanged. The `pc=` of the
  driver comes from `TIMachineTesting::pc`.
- **Reference runs** (57). The debug examples (`debug_*`,
  `screentest_test_cdebug`) freeze and step through L-E1. Their `.txt`
  files must match with LIB-5's accepted differences (M-36 list)
  unchanged.
- **Speed.** Today the observer is called per instruction whenever
  `setRunControl` is set. After M-53 it is set only while a pause waits, a
  step runs, a start is armed, or the profile is on. No measurement is
  needed beyond the Frequent and Longer groups' times.

### 12.10 Tools and agent knowledge (in M-53)

- `reference/DebugAPI.md:12, 83, 99, 410, 417-493` (`FreezeController`,
  `FreezeOwner`, `TraceWriter`, the key and card records go from the format
  description: version 2 stays, the records are never written);
  `reference/SharedArchitecture.md:44, 57, 92, 146, 545`;
  `reference/CoreArchitecture.md:556` (trace infrastructure);
  `reference/FrontendParity.md:68, 70, 75`; `reference/NewGUIGuide.md:43`
  (`c59.run-state`).
- Skills: `calc-u-59-core` (trace infrastructure, `TIDebugging`),
  `calc-u-59-shared` (freeze state, trace writer), `calc-u-59-uitesting`
  (traces, plan M-53), `calc-u-59-qt`, `calc-u-59-web` and
  `calc-u-59-swift` (the buttons).
- `docs/glossary-mapping.md:74`, row "Pause": `freeze`,
  `FreezeController` and `FreezeOwner` go; FREEZE and RESUME stay as the
  panel's labels (C59-4).
- `docs/test-groups.md`: the new and renamed cases, `CalcU59ReadTrace`,
  `CalcU59QtTests` if new.
- Memory: the Calc-U-59 memory files that name `FreezeController` or
  `TraceWriter` (none in `products/calc-u-59/memory/`, checked 2026-11-16).
- Tools: `tools/headless/capi.cpp` (§12.3, L-E3); `tools/read_trace.py`
  unchanged.

### 12.11 Risks

- **`beginNextStep` (§12.4).** Today it is called after every step and
  freeze, and the CALCULATOR tab relies on it. If doing it in
  `TIDebugging` at a landed `program` pause is not enough for the `cpu`
  level, the run control's pause tells the target through a product hook
  of `TILevelRules`. E10-8's `step_shows_the_next_step` shows it.
- **Deferred replies.** A STEP that is held repeats four times a second.
  Each step of `program` may run up to 50000 weighted cycles inside one
  host call, as today.
- **The ring shared by the trace, the CPU tab and the heatmap.** Today one
  ring cursor serves the trace, and the panel reads without draining. If
  the trace session's `take` and the panel's `readCpuFrames` disagree on
  the cursor, the trace takes priority, and the CPU tab reads what is left
  (as today).
- **Swift on new commands.** The bridge's methods change names. The
  XCUITests that press FREEZE, STEP and RESUME
  (`Calc-U-59UITests/ScreenshotTests.swift:150-160, 229-271`) run in Longer
  at the end of the part.

### 12.12 Decided (best guess)

- **D-21 (AC-28, registers).** The registers are read only, because no
  frontend and no tool writes them today. `ram` and `program` take writes
  through the core's existing writes. `rom` and `scom` refuse them.
- **D-22 (marks).** The weight travels in the marks, so `TILevelRules`
  counts "50000 instructions as the ROM's idle state counts them" without
  calling the machine.
- **D-23 (the freeze is the run control).** FREEZE (CALCULATOR) is
  `pause program`, FREEZE (CPU) is `pause cpu`, and "the owner" is the run
  state's level (AC-45). `EmulatorSession` keeps no freeze state.
- **D-24 (trace records as a product kind).** `c59.trace-records` gives
  the frames, and the app encodes them. The file's content is the
  product's (ADR 0037), and the app reaches the core only through the
  machine's capabilities (C59-1), as Calc-U-1600's D-18.
- **D-25 (the Web's list of traces).** The page keeps the names of the
  traces `KeepFile` handed over, for C59-4 AC-12's list. The worker keeps
  no copy.
- **D-26 (the panel's views).** `debug-panel:` takes `calculator`, `cpu`
  and `log` on every model (AC-32). Passing over `calculator` while an
  overlay runs is the product's (C59-4 AC-2), in `showPanel`.
- **D-27 (AC-34 in Swift in M-53; settles D-12).** The Swift buttons
  follow `ButtonRule` through the bridge, so they need no C59-8 panel. The
  bridge must change in M-53 anyway, because the commands it sends go.
  LIB-10 59 is Done in M-53.
- **D-28 (L-E5).** `c59.run-state` keeps its two fields that are not
  debugger state until their owners, instead of a new topic for one part.
- **D-29 (AC-31).** No Calc-U-59 frontend sets a breakpoint. The harness
  sets them with the setter `harness`, and an app session pauses at them,
  which E10-4 shows in a session.

### 12.13 Ledger rows, plan notes, questions

- **Ledger.** 59-19 becomes visible in M-53 (one trace at a time; no key
  and card records; a preset's trace fails its preset when it cannot
  start; the limit's notice for every trace). No new row.
- **Plan notes.**
  - Plan M-53, "Completes": "LIB-10 59" without "or with M-58"
    (D-27); plan §5.1, LIB-10's Calc-U-59 row: "M-53 / M-53".
  - Plan M-53, "Waits for": add M-36 (the typing part's pause).
  - Plan M-53, "Knowledge, ledger": add `reference/SharedArchitecture.md`,
    `CoreArchitecture.md`, `FrontendParity.md`, `NewGUIGuide.md` and the
    skills `calc-u-59-core`, `-shared`, `-qt`, `-web`, `-swift`.
- **Q-2 (AC-30 against ledger row 59-34).** AC-30 says a Reset of the
  TI-59 or TI-58 does not fire pause-on-start `cpu`. Since C59-1 (row
  59-34) the ROM's start-up after a Reset runs in the run loop, and an
  armed CPU FREEZE ON START fires there, as on a Cold boot. This design
  keeps today's behaviour, row 59-34: it fires on a Reset on every model.
  Recommendation: keep it, with a decision-log row in LIB-10's spec that
  AC-30's Reset clause is replaced by row 59-34.
  Answered (user, 2026-11-16, with the approval of the Part E designs): kept; the
  decision-log row is in the spec.

## Part E simplification E-1 (2026-11-17, branch `es-assembly`)

`docs/reviews/e-simplification.md` §1 S-1.

- `parts::LibraryParts` sets the trace session's folder and limit of every
  product: `debugger::traceFolderOf(paths, available)` on the host's places
  and `[trace] max-file-size-mb` of the product's settings, read at each
  trace as Calc-U-1600 read them. The products' debugger setups give the
  recorder, the rules and `handOver` only (Calc-U-1600:
  `sharpDebuggerSetup()` without arguments; Calc-U-59: `TIDebuggerOptions`
  without folder and limit, `handOver` always true, which takes effect
  only where the places have no trace folder, the Web).
- `traceFolderOf` takes an optional `available` (Calc-U-59's Swift
  frontend: the iCloud trace location, C59-4 AC-12): while it says no, the
  folder is reported with "The trace location is not available." (the
  sentence Calc-U-59's own lambda gave).
- `DebuggerPart::setTraceFolder`/`setTraceLimit` and
  `TraceSession::setFolder`/`setLimit` are gone (the harness's late patch,
  LIB-11 this step). `DebuggerPart::runControl()` has a const overload.
- Calc-U-59's `TraceTarget` (a folder and a byte count, divided back into
  MiB) and its two folder lambdas are gone. Its trace folder now follows
  the places at each trace, as Calc-U-1600's always did, not the folder of
  the start; its limit reads the setting at each trace (a negative value
  now gives 50 as AC-11 says).
