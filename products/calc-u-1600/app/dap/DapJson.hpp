#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "sessions/Value.hpp"

// JSON text to and from calcu::sessions::Value (C1600-3 design §3.1, D-4):
// the DAP's messages. A number without a fraction or an exponent is an
// integer, any other a double; objects keep their members in order. The
// DAP is the only user; a second one moves this to library/formats/.

namespace shared::dap {

using calcu::sessions::Value;

// The value of `text`, or std::nullopt when it is not one JSON value.
std::optional<Value> parseJson(std::string_view text);
// One line of JSON (bytes, which JSON has not, as base64 text).
std::string writeJson(const Value& value);

// ── Reading and building values ─────────────────────────────────────────

// A member of an object (nullptr: none, or `v` is not an object).
const Value* member(const Value& v, std::string_view key);
std::string textOf(const Value& v, std::string_view key, std::string fallback = {});
std::int64_t intOf(const Value& v, std::string_view key, std::int64_t fallback = 0);
bool boolOf(const Value& v, std::string_view key, bool fallback = false);
bool isObject(const Value* v);
bool isArray(const Value* v);
// The members of an object (empty for anything else), to change and wrap again.
Value::Object membersOf(const Value& v);
// Sets `key` (replaces it where it is, else appends it).
void put(Value::Object& o, std::string_view key, Value value);

std::string base64(std::string_view bytes);
std::string unbase64(std::string_view text);

} // namespace shared::dap
