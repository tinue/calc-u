#include "shell/Inputs.hpp"

namespace calcu::shell {

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
    // LIB-4 writes `available`/`reason` on the entry, or an object
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
