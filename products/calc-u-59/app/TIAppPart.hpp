#pragma once
#include "ConstantMemoryKeeper.hpp"
#include "CueCard.hpp"
#include "TIDebugPart.hpp"
#include "TIFileKinds.hpp"
#include "media/FileAccess.hpp"
#include "media/MediaPart.hpp"
#include "settings/Paths.hpp"
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
// C59-3). Since M-55 the card reader is the library's media part (LIB-7 59,
// C59-7): the kind `magnetic-card` (app/TIMagneticCardKind), the TI-59's
// pass-through place `card-reader`, the save folder, the steps `card:` and
// `arm:`, the header key `magnetic-cards:`; the media part is the card
// wait's WaitProbe, and LIB-13's `open` fits a card file through it. What
// stays the part's: the PC-100C's TRACE position (C59-6 AC-6), the cue card
// shown (one fitted at a time, the module cue card of the library program on
// top, C59-7 AC-22), the corner taps (c59.tap-card). The debug panel is
// TIDebugPart's (C59-4, M-56), a part of the same session it owns; the file
// kinds of LIB-13's `open` are TIFileKinds (M-56). Errors are notices, diagnostics log lines. What
// 2.3.0's change flags announced arrives as the interim product topics below
// (removed with their owners, design §5).

namespace cu59 {

namespace topics {
using calcu::sessions::TopicName;
inline const TopicName kLed{"c59.led"};
inline const TopicName kCueCard{"c59.cue-card"};
inline const TopicName kPrinter{"c59.printer"};
} // namespace topics

// ── Payloads of the interim topics. Each is a snapshot set when today's
// change flag fires; two snapshots are equal when they are the same set
// (the serial), so a flag sends its topic once.

template <class D> struct TISnapshot : calcu::sessions::PayloadOf<D> {
    std::uint64_t serial = 0;
    bool same(const D& o) const { return serial == o.serial; }
};

struct TILedState final : TISnapshot<TILedState> {
    static constexpr calcu::sessions::PayloadKind kKind{"c59.led"};
    LedFrame led;
    double calcIndicatorOpacity = 0;
    calcu::sessions::Value toValue() const override;
};
/// The cue card to show (C59-2 AC-6, C59-7 AC-22): the module cue card of
/// the library program selected, else the fitted one. `onShow`: a fitted
/// magnetic card that no module cue card covers, whose corners feed it
/// (AC-12, AC-24); `litCorner`: the side passing now, while on show.
struct TICueCardState final : TISnapshot<TICueCardState> {
    static constexpr calcu::sessions::PayloadKind kKind{"c59.cue-card"};
    std::optional<CueCardContent> card;
    bool onShow = false;
    std::optional<int> litCorner;
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
/// The result of a command that answers yes or no (fit-card, tap-card, …).
struct TIFlag final : calcu::sessions::PayloadOf<TIFlag> {
    static constexpr calcu::sessions::PayloadKind kKind{"c59.flag"};
    explicit TIFlag(bool v) : value(v) {}
    bool value;
    bool same(const TIFlag& o) const { return value == o.value; }
    calcu::sessions::Value toValue() const override { return calcu::sessions::Value(value); }
};

/// The commands of the part (`c59.` + name; arguments in the comments).
namespace commands {
using calcu::sessions::CommandName;
// LIB-8 P-E2 (until M-57): `set-control {peripheral: "PC-100C", control: PRINT|ADV|TRACE, on}`,
// the library's command, as the paper panel sends it.
inline const CommandName kSetControl{"set-control"};
// C59-7 D-12: a tap on the fitted card; a top corner feeds that side
// (media.feed of the card reader). The card commands are the library's
// media.* (media.choose, media.start-new, media.eject, media.feed).
inline const CommandName kTapCard{"c59.tap-card"};               // {x, y} → TIFlag
} // namespace commands

/// What the host gives the part when the session is made (C59-5 D-12, A-12).
struct TIAppSetup {
    /// The host's lookUpRoms(). Without the full set no machine is built;
    /// LIB-18's StartupPart publishes LIB-12's topic `roms-missing` {text}
    /// and the shell shows it in place of the faceplate (LIB-12 AC-5, LIB-14
    /// AC-19; M-57).
    calcu::roms::LookupResult roms;
    /// The product's settings (LIB-9; the startup model, `last-model`, the
    /// startup presets [preset], [trace] max-file-size-mb) and the host's
    /// places on them: the save folder (cards, ti58c.memory.yaml), the trace
    /// folder (none on the Web: traces are handed over with KeepFile, LIB-10
    /// AC-26), the folder for screenshots, `~/` of a preset (README §3). Both
    /// outlive the part.
    calcu::settings::SettingsFile* settings = nullptr;
    const calcu::settings::Paths* paths = nullptr;
    /// The save folder's file access (cards and the constant memory file):
    /// null: plain file I/O; Swift: the coordinated iCloud access (C59-5 D-6, LIB-7 D-30).
    std::shared_ptr<FileIO> fileIO;
    /// Swift: whether the trace location can be used (iCloud, C59-4 AC-12);
    /// null: always.
    std::function<bool()> traceAvailable;
    /// The bundled examples (examples/ as bundle.yaml lists them; LIB-6
    /// AC-31, AC-33); empty: none (a bundle path is refused).
    std::string examplesFolder;
    calcu::sessions::Hosting hosting = calcu::sessions::Hosting::Paced;
    /// The harness (LIB-11 §11.3): the debug panel's tabs computed as if
    /// shown, so that `state calculator-debug` reads them (TIDebugSetup).
    bool debugTabsShown = false;
};

class TIAppPart final : public calcu::sessions::SessionPart {
public:
    explicit TIAppPart(TIAppSetup setup);
    ~TIAppPart();
    TIAppPart(const TIAppPart&) = delete;
    TIAppPart& operator=(const TIAppPart&) = delete;

    /// Every part of the session in the one order (parts::LibraryParts): the
    /// control part, the host's `host`, the library's with the debug panel
    /// before the open part (the file kinds use it), the program memory
    /// commands (C59-2 AC-9) and this part, then the host's `after`. The host
    /// builds its AppSessionSetup from them.
    std::vector<calcu::sessions::SessionPart*> sessionParts(std::vector<calcu::sessions::SessionPart*> host = {},
                                                            std::vector<calcu::sessions::SessionPart*> after = {});
    calcu::parts::LibraryParts& libraryParts() { return m_lib; }
    /// LIB-18's machines of the session.
    TIProductMachines& productMachines() { return m_machines; }
    /// The host's ROM lookup (LIB-12), as the part was made with it.
    const calcu::roms::LookupResult& romLookup() const { return m_setup.roms; }

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
    void beforeFlush(calcu::sessions::SessionContext&) override;

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

    // Display, keys, speed
    const LedFrame& display() const { return m_display; }
    double calcIndicatorOpacity() const { return m_calcOpacity; }
    calcu::typing::TypingPart& typing() { return m_lib.typing(); }
    /// The session's commanded speed is full (the library's `speed`, which
    /// the display's hold sends; the Web page shows it).
    bool fullSpeed() const { return m_fullSpeed; }
    /// An operation runs (a preset, a build, a plug change).
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
    calcu::paper::PaperPart& paperPart() { return m_lib.paper(); }
    calcu::debugger::DebuggerPart& debuggerPart() { return m_lib.debugger(); }
    calcu::presets::PresetsPart& presetsPart() { return m_lib.presets(); }
    TILevelRules* levelRules() { return &m_levelRules; }
    /// The debug panel (C59-4).
    TIDebugPart& debugPart() { return m_debugPart; }

    // Magnetic cards (C59-7; LIB-7 59): the library's media part, its place
    // `card-reader` on the TI-59 (reference/SharedArchitecture.md § "Card Reader Flow").
    calcu::media::MediaPart& mediaPart() { return m_lib.media(); }
    /// A tap at (nx, ny) on the card window: a top corner of a fitted card on
    /// show feeds that side (AC-12, D-12). False: nothing fed.
    bool tapCard(double nx, double ny);
    /// A magnetic card is fitted (the media part's binding of `card-reader`).
    bool cardFitted() const;
    /// The fitted magnetic card shows (no module cue card covers it, AC-24).
    bool cardOnShow() const { return m_onShow; }
    /// The side of the card on show passing through the reader now.
    std::optional<int> litCardCorner() const { return m_litCorner; }
    /// The library program selected now (C59-7 AC-21), 0: none.
    int activeProgram() const { return m_activeProgram; }

    // Debugger
    calcu::debugger::RunState runState() const;
    bool isFrozen() const;
    /// What changed since the last publish: each bit moves its interim topic.
    enum Change : uint32_t {
        ModelChanged = 1u << 0,     ///< model, module, machine replaced or reset (card reader emptied)
        DisplayChanged = 1u << 1,   ///< display digits or the C indicator
        CueCardChanged = 1u << 2,
        PrinterChanged = 1u << 3,   ///< connection, TRACE latch
        CardChanged = 1u << 5,      ///< the fitted card or the side passing
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
    /// The constant-memory file (TIFileAccess, on the save folder).
    TIFileAccess& files() { return m_files; }
    ConstantMemoryKeeper& constantMemory() { return m_keeper; }

private:
    static uint32_t changeIf(bool on, Change bit) { return on ? static_cast<uint32_t>(bit) : 0u; }
    void emitChanged(uint32_t c) { m_changes |= c; }
    calcu::parts::LibraryPartsSetup librarySetup();   // the constructor's, on the members before m_lib

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
    /// The 60 Hz duties after a host call: display and the cue card shown.
    void frame(std::chrono::steady_clock::time_point now);
    /// `cue-card:` of a preset (C59-2 AC-6): the machine's own card.
    void fitOwnCueCard(const CueCardContent& card);

    /// The cue card shown, the fitted card and the corner from the media
    /// part's `card-reader` and the library program (AC-21, AC-22, AC-24).
    void refreshCueCard();
    void followRunState();
    const calcu::debugger::RunControl& runControl() const;
    calcu::machine::Machine* machine() const;
    TIAppSetup m_setup;
    TIFileAccess m_files;
    std::shared_ptr<const calcu::media::FileAccess> m_mediaFiles;   ///< m_setup.fileIO as the media part's (D-30)
    calcu::sessions::SessionContext* m_ctx = nullptr;
    calcu::sessions::MachineSession* m_ms = nullptr;

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
    std::optional<CueCardContent> m_cueCard;
    bool m_onShow = false;
    std::optional<int> m_litCorner;
    /// D-6: which source fitted last (each takes the place of the one before).
    enum class Fitted { None, Card, Own } m_lastFit = Fitted::None;
    std::uint64_t m_seenArm = 0;   ///< the card reader's binding's arm serial last seen
    std::optional<CueCardContent> m_userCueCard;
    std::uint64_t m_userCueCardGeneration = 0;   ///< the machine it was fitted on (a preset's section)
    int m_activeProgram = 0;

    LedFrame m_display{};
    double m_calcOpacity = 0;

    bool m_printerConnected = false;
    bool m_printerTrace = false;   ///< C59-6 AC-6

    using Clock = std::chrono::steady_clock;
    calcu::debugger::RunState m_seenRun;

    calcu::sessions::StateTopic* m_led = nullptr;
    calcu::sessions::StateTopic* m_cue = nullptr;
    calcu::sessions::StateTopic* m_printer = nullptr;
};

/// The session of a native frontend (C59-5 D-12; LIB-6 D-40): the library's
/// app session with TIAppPart::sessionParts() (every part in the one order),
/// the part owned by the session (AppSessionSetup::owned), which lets it go
/// first, paced on `clock` (the host's wall clock; the steady clock's time
/// since its epoch in the apps), and LIB-18's `start` sent once (the startup
/// model with its startup preset, or its built-in default). `before`: the
/// host's own parts, after the control part (LIB-14's SettingsPart, tests);
/// `after`: parts behind the product's (tests of TIWebPart; the Web engine
/// builds its own, frontends/web/bindings.cpp). Single-threaded: built,
/// called and destroyed on the host's session thread.
calcu::sessions::ProductSession<TIAppPart> makeAppSession(TIAppSetup setup, calcu::pacing::WallClock clock,
                                                          std::optional<std::uint32_t> jitterSeed = std::nullopt,
                                                          std::vector<calcu::sessions::SessionPart*> before = {},
                                                          std::vector<calcu::sessions::SessionPart*> after = {},
                                                          bool start = true);

/// The steady clock as the session's wall clock, and back (the library's).
using calcu::pacing::steadyTimeOf;
using calcu::pacing::steadyWallTime;

} // namespace cu59
