#pragma once
#include <string>
#include <vector>

// ── KBII sequences and the latches they read ─────────────────────────────
//
// Paste Text and the live keys go through the library's typing engine since
// M-36 (LIB-5; app/SharpTyping): what remains here is the one rule for an
// accented character on the PC-1600, which the engine's PC-1600 profile and
// the tests' PC1600BasicTyper (tests/support/) both play.

/// SHIFT / KBII as latched on the machine when an accented character's
/// KBII sequence is about to start (pc1600ReadLatches, the typing support):
///   nothing latched  KBII, [SHIFT,] key, KBII
///   SHIFT            SHIFT (un-latch -- with SHIFT on, the KBII key is the
///                    key-click toggle), then the same
///   KBII (+/- SHIFT) nothing: the character is dropped silently
struct KeyLatches {
    bool shift = false;
    bool kbii = false;
};

/// One tap of an accented character's KBII sequence. Every KBII / SHIFT tap
/// is followed by the SHIFT gap, so the key-scan sees it before the next key.
struct KbiiTap {
    std::string key;
    bool gapAfter = false;
};

/// The taps that type `key` (with SHIFT first when `shift`) under KBII,
/// given what is latched (see KeyLatches); empty when the character is
/// dropped. The last tap is the closing KBII.
inline std::vector<KbiiTap> kbiiSequence(const std::string& key, bool shift, const KeyLatches& latched) {
    if (latched.kbii) return {}; // the user's own KBII is on: ignored silently

    // KBII latches until tapped again: every accented character is its own
    // KBII-on ... KBII-off sequence, so the next character always starts
    // with KBII off.
    std::vector<KbiiTap> seq;
    // With SHIFT on, the KBII key toggles the key click: un-latch it.
    if (latched.shift) seq.push_back({"shift", true});
    seq.push_back({"kbii", true});
    if (shift) seq.push_back({"shift", true});
    seq.push_back({key, false});
    seq.push_back({"kbii", true});
    return seq;
}
