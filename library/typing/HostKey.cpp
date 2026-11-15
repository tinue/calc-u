#include "typing/HostKey.hpp"
#include "sessions/Args.hpp"
#include <array>
#include <utility>

namespace calcu::typing {

namespace {

constexpr std::array<std::pair<NamedKey, std::string_view>, 22> kNames{{
    {NamedKey::Escape, "escape"},     {NamedKey::Backspace, "backspace"}, {NamedKey::Return, "return"},
    {NamedKey::Enter, "enter"},       {NamedKey::Tab, "tab"},             {NamedKey::Delete, "delete"},
    {NamedKey::Up, "up"},             {NamedKey::Down, "down"},           {NamedKey::Left, "left"},
    {NamedKey::Right, "right"},       {NamedKey::F1, "f1"},               {NamedKey::F2, "f2"},
    {NamedKey::F3, "f3"},             {NamedKey::F4, "f4"},               {NamedKey::F5, "f5"},
    {NamedKey::F6, "f6"},             {NamedKey::ScrollLock, "scroll-lock"}, {NamedKey::Insert, "insert"},
    {NamedKey::Home, "home"},         {NamedKey::End, "end"},             {NamedKey::PageUp, "page-up"},
    {NamedKey::PageDown, "page-down"},
}};

} // namespace

std::string_view namedKeyName(NamedKey k) {
    for (const auto& [key, name] : kNames)
        if (key == k) return name;
    return {};
}

NamedKey namedKeyOf(std::string_view s) {
    for (const auto& [key, name] : kNames)
        if (name == s) return key;
    return NamedKey::None;
}

sessions::Value toValue(const HostKeyEvent& e) {
    return sessions::Value::object({{"physical", e.physical},
                                    {"named", std::string(namedKeyName(e.named))},
                                    {"produced", static_cast<std::int64_t>(e.produced)},
                                    {"carried", static_cast<std::int64_t>(e.carried)},
                                    {"shift", e.mods.shift},
                                    {"control", e.mods.control},
                                    {"alt", e.mods.alt},
                                    {"command", e.mods.command},
                                    {"meta", e.mods.meta},
                                    {"mac-host", e.mods.macHost},
                                    {"auto-repeat", e.autoRepeat}});
}

HostKeyEvent hostKeyEventOf(const sessions::Value& a) {
    using sessions::flagArg;
    HostKeyEvent e;
    e.physical = sessions::textArg(a, "physical");
    e.named = namedKeyOf(sessions::textArg(a, "named"));
    e.produced = static_cast<char32_t>(sessions::truncatedIntegerArg(a, "produced"));
    e.carried = static_cast<char32_t>(sessions::truncatedIntegerArg(a, "carried"));
    e.mods.shift = flagArg(a, "shift");
    e.mods.control = flagArg(a, "control");
    e.mods.alt = flagArg(a, "alt");
    e.mods.command = flagArg(a, "command");
    e.mods.meta = flagArg(a, "meta");
    e.mods.macHost = flagArg(a, "mac-host");
    e.autoRepeat = flagArg(a, "auto-repeat");
    return e;
}

} // namespace calcu::typing
