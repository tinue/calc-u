# C59-5 Swift frontend hosting: design
Status: approved by the user at the Part B/C1 boundary (2026-11-14)

## 1. Scope

C59-5 is a product row (Calc-U-59 only). Plan §5.1: tested in M-15
(AC-1–3) and M-33 (AC-4–14), Done in M-33.

| Step | Does (plan) | Tests from (C59-5) | State |
|---|---|---|---|
| M-15 | XCFramework from the root CMake project; Xcode builds only Swift and the bridge | AC-1–3 | done, `qa-report.md` (2026-11-14) |
| M-16 | spikes FS 3, FS 4 | none (results in `docs/walkthroughs/first-steps.md`) | done, branch `spike-fs3-fs4` (not merged) |
| **M-33** | the Swift frontend hosts the library's app session on a background thread with the bridge as a message boundary; attach on a localhost TCP port in Debug builds; Qt6 on the same session; `EmulationRunner`, the threaded runner and the pacer go; `TIPlatform`'s duties as LIB-3 AC-30 says | **AC-4–14, EC-1–4** | this design |

M-33 also takes tests from LIB-3 AC-28–30, AC-32–33 and LIB-17 AC-15–19.
Those are the LIB-3 59 and LIB-17 59 designs' (§9, A-1 to A-12). This
design owns the Swift side of the step and attach. It touches the shared
parts in three places only: the Swift share of LIB-3 AC-30 (C59-5
Technical Requirements, "`TIPlatform`"), the Swift share of LIB-3 AC-31
(= AC-8 here; AC-31 itself is tested in M-34), and the attach client
that LIB-17's P59-5 uses for AC-19 on iOS and macOS.

Nothing of C59-5 lands after M-33. The test aids of the UI tests
(`--UITesting`, the hidden R/S button, `keystroke-playback-status`) stay
until M-58 (C59-8 AC-21; C59-5 spec OQ-4 as decided). M-33 adds the first
XCUITest that uses attach (plan, M-33 "Green after").

Waits for (plan): M-16 (FS 3, FS 4 hold), M-3 (FS 12: AC-8, AC-9 stand),
M-10 (FS 14), M-15, M-28 (`calcu`, LIB-11), M-32 (LIB-9 59).

## 2. Modules and files

Two parts, both host adapters at the ADR 0009 boundary, the one
exception LIB-1 design §2a allows to thread and lock. The session thread
and its queues are Calc-U-59 product code: only the Swift frontend uses
them, and no Calc-U-1600 Swift frontend is planned (user, 2026-11-14;
LIB-1 §2a, "code only one product uses lives in that product"). Attach
stays in the library, because LIB-14 AC-44 (Qt6 attach, both products)
reuses it.

**Calc-U-59, app layer: the session thread** (`products/calc-u-59/app/`,
namespace `cu59`, product style; D-11):

| File | Holds |
|---|---|
| `app/Mailbox.hpp/.cpp` | the two queues between the frontend's thread and the session thread: posts in (FIFO), messages out (FIFO per listener), a wake callback on empty → non-empty |
| `app/SessionThread.hpp/.cpp` | the thread that owns a hosted session: builds it on the thread, drains the inbox, calls it about 60 times a second, stops calling while suspended; implements the library's `calcu::harness::AttachInbox` (§3.2) |
| ~~`app/HostedSession.hpp`~~ | gone in the C1 simplification: the thread drives the library's `calcu::sessions::AppSession` itself (made by `cu59::makeAppSession(...).session`, D-12) through a `calcu::sessions::Listener`; the H2 tests use the library's session with a test part |
| `tests/mailbox_tests.cpp` | AC-5, AC-6, in the product test program `CalcU59Tests` |
| `tests/session_thread_tests.cpp` | AC-4, AC-5, AC-8, same program |

`app/` is also compiled into the Web engine (`calcu59_shared`); the Web
never builds a `SessionThread`, as it never starts today's threaded
runner.

**Library, attach, in LIB-11's folder: `library/harness/attach/`, namespace
`calcu::harness`**, compiled into `calcu_library` only with the CMake
option `CALCU_ATTACH` (default OFF; POSIX only), LIB-1 §2a. It knows no
product code: the host of the session implements `AttachInbox` (D-11).

| File | Holds |
|---|---|
| `library/harness/attach/AttachEndpoint.hpp/.cpp` | POSIX socket on 127.0.0.1, one client at a time, line framing, its own I/O thread and its own outgoing line queue; posts to an `AttachInbox`; never touches the session |
| `library/harness/attach/AttachedHarness.hpp/.cpp` | on the session's thread: the second listener; runs LIB-11's command processor with app-paced time; refuses while suspended |
| `library/harness/attach/AttachPort.hpp/.cpp` | `attachPort(arguments)`: `--attach-port N`, else 59059 |
| `library/harness/attach/tests/attach_tests.cpp` | AC-10–12, AC-14, EC-1–4 (client sockets in the test, test machine, a test host loop), in `calcu_library_tests` when `CALCU_ATTACH` is on |
| `library/harness/cli/AttachCommand.cpp` | `calcu attach [--port N] [--json] [--messages]`: relays stdin lines to the app and its lines to stdout |
| `library/harness/cli/tests/attach_cli_tests.cpp` | AC-10, AC-11 (the options that do not apply) |
| `library/CMakeLists.txt` (changed) | `option(CALCU_ATTACH …)`: adds `library/harness/attach/` and its tests; the `calcu` executable always has `attach` (the client side is no attach in an app, I-3) |
| `.github/workflows/calc-u-59.yml`, `calc-u-1600.yml` | the `library` job on macOS and Linux configures with `-DCALCU_ATTACH=ON`, so the attach tests run in CI |

**Calc-U-59, Swift frontend** (`products/calc-u-59/frontends/swift/`):

| File | Change |
|---|---|
| `Bridge/TIAppSession.h/.mm` (new) | the bridge class: `send`, the message handler, suspend/continue, attach (Debug, macOS and simulator only). Replaces `TISessionController` |
| `Bridge/TIMessages.h/.mm` (new) | ObjC value types for the commands and messages and their translation to and from the library's and the app layer's types |
| `Bridge/TISessionController.h/.mm` | deleted (the `TIPlatform` protocol with it) |
| `Bridge/Calc-U-59-Bridging-Header.h` | imports `TIAppSession.h`, `TIMessages.h` |
| `Bridge/TIViewRules.*`, `TISharedTables.*`, `TIFileFormats.*` | stay: the app layer's view data and pure rules (Out of Scope) |
| `Bridge/TISettings.*` | stay with the frontend on the main thread (LIB-9 design, "Threading"; D-3) |
| `Bridge/TICoordinatedFiles.h/.mm` (new) | the `cu59::FileIO` the bridge hands `TIFileAccess`: reads and writes through `NSFileCoordinator` and starts the download of a `.icloud` placeholder, as `EmulatorViewModel.swift:490-546` does today (D-6) |
| `App/EmulatorViewModel.swift` | holds the last message of each state topic and what each stream topic appended; sends commands; no `Timer`, no `tick`, no getter of the session. `PlatformForwarder` (lines 486-561) goes |
| `App/HostServices.swift` (new) | the host services the frontend states (AC-7) and the handlers that answer the session's requests |
| `App/Calc-U-59App.swift` | `scenePhase` → `viewModel.suspend()` / `continueSession()` (lines 17-26) |
| `App/CardStorage.swift` | resolves the save folder off the main thread as today (`warmUp`), before the session is created; reads no card file any more (the coordinated reads move to `TICoordinatedFiles`) |
| `Calc-U-59-Debug.entitlements` (new) | the iCloud keys of `Calc-U-59.entitlements` plus `com.apple.security.network.server` |
| `Calc-U-59.xcodeproj/project.pbxproj` | app target Debug: `CODE_SIGN_ENTITLEMENTS = Calc-U-59-Debug.entitlements`, `ENABLE_INCOMING_NETWORK_CONNECTIONS = YES` (today `NO`, line 512); Release unchanged (lines 567, 573); new unit-test target `Calc-U-59Tests` (Swift only) |
| `build_xcframework.sh` | `-DCALCU_ATTACH=ON` only for the Debug `macos` and `ios-simulator` slices, never for `ios`; the library archive and headers are merged as LIB-1 §2a says (from M-19), so nothing else changes |
| `check_xcode_project.sh` | also: Release has no `network.server`, Debug's entitlements file has it (AC-10, AC-13) |
| `check_bridge_boundary.sh` (new) | the bridge includes no `SessionController.hpp`, declares no getter of session state, calls the session only through `cu59::SessionThread` (Technical Requirements) |
| `check_no_attach.sh` (new) | builds Release and the Debug `ios` slice and finds no `AttachEndpoint` symbol in them; reads the signed Release app's entitlements (AC-13) |
| `Calc-U-59UITests/HarnessClient.swift` (new) | a line client for the attach port (the spike's `Harness`, `UITests/AttachTests.swift:8-58` on `spike-fs3-fs4`) |
| `Calc-U-59UITests/AttachTests.swift` (new) | AC-5, AC-8, AC-10–12, AC-14, EC-3 in the iOS simulator; in `UIRegressionTests` |
| `Calc-U-59Tests/HostServicesTests.swift`, `MessageOrderTests.swift` (new) | AC-6, AC-7 through the real bridge |
| `tests/mac_attach_check.sh` (new) | AC-9, AC-10 on the Mac (Checkpoint) |
| `products/calc-u-59/CMakeLists.txt` | CTest entries of §6 |

## 3. Seams and interfaces

### 3.1 The session thread (`cu59`, `products/calc-u-59/app/`): AC-4, AC-5, AC-6, AC-8

```cpp
namespace cu59 {

using calcu::sessions::ListenerId;      // A-3
using calcu::sessions::HostServices;    // A-4
using calcu::sessions::Command;         // A-2: owning value types, movable
using calcu::sessions::Message;
namespace sessions = calcu::sessions;  namespace pacing = calcu::pacing;

// What the thread drives (as drafted; since the C1 simplification the thread
// drives sessions::AppSession directly, through a sessions::Listener, and the
// tests use a session with a test part whose calls take as long as they want).
class HostedSession {
public:
    virtual ListenerId listen(sessions::MessageSink&, HostServices) = 0;  // LIB-3 AC-6, AC-26
    virtual void stopListening(ListenerId) = 0;
    virtual void send(ListenerId from, Command) = 0;     // carried out on arrival (LIB-3 AC-5, spec OQ-3)
    // (names as LIB-3's AppSession: listen, stopListening, send; requests go through ask(), answers
    //  through answer(); reconciled with the LIB-3 design)
    virtual void call(pacing::WallTime now) = 0;         // one host call (LIB-3 AC-2)
    virtual sessions::AppSession* appSession() = 0;      // for AttachedHarness; nullptr in the H2 test session
    virtual ~HostedSession() = default;
};

struct Post {                       // one item of the inbox, FIFO
    enum class Kind { Command, Attach } kind;
    ListenerId from;                // Command
    Command command;
    calcu::harness::AttachEvent attach;   // Attach: attached, a line, detached (§3.2)
};

class Mailbox {                     // the only shared state; one mutex per queue
public:
    void post(Post);                                   // any thread; never waits for a call (AC-5)
    std::vector<Message> drain(ListenerId);            // the listener's own thread
    void setWake(ListenerId, std::function<void()>);   // called on the session thread when that
                                                       // listener's outbox goes empty → non-empty
};

class SessionThread : public calcu::harness::AttachInbox {
public:
    using Factory = std::function<std::unique_ptr<sessions::AppSession>()>;  // runs on the thread (was HostedSession)
    static constexpr std::chrono::microseconds kPeriod{16'667};       // AC-4

    SessionThread(Factory, HostServices frontendServices, pacing::WallClock clock);
    ~SessionThread();                       // stops calling, joins
    ListenerId frontend() const;            // the app's own frontend, listening from the start
    Mailbox& mailbox();
    void suspendCalls();                    // any thread; see §3.3
    void continueCalls();
    bool callsSuspended() const;            // for AttachedHarness (EC-3)
    void post(calcu::harness::AttachEvent) override;  // AttachInbox: the endpoint's I/O thread
    void setAttachedHarness(calcu::harness::AttachedHarness*);  // Debug: events go to it; told after each host call
};

}  // namespace cu59
```

Rules:
- The session (and the app layer behind it) is built, called and
  destroyed on the session thread only. Nothing else touches it
  (`frontend.md` I-1; LIB-3 Technical Requirements).
- Loop: drain the inbox and apply each item in order (attach events go
  to the `AttachedHarness`), then one host call with `clock()`, then
  `AttachedHarness::afterHostCall()`, then sleep until the next
  absolute slot `t₀ + n·kPeriod`. When the thread is more than 100 ms
  behind, it rebases `t₀` and does not run calls back to back. Lag is the
  pacer's (LIB-17 R1); the thread only keeps its own rate. This is the
  spike's loop (`Host/SessionHost.cpp`, `run()`).
- One `QueuedListener` per listener copies each message into that
  listener's outbox. The wake runs once per batch: the Swift wake is
  one `dispatch_async` to the main queue. The main thread drains every
  message of the batch in one go (FS 3: "one hop per batch").
- Order (AC-6): the inbox is one FIFO and the outbox is one FIFO per
  listener, filled only by the session thread. There is no other path.
- The thread's QoS on Apple platforms is `QOS_CLASS_USER_INITIATED`
  (`pthread_set_qos_class_self_np`). Today's emulation queue uses
  `.utility` only for the card warm-up (`EmulatorViewModel.swift:209`)
  (D-7).

### 3.2 Attach (`calcu::harness`): AC-10–14, EC-1–4

```cpp
namespace calcu::harness {

constexpr std::uint16_t kDefaultAttachPort = 59059;              // C59-5 decision log, 2026-11-14
std::uint16_t attachPort(std::span<const std::string> arguments); // "--attach-port N", else the default

struct AttachEvent { enum class Kind { Attached, Line, Detached } kind; std::string line; };

class AttachInbox {               // implemented by whoever hosts the session (cu59::SessionThread;
public:                           // later the Qt6 GUI-thread loop, LIB-14 AC-44; a loop in the tests)
    virtual void post(AttachEvent) = 0;   // any thread; never waits for a host call
protected: ~AttachInbox() = default;
};

class AttachEndpoint {            // the I/O thread; owns the socket and its outgoing lines, never the session
public:
    struct Opened { bool ok; std::string problem; };  // problem: "Address already in use", …
    AttachEndpoint(AttachInbox&, std::uint16_t port);
    Opened open();                // bind 127.0.0.1:port, listen. Fails without throwing (EC-1)
    void write(std::vector<std::string> lines);   // any thread; queued for the client, in order
    ~AttachEndpoint();            // closes the client and the socket, joins
};

class AttachedHarness : public sessions::MessageSink {
public:                           // lives on the session's thread
    AttachedHarness(sessions::AppSession&, AttachEndpoint&, std::function<bool()> callsSuspended);
    void handle(const AttachEvent&);
    // Attached → listen(*this, {}) (no host services, AC-12)
    // Line     → refused if callsSuspended() (EC-3), else LIB-11's command processor
    // Detached → stopListening, drop pending replies, release keys it holds down (EC-2)
    void afterHostCall();         // completes pending `wait`s on the app's emulated time (AC-14)
};

}  // namespace calcu::harness
```

How a line travels: the socket → the I/O thread frames the line →
`AttachInbox::post(Line)` → the session thread → `AttachedHarness` →
LIB-11's command processor with the attach time policy (A-9). The
processor sends LIB-3 commands as the harness's listener. Replies come
back as messages to that listener only (LIB-3 AC-13; AC-12).
`AttachedHarness` formats them, as plain text or with `--json` (LIB-11
AC-4), and hands the lines to `AttachEndpoint::write`; the I/O thread
writes them.

| Case | Rule | AC |
|---|---|---|
| port | `attachPort` from the launch arguments the bridge hands over; default 59059; one per simulator by argument (FS 4) | AC-10 |
| bind | 127.0.0.1 only (`INADDR_LOOPBACK`), never `INADDR_ANY` | AC-10 |
| bind fails | `open()` returns the problem; the bridge posts it as a line of the session's log ("Attach is not available on port 59059: Address already in use."); the app runs on | EC-1 |
| connect | the `Attached` event; the session sends every state topic once, and the paper and the log from their last reset marker (LIB-3 AC-6, spec OQ-5) | AC-12 |
| second client | one line `refused: one harness is already attached` (LIB-11 §3.9), then close; the first stays | EC-4 |
| `quit` | taken by the endpoint: the `Detached` event, close the client; the app goes on | AC-11 |
| EOF or error without `quit` | as `quit` | EC-2 |
| suspended | every command but `quit` gets `refused: the app is suspended` | EC-3 |
| `wait <d>` | target = the emulated time when the command arrived + d (the session's emulated-time request, LIB-3 AC-39). Checked after each host call; the reply comes when it has passed. Never advances the machine itself. The speed is whatever was commanded. While the debugger has the machine paused it ends at once (LIB-11 AC-9) | AC-14 |
| other commands | as in `calcu serve` (LIB-11 AC-9 and later columns); an operation started from the harness runs in parts over the app's host calls, and its reply comes when it ends (LIB-11 AC-7 holds by waiting) | AC-11 |
| messages | the harness listener's messages go to the client as lines, beside the replies, in the form of LIB-11 §3 attach line format (C59-8 AC-21, "reads the messages as a stream"; D-1) | AC-12 |
| `json on` | taken by `AttachedHarness`, like `quit` by the transport: from then on replies are formatted as with `--json` (D-2) | AC-11 |

**`calcu attach`.** `calcu attach [--port N] [--json] [--messages]`
connects to 127.0.0.1:N (default 59059). It sends `json on` first when
`--json` is given (D-2), relays stdin to the app line by line, and
prints the replies; with `--messages` it prints the message lines too,
else it drops them (D-1). It
ends at `quit` or at the end of stdin, which sends `quit`. Given
`--product`, a settings file, a save folder or a ROM folder, it says that
these do not apply to an attached harness and exits with status 2
(AC-11; D-4).

### 3.3 The bridge (`TIAppSession`): AC-4–8, Technical Requirements

```objc
@interface TIAppSession : NSObject
/// Builds the thread; the session is built on it. `start` carries the host folders
/// (LIB-9 HostDirs), the startup model and the host services stated (AC-7).
- (instancetype)initWithStart:(TISessionStart *)start
                   onMessages:(void (^)(NSArray<TIMessage *> *batch))handler;  // main thread, in order
- (void)send:(TICommand *)command;          // any thread; returns at once (AC-5)
- (void)suspend;                            // send suspend, then stop calling (AC-8)
- (void)continueSession;                    // start calling, then send continue
#if DEBUG && (TARGET_OS_OSX || TARGET_OS_SIMULATOR)
/// Opens attach on the port the arguments name (AC-10). NO, and a log line, on EC-1.
- (BOOL)openAttachWithArguments:(NSArray<NSString *> *)arguments;
#endif
@end
```

- `TICommand` has one class factory per command, generic or a product
  command of Calc-U-59 (A-2, A-6). `TIMessage` has one subclass per topic
  shape: `TIStateMessage(topic, value)`, `TIStreamMessage(topic, appended,
  reset)`, `TIReply(correlation, done, result, reason)`,
  `TIHostServiceRequest(id, service, arguments)`. The values are the
  translated types (display frame, busy, notices, log, and the product
  topics A-6 lists). The bridge holds no state of the machine and
  decides nothing (ADR 0014). Its one map is topic name → translator.
- `suspend` posts the suspend command, then `suspendCalls()`. The thread
  carries out the suspend (and the TI-58C constant-memory write that
  goes with it, A-7) before it stops calling. While suspended the thread
  sleeps on the inbox, so commands are still carried out on arrival.
  `continueSession` is the reverse order. LIB-3 AC-24 holds: the session
  does not suspend itself. The thread only stops calling, as "the
  session thread stops calling" in AC-8.
- The compile guard and the CMake option are the same condition:
  `build_xcframework.sh` passes `-DCALCU_ATTACH=ON` exactly for Debug
  `macos` and Debug `ios-simulator`. The Swift call site is under
  `#if DEBUG && (os(macOS) || targetEnvironment(simulator))`. A Release
  build or a device build therefore has neither the code nor the call
  (AC-13).

### 3.4 Swift side: AC-4–9

- `EmulatorViewModel` creates `TIAppSession` once, after
  `CardStorage.warmUp()` (today `start(model:)`, `EmulatorViewModel.swift:205-216`).
  Its handler applies each message of a batch to the observable
  properties. A state topic replaces the property; a stream topic
  appends, or clears on a reset marker. Nothing is read back.
  `startDisplayRefresh()` and `displayTimer` (lines 188, 219-228) go:
  the session thread calls the session, and SwiftUI redraws when a
  property changes (AC-4).
- One app session for the app, shared by all windows (C59-5 spec OQ-6): the
  view model stays the `@State` of `CalcU59App`
  (`Calc-U-59App.swift:8`).
- `scenePhase` `.background` → `suspend()`; `.active` → `continueSession()`
  (AC-8, lines 17-26). On iOS, `suspend()` runs inside
  `UIApplication.beginBackgroundTask`. The task ends when the reply to
  the suspend arrives, so the constant memory is written before iOS
  stops the process (today the write is synchronous on the main thread
  in `SessionController::suspend`).
- The App Nap activity stays as it is (lines 192, 198-202). With it the
  covered Mac window keeps running (AC-9, FS 12).
- Host services: `HostServices.swift` states, on iOS, clipboard text,
  clipboard image, file picker, keep file and open URL, haptics; on
  macOS, clipboard text, clipboard image, file picker, folder picker,
  keep file and open URL. It states no audio sink (AC-7). Each handler
  answers a `TIHostServiceRequest` with one command (LIB-3 AC-27). The
  folder picker answer carries the security-scoped grant that LIB-9
  AC-34 keeps in `UserDefaults` (LIB-9 design: `HostDirs::chosenTraceFolder`).
  The key click and haptics on a key press stay the frontend's own
  (LIB-3 AC-33), as in `KeyboardView`.
- Three calls of today return a value at once, and these change shape
  (D-5):
  - `tapCardAt` → `Bool` (`EmulatorViewModel.swift:300`, used by
    `KeyboardView.swift:133` to swallow the touch);
  - `acceptsDroppedFile` (line 363);
  - `printerStripHeightUnits` (line 269).

  The hit test and the drop check become pure view rules of
  the app layer in `TIViewRules`. They are called with the card geometry
  and the model from the last messages, and the action is then a
  command. The strip height becomes part of the paper topic. The macOS
  security scope of a dropped card stays open until the reply and the
  media state say the card is fitted (lines 362-372).

## 4. Product side

| Today (`products/calc-u-59/`) | After M-33 |
|---|---|
| `frontends/swift/Bridge/TISessionController.mm` with `PlatformAdapter` (lines 159-197) over the weak `TIPlatform` | deleted; `TIAppSession` over `cu59::SessionThread` |
| `TIPlatform.runOnUiThread` → `dispatch_async(main)` | no successor (LIB-3 AC-30). The one main-thread hop is the mailbox wake |
| `readConstantMemory`, `writeConstantMemory`, `readCardFile`, `cardFileExists`, `writeCard`, `cardFilePath` with `NSFileCoordinator` (`EmulatorViewModel.swift:490-546`) | the library's, in the save folder that `CardStorage` resolves and LIB-9's `HostDirs::saveFolder` carries. Until C59-3 / C59-7 (M-54, M-55) behind LIB-3 59's `TIFileAccess` (A-7, §5). The coordination stays: on Swift `TIFileAccess` reads and writes through the bridge's `TICoordinatedFiles` (D-6) |
| `traceDirectory`, `traceMaxBytes`, `traceAvailable` | LIB-9 (`Paths`, `[trace]` keys), given at session creation |
| `saveLastUsedModel` | a setting (LIB-9, M-32); the Swift frontend writes it on the main thread through `TISettings` when the model topic changes (D-3) |
| `saveModuleID` | gone (row 59-9; LIB-3 AC-30) |
| `showErrorTitle`, `diagnostic` | a notice, a log line (LIB-3 AC-10, AC-11); the view model shows notices as today's alert until C59-8 |
| `showDebugPanel` | a product message of C59-4 (LIB-3 AC-30); until then the product topic A-6 names |
| `changed(TIChange)` and about 70 getters (`TISessionController.h:136-239`) | messages (A-6); `TIChange` goes |
| `EmulatorViewModel` `Timer` at 60 Hz calling `tick` on the main run loop (`EmulatorViewModel.swift:219-228`) | gone; the session thread (AC-4) |
| `cu59::EmulationRunner` threaded mode, `KeySequencer` lanes with `Threading::Threads`, `requireThreadSupport` | gone in M-33 (LIB-17 design §2; LIB-3 59). `SessionController` stays, cooperative only, for the Web and its tests until M-34 |
| Core mutexes (`core/TI59Machine.hpp:196`, `core/TMC0501.hpp:420, 430, 485`) | gone in M-19 (LIB-1 design §4). M-33 checks that none is left (C59-5 Technical Requirements) |
| `ProcessInfo.beginActivity` App Nap token | stays (AC-9) |
| `CalculatorView.swift:84-147` test aids | stay until M-58. `keystroke-playback-status` reads busy (LIB-3 AC-9) in place of `isKeystrokesPlaying` |
| `Calc-U-59.entitlements` for both configurations | Release keeps it; Debug gets `Calc-U-59-Debug.entitlements` |

The Qt6 frontend's move onto the library session in M-33
(`frontends/qt6/app/EmulatorController.*`, LIB-3 AC-28) is the LIB-3 59
design's. Qt6 gets no attach in M-33 (LIB-14 AC-44, M-49 / M-57). When
it does, it reuses `AttachEndpoint` and `AttachedHarness`. Its host loop
on the GUI thread implements `AttachInbox` and calls `afterHostCall`,
so the endpoint does not depend on which thread hosts.

## 5. Adapters

| Adapter | File | Wraps | Removed in |
|---|---|---|---|
| `TIFileAccess`: card and constant-memory files on the session thread (LIB-3 59's design, A-7) | `products/calc-u-59/app/TIFileAccess.*` | today's `Platform` file calls, in the save folder, through a `cu59::FileIO` the host hands it: plain file I/O for Qt6, the bridge's coordinated I/O for Swift (D-6) | M-54 (C59-3, constant memory), M-55 (C59-7, cards) |
| Calc-U-59 interim product topics and commands of `cu59::TIAppPart` (`c59.model`, `c59.printer`, `c59.card-reader`, `c59.run-state`, `c59.trace`, `c59.debug.*`, `c59.cue-card`; LIB-3 59's design §5, A-6) | `products/calc-u-59/app/TIAppPart.*` | what `TISessionController` reads today and no generic topic carries yet: printer lines, cue card, card reader, run state, debug snapshots, ASM, log level | each with its owner: LIB-8 (M-53), LIB-7 / C59-7 (M-55), LIB-10 / C59-4 (M-53, M-56), LIB-4 (M-57, M-58) |
| test aids in `CalculatorView.swift:84-147` | Swift | not an old call; kept for the UI tests that do not use attach yet | M-58 (C59-8 AC-21) |

C59-5's own code needs no adapter. `TISessionController` and
`TIPlatform` go in M-33 in one step. Plan §1: no adapter is left at the
end. M-59 checks it.

## 6. Tasks

Tests come before the code. Groups as in `docs/test-groups.md`.
The session thread's tests (H1–H3) are in `products/calc-u-59/tests/`,
program `CalcU59Tests`, with the library's session and a part made for
the test. The
attach tests (A1–A5) are in `calcu_library_tests` with `CALCU_ATTACH` on
and use LIB-1's test machines (`library/machine/tests/TestMachines.hpp`)
in a LIB-3 session. Case names carry the row: `TEST(c59_5_ac6_…)`
(LIB-1 §2a; D-8).

| Task | What | ACs/ECs | Test (file, case, group) |
|---|---|---|---|
| H1 | `Mailbox` | AC-5, AC-6 | `products/calc-u-59/tests/mailbox_tests.cpp`: `c59_5_ac6_posts_reach_the_session_in_order` (1000 posts from two threads, sequence kept per poster), `c59_5_ac6_messages_drained_in_order_one_wake_per_batch`, `c59_5_ac5_post_returns_while_a_call_runs` (post < 1 ms during a 200 ms call); Frequent |
| H2 | `SessionThread` loop | AC-4, AC-5 | `session_thread_tests.cpp`: `c59_5_ac4_about_60_calls_a_second` (1 s real clock: 57–63 calls), `c59_5_ac4_session_built_called_destroyed_on_one_thread` (thread id ≠ the test's), `c59_5_ac5_command_during_slow_call_applied_after_it` (15 calls of 200 ms, as FS 3), `c59_5_ac4_behind_more_than_100_ms_rebases`; Frequent |
| H3 | suspend / continue | AC-8 | `session_thread_tests.cpp`: `c59_5_ac8_suspended_no_calls_commands_still_carried_out`, `c59_5_ac8_continue_calls_again_and_session_got_continue`; Frequent |
| A1 | port and bind | AC-10, EC-1 | `library/harness/attach/tests/attach_tests.cpp`: `c59_5_ac10_port_from_argument_else_59059`, `c59_5_ac10_bound_to_loopback_only` (`getsockname`), `c59_5_ec1_port_in_use_host_runs_and_log_says_so`; Frequent |
| A2 | connection rules | AC-11, EC-2, EC-4 | `attach_tests.cpp`: `c59_5_ac11_quit_ends_attach_not_host`, `c59_5_ec2_gone_without_quit_as_never_attached` (keys released, listener gone, the next client attaches), `c59_5_ec4_second_harness_one_line_and_closed`; Frequent |
| A3 | second listener | AC-11, AC-12 | `attach_tests.cpp`: `c59_5_ac11_command_script_as_in_serve` (the same script through `serve` and attach: the same replies except for `wait`'s timing), `c59_5_ac12_every_state_topic_once_on_attach`, `c59_5_ac12_reply_only_to_sender`, `c59_5_ac12_host_service_never_asked_of_harness`, `c59_5_ac12_frontend_keeps_receiving`; Frequent |
| A4 | time on attach | AC-14 | `attach_tests.cpp`: `c59_5_ac14_wait_passes_on_the_app_clock` (`wait 300ms` at regular speed: ≥ 300 ms wall, ticks within LIB-17 AC-1's bound), `c59_5_ac14_wait_at_full_speed_sooner`, `c59_5_ac14_harness_never_advances_the_machine`; Frequent (real clock, under 2 s together) |
| A5 | suspended | EC-3 | `attach_tests.cpp`: `c59_5_ec3_command_refused_while_suspended`, `c59_5_ec3_quit_still_works`; Frequent |
| A6 | `calcu attach` | AC-10, AC-11 | `library/harness/cli/tests/attach_cli_tests.cpp`: `c59_5_ac10_calcu_attach_relays_lines`, `c59_5_ac11_options_that_do_not_apply_are_refused`; Frequent |
| B1 | XCFramework with `calcu_library`; attach only in Debug macOS and simulator | AC-10, AC-13 | `check_xcode_project.sh` (CTest `CalcU59XcodeProjectCheck`, Frequent) extended; `check_no_attach.sh` (CTest `CalcU59NoAttachInRelease`, Longer: Release slices and the Debug `ios` slice have no `AttachEndpoint` symbol; the Release app's entitlements have no `network.server`) |
| B2 | bridge boundary | AC-4, AC-6; TR | `check_bridge_boundary.sh` (CTest `CalcU59BridgeBoundaryCheck`, Frequent) |
| S1 | bridge and view model on messages | AC-4, AC-6 | `Calc-U-59Tests/MessageOrderTests.swift`: `testMessagesArriveOnMainInOrder` (500 key commands, replies in order, all on the main thread); CTest `CalcU59SwiftUnitTests` (`xcodebuild test`, iOS simulator), Longer |
| S2 | host services | AC-7 | `Calc-U-59Tests/HostServicesTests.swift`: `testStatedServicesIOS`, `testStatedServicesMacOS` (macOS run), `testNoAudioSink`; Longer (iOS), Checkpoint (macOS, as `CalcU59UITestsMac`) |
| S3 | attach in the simulator | AC-10, AC-11, AC-12, AC-14 | `Calc-U-59UITests/AttachTests.swift`: `testHarnessDrivesTheApp` (launch with `--attach-port 59160`; the state on attach; `key-down`/`key-up` 1, 2, 3; `display-text` says `123`; a screenshot of the display element shows the change; `wait 1s` takes 1–1.5 s; `quit`, the app goes on); in `UIRegressionTests`, CTest `CalcU59UIRegression`, Longer |
| S4 | background in the simulator | AC-8, EC-3 | `AttachTests.swift`: `testBackgroundSuspendsAndContinues` (Home: a command is refused "suspended"; `activate()`: `ticks` grows again); Longer |
| S5 | busy calls and the main thread | AC-5 | `AttachTests.swift`: `testTapsReachSessionAtFullSpeed` (`speed full` over attach, a running program; taps on keys arrive, read with `display-text`); the frame gaps are measured as in FS 3 and written into `qa-report.md` (D-9); Longer |
| S6 | Mac | AC-8, AC-9, AC-10 | `tests/mac_attach_check.sh` (Debug Mac app, sandboxed): `calcu attach` works; a program runs; another window covers it 30 s; `ticks` grew by 30 s ± LIB-17 AC-1's bound; the app is hidden and shown, `suspend`/`continue` seen in the log, as far as macOS reports `.background` (D-10); CTest `CalcU59MacAttach`, Checkpoint |

Coverage: AC-4 (H2, B2, S1), AC-5 (H1, H2, S5), AC-6 (H1, S1),
AC-7 (S2), AC-8 (H3, S4, S6), AC-9 (S6), AC-10 (A1, A6, B1, S3, S6),
AC-11 (A2, A3, A6, S3), AC-12 (A3, S3), AC-13 (B1), AC-14 (A4, S3),
EC-1 (A1), EC-2 (A2), EC-3 (A5, S4), EC-4 (A2). 15 of 15 assigned to
M-33. AC-1–3 were tested in M-15 and must stay green after B1's script
changes; `qa-report.md` reports them again for M-33.

Review checks of the Technical Requirements (no test): no file of the
library, the core or the app layer in the Xcode project (B1);
`EmulationRunner.*` and `Threading::Threads` gone (`git grep`); no
mutex in `core/` (`git grep`); `App/MachineModel.swift` untouched
(LIB-4, C59-8).

## 7. Tools and agent knowledge

**Today's tools.** `tools/headless/` and `tools/agentic/` drive the
core directly and do not use `SessionController` or the runner
(`tools/headless/capi.cpp`, moved onto the machine interface in M-19).
They are unchanged. `calcu` gains `attach`. The spike's `attach.py` is
not taken over: `calcu attach` replaces it.

**Agent knowledge that changes in M-33** (renames per
`docs/glossary-mapping.md`: `SessionController` → app session;
`Platform` → frontend interface; `TISessionController` → `TIAppSession`):

| File | Change |
|---|---|
| `products/calc-u-59/.claude/skills/calc-u-59-swift/SKILL.md` (lines 11, 22) | the session thread, `TIAppSession`, messages only; how to attach (`--attach-port`, `calcu attach`); Debug only |
| `products/calc-u-59/.claude/skills/calc-u-59-uitesting/SKILL.md` | `HarnessClient`, one port per simulator, `AttachTests` in `UIRegressionTests` |
| `products/calc-u-59/.claude/skills/calc-u-59-shared/SKILL.md` (lines 3, 9, 24-26, 44-45) | no threaded mode; a new Swift wrapper is a translator in `TIMessages`, not a getter |
| `products/calc-u-59/.claude/skills/calc-u-59-qt/SKILL.md` | the LIB-3 59 design's (threads named there) |
| `products/calc-u-59/CLAUDE.md` (lines 31, 82-83) | `Threading::Cooperative` only until M-34; the bridge rule of line 65 now reads "translates messages" |
| `products/calc-u-59/reference/AppArchitecture.md` (10 mentions) | the Swift app's threads, bridge and attach |
| `products/calc-u-59/reference/SharedArchitecture.md` (27), `DebugAPI.md` (21), `FrontendParity.md` (19), `NewGUIGuide.md` (5) | named in plan M-33. `SharedArchitecture.md` with LIB-17 / LIB-3 59; this design adds § "Hosting and attach" |
| `products/calc-u-59/reference/CoreArchitecture.md:834-850` (Threading Model) | the core has one thread (with M-19, if not already done) |
| memory `calc-u-59-memories.md` (intro names `SessionController`) | the term "app session" |
| `docs/test-groups.md` | the new CTest entries of §6 |

No memory file names `UITesting`, `TISessionController` or
`EmulationRunner` (checked with `grep -r` on 2026-11-14).

## 8. Golden runs and risks

- **Golden runs** (`products/calc-u-59/tests/golden-runs/`, replayed by
  `tests/golden_runs_tests.cpp`) advance the machine directly and do not
  go through the Swift host. They guard M-33 only against changes to the
  app layer that move with the session (LIB-3 59). A difference is a
  finding. The reference runs (`tests/reference_runs_tests.cpp`) drive
  `SessionController` cooperatively and stay on it until M-34.
- **The spike's results are the baseline.** FS 3 (main thread p95
  16.7 ms on iOS; one 53 ms gap on macOS during 200 ms calls; 1020 of
  1020 commands in order) and FS 4 (attach, EC-1 to EC-4, `wait` 1.5 s in
  1.53 s) were measured with `SessionController` cooperative, not with
  the library session. A slower library host call is a finding.
- **iCloud.** Today every card and constant-memory read and write goes
  through `NSFileCoordinator` on the Swift side
  (`EmulatorViewModel.swift:490-546`). Plain C++ file I/O on the session
  thread skips the coordination, so a file that is not downloaded yet
  (`.name.icloud`, `CardStorage.swift:56-60`) would read as missing.
  Therefore Swift's `TIFileAccess` goes through `TICoordinatedFiles`
  (D-6); `NSFileCoordinator` is callable from the session thread.
- **The sandbox and entitlements.** The project sets sandbox
  capabilities through build settings (`ENABLE_APP_SANDBOX`,
  `ENABLE_INCOMING_NETWORK_CONNECTIONS`, `project.pbxproj:510-513`), and
  the spike needed only the setting. Whether Xcode merges a Debug file's
  key with the setting, or one overrides the other, is not verified. The
  check reads the signed product, not the project.
- **Termination.** Whether today's app writes the constant memory at
  quit, other than through `suspend`, is not verified. If it does, the
  session thread must do it before `~SessionThread` joins.
- **Shutdown order.** The wake block holds the bridge weakly (the spike
  does too). A batch that arrives after `TIAppSession` is gone is
  dropped.
- **Port collisions.** Several simulators share the Mac's loopback (FS 4).
  `AttachTests` uses a fixed 59160, and a parallel run on two simulators
  needs two ports. `UIRegressionTests` runs on one simulator (plan §1.1).

## 9. Decided (best guess)

- **D-1 (was OQ-1; AC-11, AC-12; C59-8 AC-21).** An attached connection
  carries the replies and the harness listener's messages as text lines
  (the harness is a second listener; C59-8 AC-21 reads a stream), in the
  one line format of LIB-11 §3 attach line format. `calcu attach` drops
  the message lines unless `--messages` is given.
- **D-2 (was OQ-2; LIB-11 AC-4).** `calcu attach --json` sends a first
  line `json on`, taken by `AttachedHarness` (not a LIB-11 command), so
  the app formats JSON; no client-side conversion.
- **D-3 (was OQ-3).** Swift settings stay with the frontend on the main
  thread (`TISettings`, LIB-9 design "Threading"); the session gets the
  values it needs when it is built, and the frontend writes settings from
  the topics (the last model).
- **D-4 (was OQ-4; AC-11).** Options that do not apply are refused with a
  reason and exit status 2.
- **D-5 (was OQ-5).** `tapCardAt` and `acceptsDroppedFile` become pure
  view rules of the app layer in `TIViewRules`, called with the last
  messages' data; the strip height is part of the paper topic.
- **D-6 (was OQ-6; LIB-3 AC-30).** Today's iCloud coordination stays for
  users: `TIFileAccess` takes a `cu59::FileIO`, which the Swift bridge
  implements with `NSFileCoordinator` and the placeholder download as
  today; no new host service.
- **D-7 (was OQ-7; AC-4).** The session thread runs at
  `QOS_CLASS_USER_INITIATED`.
- **D-8 (was OQ-8).** Closed before: `<row>_acN_…` case names (LIB-1
  §2a).
- **D-9 (was OQ-9; AC-5).** The main thread's responsiveness is measured
  with Instruments for the qa-report; no probe in the app.
- **D-10 (was OQ-10; AC-8 on macOS).** The Mac app suspends when SwiftUI
  reports `.background`, as the code does today; S6 records when macOS
  reports it.
- **D-11 (hosting in the product).** The session thread and the mailbox
  are `cu59` code in `products/calc-u-59/app/` (`HostedSession` went in
  the C1 simplification). The
  library's attach endpoint depends on no product: it posts
  `AttachEvent`s to an `AttachInbox` that `cu59::SessionThread`
  implements, keeps its own outgoing line queue, and `AttachedHarness`
  gets the `AppSession` and a `callsSuspended` function.
- **D-12 (was A-8).** The factory is `cu59::makeAppSession(...)`; the
  `SessionThread` factory calls it on the session thread.
- **Not verified.** That LIB-1's test machines can make a slice take
  200 ms. H2 uses a session part made for the test (its `beforeCall`
  takes the 200 ms), so the slow call does not depend on them.

**What this design assumes of the designs drafted at the same time**
(for the orchestrator to reconcile):

(Reconciled with the LIB-3 design, 2026-11-14: A-1 to A-7 are met in
LIB-3's names; A-8 is D-12; A-9 is LIB-11's.)
- **A-1 (LIB-3, met):** `calcu::sessions::AppSession` with
  `listen(MessageSink&, HostServices) -> ListenerId`, `stopListening`,
  `send(ListenerId, Command) -> CallId` (carried out at once, the reply
  to that listener only), `ask`, `answer` and `call(WallTime now)`.
  Several listeners at once; each new listener is sent every state topic
  once, and the `SinceReset` streams from the last reset (LIB-3 §3.3).
- **A-2 (LIB-3, met):** `Command{name, args}` and `Message` (a
  `std::variant` of owning structs, payloads as
  `std::shared_ptr<const Payload>`, immutable once sent) move across
  threads; a reply carries its `CallId`.
- **A-3 (LIB-3, met with a change):** `ListenerId` is `std::uint32_t`
  (LIB-3 §3.2), not a LIB-1 `Id<Tag>`.
- **A-4 (LIB-3):** `HostServices` is a set of the services of LIB-3
  AC-26, and the session asks only a listener that stated the service.
- **A-5 (LIB-3):** suspend and continue are commands (AC-24). The
  session's emulated time is a request (AC-39). Busy and full speed are
  a state topic (AC-9).
- **A-6 (LIB-3 59, met: `cu59::TIAppPart` and its interim topics):** by M-33 the Calc-U-59 app layer supplies
  everything `TISessionController` reads today as topics (generic where
  the feature exists, else Calc-U-59 product topics), and every action
  as a command, under an adapter LIB-3 59 names. The bridge translates
  that set and gains a translator when a later step moves a topic.
- **A-7 (LIB-3 59, met: `TIFileAccess`; on the Web see LIB-15 §9, was OQ-9):** the adapter for card and constant-memory files on
  the session thread, given the save folder at creation. The constant
  memory is written when the suspend command is carried out.
- **A-8 (decided, D-12):** the factory `cu59::makeAppSession(...)` (LIB-15
  uses `makeWebAppSession` for the Web), called on the session thread.
- **A-9 (LIB-11):** a command processor that runs as a listener of an app
  session, with a time policy: serve = advance directly (LIB-11 AC-6);
  attach = wait for the host's calls (AC-14). It also has a reply
  formatter for text and JSON (AC-4) and the attach line format for
  messages (D-1). `quit` and `json on` are the attach transport's.
  `calcu` lives in `library/harness/cli/` (LIB-1 design §2a).
- **A-10 (LIB-17):** the pacer is inside the machine session. The host
  only calls about 60 times a second with `steady_clock` as the
  `WallClock`. `calcu::pacing::WallTime` / `WallClock` as in the LIB-17
  design.
- **A-11 (LIB-17 P59-5):** AC-19 on iOS and macOS reads `ticks` through
  `HarnessClient` / `calcu attach` from this design.
- **A-12 (LIB-9):** `HostDirs::saveFolder` and `chosenTraceFolder` are
  passed when the session is created. A new trace folder from the folder
  picker arrives as the answer to the host service.

Names other designs must match: Calc-U-59 `products/calc-u-59/app/`
(`cu59::SessionThread`, `Mailbox`, `makeAppSession`,
`FileIO`); `library/harness/attach/` (`AttachEndpoint`, `AttachInbox`,
`AttachEvent`, `AttachedHarness`, `attachPort`, `kDefaultAttachPort`),
in `calcu_library` with the CMake option `CALCU_ATTACH`;
`calcu attach`; launch argument `--attach-port`; bridge class
`TIAppSession`.

## Part E simplification E-8 (2026-11-17, branch `es-dead`)

`docs/reviews/e-simplification.md` §1 S-8. D-6's coordinated file access implements the
library's `calcu::media::FileAccess` (LIB-7 D-30), not `cu59::FileIO`,
which is gone: `CoordinatedFiles` in `Bridge/TIAppSession.mm` is handed as
`TIAppSetup::files`; `TIFileAccess` and the media part both use it. Its
reads and writes, the `.icloud` placeholder and `cardWritten` are
unchanged; a failed write gives "the file cannot be written." as before.
