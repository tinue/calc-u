#include "paper/Paper.hpp"
#include <algorithm>
#include <utility>

namespace calcu::paper {

Paper::Paper(PaperDescription d) : description_(std::move(d)) {}

void Paper::release() {
    closeStroke();
    penColour_.reset();
    penAt_.reset();
    bound_ = nullptr;
}

void Paper::bind(PaperOutput* printer) {
    if (printer == bound_) return;
    if (bound_) bound_->connect(nullptr);
    release();
    bound_ = printer;
    if (!bound_) return;
    offset_ = standing_ - bound_->connectStanding();
    bound_->connect(this);   // what it kept arrives now
}

void Paper::cut() {
    closeStroke();   // the pen stays down: the next move starts a stroke on the new paper (D-5)
    marks_.clear();
    text_.clear();
    fed_ = false;
    reach_ = {};
    standing_ = 0;
    offset_ = bound_ ? -bound_->standing() : 0;
    emptiedSinceTake_ = true;
    sentMarks_ = sentLines_ = sentLastPath_ = 0;
}

bool Paper::empty() const { return marks_.empty() && text_.empty() && !fed_; }

Reach Paper::reach() const { return fed_ ? reach_ : Reach{}; }

// AC-4: from the least to the greatest y that a mark, a text line or a feed
// has since the paper was last empty.
void Paper::touch(Coord y) {
    if (!fed_) {
        fed_ = true;
        reach_ = Reach{y, y};
        return;
    }
    reach_.from = std::min(reach_.from, y);
    reach_.to = std::max(reach_.to, y);
}

void Paper::closeStroke() { strokeOpen_ = false; }

void Paper::dot(Position p, std::uint8_t darkness) {
    const Position at = moved(p);
    marks_.push_back(Dot{at, darkness});
    touch(at.y);
}

void Paper::penDown(Rgb colour, Position p) {
    closeStroke();
    penColour_ = colour;
    penAt_ = p;
    const Position at = moved(p);
    marks_.push_back(Stroke{colour, {at}});
    strokeOpen_ = true;
    touch(at.y);
}

void Paper::penTo(Position p) {
    if (!penColour_) {   // a move with the pen up gives no mark (AC-33)
        penAt_ = p;
        return;
    }
    const Position at = moved(p);
    if (!strokeOpen_) {   // the pen went on after a cut (D-5)
        const Position from = moved(penAt_ ? *penAt_ : p);
        marks_.push_back(Stroke{*penColour_, {from}});
        strokeOpen_ = true;
        touch(from.y);
    }
    std::get<Stroke>(marks_.back()).path.push_back(at);
    touch(at.y);
    penAt_ = p;
}

void Paper::penUp() {
    closeStroke();
    penColour_.reset();
}

void Paper::textLine(std::string text, Position p) {
    const Position at = moved(p);
    text_.push_back(TextLine{std::move(text), at});
    touch(at.y);
}

void Paper::fedTo(Coord y) {
    standing_ = y + offset_;
    touch(standing_);
}

std::optional<Paper::Delta> Paper::takeDelta() {
    Delta d;
    d.emptied = emptiedSinceTake_;
    d.reach = reach();
    if (sentMarks_ > 0 && sentMarks_ <= marks_.size()) {
        if (const auto* s = std::get_if<Stroke>(&marks_[sentMarks_ - 1]); s && s->path.size() > sentLastPath_) {
            d.continuesStroke = true;
            d.marks.push_back(Stroke{s->colour, std::vector<Position>(s->path.begin() + std::ptrdiff_t(sentLastPath_),
                                                                      s->path.end())});
        }
    }
    d.marks.insert(d.marks.end(), marks_.begin() + std::ptrdiff_t(sentMarks_), marks_.end());
    d.lines.assign(text_.begin() + std::ptrdiff_t(sentLines_), text_.end());
    const bool nothing = !d.emptied && d.marks.empty() && d.lines.empty() && d.reach == sentReach_ &&
                         empty() == sentEmpty_;
    emptiedSinceTake_ = false;
    sentMarks_ = marks_.size();
    sentLines_ = text_.size();
    const auto* last = marks_.empty() ? nullptr : std::get_if<Stroke>(&marks_.back());
    sentLastPath_ = last ? last->path.size() : 0;
    sentReach_ = d.reach;
    sentEmpty_ = empty();
    if (nothing) return std::nullopt;
    return d;
}

} // namespace calcu::paper
