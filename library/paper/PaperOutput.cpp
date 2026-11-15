#include "paper/PaperOutput.hpp"
#include <utility>

namespace calcu::paper {

// A printer calls emitFedTo before standing() moves to the new place, so the
// first kept call records where the paper stood when it was made.
void PaperOutputBase::keep(Call c) {
    if (kept_.empty()) keptFrom_ = standing();
    kept_.push_back(std::move(c));
}

void PaperOutputBase::connect(PaperSink* sink) {
    sink_ = sink;
    if (!sink_) return;
    std::vector<Call> calls;
    calls.swap(kept_);
    for (auto& c : calls) {
        switch (c.op) {
        case Call::Op::Dot: sink_->dot(c.at, c.darkness); break;
        case Call::Op::PenDown: sink_->penDown(c.colour, c.at); break;
        case Call::Op::PenTo: sink_->penTo(c.at); break;
        case Call::Op::PenUp: sink_->penUp(); break;
        case Call::Op::Text: sink_->textLine(std::move(c.text), c.at); break;
        case Call::Op::Fed: sink_->fedTo(c.y); break;
        }
    }
}

void PaperOutputBase::emitDot(Position p, std::uint8_t d) {
    if (sink_) sink_->dot(p, d);
    else keep(Call{Call::Op::Dot, p, d, {}, {}, 0});
}
void PaperOutputBase::emitPenDown(Rgb c, Position p) {
    if (sink_) sink_->penDown(c, p);
    else keep(Call{Call::Op::PenDown, p, 0, c, {}, 0});
}
void PaperOutputBase::emitPenTo(Position p) {
    if (sink_) sink_->penTo(p);
    else keep(Call{Call::Op::PenTo, p, 0, {}, {}, 0});
}
void PaperOutputBase::emitPenUp() {
    if (sink_) sink_->penUp();
    else keep(Call{Call::Op::PenUp, {}, 0, {}, {}, 0});
}
void PaperOutputBase::emitTextLine(std::string t, Position p) {
    if (sink_) sink_->textLine(std::move(t), p);
    else keep(Call{Call::Op::Text, p, 0, {}, std::move(t), 0});
}
void PaperOutputBase::emitFedTo(Coord y) {
    if (sink_) sink_->fedTo(y);
    else keep(Call{Call::Op::Fed, {}, 0, {}, {}, y});
}

} // namespace calcu::paper
