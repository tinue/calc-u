#include "paper/PaperTopics.hpp"
#include "paper/Replies.hpp"

namespace calcu::paper {

using sessions::Value;

std::string noTextLayerSentence(const std::string& owner) { return "The " + owner + " has no text layer."; }

std::string unknownPrinterSentence(const std::string& name, const std::vector<std::string>& owners) {
    std::string list;
    for (const auto& o : owners) list += (list.empty() ? "" : ", ") + o;
    return "Unknown printer: " + name + ". Printers: " + list + ".";
}

Value descriptionValue(const PaperDescription& d) {
    Value::Array greys;
    for (auto g : d.darknessGreys) greys.push_back(Value(int(g)));
    return Value::object({{"unit", Value(Value::Array{Value(d.unit.numerator), Value(d.unit.denominator)})},
                          {"width", d.width},
                          {"printableLeft", d.printableLeft},
                          {"printableWidth", d.printableWidth},
                          {"form", d.form == MarkForm::Dots ? "dots" : "pen-strokes"},
                          {"dotDiameter", d.dotDiameter},
                          {"darknessGreys", Value(std::move(greys))},
                          {"strokeWidth", d.strokeWidth},
                          {"pictureDotsPerInch", d.pictureDotsPerInch},
                          {"textLayer", d.textLayer}});
}

Value PaperList::toValue() const {
    Value::Array a;
    for (const auto& p : papers)
        a.push_back(Value::object(
            {{"printer", p.printer.value}, {"owner", p.owner}, {"description", descriptionValue(p.description)}}));
    return Value(std::move(a));
}

Value PaperAppended::toValue() const {
    Value::Array marksOut, dots;
    auto flush = [&] {
        if (!dots.empty()) marksOut.push_back(Value(std::move(dots)));
        dots.clear();
    };
    for (const Mark& m : marks) {
        if (const auto* d = std::get_if<Dot>(&m)) {
            dots.push_back(Value(d->at.x));
            dots.push_back(Value(d->at.y));
            dots.push_back(Value(int(d->darkness)));
            continue;
        }
        flush();
        const Stroke& s = std::get<Stroke>(m);
        Value::Array path;
        path.reserve(s.path.size() * 2);
        for (const Position& p : s.path) {
            path.push_back(Value(p.x));
            path.push_back(Value(p.y));
        }
        marksOut.push_back(Value::object({{"colour", colourText(s.colour)}, {"path", Value(std::move(path))}}));
    }
    flush();
    Value::Array linesOut;
    for (const auto& l : lines) linesOut.push_back(Value::object({{"text", l.text}, {"x", l.at.x}, {"y", l.at.y}}));
    return Value::object({{"continuesStroke", continuesStroke},
                          {"marks", Value(std::move(marksOut))},
                          {"lines", Value(std::move(linesOut))},
                          {"reach", Value(Value::Array{Value(reach.from), Value(reach.to)})}});
}

Value CopyOutcome::toValue() const {
    const char* o = outcome == Kind::Copied ? "copied" : outcome == Kind::TextOnly ? "text-only" : "failed";
    return Value::object({{"owner", owner}, {"outcome", o}, {"reason", reason}});
}

sessions::PayloadPtr linesPayload(const std::vector<std::string>& lines) {
    Value::Array a;
    for (const auto& l : lines) a.push_back(Value(l));
    return sessions::valuePayload("paper-lines", Value::object({{"lines", Value(std::move(a))}}));
}

} // namespace calcu::paper
