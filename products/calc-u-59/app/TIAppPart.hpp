#pragma once
#include "ConstantMemoryKeeper.hpp"
#include "CueCard.hpp"
#include "TIControlRules.hpp"
#include "TIDebugPart.hpp"
#include "TIFileKinds.hpp"
#include "media/FileAccess.hpp"
#include "media/MediaPart.hpp"
#include "settings/Paths.hpp"
#include "FrameSampler.hpp"
#include "MachineModelInfo.hpp"
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
#include "shell/HostSession.hpp"
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

// Calc-U-59's app layer in the library's app session. The frontends
// (Qt6 on its GUI thread, Swift on calcu::shell::SessionThread) host a
// calcu::sessions::AppSession made by makeAppSession(); everything they do
// is a command or a request, everything they get is a message.
//
// The part owns the library's parts of its session (parts::LibraryParts,
// as Calc-U-1600's SharpAppPart): the library's typing, peripherals, presets
// on the TI rules and keywords (app/TIPresetRules, app/TIPresetKeys; the
// bundle of examples/), the start, model switch, Reset, Cold boot, attach and
// detach on app/TIProductMachines, the paper, the debugger; then the program
// memory commands (app/TIPresetCommands) and this part. The
// machine is built by the library's machine control and held by the machine
// session; the part follows each new machine (machineChanged): the runtime
// around it (the frame sampler),
// the module and the printer read from what is plugged, the
// own cue card, the TI-58C's constant memory file (ConstantMemoryKeeper).
// The card reader is the library's media part: the kind `magnetic-card` (app/TIMagneticCardKind), the TI-59's
// pass-through place `card-reader`, the save folder, the steps `card:` and
// `arm:`, the header key `magnetic-cards:`; the library's `open` fits a
// card file through the media part. What
// stays the part's: the PC-100C's TRACE position (the switch is
// set through the library's `set-control`, the position kept while the PC-100C is
// detached and put into the next one by TIProductMachines::attaching), the
// cue card shown (one fitted at a time; it and the module cue card of the
// library program: the one put there last on top), the corner taps (c59.tap-card). The debug
// panel is TIDebugPart's, a part of the same session it owns;
// the file kinds of the library's `open` are TIFileKinds. Diagnostics are
// log lines. The product topics below are set after each flow with what it
// left; the session sends one when its value changed. The
// PC-100C in `printer` and the positions of its controls are the library's
// control state (`places`, `controls`).

namespace cu59 {

namespace topics {
using calcu::sessions::TopicName;
inline const TopicName kCalcIndicator{"c59.calc-indicator"};
inline const TopicName kCueCard{"c59.cue-card"};
} // namespace topics

// ── Payloads of the product topics, equal when their content is.

/// The display's LED frame and the C indicator (the frontends draw it). The
/// frame is the frame sampler's persistence model (held glyph, smoothed
/// levels), not the raw machine frame of the library's `display` topic, so it
/// stays in this payload.
struct TICalcIndicatorState final : calcu::sessions::PayloadOf<TICalcIndicatorState> {
    static constexpr calcu::sessions::PayloadKind kKind{"c59.calc-indicator"};
    LedFrame led;
    double calcIndicatorOpacity = 0;
    bool same(const TICalcIndicatorState& o) const { return led == o.led && calcIndicatorOpacity == o.calcIndicatorOpacity; }
    calcu::sessions::Value toValue() const override;
};
/// The cue card to show: of the module cue card of the library program
/// selected and the fitted one, the one put there last. `onShow`: a fitted
/// magnetic card that no module cue card covers, whose corners feed it; `litCorner`: the side
/// passing now, while on show.
struct TICueCardState final : calcu::sessions::PayloadOf<TICueCardState> {
    static constexpr calcu::sessions::PayloadKind kKind{"c59.cue-card"};
    std::optional<CueCardContent> card;
    bool onShow = false;
    std::optional<int> litCorner;
    bool same(const TICueCardState& o) const { return card == o.card && onShow == o.onShow && litCorner == o.litCorner; }
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
/// The PC-100C's PRINT, ADV and TRACE are the library's `set-control
/// {peripheral: "PC-100C", control, on}` (calcu::peripherals::kSetControlCommand).
namespace commands {
using calcu::sessions::CommandName;
// A tap on the fitted card; a top corner feeds that side
// (media.feed of the card reader). The card commands are the library's
// media.* (media.choose, media.start-new, media.eject, media.feed).
inline const CommandName kTapCard{"c59.tap-card"};               // {x, y} → TIFlag
} // namespace commands

/// What the host gives the part when the session is made.
struct TIAppSetup {
    /// The host's lookUpRoms(). Without the full set no machine is built;
    /// the library's StartupPart publishes the topic `roms-missing` {text}
    /// and the shell shows it in place of the faceplate.
    calcu::roms::LookupResult roms;
    /// The product's settings (the startup model, `last-model`, the
    /// startup presets [preset], [trace] max-file-size-mb) and the host's
    /// places on them: the save folder (cards, ti58c.memory.yaml), the trace
    /// folder (none on the Web: traces are handed over with KeepFile), the
    /// folder for screenshots, `~/` of a preset (README §3); each read when it
    /// is used, so a change in Settings applies at once (the trace folder and
    /// limit at each trace start).
    /// Both outlive the part.
    calcu::settings::SettingsFile* settings = nullptr;
    const calcu::settings::Paths* paths = nullptr;
    /// The save folder's file access (cards and the constant memory file):
    /// null: the plain one; Swift: the coordinated iCloud access.
    std::shared_ptr<const calcu::media::FileAccess> files;
    /// The bundled examples (examples/ as bundle.yaml lists them); empty:
    /// none (a bundle path is refused).
    std::string examplesFolder;
    /// The Web: where several files of the preset picker's upload become one
    /// folder; unset: none.
    std::optional<std::string> uploadFolder;
    calcu::sessions::Hosting hosting = calcu::sessions::Hosting::Paced;
    /// The harness: the debug panel's tabs computed as if
    /// shown, so that `state c59.calculator-debug` reads them (TIDebugSetup).
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
    /// commands and this part, then the host's `after`. The host
    /// builds its AppSessionSetup from them.
    std::vector<calcu::sessions::SessionPart*> sessionParts(std::vector<calcu::sessions::SessionPart*> host = {},
                                                            std::vector<calcu::sessions::SessionPart*> after = {});
    calcu::parts::LibraryParts& libraryParts() { return m_lib; }
    /// The library's machines of the session.
    TIProductMachines& productMachines() { return m_machines; }
    /// The host's ROM lookup, as the part was made with it.
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
    // adapters and the tests; a frontend sends the
    // commands. A test that calls one directly runs the session after it.

    // Machine (built and held by the library's machine control and the machine
    // session; the start, the model switch, Reset, Cold boot, the module and
    // the printer are the library's commands: `start`, `model`, `reset`,
    // `cold-boot`, `attach`, `detach`)
    bool hasMachine() const { return m_sampler.has_value(); }
    const ModelId& model() const { return m_model; }
    /// The machine session's machine (null: none yet).
    calcu::machine::Machine* machine() const;
    /// The module in `slot-1` (empty: none), as the machine says.
    const std::string& moduleID() const { return m_moduleID; }

    // Display, keys, speed
    const LedFrame& display() const { return m_display; }
    double calcIndicatorOpacity() const { return m_calcOpacity; }
    calcu::typing::TypingPart& typing() { return m_lib.typing(); }

    // Cue card
    const std::optional<CueCardContent>& cueCard() const { return m_cueCard; }
    /// The machine's own card (a preset's `cue-card:`, the constant memory
    /// file's), under a module program's card.
    const std::optional<CueCardContent>& ownCueCard() const { return m_userCueCard; }

    // Printer (the paper: the library's PaperPart)
    bool printerConnected() const { return m_printerConnected; }
    /// TRACE's position: kept by the app session from the start (off) to the
    /// end, through reset, power cycle, cold boot, detach and attach; never
    /// saved; a preset's build sets it as the preset
    /// says. It follows the plugged PC-100C's switch (the library's
    /// `set-control`) and goes into the next one attached (TIProductMachines).
    bool printerTrace() const { return m_printerTrace; }
    calcu::paper::PaperPart& paperPart() { return m_lib.paper(); }
    calcu::debugger::DebuggerPart& debuggerPart() { return m_lib.debugger(); }
    calcu::presets::PresetsPart& presetsPart() { return m_lib.presets(); }
    TILevelRules* levelRules() { return &m_levelRules; }
    /// The debug panel.
    TIDebugPart& debugPart() { return m_debugPart; }

    // Magnetic cards: the library's media part, its place
    // `card-reader` on the TI-59 (docs/developer/App-Layer.md § "Card Reader Flow").
    calcu::media::MediaPart& mediaPart() { return m_lib.media(); }
    /// A tap at (nx, ny) on the card window: a top corner of a fitted card on
    /// show feeds that side. False: nothing fed.
    bool tapCard(double nx, double ny);
    /// A magnetic card is fitted (the media part's binding of `card-reader`).
    bool cardFitted() const;
    /// The fitted magnetic card shows (no module cue card covers it).
    bool cardOnShow() const { return m_onShow; }
    /// The side of the card on show passing through the reader now.
    std::optional<int> litCardCorner() const { return m_litCorner; }
    /// The library program selected now, 0: none.
    int activeProgram() const { return m_activeProgram; }

    // Debugger
    calcu::debugger::RunState runState() const;
    bool isFrozen() const;

    // What the flows report
    /// Developer diagnostics: a line of the log.
    void diagnostic(const std::string& message);
    /// The constant-memory file (TIFileAccess, on the save folder).
    TIFileAccess& files() { return m_files; }
    ConstantMemoryKeeper& constantMemory() { return m_keeper; }

private:
    calcu::parts::LibraryPartsSetup librarySetup();   // the constructor's, on the members before m_lib

    using Handler = std::function<calcu::sessions::CommandOutcome(const calcu::sessions::Value&)>;
    void add(calcu::sessions::SessionSetup&, const calcu::sessions::CommandName&, Handler);
    /// After everything a flow did: the product topics set from what it left.
    void settle();
    /// A new machine of the session: the runtime around it, the
    /// plugs read from it, the own cue card, the constant memory keeper.
    void followMachine();
    /// What is plugged now: the module (and its cards), the printer, TRACE.
    void followPlugs(bool newMachine);
    /// The 60 Hz duties after a host call: display and the cue card shown.
    void frame(std::chrono::steady_clock::time_point now);
    /// `cue-card:` of a preset: the machine's own card.
    void fitOwnCueCard(const CueCardContent& card);

    /// The cue card shown, the fitted card and the corner from the media
    /// part's `card-reader` and the library program.
    void refreshCueCard();
    const calcu::debugger::RunControl& runControl() const;
    TIAppSetup m_setup;
    TIFileAccess m_files;   ///< its file access is the media part's too
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
    TIControlRules m_controlRules;
    TIFileKinds m_fileKinds;
    calcu::parts::LibraryParts m_lib;

    /// The 60 Hz frame sampler of the machine followed; a new one per machine.
    std::optional<FrameSampler> m_sampler;
    std::uint64_t m_seenGeneration = 0;
    ModelId m_model{kTI59};

    std::string m_moduleID;
    std::map<int, CueCardContent> m_moduleCards;
    std::optional<CueCardContent> m_cueCard;
    bool m_onShow = false;
    std::optional<int> m_litCorner;
    /// Which source fitted last (each takes the place of the one before).
    enum class Fitted { None, Card, Own } m_lastFit = Fitted::None;
    std::uint64_t m_seenArm = 0;   ///< the card reader's binding's arm serial last seen
    std::optional<CueCardContent> m_userCueCard;
    std::uint64_t m_userCueCardGeneration = 0;   ///< the machine it was fitted on (a preset's section)
    int m_activeProgram = 0;
    /// The module cue card lies on top of the fitted card: a program was
    /// selected after the card was fitted (the one put there last shows).
    bool m_moduleOnTop = true;

    LedFrame m_display{};
    double m_calcOpacity = 0;

    bool m_printerConnected = false;
    bool m_printerTrace = false;   ///< TRACE's position (printerTrace())

    using Clock = std::chrono::steady_clock;

    calcu::sessions::StateTopic* m_calcIndicator = nullptr;
    calcu::sessions::StateTopic* m_cue = nullptr;
};

/// The session of a native frontend: the library's
/// app session with TIAppPart::sessionParts() (every part in the one order),
/// the part owned by the session (AppSessionSetup::owned), which lets it go
/// first, paced on `clock` (the host's wall clock; the steady clock's time
/// since its epoch in the apps), and the library's `start` sent once (the startup
/// model with its startup preset, or its built-in default). `before`: the
/// host's own parts, after the control part (the library's SettingsPart, tests);
/// `after`: parts behind the product's (tests of TIWebPart; the Web engine
/// builds its own, frontends/web/bindings.cpp). Single-threaded: built,
/// called and destroyed on the host's session thread.
calcu::sessions::ProductSession<TIAppPart> makeAppSession(TIAppSetup setup, calcu::pacing::WallClock clock,
                                                          std::optional<std::uint32_t> jitterSeed = std::nullopt,
                                                          std::vector<calcu::sessions::SessionPart*> before = {},
                                                          std::vector<calcu::sessions::SessionPart*> after = {},
                                                          bool start = true);
/// The same on the library's host builder (calcu::shell::makeHostSession):
/// a frontend host's settings for the library's SettingsPart (Qt6, Swift), its
/// parts, clock and `start`; `host.owned` is the part's.
calcu::sessions::ProductSession<TIAppPart> makeAppSession(TIAppSetup setup, calcu::shell::HostSessionSetup host);

/// The steady clock as the session's wall clock, and back (the library's).
using calcu::pacing::steadyTimeOf;
using calcu::pacing::steadyWallTime;

} // namespace cu59
