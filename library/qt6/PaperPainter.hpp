#pragma once
#include "paper/PaperOutput.hpp"
#include <QPainter>
#include <vector>

// Draws the marks of a paper on screen (LIB-8 design §3.7; spec AC-12,
// AC-34): a dot as a disc of its scaled diameter in the grey of its
// darkness, a pen stroke in its colour at its scaled width but at least one
// point, with round ends and joins. Only the marks between `viewTop` and
// `viewBottom` (paper y, in Coord) are drawn. The panel uses it; the picture
// of Copy is the library's own (paper/Picture.hpp).

namespace calcu::qt6 {

struct PaperScale {
    double pointsPerCoord = 0;   // the panel's width over the paper's width
    paper::Coord top = 0;        // the paper y drawn at `originY`
    double originY = 0;          // in the painter's points
};

// The extent of the marks and the reach in paper y: the top of the highest
// mark or reach.from, and the bottom of the lowest mark or reach.to.
struct PaperExtent {
    paper::Coord top = 0, bottom = 0;
};
PaperExtent paperExtent(const paper::PaperDescription&, const std::vector<paper::Mark>&, paper::Reach);

void paintMarks(QPainter&, const paper::PaperDescription&, const std::vector<paper::Mark>&, const PaperScale&,
                paper::Coord viewTop, paper::Coord viewBottom);

} // namespace calcu::qt6
