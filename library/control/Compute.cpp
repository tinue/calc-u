#include "control/Compute.hpp"
#include "machine/Attach.hpp"
#include "sessions/LibraryTopics.hpp"
#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// LIB-4 design §3.4. Every list in the order of the descriptors; every
// availability by the order of AC-2 (Compute.hpp).

namespace calcu::control {

using machine::PeripheralId;
using machine::PlacePath;
using machine::VariantId;

namespace {

class Computation {
public:
    Computation(const machine::Machine& m, const ControlSetup& s, bool runs)
        : m_(m), setup_(s), cat_(*s.catalogue), runs_(runs), plugged_(m.plugged()) {}

    ControlState run() {
        ControlState out;
        models(out);
        variants(out);
        for (const auto& p : m_.descriptor().places) placeWalk(PlacePath{{p.name}}, nullptr, out);
        mediaPlaces(out);
        panels(out);
        actions(out);
        for (const ControlRules* r : setup_.rules)
            if (r) r->productEntries(m_, out.productEntries);
        return out;
    }

private:
    // Rules 1 and 2 of AC-2; `inherited` is the reason of a refused place
    // (D-5). std::nullopt: neither applies.
    std::optional<Availability> before(const EntryKey& key, bool endsOperation,
                                       const std::optional<std::string>& inherited = std::nullopt) const {
        if (runs_ && !endsOperation) return Availability{false, sessions::kAnotherOperationRuns};
        if (inherited) return Availability{false, *inherited};
        if (auto why = productSays(key)) return Availability{false, std::move(*why)};
        return std::nullopt;
    }
    Availability ruled(const EntryKey& key, bool endsOperation = false,
                       const std::optional<std::string>& inherited = std::nullopt) const {
        return before(key, endsOperation, inherited).value_or(Availability{});
    }
    std::optional<std::string> productSays(const EntryKey& key) const {
        for (const ControlRules* r : setup_.rules)
            if (r)
                if (auto why = r->unavailable(key, m_)) return why;
        return std::nullopt;
    }

    void models(ControlState& out) const {
        for (const auto& d : cat_.models()) out.models.push_back(ModelEntry{d.model, ruled(ModelKey{d.model}, true)});
        out.model = m_.model();
    }

    void variants(ControlState& out) const {
        const machine::MachineDescriptor& d = m_.descriptor();
        for (const auto& v : d.variants) out.variants.push_back(VariantChoice{v.id, ruled(VariantKey{v.id})});
        if (d.variants.empty()) return;
        out.variant = m_.variant() ? m_.variant() : d.defaultVariant;
    }

    std::vector<const machine::PlugEntry*> at(const PlacePath& path) const {
        std::vector<const machine::PlugEntry*> out;
        for (const auto& e : plugged_.byPlace)
            if (e.place == path) out.push_back(&e);
        return out;
    }

    // The request the control state asks with (§3.2): the place, the
    // peripheral, the variant, and ROM images that carry the file names of
    // the variant's entries and no bytes (D-2).
    machine::PeripheralRequest request(const PlacePath& path, const machine::PeripheralDescriptor& d,
                                       const std::optional<VariantId>& variant) const {
        machine::PeripheralRequest r;
        r.place = path;
        r.peripheral = d.id;
        r.variant = variant;
        for (const auto& e : d.romsFor(variant)) r.roms.push_back(machine::RomImage{e.fileName, {}});
        return r;
    }

    // Rule 3: what the machine says of plugging `d` in `variant` at `path`.
    Availability machineSays(const PlacePath& path, const machine::PlaceDescriptor& place,
                             const machine::PeripheralDescriptor& d, const std::optional<VariantId>& variant,
                             bool isPlugged) const {
        const machine::PeripheralRequest r = request(path, d, variant);
        const auto held = at(path);
        std::optional<machine::Refusal> no;
        if (isPlugged) no = m_.canReplace(d.id, r, cat_);   // another variant of what is plugged in
        else if (!held.empty() && static_cast<int>(held.size()) >= place.takesAtOnce)
            no = m_.canReplace(held.front()->peripheral, r, cat_);   // a full place: a replace
        else no = m_.canAttach(r, cat_);
        return no ? Availability{false, no->reason} : Availability{};
    }

    PeripheralChoice choiceFor(const PlacePath& path, const machine::PlaceDescriptor& place,
                               const machine::PeripheralDescriptor& d,
                               const std::optional<std::string>& placeReason) const {
        PeripheralChoice c;
        c.id = d.id;
        c.name = d.name.empty() ? d.id.value : d.name;
        const machine::PlugEntry* in = nullptr;
        for (const machine::PlugEntry* e : at(path))
            if (e->peripheral == d.id) in = e;
        c.plugged = in != nullptr;
        const std::optional<VariantId> running =
            d.variants.empty() ? std::nullopt : (in && in->variant ? in->variant : d.defaultVariant);
        c.variant = running;
        for (const auto& v : d.variants) {
            VariantChoice vc{v.id, {}};
            if (auto a = before(ChoiceKey{path, d.id, v.id}, false, placeReason)) vc.available = *a;
            else if (!(c.plugged && running == v.id)) vc.available = machineSays(path, place, d, v.id, c.plugged);
            c.variants.push_back(std::move(vc));   // the choice in force follows rules 1 and 2 only (D-7)
        }
        if (auto a = before(ChoiceKey{path, d.id, std::nullopt}, false, placeReason)) c.available = *a;
        else if (!c.plugged) c.available = machineSays(path, place, d, running, false);   // a detach: never refused
        return c;
    }

    void placeWalk(const PlacePath& path, const machine::PeripheralId* host, ControlState& out) {
        const machine::PlaceName& name = path.steps.back();
        const auto info = host ? cat_.place(*host, name) : cat_.place(m_.model(), name);
        if (!info || !info->place) return;   // a place nobody has (AC-5)
        PlaceEntry e;
        e.path = path;
        e.connector = info->place->connector;
        e.pinOut = info->place->pinOut;
        std::optional<std::string> placeReason = productSays(PlaceKey{path});
        if (placeReason) e.available = Availability{false, *placeReason};
        e.empty = ruled(ChoiceKey{path, std::nullopt, std::nullopt}, false, placeReason);
        for (const PeripheralId& id : info->fitting)
            if (const machine::PeripheralDescriptor* d = cat_.peripheral(id))
                e.fitting.push_back(choiceFor(path, *info->place, *d, placeReason));
        out.places.push_back(std::move(e));
        // The places of what is plugged in here follow at once, depth first (LIB-2 AC-9).
        for (const machine::PlugEntry* p : at(path)) {
            walked_.push_back(p);
            if (const machine::PeripheralDescriptor* d = cat_.peripheral(p->peripheral))
                for (const auto& sub : d->places) {
                    PlacePath next = path;
                    next.steps.push_back(sub.name);
                    placeWalk(next, &p->peripheral, out);
                }
        }
    }

    void mediaPlaces(ControlState& out) const {
        auto add = [&](const machine::MediaPlaceDescriptor& d, const PlacePath& ownerPlace,
                       std::optional<PeripheralId> owner) {
            MediaPlaceEntry e{d.id, ownerPlace, std::move(owner), d.mediumKind, d.passage, d.sides, {}};
            e.insert = ruled(InsertKey{d.id, ownerPlace});
            out.mediaPlaces.push_back(std::move(e));
        };
        for (const auto& d : m_.descriptor().mediaPlaces) add(d, {}, std::nullopt);
        for (const machine::PlugEntry* p : walked_)   // in the order of the place walk (EC-4)
            if (const machine::PeripheralDescriptor* d = cat_.peripheral(p->peripheral))
                for (const auto& mp : d->mediaPlaces) add(mp, p->place, p->peripheral);
    }

    // D-3: one paper panel when a printer of the product's list fits a place
    // the machine lists; owned by the first plugged-in printer of the walk.
    void panels(ControlState& out) const {
        auto isPrinter = [&](const PeripheralId& id) {
            return std::find(setup_.printers.begin(), setup_.printers.end(), id) != setup_.printers.end();
        };
        bool fits = false;
        for (const auto& p : out.places)
            for (const auto& c : p.fitting) fits = fits || isPrinter(c.id);
        if (!fits) return;
        PanelEntry panel{kPaperPanel, std::nullopt, std::nullopt};
        for (const machine::PlugEntry* p : walked_)
            if (isPrinter(p->peripheral)) {
                panel.ownerPlace = p->place;
                panel.owner = p->peripheral;
                break;
            }
        out.panels.push_back(std::move(panel));
    }

    void actions(ControlState& out) const {
        for (const auto* c : {&kResetAction, &kColdBootAction, &kLoadPresetAction, &kOpenAction}) {
            const bool ends = *c == kResetAction || *c == kColdBootAction;   // LIB-18 AC-16
            out.actions.push_back(ActionEntry{*c, ruled(ActionKey{*c}, ends)});
        }
    }

    const machine::Machine& m_;
    const ControlSetup& setup_;
    const machine::Catalogue& cat_;
    bool runs_;
    machine::Plugged plugged_;
    std::vector<const machine::PlugEntry*> walked_;   // what is plugged in, in the order of the place walk
};

} // namespace

ControlState computeControlState(const machine::Machine& m, const ControlSetup& setup, bool operationRuns) {
    return Computation(m, setup, operationRuns).run();
}

} // namespace calcu::control
