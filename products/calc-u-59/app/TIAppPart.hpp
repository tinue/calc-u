#pragma once
#include "CardFile.hpp"
#include "CueCard.hpp"
#include "DebugModel.hpp"
#include "EmulatorSession.hpp"
#include "ModuleCatalog.hpp"
#include "ProductRoms.hpp"
#include "StateFile.hpp"
#include "TIFileAccess.hpp"
#include "sessions/AppSession.hpp"
#include "sessions/MachineSession.hpp"
#include "sessions/Payload.hpp"
#include "sessions/SessionPart.hpp"
#include <chrono>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <functional>
#include <map>
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
// The part holds the product's flows (2.3.0's SessionController, taken
// apart in M-34 when the Web frontend moved onto the session too): starting
// and switching machines, presets, the TI-58C constant memory, the card
// reader, the printer strip, the cue card shown, freeze and step, traces,
// the ASM overlay and the LOG tab. Adapters (design §5): the machine is the
// machine session's; the KeySequencer runs at its slice ends
// (TIKeySequencerHook), the freeze checks inside its advance (TIFreezeGate),
// the card wait is a WaitProbe (TICardWaitAdapter). Errors are notices,
// diagnostics log lines, the files TIFileAccess; what the EmulatorSession
// reports waits in a queue of this part that runs on the session's thread
// after the call. What 2.3.0's change flags announced arrives as the interim
// product topics below (removed with their owners, design §5).

namespace cu59 {

/// What the card reader does now.
enum class CardState { NoCard, Swiping };
/// The debug level of the LOG tab (cycled by c59.cycle-debug-level).
enum class DebugLevel { Off = 0, Info = 1, Debug = 2 };
/// What a file dropped on the calculator does (reference/SharedArchitecture.md § "Dropped Files").
enum class DropAction {
    LoadPreset,   ///< as the Preset button
    FitCard,      ///< fitted into the calculator's card cutout
    Ignore,       ///< a card, and this model has no reader
    NotAccepted,  ///< not a preset or a card
};
/// The pure rule: by the file's name and the model (C59-5 D-5, a view rule).
DropAction dropActionFor(const std::string& fileName, Model model);
/// Where manual and scripted traces go (LIB-9 Paths), and the size limit per trace session.
struct TraceTarget {
    std::filesystem::path dir;
    uint64_t maxBytes = 0;
};

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
    CardState state = CardState::NoCard;
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
    DebugLevel level = DebugLevel::Off;
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
    explicit TIDropAction(DropAction a) : action(a) {}
    DropAction action;
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
    TraceTarget trace;                                 ///< LIB-9 Paths (traces), [trace] max-file-size
    std::function<bool()> traceAvailable;              ///< null: always
};

class TIAppPart final : public calcu::sessions::SessionPart {
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

    // ── The flows, as the commands run them. Public for the host's own
    // adapters and the tests (LIB-3 design §3.9); a frontend sends the
    // commands. A test that calls one directly runs the session after it.

    // Machine
    /// Build a fresh machine of `model` from the ROM set (it cannot fail for a
    /// missing ROM, LIB-12 AC-6). Always true.
    bool startMachine(Model model);
    bool hasMachine() const { return m_session != nullptr; }
    Model model() const { return m_model; }
    /// The machine's runtime (freeze state, keys, traces); the machine itself
    /// is the machine session's.
    EmulatorSession* emulator() { return m_session.get(); }
    void resetMachine();
    void cleanResetMachine();
    /// TI-58C: write ti58c.mem now if its contents changed (also debounced in afterCall()).
    void persistConstantMemory();
    const std::string& moduleID() const { return m_moduleID; }
    /// The module of that ID into `slot-1` (a replace), then the power cycle
    /// that follows (C59-6 AC-11; row 59-11); false for an ID that is no
    /// module, or when the attach is refused (EC-1), and nothing changes.
    bool selectModule(const std::string& moduleID);
    /// The Settings choice: a different module (empty: the slot emptied),
    /// then the power cycle.
    void changeModule(const std::string& moduleID);
    /// The 14 modules from their module files, in menu order (C59-6 AC-8).
    std::vector<ModuleMetadata> allModules() const { return allModuleMetadata(*m_setup.roms); }

    // Display, keys, speed
    const LedFrame& display() const { return m_display; }
    double calcIndicatorOpacity() const { return m_calcOpacity; }
    void pressKey(uint8_t matrixCode);
    void releaseKey(uint8_t matrixCode);
    void keyboardDown(const std::vector<uint8_t>& codes);
    void keyboardUp();
    void keyboardRelease();
    int heldKey() const { return m_heldKey; }  ///< row×5 + col shown pressed, -1 none
    void setFullSpeed(bool on);
    bool fullSpeed() const { return m_fullSpeed; }
    bool keystrokesPlaying() const { return m_keystrokesPlaying; }
    /// The banks of the in-file (`mem:`) cards as they are now, by name
    /// (read-only; the reference runs of M-9 record what an example wrote).
    std::vector<std::pair<std::string, std::vector<uint8_t>>> memoryCardBanks() const {
        std::vector<std::pair<std::string, std::vector<uint8_t>>> out;
        for (const auto& [name, card] : m_memoryCards) out.emplace_back(name, card.bank);
        return out;
    }

    // Cue card
    const std::optional<CueCardContent>& cueCard() const { return m_cueCard; }

    // Printer
    const std::vector<std::string>& printerLines() const { return m_printerLines; }
    const std::vector<PrinterCodeLine>& printerCodeLines() const { return m_printerCodeLines; }
    /// The strip's height in dot pitches (a text line without dot codes yet counts as complete).
    double printerStripHeightUnits() const;
    bool printerConnected() const { return m_printerConnected; }
    /// TRACE's position: the app session keeps it from the start (off) to the
    /// end, through reset, power cycle, cold boot, model switch and detach;
    /// never saved (C59-6 AC-6; row 59-25). A state file sets it off.
    bool printerTrace() const { return m_printerTrace; }
    /// Attach or detach the PC-100C, then the power cycle that follows (row
    /// 59-11). The paper stays (AC-1); a line cut by the detach keeps its rows (EC-2).
    void setPrinterConnected(bool connected);
    void togglePrinterConnection();
    void togglePrinterTrace();
    void pressPrinterPrint(bool pressed);
    void pressPrinterAdv(bool pressed);
    void cutPaper();

    // Card reader (reference/SharedArchitecture.md § "Card Reader Flow")
    CardState cardState() const { return m_pass ? CardState::Swiping : CardState::NoCard; }
    const std::string& cardFileName() const { return m_cardFileName; }
    /// Fit a card file into the cutout, replacing the card there (nullopt bytes:
    /// it could not be read). Nothing goes into the reader yet. Written data goes
    /// back to `path`. False if it could not be fitted, or a card is in the reader.
    bool fitCard(const std::optional<std::vector<uint8_t>>& raw, const std::string& path, const std::string& fileName);
    /// Fit a fresh card; `savePath` is created or replaced by its first write.
    bool fitBlankCard(const std::string& savePath, const std::string& fileName);
    /// Feed a side (0 = top-left corner, 1 = top-right) of the fitted card into the reader.
    bool feedSide(int side);
    /// A press at canvas position (nx, ny): feeds a side if it hits a corner of
    /// the fitted card on show. True if the press hit such a corner.
    bool tapCard(double nx, double ny);
    bool cardFitted() const { return m_fitted.has_value(); }
    /// A fitted card, not covered by a module's cue card: a tap on a corner feeds it.
    bool cardOnShow() const { return m_fitted && m_activeProgram <= 0; }
    /// The corner to draw lit: the side of the fitted card in the reader, if that card is on show.
    std::optional<int> litCardCorner() const;
    void ejectCard();

    // State files and dropped files
    /// A preset's bytes (nullopt: it could not be read).
    void loadStateFile(const std::optional<std::vector<uint8_t>>& raw);
    /// By the file's name and the current model (dropActionFor).
    DropAction dropAction(const std::string& fileName) const { return dropActionFor(fileName, m_model); }
    /// Load the preset or insert the card; false when nothing was done (a card
    /// is already in the reader, the file could not be read, or dropAction
    /// says no). A write to the card goes back to `path`.
    bool openDroppedFile(const std::optional<std::vector<uint8_t>>& raw, const std::string& path,
                         const std::string& fileName);

    // Debugger
    void setLiveDebugEnabled(bool on);
    void setCpuDebugEnabled(bool on);
    bool liveDebugEnabled() const { return m_liveDebugEnabled; }
    bool cpuDebugEnabled() const { return m_cpuDebugEnabled; }
    DebugState debugState() const { return m_debugState; }
    bool isFrozen() const { return m_debugState.frozen; }
    void freeze(FreezeOwner owner);
    void resume();
    void armPcChange();
    void armScanLoopExit();
    void disarmPcChange();
    void disarmScanLoopExit();
    void stepKeycode();
    void stepInstruction();
    const DebugModel& debug() const { return m_debug; }
    void clearHeatmap();
    DebugLevel debugLevel() const { return m_debugLevel; }
    void cycleDebugLevel();
    const std::string& logText() const { return m_logText; }
    void clearLog();
    void dumpVars();
    void dumpScom();
    void dumpProg();
    void dumpMemory();
    bool manualTrace() const { return m_manualTrace; }
    void setManualTrace(bool on);

    // ASM overlay
    /// An overlay file's text (nullopt: it could not be read).
    void loadAsm(const std::optional<std::string>& text, const std::string& fileName);
    void runAsm();
    void clearAsm();
    const std::string& asmFileName() const { return m_asmFileName; }
    int asmWordCount() const { return static_cast<int>(m_asmWords.size()); }
    const std::string& asmStatus() const { return m_asmStatus; }
    bool asmActive() const { return m_asmActive; }

    /// What changed since the last publish: each bit moves its interim topic.
    enum Change : uint32_t {
        ModelChanged = 1u << 0,     ///< model, module, machine replaced or reset (card reader emptied)
        DisplayChanged = 1u << 1,   ///< display digits or the C indicator
        CueCardChanged = 1u << 2,
        PrinterChanged = 1u << 3,   ///< lines, connection, TRACE latch
        PaperCut = 1u << 4,         ///< the strip was cleared
        CardChanged = 1u << 5,      ///< card reader state or file name
        RunStateChanged = 1u << 6,  ///< freeze, armed freezes, full speed, playback, held key
        LiveChanged = 1u << 7,      ///< CALCULATOR tab snapshot
        CpuChanged = 1u << 8,       ///< CPU tab history
        HeatmapChanged = 1u << 9,
        LogChanged = 1u << 10,
        AsmChanged = 1u << 11,
        TraceChanged = 1u << 12,    ///< manual trace on/off
        ListingChanged = 1u << 13,  ///< CALCULATOR tab program listing (rebuilt when frozen)
    };
    /// Told the changes each time the interim topics are published, on the
    /// session's thread: another part of the same session that draws its own
    /// form of them (the Web's TIWebPart, LIB-15 59).
    void setChangeObserver(std::function<void(uint32_t)> observer) { m_observer = std::move(observer); }
    /// A preset's operation: it ends when its KEYSTROKES have played (AC-32);
    /// for a part that loads a preset as the start (the Web's `start`).
    /// The session was destroyed before the part (the Web engine's
    /// WebSession destroys the session first): the EmulatorSession, whose
    /// machine went with it, is let go without touching it.
    void forgetSession();
    /// The machine a load built joins the machine session first.
    calcu::sessions::Operation presetOperation();

    // What the flows report (LIB-3 AC-30)
    /// An error: the notice "<title>: <message>" (today's alert).
    void error(const std::string& title, const std::string& message);
    /// Developer diagnostics: a line of the log.
    void diagnostic(const std::string& message);
    /// The card and constant-memory files (TIFileAccess, on the save folder).
    TIFileAccess& files() { return m_files; }

private:
    static uint32_t changeIf(bool on, Change bit) { return on ? static_cast<uint32_t>(bit) : 0u; }
    void emitChanged(uint32_t c) { m_changes |= c; }

    struct Hooks;
    struct Events;
    using Handler = std::function<calcu::sessions::CommandOutcome(const calcu::sessions::Value&)>;
    void add(calcu::sessions::SessionSetup&, const calcu::sessions::CommandName&, Handler);
    /// After everything a flow did: the queue, the machine, the speed, the topics.
    void settle();
    void adoptMachine();
    void syncSpeed();
    void publish(uint32_t changes);
    calcu::sessions::Operation untilIdle(bool keystrokes);
    calcu::sessions::Task<calcu::sessions::OperationResult> runUntilIdle(calcu::sessions::OperationContext&, bool keystrokes);
    /// The 60 Hz duties after a host call: display, printer, log, card
    /// auto-eject, the debugger, traces, and the slower checks (cue-card
    /// program 2 Hz, LOG text 1 Hz, TI-58C persist debounce 0.5 s).
    void frame(std::chrono::steady_clock::time_point now);
    /// Run `f` after the call unless the EmulatorSession that asked has been replaced by then.
    void later(uint64_t sessionGeneration, std::function<void()> f);

    /// Shared start of both resets: stop playback, unfreeze, drop the overlay, card and cue card.
    void beginReset();
    void resetStopped();
    void refreshCueCard();
    void checkProgramNumber();
    void refreshLog();
    void log(const std::vector<std::string>& lines);
    std::optional<std::vector<uint8_t>> loadConstantMemory();
    void applyState(const LoadStateResult& parsed);
    void configureTraceLocation();
    void onFrozenView(uint64_t freezeGeneration, FrozenViewReason reason);
    void pullDebugState();
    void showDebugPanel(DebugPanel panel);
    /// The card in the reader now.
    struct CardPass {
        int side = 0;          ///< of the fitted card
        bool queued = false;   ///< a fitted card fed by ArmCard: / InsertCard:
        std::optional<std::string> memoryKey;  ///< a mem: card: a written bank goes to m_memoryCards
    };
    /// No card fitted or in the reader, no cue card, no program shown.
    void clearCardAndCue();
    /// A preset's cue card takes over the cutout, from any fitted card.
    void adoptPresetCue(const std::optional<CueCardContent>& cue);
    bool canFit() const;
    void fit(const std::string& path, const std::string& fileName, CardDocument doc, bool fresh);
    bool feed(int side, bool queued);
    /// The machine wrote `written` to a side of the fitted card: merge and save it.
    void writeFittedSide(int side, const std::vector<uint8_t>& written, bool queued);
    /// What the cutout shows for the fitted card: its cue card with the corners' numbers.
    CueCardContent fittedCueCard() const;
    /// Put a bank image (empty: a blank card) into the reader.
    void startSwipe(const std::vector<uint8_t>& data, const CardPass& pass);
    /// KEYSTROKES ArmCard: / InsertCard: — feed the next queued card when the machine takes it.
    void feedQueuedCard();
    /// A preset's card has left the reader (or never got in): tell playback if it waits.
    void queuedCardDone();
    /// A new preset or a reset: the previous preset's queued and mem: cards go (logged if unused).
    void dropPresetCards();
    void applyReloadStage(const ReloadStage& stage);
    /// A KEYSTROKES GUI control (PrinterTrace:, Freeze:, …), as its button does.
    void applyControl(const KeystrokeEvent& event);

    /// What changing the places did (C59-6).
    struct PlaceChange {
        std::optional<std::string> refusal;
        bool powerCycle = false;      ///< the pin-out says one follows
        bool moduleChanged = false, printerChanged = false;
        std::vector<std::string> lines;
        std::vector<PrinterCodeLine> codeLines;
    };
    std::optional<std::string> currentModule() const;
    /// The module and the printer as asked; stops at a refusal.
    PlaceChange changePlacesNow(const std::optional<std::string>& module, bool printer);
    /// The IDs, the cue cards, the debugger's module, the paper.
    void adoptPlaces(const PlaceChange&, const std::optional<std::string>& module, bool printer);
    /// The whole change: the places, the power cycle, the bookkeeping. False
    /// on a refusal (nothing changed).
    bool changePlaces(const std::optional<std::string>& module, bool printer);

    TIAppSetup m_setup;
    TIFileAccess m_files;
    calcu::sessions::SessionContext* m_ctx = nullptr;
    calcu::sessions::MachineSession* m_ms = nullptr;
    std::unique_ptr<Hooks> m_hooks;
    std::function<void(uint32_t)> m_observer;
    std::deque<std::function<void()>> m_queue;
    uint32_t m_changes = 0;
    bool m_lastFull = false;
    calcu::pacing::Speed m_lastCommanded = calcu::pacing::Speed::Regular;
    std::uint64_t m_serial = 0;

    std::unique_ptr<Events> m_events;
    std::unique_ptr<EmulatorSession> m_session;
    uint64_t m_sessionGeneration = 0;
    Model m_model = Model::TI59;

    std::string m_moduleID;
    std::map<int, CueCardContent> m_moduleCards;
    ModuleMetadata m_moduleMetadata;
    std::optional<CueCardContent> m_cueCard;
    std::optional<CueCardContent> m_userCueCard;
    int m_activeProgram = 0;

    LedFrame m_display{};
    double m_calcOpacity = 0;
    bool m_fullSpeed = false;
    bool m_keystrokesPlaying = false;
    int m_heldKey = -1;

    std::vector<std::string> m_printerLines;
    std::vector<PrinterCodeLine> m_printerCodeLines;
    bool m_printerConnected = true;
    bool m_printerTrace = false;   ///< C59-6 AC-6

    std::string m_cardFileName = "card.U59";  ///< the fitted card's, or the last one's in the reader
    /// The card in the cutout, while it is out of its sleeve.
    struct FittedCard {
        std::string path;
        CardDocument doc;
        bool fresh = false;  ///< no write yet: the first one replaces whatever is at `path`
    };
    std::optional<FittedCard> m_fitted;
    std::optional<CueCardContent> m_presetCue;  ///< the preset's own cue card: goes onto cards it writes
    std::optional<CardPass> m_pass;
    /// mem: cards of the loaded preset, by the name after "mem:": the bank
    /// last written to it, and the cue card shown then.
    struct MemoryCard { std::vector<uint8_t> bank; std::optional<CueCardContent> cueCard; };
    std::map<std::string, MemoryCard> m_memoryCards;
    bool m_queuedCardInReader = false;                ///< an InsertCard: card: playback waits for it

    bool m_liveDebugEnabled = false;
    bool m_cpuDebugEnabled = false;
    DebugState m_debugState;
    DebugModel m_debug;
    DebugLevel m_debugLevel = DebugLevel::Off;
    std::string m_logText;
    bool m_manualTrace = false;

    std::vector<uint16_t> m_asmWords;
    std::string m_asmFileName = "No file selected";
    std::string m_asmStatus = "Load a hex opcode file and press Run.";
    bool m_asmActive = false;

    using Clock = std::chrono::steady_clock;
    Clock::time_point m_nextProgramCheck, m_nextLogRefresh;
    std::optional<Clock::time_point> m_persistDue;
    std::string m_persistedText;

    calcu::sessions::StateTopic* m_modelTopic = nullptr;
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

/// The session of a frontend (C59-5 D-12; the Web's through
/// makeWebAppSession, LIB-15): the library's app session with the part,
/// paced on `clock` (the host's wall clock; the steady clock's time since its
/// epoch in the apps). Single-threaded: built, called and destroyed on the
/// host's session thread.
struct TIAppSession {
    std::unique_ptr<calcu::sessions::AppSession> session;   // destroyed after the part
    std::unique_ptr<TIAppPart> part;
    ~TIAppSession();
};
/// `before`: parts of the host's that go in front of the product's (the
/// Web's shell part, tests); `after`: parts behind it (the Web's TIWebPart).
std::unique_ptr<TIAppSession> makeAppSession(TIAppSetup setup, calcu::pacing::WallClock clock,
                                             std::optional<std::uint32_t> jitterSeed = std::nullopt,
                                             std::vector<calcu::sessions::SessionPart*> before = {},
                                             std::vector<calcu::sessions::SessionPart*> after = {});

/// The steady clock as the session's wall clock, and back.
calcu::pacing::WallTime steadyWallTime();
std::chrono::steady_clock::time_point steadyTimeOf(calcu::pacing::WallTime);

} // namespace cu59
