#include "app/LcdLayout.hpp"

#include <string_view>

#include "Display/LcdText.hpp"
#include "machine/Display.hpp"

namespace cu1600::lcd {

namespace {

bool lit(const calcu::machine::DisplayOutput& frame, std::string_view name) {
    for (const auto& ind : frame.indicators)
        if (ind.name == name) return ind.on;
    return false;
}

double p16X(std::string_view item) {
    for (const auto& i : kP16Items)
        if (item == i.name) return p16StartX(i.rawX);
    return 0.0;
}

using K = IndicatorSlot::Kind;
using S = Indicator::Style;

IndicatorSlot flag(const char* name, double x, const char* caption, double scale = 1.0, S style = S::Text) {
    return IndicatorSlot{K::Flag, {name}, caption, x, scale, style};
}

std::vector<IndicatorSlot> pc1500Slots() {
    return {flag("BUSY", kP15BusyX, "BUSY"),
            flag("SHIFT", kP15ShiftX, "SHIFT"),
            flag("KANA", kP15JapX, "JPN"),
            flag("SMALL", kP15SmallX, "SMALL"),
            flag("RUN", kP15RunX, "RUN"),
            flag("PRO", kP15ProX, "PRO"),
            flag("RESERVE", kP15ReserveX, "RESERVE"),
            flag("DEF", kP15DefX, "DEF"),
            flag("I", kP15RomanIX, "I"),
            flag("II", kP15RomanIIX, "II"),
            flag("III", kP15RomanIIIX, "III"),
            // DE/G/RAD read as one word (DE+G = DEG, G+RAD = GRAD).
            IndicatorSlot{K::Join, {"DE", "G", "RAD"}, "", kP15AngleX},
            IndicatorSlot{K::Always, {}, "", kP15BatteryX, 1.0, S::Dot}};
}

std::vector<IndicatorSlot> pc1600Slots() {
    return {flag("BUSY", p16X("busy"), "BUSY"),
            flag("SHIFT", p16X("shift"), "SHIFT"),
            flag("S", p16X("s"), "S", 1.0, S::Inverted),
            // The romaji->kana caption shares the window after S and is
            // printed small on the glass. The "kana" position is an estimate
            // (the other positions are measured) that keeps both parts clear
            // of SMALL.
            flag("ROMAJI", p16X("romaji"), "ローマ字", 0.75),
            flag("KANA", p16X("kana"), "→カナ", 0.75),
            flag("SMALL", p16X("small"), "SMALL"),
            flag("RUN", p16X("run"), "RUN"),
            flag("PRO", p16X("pro"), "PRO"),
            flag("RESERVE", p16X("reserve"), "RESERVE"),
            flag("DEF", p16X("def"), "DEF"),
            flag("CTRL", p16X("ctrl"), "CTRL"),
            flag("BATT", p16X("batt"), "BATT", 1.0, S::Inverted),
            IndicatorSlot{K::Join, {"DE", "G", "RAD"}, "", p16X("degrad")},
            // I/II/III share one position: the first lit one wins.
            IndicatorSlot{K::First, {"I", "II", "III"}, "", p16X("roman")}};
}

} // namespace

const std::vector<IndicatorSlot>& indicatorSlots(const calcu::machine::ModelId& model) {
    static const std::vector<IndicatorSlot> p15 = pc1500Slots();
    static const std::vector<IndicatorSlot> p16 = pc1600Slots();
    return model.value == "PC-1600" ? p16 : p15;
}

std::vector<Indicator> indicators(const calcu::machine::ModelId& model, const calcu::machine::DisplayOutput& frame) {
    std::vector<Indicator> out;
    for (const IndicatorSlot& s : indicatorSlots(model)) {
        switch (s.kind) {
            case K::Flag:
                if (lit(frame, s.names[0])) out.push_back({s.caption, s.x, s.scale, s.style});
                break;
            case K::Join: {
                std::string text;
                for (const auto& n : s.names)
                    if (lit(frame, n)) text += n;
                if (!text.empty()) out.push_back({text, s.x, s.scale, s.style});
                break;
            }
            case K::First:
                for (const auto& n : s.names)
                    if (lit(frame, n)) {
                        out.push_back({n, s.x, s.scale, s.style});
                        break;
                    }
                break;
            case K::Always:
                out.push_back({s.caption, s.x, s.scale, s.style});
                break;
        }
    }
    return out;
}

} // namespace cu1600::lcd
