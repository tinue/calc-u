#include "qt6/PaperPainter.hpp"
#include <QPainterPath>
#include <QPen>
#include <algorithm>

namespace calcu::qt6 {

using namespace calcu::paper;

namespace {
Coord halfOf(const PaperDescription& d, const Mark& m) {
    return std::holds_alternative<Dot>(m) ? d.dotDiameter / 2 : d.strokeWidth / 2;
}
} // namespace

PaperExtent paperExtent(const PaperDescription& d, const std::vector<Mark>& marks, Reach reach) {
    PaperExtent e{reach.from, reach.to};
    for (const Mark& m : marks) {
        const Coord h = halfOf(d, m);
        if (const auto* dot = std::get_if<Dot>(&m)) {
            e.top = std::min(e.top, dot->at.y - h);
            e.bottom = std::max(e.bottom, dot->at.y + h);
        } else {
            for (const Position& p : std::get<Stroke>(m).path) {
                e.top = std::min(e.top, p.y - h);
                e.bottom = std::max(e.bottom, p.y + h);
            }
        }
    }
    return e;
}

void paintMarks(QPainter& p, const PaperDescription& d, const std::vector<Mark>& marks, const PaperScale& s,
                Coord viewTop, Coord viewBottom) {
    const auto at = [&](Position q) {
        return QPointF(static_cast<double>(q.x) * s.pointsPerCoord,
                       s.originY + static_cast<double>(q.y - s.top) * s.pointsPerCoord);
    };
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);
    for (const Mark& m : marks) {
        const Coord h = halfOf(d, m);
        if (const auto* dot = std::get_if<Dot>(&m)) {
            if (dot->at.y + h < viewTop || dot->at.y - h > viewBottom) continue;
            const int grey = d.darknessGreys.empty()
                                 ? 0
                                 : d.darknessGreys[std::min<std::size_t>(dot->darkness, d.darknessGreys.size() - 1)];
            const double r = std::max(0.5, static_cast<double>(d.dotDiameter) / 2.0 * s.pointsPerCoord);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(grey, grey, grey));
            p.drawEllipse(at(dot->at), r, r);
            continue;
        }
        const Stroke& st = std::get<Stroke>(m);
        if (st.path.empty()) continue;
        Coord lo = st.path.front().y, hi = lo;
        for (const Position& q : st.path) {
            lo = std::min(lo, q.y);
            hi = std::max(hi, q.y);
        }
        if (hi + h < viewTop || lo - h > viewBottom) continue;
        // D-12: the scaled width, at least one point.
        QPen pen(QColor(st.colour.r, st.colour.g, st.colour.b));
        pen.setWidthF(std::max(1.0, static_cast<double>(d.strokeWidth) * s.pointsPerCoord));
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        if (st.path.size() == 1) {
            const QPointF c = at(st.path.front());
            p.drawLine(c, c + QPointF(0.01, 0));   // one position: a round dot of the stroke width
            continue;
        }
        QPainterPath path(at(st.path.front()));
        for (std::size_t i = 1; i < st.path.size(); ++i) path.lineTo(at(st.path[i]));
        p.drawPath(path);
    }
    p.restore();
}

} // namespace calcu::qt6
