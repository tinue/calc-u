#pragma once
#include "debugger/ButtonRule.hpp"
#include "debugger/RunState.hpp"
#include "sessions/Value.hpp"
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

// The table of the run-control buttons' rule (LIB-10 design §3.10),
// button_cases.json beside this file, read by the C++ tests of both the
// library and the Qt6 component set; the Playwright test reads the same file.

namespace calcu::debugger::test {

struct ButtonCase {
    LevelName level;
    std::optional<RunState> state;
    ButtonUse use;
};

struct ButtonCases {
    int repeatMs = 0;
    std::vector<ButtonCase> cases;
};

namespace json {
// A small reader of JSON into sessions::Value: enough for the table.
struct Reader {
    const std::string& s;
    std::size_t i = 0;
    void ws() {
        while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
    }
    sessions::Value value() {
        ws();
        if (i >= s.size()) return {};
        const char c = s[i];
        if (c == '{') {
            ++i;
            sessions::Value::Object o;
            ws();
            if (s[i] == '}') return ++i, sessions::Value(std::move(o));
            for (;;) {
                ws();
                std::string k = *value().string();
                ws();
                ++i;   // ':'
                o.push_back({std::move(k), value()});
                ws();
                if (s[i++] == '}') return sessions::Value(std::move(o));
            }
        }
        if (c == '[') {
            ++i;
            sessions::Value::Array a;
            ws();
            if (s[i] == ']') return ++i, sessions::Value(std::move(a));
            for (;;) {
                a.push_back(value());
                ws();
                if (s[i++] == ']') return sessions::Value(std::move(a));
            }
        }
        if (c == '"') {
            std::string out;
            for (++i; i < s.size() && s[i] != '"'; ++i) {
                if (s[i] == '\\') ++i;
                out += s[i];
            }
            ++i;
            return sessions::Value(std::move(out));
        }
        if (s.compare(i, 4, "true") == 0) return i += 4, sessions::Value(true);
        if (s.compare(i, 5, "false") == 0) return i += 5, sessions::Value(false);
        if (s.compare(i, 4, "null") == 0) return i += 4, sessions::Value();
        std::size_t end = i;
        while (end < s.size() && (std::isdigit(static_cast<unsigned char>(s[end])) || s[end] == '-')) ++end;
        const long long n = std::stoll(s.substr(i, end - i));
        i = end;
        return sessions::Value(n);
    }
};
} // namespace json

inline ButtonCases readButtonCases() {
    const std::filesystem::path p = std::filesystem::path(__FILE__).parent_path() / "button_cases.json";
    std::ifstream in(p, std::ios::binary);
    const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    json::Reader r{text};
    const sessions::Value root = r.value();
    ButtonCases out;
    if (const auto* ms = root.find("repeatMs"); ms && ms->integer()) out.repeatMs = static_cast<int>(*ms->integer());
    if (const auto* cs = root.find("cases"); cs && cs->elements())
        for (const auto& c : *cs->elements()) {
            ButtonCase bc;
            bc.level = LevelName{*c.find("level")->string()};
            if (const auto* st = c.find("state"); st && !st->isNull()) {
                RunState s;
                s.paused = *st->find("paused")->boolean();
                if (const auto* o = st->find("owner"); o && o->string()) s.owner = LevelName{*o->string()};
                bc.state = s;
            }
            bc.use = ButtonUse{*c.find("pause")->boolean(), *c.find("step")->boolean(), *c.find("resume")->boolean()};
            out.cases.push_back(std::move(bc));
        }
    return out;
}

} // namespace calcu::debugger::test
