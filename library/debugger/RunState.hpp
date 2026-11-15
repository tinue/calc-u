#pragma once
#include "debugger/Breakpoints.hpp"
#include "debugger/Levels.hpp"
#include "sessions/Messages.hpp"
#include "sessions/Payload.hpp"
#include <optional>
#include <string>
#include <vector>

// The run state, the trace state and the session's names of the debugger
// (LIB-10 design §3.6; spec AC-17, AC-25). Library names, no prefix.

namespace calcu::debugger {

inline const sessions::TopicName kRunStateTopic{"run-state"};   // state: RunState
inline const sessions::TopicName kTraceTopic{"trace"};          // state: TraceState

inline const sessions::CommandName kPauseCommand{"pause"};                  // {level}
inline const sessions::CommandName kResumeCommand{"resume"};
inline const sessions::CommandName kStepCommand{"step"};                    // {level, count, target?, kind: in|over|out}
inline const sessions::CommandName kPauseOnStartCommand{"pause-on-start"};  // {level} | {off: true}
inline const sessions::CommandName kPauseAtStartCommand{"pause-at-start"};
inline const sessions::CommandName kBreakCommand{"break"};                  // {setter, target, address, qualifier?}
inline const sessions::CommandName kBreakOffCommand{"break-off"};           // {setter, id?}
inline const sessions::CommandName kWatchCommand{"watch"};                  // {setter, target, space, address, count, access}
inline const sessions::CommandName kWatchOffCommand{"watch-off"};           // {setter, id?}
inline const sessions::CommandName kSetRegisterCommand{"set-register"};     // {target, register, value}
inline const sessions::CommandName kPokeCommand{"poke"};                    // {target, space, address, values, bank?, vbank?}
inline const sessions::CommandName kTraceCommand{"trace"};                  // {file} | {button: true} | {off: true}
inline const sessions::CommandName kProfileCommand{"profile"};              // {on}
inline const sessions::CommandName kSymbolsCommand{"symbols"};              // {file, target?, bytes?}

inline const sessions::RequestName kTargetsRequest{"targets"};
inline const sessions::RequestName kRegistersRequest{"registers"};          // {target?}
inline const sessions::RequestName kPeekRequest{"peek"};                    // {target, space, address, count, bank?, vbank?}
inline const sessions::RequestName kHistoryRequest{"history"};              // {target?, count?}
inline const sessions::RequestName kDisassembleRequest{"disassemble"};      // {target, address, count}
inline const sessions::RequestName kBreakpointsRequest{"breakpoints"};      // {setter?}
inline const sessions::RequestName kWatchesRequest{"watches"};              // {setter?}
inline const sessions::RequestName kProfileReportRequest{"profile-report"}; // {count?}

inline constexpr const char* kNoTarget = "The machine offers no debug target.";
inline constexpr const char* kNotPaused = "The machine is not paused.";

struct RunState final : sessions::PayloadOf<RunState> {
    static constexpr sessions::PayloadKind kKind{"run-state"};
    enum class Reason { Pause, Step, Breakpoint, Watch, PauseOnStart, Entry };
    struct Hit {
        BreakpointId id = 0;
        Setter setter;
        bool operator==(const Hit&) const = default;
    };
    bool paused = false;
    std::optional<LevelName> owner;             // when paused
    std::optional<Reason> reason;
    std::optional<machine::DebugTargetId> target;   // the current target (AC-14)
    std::vector<Hit> hits;                      // a breakpoint or watch stop
    std::optional<LevelName> armed;             // pause-on-start
    bool pauseAtStart = false;
    bool same(const RunState& o) const {
        return paused == o.paused && owner == o.owner && reason == o.reason && target == o.target && hits == o.hits &&
               armed == o.armed && pauseAtStart == o.pauseAtStart;
    }
    sessions::Value toValue() const override;
};

const char* reasonName(RunState::Reason);   // "pause", "step", "breakpoint", "watch", "pause-on-start", "entry"

struct TraceState final : sessions::PayloadOf<TraceState> {
    static constexpr sessions::PayloadKind kKind{"trace"};
    bool running = false;
    std::string fileName;
    bool same(const TraceState& o) const { return running == o.running && fileName == o.fileName; }
    sessions::Value toValue() const override;
};

} // namespace calcu::debugger
