#include "startup/StartupPart.hpp"
#include "machine/Names.hpp"
#include "sessions/Args.hpp"
#include "sessions/LibraryTopics.hpp"
#include "sessions/MachineSession.hpp"
#include "settings/Startup.hpp"
#include <algorithm>
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

StartupPart::StartupPart(StartupSetup s) : setup_(s) {}

void StartupPart::machineChanged(SessionContext& c) {
    // AC-9, D-10: whoever built it. A file that is not writable keeps it
    // until the product quits (LIB-9).
    if (const Machine* m = c.machine()) setup_.settings.set("startup", "last-model", m->model().value);
}

void StartupPart::operationEnded(SessionContext& c) {
    if (setup_.control.takeRegularSpeedDue()) c.setCommandedSpeed(pacing::Speed::Regular);   // D-11
}

Task<OperationResult> StartupPart::builtInDefault(OperationContext& c, ModelId model, BuildOrigin origin) {
    if (auto refused = setup_.control.build(c.session(), setup_.control.product().builtInDefault(model), origin))
        co_return OperationResult{false, refused->reason};
    co_await setup_.control.untilKeyReady(c);
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
    const PresetLoad load = co_await loader->load(preset, forModel, setup_.control, c);
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
    co_await setup_.control.untilKeyReady(c);
    co_return OperationResult{};
}

Task<OperationResult> StartupPart::switchVariant(OperationContext& c, VariantId v) {
    Machine* m = c.session().machine();
    if (!m) co_return OperationResult{false, sessions::kNoMachine};
    if (auto refused = setup_.control.build(c.session(), setup_.control.variantPlan(*m, std::move(v)), BuildOrigin::VariantSwitch))
        co_return OperationResult{false, refused->reason};
    co_await setup_.control.untilKeyReady(c);
    co_return OperationResult{};
}

Task<OperationResult> StartupPart::changePlug(OperationContext& c, PlugChange ch) {
    co_await setup_.control.change(c, std::move(ch));
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
    // LIB-10 §3.11: the parts are told of a Reset and a Cold boot before the
    // run to key readiness (the run control's pause-at-start holds it).
    if (what == Restart::Reset) c.session().tellRestarted(sessions::Restart::Reset);
    if (what == Restart::ColdBoot) c.session().tellRestarted(sessions::Restart::ColdBoot);
    co_await setup_.control.untilKeyReady(c);   // AC-13; the clocks: PeripheralsPart::operationEnded
    co_return OperationResult{};
}

void StartupPart::attach(sessions::SessionSetup& setup) {
    const machine::Catalogue& cat = setup_.control.product().catalogue();
    // LIB-12 AC-5, LIB-14 AC-19: without the full set no machine is built;
    // the frontends show the files.
    if (!setup_.control.hasRoms() && setup_.romsMissing) {
        auto p = std::make_shared<RomsMissingState>();
        p->missing = *setup_.romsMissing;
        p->text = roms::describe(p->missing);
        setup.stateTopic(kRomsMissingTopic).set(std::move(p));
    }

    // The start (AC-3 to AC-5): accepted once (D-8), not while an operation runs.
    setup.command(sessions::kStartCommand, [this, &cat](const Value& args, SessionContext& c) -> CommandOutcome {
        if (started_) return Refused{kStartedAlready};
        if (c.busy()) return anotherRuns();
        if (!setup_.control.hasRoms() && setup_.romsMissing) return Refused{roms::describe(*setup_.romsMissing)};   // LIB-12 AC-5
        ModelId model{settings::startupModel(setup_.settings)};   // AC-3
        const std::string named = sessions::textArg(args, "model");
        if (!named.empty()) {
            if (const auto* m = machine::modelNamed(cat, named)) model = m->model;
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
            const machine::MachineDescriptor* model = machine::modelNamed(cat, name);
            if (!model) return Refused{"There is no model \"" + name + "\"."};   // D-18
            if (const Machine* m = c.machine(); m && m->model() == model->model) return Done{};   // AC-8
            return StartOperation{[this, m = model->model](OperationContext& o) {
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
        const auto v = machine::variantNamed(m->descriptor().variants, name);
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
        // D-16: the catalogue's spelling; an unknown name as written, which the machine refuses.
        const std::string written = sessions::textArg(args, "peripheral");
        const machine::PeripheralDescriptor* d = machine::peripheralNamed(cat, written);
        const machine::PeripheralId id = d ? d->id : machine::PeripheralId{written};
        std::optional<VariantId> variant;
        if (const std::string v = sessions::textArg(args, "variant"); !v.empty()) {
            variant = d ? machine::variantNamed(d->variants, v) : std::nullopt;
            if (!variant) variant = VariantId{v};   // the machine refuses it, with its reason
        }
        const machine::Plugged plugged = m->plugged();
        const auto there = entriesAt(plugged, place);
        const std::optional<VariantId> resolved = variant ? variant : (d ? d->defaultVariant : std::nullopt);
        for (const machine::PlugEntry* e : there)
            if (e->peripheral == id && e->variant == resolved) return Done{};   // the same again (D-14)
        PlugChange ch;
        ch.place = place;
        ch.request = setup_.control.requestFor(place, id, variant);
        ch.kind = !there.empty() && takesAtOnce(*m, plugged, cat, place) == 1 ? PlugChange::Kind::Replace
                                                                              : PlugChange::Kind::Attach;
        if (auto refused = setup_.control.check(*m, ch)) return Refused{refused->reason};   // AC-12
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
