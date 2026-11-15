#pragma once
#include "SessionController.hpp"
#include "TIFileAccess.hpp"
#include "sessions/AppSession.hpp"
#include "sessions/MachineSession.hpp"
#include "sessions/Payload.hpp"
#include "sessions/SessionPart.hpp"
#include <cstdint>
#include <deque>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// Calc-U-59's app layer in the library's app session (LIB-3 59 design §2,
// §4, §5; LIB-3 AC-28 to AC-30, AC-32, AC-33; LIB-17 59). The frontends
// (Qt6 on its GUI thread, Swift on cu59::SessionThread) host a
// calcu::sessions::AppSession made by makeAppSession(); everything they do
// is a command or a request, everything they get is a message.
//
// Adapters (design §5): the flows of SessionController run inside this part
// on a SessionController in Threading::Hosted (M-33 deviation, recorded in
// the qa-report: the part hosts the controller rather than re-implementing
// its flows; the controller goes with the Web in M-34). The machine is the
// machine session's; the controller's KeySequencer runs at its slice ends
// (TIKeySequencerHook), its freeze checks inside its advance (TIFreezeGate),
// the card wait is a WaitProbe (TICardWaitAdapter). Its Platform is this
// part: errors are notices, diagnostics log lines, the files TIFileAccess,
// the UI-thread queue a queue of this part that runs on the session's
// thread. What the change flags announced arrives as the interim product
// topics below (removed with their owners, design §5).

namespace cu59 {

namespace topics {
using calcu::sessions::TopicName;
inline const TopicName kModel{"c59.model"};
inline const TopicName kLed{"c59.led"};
inline const TopicName kCueCard{"c59.cue-card"};
inline const TopicName kPrinter{"c59.printer"};
inline const TopicName kCardReader{"c59.card-reader"};
inline const TopicName kRunState{"c59.run-state"};
inline const TopicName kTrace{"c59.trace"};
inline const TopicName kDebugLive{"c59.debug.live"};
inline const TopicName kDebugCpu{"c59.debug.cpu"};
inline const TopicName kDebugHeatmap{"c59.debug.heatmap"};
inline const TopicName kDebugLog{"c59.debug.log"};
inline const TopicName kDebugAsm{"c59.debug.asm"};
inline const TopicName kShowDebugPanel{"c59.show-debug-panel"};   // stream, History::None
} // namespace topics

// ── Payloads of the interim topics. Each is a snapshot set when today's
// change flag fires; two snapshots are equal when they are the same set
// (the serial), so a flag sends its topic once.

template <class D> struct TISnapshot : calcu::sessions::PayloadOf<D> {
    std::uint64_t serial = 0;
    bool same(const D& o) const { return serial == o.serial; }
};

struct TIModelState final : TISnapshot<TIModelState> {
    static constexpr calcu::sessions::PayloadKind kKind{"c59.model"};
    Model model = Model::TI59;
    std::string moduleID;
    std::vector<ModuleMetadata> modules;
    calcu::sessions::Value toValue() const override;
};
struct TILedState final : TISnapshot<TILedState> {
    static constexpr calcu::sessions::PayloadKind kKind{"c59.led"};
    LedFrame led;
    double calcIndicatorOpacity = 0;
    calcu::sessions::Value toValue() const override;
};
struct TICueCardState final : TISnapshot<TICueCardState> {
    static constexpr calcu::sessions::PayloadKind kKind{"c59.cue-card"};
    std::optional<CueCardContent> card;
    calcu::sessions::Value toValue() const override;
};
struct TIPrinterState final : TISnapshot<TIPrinterState> {
    static constexpr calcu::sessions::PayloadKind kKind{"c59.printer"};
    std::vector<std::string> lines;
    std::vector<PrinterCodeLine> codeLines;
    double stripHeightUnits = 0;
    bool connected = true;
    bool trace = false;
    calcu::sessions::Value toValue() const override;
};
struct TICardReaderState final : TISnapshot<TICardReaderState> {
    static constexpr calcu::sessions::PayloadKind kKind{"c59.card-reader"};
    SessionController::CardState state = SessionController::CardState::NoCard;
    std::string fileName;
    bool fitted = false;
    bool onShow = false;   ///< fitted and not covered by a module's cue card (the tap rule, C59-5 D-5)
    std::optional<int> litCorner;
    calcu::sessions::Value toValue() const override;
};
struct TIRunState final : TISnapshot<TIRunState> {
    static constexpr calcu::sessions::PayloadKind kKind{"c59.run-state"};
    DebugState debug;
    bool fullSpeed = false;
    bool keystrokesPlaying = false;
    int heldKey = -1;
    calcu::sessions::Value toValue() const override;
};
struct TITraceState final : TISnapshot<TITraceState> {
    static constexpr calcu::sessions::PayloadKind kKind{"c59.trace"};
    bool manualTrace = false;
    calcu::sessions::Value toValue() const override;
};
struct TIDebugLiveState final : TISnapshot<TIDebugLiveState> {
    static constexpr calcu::sessions::PayloadKind kKind{"c59.debug.live"};
    bool enabled = false;
    LiveDebugSnapshot live;
    std::vector<StepEntry> listing;
    ProgramSource listingSource{};
    int listingCurrentIndex = -1;
    calcu::sessions::Value toValue() const override;
};
struct TIDebugCpuState final : TISnapshot<TIDebugCpuState> {
    static constexpr calcu::sessions::PayloadKind kKind{"c59.debug.cpu"};
    bool enabled = false;
    std::vector<InspectorEntry> history;
    calcu::sessions::Value toValue() const override;
};
struct TIDebugHeatmapState final : TISnapshot<TIDebugHeatmapState> {
    static constexpr calcu::sessions::PayloadKind kKind{"c59.debug.heatmap"};
    std::vector<uint32_t> counts;
    calcu::sessions::Value toValue() const override;
};
struct TIDebugLogState final : TISnapshot<TIDebugLogState> {
    static constexpr calcu::sessions::PayloadKind kKind{"c59.debug.log"};
    SessionController::DebugLevel level = SessionController::DebugLevel::Off;
    std::string text;
    calcu::sessions::Value toValue() const override;
};
struct TIDebugAsmState final : TISnapshot<TIDebugAsmState> {
    static constexpr calcu::sessions::PayloadKind kKind{"c59.debug.asm"};
    std::string fileName, status;
    int wordCount = 0;
    bool active = false;
    calcu::sessions::Value toValue() const override;
};
struct TIShowDebugPanel final : calcu::sessions::PayloadOf<TIShowDebugPanel> {
    static constexpr calcu::sessions::PayloadKind kKind{"c59.show-debug-panel"};
    explicit TIShowDebugPanel(DebugPanel p) : panel(p) {}
    DebugPanel panel;
    bool same(const TIShowDebugPanel& o) const { return panel == o.panel; }
    calcu::sessions::Value toValue() const override;
};
/// The result of a command that answers yes or no (fit-card, tap-card, …).
struct TIFlag final : calcu::sessions::PayloadOf<TIFlag> {
    static constexpr calcu::sessions::PayloadKind kKind{"c59.flag"};
    explicit TIFlag(bool v) : value(v) {}
    bool value;
    bool same(const TIFlag& o) const { return value == o.value; }
    calcu::sessions::Value toValue() const override { return calcu::sessions::Value(value); }
};
/// The reply of the request c59.drop-action.
struct TIDropAction final : calcu::sessions::PayloadOf<TIDropAction> {
    static constexpr calcu::sessions::PayloadKind kKind{"c59.drop-action"};
    explicit TIDropAction(SessionController::DropAction a) : action(a) {}
    SessionController::DropAction action;
    bool same(const TIDropAction& o) const { return action == o.action; }
    calcu::sessions::Value toValue() const override { return calcu::sessions::Value(static_cast<int>(action)); }
};

/// A notice of the part is "<title>: <message>", today's alert (LIB-3 AC-30:
/// `error` is a notice); the frontends split it for their alert until C59-8.
struct NoticeParts { std::string title, message; };
NoticeParts splitNotice(const std::string& sentence);

/// The commands of the part (`c59.` + name; arguments in the comments).
namespace commands {
using calcu::sessions::CommandName;
inline const CommandName kStartMachine{"c59.start-machine"};     // {model: 0 TI-59, 1 TI-58, 2 TI-58C}
inline const CommandName kReset{"c59.reset"};
inline const CommandName kCleanReset{"c59.clean-reset"};
inline const CommandName kPersistConstantMemory{"c59.persist-constant-memory"};
inline const CommandName kChangeModule{"c59.change-module"};     // {id} (empty: the slot emptied); an operation
inline const CommandName kPressKey{"c59.press-key"};             // {code: matrix code}
inline const CommandName kReleaseKey{"c59.release-key"};         // {code}
inline const CommandName kKeyboardDown{"c59.keyboard-down"};     // {codes: bytes}
inline const CommandName kKeyboardUp{"c59.keyboard-up"};
inline const CommandName kKeyboardRelease{"c59.keyboard-release"};
inline const CommandName kFullSpeed{"c59.full-speed"};           // {on}
inline const CommandName kPrinterConnected{"c59.printer-connected"};   // {on}; an operation
inline const CommandName kTogglePrinterConnection{"c59.toggle-printer-connection"};   // an operation
inline const CommandName kTogglePrinterTrace{"c59.toggle-printer-trace"};
inline const CommandName kPrinterPrint{"c59.printer-print"};     // {pressed}
inline const CommandName kPrinterAdv{"c59.printer-adv"};         // {pressed}
inline const CommandName kCutPaper{"c59.cut-paper"};
inline const CommandName kFitCard{"c59.fit-card"};               // {bytes (absent: unreadable), path, name} → TIFlag
inline const CommandName kFitBlankCard{"c59.fit-blank-card"};    // {path, name} → TIFlag
inline const CommandName kFeedSide{"c59.feed-side"};             // {side} → TIFlag
inline const CommandName kTapCard{"c59.tap-card"};               // {x, y} → TIFlag
inline const CommandName kEjectCard{"c59.eject-card"};
inline const CommandName kLoadState{"c59.load-state"};           // {bytes (absent: unreadable)}; an operation
inline const CommandName kOpenDropped{"c59.open-dropped"};       // {bytes, path, name} → TIFlag; a preset: an operation
inline const CommandName kLiveDebug{"c59.live-debug"};           // {on}
inline const CommandName kCpuDebug{"c59.cpu-debug"};             // {on}
inline const CommandName kFreeze{"c59.freeze"};                  // {owner: 0 CPU, 1 CALCULATOR}
inline const CommandName kResume{"c59.resume"};
inline const CommandName kArmPcChange{"c59.arm-pc-change"};
inline const CommandName kArmScanLoopExit{"c59.arm-scan-loop-exit"};
inline const CommandName kDisarmPcChange{"c59.disarm-pc-change"};
inline const CommandName kDisarmScanLoopExit{"c59.disarm-scan-loop-exit"};
inline const CommandName kStepKeycode{"c59.step-keycode"};
inline const CommandName kStepInstruction{"c59.step-instruction"};
inline const CommandName kClearHeatmap{"c59.clear-heatmap"};
inline const CommandName kCycleDebugLevel{"c59.cycle-debug-level"};
inline const CommandName kClearLog{"c59.clear-log"};
inline const CommandName kDumpVars{"c59.dump-vars"};
inline const CommandName kDumpScom{"c59.dump-scom"};
inline const CommandName kDumpProg{"c59.dump-prog"};
inline const CommandName kDumpMemory{"c59.dump-memory"};
inline const CommandName kManualTrace{"c59.manual-trace"};       // {on}
inline const CommandName kLoadAsm{"c59.load-asm"};               // {text (absent: unreadable), name}
inline const CommandName kRunAsm{"c59.run-asm"};
inline const CommandName kClearAsm{"c59.clear-asm"};
inline const calcu::sessions::RequestName kDropAction{"c59.drop-action"};   // {name} → TIDropAction
} // namespace commands

/// What the host gives the part when the session is made (C59-5 D-12, A-12).
struct TIAppSetup {
    std::shared_ptr<const calcu::roms::RomSet> roms;   ///< from lookUpRoms()
    std::string cuecards;                              ///< roms/solid-state/cuecards.txt
    std::string moduleID = kStartModuleID;
    Model startModel = Model::TI59;
    std::string saveFolder;                            ///< cards, ti58c.mem (LIB-9 Paths)
    std::shared_ptr<FileIO> fileIO;                    ///< null: plain file I/O
    SessionController::TraceLocation trace;            ///< LIB-9 Paths (traces), [trace] max-file-size
    std::function<bool()> traceAvailable;              ///< null: always
};

class TIAppPart final : public calcu::sessions::SessionPart, private SessionController::Platform {
public:
    explicit TIAppPart(TIAppSetup setup);
    ~TIAppPart();
    TIAppPart(const TIAppPart&) = delete;
    TIAppPart& operator=(const TIAppPart&) = delete;

    /// Once the session exists: the gate, the hook and the probe on its
    /// machine session, then the first machine.
    void start(calcu::sessions::AppSession&);

    // SessionPart
    void attach(calcu::sessions::SessionSetup&) override;
    void beforeCall(calcu::sessions::SessionContext&, calcu::pacing::WallTime) override;
    void afterCall(calcu::sessions::SessionContext&, calcu::pacing::WallTime) override;
    void suspending(calcu::sessions::SessionContext&) override;

    /// For the host's own adapters and the tests (LIB-3 design §3.9, context()).
    SessionController& controller() { return *m_c; }
    /// What the controller calls (LIB-3 AC-30 tests).
    SessionController::Platform& platform() { return *this; }

private:
    struct Hooks;
    using Handler = std::function<calcu::sessions::CommandOutcome(const calcu::sessions::Value&)>;
    void add(calcu::sessions::SessionSetup&, const calcu::sessions::CommandName&, Handler);
    /// After everything the controller did: its queue, its machine, the speed, the topics.
    void settle();
    void adoptMachine();
    void syncSpeed();
    void publish(uint32_t changes);
    calcu::sessions::Operation untilIdle(bool keystrokes);
    calcu::sessions::Task<calcu::sessions::OperationResult> runUntilIdle(calcu::sessions::OperationContext&, bool keystrokes);

    // SessionController::Platform
    void runOnUiThread(std::function<void()> job) override { m_queue.push_back(std::move(job)); }
    std::optional<std::string> readConstantMemory() override { return m_files.readConstantMemory(); }
    void writeConstantMemory(const std::string& text) override { m_files.writeConstantMemory(text); }
    bool writeCard(const std::string& path, const std::string& text) override { return m_files.writeCard(path, text); }
    std::string cardFilePath(const std::string& fileName) override { return m_files.cardFilePath(fileName); }
    CardRead readCard(const std::string& path) override;
    void showDebugPanel(DebugPanel panel) override;
    SessionController::TraceLocation traceLocation() override { return m_setup.trace; }
    bool traceAvailable() override { return !m_setup.traceAvailable || m_setup.traceAvailable(); }
    void saveLastUsedModel(Model) override {}       // LIB-9: the frontend writes it from c59.model (C59-5 D-3)
    void saveModuleID(const std::string&) override {}   // gone (row 59-9; LIB-3 AC-30)
    void error(const std::string& title, const std::string& message) override;
    void romsMissing(const calcu::roms::RomsMissing&) override {}   // the lookup is the host's, before the part
    void diagnostic(const std::string& message) override;
    void changed(uint32_t changes) override { m_changes |= changes; }

    TIAppSetup m_setup;
    TIFileAccess m_files;
    std::unique_ptr<SessionController> m_c;
    calcu::sessions::SessionContext* m_ctx = nullptr;
    calcu::sessions::MachineSession* m_ms = nullptr;
    std::unique_ptr<Hooks> m_hooks;
    std::deque<std::function<void()>> m_queue;
    uint32_t m_changes = 0;
    bool m_lastFull = false;
    calcu::pacing::Speed m_lastCommanded = calcu::pacing::Speed::Regular;
    std::uint64_t m_serial = 0;

    calcu::sessions::StateTopic* m_model = nullptr;
    calcu::sessions::StateTopic* m_led = nullptr;
    calcu::sessions::StateTopic* m_cue = nullptr;
    calcu::sessions::StateTopic* m_printer = nullptr;
    calcu::sessions::StateTopic* m_card = nullptr;
    calcu::sessions::StateTopic* m_run = nullptr;
    calcu::sessions::StateTopic* m_trace = nullptr;
    calcu::sessions::StateTopic* m_live = nullptr;
    calcu::sessions::StateTopic* m_cpu = nullptr;
    calcu::sessions::StateTopic* m_heatmap = nullptr;
    calcu::sessions::StateTopic* m_log = nullptr;
    calcu::sessions::StateTopic* m_asm = nullptr;
    calcu::sessions::StreamTopic* m_panel = nullptr;
};

/// The session of a Swift or Qt6 frontend (C59-5 D-12): the library's app
/// session with the part, paced on `clock` (the host's wall clock; the
/// steady clock's time since its epoch in the apps). Single-threaded: built,
/// called and destroyed on the host's session thread.
struct TIAppSession {
    std::unique_ptr<calcu::sessions::AppSession> session;   // destroyed after the part
    std::unique_ptr<TIAppPart> part;
    ~TIAppSession();
};
/// `before`: parts of the host's that go in front of the product's (tests).
std::unique_ptr<TIAppSession> makeAppSession(TIAppSetup setup, calcu::pacing::WallClock clock,
                                             std::optional<std::uint32_t> jitterSeed = std::nullopt,
                                             std::vector<calcu::sessions::SessionPart*> before = {});

/// The steady clock as the session's wall clock, and back.
calcu::pacing::WallTime steadyWallTime();
std::chrono::steady_clock::time_point steadyTimeOf(calcu::pacing::WallTime);

} // namespace cu59
