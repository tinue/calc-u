#pragma once
#include "CardFile.hpp"
#include "ConstantMemoryKeeper.hpp"
#include "CueCard.hpp"
#include "TIDebugPart.hpp"
#include "TIFileKinds.hpp"
#include "EmulatorSession.hpp"
#include "ModuleCatalog.hpp"
#include "ProductRoms.hpp"
#include "TIFileAccess.hpp"
#include "TILevelRules.hpp"
#include "TIPresetCommands.hpp"
#include "TIPresetRules.hpp"
#include "TIProductMachines.hpp"
#include "TITyping.hpp"
#include "debugger/DebuggerPart.hpp"
#include "paper/PaperPart.hpp"
#include "parts/LibraryParts.hpp"
#include "presets/Source.hpp"
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
// Since M-54 the part owns the library's parts of its session
// (parts::LibraryParts; LIB-6 D-40, as Calc-U-1600's SharpAppPart): LIB-5's
// typing, LIB-2's peripherals, LIB-6's presets on the TI rules and keywords
// (app/TIPresetRules, app/TIPresetKeys; the bundle of examples/), LIB-18's
// start, model switch, Reset, Cold boot, attach and detach on
// app/TIProductMachines, LIB-8's paper, LIB-10's debugger; then the program
// memory commands (app/TIPresetCommands, C59-2 AC-9) and this part. The
// machine is built by LIB-18's machine control and held by the machine
// session; the part follows each new machine (machineChanged): the runtime
// around it (EmulatorSession: the frame sampler, the debug panel's reads),
// the module and the printer read from what is plugged (LIB-18 D-26), the
// own cue card, the TI-58C's constant memory file (ConstantMemoryKeeper,
// C59-3). What stays the part's: the card reader (until M-55), the PC-100C's
// TRACE position (C59-6 AC-6), the cue card shown. The debug panel is
// TIDebugPart's (C59-4, M-56), a part of the same session it owns; the file
// kinds of LIB-13's `open` are TIFileKinds (M-56). The card wait is a WaitProbe
// (TICardWaitAdapter). Errors are notices, diagnostics log lines. What
// 2.3.0's change flags announced arrives as the interim product topics below
// (removed with their owners, design §5).

namespace cu59 {

/// What the card reader does now.
enum class CardState { NoCard, Swiping };
/// Where manual and scripted traces go (LIB-9 Paths), and the size limit per trace session.
/// An empty dir: this host has no trace folder (the Web: traces are handed
/// over with KeepFile, LIB-10 AC-26). maxBytes / 1'000'000: the limit in
/// MiB of the library's trace session (LIB-10 T2).
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
/// The PC-100C's place and TRACE (LIB-8 adapter P-E1, until M-57): the
/// paper panel's owner and the connect toggle above it; the paper itself is
/// the library's `paper PC-100C`.
struct TIPrinterState final : TISnapshot<TIPrinterState> {
    static constexpr calcu::sessions::PayloadKind kKind{"c59.printer"};
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
/// What no debugger state is (LIB-10 adapter L-E5, until M-57): the run
/// state is the library's `run-state`; `keystrokes-playing` says that an
/// operation runs (a preset, a build; LIB-6 replaced the KEYSTROKES playback).
struct TIRunState final : TISnapshot<TIRunState> {
    static constexpr calcu::sessions::PayloadKind kKind{"c59.run-state"};
    bool fullSpeed = false;
    bool keystrokesPlaying = false;
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
/// A notice of the part is "<title>: <message>", today's alert (LIB-3 AC-30:
/// `error` is a notice); the frontends split it for their alert until C59-8.
struct NoticeParts { std::string title, message; };
NoticeParts splitNotice(const std::string& sentence);

/// The commands of the part (`c59.` + name; arguments in the comments).
namespace commands {
using calcu::sessions::CommandName;
inline const CommandName kFullSpeed{"c59.full-speed"};           // {on}
// LIB-8 P-E2 (until M-57): `set-control {peripheral: "PC-100C", control: PRINT|ADV|TRACE, on}`,
// the library's command, as the paper panel sends it.
inline const CommandName kSetControl{"set-control"};
inline const CommandName kFitCard{"c59.fit-card"};               // {bytes (absent: unreadable), path, name} → TIFlag
inline const CommandName kFitBlankCard{"c59.fit-blank-card"};    // {path, name} → TIFlag
inline const CommandName kFeedSide{"c59.feed-side"};             // {side} → TIFlag
inline const CommandName kTapCard{"c59.tap-card"};               // {x, y} → TIFlag
inline const CommandName kEjectCard{"c59.eject-card"};
} // namespace commands

/// What the host gives the part when the session is made (C59-5 D-12, A-12).
struct TIAppSetup {
    std::shared_ptr<const calcu::roms::RomSet> roms;   ///< from lookUpRoms()
    std::string cuecards;                              ///< roms/solid-state/cuecards.txt
    std::string saveFolder;                            ///< cards, ti58c.memory.yaml (LIB-9 Paths)
    std::shared_ptr<FileIO> fileIO;                    ///< null: plain file I/O
    TraceTarget trace;                                 ///< LIB-9 Paths (traces), [trace] max-file-size
    std::function<bool()> traceAvailable;              ///< null: always
    /// The product's settings (LIB-9; the startup model, `last-model`, the
    /// startup presets [preset]); null: the defaults, kept in memory.
    calcu::settings::SettingsFile* settings = nullptr;
    /// The bundled examples (examples/ as bundle.yaml lists them; LIB-6
    /// AC-31, AC-33); empty: none (a bundle path is refused).
    std::string examplesFolder;
    std::string home;                                  ///< `~/` of a preset (README §3)
    std::function<std::optional<std::string>()> screenshotFolder;   ///< LIB-6 AC-25; unset: the save folder
    calcu::sessions::Hosting hosting = calcu::sessions::Hosting::Paced;
};

class TIAppPart final : public calcu::sessions::SessionPart {
public:
    explicit TIAppPart(TIAppSetup setup);
    ~TIAppPart();
    TIAppPart(const TIAppPart&) = delete;
    TIAppPart& operator=(const TIAppPart&) = delete;

    /// The library's parts (LIB-6 D-40: built by parts::LibraryParts), the
    /// program memory commands (C59-2 AC-9), then this part, in the session's
    /// order. The host builds its AppSessionSetup from them.
    std::vector<calcu::sessions::SessionPart*> sessionParts();
    calcu::parts::LibraryParts& libraryParts() { return m_lib; }

    // SessionPart.
    void attach(calcu::sessions::SessionSetup&) override;
    void beforeCall(calcu::sessions::SessionContext&, calcu::pacing::WallTime) override;
    void afterCall(calcu::sessions::SessionContext&, calcu::pacing::WallTime) override;
    void suspending(calcu::sessions::SessionContext&) override;
    void machineChanged(calcu::sessions::SessionContext&) override;
    void machineRestarted(calcu::sessions::SessionContext&, calcu::sessions::Restart) override;
    void operationEnded(calcu::sessions::SessionContext&) override;
    void beforeMachineChange(calcu::sessions::SessionContext&) override;
    void closing(calcu::sessions::SessionContext&) override;

    // ── The flows, as the commands run them. Public for the host's own
    // adapters and the tests (LIB-3 design §3.9); a frontend sends the
    // commands. A test that calls one directly runs the session after it.

    // Machine (built and held by LIB-18's machine control and the machine
    // session; the start, the model switch, Reset, Cold boot, the module and
    // the printer are LIB-18's commands: `start`, `model`, `reset`,
    // `cold-boot`, `attach`, `detach`)
    bool hasMachine() const { return m_session != nullptr; }
    Model model() const { return m_model; }
    /// The machine's runtime (frames, the debug panel's reads); the machine
    /// itself is the machine session's.
    EmulatorSession* emulator() { return m_session.get(); }
    /// The module in `slot-1` (empty: none), as the machine says (D-26).
    const std::string& moduleID() const { return m_moduleID; }
    /// The 14 modules from their module files, in menu order (C59-6 AC-8).
    std::vector<ModuleMetadata> allModules() const { return allModuleMetadata(*m_setup.roms); }

    // Display, keys, speed
    const LedFrame& display() const { return m_display; }
    double calcIndicatorOpacity() const { return m_calcOpacity; }
    calcu::typing::TypingPart* typing() { return m_lib.typing(); }
    /// Full speed as the display's hold gesture asks (c59.full-speed): the
    /// session's commanded speed.
    void setFullSpeed(bool on);
    bool fullSpeed() const { return m_fullSpeed; }
    /// An operation runs (a preset, a build, a plug change): the field the
    /// interim topic `c59.run-state` names keystrokes-playing.
    bool keystrokesPlaying() const { return m_operationRuns; }

    // Cue card
    const std::optional<CueCardContent>& cueCard() const { return m_cueCard; }
    /// The machine's own card (a preset's `cue-card:`, the constant memory
    /// file's; C59-2 AC-6, C59-3 AC-7), under a module program's card.
    const std::optional<CueCardContent>& ownCueCard() const { return m_userCueCard; }

    // Printer (the paper: LIB-8's PaperPart)
    bool printerConnected() const { return m_printerConnected; }
    /// TRACE's position: kept by the app session from the start (off) to the
    /// end, through reset, power cycle, cold boot, detach and attach; never
    /// saved (C59-6 AC-6; row 59-25); a preset's build sets it as the preset
    /// says (LIB-18 D-27).
    bool printerTrace() const { return m_printerTrace; }
    /// A control of the PC-100C as LIB-2 names it: PRINT, ADV (keys), TRACE
    /// (the switch the app session keeps). False: no such control.
    bool setPrinterControl(const std::string& control, bool on);
    calcu::paper::PaperPart* paperPart() { return m_lib.paper(); }
    calcu::debugger::DebuggerPart* debuggerPart() { return m_lib.debugger(); }
    calcu::presets::PresetsPart* presetsPart() { return m_lib.presets(); }
    TILevelRules* levelRules() { return &m_levelRules; }
    /// The debug panel (C59-4).
    TIDebugPart& debugPart() { return m_debugPart; }

    // Card reader (reference/SharedArchitecture.md § "Card Reader Flow"; M-55: C59-7)
    CardState cardState() const { return m_pass ? CardState::Swiping : CardState::NoCard; }
    const std::string& cardFileName() const { return m_cardFileName; }
    bool fitCard(const std::optional<std::vector<uint8_t>>& raw, const std::string& path, const std::string& fileName);
    bool fitBlankCard(const std::string& savePath, const std::string& fileName);
    bool feedSide(int side);
    bool tapCard(double nx, double ny);
    bool cardFitted() const { return m_fitted.has_value(); }
    bool cardOnShow() const { return m_fitted && m_activeProgram <= 0; }
    std::optional<int> litCardCorner() const;
    void ejectCard();

    // Debugger
    calcu::debugger::RunState runState() const;
    bool isFrozen() const;
    /// What changed since the last publish: each bit moves its interim topic.
    enum Change : uint32_t {
        ModelChanged = 1u << 0,     ///< model, module, machine replaced or reset (card reader emptied)
        DisplayChanged = 1u << 1,   ///< display digits or the C indicator
        CueCardChanged = 1u << 2,
        PrinterChanged = 1u << 3,   ///< connection, TRACE latch
        CardChanged = 1u << 5,      ///< card reader state or file name
        RunStateChanged = 1u << 6,  ///< full speed, an operation runs, the run control's state
    };
    /// Told the changes each time the interim topics are published, on the
    /// session's thread: another part of the same session that draws its own
    /// form of them (the Web's TIWebPart, LIB-15 59).
    void setChangeObserver(std::function<void(uint32_t)> observer) { m_observer = std::move(observer); }

    // What the flows report (LIB-3 AC-30)
    /// An error: the notice "<title>: <message>" (today's alert).
    void error(const std::string& title, const std::string& message);
    /// Developer diagnostics: a line of the log.
    void diagnostic(const std::string& message);
    /// The card and constant-memory files (TIFileAccess, on the save folder).
    TIFileAccess& files() { return m_files; }
    ConstantMemoryKeeper& constantMemory() { return m_keeper; }

private:
    static uint32_t changeIf(bool on, Change bit) { return on ? static_cast<uint32_t>(bit) : 0u; }
    void emitChanged(uint32_t c) { m_changes |= c; }
    calcu::parts::LibraryPartsSetup librarySetup();   // the constructor's, on the members before m_lib

    struct Hooks;
    using Handler = std::function<calcu::sessions::CommandOutcome(const calcu::sessions::Value&)>;
    void add(calcu::sessions::SessionSetup&, const calcu::sessions::CommandName&, Handler);
    /// After everything a flow did: the queue, the machine, the speed, the topics.
    void settle();
    /// A new machine of the session (LIB-18 §11.3): the runtime around it, the
    /// plugs read from it, the own cue card, the constant memory keeper.
    void followMachine();
    /// What is plugged now: the module (and its cards), the printer, TRACE.
    void followPlugs(bool newMachine);
    void syncSpeed();
    void publish(uint32_t changes);
    /// The 60 Hz duties after a host call: display, card auto-eject, and the
    /// cue-card program check (2 Hz).
    void frame(std::chrono::steady_clock::time_point now);
    /// `cue-card:` of a preset (C59-2 AC-6): the machine's own card.
    void fitOwnCueCard(const CueCardContent& card);

    void refreshCueCard();
    void checkProgramNumber();
    void followRunState();
    calcu::debugger::RunControl* runControl() const;
    calcu::machine::Machine* machine() const;
    /// The card in the reader now.
    struct CardPass {
        int side = 0;          ///< of the fitted card
    };
    bool canFit() const;
    /// LIB-13's `open` of a card file until M-55 (TIFileKinds' interim kind): the refusal, or none.
    std::optional<std::string> openCard(const calcu::opening::OpenedFile& f);
    void fit(const std::string& path, const std::string& fileName, CardDocument doc, bool fresh);
    bool feed(int side);
    void writeFittedSide(int side, const std::vector<uint8_t>& written);
    CueCardContent fittedCueCard() const;
    void startSwipe(const std::vector<uint8_t>& data, const CardPass& pass);

    TIAppSetup m_setup;
    TIFileAccess m_files;
    std::optional<calcu::settings::SettingsFile> m_ownSettings;
    calcu::sessions::SessionContext* m_ctx = nullptr;
    calcu::sessions::MachineSession* m_ms = nullptr;
    std::unique_ptr<Hooks> m_hooks;

    // The product's data for the library's parts (before m_lib).
    ConstantMemoryKeeper m_keeper;
    TIProductMachines m_machines;
    TIPresetRules m_rules;
    std::unique_ptr<calcu::presets::PresetBundle> m_bundle;
    TILevelRules m_levelRules;
    ProgramMemoryPart m_memoryPart;
    TIDebugPart m_debugPart;
    TIFileKinds m_fileKinds;
    calcu::parts::LibraryParts m_lib;

    std::function<void(uint32_t)> m_observer;
    uint32_t m_changes = 0;
    bool m_fullSpeed = false;
    bool m_operationRuns = false;
    std::uint64_t m_serial = 0;

    std::unique_ptr<EmulatorSession> m_session;
    std::uint64_t m_seenGeneration = 0;
    Model m_model = Model::TI59;

    std::string m_moduleID;
    std::map<int, CueCardContent> m_moduleCards;
    ModuleMetadata m_moduleMetadata;
    std::optional<CueCardContent> m_cueCard;
    std::optional<CueCardContent> m_userCueCard;
    std::uint64_t m_userCueCardGeneration = 0;   ///< the machine it was fitted on (a preset's section)
    int m_activeProgram = 0;

    LedFrame m_display{};
    double m_calcOpacity = 0;

    bool m_printerConnected = false;
    bool m_printerTrace = false;   ///< C59-6 AC-6

    std::string m_cardFileName = "card.U59";  ///< the fitted card's, or the last one's in the reader
    struct FittedCard {
        std::string path;
        CardDocument doc;
        bool fresh = false;  ///< no write yet: the first one replaces whatever is at `path`
    };
    std::optional<FittedCard> m_fitted;
    std::optional<CardPass> m_pass;

    using Clock = std::chrono::steady_clock;
    Clock::time_point m_nextProgramCheck;
    calcu::debugger::RunState m_seenRun;

    calcu::sessions::StateTopic* m_modelTopic = nullptr;
    calcu::sessions::StateTopic* m_led = nullptr;
    calcu::sessions::StateTopic* m_cue = nullptr;
    calcu::sessions::StateTopic* m_printer = nullptr;
    calcu::sessions::StateTopic* m_card = nullptr;
    calcu::sessions::StateTopic* m_run = nullptr;
};

/// The session of a native frontend (C59-5 D-12; LIB-6 D-40): the library's
/// app session with TIAppPart::sessionParts() (the library's parts, the
/// program memory commands, the part), which the session owns
/// (AppSessionSetup::owned) and lets go first, paced on `clock` (the host's
/// wall clock; the steady clock's time since its epoch in the apps), and
/// LIB-18's `start` sent once (the startup model with its startup preset, or
/// its built-in default). `before`: parts of the host's that go in front of
/// the product's (tests); `after`: parts behind it (tests of TIWebPart; the
/// Web engine builds its own, frontends/web/bindings.cpp). Single-threaded:
/// built, called and destroyed on the host's session thread.
calcu::sessions::ProductSession<TIAppPart> makeAppSession(TIAppSetup setup, calcu::pacing::WallClock clock,
                                                          std::optional<std::uint32_t> jitterSeed = std::nullopt,
                                                          std::vector<calcu::sessions::SessionPart*> before = {},
                                                          std::vector<calcu::sessions::SessionPart*> after = {},
                                                          bool start = true);

/// The steady clock as the session's wall clock, and back (the library's).
using calcu::pacing::steadyTimeOf;
using calcu::pacing::steadyWallTime;

} // namespace cu59
