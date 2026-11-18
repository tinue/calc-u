#pragma once
#include "machine/Capability.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

// The generic kind paper output: what a printer or plotter puts on paper, as marks
// (dots, or pen strokes with a colour) and an optional text layer. The
// machine draws nothing; the paper itself lives in the session (Paper.hpp,
// PaperPart.hpp), because it outlives every machine.

namespace calcu::paper {

// Ten-thousandths of the printer's unit: no floating point in a mark,
// so two runs and every platform give the same marks and replies.
using Coord = std::int64_t;
inline constexpr Coord kPerUnit = 10'000;
constexpr Coord units(std::int64_t whole) { return whole * kPerUnit; }

struct Position {
    Coord x = 0, y = 0;
    bool operator==(const Position&) const = default;
};
struct Rgb {
    std::uint8_t r = 0, g = 0, b = 0;
    bool operator==(const Rgb&) const = default;
};
// The unit in millimetres, exact: 430/1000 (PC-100C), 190/1920 (plotters).
struct Millimetres {
    std::int64_t numerator = 1, denominator = 1;
    double value() const { return static_cast<double>(numerator) / static_cast<double>(denominator); }
    bool operator==(const Millimetres&) const = default;
};

enum class MarkForm { Dots, PenStrokes };

struct PaperDescription {
    Millimetres unit;
    Coord width = 0;                             // the paper, from its left edge
    Coord printableLeft = 0, printableWidth = 0; // the part that can be printed on
    MarkForm form = MarkForm::Dots;
    Coord dotDiameter = 0;                       // Dots
    std::vector<std::uint8_t> darknessGreys;     // Dots: the grey of darkness d (0 black … 255 white)
    Coord strokeWidth = 0;                       // PenStrokes, in the picture
    int pictureDotsPerInch = 0;                  // the resolution of Copy's picture
    bool textLayer = false;
    bool operator==(const PaperDescription&) const = default;
};

struct Dot {
    Position at;
    std::uint8_t darkness = 0;
    bool operator==(const Dot&) const = default;
};
struct Stroke {
    Rgb colour;
    std::vector<Position> path;
    bool operator==(const Stroke&) const = default;
};
using Mark = std::variant<Dot, Stroke>;
struct TextLine {
    std::string text;   // UTF-8; "" for a feed
    Position at;
    bool operator==(const TextLine&) const = default;
};
struct Reach {
    Coord from = 0, to = 0;
    bool operator==(const Reach&) const = default;
};

// What a printer calls as it prints, on the session's thread, inside
// Machine::advance. Positions are in the printer's own
// coordinates: y = 0 where the paper stood when the printer was plugged in.
class PaperSink {
public:
    virtual void dot(Position, std::uint8_t darkness) = 0;
    virtual void penDown(Rgb, Position) = 0;
    virtual void penTo(Position) = 0;
    virtual void penUp() = 0;
    virtual void textLine(std::string text, Position) = 0;
    virtual void fedTo(Coord y) = 0;   // the paper stands at y now (feeds back too)

protected:
    ~PaperSink() = default;
};

class PaperOutput : public machine::Capability {
public:
    static constexpr machine::KindId kKind{"paper-output"};
    machine::KindId kind() const override { return kKind; }
    virtual const PaperDescription& description() const = 0;
    virtual Coord standing() const = 0;   // where the paper stands now (printer coordinates)
    // Where the paper stood when the first call not yet handed to a sink was
    // made; standing() when none is kept. A sink that binds takes its
    // offset from it, so kept marks land where they were made.
    virtual Coord connectStanding() const { return standing(); }
    // Hands over what was kept while unconnected; nullptr: keep again.
    virtual void connect(PaperSink*) = 0;
    virtual const PaperSink* connected() const = 0;
};

// For cores: forwards to the sink, or keeps the calls while none is
// connected and replays them at connect. A core's printer derives
// from it and calls emitDot, emitPenDown, … as it prints.
class PaperOutputBase : public PaperOutput {
public:
    void connect(PaperSink*) final;
    const PaperSink* connected() const final { return sink_; }
    Coord connectStanding() const final { return kept_.empty() ? standing() : keptFrom_; }

protected:
    void emitDot(Position, std::uint8_t darkness);
    void emitPenDown(Rgb, Position);
    void emitPenTo(Position);
    void emitPenUp();
    void emitTextLine(std::string, Position);
    void emitFedTo(Coord);

private:
    struct Call {
        enum class Op { Dot, PenDown, PenTo, PenUp, Text, Fed } op;
        Position at;
        std::uint8_t darkness = 0;
        Rgb colour;
        std::string text;
        Coord y = 0;
    };
    void keep(Call);
    PaperSink* sink_ = nullptr;
    std::vector<Call> kept_;
    Coord keptFrom_ = 0;
};

} // namespace calcu::paper
