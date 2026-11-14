#include "sessions/LibraryTopics.hpp"
#include <variant>

namespace calcu::sessions {

using machine::DisplayOutput;

bool sameDisplay(const DisplayOutput& a, const DisplayOutput& b) {
    if (a.on != b.on || a.frame.index() != b.frame.index() || a.indicators.size() != b.indicators.size()) return false;
    for (std::size_t i = 0; i < a.indicators.size(); ++i) {
        const auto &x = a.indicators[i], &y = b.indicators[i];
        if (x.name != y.name || x.on != y.on || x.brightness != y.brightness) return false;
    }
    if (const auto* sa = std::get_if<machine::SegmentFrame>(&a.frame)) {
        const auto& sb = std::get<machine::SegmentFrame>(b.frame);
        if (sa->positions.size() != sb.positions.size()) return false;
        for (std::size_t i = 0; i < sa->positions.size(); ++i) {
            const auto &p = sa->positions[i], &q = sb.positions[i];
            if (p.lit != q.lit || p.brightness != q.brightness || p.segmentBrightness != q.segmentBrightness)
                return false;
        }
        return true;
    }
    const auto& da = std::get<machine::DotMatrixFrame>(a.frame);
    const auto& db = std::get<machine::DotMatrixFrame>(b.frame);
    return da.width == db.width && da.height == db.height && da.dots == db.dots;
}

namespace {
Value brightness(const std::optional<float>& b) { return b ? Value(static_cast<double>(*b)) : Value(); }

Value placePath(const machine::PlacePath& p) {
    Value::Array steps;
    for (const auto& s : p.steps) steps.emplace_back(s.value);
    return Value(std::move(steps));
}

Value plugEntry(const machine::PlugEntry& e) {
    return Value::object({{"place", placePath(e.place)},
                          {"peripheral", e.peripheral.value},
                          {"variant", e.variant ? Value(e.variant->value) : Value()}});
}
} // namespace

Value DisplayState::toValue() const {
    Value frame;
    if (const auto* s = std::get_if<machine::SegmentFrame>(&output.frame)) {
        Value::Array positions;
        for (const auto& p : s->positions) {
            Value::Array lit, own;
            for (bool l : p.lit) lit.emplace_back(l);
            for (const auto& b : p.segmentBrightness) own.push_back(brightness(b));
            positions.push_back(Value::object(
                {{"lit", Value(std::move(lit))}, {"brightness", brightness(p.brightness)}, {"segmentBrightness", Value(std::move(own))}}));
        }
        frame = Value::object({{"kind", "segments"}, {"positions", Value(std::move(positions))}});
    } else {
        const auto& d = std::get<machine::DotMatrixFrame>(output.frame);
        frame = Value::object({{"kind", "dots"}, {"width", d.width}, {"height", d.height}, {"dots", Value(d.dots)}});
    }
    Value::Array indicators;
    for (const auto& i : output.indicators)
        indicators.push_back(
            Value::object({{"name", std::string(i.name)}, {"on", i.on}, {"brightness", brightness(i.brightness)}}));
    return Value::object({{"on", output.on}, {"frame", std::move(frame)}, {"indicators", Value(std::move(indicators))}});
}

Value BusyState::toValue() const {
    return Value::object({{"operationRuns", operationRuns}, {"speed", speed == pacing::Speed::Full ? "full" : "regular"}});
}

Value Notice::toValue() const { return Value::object({{"sentence", sentence}}); }

Value LogLine::toValue() const { return Value::object({{"text", text}}); }

Value EmulatedTimeReply::toValue() const { return Value::object({{"ticks", ticks}, {"seconds", seconds}}); }

Value MachineOffer::toValue() const {
    Value::Array caps;
    for (const auto& c : capabilities) {
        Value owner;
        if (const auto* co = std::get_if<machine::ComponentOwner>(&c.owner))
            owner = Value::object({{"component", co->component.value}});
        else {
            const auto& po = std::get<machine::PeripheralOwner>(c.owner);
            owner = Value::object({{"peripheral", po.peripheral.value}, {"place", placePath(po.place)}});
        }
        caps.push_back(Value::object({{"kind", std::string(c.kind.value)}, {"id", c.id.value}, {"owner", std::move(owner)}}));
    }
    Value::Array byPlace, byConnector, placeNames;
    for (const auto& e : plugged.byPlace) byPlace.push_back(plugEntry(e));
    for (const auto& c : plugged.byConnector) {
        Value::Array entries;
        for (const auto& e : c.plugged) entries.push_back(plugEntry(e));
        byConnector.push_back(Value::object(
            {{"owner", placePath(c.owner)}, {"connector", c.connector.value}, {"plugged", Value(std::move(entries))}}));
    }
    for (const auto& p : places) placeNames.emplace_back(p.value);
    return Value::object({{"capabilities", Value(std::move(caps))},
                          {"plugged", Value::object({{"byPlace", Value(std::move(byPlace))},
                                                     {"byConnector", Value(std::move(byConnector))}})},
                          {"places", Value(std::move(placeNames))}});
}

Value TopicList::toValue() const {
    Value::Array out;
    for (const auto& t : topics) out.push_back(Value::object({{"name", t.name.value}, {"stream", t.stream}}));
    return Value::object({{"topics", Value(std::move(out))}});
}

} // namespace calcu::sessions
