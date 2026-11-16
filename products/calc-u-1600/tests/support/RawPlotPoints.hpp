#pragma once
#include <cstdint>
#include <variant>
#include <vector>

#include "Connector/PlotterPaperOutput.hpp"
#include "paper/Paper.hpp"
#include "paper/PaperOutput.hpp"

// The tests' reading of a plotter's paper, moved out of the core's
// Connector/PlotterPaperOutput.hpp (no product code reads it).

namespace sharp {

// A plotter's paper back in the mechanism's raw steps, one point per place,
// `newStroke` on each stroke's first: the form of today's summaries (the
// golden runs' `paper` lines, CALCU_PAPER_OUT of v0.8.1's CLIs; LIB-8 §10.7).
struct RawPlotPoint {
    std::int32_t x = 0, y = 0;
    std::uint8_t color = 0, newStroke = 0;
};
inline std::vector<RawPlotPoint> rawPlotPoints(const calcu::paper::Paper& paper) {
    std::vector<RawPlotPoint> out;
    const calcu::paper::Coord left = paper.description().printableLeft;
    for (const calcu::paper::Mark& m : paper.marks()) {
        const auto* s = std::get_if<calcu::paper::Stroke>(&m);
        if (!s) continue;
        std::uint8_t colour = 0;
        for (std::size_t i = 0; i < kPenColours.size(); ++i)
            if (kPenColours[i] == s->colour) colour = static_cast<std::uint8_t>(i);
        bool first = true;
        for (const calcu::paper::Position& p : s->path) {
            out.push_back(RawPlotPoint{static_cast<std::int32_t>((p.x - left) / calcu::paper::kPerUnit),
                                       static_cast<std::int32_t>(p.y / calcu::paper::kPerUnit), colour,
                                       static_cast<std::uint8_t>(first ? 1 : 0)});
            first = false;
        }
    }
    return out;
}

// What the plotter kept while no paper was connected, read once on a paper
// of its own (tests): its points in raw steps. Empty for
// nullptr. The output keeps what follows again.
inline std::vector<RawPlotPoint> takeRawPlotPoints(calcu::paper::PaperOutput* output) {
    if (!output) return {};
    calcu::paper::Paper paper(output->description());
    paper.bind(output);
    std::vector<RawPlotPoint> points = rawPlotPoints(paper);
    paper.bind(nullptr);
    return points;
}

} // namespace sharp
