#include "Machine/SharpDisplayOutput.hpp"

#include <algorithm>
#include <string_view>

using namespace calcu::machine;

namespace sharp {

namespace {

int symbolIndex(std::string_view name) {
    for (std::size_t i = 0; i < StatusLine::kCount; ++i)
        if (name == kStatusSymbolNames[i]) return static_cast<int>(i);
    return -1;
}

} // namespace

DisplayOutput displayOutput(const MachineDescriptor& d, bool on, int width, int height, std::vector<std::uint8_t> dots,
                            const StatusLine& line) {
    DisplayOutput out;
    out.on = on;
    if (!on) std::fill(dots.begin(), dots.end(), std::uint8_t{0});
    out.frame = DotMatrixFrame{width, height, std::move(dots)};
    for (const IndicatorDef& ind : d.display.indicators) {
        const int i = symbolIndex(ind.name);
        const bool lit = on && i >= 0 && line.all()[static_cast<std::size_t>(i)];
        out.indicators.push_back({ind.name, lit, std::nullopt});
    }
    return out;
}

StatusLine statusLineOf(const DisplayOutput& o) {
    StatusLine line;
    for (const IndicatorOutput& ind : o.indicators)
        if (const int i = symbolIndex(ind.name); i >= 0) line.set(static_cast<StatusLine::Symbol>(i), ind.on);
    return line;
}

} // namespace sharp
