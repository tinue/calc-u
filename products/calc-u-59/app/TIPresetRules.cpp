#include "TIPresetRules.hpp"
#include "Bcd.hpp"
#include "TIDisplayPicture.hpp"
#include "media/MediaPlace.hpp"
#include "../core/TIDebugKinds.hpp"
#include "../core/TICatalogue.hpp"
#include "../core/TISegments.hpp"
#include <sstream>
#include <variant>

namespace cu59 {

using calcu::presets::Applies;

namespace {

std::string positionText(const calcu::machine::SegmentPosition& p) {
    std::uint8_t bits = 0;
    for (std::size_t s = 0; s < 7 && s < p.lit.size(); ++s)
        if (p.lit[s]) bits = static_cast<std::uint8_t>(bits | (1u << s));
    std::string t = " ";
    if (p.brightness.value_or(0) > 0) {
        for (int i = 0; i < 10; ++i)
            if (bits == kDigitSegments[i]) t = std::string(1, static_cast<char>('0' + i));
        if (bits == kSegmentsMinus) t = "-";
    }
    if (p.lit.size() > static_cast<std::size_t>(kTIDecimalPoint) && p.lit[kTIDecimalPoint]) t += ".";
    return t;
}

bool anyLit(const calcu::machine::DisplayOutput& o) {
    const auto* f = std::get_if<calcu::machine::SegmentFrame>(&o.frame);
    if (!o.on || !f) return false;
    for (const auto& p : f->positions)
        if (p.brightness.value_or(0) > 0)
            for (bool l : p.lit)
                if (l) return true;
    return false;
}

// The number the display shows while one is keyed in: the mantissa and,
// apart from it, the exponent (`1.5   03`, `-2. -12`).
std::optional<RegisterNibbles> fromDisplay(const std::string& text) {
    std::istringstream in(text);
    std::vector<std::string> words;
    for (std::string w; in >> w;) words.push_back(w);
    if (words.empty() || words.size() > 2) return std::nullopt;
    std::string number = words[0];
    if (!number.empty() && number.back() == '.') number.pop_back();
    if (words.size() == 2) number += "e" + words[1];
    return parseDecimal(number);
}

} // namespace

std::string tiDisplayText(const calcu::machine::DisplayOutput& o) {
    const auto* f = std::get_if<calcu::machine::SegmentFrame>(&o.frame);
    if (!f) return {};
    std::string t;
    for (std::size_t i = f->positions.size(); i-- > 0;) t += positionText(f->positions[i]);
    const std::size_t a = t.find_first_not_of(' '), b = t.find_last_not_of(' ');
    return a == std::string::npos ? std::string() : t.substr(a, b - a + 1);
}

std::optional<RegisterNibbles> displayRegisterX(calcu::machine::Machine& m, const calcu::machine::DisplayOutput& o) {
    const CalculatorState* panel = calcu::machine::first<CalculatorState>(m);
    if (panel) {
        const CpuFrame cpu = panel->snapshotCPU();
        // X between two steps (app/DebugSnapshots.cpp decodeX; rom/TI59-ROM-Guide.md,
        // "Number formats"): a result on display (fB[5]) normalized in C with its
        // signs in fB[2]/fB[3]; else normalized in A with its signs in fA[2]/fA[3]
        // (an error's clamp, a keycode sub-program's launch); else the number
        // being keyed in, as the display shows it.
        auto normalized = [](const std::uint8_t* n, std::uint16_t flags) -> std::optional<RegisterNibbles> {
            bool digits = true;
            for (int i = 1; i < 16; ++i) digits = digits && n[i] <= 9;
            bool zero = true;
            for (int i = 3; i < 16; ++i) zero = zero && n[i] == 0;
            if (n[0] != 0xC || !digits || (n[15] == 0 && !zero)) return std::nullopt;   // normalized: MSD first
            RegisterNibbles r{};
            for (int i = 1; i < 16; ++i) r[static_cast<std::size_t>(i)] = n[i];
            r[0] = static_cast<std::uint8_t>(((flags & (1u << 2)) ? 2 : 0) | ((flags & (1u << 3)) ? 4 : 0));
            return r;
        };
        if (cpu.fB & (1u << 5)) {
            if (auto r = normalized(cpu.C, cpu.fB)) return r;
        } else if (auto r = normalized(cpu.A, cpu.fA)) {
            return r;
        }
    }
    return fromDisplay(tiDisplayText(o));
}

bool sameValue(const RegisterNibbles& a, const RegisterNibbles& b) {
    auto zero = [](const RegisterNibbles& n) {
        for (int i = 3; i < 16; ++i)
            if (n[static_cast<std::size_t>(i)] != 0) return false;
        return true;
    };
    if (zero(a) || zero(b)) return zero(a) && zero(b);
    for (int i = 3; i < 16; ++i)
        if (a[static_cast<std::size_t>(i)] != b[static_cast<std::size_t>(i)]) return false;
    const bool expZero = a[1] == 0 && a[2] == 0;
    return (a[0] & 2) == (b[0] & 2) && a[1] == b[1] && a[2] == b[2] && (expZero || (a[0] & 4) == (b[0] & 4));
}

calcu::presets::IdleRule TIModelRules::idle() const {
    // AC-36, AC-53, D-43: key readiness or the card wait of Write / INV
    // Write, without a break for 100 ms of emulated time, asked every 1 ms.
    calcu::presets::IdleRule r;
    r.readyNow = [](const calcu::machine::Machine& m) {
        if (m.isKeyReady()) return true;
        // The card reader's media place says when the ROM waits for a card (C59-7 AC-9).
        for (const auto& e : m.capabilities(calcu::media::MediaPlace::kKind))
            if (auto* p = calcu::machine::capabilityAs<calcu::media::MediaPlace>(m, e.id); p && p->waitsForMedium()) return true;
        return false;
    };
    r.stretch = calcu::machine::Seconds(0.1);
    const calcu::machine::TickRate rate = tiCatalogue().model(calcu::machine::ModelId{m_model})->tickRate;
    r.sample = rate / 1000;
    return r;
}

std::optional<std::vector<std::uint8_t>> TIModelRules::screenshot(calcu::machine::Machine&,
                                                                  const calcu::machine::DisplayOutput& o) {
    return displayPicture(o);
}

calcu::presets::ExpectReading TIModelRules::compare(calcu::machine::Machine& m, const calcu::machine::DisplayOutput& o,
                                                    const calcu::presets::ExpectText& e) {
    calcu::presets::ExpectReading r;
    if (!anyLit(o)) {   // EC-7
        r.displayOff = true;
        r.shows = "nothing (the display is dark)";
        return r;
    }
    r.shows = tiDisplayText(o);
    const auto want = parseDecimal(e.text);
    const auto x = displayRegisterX(m, o);
    r.found = want && x && sameValue(*want, *x);
    return r;
}

std::optional<std::string> TIModelRules::checkExpect(const calcu::presets::ExpectText& e, std::string_view) const {
    if (e.regex) return std::string("A TI model compares expect: with a number; a regular expression is not taken.");
    if (!parseDecimal(e.text)) return std::string("A TI model compares expect: with a number.");
    return std::nullopt;
}

Applies TIModelRules::applies(std::string_view k) const {
    if (k == "type" || k == "sync-clock") return Applies::NotForThisModel;
    if (m_model != "TI-59" && (k == "magnetic-cards" || k == "card" || k == "arm")) return Applies::NotForThisModel;
    return Applies::Yes;
}

calcu::presets::PlaceKeyRule TIModelRules::placeKey(const calcu::machine::PlaceName& place) const {
    calcu::presets::PlaceKeyRule r;
    if (place == kTIModulePlace) {   // AC-35, D-42
        r.bare = false;
        r.byFile = false;
        r.noFile = "A Solid State ROM module is named by name: in this release.";
    } else {
        r.byFile = false;
    }
    return r;
}

void TIModelRules::checkHeader(const calcu::presets::LoadPlan&, calcu::presets::ReadContext&) const {}

calcu::presets::ModelRules& TIPresetRules::forModel(const calcu::machine::ModelId& m) {
    if (m.value == "TI-58") return m_ti58;
    if (m.value == "TI-58C") return m_ti58c;
    return m_ti59;
}

} // namespace cu59
