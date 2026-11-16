#pragma once
#include <cctype>
#include <cstdint>
#include <string>

#include "../SharpShiftedSymbols.hpp"
#include "PC1500Keyboard.hpp"

class PC1500Machine;

// ── Typed-text helpers shared by every scripted PC-1500 input path ───────
//
// the tests' PC1500BasicTyper (tests/support/) and the
// GUI's clipboard paste (Core/KeyPaste) both turn a host character into a
// PC-1500 keystroke and both need to know when the ROM is back at its
// BASIC prompt. Machine-free but for pc1500AtBasicPrompt() (PC1500TypedInput.cpp), so the
// app layer can resolve keys without a machine class (C1600-1).

/// Maps a host character to the PC-1500 key that types it. `needsShift`
/// is set when SHIFT must be tapped first (SHIFT is a one-shot latch on
/// this hardware): a-z (a physical PC-1500 has one keycap per letter and
/// SHIFT selects lowercase -- NOT the persistent SML toggle) and the
/// shared "second legend" punctuation (SharpShiftedSymbols.hpp). "^", Pi
/// and the square-root glyph are deliberately unmapped. Returns false for
/// a character with no key.
inline bool pc1500ResolveTypedChar(char c, std::string* baseKey, bool* needsShift) {
    if (c >= 'a' && c <= 'z') {
        *baseKey = std::string(1, static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
        *needsShift = true;
        return true;
    }
    if (sharpShiftedSymbolBaseKey(c, baseKey)) {
        *needsShift = true;
        return true;
    }
    *needsShift = false;
    if (c == ' ') {
        *baseKey = "space";
    } else {
        *baseKey = std::string(1, static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
    }
    return PC1500Keyboard::keyFromName(*baseKey) != PC1500Keyboard::Key::Unknown;
}

/// True when the interpreter is in its prompt command loop right now -- a
/// single-frame sample; callers require it to hold for a stretch. Measured
/// on A04: at the `>` prompt the end-of-frame PC is $E2AA ~90% of frames
/// and otherwise still within $E2xx-$E4xx (the loop body + the RTC /
/// key-scan interrupt service it wakes into); a running program or CE-150
/// plot never sits there (a pen-move busy-wait is at $F7xx). The ROM set
/// is fixed, so the window is safe to bake in.
bool pc1500AtBasicPrompt(const PC1500Machine& machine);
