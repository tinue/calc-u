#pragma once
#include "debugger/Breakpoints.hpp"
#include "debugger/Levels.hpp"
#include "debugger/Profile.hpp"
#include "debugger/RunState.hpp"
#include "debugger/TraceSession.hpp"
#include "machine/DebugTarget.hpp"
#include "sessions/MachineSession.hpp"
#include "sessions/SessionPart.hpp"
#include "sessions/Topics.hpp"
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

// The run control (LIB-10 design §3.4, rules G1 to G14; spec AC-5 to AC-16,
// AC-45 to AC-47, EC-1, EC-2, EC-4 to EC-7): one per app session, owned by
// the DebuggerPart, whether or not anything uses it; the machine session's
// AdvanceGate. Its state is Running, PausePending, Paused or Stepping;
// MachineSession::paused() is true exactly while it is Paused. It runs on
// the session's thread and holds no lock (I-4).
//
// The calls of G2 to G9 are the same whoever makes them: the session
// commands (DebuggerPart), the preset steps (DebuggerSteps), the harness
// (through the commands) and the buttons. A call that waits for the pause
// to land gives its CallId; the reply goes out then, with the run state.

namespace calcu::debugger {

enum class StepKind { In, Over, Out };

// D-16 (LIB-10 design §10.15, M-45): a step's test, asked after each counted
// step (in or over) of its target; true: go on, up to the count. The DAP's
// source-line steps (Calc-U-1600 app/dap/) are one such step: on until the
// source line changes, in the gate's pieces, ending at a breakpoint.
using StepTest = std::function<bool(const machine::DebugTarget&)>;

class RunControl final : public sessions::AdvanceGate, public machine::InstructionObserver {
public:
    RunControl(LevelRules&, TraceSession&);
    void bind(sessions::SessionContext& c, sessions::StateTopic* runState) {
        ctx_ = &c;
        topic_ = runState;
        publish();
    }

    // G1. The gate of the machine session.
    machine::Ticks advance(machine::Machine&, machine::Ticks atLeast, bool& stoppedEarly) override;
    bool stopAfter(const machine::InstructionEvent&) override;

    struct Result {
        bool ok = true;
        std::string reason;     // when refused
        bool deferred = false;  // the reply comes when the pause lands
    };
    Result pause(const LevelName&, sessions::CallId call = 0);   // G2
    Result resume();                                              // G3
    Result step(const LevelName&, std::uint64_t count, std::optional<machine::DebugTargetId> target, StepKind,
                sessions::CallId call = 0, StepTest until = {});  // G4, G5; D-16
    Result pauseOnStart(std::optional<LevelName>);                // G8; nullopt: off
    void pauseAtStart();                                          // G9
    // An event outside an advance on which an armed start fires (Calc-U-59's
    // ASM overlay entered at 1800, C59-4 AC-17): paused with the reason
    // pause-on-start when `level` is armed and the machine runs; else false.
    bool fireStart(const LevelName& level);
    void restarted(RestartKind);                                  // G10 (and G8, G9 at a restart)
    void sync();                                                  // G11 when the machine changed

    RunState state() const;
    bool paused() const { return mode_ == Mode::Paused; }
    bool running() const { return mode_ == Mode::Running; }
    BreakpointTable& table() { return table_; }
    Profile& profile() { return profile_; }
    LevelRules& rules() { return rules_; }
    // The refusal when the machine offers no target to control, else none.
    std::optional<std::string> refusal() const;

private:
    enum class Mode { Running, PausePending, Paused, Stepping };
    enum class Landed { None, StartFired, PauseLanded, StepDone };

    machine::Checks checks();
    void onStop(const machine::DebugStop&, machine::Debugging&);
    void land();
    void enterPaused(LevelName, RunState::Reason, std::vector<RunState::Hit> hits = {});
    void finishCalls();
    void publish();
    bool stepDoneAfter(const machine::InstructionEvent&);
    bool countedStepDone(const machine::DebugTarget&);
    machine::Debugging* debugging() const;

    LevelRules& rules_;
    TraceSession& trace_;
    sessions::SessionContext* ctx_ = nullptr;
    sessions::StateTopic* topic_ = nullptr;
    BreakpointTable table_;
    Profile profile_;
    std::uint64_t seenGeneration_ = 0;

    Mode mode_ = Mode::Running;
    LevelName level_ = kCpu;   // pending, paused or stepping
    RunState::Reason reason_ = RunState::Reason::Pause;
    std::vector<RunState::Hit> hits_;
    std::optional<LevelName> armed_;
    bool pauseAtStart_ = false;
    std::vector<sessions::CallId> calls_;   // replies that wait for the pause
    Landed landed_ = Landed::None;
    LevelName firedLevel_ = kCpu;

    // Stepping.
    StepKind kind_ = StepKind::In;
    StepTest until_;
    std::size_t stepTarget_ = 0;
    std::uint64_t left_ = 0;
    int repeats_ = 0;
    bool overCall_ = false;
    std::uint64_t depth0_ = 0;
};

} // namespace calcu::debugger
