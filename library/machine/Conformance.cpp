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
        if (e.place.steps.size() != 1) continue;   // a place on a peripheral: LIB-2
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

} // namespace calcu::machine
