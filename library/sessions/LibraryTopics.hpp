#pragma once
#include "machine/Capability.hpp"
#include "machine/Display.hpp"
#include "machine/EmulatedTime.hpp"
#include "machine/Machine.hpp"
#include "pacing/PacingProfile.hpp"
#include "sessions/LogLevel.hpp"
#include "sessions/Messages.hpp"
#include "sessions/Payload.hpp"
#include <string>
#include <vector>

// The session's own topics, commands and requests and their payloads.

namespace calcu::sessions {

inline const TopicName kDisplayTopic{"display"};   // state: DisplayState
inline const TopicName kBusyTopic{"busy"};         // state: BusyState
inline const TopicName kNoticesTopic{"notices"};   // stream, History::None: Notice
inline const TopicName kLogTopic{"log"};           // stream, History::SinceReset: LogLine
inline const TopicName kLogLevelTopic{"log-level"};   // state: LogLevelState

inline const CommandName kSuspendCommand{"suspend"};
inline const CommandName kContinueCommand{"continue"};
inline const CommandName kSpeedCommand{"speed"};     // {full: bool}
inline const CommandName kKeyDownCommand{"key-down"};   // {key: string}
inline const CommandName kKeyUpCommand{"key-up"};
inline const CommandName kLogLevelCommand{"log-level"};   // {level: off|info|debug}
// What a host sends to start the product (the Web shell; {model?,
// preset?}); the product's part handles it, the library does not.
inline const CommandName kStartCommand{"start"};

inline const RequestName kTicksRequest{"ticks"};
inline const RequestName kCapabilitiesRequest{"capabilities"};
inline const RequestName kTopicsRequest{"topics"};

// The fixed English sentences of the session's refusals and notices.
inline constexpr const char* kAnotherOperationRuns = "Another operation is running.";
inline constexpr const char* kStopped = "Stopped.";
inline constexpr const char* kNoMachine = "No machine is running.";

bool sameDisplay(const machine::DisplayOutput&, const machine::DisplayOutput&);

struct DisplayState final : PayloadOf<DisplayState> {
    static constexpr PayloadKind kKind{"display"};
    explicit DisplayState(machine::DisplayOutput o) : output(std::move(o)) {}
    machine::DisplayOutput output;
    bool same(const DisplayState& o) const { return sameDisplay(output, o.output); }
    Value toValue() const override;
};

// Whether an operation runs, and the speed now, with or without one. Full
// speed and a playback (an operation) are both told here.
struct BusyState final : PayloadOf<BusyState> {
    static constexpr PayloadKind kKind{"busy"};
    BusyState(bool runs, pacing::Speed s) : operationRuns(runs), speed(s) {}
    bool operationRuns;
    pacing::Speed speed;
    bool same(const BusyState& o) const { return operationRuns == o.operationRuns && speed == o.speed; }
    Value toValue() const override;
};

struct Notice final : PayloadOf<Notice> {   // an English sentence for the user
    static constexpr PayloadKind kKind{"notice"};
    explicit Notice(std::string s) : sentence(std::move(s)) {}
    std::string sentence;
    bool same(const Notice& o) const { return sentence == o.sentence; }
    Value toValue() const override;
};

struct LogLine final : PayloadOf<LogLine> {
    static constexpr PayloadKind kKind{"log-line"};
    explicit LogLine(std::string t) : text(std::move(t)) {}
    std::string text;
    bool same(const LogLine& o) const { return text == o.text; }
    Value toValue() const override;
};

struct LogLevelState final : PayloadOf<LogLevelState> {
    static constexpr PayloadKind kKind{"log-level"};
    explicit LogLevelState(LogLevel l) : level(l) {}
    LogLevel level;
    bool same(const LogLevelState& o) const { return level == o.level; }
    Value toValue() const override;
};

struct EmulatedTimeReply final : PayloadOf<EmulatedTimeReply> {
    static constexpr PayloadKind kKind{"emulated-time"};
    EmulatedTimeReply(machine::Ticks t, double s) : ticks(t), seconds(s) {}
    machine::Ticks ticks;
    double seconds;
    bool same(const EmulatedTimeReply& o) const { return ticks == o.ticks && seconds == o.seconds; }
    Value toValue() const override;
};

struct MachineOffer final : PayloadOf<MachineOffer> {   // what the machine offers now
    static constexpr PayloadKind kKind{"machine-offer"};
    std::vector<machine::CapabilityEntry> capabilities;
    machine::Plugged plugged;
    std::vector<machine::PlaceName> places;
    bool same(const MachineOffer& o) const { return toValue() == o.toValue(); }
    Value toValue() const override;
};

struct TopicEntry {
    TopicName name;
    bool stream;
};
struct TopicList final : PayloadOf<TopicList> {   // every declared topic, in declaration order, before one is sent
    static constexpr PayloadKind kKind{"topic-list"};
    std::vector<TopicEntry> topics;
    bool same(const TopicList& o) const { return toValue() == o.toValue(); }
    Value toValue() const override;
};

} // namespace calcu::sessions
