#pragma once
#include "sessions/Value.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

// Reading the arguments of a command or request: a
// member of the argument object by key, with what a missing or mistyped
// member gives. A Web page's numbers arrive as integers when they are whole
// (library/web/glue/MessageCodec.cpp), so a command of a page that takes a
// whole number reads it with truncatedIntegerArg.

namespace calcu::sessions {

// The string, or "".
inline std::string textArg(const Value& args, std::string_view key) {
    const Value* v = args.find(key);
    return v && v->string() ? *v->string() : std::string();
}

// The integer, or `fallback` (a real is not an integer).
inline std::int64_t integerArg(const Value& args, std::string_view key, std::int64_t fallback = 0) {
    const Value* v = args.find(key);
    return v && v->integer() ? *v->integer() : fallback;
}

// The number (an integer or a real), or `fallback`.
inline double realArg(const Value& args, std::string_view key, double fallback = 0) {
    const Value* v = args.find(key);
    if (v && v->real()) return *v->real();
    if (v && v->integer()) return static_cast<double>(*v->integer());
    return fallback;
}

// The integer, a real truncated towards zero, or `fallback`.
inline std::int64_t truncatedIntegerArg(const Value& args, std::string_view key, std::int64_t fallback = 0) {
    const Value* v = args.find(key);
    if (v && v->integer()) return *v->integer();
    if (v && v->real()) return static_cast<std::int64_t>(*v->real());
    return fallback;
}

// True only for `true`.
inline bool flagArg(const Value& args, std::string_view key) {
    const Value* v = args.find(key);
    return v && v->boolean() && *v->boolean();
}

// The bytes, or std::nullopt (absent or not bytes: "could not be read").
inline std::optional<Value::Bytes> bytesArg(const Value& args, std::string_view key) {
    const Value* v = args.find(key);
    if (v && v->bytes()) return *v->bytes();
    return std::nullopt;
}

} // namespace calcu::sessions
