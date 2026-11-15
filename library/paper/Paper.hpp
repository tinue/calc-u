#pragma once
#include "paper/PaperOutput.hpp"
#include <optional>
#include <vector>

// One printer type's paper (LIB-8 design §3.2; spec AC-2 to AC-5, AC-8,
// EC-1): the marks and text lines in the order made, how far the paper
// reaches, the cut, and the binding to the printer that prints on it.
// A position on the paper counts y from where the paper stood when it was
// last empty; the printer's own coordinates are moved by the offset taken at
// bind and at cut. No limit on the length (AC-4).

namespace calcu::paper {

class Paper final : public PaperSink {
public:
    explicit Paper(PaperDescription);

    // Unbinds the printer bound before (its open stroke closed, its
    // capability told to keep its calls again) and binds `printer`: from now
    // on its marks land here, moved so that it prints on from where this
    // paper stands (AC-8). nullptr: unbound.
    void bind(PaperOutput*);
    // Forgets the bound printer without calling it: its capability is gone
    // (a detach, a new machine). Its open stroke is closed.
    void release();
    PaperOutput* bound() const { return bound_; }
    // AC-5, EC-1: empty; y counts again from where the paper stands now. A
    // stroke open at the cut is closed here and goes on, on the new paper,
    // with the pen's next move (D-5).
    void cut();

    bool empty() const;                       // no marks, no text, nothing fed
    Reach reach() const;                      // AC-4; 0..0 while empty
    const std::vector<Mark>& marks() const { return marks_; }
    const std::vector<TextLine>& text() const { return text_; }
    const PaperDescription& description() const { return description_; }

    // What changed since the last take (the topic, AC-6): emptied (a cut in
    // between: what follows is the new paper's), whether the first mark
    // continues the stroke that the last take ended with (its positions are
    // appended to it), the marks and lines added, the reach now.
    struct Delta {
        bool emptied = false;
        bool continuesStroke = false;
        std::vector<Mark> marks;
        std::vector<TextLine> lines;
        Reach reach;
    };
    std::optional<Delta> takeDelta();

    // PaperSink: in the bound printer's coordinates.
    void dot(Position, std::uint8_t darkness) override;
    void penDown(Rgb, Position) override;
    void penTo(Position) override;
    void penUp() override;
    void textLine(std::string text, Position) override;
    void fedTo(Coord y) override;

private:
    Position moved(Position p) const { return Position{p.x, p.y + offset_}; }
    void touch(Coord y);
    void closeStroke();

    PaperDescription description_;
    PaperOutput* bound_ = nullptr;
    Coord offset_ = 0;      // paper y = printer y + offset
    Coord standing_ = 0;    // where the paper stands, in paper y
    std::vector<Mark> marks_;
    std::vector<TextLine> text_;
    bool fed_ = false;      // something was put down or fed since the last cut
    Reach reach_;
    bool strokeOpen_ = false;
    std::optional<Rgb> penColour_;   // the pen is down (also across a cut)
    std::optional<Position> penAt_;  // in printer coordinates
    // For takeDelta.
    bool emptiedSinceTake_ = false;
    std::size_t sentMarks_ = 0, sentLines_ = 0, sentLastPath_ = 0;
    Reach sentReach_;
    bool sentEmpty_ = true;
};

} // namespace calcu::paper
