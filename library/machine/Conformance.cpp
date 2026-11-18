#include "machine/Conformance.hpp"
#include <algorithm>
#include <variant>

namespace calcu::machine {

namespace {
std::string q(const std::string& s) { return "\"" + s + "\""; }

void checkDisplay(Machine& m, const MachineDescriptor& d, std::vector<std::string>& out) {
    const DisplayOutput o = m.readDisplay();
    if (const auto* g = std::get_if<SegmentGeometry>(&d.display.geometry)) {
        const auto* f = std::get_if<SegmentFrame>(&o.frame);
        if (!f) {
            out.push_back("display: a segment display by the descriptor, another kind at run time");
        } else {
            if (static_cast<int>(f->positions.size()) != g->positions)
                out.push_back("display: " + std::to_string(f->positions.size()) + " positions, the descriptor states " +
                              std::to_string(g->positions));
            for (const auto& p : f->positions)
                if (p.lit.size() != g->layout.segments.size()) {
                    out.push_back("display: a position with " + std::to_string(p.lit.size()) +
                                  " segments, the layout has " + std::to_string(g->layout.segments.size()));
                    break;
                }
        }
    } else {
        const auto& dm = std::get<DotMatrixGeometry>(d.display.geometry);
        const auto* f = std::get_if<DotMatrixFrame>(&o.frame);
        if (!f) {
            out.push_back("display: a dot-matrix display by the descriptor, another kind at run time");
        } else if (f->width != dm.width || f->height != dm.height ||
                   f->dots.size() != static_cast<std::size_t>(dm.width) * static_cast<std::size_t>(dm.height)) {
            out.push_back("display: " + std::to_string(f->width) + " x " + std::to_string(f->height) +
                          " dots, the descriptor states " + std::to_string(dm.width) + " x " + std::to_string(dm.height));
        }
    }
    bool sameNames = o.indicators.size() == d.display.indicators.size();
    for (std::size_t i = 0; sameNames && i < o.indicators.size(); ++i)
        sameNames = o.indicators[i].name == d.display.indicators[i].name;
    if (!sameNames) out.push_back("display: the indicators differ from the descriptor's");
}

void checkKeys(Machine& m, const MachineDescriptor& d, std::vector<std::string>& out) {
    for (const auto& k : d.keys) {
        if (m.keyDown(k.id) != KeyCall::Done) out.push_back("keys: key " + q(k.id.value) + " is refused");
        m.keyUp(k.id);
    }
    const KeyId madeUp{"calcu.conformance.no-such-key"};
    if (m.keyDown(madeUp) != KeyCall::UnknownKey) {
        out.push_back("keys: a key that is not in the descriptor is taken");
        m.keyUp(madeUp);
    }
}

void checkPlugged(Machine& m, const Catalogue& cat, const MachineDescriptor& d, std::vector<std::string>& out) {
    for (const auto& e : m.plugged().byPlace) {
        if (e.place.steps.size() != 1) continue;   // a place on a peripheral
        const PlaceName& place = e.place.steps[0];
        const auto info = cat.place(d.model, place);
        if (!info) {
            out.push_back("plugged: place " + q(place.value) + " is not in the descriptor");
        } else if (std::find(info->fitting.begin(), info->fitting.end(), e.peripheral) == info->fitting.end()) {
            out.push_back("plugged: " + q(e.peripheral.value) + " does not fit place " + q(place.value));
        }
    }
}
} // namespace

std::vector<std::string> checkAgainstDescriptor(Machine& m, const Catalogue& cat) {
    std::vector<std::string> out;
    const MachineDescriptor* d = cat.model(m.model());
    if (!d) return {"model: " + q(m.model().value) + " is not in the catalogue"};
    if (m.descriptor().tickRate != d->tickRate) out.push_back("tick rate: differs from the catalogue's");
    const auto v = m.variant();
    if (d->variants.empty() ? v.has_value()
                            : !v || std::none_of(d->variants.begin(), d->variants.end(),
                                                 [&](const VariantDescriptor& x) { return x.id == *v; }))
        out.push_back("variant: " + (v ? q(v->value) : std::string("none")) + " is not one of the descriptor's");
    checkDisplay(m, *d, out);
    checkKeys(m, *d, out);
    checkPlugged(m, cat, *d, out);
    if (m.debugTargets() != d->debugTargets) out.push_back("debug targets: differ from the descriptor's");
    return out;
}

namespace {
bool endsWith(const std::string& s, const std::string& tail) {
    return s.size() >= tail.size() && s.compare(s.size() - tail.size(), tail.size(), tail) == 0;
}

void checkInfo(const TargetInfo& info, std::vector<std::string>& out) {
    const std::string t = "debug target " + q(info.name.value) + ": ";
    for (const auto& r : info.registers)
        if (r.name.empty() || r.bits < 1 || r.bits > 64) out.push_back(t + "register " + q(r.name) + " has " + std::to_string(r.bits) + " bits");
    if (info.spaces.empty()) out.push_back(t + "no address space");
    else if (info.codeSpace >= info.spaces.size()) out.push_back(t + "the code space is not one of its spaces");
    for (const auto& sp : info.spaces) {
        if (sp.name.empty() || sp.cellBits < 1 || sp.cellBits > 64 || sp.size == 0)
            out.push_back(t + "space " + q(sp.name) + " is not well formed");
        if (sp.writtenView) {
            const bool named = endsWith(sp.name, "-written") &&
                               std::any_of(info.spaces.begin(), info.spaces.end(), [&](const SpaceInfo& o) {
                                   return !o.writtenView && o.name + "-written" == sp.name;
                               });
            if (!named) out.push_back(t + "the view " + q(sp.name) + " is not named after a space");
        }
        for (const auto& w : sp.banked)
            if (w.count == 0 || w.first + w.count > sp.size || w.banks == 0)
                out.push_back(t + "a bank window of " + q(sp.name) + " lies outside it");
    }
    for (const auto& f : info.offers.qualifier)
        if (f.name.empty() || f.values.empty()) out.push_back(t + "qualifier field " + q(f.name) + " has no values");
}
} // namespace

std::vector<std::string> checkDebugging(Machine& m) {
    std::vector<std::string> out;
    const std::vector<DebugTargetId> listed = m.debugTargets();
    Debugging* d = m.debugging();
    if (!d) {
        if (!listed.empty()) out.push_back("debug targets: listed, but the machine offers none");
        return out;
    }
    if (d->targetCount() != listed.size()) {
        out.push_back("debug targets: " + std::to_string(d->targetCount()) + " offered, " +
                      std::to_string(listed.size()) + " listed");
        return out;
    }
    for (std::size_t i = 0; i < listed.size(); ++i) {
        const TargetInfo& info = d->target(i).info();
        if (info.name != listed[i]) out.push_back("debug targets: target " + std::to_string(i) + " is named " + q(info.name.value));
        checkInfo(info, out);
    }
    // Reading has no effect: the tick count, every register and
    // every first and last cell stay as they were.
    const Ticks ticks = m.tickCount();
    auto snapshot = [&] {
        std::vector<std::uint64_t> v;
        for (std::size_t i = 0; i < d->targetCount(); ++i) {
            const DebugTarget& t = d->target(i);
            for (std::size_t r = 0; r < t.info().registers.size(); ++r) v.push_back(t.readRegister(r));
            for (std::size_t s = 0; s < t.info().spaces.size(); ++s) {
                const std::uint64_t size = t.info().spaces[s].size;
                for (std::uint64_t a : {std::uint64_t{0}, size ? size - 1 : 0}) {
                    const CellRead c = t.peek(s, a, {});
                    v.push_back(static_cast<std::uint64_t>(c.kind));
                    v.push_back(c.value);
                }
            }
        }
        return v;
    };
    const auto first = snapshot();
    const auto second = snapshot();
    if (first != second) out.push_back("debug targets: reading registers and cells changed them");
    if (m.tickCount() != ticks) out.push_back("debug targets: reading changed the tick count");
    return out;
}

} // namespace calcu::machine
