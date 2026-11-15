#include "startup/StartupPart.hpp"
#include "sessions/Args.hpp"
#include "sessions/LibraryTopics.hpp"
#include "sessions/MachineSession.hpp"
#include "settings/Startup.hpp"
#include <algorithm>
#include <cctype>
#include <string_view>
#include <utility>

namespace calcu::startup {

using machine::Machine;
using machine::ModelId;
using machine::PlacePath;
using machine::VariantId;
using sessions::CommandOutcome;
using sessions::Done;
using sessions::Operation;
using sessions::OperationContext;
using sessions::OperationResult;
using sessions::Refused;
using sessions::SessionContext;
using sessions::StartOperation;
using sessions::Task;
using sessions::Value;

namespace {

std::string lower(std::string_view s) {
    std::string out(s);
    for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

// D-16: names of models, variants and peripherals in any case; the
// catalogue's spelling back.
std::optional<ModelId> modelNamed(const machine::Catalogue& cat, std::string_view name) {
    for (const auto& m : cat.models())
        if (lower(m.model.value) == lower(name)) return m.model;
    return std::nullopt;
}

std::optional<VariantId> variantNamed(const std::vector<machine::VariantDescriptor>& vs, std::string_view name) {
    for (const auto& v : vs)
        if (lower(v.id.value) == lower(name)) return v.id;
    return std::nullopt;
}

machine::PeripheralId peripheralNamed(const machine::Catalogue& cat, std::string_view name) {
    for (const auto& p : cat.peripherals())
        if (lower(p.id.value) == lower(name)) return p.id;
    return machine::PeripheralId{std::string(name)};
}

// "a/jack" as placeText writes it.
PlacePath placeFrom(std::string_view text) {
    PlacePath p;
    std::size_t at = 0;
    while (at <= text.size()) {
        const std::size_t end = std::min(text.find('/', at), text.size());
        if (end > at) p.steps.push_back(machine::PlaceName{std::string(text.substr(at, end - at))});
        at = end + 1;
    }
    return p;
}

// How many the place holds at once: on the machine for one step, on the
// peripheral plugged in at the prefix otherwise; 0 when there is no such place.
int takesAtOnce(const Machine& m, const machine::Plugged& plugged, const machine::Catalogue& cat,
                const PlacePath& place) {
    if (place.steps.empty()) return 0;
    if (place.steps.size() == 1) {
        const machine::PlaceDescriptor* d = m.descriptor().place(place.steps.front());
        return d ? d->takesAtOnce : 0;
    }
    PlacePath prefix = place;
    prefix.steps.pop_back();
    for (const auto& e : plugged.byPlace)
        if (e.place == prefix)
            if (const auto info = cat.place(e.peripheral, place.steps.back())) return info->place->takesAtOnce;
    return 0;
}

std::vector<const machine::PlugEntry*> entriesAt(const machine::Plugged& plugged, const PlacePath& place) {
    std::vector<const machine::PlugEntry*> out;
    for (const auto& e : plugged.byPlace)
        if (e.place == place) out.push_back(&e);
    return out;
}

Refused anotherRuns() { return Refused{sessions::kAnotherOperationRuns}; }

} // namespace

StartupPart::StartupPart(StartupSetup s) : setup_(s), control_(s.product, s.roms) {}

void StartupPart::machineChanged(SessionContext& c) {
    // AC-9, D-10: whoever built it. A file that is not writable keeps it
    // until the product quits (LIB-9).
    if (const Machine* m = c.machine()) setup_.settings.set("startup", "last-model", m->model().value);
}

void StartupPart::operationEnded(SessionContext& c) {
    if (control_.takeRegularSpeedDue()) c.setCommandedSpeed(pacing::Speed::Regular);   // D-11
}

Task<OperationResult> StartupPart::builtInDefault(OperationContext& c, ModelId model, BuildOrigin origin) {
    if (auto refused = control_.build(c.session(), setup_.product.builtInDefault(model), origin))
        co_return OperationResult{false, refused->reason};
    co_await control_.untilKeyReady(c);
    co_return OperationResult{};
}

Task<OperationResult> StartupPart::buildFor(OperationContext& c, ModelId model, std::string preset,
                                            BuildOrigin origin) {
    SessionContext& s = c.session();
    PresetLoader* loader = setup_.loader;
    std::optional<ModelId> forModel;
    if (preset.empty() && loader) {   // D-9: without a loader [preset] is not read
        preset = settings::startupPreset(setup_.settings, model.value);
        forModel = model;
    }
    if (preset.empty() || !loader) co_return co_await builtInDefault(c, model, origin);   // AC-4
    const PresetLoad load = co_await loader->load(preset, forModel, control_, c);
    switch (load.outcome) {
    case PresetLoad::Outcome::CannotLoad:   // EC-1
        s.notice("The startup preset of the " + model.value + " could not be loaded: " + load.reason + ". The " +
                 model.value + " starts as its built-in default machine.");
        co_return co_await builtInDefault(c, model, origin);
    case PresetLoad::Outcome::FailedWhileRunning:   // EC-6: the machine stays as far as it got
        s.notice("The startup preset of the " + model.value + " stopped: " + load.reason + ".");
        co_return OperationResult{};
    case PresetLoad::Outcome::Built: break;
    }
    if (!s.machine()) co_return OperationResult{false, sessions::kNoMachine};
    co_await control_.untilKeyReady(c);
    co_return OperationResult{};
}

Task<OperationResult> StartupPart::switchVariant(OperationContext& c, VariantId v) {
    Machine* m = c.session().machine();
    if (!m) co_return OperationResult{false, sessions::kNoMachine};
    if (auto refused = control_.build(c.session(), control_.variantPlan(*m, std::move(v)), BuildOrigin::VariantSwitch))
        co_return OperationResult{false, refused->reason};
    co_await control_.untilKeyReady(c);
    co_return OperationResult{};
}

Task<OperationResult> StartupPart::changePlug(OperationContext& c, PlugChange ch) {
    co_await control_.change(c, std::move(ch));
    co_return OperationResult{};
}

Task<OperationResult> StartupPart::restart(OperationContext& c, Restart what) {
    Machine* m = c.session().machine();
    if (!m) co_return OperationResult{false, sessions::kNoMachine};
    c.session().machineSession().dropInput();   // AC-14
    switch (what) {
    case Restart::Reset: m->reset(); break;
    case Restart::ColdBoot: m->coldBoot(); break;
    case Restart::PowerCycle: m->powerCycle(); break;
    }
    co_await control_.untilKeyReady(c);   // AC-13; the clocks: PeripheralsPart::operationEnded
    co_return OperationResult{};
}

void StartupPart::attach(sessions::SessionSetup& setup) {
    const machine::Catalogue& cat = setup_.product.catalogue();

    // The start (AC-3 to AC-5): accepted once (D-8), not while an operation runs.
    setup.command(sessions::kStartCommand, [this, &cat](const Value& args, SessionContext& c) -> CommandOutcome {
        if (started_) return Refused{kStartedAlready};
        if (c.busy()) return anotherRuns();
        ModelId model{settings::startupModel(setup_.settings)};   // AC-3
        const std::string named = sessions::textArg(args, "model");
        if (!named.empty()) {
            if (auto m = modelNamed(cat, named)) model = *m;
            else c.notice("There is no model \"" + named + "\"; the startup model is used.");   // LIB-15 EC-4
        }
        const std::string preset = sessions::textArg(args, "preset");
        started_ = true;
        return StartOperation{[this, model, preset](OperationContext& o) {
            return buildFor(o, model, preset, BuildOrigin::Start);
        }};
    });

    // Model switch (AC-6, AC-8): ends a running operation (AC-16).
    setup.command(
        kModelCommand,
        [this, &cat](const Value& args, SessionContext& c) -> CommandOutcome {
            const std::string name = sessions::textArg(args, "name");
            const auto model = modelNamed(cat, name);
            if (!model) return Refused{"There is no model \"" + name + "\"."};   // D-18
            if (const Machine* m = c.machine(); m && m->model() == *model) return Done{};   // AC-8
            return StartOperation{[this, m = *model](OperationContext& o) {
                return buildFor(o, m, std::string(), BuildOrigin::ModelSwitch);
            }};
        },
        /*endsRunningOperation=*/true);

    // Variant switch (AC-7, AC-8).
    setup.command(kVariantCommand, [this](const Value& args, SessionContext& c) -> CommandOutcome {
        if (c.busy()) return anotherRuns();   // D-7
        const Machine* m = c.machine();
        if (!m) return Refused{sessions::kNoMachine};
        const std::string name = sessions::textArg(args, "name");
        const auto v = variantNamed(m->descriptor().variants, name);
        if (!v) return Refused{"The " + m->model().value + " has no variant \"" + name + "\"."};   // D-18
        if (m->variant() == v) return Done{};   // AC-8
        return StartOperation{[this, chosen = *v](OperationContext& o) { return switchVariant(o, chosen); }};
    });

    // Attach and replace (AC-10 to AC-12; D-14, D-15).
    setup.command(kAttachCommand, [this, &cat](const Value& args, SessionContext& c) -> CommandOutcome {
        if (c.busy()) return anotherRuns();
        const Machine* m = c.machine();
        if (!m) return Refused{sessions::kNoMachine};
        const PlacePath place = placeFrom(sessions::textArg(args, "place"));
        const machine::PeripheralId id = peripheralNamed(cat, sessions::textArg(args, "peripheral"));
        std::optional<VariantId> variant;
        const machine::PeripheralDescriptor* d = cat.peripheral(id);
        if (const std::string v = sessions::textArg(args, "variant"); !v.empty()) {
            variant = d ? variantNamed(d->variants, v) : std::nullopt;
            if (!variant) variant = VariantId{v};   // the machine refuses it, with its reason
        }
        const machine::Plugged plugged = m->plugged();
        const auto there = entriesAt(plugged, place);
        const std::optional<VariantId> resolved = variant ? variant : (d ? d->defaultVariant : std::nullopt);
        for (const machine::PlugEntry* e : there)
            if (e->peripheral == id && e->variant == resolved) return Done{};   // the same again (D-14)
        PlugChange ch;
        ch.place = place;
        ch.request = control_.requestFor(place, id, variant);
        ch.kind = !there.empty() && takesAtOnce(*m, plugged, cat, place) == 1 ? PlugChange::Kind::Replace
                                                                              : PlugChange::Kind::Attach;
        if (auto refused = control_.check(*m, ch)) return Refused{refused->reason};   // AC-12
        return StartOperation{[this, ch](OperationContext& o) { return changePlug(o, ch); }};
    });

    // Detach (AC-10, EC-5): an empty place is Done (LIB-2 EC-2; D-14).
    setup.command(kDetachCommand, [this](const Value& args, SessionContext& c) -> CommandOutcome {
        if (c.busy()) return anotherRuns();
        const Machine* m = c.machine();
        if (!m) return Refused{sessions::kNoMachine};
        PlugChange ch;
        ch.kind = PlugChange::Kind::Detach;
        ch.place = placeFrom(sessions::textArg(args, "place"));
        if (entriesAt(m->plugged(), ch.place).empty()) return Done{};
        return StartOperation{[this, ch](OperationContext& o) { return changePlug(o, ch); }};
    });

    // Reset, Cold boot (end a running operation, AC-16), power cycle (AC-13, AC-14).
    auto restartCommand = [this](Restart what, bool ends) {
        return [this, what, ends](const Value&, SessionContext& c) -> CommandOutcome {
            if (!ends && c.busy()) return anotherRuns();
            if (!c.machine()) return Refused{sessions::kNoMachine};
            return StartOperation{[this, what](OperationContext& o) { return restart(o, what); }};
        };
    };
    setup.command(kResetCommand, restartCommand(Restart::Reset, true), /*endsRunningOperation=*/true);
    setup.command(kColdBootCommand, restartCommand(Restart::ColdBoot, true), /*endsRunningOperation=*/true);
    setup.command(kPowerCycleCommand, restartCommand(Restart::PowerCycle, false));
}

} // namespace calcu::startup
