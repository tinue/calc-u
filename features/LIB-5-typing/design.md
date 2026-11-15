# LIB-5 Typing: design
Status: draft for the Part C1/C2 boundary review (2026-11-15)

## 1. Scope

All three columns are worked in one step, M-36.

| Column | Step | Tests from (plan §3) | Done in (proposed; plan §5.1 says M-36) |
|---|---|---|---|
| Library | M-36 | AC-1–19, AC-38, AC-39; EC-1–6 | M-38 (AC-15's "found when the preset is read" is LIB-6 L's loader) |
| Calc-U-59 | M-36 | AC-20–27 | M-54 (AC-26 "in a preset", AC-17 "in a preset": LIB-6 59) |
| Calc-U-1600 | M-36 | AC-28–31, AC-33–37, AC-40, AC-41 (AC-32 withdrawn) | M-46 (AC-14 and AC-41 in presets, AC-28's "in a preset": LIB-6 1600) |

Every criterion becomes a test in M-36, as far as it can be tested
without a preset of format 2: through the library's commands, the
harness commands `key` and `type`, and the products' frontends. The
preset parts of AC-14, AC-15, AC-17, AC-26, AC-28 and AC-41 are tested
again by LIB-6 in its steps (M-38, M-46, M-54), against the engine's
interface of §3.5. Plan rule "Completes and Done" (§1) puts the Done
there; §9 D-1 proposes the plan change.

The steps this design uses:
- M-3 (FS 5): TI key readiness tells idle from running; a tap of 100 ms
  of emulated time does not always stop a printing loop, 200 ms always
  does (`docs/walkthroughs/first-steps.md` §5). This is the fallback of
  Technical Requirements (§3.4 "sampled").
- M-4 (FS 13): the Sharp reference runs (`products/calc-u-1600/tests/reference/v0.8.1-examples/`).
- M-5 (FS 15): the PC-1500's key readiness, `WAIT_4_KB` (E243-E33D)
  with `CURS_CTRL` (7B0E) bit 0 clear, recorded in `Sharp1500-1600-Ref`,
  `PC-1500/Keyboard/PC-1500-Keyboard-Scan.md` (`5fdf30b`). C1600-1 built it
  into the core in M-26: `PC1500Machine::keyReady()`
  (`core/PC1500/PC1500Machine.hpp:140-143`), reached as
  `Machine::isKeyReady()` through `SharpMachine` (`core/Machine/SharpMachine.hpp:63`).
  AC-32 is withdrawn, so the PC-1500's fixed cadence (the core's key
  queue) goes in this step. The research also gives: a running program
  samples only BREAK (latched by the LH5811); the ROM repeats only the
  four cursor keys. Nothing here rests on the pass length, which no
  primary source gives.
- M-22, M-31, M-33, M-34 (LIB-3): the machine session with its
  `SliceHook`, `WaitProbe`, `AdvanceGate`; `SessionPart`; the operations;
  the adapters `TIKeySequencerHook` and `SharpTypingHook` that this step
  removes (LIB-3 design §5).
- M-28 (LIB-11 L): the harness, its `CommandRegistry` and `HarnessHost`;
  `key-down` / `key-up` (raw, AC-39).
- M-35 (LIB-18 L): Reset, Cold boot, power cycle and model switch as
  commands. The engine is told of them (EC-5, §3.7).

Not here: the step engine and the `key:` / `type:` steps (LIB-6), the
hit test and the drawing of a pressed key (LIB-14, C59-8), focus itself
(FE-4), key readiness itself (LIB-1, C59-1, C1600-1), the PC-1500's
golden C-7 mode (done, M-8).

## 2. Modules and files

### Library: `library/typing/`, namespace `calcu::typing`

A new area of `calcu_library` (LIB-1 §2a). It includes `machine`,
`pacing` and `sessions`. `harness` includes it for the two commands.
§2a's dependency list gains the line "`typing`: `machine`, `pacing`,
`sessions`" (agent knowledge, §7). Tests go into `calcu_library_tests`
(Frequent).

| File | Holds |
|---|---|
| `library/typing/HostKey.hpp` | `NamedKey`, `Modifiers`, `HostKeyEvent` (§3.1) |
| `library/typing/KeyboardMap.hpp` | `KeyboardMap` (interface), `Mapped` (AC-7, AC-8) |
| `library/typing/TypingProfile.hpp/.cpp` | `TypingProfile`, `Cadence`, `StartRule`, `CharKeys`, `Press`, `NameLookup`; `frames(n, rate)` (§3.2) |
| `library/typing/TypingSupport.hpp` | the generic capability kind `typing-support` (`capabilities.md` C-1): prompt readiness, latches, the sample count (§3.4) |
| `library/typing/TypingEngine.hpp/.cpp` | `TypingEngine : sessions::SliceHook`: presses, holds, the queue, live input, typed text (§3.3) |
| `library/typing/TypingPart.hpp/.cpp` | `TypingPart : sessions::SessionPart`: the commands, the request, the topic, the product's profiles and maps (§3.5) |
| `library/typing/TypingTopics.hpp/.cpp` | the payloads `HeldKeys`, `TypingState`; topic, command and request names (§3.5) |
| `library/typing/Awaitable.hpp/.cpp` | `typeAndWait(OperationContext&, TypingEngine&, ...)`: typing inside an operation, for LIB-6's steps (§3.6) |
| `library/harness/TypingCommands.cpp` | the harness commands `key` and `type` (AC-15–17), added to the registry by `addFirstCommands` (LIB-11 design §3.3) |
| `library/sessions/SessionPart.hpp`, `AppSession.cpp` (changed) | `CommandOutcome` gains `Deferred`; `SessionContext::currentCall()` and `SessionContext::reply(CallId, …)` (§3.5, D-4); LIB-10 uses them from M-40 (the consolidated list of C2's LIB-3 additions: LIB-18 §8) |
| `library/harness/HarnessHost.cpp` (changed) | `command()` advances in virtual time until a deferred reply comes, at most one hour of emulated time, then replies with the sentence the harness command gives (D-5) |
| `library/typing/tests/TypingTestMachine.hpp/.cpp` | `test-type`: LIB-1's `test-seg` with a scriptable key readiness, a key log with tick counts, typing support with two latches and a sample count (§6) |
| `library/typing/tests/TestProfiles.hpp` | a profile and a keyboard map made for the tests |
| `library/typing/tests/*_tests.cpp` | one file per area (§6) |
| `library/CMakeLists.txt` (changed) | the sources of `library/typing/` and its tests |

### Calc-U-59

| File | Change |
|---|---|
| `products/calc-u-59/app/TITyping.hpp/.cpp` (new) | `cu59::typingProfile(Model)` (AC-20) with the names of AC-26 (`preset.md` §8.1, §8.1.1); `cu59::TIKeyboardMap : calcu::typing::KeyboardMap` (AC-21), the rules of today's `matrixCodesFor` |
| `products/calc-u-59/app/KeyboardMap.hpp/.cpp` | deleted; its table and rules move into `TIKeyboardMap`, its key IDs from `core/TIKeys.hpp` instead of matrix codes |
| `products/calc-u-59/app/KeySequencer.hpp/.cpp` | deleted. The keyboard lane, the held code, the deferred releases, `kHold`, `kScanPoll`, `releaseAll` go to the engine. The script lane (KEYSTROKES playback of format 1) becomes `TIKeystrokePlayback` (§5) |
| `products/calc-u-59/app/TIKeystrokePlayback.hpp/.cpp` (new, adapter) | the script lane of `KeySequencer` without its key handling: a `Key` event is handed to the engine as one press with the origin "preset" and the playback waits until the engine is idle (§4) |
| `products/calc-u-59/app/EmulatorSession.hpp/.cpp` | `KeySequencer::Host` → `TIKeystrokePlayback::Host` (no `pressKey`, `releaseKey`, `isKeyReady`, `heldKeyChanged`); `keyboardDown`, `keyboardUp`, `keyboardRelease`, `pressKey`, `releaseKey` go; `sliceEnded(due)` advances the playback only. The key trace (`m_manualTrace.writeKeyDown/Up`) moves to `TIKeyTraceObserver` (§5) |
| `products/calc-u-59/core/TIAdapters.hpp/.cpp` | `TITypingSupport : calcu::typing::TypingSupport`: the sample count (§3.4), no prompt readiness, no latches; listed by the TI machine as `typing-support` (owner `component:TMC0501`) |
| `products/calc-u-59/core/TMC0501.hpp/.cpp` | the keyboard-scan instruction (`TMC0501.cpp:501-545`) counts a match per key line it finds pressed, `keySamples(row, col)`; nothing else changes (golden runs must stay equal, §8) |
| `products/calc-u-59/app/TIAppPart.hpp/.cpp`, `TIAppPartFlows.cpp`, `TIAppPartInternal.hpp` | the commands `c59.press-key`, `c59.release-key`, `c59.keyboard-down`, `c59.keyboard-up`, `c59.keyboard-release` go; `heldKey` leaves `c59.run-state` (interim topic; LIB-3 design §5, "LIB-5 M-36"); `Hooks::act` drives `TIKeystrokePlayback`; `Hooks` gains the pause probe of AC-20 (`TIFreezeProbe`, §5); the reset and model-switch flows call `MachineSession::dropInput()` (EC-5, §3.7) |
| `products/calc-u-59/app/TIWebPart.cpp` | the page's key messages become the library commands (`host-key-down`, `screen-key-down`, …); `matrixCodesFor` goes |
| `products/calc-u-59/app/HarnessProduct.cpp`, `SessionThread.cpp`, the frontends' session setup | `calcu::typing::TypingPart` in the parts, before the product's (LIB-3 AC-12), with `cu59::typingProduct()` |
| `products/calc-u-59/frontends/swift/App/Views/KeyboardView.swift`, `CalculatorView.swift`, `App/EmulatorViewModel.swift`, `Bridge/TIAppSession.h/.mm`, `Bridge/TISharedTables.h/.mm` | host keys (macOS only, AC-22) as `host-key-down/up` with the event's fields; on-screen keys as `screen-key-down/up {key, pointer}`; focus loss as `focus-lost`; the pressed key drawn from `held-keys`. `TISharedTables`' keyboard-map wrapper goes |
| `products/calc-u-59/frontends/qt6/app/KeyboardWidget.cpp`, `EmulatorController.hpp/.cpp` | the same on Qt6 |
| `products/calc-u-59/frontends/web/src/PlayCalculator.jsx`, `bindings.cpp` | the same on the Web; `KeyboardEvent.key` of a letter, lowered, is the letter the key carries (AC-21, OQ-2) |
| `products/calc-u-59/tests/typing_tests.cpp` (new) | AC-20, AC-21, AC-23–27 (§6) |
| `products/calc-u-59/tests/shared_keys_tests.cpp`, `shared_tables_tests.cpp` | the key cases move to `typing_tests.cpp` (AC-27); the playback cases (`keys_reload_…`, `keys_arm_card_…`, `keys_script_waits_out_a_freeze_…`, `keys_wait_runs_at_the_speed_…`) stay on `TIKeystrokePlayback` |
| `products/calc-u-59/tests/SessionRig.hpp` | `kKeyboardDown` → the library's `screen-key-down`/`-up` |
| `products/calc-u-59/frontends/swift/Calc-U-59Tests/FrameGapTests.swift`, `MessageOrderTests.swift` | `pressKey`/`releaseKey` → LIB-3's raw `key-down`/`key-up` (they test message order, not typing; AC-39) |
| `products/calc-u-59/frontends/web/tests/play-calculator.spec.js` | the physical-keyboard cases follow AC-21 (letters by the carried letter) |

### Calc-U-1600

| File | Change |
|---|---|
| `products/calc-u-1600/app/SharpTyping.hpp/.cpp` (new) | `shared::typingProfile(model)` (AC-14, AC-28, AC-30, AC-31, AC-41); `shared::SharpKeyboardMap : calcu::typing::KeyboardMap` per model (AC-29), the rules of `resolveHostKey`; `kbiiSequence` and the character tables reached from the profile |
| `products/calc-u-1600/app/SharpMachineParts.hpp/.cpp` | `SharpTypingHook` deleted; `SharpDebuggerGate` stays (until M-45) |
| `products/calc-u-1600/app/HostKeyMap.hpp/.cpp` | deleted; `HostKey`/`HostModifiers` are the library's `NamedKey`/`Modifiers`; `resolveHostKey` becomes `SharpKeyboardMap::map` |
| `products/calc-u-1600/core/KeyPaste.hpp/.cpp` | `KeyPasteFeeder`, `buildPasteSteps`, `PastePacing` go to the engine and the profile. `KeyLatches` and `kbiiSequence` stay in `KeyPaste.hpp` for the typers until M-46 (§5) |
| `products/calc-u-1600/core/SharpShiftedSymbols.hpp`, `PC1500/PC1500TypedInput.*`, `PC1600/PC1600TypedInput.*` | stay as the character tables the profile reads (AC-30); `pc1600ReadLatches` and the prompt readiness become `SharpTypingSupport` |
| `products/calc-u-1600/core/Machine/SharpTypingSupport.hpp/.cpp` (new) | `typing-support` on the Sharp machines: prompt readiness, on the PC-1600 the latches `shift` and `kbii` (AC-35); no sample count |
| `products/calc-u-1600/core/Machine/LegacyCalls.hpp/.cpp` | `readLatches` (marked "M-36, LIB-5 AC-35") goes |
| `products/calc-u-1600/core/Machine/SharpProductKinds.hpp`, `PC1500/PC1500InterfaceMachine.cpp`, `PC1500/PC1500Machine.hpp/.cpp` | the adapter kind `c1600.key-queue` (`KeyQueue`) and the core's queue (`enqueueKey`, `advanceKeyQueue`, `m_keyQueue*`, `kKeyQueueCapacity`, `PC1500Machine.cpp:311-345`) go (AC-31, AC-32 withdrawn, row 1600-8) |
| `products/calc-u-1600/app/SharpAppPart.hpp/.cpp` | `typing()` and `m_typing` go; the reset, model and rebuild flows call `MachineSession::dropInput()` (EC-5, §3.7); `machineGone()`'s duty is `TypingPart::machineChanged` |
| `products/calc-u-1600/app/SharpWebPart.cpp` | `c1600.host-key-down/up`, `c1600.shift-tap`, `c1600.paste` and the faceplate key messages become the library commands |
| `products/calc-u-1600/app/HarnessProduct.cpp`, the Qt6 and Web session setup | `TypingPart` in the parts, with `shared::typingProduct()` |
| `products/calc-u-1600/frontends/qt6/app/MainWindow.cpp`, `MachineController.hpp/.cpp` | host keys as `host-key-down/up`; the input method's commit (`MainWindow.cpp:632-644`) as `host-text` (EC-4); the SHIFT tap of AC-33 stays in the frontend (wall time, TR) and is sent as `screen-key-down` + `screen-key-up` of `shift`; the faceplate as `screen-key-*`; `pasteText` → `paste`; `typeCommand` → engine `typeText` with ENTER through `TypingPart` (the debugger's auto-start, not a user key); `MachineController.cpp:114`'s wait for `typing().idle()` → request `typing` |
| `products/calc-u-1600/frontends/web/src/CalcU1600.jsx`, `Faceplate.jsx` | the same messages; the SHIFT tap (`CalcU1600.jsx:15, 321, 335-339`) stays in the page |
| `products/calc-u-1600/tests/golden_runs_tests.cpp` | the driver's `keys` (`:18`, `:155`) taps a PC-1500 key through the engine with the PC-1500 profile instead of `c1600.key-queue` (§8) |
| `products/calc-u-1600/tests/lh5801_tests.cpp` | `test_machine_enqueuekey_*` (`:1209-1275`) go with the queue |
| `products/calc-u-1600/app/tests/shared_machine_session_tests.cpp`, `shared_host_key_map_tests.cpp`, `tests/key_paste_tests.cpp` | ported to the engine (AC-37); the feeder cases become profile and engine cases |
| `products/calc-u-1600/tests/typing_tests.cpp` (new) | AC-28–31, AC-33–36, AC-40, AC-41 (§6) |

## 3. Seams and interfaces

Sketches. Every class is single-threaded on the session's thread,
throws nothing across its interface and reads no wall clock (TR;
ADR 0009). The one wall-time rule of the engine, the rest of a hold on
a paused machine (AC-20), uses the `now` of the host call that the
session hands its parts, never a clock of its own.

### 3.1 Host keys (AC-4, AC-7, EC-4)

```cpp
namespace calcu::typing {
enum class NamedKey { None, Escape, Backspace, Return, Enter /*keypad*/, Tab, Delete,
                      Up, Down, Left, Right, F1, F2, F3, F4, F5, F6,
                      ScrollLock, Insert, Home, End, PageUp, PageDown };
struct Modifiers { bool shift, control, alt, command, meta; bool macHost; };   // by what the user pressed
struct HostKeyEvent {
  std::string physical;      // the host's own key identity (browser `code`, Qt native scan code, NSEvent keyCode), pairs down and up (EC-1)
  NamedKey named = NamedKey::None;
  char32_t produced = 0;     // the character the layout produced, 0 for none or more than one
  char32_t carried = 0;      // a letter key: the letter it carries on the user's layout, lower case (AC-21, OQ-2)
  Modifiers mods;
  bool autoRepeat = false;   // the host's repeat: ignored (AC-4)
};
}
```

The frontend fills it from its event and sends it. It maps nothing
(AC-7). `NamedKey` is the union of today's `shared::HostKey`
(`app/HostKeyMap.hpp:18-27`) and the Escape of Calc-U-59
(`app/KeyboardMap.hpp:27-29`).

### 3.2 Profile and map (AC-7, AC-8, AC-11, AC-15, AC-20, AC-28)

```cpp
struct Press { machine::KeyId key; bool mustFinish = false; };          // mustFinish: AC-9 (the closing KBII)
struct Cadence { machine::Ticks hold; machine::Ticks gap; };           // a press of typed text or of a sequence
enum class StartRule { AtOnce, KeyReadiness };
struct TypingProfile {
  machine::Ticks minHold;                       // AC-1: TI 100 ms; Sharp 4 frames
  Cadence typed;                                // AC-11, AC-28: TI {100 ms, 0}; Sharp {4, 4} frames
  std::function<machine::Ticks(const machine::KeyId&)> extraGapAfter;   // PC-1600: 6 frames after shift and kbii
  bool queuedWaitsForReadiness;                 // AC-3, AC-20, AC-31: TI, PC-1500, PC-1500A yes; PC-1600 no (D-6)
  bool waitsForReadinessAfterRelease;           // AC-20: TI yes; Sharp no
  StartRule firstPressOfStep;                   // AC-14: PC-1600 KeyReadiness; PC-1500, TI by the two flags above
  std::optional<machine::Ticks> firstPressBound;// AC-14: PC-1600 15 s; EC-3: none on TI
  machine::Ticks readinessPoll;                 // how often a waiting press looks again (D-7): TI 1.5 ms, Sharp 1 frame
  std::optional<SampleRule> holdUntilSampled;   // §3.4: TI only
  // Typed text: a character → the presses, decided when its first press is due,
  // with the machine's typing support (latches, AC-13); nullopt = no key (AC-12, AC-16).
  // With no typing support (nullptr: LIB-6 checks a preset when it reads it) it answers
  // as for latches all off; it gives nullopt only for a character the model has no key
  // for, so the answer to "has a key" never depends on the latches (D-14).
  std::function<std::optional<std::vector<Press>>(char32_t, const TypingSupport*)> charKeys;
  std::optional<machine::KeyId> enter;          // AC-16: `type` presses it; TI: none (no characters, so `type` does not exist)
  // `key` names (AC-15, AC-26, AC-41): a name → presses; nullopt = refused
  std::function<std::optional<std::vector<Press>>(std::string_view name)> name;
};
machine::Ticks frames(int n, machine::TickRate);   // n × rate / 60, as SharpTypingHook::frameTicks today

struct Mapped { std::vector<Press> presses; bool held; };    // held: one key that stays down until key-up (AC-4); else a sequence (AC-8)
class KeyboardMap { public: virtual std::optional<Mapped> map(const HostKeyEvent&) const = 0; protected: ~KeyboardMap() = default; };

class TypingProduct {                                  // what a product hands TypingPart
public:
  virtual const TypingProfile* profile(const machine::ModelId&) const = 0;
  virtual const KeyboardMap* keyboardMap(const machine::ModelId&) const = 0;   // nullptr: no host keys (none today)
protected: ~TypingProduct() = default;
};
```

Ticks are computed from the profile's durations with the machine's tick
rate when a machine is set (`EmulatedTime::fromSeconds`, LIB-1), so
every rule below is in ticks.

### 3.3 The engine (AC-1–AC-10, AC-18, AC-38, AC-39; EC-1–EC-6)

```cpp
enum class Origin { Live, Typed /*paste, `type`, `key`, debugger line*/, Preset /*LIB-6, today's playback*/ };
class TypingEngine final : public sessions::SliceHook {
public:
  void setMachine(machine::Machine*, const TypingProfile*, const KeyboardMap*);   // EC-5: everything dropped
  // Live input
  void hostKeyDown(const HostKeyEvent&);  void hostKeyUp(const std::string& physical);
  void hostText(std::u32string_view);                          // EC-4
  void screenKeyDown(const machine::KeyId&, std::uint32_t pointer);  void screenKeyUp(const machine::KeyId&, std::uint32_t pointer);
  void focusLost();                                            // AC-10
  // Typed text and commands
  void paste(std::string_view utf8);                           // AC-12, AC-34
  std::optional<std::string> typeKeys(std::vector<Press>, Origin);         // AC-15
  std::optional<std::string> typeText(std::string_view utf8, bool enter, bool refuseUnknown, Origin);  // AC-16; reason when refused, nothing queued
  void drop() override;                                        // EC-5: LIB-18's SliceHook::drop() (its design §3.4, D-12): every key the engine holds up now, nothing waits
  bool idle() const;                                           // nothing down and nothing waiting
  std::vector<machine::KeyId> liveHeld() const;                // AC-19
  void setPaused(bool paused, pacing::WallTime now);           // AC-20
  void hostCall(pacing::WallTime now, bool virtualTime);       // AC-20: the rest of a hold on a paused machine
  void setObserver(KeyObserver*);                              // §5: TIKeyTraceObserver until M-53
  // SliceHook
  machine::Ticks nextDue(machine::Ticks now) const override;
  void act(machine::Machine&, machine::Ticks now, pacing::WallTime) override;   // the wall time is not read
};
```

**State.** A list of keys the engine has put down, each with the tick
it went down, its holders (host `physical` identities and screen
pointers; empty for an engine press) and whether its release is asked
for. A queue of presses that wait, each with its origin, whether it is
the first of a step, and `mustFinish`. Keys put down by `key-down`
(LIB-3, harness) are not in it (AC-39): the engine never releases or
waits for them.

**Rules.**

| # | Rule | Serves |
|---|---|---|
| E1 | A key the engine put down comes up no earlier than `down + minHold`. A release asked for earlier is carried out then (`nextDue` = that tick), never dropped. On a TI profile also not before the key is sampled (§3.4) | AC-1, TR |
| E2 | A live single key (`Mapped.held`, a screen key) that finds no engine press in its hold or gap goes down at once, whatever readiness says; otherwise right after that hold and gap. What waits is dropped first (E4) | AC-2, AC-25, AC-40, OQ-1 |
| E3 | A waiting press starts when the press before it is up, its gap (`typed.gap` + `extraGapAfter`) has passed, and, where the profile says, the machine is key ready (polled every `readinessPoll` through `nextDue`). The first press of a step starts by `firstPressOfStep`; with a bound, it starts when the bound has passed all the same. A press is held `typed.hold` (at least `minHold`) | AC-3, AC-11, AC-14, AC-20, AC-28, AC-31, EC-3 |
| E4 | Live input that arrives while presses wait drops them, except the `mustFinish` ones, which are typed first; the live key takes its turn after them | AC-9, AC-2 |
| E5 | A held live key stays down until its last holder lets go (E1 for the minimum). A holder that lets go before its press started makes it one press of `minHold`. `autoRepeat` events do nothing | AC-4, EC-6 |
| E6 | Two live keys held together are both down, each from its own down to its own up. Engine presses never overlap each other (E3) | AC-5, AC-23 |
| E7 | A host key that maps to a sequence queues its presses as one sequence (E3 cadence, origin Live); its key-up does nothing | AC-8 |
| E8 | A key-up for a `physical` (or pointer) that holds nothing does nothing | EC-1 |
| E9 | A host key or text with no mapping, or a character with no key: nothing is typed, nothing waits | EC-2, AC-12 |
| E10 | `focusLost`: every live holder lets go (E1 still applies), and what waits is dropped (`mustFinish` excepted) | AC-10, OQ-3 |
| E11 | Live input while the session is busy is dropped by `TypingPart` before it reaches the engine. A key still held when the operation ends is not pressed then. Releases always pass | AC-18, AC-36, T-4 |
| E12 | `drop()` (reset, power cycle, cold boot, model switch: `MachineSession::dropInput()`) and `setMachine` (a new or replaced machine): every key the engine holds comes up at once, without its minimum (the machine starts over), nothing waits, a deferred reply is answered "not done" | EC-5 |
| E13 | A character's presses are decided when its first press is due, with the typing support of that moment (latches) | AC-13, AC-34 |
| E14 | Nothing reads a clock; every decision is a function of the inputs and the tick count at which the host call delivered them | AC-6, I-1, I-3 |

**Inputs at a tick.** A command arrives between two host calls. The
engine applies it at the machine's tick count at that moment, so two
runs with the same inputs at the same tick counts give the same key
log (AC-6), paced, at full speed or in virtual time. The session cuts
its advances at `nextDue` (LIB-3 §3.7), so a release or a start lands
on its tick within one instruction.

### 3.4 Typing support and "sampled" (AC-13, AC-35; TR; Q-2)

```cpp
class TypingSupport : public machine::Capability {      // generic kind "typing-support"
public:
  static constexpr machine::KindId kKind{"typing-support"};
  virtual std::optional<bool> promptReady() const { return {}; }          // AC-35; nullopt: the model has none
  virtual std::vector<std::pair<std::string, bool>> latches() const { return {}; }  // AC-13: PC-1600 {shift, kbii}
  virtual std::optional<std::uint64_t> samples(const machine::KeyId&) const { return {}; }   // TI: scans that found the key down
};
struct SampleRule { std::uint32_t samplesNeeded; machine::Ticks maxHold; };
```

**How the TI machine tells that a key was sampled.** The ROM reads
the key matrix only through the keyboard-scan instruction (opcode
`0x08xx`, `core/TMC0501.cpp:501-545`, both modes). C59's
`TITypingSupport` counts, per key, the scan instructions that found
its line pressed at its row (the `kmask` match). The engine notes the
count when the key goes down. A TI press then comes up when `minHold`
has passed and the count has grown by `samplesNeeded`, or at `maxHold`
(EC-3: a program that never scans must not keep a key down for ever).
At the idle scan loop the count grows within a few sweeps, so a tap
stays 100 ms (AC-20, AC-27). On a printing loop that scans rarely the
key stays down until it is seen. AC-25 is the measure (TR).

The values are not known: FS 5 gives only that 100 ms is too short and
200 ms always enough on `Lbl A Prt GTO A`. Task T59-0 measures them
first (§6): the smallest `samplesNeeded` with which a tap of R/S stops
that loop in each of the four phases on the TI-59, TI-58 and TI-58C, at
regular and full speed, while a key at the idle loop still gives one
keycode and Adv one line. Best guess until then: 2 (the idle loop arms
and confirms, `TMC0501.cpp:521-528`), `maxHold` 1 s (D-3).

### 3.5 The session part, commands and topics (AC-17, AC-19, AC-38)

| Name | Shape | Payload, args | Notes |
|---|---|---|---|
| command `host-key-down` | — | `HostKeyEvent` as a `Value` | dropped while busy (E11) |
| command `host-key-up` | — | `{physical}` | always |
| command `host-text` | — | `{text}` | EC-4; dropped while busy |
| command `screen-key-down`, `screen-key-up` | — | `{key, pointer}` | AC-24; an unknown key ID is refused (LIB-1 EC-2) |
| command `focus-lost` | — | — | AC-10 |
| command `paste` | — | `{text}` | AC-12, AC-34; dropped while busy |
| command `key` | — | `{names: [..]}` | AC-15, AC-17: `Deferred`, replied done when the last key is up; refused for an unknown name, nothing typed |
| command `type` | — | `{text}` | AC-16, AC-17: `Deferred`, replied done when ENTER is up; refused for a character without a key, or on a profile without `enter` ("Unknown command" is what the harness gives, D-8) |
| topic `held-keys` | state | `HeldKeys{std::vector<KeyId> live;}` | AC-19: live input only |
| request `typing` | — | `TypingState{bool idle; std::size_t waiting;}` | for frontends that wait (1600 Qt6 `MachineController.cpp:114`), tests |

`TypingPart::attach` declares them and adds the engine as a hook of
the machine session. `machineChanged` calls `setMachine` with the
product's profile and map for the model. `beforeCall` hands the engine
the pause state (`MachineSession::paused()`, or a product's
`PauseProbe` until LIB-10, §5) and `afterCall(now)` the host call's
time (E12, AC-20). In `Hosting::VirtualTime` the rest of a paused hold
passes at once (AC-20).

**Deferred replies** (D-4). `key` and `type` are not operations
(AC-38): the session is not busy, live input drops them (AC-9; the
reply is then "not done: stopped by a key", D-9), yet AC-17 wants the
reply when the keys are up. `CommandOutcome` gains
`Deferred{}`; the part keeps the `CallId` it gets from
`SessionContext::currentCall()` and later calls
`SessionContext::reply(call, ReplyMessage)`, once. A paced host (an attached
harness) gets the reply when it comes, through LIB-11's pending reply
(`HarnessHost::kPendingReply`). `HarnessHost::command` in virtual time
advances in pieces of at most `kRunPiece` (1/60 s; the machine session
cuts them at the hooks' `nextDue`) until the reply comes, at most one hour
of emulated time (as `wait`, LIB-6 spec, `wait` row), then replies
not done with the sentence its harness command gives; for `key` and
`type`: "The keys were not taken within an hour." (D-5). LIB-10's
`pause` and `step` use the same outcome and the same advance (M-40).

### 3.6 Typing inside an operation (for LIB-6; AC-14, AC-17)

```cpp
struct TypeOptions {
  Origin origin = Origin::Preset;
  bool firstPressAtOnce = false;   // LIB-6 AC-58: after `skip-wait:` the first press goes down at once (StartRule::AtOnce for this step)
};
sessions::Task<TypeResult> typeAndWait(sessions::OperationContext&, TypingEngine&, std::vector<Press>, TypeOptions = {});
sessions::Task<TypeResult> typeTextAndWait(sessions::OperationContext&, TypingEngine&, std::string text, bool enter, TypeOptions = {});
```

They queue, then `co_await ctx.run(min(nextDue − now, runPiece))` until
the engine is idle. `ctx.run` goes through `MachineSession::advance`,
which cuts at the hooks (`library/sessions/Operation.cpp:16-31`), so
the cadence inside an operation is the same as outside. LIB-6's step
engine uses these; after `skip-wait:` (LIB-6 AC-58) it passes
`firstPressAtOnce = true` for the next step, which overrides the
profile's `firstPressOfStep` for that step's first press only.

### 3.7 Who tells the engine of a reset

EC-5 needs the engine to know of a reset, a power cycle and a cold
boot, which do not change the machine object. LIB-18's design (§3.4,
D-12) adds `virtual void drop() {}` to `sessions::SliceHook` and
`MachineSession::dropInput()`, which calls it on every hook; its
library commands call `dropInput()` before each build, Reset, Cold boot
and power cycle. The engine implements `drop()` (E12). Until LIB-18's
product columns (M-46, M-54) the products' own flows run these, and
each calls `MachineSession::dropInput()` from M-36: `TIAppPart`'s reset
and `changeModule`/`togglePrinterConnection` flows, `SharpAppPart`'s
`resetJob`, `pickModelJob` and rebuild. `TIKeystrokePlayback`'s hook
keeps the default `drop()`: its cancel on a reset is today's flow.

## 4. Product side

**Calc-U-59.** The profile (AC-20): `minHold` 100 ms; `typed` {100 ms,
0}; `queuedWaitsForReadiness` and `waitsForReadinessAfterRelease`;
`readinessPoll` 1.5 ms (today's `kScanPoll`); `holdUntilSampled` (§3.4);
`charKeys` none, `enter` none (AC-16: no `type` on a TI model); `name`
by `preset.md` §8.1 and §8.1.1, matched without case, a second-function
name as `2nd` + key, a `2nd` written before it pressed once (AC-26).
The keyboard map (AC-21) is today's 26 bindings by today's rules, with
the letter taken from `carried` (OQ-2): a letter with Shift is the
sequence `2nd`, letter; a single key is `held`. Live keys go down at
once (AC-2, AC-25): R/S and GTO reach a running program as today.

`KeySequencer`'s rule "a keyboard tap goes in while frozen, a script
waits" becomes AC-20: a live single key goes down at once (E2), a press
that waits (a Shift+A sequence, a playback key) waits for the machine
to run. The TI freeze is today's `EmulatorSession` state (LIB-10 59 is
M-53): `TIFreezeProbe` (§5) reports it to `TypingPart` as paused.

KEYSTROKES playback (format 1, until LIB-6 59 in M-54) keeps its lane,
its `Wait:` in wall time (2.3.0; format 2's `wait:` is emulated time,
LIB-6), its reloads, cards and controls. Its `Key` events go through
the engine as presses of origin Preset, held 100 ms of emulated time and
until sampled (row 59-23). This is the fix of the printer-timing
finding (§8). `TIPresetOperation` (LIB-3 adapter) still ends at
`playbackFinished`; while it runs, live input is dropped (E11).

The pressed key is drawn from `held-keys` in Swift, Qt6 and the Web;
`c59.run-state` loses `heldKey`. On-screen keys of every frontend go
through `screen-key-*` (AC-24; today Swift and Qt6 press the matrix
directly with `c59.press-key`).

**Calc-U-1600.** The profiles, per model (AC-28, AC-14, AC-31):

| | PC-1500, PC-1500A | PC-1600 |
|---|---|---|
| `minHold`, `typed` | 4 frames; {4, 4} frames | 4 frames; {4, 4} frames, + 6 after `shift` and `kbii` |
| `queuedWaitsForReadiness` | yes (AC-31, row 1600-8) | no (D-6) |
| `firstPressOfStep`, bound | at once (by the flag: at readiness, D-6) | key readiness (C1600-1 AC-21), 15 s |
| `charKeys` | AC-30: `PC1500TypedInput`, `SharpShiftedSymbols`; a lower-case letter `shift` + key | AC-30: `PC1600TypedInput`, digit-row legends, KBII by `kbiiSequence` and the latches (AC-34), closing `kbii` `mustFinish` |
| `name` | key IDs (C1600-1 AC-9, AC-10) and `break` for `on`, any case; `ent` refused (AC-41) | the same |
| `enter` | `enter` | `enter` |

The waits after ENTER (`kPostEnterBusyCap`, `kPostEnterFloorFrames`,
`settleUntilIdle`) are not in the profile (AC-28); they stay in the
preset typers until LIB-6 1600 (§5).

The keyboard map is `resolveHostKey`'s rules per model (AC-29). On
every model a single key is `held` (AC-31: the cursor keys and every
other key stay down; today the PC-1500 queues all but the cursor keys);
a key that needs `shift` or KBII is a sequence (AC-8). The PC-1500's
core queue (`PC1500Machine::enqueueKey`, 16 entries, 4 + 4 frames) and
`c1600.key-queue` go. Paste Text on every model is `paste` (AC-34; a
second paste appends, `SharpTypingHook::pasteText` today). BREAK is the
live key `on`, down at once (AC-40). The SHIFT tap of AC-33 stays the
frontends' wall-time rule and is sent as one screen press of `shift`.

## 5. Adapters

| Adapter | File | Wraps | Removed in |
|---|---|---|---|
| `TIKeystrokePlayback` | `products/calc-u-59/app/TIKeystrokePlayback.*` | the KEYSTROKES script lane of `KeySequencer` (reloads, cards, controls, `Wait:` in wall time, speed directives) over the engine for its keys; advanced from `TIAppPart::Hooks::act` with `SliceRequest::due` (was `TIKeySequencerHook`) | M-54 (LIB-6 59: format 2 steps over `typeAndWait`) |
| `SliceRequest::due` (LIB-17 §5) | `library/pacing/Pacer.hpp` | the wall time at slice ends, now read only by `TIKeystrokePlayback`'s `Wait:` | M-54 (was "M-36 if nothing else reads it") |
| `TIFreezeProbe` | `products/calc-u-59/app/TIAppPartInternal.hpp` | the TI freeze (`EmulatorSession::frozen()`) as the pause of AC-20, for `TypingPart` (`PauseProbe`, a one-function interface of `library/typing/TypingPart.hpp`) | M-53 (LIB-10 59: the freeze is `MachineSession::setPaused`) |
| `TIKeyTraceObserver` | `products/calc-u-59/app/EmulatorSession.*` | the manual trace's key records (`TraceWriter::writeKeyDown/Up`), fed from the engine's `KeyObserver` | M-53 (row 59-19: a trace holds instructions and gaps only) |
| Sharp preset typers | `products/calc-u-1600/core/PC1500/PC1500BasicTyper.*`, `PC1600/PC1600BasicTyper.*`, their users in `PC1500PresetLoader.cpp`, `PC1600PresetLoader.cpp`, `Preset/PresetRunner.cpp` | `tapKeyJob`, `typeLineJob` (format 1 `key:`/`type:` with their waits after ENTER) and `typeBasicProgramTextJob`; they already count in emulated frames, run inside operations through `JobForward`, and read `KeyLatches`/`kbiiSequence` from `KeyPaste.hpp` | M-44 (C1600-2: typing a program in), M-46 (LIB-6 1600: the steps over `typeAndWait`); `JobForward` with them, at M-46 as LIB-3 §5 allows |

`SharpTypingHook` (LIB-3 adapter) goes in this step, with
`KeyPasteFeeder`, `HostKeyMap` and the PC-1500 key queue.
`TIKeySequencerHook`'s key part goes; its slice-end call stays for
`TIKeystrokePlayback`. `KeySequencer` and `KeyboardMap.*` go.

## 6. Tasks

Tests come before the code of each task. Library tests run
`test-type` (§2) in a library `AppSession` with `TypingPart`, the test
frontend and `ScriptedClock` of `library/sessions/tests/`, paced and in
virtual time. The key log records (tick, key, down/up). Case names:
`acN_`/`ecN_` in the library, `lib5_acN_` in the products (LIB-1 §2a).

| Task | What | ACs/ECs | Test (file, case, group) |
|---|---|---|---|
| L1 | `test-type`, `TestProfiles` | — | `typing_machine_tests.cpp`: `test_type_logs_keys_with_ticks`; Frequent |
| L2 | E1: minimum hold | AC-1 | `holds_tests.cpp`: `ac1_early_key_up_carried_out_at_min_hold`, `ac1_key_up_never_dropped`; Frequent |
| L3 | E2, E4: live at once, drops waiting | AC-2, AC-9 | `live_tests.cpp`: `ac2_live_key_down_at_once_when_not_ready`, `ac2_live_key_after_engine_hold_and_gap`, `ac9_live_drops_waiting_but_finishes_must_finish`; Frequent |
| L4 | E3: queue, gap, readiness, first press | AC-3, AC-11, EC-3 | `queue_tests.cpp`: `ac3_next_press_after_up_gap_and_readiness`, `ac11_text_typed_with_profile_cadence`, `ec3_waits_without_limit_until_dropped`, `ec3_first_press_bound_starts_all_the_same`; Frequent |
| L5 | E5, E6, E8: holds, two keys, stray key-up | AC-4, AC-5, EC-1, EC-6 | `live_tests.cpp`: `ac4_held_until_let_go`, `ac4_let_go_before_start_is_one_min_press`, `ac4_auto_repeat_ignored`, `ac5_two_keys_held_together_both_down`, `ac5_engine_never_overlaps`, `ec1_key_up_of_other_key_does_nothing`, `ec6_two_holders_one_key_down_until_last`; Frequent |
| L6 | E14: determinism | AC-6 | `determinism_tests.cpp`: `ac6_same_inputs_same_key_log_paced_full_virtual` (three hostings, one log); Frequent |
| L7 | Map and sequences | AC-7, AC-8, EC-2 | `map_tests.cpp`: `ac7_frontend_event_mapped_by_model_map`, `ac8_sequence_presses_key_up_does_nothing`, `ec2_unmapped_types_nothing`; Frequent |
| L8 | E10, E12 | AC-10, EC-5 | `live_tests.cpp`: `ac10_focus_lost_releases_after_min_drops_waiting`; `reset_tests.cpp`: `ec5_reset_power_cycle_cold_boot_replace_all_up_nothing_waits`; Frequent |
| L9 | Typed text, paste, latches, EC-4 | AC-11, AC-12, AC-13, EC-4 | `text_tests.cpp`: `ac12_paste_first_line_skips_unknown_no_enter`, `char_keys_without_support_same_has_a_key_answer` (D-14), `ac13_latched_key_released_first_or_skipped`, `ec4_committed_text_each_char_one_press`; Frequent |
| L10 | `TypingPart`: commands, busy, raw keys | AC-18, AC-38, AC-39 | `part_tests.cpp`: `ac18_live_dropped_while_busy_not_pressed_after`, `ac38_key_and_type_not_busy_stopped_by_live`, `ac39_raw_key_down_not_engines`; Frequent |
| L11 | `held-keys` topic | AC-19 | `part_tests.cpp`: `ac19_held_keys_lists_live_only`; Frequent |
| L12 | `Deferred` in sessions; harness `key`, `type` | AC-15, AC-16, AC-17 | `sessions/tests/command_tests.cpp`: `deferred_reply_comes_later_and_not_busy`; `harness/tests/commands_tests.cpp`: `ac15_key_names_and_refusal`, `ac16_type_presses_enter_refuses_unknown_char`, `ac16_type_absent_without_char_profile`, `ac17_key_replies_when_last_key_up_not_idle`, `ac17_type_replies_when_enter_up`, `harness_key_gives_up_after_an_hour`; Frequent |
| L13 | `typeAndWait` | AC-14 (engine part), AC-17 | `operation_tests.cpp`: `ac14_first_press_rule_inside_an_operation`, `ac14_first_press_at_once_overrides_the_rule`, `type_and_wait_same_log_as_outside`; Frequent |
| L14 | Pause | AC-20 (library part) | `pause_tests.cpp`: `held_key_on_paused_machine_comes_up_after_rest_in_host_time`, `at_once_in_virtual_time`, `waiting_press_waits_for_run`; Frequent |
| T59-0 | Measure the sample rule (§3.4) | TR, AC-25 | `tests/typing_tests.cpp`: `lib5_tr_sample_rule_measurement` prints the table for the qa-report; then fixes `samplesNeeded`, `maxHold` (D-3). Frequent if within budget, else Longer |
| T59-1 | TI profile, sampled, pause | AC-20, AC-25 | `typing_tests.cpp`: `lib5_ac20_ti_profile`, `lib5_ac20_freeze_inside_hold_key_up_on_frozen_machine`, `lib5_ac25_rs_stops_running_program` (`Lbl A GTO A`, `Pause` loop), `lib5_ac25_rs_stops_printing_loop_every_phase` (TI-59, TI-58, TI-58C, regular and full speed), `lib5_ac25_gto_acted_on`; Frequent |
| T59-2 | Keyboard map | AC-21 | `typing_tests.cpp`: `lib5_ac21_the_26_bindings`, `lib5_ac21_letter_by_carried_letter`, `lib5_ac21_command_modifiers_map_nothing`; Frequent |
| T59-3 | Frontends | AC-22, AC-24 | Qt6: `CalcU59QtTyping` (offscreen, a host key and an on-screen key, `held-keys`); Web: `play-calculator.spec.js` cases (host keys, on-screen key held 100 ms); Swift macOS: an XCUITest of `UIRegressionTests` over attach reading `held-keys`; iOS: no host keyboard; Longer |
| T59-4 | Two keys, names | AC-23, AC-26 | `typing_tests.cpp`: `lib5_ac23_second_key_does_not_release_first`, `lib5_ac26_names_of_preset_md_8_1`, `lib5_ac26_2nd_before_second_function_once`, `lib5_ac26_old_names_refused`; Frequent |
| T59-5 | Today's key tests ported | AC-27 | `typing_tests.cpp`: `lib5_ac27_sequence_key_waits_readiness_held_min`, `_key_up_deferred`, `_shift_binding_two_presses_new_key_drops_second`, `_map_rules`; Frequent |
| T16-1 | Sharp profiles | AC-14, AC-28, AC-30 | `tests/typing_tests.cpp`: `lib5_ac28_cadence_4_4_and_6_after_shift_pc1600`, `lib5_ac14_type_starts_at_readiness_pc1600_bound_15s`, `lib5_ac14_type_starts_at_once_pc1500`, `lib5_ac30_characters_per_model`; Frequent |
| T16-2 | Map | AC-29 | `typing_tests.cpp`: `lib5_ac29_named_keys_and_shift_forms_per_model`, `lib5_ac29_command_modifiers_nothing_altgr_types`; Frequent |
| T16-3 | Held keys, PC-1500 readiness | AC-31 | `typing_tests.cpp`: `lib5_ac31_cursor_key_held_rom_repeats_pc1500` (the exit check), `lib5_ac31_every_key_held_pc1500a`, `lib5_ac31_waiting_press_at_key_readiness_pc1500`; Frequent |
| T16-4 | SHIFT tap | AC-33 | Qt6 `CalcU1600QtTyping` (offscreen) and Web `typing.spec.js`: tap within 400 ms → one `shift` press; Longer |
| T16-5 | Paste and latches | AC-34, AC-35 | `typing_tests.cpp`: `lib5_ac34_paste_releases_latched_shift_skips_under_kbii`, `lib5_ac34_second_paste_appended`, `lib5_ac35_prompt_readiness_and_latches`; Frequent |
| T16-6 | Busy in both frontends | AC-36 | `typing_tests.cpp`: `lib5_ac36_keys_dropped_during_preset_operation`; Web `typing.spec.js` `keys during a load are dropped`; Frequent, Longer |
| T16-7 | Today's key tests ported | AC-37 | `app/tests/shared_machine_session_tests.cpp` (instant press and release one key, double tap two, fast sequence in order), `shared_host_key_map_tests.cpp` → `SharpKeyboardMap`, `tests/key_paste_tests.cpp` → profile and engine; Frequent |
| T16-8 | BREAK, names | AC-40, AC-41 | `typing_tests.cpp`: `lib5_ac40_on_breaks_running_program`, `lib5_ac41_break_alias_ent_refused`; Frequent |
| R-1 | Reference and golden runs | §8 | `CalcU59ReferenceRuns`, `lib3_reference_examples_through_session`, both golden runs; Longer, Frequent |

Coverage: L1-L14 cover AC-1 to AC-19, AC-38, AC-39 and EC-1 to EC-6
(the library part of AC-20 in L14). T59 cover AC-20 to AC-27. T16
cover AC-28 to AC-31, AC-33 to AC-37, AC-40, AC-41. AC-32 is withdrawn.

## 7. Tools and agent knowledge

Today's tools stay (Briefing §8): `tools/headless/ti59.py` keeps its own
key names and its own hold until M-59 (C59-9, LIB-11 59); `pc1500_cli`
and `pc1600_cli` press keys on the core directly until M-51. None of
them uses `KeySequencer`, `SharpTypingHook` or the PC-1500 key queue
(checked: no use under `tools/`).

| Step | Files |
|---|---|
| M-36 | `features/LIB-1-machine-interface-and-descriptors/design.md` §2a (the `typing` area and its includes); `docs/glossary-mapping.md` row "Keyboard map" (`KeyboardMap`, `KeySequencer` → `cu59::TIKeyboardMap`, the library's typing engine; `HostKeyMap` → `shared::SharpKeyboardMap`), a row "Typing engine" (`KeySequencer`, `SharpTypingHook`, `KeyPasteFeeder`, the PC-1500 key queue → `calcu::typing::TypingEngine`), row "Machine session" (`TIKeySequencerHook` → `TIKeystrokePlayback`); Calc-U-59: `reference/SharedArchitecture.md` (§ "Physical Keyboard Mapping", key sequencing), `AppArchitecture.md`, `FrontendParity.md`, `NewGUIGuide.md`, `StateFileFormat.md` (the matrix-code note), the `calc-u-59-shared`, `calc-u-59-swift`, `calc-u-59-qt`, `calc-u-59-web` skills (they name `KeySequencer`, `KeyboardMap`), `prompt/ti59-agent-prompt.md` if it names keys (plan M-36), memory `calc-u-59/headless_printer_lines.md` ("like KeySequencer"); Calc-U-1600: `docs/developer/Shared-Architecture.md`, `Web-Client.md`, `Debugger-Internals.md`, `docs/Keyboard-Mapping.md` (the map's place), memory `calc-u-1600/feedback_paste_vs_inbound_typing.md` (`typeCommand`), backlog B1600-69 closed by the engine (no PC-1600 key queue needed), B59-6 input to the frontends' design (not built: FE-4) |

`products/calc-u-1600/docs/background/*` stays as history.

## 8. Golden runs and risks

- **The printer-timing finding (M-34; `CalcU59ReferenceRuns`).**
  `examples/debug/printer-timing.ti59` gives 106 printed lines instead
  of 104 (two more blank feeds) and a last `Elapsed:` of 3.236 s
  instead of 2.213 s (LIB-3 qa-report). Diagnosis by reading the code
  (unverified by a run, HANDOVER 2026-11-15): `KeySequencer` times its
  100 ms hold against the slice due times it is handed
  (`app/KeySequencer.cpp:106,112`, through `TIAppPartInternal.hpp:21-23`
  and `EmulatorSession::sliceEnded`, `EmulatorSession.hpp:178-180`). At
  full speed each slice's due time is a fresh read of the clock
  (`library/pacing/Pacer.cpp:121`), and since M-33 the key-detect spin
  during a hold is not paced (LIB-17 D-3, row 59-32). One hold therefore
  spans many slices and a held Adv repeats. The test driver adds 1 ms
  per clock read (`tests/SessionRig.hpp`, `SessionRig`'s clock), so a hold is about 300 ms of
  emulated time there: the two feeds and the +1.02 s. 2.3.0 passed the
  pump's fixed `now` (`Shared/EmulationRunner.cpp:168`) and paced the
  key-detect range (`Shared/Pacing.cpp:26-29`).
  **This step fixes it:** a preset's key is held 100 ms of emulated time
  (and until sampled), however the host's clock moves (E1, §4; AC-20,
  row 59-23). The result will still differ from the 2.3.0 recording in
  the example's "full speed via keypress" part, by design: in 2.3.0 one
  Adv press at full speed fed as many lines as 100 ms of wall time
  allowed, now it feeds the lines of 100 ms of emulated time, at every
  speed (row 59-23; the exit check of M-36). Plan: in R-1, run
  `printer-timing.ti59` at regular and at full speed through the
  session and check that both give the same paper (the exit check);
  compare with the reference; record the difference in the qa-report
  as the 59-23 change. A new recording replaces the reference only with
  the user's OK at the C2/D boundary (plan §1: a difference is a
  finding). The stopgap of the handover (the call's start as the due
  time of unpaced slices) is not taken.
- **`historic/life.ti59`** (one register at the 30-min cut) is
  plausibly the full-speed time per 30 min of driver time (inference,
  not checked). It is not a typing matter; R-1 reports whether it
  changes.
- **The other 32 TI examples** differ in time lines only since M-33. The
  `Elapsed:` lines move again where a preset key was held over a
  full-speed stretch (row 59-23: "an `Elapsed:` line over a full-speed
  stretch counts less"). A finding per example in the qa-report.
- **TI golden runs** (`products/calc-u-59/tests/golden-runs/`) drive the
  machine directly. The sample counter in `TMC0501` must leave them
  equal; a difference is a finding.
- **Sharp golden runs.** The driver's `keys` taps a PC-1500 key through
  `c1600.key-queue` (`tests/golden_runs_tests.cpp:155`), which goes.
  The engine with the PC-1500 profile gives the same 4 + 4 frames, but
  the queue acted inside `runCycles` at its own cycle count
  (`PC1500Machine.cpp:317-345`) and the engine acts at the tick the
  session cut at; the press may land a few cycles apart. The runs
  compare the state 200 ms later; a difference is a finding, not a new
  reference.
- **Sharp reference runs** (M-4, `lib3_reference_examples_through_session`)
  load through the preset typers, which this step does not change (§5).
  They must stay equal.
- **TI tap held until sampled.** A key held longer than 100 ms at a
  repeat-while-held key (Adv) could feed one more line. At the idle loop
  the count grows within 100 ms, so the press stays 100 ms; T59-0
  checks "Adv feeds one line" with the measured rule.
- **Readiness polling cost.** A press that waits cuts the advance every
  `readinessPoll` (TI 1.5 ms, as `kScanPoll` polled today; Sharp one
  frame). While a TI program runs and a press waits for readiness (EC-3)
  the session runs in 1.5 ms pieces without limit; the cost is that of
  2.3.0's poll.
- **Deferred replies** are new in LIB-3's sessions (Done). The change is
  additive (a new alternative of `CommandOutcome`); LIB-3's tests stay.
- **Frontends in one step.** M-36 changes the key path of five
  frontends at once. The Longer group covers Qt6 and the Web; the Swift
  part is checked by its XCUITest and by hand (macOS host keys). CI runs
  at the end of C2 (plan §1).

## 9. Decided (best guess)

- **D-1 (Done in).** The Library column is Done in M-38 (AC-15's preset
  reading), Calc-U-59 in M-54, Calc-U-1600 in M-46, where the preset
  parts of AC-14, AC-15, AC-17, AC-26, AC-28, AC-41 pass (plan §1,
  "Completes and Done"). Every other criterion passes in M-36.
- **D-2 (adapters past M-36).** KEYSTROKES playback (format 1) keeps its
  lane and its wall-time `Wait:` until M-54; its keys go through the
  engine now. The Sharp preset typers stay until M-44 / M-46: they
  already count in emulated frames and their waits after ENTER are
  preset behaviour (LIB-6). `SliceRequest::due` therefore stays until
  M-54, and `JobForward` until M-46.
- **D-3 (TR, Q-2: "sampled").** The TI machine's typing support counts
  the keyboard-scan matches per key; a TI press is held `minHold` and
  until `samplesNeeded` new matches, at most `maxHold`. Values measured
  in T59-0; best guess 2 and 1 s. Typing support gains this field
  (proposed wording for `capabilities.md` C-1 and `typing.md` T-2:
  "and, on the TI models, how often the ROM has read a key"). Open
  point: whether a single kind of match (scan-all or test-row) is the
  right count is part of T59-0.
- **D-4 (AC-17, AC-38).** `key` and `type` reply through a new
  `Deferred` outcome of LIB-3's commands, not as operations.
- **D-5 (AC-17, EC-3).** The harness gives up on a `key` or `type`
  after one hour of emulated time, as `wait` does, and replies "not
  done". A TI program that never comes back to the scan loop would
  otherwise hang the harness.
- **D-6 (AC-28 and AC-31 read together).** "Live input is not held back
  for key readiness on any Sharp model" is read as: a live key that
  finds nothing waiting goes down at once (E2). "On the PC-1500 a press
  that waits goes down at key readiness" is read as: a press queued
  behind another (a sequence, typed text) waits for readiness there.
  On the PC-1600 queued presses follow the cadence only, as its paste
  feeder does today. A `type` on the PC-1500 therefore starts at once
  when the machine is ready (AC-14 "at once") and each later character
  at readiness.
- **D-7 (readiness poll).** TI 1.5 ms of emulated time (today's
  `kScanPoll` of wall time); Sharp one frame (the typers look per frame
  today).
- **D-8 (AC-16).** On a TI model `type` is refused as an unknown
  command ("Unknown command: type.", LIB-3 AC-13); `TypingPart` does not
  declare it for a profile without characters. The harness lists it
  only for such models.
- **D-9 (AC-9 and a deferred reply).** A `key` or `type` stopped by live
  input replies "not done", reason "Stopped by a key.".
- **D-10 (Sharp minimum hold).** 4 frames for every press, live input
  too (the 2-frame minimum of `SharpTypingHook` was replaced by the
  user's ruling of 2026-11-13, decision log row "Sharp presses as
  today"); PC-1600 shifted host keys get the 4 + 6 + 4 frames of the
  profile instead of today's 30 ms, 100 ms, 30 ms
  (`SharpMachineParts.cpp:31-32`). Not visible; no ledger row.
- **D-11 (EC-5 holds).** `drop()` (LIB-18's seam) releases at once, without the minimum:
  the machine starts over and cannot take the key anyway.
- **D-12 (frontends' wall-time rule).** The SHIFT tap (AC-33) stays in
  the frontends and is sent as one screen press of `shift`; no wall
  time reaches the engine.
- **D-13 (the key trace).** Until M-53 the manual trace still records
  keys, now from the engine's observer (`TIKeyTraceObserver`); row
  59-19 drops them in M-53.
- **D-14 (for LIB-6's read check).** `charKeys(c, nullptr)` answers as
  for latches all off and is `nullopt` only for a character without a
  key, so a preset is refused at read for exactly the characters that
  cannot be typed (LIB-6 §3.10). The first press "at once" after
  `skip-wait:` is `TypeOptions::firstPressAtOnce` (§3.6).
