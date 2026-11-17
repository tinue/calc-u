#pragma once
#include "DebugModel.hpp"
#include "DebugSnapshots.hpp"
#include "control/ControlRules.hpp"
#include "debugger/DebuggerPart.hpp"
#include "roms/RomSet.hpp"
#include "sessions/Messages.hpp"
#include "sessions/Payload.hpp"
#include "sessions/SessionPart.hpp"
#include "sessions/Topics.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// Calc-U-59's debug panel below the frontends (C59-4 design §3.1 to §3.10;
// spec AC-1 to AC-26): a session part of the TI app session and of the
// harness, after LIB-10's DebuggerPart. It holds what the panel shows and
// sends it as six product topics (state topics; the harness reads them with
// `state`); the frontends draw them and send the commands below:
//
//   calculator-debug   the CALCULATOR tab (AC-3), while a frontend shows it
//   program-listing    the whole listing of the program that runs and its
//                      current step (AC-4), while the machine is paused
//   cpu-debug          the CPU tab's instructions (AC-6), while shown or paused
//   heatmap            the ROM heatmap's counts (AC-8), about 10 Hz
//   asm-overlay        the ASM overlay's box (AC-16)
//   debug-view         the tab a `debug-panel:` step asked for last and the
//                      level of the machine's log (AC-2, AC-11; D-6)
//
// Each payload's value (toValue) is the form the Web page draws (D-7: the
// formatted rows, the heatmap's steps as bytes); the native frontends read
// the payload's fields. The log is LIB-3's `log` topic (AC-9): the machine's
// own lines (the product kind c59.machine-log) go there at their level, the
// dumps at every level. The run state and the trace are LIB-10's.
//
// It is also LIB-4 control rules (AC-25): the product entries c59.asm-run,
// c59.trace and c59.calculator-tab, each {available, reason}.
// Single-threaded, on the session's thread.

namespace cu59 {

/// A tab of the debug panel, as the step `debug-panel:` names it.
enum class DebugPanel { Calculator, Cpu, Log };
/// The level of the machine's own log lines (AC-11).
enum class DebugLevel { Off = 0, Info = 1, Debug = 2 };
std::string_view debugPanelName(DebugPanel);
std::optional<DebugPanel> debugPanelNamed(std::string_view);
std::string_view debugLevelName(DebugLevel);
std::optional<DebugLevel> debugLevelNamed(std::string_view);

namespace topics {
using calcu::sessions::TopicName;
inline const TopicName kCalculatorDebug{"calculator-debug"};
inline const TopicName kProgramListing{"program-listing"};
inline const TopicName kCpuDebug{"cpu-debug"};
inline const TopicName kHeatmap{"heatmap"};
inline const TopicName kAsmOverlay{"asm-overlay"};
inline const TopicName kDebugView{"debug-view"};
} // namespace topics

namespace commands {
using calcu::sessions::CommandName;
inline const CommandName kLiveDebug{"c59.live-debug"};         // {on}: the CALCULATOR tab is shown
inline const CommandName kCpuDebug{"c59.cpu-debug"};           // {on}: the CPU tab is shown (the heatmap counts, from zero)
inline const CommandName kHeatmapReset{"c59.heatmap-reset"};   // AC-8
inline const CommandName kLogLevel{"c59.log-level"};           // {level: off|info|debug} (AC-11)
inline const CommandName kDump{"c59.dump"};                    // {what: scom|memory|prog|vars} → the lines (AC-10)
inline const CommandName kAsmLoad{"c59.asm-load"};             // {name, text (absent: unreadable)} → the count (AC-16)
inline const CommandName kAsmRun{"c59.asm-run"};               // → the status line (AC-17)
inline const CommandName kAsmClear{"c59.asm-clear"};
} // namespace commands

/// The product entries of the control state (AC-25): {available, reason?}.
namespace entries {
inline constexpr const char* kAsmRun = "c59.asm-run";
inline constexpr const char* kTrace = "c59.trace";
inline constexpr const char* kCalculatorTab = "c59.calculator-tab";
} // namespace entries

inline constexpr const char* kNoAsmOverlay = "No ASM overlay loaded.";
inline constexpr const char* kAsmOverlayRuns = "An ASM overlay runs.";

/// FREEZE and FREEZE ON START of a tab (AC-5, AC-7) for the level of the tab
/// (`program`: CALCULATOR, `cpu`: CPU): FREEZE and FREEZE ON START while the
/// machine runs and the tab's start is not armed; while it is armed the tab
/// shows ARMED, which disarms it. STEP and RESUME are LIB-10's buttonUse.
struct FreezeButtons {
    bool freeze = false, freezeOnStart = false, armed = false;
    bool operator==(const FreezeButtons&) const = default;
};
FreezeButtons freezeButtons(const calcu::debugger::LevelName& level, const calcu::debugger::RunState* state);

// ── Payloads ─────────────────────────────────────────────────────────────────

struct TICalculatorDebug final : calcu::sessions::PayloadOf<TICalculatorDebug> {
    static constexpr calcu::sessions::PayloadKind kKind{"calculator-debug"};
    LiveDebugSnapshot live;
    bool same(const TICalculatorDebug& o) const { return live == o.live; }
    calcu::sessions::Value toValue() const override;
};
struct TIProgramListing final : calcu::sessions::PayloadOf<TIProgramListing> {
    static constexpr calcu::sessions::PayloadKind kKind{"program-listing"};
    std::vector<StepEntry> steps;   ///< empty while the machine runs
    ProgramSource source = ProgramSource::UserProgram;
    int currentIndex = -1;
    bool same(const TIProgramListing& o) const { return steps == o.steps && source == o.source; }
    calcu::sessions::Value toValue() const override;
};
struct TICpuDebug final : calcu::sessions::PayloadOf<TICpuDebug> {
    static constexpr calcu::sessions::PayloadKind kKind{"cpu-debug"};
    std::vector<InspectorEntry> entries;   ///< the newest 32 running; up to 1024 and the next paused
    bool paused = false;
    std::uint64_t serial = 0;
    bool same(const TICpuDebug& o) const { return serial == o.serial; }
    calcu::sessions::Value toValue() const override;
};
struct TIHeatmap final : calcu::sessions::PayloadOf<TIHeatmap> {
    static constexpr calcu::sessions::PayloadKind kKind{"heatmap"};
    std::vector<std::uint32_t> counts;   ///< 0000 to 13FF
    std::uint64_t serial = 0;
    bool same(const TIHeatmap& o) const { return serial == o.serial; }
    calcu::sessions::Value toValue() const override;   // the steps (heatStep), as bytes
};
struct TIAsmOverlayState final : calcu::sessions::PayloadOf<TIAsmOverlayState> {
    static constexpr calcu::sessions::PayloadKind kKind{"asm-overlay"};
    std::string fileName, status;
    int instructions = 0;
    bool running = false;
    bool same(const TIAsmOverlayState& o) const {
        return fileName == o.fileName && status == o.status && instructions == o.instructions && running == o.running;
    }
    calcu::sessions::Value toValue() const override;
};
struct TIDebugView final : calcu::sessions::PayloadOf<TIDebugView> {
    static constexpr calcu::sessions::PayloadKind kKind{"debug-view"};
    DebugPanel view = DebugPanel::Calculator;
    std::uint64_t requests = 0;   ///< grows with each tab asked for: a frontend shows `view` when it grows
    DebugLevel logLevel = DebugLevel::Off;
    bool same(const TIDebugView& o) const { return view == o.view && requests == o.requests && logLevel == o.logLevel; }
    calcu::sessions::Value toValue() const override;
};

struct TIDebugSetup {
    std::shared_ptr<const calcu::roms::RomSet> roms;   ///< the module's program table (AC-4)
    /// The harness: the CALCULATOR and CPU tabs computed as if shown (its
    /// `state` reads them; design §3.1).
    bool alwaysShown = false;
};

class TIDebugPart final : public calcu::sessions::SessionPart, public calcu::control::ControlRules {
public:
    explicit TIDebugPart(TIDebugSetup setup);
    TIDebugPart(const TIDebugPart&) = delete;
    TIDebugPart& operator=(const TIDebugPart&) = delete;
    /// Before the session is built: LIB-10's part (the run control).
    void bindDebugger(calcu::debugger::DebuggerPart* d) { m_debugger = d; }

    // SessionPart.
    void attach(calcu::sessions::SessionSetup&) override;
    void beforeCall(calcu::sessions::SessionContext&, calcu::pacing::WallTime) override;
    void afterCall(calcu::sessions::SessionContext&, calcu::pacing::WallTime) override;
    void machineChanged(calcu::sessions::SessionContext&) override;
    void machineRestarted(calcu::sessions::SessionContext&, calcu::sessions::Restart) override;
    void operationEnded(calcu::sessions::SessionContext&) override;

    // ControlRules (AC-25).
    void productEntries(const calcu::machine::Machine&, std::vector<calcu::control::ProductEntry>&) const override;

    // ── The flows, as the commands run them (public for the tests).
    /// The step `debug-panel:` (LIB-10 AC-32; AC-2): `calculator` is passed
    /// over while an overlay runs.
    void showView(calcu::sessions::SessionContext&, const std::string& view);
    void setLiveShown(bool on);
    void setCpuShown(bool on);
    void resetHeatmap();
    void setLogLevel(DebugLevel);
    /// AC-10: the dump's lines, also written into the log; empty: no machine
    /// or an unknown `what`.
    std::vector<std::string> dump(std::string_view what);
    /// AC-16, AC-19: read and put over the ROM at once; the sentence of a refusal.
    /// `notice`: the refusal is also a notice (Select File; `open` sends its own).
    std::optional<std::string> loadAsm(const std::string& name, const std::optional<std::string>& text, bool notice = true);
    /// The context of a call that reaches the part from another one (a file kind).
    void attachContext(calcu::sessions::SessionContext& c) { m_ctx = &c; }
    /// AC-17: the refusal, or none (entered; the status says so).
    std::optional<std::string> runAsm();
    void clearAsm();

    // What the topics carry now.
    const DebugModel& model() const { return m_debug; }
    DebugLevel logLevel() const { return m_logLevel; }
    const TIAsmOverlayState& asmState() const { return m_asm; }
    bool overlayRunning() const;
    bool liveShown() const { return m_liveShown; }
    bool cpuShown() const { return m_cpuShown; }

private:
    calcu::machine::Machine* machine() const;
    calcu::debugger::RunControl* runControl() const;
    bool paused() const;
    void follow();            // the machine, the plugs, the run state, the overlay
    void publish();
    void publishAsm();
    void refreshModule();
    void setStatus(std::string s);

    TIDebugSetup m_setup;
    calcu::debugger::DebuggerPart* m_debugger = nullptr;
    calcu::sessions::SessionContext* m_ctx = nullptr;
    calcu::sessions::StateTopic* m_liveTopic = nullptr;
    calcu::sessions::StateTopic* m_listingTopic = nullptr;
    calcu::sessions::StateTopic* m_cpuTopic = nullptr;
    calcu::sessions::StateTopic* m_heatTopic = nullptr;
    calcu::sessions::StateTopic* m_asmTopic = nullptr;
    calcu::sessions::StateTopic* m_viewTopic = nullptr;

    DebugModel m_debug;
    std::uint64_t m_generation = 0;
    std::string m_moduleID;
    bool m_liveShown = false, m_cpuShown = false;
    bool m_wasPaused = false;
    std::optional<calcu::machine::Ticks> m_pausedAt;
    bool m_listingDirty = true, m_liveDirty = true, m_cpuDirty = true, m_heatDirty = true;
    std::uint64_t m_serial = 0;
    calcu::pacing::WallTime m_nextHeat{0};
    DebugLevel m_logLevel = DebugLevel::Off;
    TIDebugView m_view;
    TIAsmOverlayState m_asm;
};

} // namespace cu59
