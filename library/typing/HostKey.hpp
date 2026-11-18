#pragma once
#include "sessions/Value.hpp"
#include <string>
#include <string_view>

// Host keys: what a frontend says of a host key event, as the host reports
// it. The frontend maps nothing; the product's keyboard map does (KeyboardMap.hpp).

namespace calcu::typing {

// The non-character keys a keyboard map may bind (the union of Calc-U-1600's
// host keys and Calc-U-59's Escape). Anything else is None and is mapped by
// the character it produced.
enum class NamedKey {
    None, Escape, Backspace, Return, Enter /*the number pad's*/, Tab, Delete,
    Up, Down, Left, Right, F1, F2, F3, F4, F5, F6,
    ScrollLock, Insert, Home, End, PageUp, PageDown,
};

// Modifiers by what the user pressed, not by how a toolkit names them (Qt
// on macOS calls Command "Control" and Control "Meta"). `macHost`: the host
// is a Mac (its command modifiers are Command and Control).
struct Modifiers {
    bool shift = false;
    bool control = false;
    bool alt = false;       // Option on a Mac
    bool command = false;   // the Mac's Command
    bool meta = false;      // the Windows or Super key
    bool macHost = false;
};

struct HostKeyEvent {
    // The host's own identity of the key (a browser's `code`, Qt's native
    // scan code, an NSEvent keyCode): it pairs a key-down with its key-up.
    std::string physical;
    NamedKey named = NamedKey::None;
    char32_t produced = 0;   // the character the layout produced; 0 for none or more than one
    char32_t carried = 0;    // a letter key: the letter it carries on the user's layout, lower case (in every frontend)
    Modifiers mods;
    bool autoRepeat = false;   // the host's own repeat: ignored
};

// The names of NamedKey in a command's arguments ("escape", "page-up", …;
// "" for None), and back (None for an unknown name).
std::string_view namedKeyName(NamedKey);
NamedKey namedKeyOf(std::string_view);

// The event as the command `host-key-down` carries it: {physical, named,
// produced, carried, shift, control, alt, command, meta, mac-host,
// auto-repeat}; produced and carried as code points.
sessions::Value toValue(const HostKeyEvent&);
HostKeyEvent hostKeyEventOf(const sessions::Value&);

} // namespace calcu::typing
