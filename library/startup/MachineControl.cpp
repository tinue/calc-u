#include "startup/MachineControl.hpp"
#include "machine/Build.hpp"
#include "pacing/Pacer.hpp"
#include "sessions/MachineSession.hpp"
#include <algorithm>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <utility>

namespace calcu::startup {

using machine::Machine;
using machine::PeripheralRequest;
using machine::PlacePath;
using machine::Ticks;

namespace {

std::string nameOf(const machine::PeripheralDescriptor& d) { return d.name.empty() ? d.id.value : d.name; }

template <class D> bool hasVariant(const D& d, const std::optional<machine::VariantId>& v) {
    if (!v) return true;
    return std::any_of(d.variants.begin(), d.variants.end(), [&](const machine::VariantDescriptor& x) { return x.id == *v; });
}

// The refusal of one peripheral of a plan on the machine's own places, with
// its place (LIB-6 AC-52): checked before the builder sees the request.
std::optional<BuildRefusal> checkPlug(const machine::Catalogue& cat, const machine::MachineDescriptor& model,
                                      const PeripheralRequest& p) {
    const std::string where = machine::placeText(p.place);
    auto refused = [&](std::string why) { return BuildRefusal{std::move(why), p.place}; };
    const machine::PeripheralDescriptor* d = cat.peripheral(p.peripheral);
    if (!d) return refused(p.peripheral.value + " is not a known peripheral.");
    if (!hasVariant(*d, p.variant)) return refused("The " + nameOf(*d) + " has no variant " + p.variant->value + ".");
    if (p.place.steps.size() != 1) return std::nullopt;   // a place on a peripheral: the builder's
    const auto info = cat.place(model.model, p.place.steps.front());
    if (!info) return refused("The " + model.model.value + " has no place " + where + ".");
    if (std::find(info->fitting.begin(), info->fitting.end(), p.peripheral) == info->fitting.end())
        return refused("The " + nameOf(*d) + " does not fit " + where + ".");
    return std::nullopt;
}

const machine::PlugEntry* entryAt(const machine::Plugged& plugged, const PlacePath& place) {
    for (const auto& e : plugged.byPlace)
        if (e.place == place) return &e;
    return nullptr;
}

// "25", "0.01": the seconds of a bound as the log line writes them.
std::string secondsText(machine::Seconds s) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "%g", s.count());
    return buf;
}

} // namespace

MachineControl::MachineControl(ProductMachines& product, std::shared_ptr<const roms::RomSet> roms)
    : product_(product), roms_(std::move(roms)) {}

PeripheralRequest MachineControl::withRoms(PeripheralRequest r) const {
    if (const machine::PeripheralDescriptor* d = product_.catalogue().peripheral(r.peripheral); d && roms_)
        r.roms = roms_->imagesFor(*d, r.variant);
    product_.addRoms(r);
    return r;
}

std::optional<BuildRefusal> MachineControl::build(sessions::SessionContext& s, MachinePlan plan, BuildOrigin origin) {
    const machine::Catalogue& cat = product_.catalogue();
    const machine::MachineDescriptor* model = cat.model(plan.model);
    if (!model) return BuildRefusal{"There is no model \"" + plan.model.value + "\".", std::nullopt};
    if (!hasVariant(*model, plan.variant))
        return BuildRefusal{"The " + model->model.value + " has no variant \"" + plan.variant->value + "\".",
                            std::nullopt};
    for (const auto& p : plan.peripherals)
        if (auto r = checkPlug(cat, *model, p)) return r;

    machine::BuildRequest req;
    req.model = plan.model;
    req.variant = plan.variant;
    if (roms_) req.roms = roms_->imagesFor(*model, plan.variant);
    for (const auto& p : plan.peripherals) req.peripherals.push_back(withRoms(p));
    req.soundRate = product_.soundRate(plan.model);
    req.content = plan.content;
    machine::BuildResult built = machine::build(cat, product_.builder(), req);
    if (!built.machine) {
        std::string why = "The " + model->model.value + " could not be built: ";
        if (!built.missingRoms.empty()) {
            std::string files;
            for (const auto& f : built.missingRoms) files += (files.empty() ? "" : ", ") + f;
            why += files + (built.missingRoms.size() == 1 ? " is" : " are") + " missing.";
        } else {
            why += built.refusal + ".";
        }
        return BuildRefusal{std::move(why), std::nullopt};
    }
    product_.startUp(*built.machine);

    sessions::MachineSession& ms = s.machineSession();
    ms.dropInput();   // AC-2, EC-4
    // D-11: full speed does not carry over. The commanded speed does not
    // change while an operation runs (LIB-3 D-6): then StartupPart sets it at
    // the operation's end.
    s.setCommandedSpeed(pacing::Speed::Regular);
    if (s.busy()) regularDue_ = true;
    ms.setMachine(std::move(built.machine), product_.pacingProfile(plan.model), req.soundRate);   // the old one goes
    lastBuild_ = BuiltWith{std::move(plan), origin};
    return std::nullopt;
}

PeripheralRequest MachineControl::requestFor(const PlacePath& place, const machine::PeripheralId& id,
                                             std::optional<machine::VariantId> variant) const {
    PeripheralRequest r;
    r.place = place;
    r.peripheral = id;
    r.variant = std::move(variant);
    return r;
}

std::optional<BuildRefusal> MachineControl::check(const Machine& m, const PlugChange& c) const {
    const machine::Catalogue& cat = product_.catalogue();
    std::optional<machine::Refusal> r;
    switch (c.kind) {
    case PlugChange::Kind::Attach: r = m.canAttach(withRoms(c.request), cat); break;
    case PlugChange::Kind::Replace: {
        const machine::Plugged now = m.plugged();
        const machine::PlugEntry* old = entryAt(now, c.place);
        if (!old) r = m.canAttach(withRoms(c.request), cat);
        else r = m.canReplace(old->peripheral, withRoms(c.request), cat);
        break;
    }
    case PlugChange::Kind::Detach: break;   // an empty place is Done, before (D-14)
    }
    if (!r) return std::nullopt;
    return BuildRefusal{r->reason, c.place};
}

sessions::Task<void> MachineControl::change(sessions::OperationContext& c, PlugChange ch) {
    Machine* m = c.session().machine();
    if (!m) throw std::runtime_error("No machine is running.");
    const machine::Catalogue& cat = product_.catalogue();
    machine::ChangeResult r;
    const machine::Plugged now = m->plugged();
    const machine::PlugEntry* at = entryAt(now, ch.place);
    const std::optional<machine::PeripheralId> there = at ? std::optional(at->peripheral) : std::nullopt;
    // LIB-7 AC-13 (M-41): the parts write what they have to save first.
    if (ch.kind != PlugChange::Kind::Detach || there) c.session().machineSession().aboutToChange();
    switch (ch.kind) {
    case PlugChange::Kind::Attach: r = m->attach(withRoms(ch.request), cat); break;
    case PlugChange::Kind::Replace:
        r = there ? m->replace(*there, withRoms(ch.request), cat) : m->attach(withRoms(ch.request), cat);
        break;
    case PlugChange::Kind::Detach:
        if (there) r = m->detach(ch.place, *there, cat);   // EC-5: what follows asked once for a chain
        break;
    }
    if (r.refused) throw std::runtime_error(r.refused->reason);   // AC-12: nothing changed
    if (r.follows == machine::AfterChange::PowerCycle) {
        c.session().machineSession().dropInput();   // AC-14
        c.session().machine()->powerCycle();         // AC-11
        co_await untilKeyReady(c);
    }
    // AfterChange::Nothing: done (a switched-off machine, AC-33).
}

sessions::Task<bool> MachineControl::untilKeyReady(sessions::OperationContext& c) {
    Machine* m = c.session().machine();
    if (!m) co_return false;
    const machine::ModelId model = m->model();
    const machine::TickRate rate = m->descriptor().tickRate;
    const machine::Seconds bound = product_.readinessBound(model);
    const Ticks limit = machine::EmulatedTime::fromSeconds(bound, rate).ticks();
    const Ticks start = m->tickCount();
    for (;;) {
        m = c.session().machine();
        if (!m) co_return false;
        if (m->isKeyReady()) co_return true;   // EC-3: also at a prompt of the ROM
        if (m->tickCount() - start >= limit) break;
        co_await c.run(sessions::OperationContext::runPiece(rate));   // D-4
    }
    // EC-7, D-5: the operation ends as done; one log line, no notice.
    c.session().log("The " + model.value + " was not ready for keys after " + secondsText(bound) +
                    " seconds; it is left as it is.");
    co_return false;
}

MachinePlan MachineControl::variantPlan(Machine& m, machine::VariantId v) {
    MachinePlan plan;
    plan.model = m.model();
    plan.variant = std::move(v);
    for (const auto& e : m.plugged().byPlace) {
        machine::PeripheralRequest r = product_.carried(e, m);
        for (const Carry& c : carries_) c(r, e, m);   // LIB-7 S-7a
        plan.peripherals.push_back(std::move(r));
    }
    return plan;
}

} // namespace calcu::startup
