#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>

#include "AlpsPlotterMechanism.hpp"
#include "paper/Paper.hpp"
#include "paper/PaperOutput.hpp"
#include <variant>
#include <vector>

// ── The plotters' paper output (LIB-8 design §10.3, §10.4; spec AC-32 to AC-34) ──
//
// The CE-150 and the CE-1600P offer the library's generic kind
// `paper-output`: pen strokes in four colours on a roll, no text layer. Each
// card owns one beside its AlpsPlotterMechanism, made and gone with the card
// (D-16), and hands it to the mechanism as its PenListener. The paper itself
// lives in the session (calcu::paper::PaperPart); this only turns the
// mechanism's pen calls into marks.
//
// Units: one unit of the description is one raw step of the mechanism,
// 190/1920 mm (a full motor step counts 2). A place is
// x = printableLeft + penX, y = yBase + penY (in ten-thousandths of a unit).
// A reset of the mechanism keeps the paper where it stands: the open stroke
// is closed and yBase takes the old penY, so the next plot goes on below
// what was drawn (D-11; ledger 1600-34).

namespace sharp {

// AC-32: 56 mm of which 42.75 mm drawn on (CE-150), 210 mm of which 190 mm
// (CE-1600P), the margins split evenly; a stroke of one point (25.4/72 mm);
// pictures at 1200 dots per inch (AC-35).
inline calcu::paper::PaperDescription plotterPaperDescription(calcu::paper::Coord width, calcu::paper::Coord left,
                                                              calcu::paper::Coord printable) {
    calcu::paper::PaperDescription d;
    d.unit = calcu::paper::Millimetres{190, 1920};
    d.width = width;
    d.printableLeft = left;
    d.printableWidth = printable;
    d.form = calcu::paper::MarkForm::PenStrokes;
    d.strokeWidth = 35'649;
    d.pictureDotsPerInch = 1200;
    d.textLayer = false;
    return d;
}
inline calcu::paper::PaperDescription ce150PaperDescription() {
    return plotterPaperDescription(5'658'947, 669'474, 4'320'000);
}
inline calcu::paper::PaperDescription ce1600pPaperDescription() {
    return plotterPaperDescription(21'221'053, 1'010'526, 19'200'000);
}

// AC-34: black, blue, dark green, red, in AlpsPlotterMechanism::PenColor's order.
inline constexpr std::array<calcu::paper::Rgb, 4> kPenColours{
    {{0, 0, 0}, {0, 0, 255}, {0, 128, 0}, {255, 0, 0}}};

class PlotterPaperOutput final : public calcu::paper::PaperOutputBase, public AlpsPlotterMechanism::PenListener {
public:
    explicit PlotterPaperOutput(calcu::paper::PaperDescription d) : m_description(std::move(d)) {}
    PlotterPaperOutput(const PlotterPaperOutput&) = delete;
    PlotterPaperOutput& operator=(const PlotterPaperOutput&) = delete;

    const calcu::paper::PaperDescription& description() const override { return m_description; }
    calcu::paper::Coord standing() const override { return m_yBase + m_penY * calcu::paper::kPerUnit; }

    // AlpsPlotterMechanism::PenListener
    void penDown(AlpsPlotterMechanism::PenColor colour, int x, int y) override {
        m_penY = y;
        m_down = true;
        emitPenDown(kPenColours[static_cast<std::size_t>(colour) & 3u], at(x, y));
    }
    void penTo(int x, int y) override {
        m_penY = y;
        emitPenTo(at(x, y));
    }
    void penUp() override {
        m_down = false;
        emitPenUp();
    }
    void fedTo(int y) override {
        emitFedTo(m_yBase + calcu::paper::Coord(y) * calcu::paper::kPerUnit);   // before standing() moves
        m_penY = y;
    }
    void reanchored() override {
        if (m_down) penUp();
        m_yBase += m_penY * calcu::paper::kPerUnit;
        m_penY = 0;
    }

private:
    calcu::paper::Position at(int x, int y) const {
        return calcu::paper::Position{m_description.printableLeft + calcu::paper::Coord(x) * calcu::paper::kPerUnit,
                                      m_yBase + calcu::paper::Coord(y) * calcu::paper::kPerUnit};
    }

    calcu::paper::PaperDescription m_description;
    calcu::paper::Coord m_yBase = 0;
    calcu::paper::Coord m_penY = 0;   // raw steps
    bool m_down = false;
};

// A plotter's paper back in the mechanism's raw steps, one point per place,
// `newStroke` on each stroke's first: the form of today's summaries (the
// golden runs' `paper` lines, CALCU_PAPER_OUT of the tools; LIB-8 §10.7).
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
// of its own (tests and today's tools): its points in raw steps. Empty for
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
