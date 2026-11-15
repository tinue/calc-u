# LIB-3 Sessions, operations and messages: design
Status: approved by the user at the Part B/C1 boundary (2026-11-14)

## 1. Scope

| Column | Step | Tests from (plan §3) | Done in (plan §5.1) |
|---|---|---|---|
| Library | M-22 | AC-1–27, AC-39; EC-1–6 | M-22 |
| Calc-U-1600 | M-31 (with LIB-17 1600, LIB-15 1600) | AC-34–38 | M-49 |
| Calc-U-59 | M-33 (with C59-5, LIB-17 59), M-34 (with LIB-15 59) | M-33: AC-28–30, AC-32, AC-33; M-34: AC-31 | M-58 |

- M-22 waits for M-18 (LIB-1 L) and M-21 (LIB-17 L). It changes only
  `library/`. No product code calls the new code before M-31.
- M-31 waits for M-10 (FS 14), M-28, M-29, M-30. M-33 waits for M-16
  (FS 3, FS 4: both hold), M-3 (FS 12: C59-5 AC-8, AC-9 stand), M-10,
  M-15, M-28, M-32. M-34 waits for M-29, M-33.
- The product columns are Done later (§5.1) because AC-29, AC-30 and
  AC-38 map today's change flags and callbacks onto topics of LIB-4, 7,
  8, 10, 18, C59-3, C59-4, C59-7. Until those steps the topics travel as
  interim product topics (§5). Their tests are written in M-31 / M-33
  against the interim names and re-pointed in the owner's step. Last
  owners: LIB-4 1600 (M-49) and C59-8 (M-58).
- The Swift part of the 59 column is C59-5 (M-33). The Web parts are
  LIB-15 (M-31 for 1600, M-34 for 59). This design gives them the
  session, the messages and the host seam. It does not design their
  threads, the bridge or the worker.
- Not here: typing (LIB-5), the operations themselves (LIB-6, 7, 13,
  18), the control state (LIB-4), paper (LIB-8), run state and the
  debugger topics (LIB-10), "ROMs missing" (LIB-12), "Loading…" as drawn
  (LIB-14, C59-8), the harness commands (LIB-11).

## 2. Modules and files

### Library (M-22): `library/sessions/`, namespace `calcu::sessions`

The folder of `calcu_library` that LIB-1 §2a expects. It uses `machine`
and `pacing`. Its tests are compiled into `calcu_library_tests`.

| File | Holds |
|---|---|
| `Value.hpp/.cpp` | `Value`: a JSON-like tree (null, bool, int64, double, string, bytes, array, object) for command arguments and for translating payloads (§3.1) |
| `Payload.hpp` | `Payload`, `PayloadKind`, `payloadAs<P>` (§3.1) |
| `Messages.hpp` | `TopicName`, `CommandName`, `RequestName`, `CallId`, `ListenerId`, `Command`, `Request`, `Message` and its alternatives, `MessageSink` (§3.2) |
| `HostServices.hpp` | `HostService`, `HostServices`, `ServiceAsk`, `ServiceAnswer` (§3.6) |
| `Topics.hpp/.cpp` | `StateTopic`, `StreamTopic`, `History`, the flush rule (§3.3) |
| `LibraryTopics.hpp/.cpp` | the payloads `DisplayState`, `BusyState`, `Notice`, `LogLine`, `EmulatedTimeReply`, `MachineOffer`; topic, command and request names of this feature (§3.4) |
| `Operation.hpp/.cpp` | `Task<T>`, `Job`, `OperationResult`, `OperationContext`, `run()` (§3.5). The coroutine part comes from `products/calc-u-1600/core/Job.hpp` |
| `MachineSession.hpp/.cpp` | `MachineSession`: the machine, its `calcu::pacing::Pacer`, its input queue (`SliceHook`), `WaitProbe`; implements `calcu::pacing::SliceRunner` (§3.7) |
| `SessionPart.hpp` | `SessionPart`, `SessionSetup`, `SessionContext`: what a library feature or a product's app layer plugs in (§3.8) |
| `AppSession.hpp/.cpp` | `AppSession`, `AppSessionSetup`, `Hosting` (§3.9) |
| `TopicMirror.hpp/.cpp` | `TopicMirror`: a `MessageSink` that keeps the last message of each state topic and what each stream topic appended. It serves the test frontend, the Qt6 frontends and the harness (§3.10) |
| `tests/TestFrontend.hpp` | the test frontend: a `TopicMirror`, the raw message log, scripted host-service answers, two delivery modes (at once, through a queue) |
| `tests/TestParts.hpp` | a test product part: one product topic of each shape, one product command that starts an operation, one that does not, one product request |
| `tests/ScriptedClock.hpp` | a wall clock the test advances (if LIB-17's tests already have one, reuse it) |
| `tests/*_tests.cpp` | one file per area (§6) |
| `library/CMakeLists.txt` (changed) | the sources of `library/sessions/` and its tests added to `calcu_library` and `calcu_library_tests`; the entries in `docs/test-groups.md` |

### Calc-U-1600 (M-31)

| File | Change |
|---|---|
| `products/calc-u-1600/app/SharpAppPart.hpp/.cpp` (new) | `shared::SharpAppPart : calcu::sessions::SessionPart`. It holds what `AppSession` holds besides the session: the bays, `ControlState`, the Sharp operations as product commands, the per-call duties (`frameDuties`) |
| `products/calc-u-1600/app/SharpMachineParts.hpp/.cpp` (new) | the adapters around the machine session: `SharpTypingHook`, `SharpDebuggerGate` (§5) |
| `products/calc-u-1600/app/AppSession.*`, `MachineSession.*` | deleted. Their flows go into `SharpAppPart` and the adapters. The ~60 places of M-26's adapter for `MachineSession.cpp` now hang off `SharpAppPart` |
| `products/calc-u-1600/app/AudioPump.*` | sink handling moves into the session (§4, D-5). `AudioSink` stays as the frontend side |
| `products/calc-u-1600/core/Job.hpp` | becomes a forwarding header onto `calcu::sessions` (adapter `JobForward`, §5) |
| `products/calc-u-1600/frontends/qt6/app/MachineController.*` | hosts `calcu::sessions::AppSession` on the GUI thread, is its listener (a `TopicMirror` plus signals), and sends commands. The `Platform` overrides go |
| `products/calc-u-1600/frontends/qt6/app/SyncOperations.*` | deleted (`runNow` goes). The "Loading…" popup is shown from the busy topic (§4) |
| `products/calc-u-1600/frontends/qt6/app/EmulationPacer.*` | stays as the 60 Hz `QTimer` that calls `AppSession::call(now)` |
| `products/calc-u-1600/frontends/qt6/app/AudioOutput.*` | the audio sink, fed from the sound messages |
| `products/calc-u-1600/frontends/web/bindings.cpp`, `calc-engine-worker.js`, `src/engine.js` | translate messages only (LIB-15 1600) |
| `products/calc-u-1600/tools/wasm_smoke.cpp` | drives the library session in virtual-time hosting |
| `products/calc-u-1600/app/tests/shared_app_session_tests.cpp`, `shared_machine_session_tests.cpp`, `shared_bay_tests.cpp` | ported to the library session with a `TopicMirror`. `test_run_now_and_selection` becomes a test in parts |
| `products/calc-u-1600/tests/session_tests.cpp` (new) | AC-34–38 (§6) |

### Calc-U-59 (M-33, M-34)

| File | Change |
|---|---|
| `products/calc-u-59/app/TIAppPart.hpp/.cpp` (new, M-33) | `cu59::TIAppPart : calcu::sessions::SessionPart`. The flows of `SessionController` (cards, cue card, printer strip, presets, freeze and step, traces, ASM overlay, LOG tab, TI-58C persist) as product commands, product topics and per-call duties |
| `products/calc-u-59/app/TIMachineParts.hpp/.cpp` (new, M-33) | `TIKeySequencerHook` (the `KeySequencer` lanes as a `SliceHook`), `TIFreezeGate`, `TIPresetOperation` (§5) |
| `products/calc-u-59/app/TIFileAccess.hpp/.cpp` (new, M-33) | the card and constant-memory file I/O that `Platform` did, on the save folder of LIB-9 (§5) |
| `products/calc-u-59/app/EmulationRunner.*`, `Pacing.*` | deleted in M-33 (LIB-17 design) |
| `products/calc-u-59/app/Threading.*` | `Threading::Threads` goes in M-33. The file goes in M-34 with the last cooperative user |
| `products/calc-u-59/app/SessionController.*`, `EmulatorSession.*` | stay for the Web frontend and its tests from M-33 to M-34 (plan M-33), deleted in M-34 |
| `products/calc-u-59/frontends/qt6/app/EmulatorController.*` | M-33: hosts the library session on the GUI thread, listener and commands; `Platform` goes |
| `products/calc-u-59/frontends/swift/Bridge/TISessionController.*`, `App/EmulatorViewModel.swift` | M-33, C59-5's design: the bridge carries `Message`s and `Command`s |
| `products/calc-u-59/frontends/web/bindings.cpp`, `calc-engine-worker.js`, `src/engine.js` | M-34, LIB-15's design |
| `products/calc-u-59/tests/session_tests.cpp` (new, M-33) | AC-28–30, AC-32, AC-33 (§6) |
| `products/calc-u-59/tests/FakePlatform.hpp`, `shared_controller_tests.cpp`, `shared_cooperative_tests.cpp` | M-34: ported to `TIAppPart` on the library session, or replaced by `session_tests.cpp` and the library tests; then deleted |

## 3. Seams and interfaces

These are signature sketches. Every class is single-threaded. It is
used on the thread that the host chose, holds no lock, starts no thread
and throws nothing across its interface (LIB-1 §2a; ADR 0009;
`frontend.md` I-1).

### 3.1 Payloads and values (AC-5, AC-12; I-3, I-4)

```cpp
namespace calcu::sessions {
struct PayloadKind { std::string_view value; auto operator<=>(const PayloadKind&) const = default; };
class Payload {                                   // immutable once sent; shared, never copied
public:
  virtual ~Payload() = default;
  virtual PayloadKind kind() const = 0;           // "display", "busy", "c59.cue-card", ...
  virtual bool equals(const Payload&) const = 0;  // AC-5, EC-2
  virtual Value toValue() const = 0;              // for translators only (bridge, bindings, harness --json)
};
template <class P> const P* payloadAs(const Payload&);   // P::kKind compared, then static_cast (no RTTI)
using PayloadPtr = std::shared_ptr<const Payload>;
}
```

- The library carries a product payload without reading it (AC-12). It
  calls only `equals` (to decide whether to send) and, in a translating
  host, `toValue`.
- An in-process frontend (Qt6) uses `payloadAs<P>`. A translating host
  (the Swift bridge, the Web bindings, the harness) uses `toValue()`.
  The translation is one function per kind, so no rule lives in it
  (I-3).
- A payload holds no pointer into a machine. Names may view into the
  catalogue's descriptors, which live as long as the product (LIB-1
  §3.4).

### 3.2 Messages (AC-4, AC-5, AC-7, AC-8, AC-13)

```cpp
using TopicName   = machine::Id<struct TopicTag>;     // "display", "busy", "notices", "log", "c1600.inspector"
using CommandName = machine::Id<struct CommandTag>;
using RequestName = machine::Id<struct RequestTag>;
using ListenerId  = std::uint32_t;
using CallId      = std::uint64_t;                     // per session, increasing

struct Command { CommandName name; Value args; };
struct Request { RequestName name; Value args; };

struct StateMessage  { TopicName topic; PayloadPtr state; };
struct StreamMessage { TopicName topic; bool reset; std::vector<PayloadPtr> appended; };   // AC-7
struct ReplyMessage  { CallId call; bool done; std::string reason; PayloadPtr result; };   // AC-8, AC-13
struct ServiceAsk    { CallId ask; HostService service; Value args; };                     // AC-27
struct SoundStart    { int sampleRate; };  struct SoundSamples { std::vector<std::int16_t> mono; };  struct SoundStop {};
using Message = std::variant<StateMessage, StreamMessage, ReplyMessage, ServiceAsk, SoundStart, SoundSamples, SoundStop>;

class MessageSink { public: virtual void deliver(const Message&) = 0; protected: ~MessageSink() = default; };
```

- The session builds an ordered outbox. It hands the outbox to each
  listener's `deliver` in order, on the session's thread. Whether the
  sink acts at once (Qt6, tests) or queues for another thread (Swift,
  the worker) is up to the sink. The order and the content are the same
  either way (AC-4). The library has no thread-safe queue: that queue is
  the host's (C59-5, LIB-15).
- Order within one flush: state topics in the order they were declared,
  then stream topics in that order, then the replies. A frontend
  therefore has the new state when the reply arrives.
- A reply goes only to the listener that sent the command or request
  (C59-5 AC-12). Topics go to every listener.

### 3.3 Topics (AC-5, AC-6, AC-7, EC-1, EC-2; OQ-3, OQ-5 of the spec)

```cpp
class StateTopic  { public: void set(PayloadPtr); const PayloadPtr& lastSent() const; };
enum class History { None, SinceReset };              // notices: None; log, paper: SinceReset (spec OQ-5)
class StreamTopic { public: void append(PayloadPtr); void reset(); };
```

- `set` only stores. At a flush, a state topic is sent if its stored
  state is not `equals` the last one sent (AC-5). Several sets in one
  host call send the last one (EC-1). A change back sends nothing
  (EC-2).
- A flush happens at the end of each host call, and at the end of each
  command and each service answer (AC-5, spec OQ-3). Machine output is
  read only in host calls: the display, the sound and what the parts
  read in `afterCall`.
- When a frontend starts to listen (AC-6), it is sent the last-sent
  state of every state topic that has one, then, for each `SinceReset`
  stream, one `StreamMessage{reset = true, all since the last reset}`.
  Notices are not sent (spec OQ-5).
- A stream that is reset and appended to in one flush sends one message
  with `reset = true` and what came after the reset.

### 3.4 This feature's topics, commands and requests (AC-9 to AC-11, AC-21, AC-24, AC-39)

| Name | Shape | Payload |
|---|---|---|
| `display` | state | `DisplayState{machine::DisplayOutput}`. The machine session reads it once per host call that ran the machine (`readDisplay` has one reader, LIB-1 §3.5) |
| `busy` | state | `BusyState{bool operationRuns; pacing::Speed speed;}`: the speed now, with or without an operation (AC-9, AC-14; spec OQ-8) |
| `notices` | stream, `History::None` | `Notice{std::string sentence}` (AC-10) |
| `log` | stream, `History::SinceReset` | `LogLine{std::string text}` (AC-11) |
| command `suspend`, `continue` | — | AC-24, AC-25 |
| command `speed` `{full: bool}` | — | AC-21. While an operation sets its own speed, the reply is done and nothing changes (D-6) |
| command `key-down`, `key-up` `{key}` | — | the raw machine call at the current tick count. Refused for an unknown key (LIB-1 EC-2). LIB-11 AC-9 uses them; LIB-5 puts live input in front of them (§9 A-3) |
| request `ticks` | — | `EmulatedTimeReply{Ticks ticks; double seconds;}` (AC-39) |
| request `capabilities` | — | `MachineOffer{std::vector<machine::CapabilityEntry>; machine::Plugged; std::vector<machine::PlaceName> places;}` (AC-39) |
| request `topics` | — | `TopicList{std::vector<{TopicName name; bool stream;}>}`: every topic the parts declared, in declaration order, whether sent yet or not (A-8; LIB-11 `state <topic>`, LIB-11 EC-5) |

Without a machine, `ticks` and `capabilities` are refused with the reason "No
machine is running.".

### 3.5 Operations (AC-14 to AC-19, EC-3, EC-4, EC-6)

```cpp
struct OperationResult { bool ok = true; std::string message; };     // message: an English sentence
template <class T = void> class Task;                                  // as products/calc-u-1600/core/Job.hpp
class OperationContext {                                               // what a running operation may do
public:
  detail::RunAwaiter run(machine::Ticks ticks);   // co_await: the machine runs `ticks` at the operation's speed
  void setSpeed(pacing::Speed);                   // AC-14: a stretch at regular speed, paced as usual
  void setReportsWaiting(bool);                   // LIB-17 AC-16: a preset `wait:` at full speed reports "not waiting"
  SessionContext& session();                      // machine, topics, notices, log (§3.8)
};
using Operation = std::function<Task<OperationResult>(OperationContext&)>;
```

- **Runs are not split.** `run(n)` issues requests of at most
  `kRunPiece` = 1/60 s of emulated time each, decided by the
  operation's own code. The session executes each request whole. So
  the machine gets the same calls in the same order however the host
  slices the operation, and AC-19 holds by construction (today's
  equivalence rule, `core/Job.hpp:20-24`). The piece keeps a part
  within the call allowance and lets the display update between parts
  (I-5).
- **One part per host call.** While an operation runs, `call()` paces
  the machine session at the operation's speed, and `runSlice` takes
  its ticks from the operation's pending requests instead of running
  the machine freely (§3.7). At full speed the pacer stops at
  `kFullSpeedAllowance` (LIB-17 R4, 14 ms), which is the allowance of
  AC-14. At regular speed it paces as usual (R2/R3), and an overshoot
  of up to one piece is carried (LIB-17 AC-3). The operation ending
  inside a slice is reported as `stoppedEarly` and ends the call.
- **Nesting (AC-17).** An operation awaits another `Task` directly.
  Only a command can start an operation, so nothing nested is ever
  refused.
- **Ending.** The result becomes the reply to the starting command,
  the busy topic goes to `operationRuns = false`, a failure is also
  appended to `notices`, and the machine session starts afresh (AC-18,
  AC-23, EC-3). An exception from the coroutine is caught inside the
  session and becomes `ok = false` with its `what()`; nothing escapes.
- **Stopping (LIB-18 AC-16).** A command declared `endsRunningOperation`
  first destroys the running `Job` and replies to its command with
  `done = false`, reason "Stopped." (D-7), then runs.
- **Virtual-time hosting (AC-19; LIB-11 AC-6, AC-7).** With
  `Hosting::VirtualTime`, a command that starts an operation runs it to
  its end inside `send()` with no pacer and no wall clock, then replies.

### 3.6 Host services (AC-20, AC-26, AC-27)

```cpp
enum class HostService { AudioSink, ClipboardText, ClipboardImage, FilePicker, FolderPicker, KeepFile, OpenUrl, Haptics };
struct HostServices { std::set<HostService> offered; bool recordsSound = false; };   // recordsSound: the harness (AC-20)
struct ServiceAnswer { bool given; Value result; std::string reason; };
// SessionContext:
std::optional<CallId> askHost(HostService, Value args, std::function<void(const ServiceAnswer&)> then);  // nullopt: nobody offers it
```

- The services are stated with `listen()` (AC-26). An ask goes to the
  first listener that offers that service, which is the app's own
  frontend, never an attached harness (C59-5 AC-12). A service nobody
  offers is not asked: `askHost` returns `nullopt` (AC-27). An ask does
  not make the session busy, and the machine goes on running. `then`
  runs when `answer()` arrives, followed by a flush.
- **Sound.** The listener that offers `AudioSink` gets `SoundStart{rate}`
  with the rate the machine was built with (LIB-1 `BuildRequest::soundRate`).
  It then gets `SoundSamples` from host calls at regular speed outside an
  operation. The samples of a call with `PaceResult::ranUnpaced`, or
  while an operation runs, are drained and thrown away. A listener with
  `recordsSound` gets every sample whatever the speed. Without either,
  the samples are thrown away. Matching the device rate is the sink's
  (EC-5). Device open, close and priming are the session's: it sends
  `SoundStart` on the first non-silent chunk after silence, primes 40 ms,
  and sends `SoundStop` after 1 s of silence, as `AudioPump` does today
  (D-5). The carry-over cap stays the sink's, because the sink's `room()`
  cannot be read across a thread boundary.

### 3.7 Machine session (AC-1, AC-3, AC-21 to AC-25; LIB-17 `SliceRunner`)

```cpp
class SliceHook {          // the input queue: LIB-5's typing engine; today's typing behind adapters (§5)
public:
  virtual machine::Ticks nextDue(machine::Ticks now) const = 0;   // kNever when idle
  virtual void act(machine::Machine&, machine::Ticks now, pacing::WallTime due) = 0;
protected: ~SliceHook() = default;
};
class WaitProbe {          // AC-22: a media place that waits for a medium (LIB-7); TICardWaitAdapter until then
public: virtual bool waitsForMedium(const machine::Machine&) const = 0; protected: ~WaitProbe() = default;
};
class AdvanceGate {        // reserved for LIB-10's run control (pause, breakpoints); SharpDebuggerGate, TIFreezeGate until then
public: virtual machine::Ticks advance(machine::Machine&, machine::Ticks atLeast, bool& stoppedEarly) = 0;
protected: ~AdvanceGate() = default;
};

class MachineSession final : public pacing::SliceRunner {
public:
  MachineSession(pacing::WallClock, std::optional<std::uint32_t> jitterSeed);
  void setMachine(std::unique_ptr<machine::Machine>, const pacing::PacingProfile&, int soundRate);  // AC-23; Pacer::setMachine
  std::unique_ptr<machine::Machine> takeMachine();
  machine::Machine* machine();
  void addHook(SliceHook&);  void addWaitProbe(WaitProbe&);  void setGate(AdvanceGate*);
  void setSuspended(bool);   // AC-24; false → startAfresh()
  void setPaused(bool);      // reserved for LIB-10 (AC-25); false → startAfresh() (AC-23)
  bool runs() const;         // machine && !suspended && !paused
  pacing::PaceResult pace(pacing::Speed);          // one host call: nothing when !runs() (AC-3, AC-24, AC-25)
  machine::Ticks advanceUnpaced(machine::Ticks);   // virtual-time hosts: the harness `wait`, tests
  pacing::SliceResult runSlice(const pacing::SliceRequest&) override;   // LIB-17
  void startAfresh();        // Pacer::startAfresh (AC-23)
};
```

`runSlice` does the following:

1. It takes the ticks from the running operation's requests, if there
   is one, else from the machine itself. In both cases it advances in
   pieces cut at each hook's `nextDue`, calls `act`, and goes on until
   at least `req.ticks` ran. The advance goes through the gate when one
   is set.
2. It returns `ticksRun`, `stoppedEarly` (the gate stopped, or the
   operation ended), and `waitsForInput`. That is
   `machine.isKeyReady() || any WaitProbe`, unless the operation called
   `setReportsWaiting(false)` (AC-22; LIB-17 AC-16).

The machine session owns the pacer (LIB-17 §3). It tells the pacer to
start afresh at creation, at continue, at the end of an operation, at a
new machine (`setMachine`, with the profile) and at a debugger resume
(AC-23). Suspend and pause are two separate flags. Neither clears the
other, so AC-25 holds by construction.

### 3.8 Session parts (L-1; AC-12; FE-6)

```cpp
class SessionSetup {     // at attach: declare what a part brings
public:
  SessionContext& context();    // kept by the part; its slice hook, gate, probe go on context().machineSession() here (C1 simplification)
  StateTopic&  stateTopic(TopicName);
  StreamTopic& streamTopic(TopicName, History);
  using Handler = std::function<CommandOutcome(const Value& args, SessionContext&)>;
  void command(CommandName, Handler, bool endsRunningOperation = false);
  void request(RequestName, std::function<ReplyMessage(const Value& args, SessionContext&)>);
};
struct Done { PayloadPtr result; };  struct Refused { std::string reason; };
struct StartOperation { Operation op; };
using CommandOutcome = std::variant<Done, Refused, StartOperation>;   // AC-13, AC-16

class SessionContext {   // what parts and handlers see, on the session's thread
public:
  MachineSession& machineSession();  machine::Machine* machine();
  void notice(std::string sentence);  void log(std::string line);
  bool busy() const;
  std::optional<CallId> askHost(HostService, Value, std::function<void(const ServiceAnswer&)>);
};

class SessionPart {      // a library feature's part (LIB-2, 4, 7, 8, 10, 12, 18) or a product's app layer
public:
  virtual void attach(SessionSetup&) = 0;   // may set the first machine: the session syncs it before its first flush
  virtual void beforeCall(SessionContext&, pacing::WallTime) {}   // e.g. LIB-2's host link queue drained
  virtual void afterCall(SessionContext&, pacing::WallTime) {}    // per-call duties: bays, debug model, 2 Hz checks
  virtual void machineChanged(SessionContext&) {}
  virtual void suspending(SessionContext&) {}                      // e.g. TI-58C persist (SessionController::suspend)
protected: ~SessionPart() = default;
};
```

- A command handler that returns `StartOperation` while an operation
  runs is refused by the session with "Another operation is running."
  (D-7). The session never reads which command it was, so the
  same rule holds for product commands (AC-16). Everything else
  (`Done`/`Refused`, requests) is carried out as usual (EC-6).
- An unknown command or request is refused with "Unknown command: <name>."
  and changes nothing (AC-13).
- Names: a product's names carry its prefix (`c59.`, `c1600.`), as
  product kinds do (LIB-1 §3.7). That prefix is what makes a name a
  product's. A library frontend part ignores a topic it does not know
  (AC-12).

### 3.9 App session (AC-1, AC-2, AC-4, AC-6, AC-26)

```cpp
enum class Hosting { Paced, VirtualTime };                 // VirtualTime: harness, AC-19 tests
struct AppSessionSetup {
  pacing::WallClock clock;                                 // the host's; the pacer reads it within a call
  std::function<std::chrono::system_clock::time_point()> hostTime;  // A-7: local date and time in the products, a fixed instant in the harness; sets the real-time clocks at build and after each operation (LIB-2 AC-23)
  std::vector<SessionPart*> parts;                         // library features first, then the product's
  Hosting hosting = Hosting::Paced;
  std::optional<std::uint32_t> jitterSeed;                 // LIB-17 AC-10
  std::shared_ptr<void> owned;                             // the parts and what they need: released first, while the machine is there
};
template <class Part> struct ProductSession { std::unique_ptr<AppSession> session; Part* part; };   // a product factory's result
class AppSession {
public:
  explicit AppSession(AppSessionSetup);
  ListenerId listen(MessageSink&, HostServices);           // AC-6, AC-26
  void stopListening(ListenerId);
  void call(pacing::WallTime now);                         // AC-2; Paced only
  CallId send(ListenerId, Command);                        // carried out at once (spec OQ-3)
  CallId ask(ListenerId, Request);
  void answer(CallId ask, ServiceAnswer);                  // AC-27
  machine::Ticks advanceUnpaced(machine::Ticks);           // VirtualTime only (LIB-11 `wait`)
  SessionContext& context();                               // a host's own log line, its checks, tests
};
```

`call(now)` does the following, in order:

1. Nothing at all when suspended (AC-24, EC-4).
2. `beforeCall` of each part.
3. One part of the running operation, or `pace(speed last commanded)`.
4. Read the display if ticks ran.
5. `afterCall(now)` of each part.
6. Sound (§3.6).
7. Flush.

`now` is for the parts' own timers (for example the 2 Hz cue-card
check in `SessionController::tick`). The pacer reads the host's clock
itself (LIB-17 §3).

One window gets one app session (AC-1). Each frontend creates it and
destroys it with the window. The session owns the parts a product's
factory made (`owned`) and lets them go first, while its machine is still
there (Calc-U-1600's part saves its bays and tape into the machine's
files); no part needs telling that the session went.

### 3.10 Topic mirror and listener (AC-6, AC-29, AC-38)

```cpp
class TopicMirror : public MessageSink {
public:
  void deliver(const Message&) override;
  const PayloadPtr* state(const TopicName&) const;                 // last of a state topic
  template <class P> const P* stateOf(const TopicName&) const;     // the same, as P
  std::vector<PayloadPtr> takeAppended(const TopicName&);          // what a stream appended since the last take
  std::function<void(const TopicName&)> onChange;                  // a Qt frontend turns this into its repaint signals
};

class Listener : public MessageSink {   // every host's side of a session (C1 simplification)
public:
  void listen(AppSession&, HostServices = {});  void stop();
  struct Sent { CallId call; std::optional<ReplyMessage> reply; };  // reply: it came at once
  Sent send(const CommandName&, Value args = {}, ReplyHandler then = {});
  void onReply(CallId, ReplyHandler);                              // for a reply still to come
  ReplyMessage runToEnd(const CommandName&, Value, const std::function<void()>& step);
  ReplyMessage ask(const RequestName&, Value = {});
  CallId ask(const RequestName&, Value, ReplyHandler then);
  TopicMirror& mirror();  template <class P> const P* state(const TopicName&) const;
  const machine::DisplayOutput* display() const;
  std::function<void(const Message&)> onMessage;                   // what went to no reply handler
};
```

The messages that arrive inside `send()` or `ask()` are taken once the
call has returned, in the session's order, so the call's id is known; a
reply handler gets its reply where it comes among them. The Qt6
frontends, the Swift session thread, the Web worker's host, the harness
and the products' tests talk to the session through it.

## 4. Product side

### Calc-U-1600 (M-31)

| Today | After M-31 |
|---|---|
| `shared::AppSession` (`app/AppSession.hpp:46-224`) | `calcu::sessions::AppSession` plus `shared::SharpAppPart` |
| `shared::MachineSession` (`app/MachineSession.hpp:48-443`) | `calcu::sessions::MachineSession`. Its typing goes to `SharpTypingHook`, its debugger slice to `SharpDebuggerGate`, its media and peripheral calls to M-26's adapters held by `SharpAppPart` |
| `AppSession::start(op, done)` and `tick()`'s operation branch (`app/AppSession.cpp:94-156`) | `StartOperation` and §3.5. `kOperationSliceSeconds` (10 ms) is replaced by LIB-17's 14 ms allowance (D-4) |
| `AppSession::runNow` (`:120-134`), `SyncOperations` (`frontends/qt6/app/SyncOperations.cpp:33-79`) | deleted (SP-2). Qt6 shows its "Loading…" `QProgressDialog` from the busy topic: `operationRuns && speed == Full` for 500 ms, as today's `kPopupDelayMs`. The cursor stays a wait cursor until LIB-14 1600 (M-49) |
| `Job::step(cyclesPerStep)` with `clockHz/1000` budgets (`AppSession.cpp:102-109`) | whole requests of at most `kRunPiece` (§3.5). The Sharp jobs keep their `co_await job::run(...)` through `JobForward` |
| `endOperation`: `seedClockFromHost`, `m_pacer.restart()` (`AppSession.cpp:86-92`) | the session's end of operation (AC-18, AC-23). The real-time clock seed is LIB-2's (`syncclock`); until LIB-2 1600 it is `SharpAppPart`'s `afterOperation` duty |
| `frameDuties()` (`AppSession.cpp:180-186`) | `SharpAppPart::afterCall` (bays, tape status) |
| `AudioPump` (`app/AudioPump.cpp`), `tick()`'s discard rule | the discard rule and the open, prime and silence rules are the session's (§3.6, D-5) |
| `Platform` callbacks (`AppSession.hpp:62-84`, `MachineSession.hpp:69-93`) | AC-38's table: `notice`, `bayError`, `attachFailed` → `notices` or the reply; `presetLog` → `log`; `busyChanged` → `busy`; `machineReplaced`, `modelSwitched`, `moduleChanged`, `tapesChanged`, `presetArmed` → interim product topics (§5); `romMissing` → LIB-12's message (done in M-27); `hostDirectories` → `HostService::FolderPicker` stated by the frontend; `serialLinksRelinked` → a notice until LIB-2 1600 decides (M-47); `machineAboutToChange`, `runDebuggerSlice` → no message (`SharpDebuggerGate`) |
| `ControlState controlState()` (`AppSession.hpp:113`) | interim product topic `c1600.control-state` until LIB-4 1600 (M-49) |
| Web `pause` / `resume` on `visibilitychange` (`frontends/web/src/engine.js:180`) | the `suspend` / `continue` commands (AC-36; LIB-15 1600) |

Pacing: the Sharp profiles come from LIB-17's `shared::pacingProfile(model)`.
They are handed in with `setMachine`.

### Calc-U-59 (M-33 Qt6 and Swift, M-34 Web)

| Today | After M-33 (Qt6, Swift) |
|---|---|
| `SessionController` (`app/SessionController.hpp:29-355`) | `calcu::sessions::AppSession` plus `cu59::TIAppPart`. The getters go; the frontends read a `TopicMirror` |
| `EmulatorSession` (`app/EmulatorSession.hpp:70-240`) | `calcu::sessions::MachineSession` with C59-1's TI machine. `FreezeController` → `TIFreezeGate`; `KeySequencer` (cooperative lanes only) → `TIKeySequencerHook`; `FrameSampler` → `TIAppPart::afterCall`; the trace writers → `TIAppPart` (LIB-10, C59-4 later) |
| `EmulationRunner`, threaded `KeySequencer` lanes, `m_stateMutex`, `m_cardMutex`, `m_traceMutex`, the atomics (`EmulatorSession.hpp:222-238`) | gone (AC-28; ADR 0009) |
| `SessionController::tick(now)` (`SessionController.cpp:270-326`) | `AppSession::call(now)`; the 2 Hz, 1 Hz and 0.5 s timers in `TIAppPart::afterCall(now)` |
| `suspend()` / `resumeAfterSuspend()` (`:209-218`) | the `suspend` / `continue` commands; the persist moves to `TIAppPart::suspending`. Swift sends them (C59-5 AC-8), Qt6 does not (AC-31) |
| `loadStateFile`, KEYSTROKES playback | `TIPresetOperation`, at the speed the preset asks for (regular unless `FullSpeed:`), as in 2.3.0. Full speed by default is LIB-6 59 (M-54; ledger 59-5) |
| `togglePrinterConnection`, `changeModule` → reset (`:368`, `:239`); from M-25 attach/detach/replace and `Machine::powerCycle()` (C59-6, LIB-2 §5) | operations (AC-32) until LIB-18 59 takes them (M-54) |
| `Platform` (`SessionController.hpp:57-90`) | AC-30: `error` → `notices`; `diagnostic` → `log`; `showDebugPanel` → product topic `c59.show-debug-panel` (stream, `History::None`); the file calls → `TIFileAccess`; `traceLocation` → LIB-9 `Paths` (traces); `saveLastUsedModel` → LIB-9 setting (done in M-32); `saveModuleID` → gone (59-9); `runOnUiThread` → none |

In M-34 the Web frontend moves onto the same session, through LIB-15.
`SessionController`, `EmulatorSession`, `CooperativePacing` and
`Threading.*` go then.

## 5. Adapters

| Adapter | File | Wraps | Removed in |
|---|---|---|---|
| `JobForward` | `products/calc-u-1600/core/Job.hpp` | `namespace job` as aliases of `calcu::sessions::Task`/`Job`/`run` for the core's Sharp jobs (`PresetRunner`, the typers, the preset loaders) | when the last core user moves: the typers with LIB-5 (M-36), `PresetRunner` with LIB-6 1600 (M-46). At the latest M-46 |
| `SharpTypingHook` | `products/calc-u-1600/app/SharpMachineParts.*` | `KeyPaste`, `fireDueKeys`, the 60 Hz paste frames of `MachineSession::advance` (`app/MachineSession.cpp:461-478`), `hostKeyDown` | M-36 (LIB-5) |
| `SharpDebuggerGate` | same | `Listener::runDebuggerSlice` and `machineAboutToChange` towards the Qt `DebugController` (same thread, called directly) | M-45 (LIB-10 1600) |
| interim topics 1600 | `SharpAppPart` | `c1600.control-state` (LIB-4, M-49); `c1600.machine` for `machineReplaced`/`modelSwitched`/`moduleChanged` (LIB-4/LIB-18, M-49/M-46); `c1600.tapes` for `tapesChanged` (LIB-7, M-47); `presetArmed` as an extra flush inside the preset operation (LIB-6, M-46) | per owner, as given |
| Sharp operations as product commands | `SharpAppPart` | `loadPresetJob`, `resetJob`, `pickModelJob`, `select*RomJob`, `togglePeripheralJob`, `mountHostDriveJob` as `c1600.*` commands that return `StartOperation`. `c1600.reset` and `c1600.pick-model` are `endsRunningOperation` (LIB-18 AC-16; ledger 1600-9) | LIB-18 1600, LIB-6 1600 (M-46); LIB-7 1600 (M-47) |
| `TIKeySequencerHook` | `products/calc-u-59/app/TIMachineParts.*` | the `KeySequencer` cooperative lanes, advanced at each slice end with `SliceRequest::due` (LIB-17 §5) | M-36 (LIB-5) and M-54 (LIB-6 59) |
| `TIFreezeGate` | same | `FreezeController`'s per-instruction freeze and breakpoint checks, `stepKeycode`/`stepInstruction`, behind C59-1's `TIDebugTargetAdapter` (`setRunControl(TIStepObserver*)`, `takeStop()`; reconciled) | M-53 (LIB-10 59) |
| `TIPresetOperation` | same | `applyState` plus playback until `playbackFinished`, as one operation | M-54 (LIB-6 59) |
| `TIFileAccess` | `products/calc-u-59/app/TIFileAccess.*` | `readConstantMemory`, `writeConstantMemory`, `writeCard`, `readCard`, `cardFilePath` of `Platform`, on the save folder of LIB-9, through a `cu59::FileIO` the host hands it (plain I/O; on Swift the bridge's coordinated I/O, C59-5 D-6). When the Web uses it from M-34 (LIB-15), cards are files in the Web's persistent save folder and survive a page load from M-34 on, before the M-55 of the plan and ledger row 59-8: LIB-15 §9 (was OQ-9) | constant memory: M-54 (C59-3); cards: M-55 (LIB-7 59, C59-7) |
| interim topics 59 | `TIAppPart` | `c59.model` (`ModelChanged`; LIB-4, M-58); `c59.printer` stream with reset (`PrinterChanged`, `PaperCut`; LIB-8, M-53); `c59.card-reader` (`CardChanged`; LIB-7/C59-7, M-55); `c59.run-state` (freeze, armed, held key; LIB-10 M-53, LIB-5 M-36); `c59.trace` (`TraceChanged`; LIB-10, M-53); `c59.debug.live/cpu/heatmap/log/asm/listing` (C59-4, M-56); `c59.cue-card` (C59-7, M-55) | per owner, as given |
| `SessionController` (cooperative only), `EmulatorSession` | `products/calc-u-59/app/` | the Web frontend and its tests, unchanged in behaviour (plan M-33) | M-34 |

## 6. Tasks

Library, M-22. The files are in `library/sessions/tests/`, the
program is `calcu_library_tests`, and every case is Frequent. The
tests use LIB-1's `test-seg` and `test-dot` (`library/machine/tests/TestMachines.hpp`):
`test-dot` has sound and a key that switches the display off, and
`test-seg` reaches key readiness after 50 ticks. They also use
`TestParts.hpp`, `TestFrontend.hpp` and `ScriptedClock.hpp`. The tests
are written before the code of each task.

| Task | What | ACs/ECs | Test (file: cases) |
|---|---|---|---|
| L1 | CMake, `Value`, `Payload`, names, `Message`, `MessageSink` | (support) | `value_tests.cpp`: `value_round_trip_every_alternative` |
| L2 | Topics and the flush | AC-5, AC-7, EC-1, EC-2 | `topic_tests.cpp`: `ac5_state_sent_only_when_it_differs`, `ac5_command_reply_and_changed_state_at_once`, `ac5_machine_output_only_with_host_calls`, `ac7_stream_sends_exactly_what_was_appended`, `ac7_stream_reset_marker`, `ec1_several_changes_send_the_last`, `ec2_change_and_back_sends_nothing` |
| L3 | `listen` and the late listener | AC-6 | `topic_tests.cpp`: `ac6_new_listener_gets_every_state_topic_once`, `ac6_late_listener_gets_log_since_reset_and_no_notices` |
| L4 | Commands, requests, replies | AC-8, AC-13, AC-39 | `command_tests.cpp`: `ac8_request_gets_exactly_one_reply`, `ac8_request_without_answer_says_why`, `ac13_command_done_or_refused`, `ac13_unknown_command_refused_changes_nothing`, `ac39_ticks_request_gives_ticks_and_seconds`, `ac39_capabilities_request_gives_kind_id_owner_places_plugged`, `topics_request_lists_declared_topics` |
| L5 | Product names, carried unread | AC-12 | `command_tests.cpp`: `ac12_product_topic_command_request_travel_like_generic`, `ac12_unknown_topic_ignored_by_mirror` |
| L6 | Library topics | AC-9, AC-10, AC-11 | `library_topics_tests.cpp`: `ac9_display_busy_notices_log_and_their_shapes`, `ac10_notice_is_the_sentence_as_written`, `ac11_log_lines_from_library_and_product_part` |
| L7 | `MachineSession`, `call`, one session per window | AC-1, AC-2, AC-3 | `host_call_tests.cpp`: `ac1_app_session_holds_one_machine_session`, `ac1_machine_replaced_inside_session`, `ac2_call_runs_machine_and_sends_changes`, `ac2_without_operation_runs_at_last_commanded_speed`, `ac3_no_call_no_run` |
| L8 | Delivery modes | AC-4 | `host_call_tests.cpp`: `ac4_at_once_and_queued_give_same_messages_in_same_order` (one script of commands and calls, two sessions, two sinks) |
| L9 | Operations: start, parts, busy, updates | AC-14, AC-15 | `operation_tests.cpp`: `ac14_busy_while_operation_runs`, `ac14_one_part_per_call_within_allowance`, `ac14_full_speed_unless_regular_stretch`, `ac14_busy_says_speed_now`, `ac15_changes_sent_at_end_of_call_during_operation` |
| L10 | Refusal and nesting | AC-16, AC-17, EC-6 | `operation_tests.cpp`: `ac16_second_operation_refused`, `ac16_product_command_operation_refused_unread`, `ac16_other_commands_go_through`, `ac16_ends_running_operation_command`, `ac17_nested_operation_not_refused`, `ec6_request_answered_while_busy` |
| L11 | End and failure | AC-18, EC-3 | `operation_tests.cpp`: `ac18_end_busy_reply_notice_start_afresh`, `ec3_failure_halfway_then_commands_accepted` |
| L12 | In one go vs in parts | AC-19 | `operation_tests.cpp`: `ac19_one_go_equals_parts` (same result, same display, then the same replies to the same key and advance calls) |
| L13 | Sound | AC-20, EC-5 | `sound_tests.cpp`: `ac20_sink_told_rate_and_gets_regular_speed_sound`, `ac20_full_speed_and_operation_sound_thrown_away`, `ac20_no_sink_thrown_away`, `ac20_recorder_gets_all`, `ec5_session_states_machine_rate_whatever_device` |
| L14 | Speed and pacing hand-offs | AC-21, AC-22, AC-23 | `speed_tests.cpp`: `ac21_speed_from_next_call`, `ac21_gesture_changes_nothing_while_operation_sets_speed`, `ac22_waits_for_input_from_key_readiness`, `ac22_waits_for_input_from_wait_probe`, `ac22_operation_reports_not_waiting`, `ac23_start_afresh_on_create_continue_end_new_machine_resume` (a counting stand-in for the pacer's hand-offs, or the observed absence of lag) |
| L15 | Suspend | AC-24, AC-25, EC-4 | `suspend_tests.cpp`: `ac24_suspended_no_run_continue_runs`, `ac24_session_never_suspends_itself`, `ac25_pause_survives_suspend_and_back`, `ac25_resume_while_suspended_does_not_run`, `ec4_operation_no_progress_while_suspended_busy_stays` |
| L16 | Host services | AC-26, AC-27 | `host_service_tests.cpp`: `ac26_listener_states_services`, `ac27_one_ask_one_answer_session_runs_meanwhile`, `ac27_not_busy_while_asking`, `ac27_service_not_offered_not_asked` |
| L17 | `TopicMirror` | (support for AC-29, AC-38) | `mirror_tests.cpp`: `mirror_keeps_last_state_and_appended` |

Coverage, Library: 28 of 28 ACs (AC-1–27, AC-39) and 6 of 6 ECs.

Products. The tests are written before the code of the step.

| Task | Step | What | ACs | Test (file: case; group) |
|---|---|---|---|---|
| P16-1 | M-31 | Qt6 hosts the library session; no session class | AC-34 | CTest `CalcU1600NoOwnSession` (a CMake script: no `AppSession`/`MachineSession` class in `products/calc-u-1600/app`, `frontends/qt6` includes `sessions/AppSession.hpp`); Frequent |
| P16-2 | M-31 | Operations in parts with live updates | AC-35 | `tests/session_tests.cpp`: `lib3_ac35_preset_in_parts_busy_display_and_paper_update` (an example preset on a scripted clock: busy over several calls, `display` sent during it, every call within the allowance); Frequent |
| P16-3 | M-31 | Web suspends when hidden, Qt6 does not | AC-36 | `frontends/web/tests/suspend.spec.js`: `hidden tab suspends, shown continues` (Longer); `CalcU1600NoOwnSession` also checks that `frontends/qt6` sends no `suspend` |
| P16-4 | M-31 | Buzzer only at regular speed | AC-37 | `session_tests.cpp`: `lib3_ac37_buzzer_heard_at_regular_speed_only` (port of `test_audio_follows_tick`); Frequent |
| P16-5 | M-31 | Callbacks become messages | AC-38 | `session_tests.cpp`: `lib3_ac38_mirror_holds_every_callback` (one case per table row, against the interim names, re-pointed in M-45 to M-49); Frequent |
| P59-1 | M-33 | Qt6 hosts the library session, no emulation thread | AC-28 | CTest `CalcU59NoOwnSession` (Qt6 and Swift sources use no `SessionController`/`EmulatorSession`/`EmulationRunner`; no `std::thread` in `products/calc-u-59/app`); Frequent. Swift: C59-5 AC-4 |
| P59-2 | M-33 | Change flags become topics | AC-29 | `tests/session_tests.cpp`: `lib3_ac29_mirror_holds_every_change_flag` (one case per row, interim names; re-pointed M-36 to M-58); Frequent |
| P59-3 | M-33 | `Platform` duties | AC-30 | `session_tests.cpp`: `lib3_ac30_error_is_notice_diagnostic_is_log`, `lib3_ac30_card_and_constant_memory_files_without_frontend`, `lib3_ac30_module_id_not_saved`, `lib3_ac30_show_debug_panel_is_product_message`; Frequent |
| P59-4 | M-33 | Preset and power cycle as operations | AC-32 | `session_tests.cpp`: `lib3_ac32_preset_load_is_busy_and_refuses_second`, `lib3_ac32_printer_toggle_is_operation`, `lib3_ac32_updates_during_preset`; Frequent |
| P59-5 | M-33 | No sound | AC-33 | `session_tests.cpp`: `lib3_ac33_no_sound_messages_and_no_sink_stated`; Frequent |
| P59-6 | M-34 | Suspend per frontend | AC-31 | `frontends/web/tests/suspend.spec.js` `hidden tab suspends, shown continues` (Longer); Swift: C59-5 AC-8's test; Qt6: `CalcU59NoOwnSession` checks no `suspend` is sent |

Coverage, products: 1600 has 5 of 5 (AC-34–38) and 59 has 6 of 6
(AC-28–33). In total 39 of 39 ACs and 6 of 6 ECs.

## 7. Tools and agent knowledge

**Today's tools.**
- M-22 moves none.
- M-31 moves `products/calc-u-1600/tools/wasm_smoke.cpp`, which today
  calls `AppSession::start(loadPresetJob…)`, onto the library session
  in `Hosting::VirtualTime`.
- `pc1500_cli`, `pc1600_cli`, the probes and Calc-U-59's
  `tools/headless/` do not use the sessions. They advance machines
  directly (M-19, M-26) and are unaffected until LIB-11 replaces them.

**Agent knowledge changed in the same step.**

| Step | Files |
|---|---|
| M-22 | `docs/test-groups.md` (the sessions cases of `calcu_library_tests`). No rename |
| M-31 | `products/calc-u-1600/docs/developer/Shared-Architecture.md`, `Web-Client.md`, `Cloud-Sessions.md`, `Debugger-Internals.md`, `PC1600-Serial-Port.md`, `Building.md`, `README.md` (`AppSession`, `MachineSession`, `runNow`, `Platform` → app session, machine session, frontend interface). Memory `calc-u-1600/project_shared_layer_wasm.md` and its line in `calc-u-1600-memories.md`. `docs/glossary-mapping.md`: the parenthesis for app session, machine session and frontend interface in the Calc-U-1600 column goes. `products/calc-u-1600/TODO.md` items that name them. `docs/background/*` is history and stays |
| M-33 | `products/calc-u-59/.claude/skills/calc-u-59-shared`, `-qt`, `-swift` `SKILL.md`; `reference/SharedArchitecture.md`, `AppArchitecture.md`, `DebugAPI.md`, `FrontendParity.md`, `NewGUIGuide.md`, `README.md` (`SessionController`, `EmulatorSession`, `Platform`, `Threading`); `frontends/swift/App/EmulatorViewModel.swift` and `CardStorage.swift` comments; the `calc-u-59-memories.md` lines that name them |
| M-34 | `calc-u-59-web` skill, `reference/WebClient.md`, `reference/StateFileFormat.md`, `frontends/web/build.sh` comments; `docs/glossary-mapping.md`: the Calc-U-59 parentheses go |

`products/calc-u-59/ideas/` is a backlog of ideas and is not renamed
(as with `docs/background/*`).

## 8. Golden runs and risks

- **Golden runs** (`products/calc-u-59/tests/golden-runs/`, 34 files;
  `products/calc-u-1600/tests/golden-runs/`, 40 files) drive machines
  directly. They guard M-31 and M-33 only against changes to the
  machine around the session. A difference is a finding.
- **Reference runs.**
  - The 1600 references (`tests/reference/v0.8.1-examples/`, M-4) were
    made with the CLIs and have no driver in the repository. M-31 adds
    `lib3_reference_examples_through_session` (Longer) that loads each
    example through the library session in virtual time and compares
    the display text and the log with them.
  - The 59 references (`tests/reference_runs_tests.cpp`) drive
    `SessionController` until M-34. M-33 adds the same runs through
    `TIAppPart` on the library session (`reference_runs_session_tests.cpp`,
    Frequent if within budget). That guards the new path while the old
    one still serves the Web.
  - In both, a difference is a finding (the slice ends move, LIB-17 §8).
- **Duplicate flows from M-33 to M-34.** `SessionController` (Web) and
  `TIAppPart` (Qt6, Swift) hold the same flows for one step. A fix in
  that window must be made twice; accepted for one step (D-1).
- **Exceptions in coroutines.** `Job` keeps its `exception_ptr`. Both
  products build with exceptions, and under Emscripten both use
  `-fwasm-exceptions` from M-19 on (LIB-1 D-6).
- **Per-call cost.** The flush calls `equals` on every state topic each
  call. The display payload compares a whole `DisplayOutput` (every dot of a
  PC-1600 frame). A dirty bit per topic, set by
  `set()`, avoids comparing when nothing was set.
- **Stream history growth.** `log` and paper keep everything since
  their last reset for late listeners (spec OQ-5). No limit is
  specified, and none is set (D-8), as today.
- **Qt6 host keys while busy.** Today Qt6 holds keys back during
  `runNow` (event loop without user input). After M-31 the keys reach
  the session while an operation runs. Dropping them is LIB-5's (T-4,
  ledger 1600-9, M-36). Until then `SharpTypingHook` passes them on, as
  today's Web frontend does (ledger: "passed on in the Web frontend").

## 9. Decided (best guess)

- **D-1 (was OQ-1; plan M-33).** Reading (a): from M-33 to M-34 the Web
  keeps `SessionController` and `EmulatorSession`, cooperative, with
  `CooperativePacing`; the flows exist twice for one step (§8).
- **D-2 (was OQ-2; AC-18, AC-5).** An operation's command is replied to
  at the end of the operation with its result; only busy goes out at
  once. A failure is told once.
- **D-3 (was OQ-3; AC-23).** The profile is handed with a new machine
  (LIB-17 AC-14); a debugger resume only starts afresh.
- **D-4 (was OQ-4; AC-14).** Calc-U-1600's operation part grows from
  10 ms to LIB-17's 14 ms, as AC-14 ties it; ledger row (slightly faster
  loading).
- **D-5 (was OQ-5; AC-20).** `AudioPump`'s open, prime and silence rules
  go into the session (one place, as today's shared `AudioPump`); the
  sink opens, writes, closes and keeps the carry-over cap.
- **D-6 (was OQ-6; AC-21).** While an operation sets the speed, the speed
  command is replied to as done and changes nothing.
- **D-7 (was OQ-7).** The sentences stand: "Another operation is
  running.", "Unknown command: <name>.", "Stopped.", "No machine is
  running.".
- **D-8 (was OQ-8).** No limit on what `log` and paper keep since their
  last reset, as today.
- **Not verified.** Whether `KeySequencer`'s cooperative mode alone
  (without its lanes' threads) gives the 2.3.0 timing on Qt6 and Swift.
  The reference runs through the new path (§8) check it.

**What this design assumes of the other C1 designs** (A-1 to A-8; until
2026-11-14 these were numbered D-1 to D-8):
- A-1 **LIB-2**: the host link of a serial port is a `SessionPart`.
  Its thread-safe queue is the host adapter's (ADR 0009), and it is
  drained in `beforeCall`. Attach and detach are operations started by
  LIB-18 commands. A refusal is a `Refused` reply or a notice.
- A-2 **LIB-11**: the harness hosts `AppSession` with
  `Hosting::VirtualTime`. It runs `wait` through `advanceUnpaced`, sends
  `key-down`/`key-up`, reads `state` from a `TopicMirror` (streams with
  `takeAppended`), and asks `ticks` and `capabilities`. `audio <file>` is
  a listener with `recordsSound`. It states no host services.
- A-3 **LIB-5** (C2, for later): live input goes in front of `key-down`/`key-up`
  as a `SliceHook`, and user keys are dropped while `busy()`.
- A-4 **LIB-15**: the worker is the host. It calls `call(now)` from
  `setInterval`, translates `Message` with `Payload::toValue()`, sends
  `suspend`/`continue` on `visibilitychange`, and states its services
  with `listen`.
- A-5 **C59-5**: the session thread (product code in
  `products/calc-u-59/app/`) is the host. Its two queues carry
  `Command`/`Request`/`ServiceAnswer` in and `Message` out. Attach is a
  second `listen()` with no services, and replies are routed per
  `ListenerId`.
- A-6 **LIB-17**: `Pacer::pace` reads its own `WallClock`.
  `SliceRequest::due` is there for `TIKeySequencerHook`.
  `PaceResult::ranUnpaced` drives the sound discard. `kFullSpeedAllowance`
  is the operation allowance.
- A-7 (asked by LIB-2 A-4 and LIB-11 S-7; decided): the host-time source
  `AppSessionSetup::hostTime` (§3.9): local date and time in the
  products, a fixed instant in the harness. The session sets the
  real-time clocks with it at build and at the end of every operation
  (LIB-2 AC-23).
- A-8 (asked by LIB-11 S-6 for `state <topic>`, LIB-11 EC-5; decided):
  the request `topics` (§3.4) lists every declared topic name and its
  shape before one is sent.
