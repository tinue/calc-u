#include "debugger/RunState.hpp"

namespace calcu::debugger {

using sessions::Value;

namespace {

const char* reasonName(RunState::Reason r) {
    switch (r) {
    case RunState::Reason::Pause: return "pause";
    case RunState::Reason::Step: return "step";
    case RunState::Reason::Breakpoint: return "breakpoint";
    case RunState::Reason::Watch: return "watch";
    case RunState::Reason::PauseOnStart: return "pause-on-start";
    case RunState::Reason::Entry: return "entry";
    }
    return "";
}

} // namespace

Value RunState::toValue() const {
    Value::Object o;
    o.push_back({"paused", paused});
    o.push_back({"owner", owner ? Value(owner->value) : Value()});
    o.push_back({"reason", reason ? Value(reasonName(*reason)) : Value()});
    o.push_back({"target", target ? Value(target->value) : Value()});
    Value::Array h;
    for (const auto& x : hits) h.push_back(Value::object({{"id", x.id}, {"setter", x.setter}}));
    o.push_back({"hits", Value(std::move(h))});
    o.push_back({"armed", armed ? Value(armed->value) : Value()});
    o.push_back({"pauseAtStart", pauseAtStart});
    return Value(std::move(o));
}

Value TraceState::toValue() const { return Value::object({{"running", running}, {"file", fileName}}); }

} // namespace calcu::debugger
