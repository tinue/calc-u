#include "shell/Inputs.hpp"

namespace calcu::shell {

bool takeState(ShellInputs& in, const InputNames& names, const sessions::StateMessage& s) {
    sessions::Value* input = nullptr;
    if (s.topic == names.controlState) input = &in.controlState;
    else if (s.topic == names.busy) input = &in.busy;
    else if (s.topic == names.runState) input = &in.runState;
    else if (s.topic == names.romsMissing) input = &in.romsMissing;
    else if (s.topic == names.settings) input = &in.settings;
    else if (s.topic == names.media) input = &in.media;
    else if (const auto t = in.productTopics.find(s.topic.value); t != in.productTopics.end()) input = &t->second;
    if (!input) return false;
    sessions::Value v = s.state ? s.state->toValue() : sessions::Value();
    if (v == *input) return false;
    *input = std::move(v);
    return true;
}

namespace read {

std::string text(const sessions::Value& v, std::string_view key) {
    const sessions::Value* m = v.find(key);
    return m && m->string() ? *m->string() : std::string();
}

bool flag(const sessions::Value& v, std::string_view key, bool fallback) {
    const sessions::Value* m = v.find(key);
    return m && m->boolean() ? *m->boolean() : fallback;
}

std::int64_t integer(const sessions::Value& v, std::string_view key, std::int64_t fallback) {
    const sessions::Value* m = v.find(key);
    if (m && m->integer()) return *m->integer();
    if (m && m->real()) return static_cast<std::int64_t>(*m->real());
    return fallback;
}

const sessions::Value::Array& elements(const sessions::Value& v, std::string_view key) {
    static const sessions::Value::Array kNone;
    const sessions::Value* m = v.find(key);
    return m && m->elements() ? *m->elements() : kNone;
}

} // namespace read

Availability availabilityOf(const sessions::Value& entry, std::string_view key) {
    const sessions::Value* holder = &entry;
    // The control state writes `available`/`reason` on the entry, or an object
    // {available, reason} under its own key (`empty`, `insert`).
    if (key != "available") {
        const sessions::Value* sub = entry.find(key);
        if (!sub) return {};
        holder = sub;
        key = "available";
    }
    Availability a;
    a.now = read::flag(*holder, key, true);
    if (!a.now) a.reason = read::text(*holder, "reason");
    return a;
}

Busy busyOf(const sessions::Value& busy) {
    return Busy{read::flag(busy, "operationRuns"), read::text(busy, "speed") == "full"};
}

} // namespace calcu::shell
