#pragma once
#include "machine/Ids.hpp"
#include "sessions/HostServices.hpp"
#include "sessions/Payload.hpp"
#include "sessions/Value.hpp"
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

// Messages: all that crosses the boundary of a session. The session hands its
// outbox to each listener's deliver() in order, on the session's thread;
// whether a sink acts at once or queues for another thread is the sink's.
// A sink does not call back into the session from deliver().

namespace calcu::sessions {

using TopicName   = machine::Id<struct TopicTag>;     // "display", "busy", "notices", "log", "c1600.inspector"
using CommandName = machine::Id<struct CommandTag>;
using RequestName = machine::Id<struct RequestTag>;
using ListenerId  = std::uint32_t;
using CallId      = std::uint64_t;                    // per session, increasing

struct Command { CommandName name; Value args; };
struct Request { RequestName name; Value args; };

struct StateMessage  { TopicName topic; PayloadPtr state; };
struct StreamMessage { TopicName topic; bool reset = false; std::vector<PayloadPtr> appended; };
struct ReplyMessage  { CallId call = 0; bool done = false; std::string reason; PayloadPtr result; };
struct ServiceAsk    { CallId ask = 0; HostService service; Value args; };
struct SoundStart    { int sampleRate = 0; };
struct SoundSamples  { std::vector<std::int16_t> mono; };
struct SoundStop     {};
using Message = std::variant<StateMessage, StreamMessage, ReplyMessage, ServiceAsk, SoundStart, SoundSamples, SoundStop>;

class MessageSink {
public:
    virtual void deliver(const Message&) = 0;

protected:
    ~MessageSink() = default;
};

// Replies a request handler gives.
inline ReplyMessage replyGiven(PayloadPtr result) { return ReplyMessage{0, true, {}, std::move(result)}; }
inline ReplyMessage replyRefused(std::string reason) { return ReplyMessage{0, false, std::move(reason), nullptr}; }

} // namespace calcu::sessions
