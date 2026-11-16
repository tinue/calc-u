#include "presets/Engine.hpp"
#include "presets/IdleWatch.hpp"
#include "sessions/LibraryTopics.hpp"
#include <cstdio>

namespace calcu::presets {

using machine::Ticks;
using sessions::Task;

RunContext::RunContext(sessions::OperationContext& op, const LoadPlan* plan, ModelRules& rules, EngineDeps& deps)
    : op_(op), plan_(plan), rules_(rules), deps_(deps) {}

Task<bool> RunContext::untilRunning() { return presets::untilRunning(op_); }

machine::DisplayOutput RunContext::display() {
    if (machine::Machine* m = machine()) return m->readDisplay();
    if (const machine::DisplayOutput* d = session().display()) return *d;
    machine::DisplayOutput off;
    off.on = false;
    return off;
}

ElapsedBase& RunContext::elapsed() { return deps_.elapsed ? *deps_.elapsed : ownElapsed_; }

startup::MachinePlan machinePlan(const LoadPlan& p) {
    startup::MachinePlan plan;
    plan.model = p.model;
    plan.variant = p.variant;
    for (const auto& r : p.places) plan.peripherals.push_back(r.request);
    if (p.rules) p.rules->amendPlan(p, plan);   // D-33
    return plan;
}

std::string elapsedText(Ticks ticks, machine::TickRate rate) {
    const Ticks ms = rate ? ticks * 1000 / rate : 0;
    char buf[48];
    std::snprintf(buf, sizeof buf, "%llu:%02llu.%03llu", static_cast<unsigned long long>(ms / 60000),
                  static_cast<unsigned long long>(ms / 1000 % 60), static_cast<unsigned long long>(ms % 1000));
    return buf;
}

namespace {

std::string withVariant(const std::string& id, const std::optional<machine::VariantId>& v) {
    return v ? id + ":" + v->value : id;
}

int lineOfPlace(const LoadPlan& p, const std::optional<machine::PlacePath>& place) {
    if (place)
        for (const auto& r : p.places)
            if (r.request.place == *place) return r.line;
    return p.modelLine;
}

} // namespace

Task<LoadOutcome> carryOut(sessions::OperationContext& op, std::shared_ptr<const LoadPlan> plan, LoadOptions opts,
                           EngineDeps& deps) {
    sessions::SessionContext& s = op.session();
    const std::string& name = plan->name;
    ModelRules& rules = *plan->rules;
    RunContext run(op, plan.get(), rules, deps);
    auto stopped = [&](int line, const std::string& why) {
        const std::string message = errorText(name, line, why);
        s.log("Preset " + name + " stopped: " + message);
        return LoadOutcome{LoadOutcome::Kind::Failed, message};
    };

    // 1. The build (AC-10, AC-16, AC-52): at the operation's full speed.
    if (!deps.control) co_return LoadOutcome{LoadOutcome::Kind::NotBuilt, errorText(name, 0, "No machine can be built.")};
    const auto refused = deps.control->build(
        s, machinePlan(*plan), opts.startup ? startup::BuildOrigin::StartupPreset : startup::BuildOrigin::Preset);
    if (refused)   // nothing changed (LIB-18 §3.4)
        co_return LoadOutcome{LoadOutcome::Kind::NotBuilt, errorText(name, lineOfPlace(*plan, refused->place), refused->reason)};
    s.log("Preset " + name + ": " + withVariant(plan->model.value, plan->variant));   // AC-18
    for (const auto& r : plan->places)
        s.log("  " + machine::placeText(r.request.place) + ": " + withVariant(r.request.peripheral.value, r.request.variant));
    run.elapsed() = ElapsedBase{s.machineSession().generation(), std::nullopt};   // D-10: from tick 0
    if (opts.buildOnly) {   // AC-55: over at once, without waiting for key readiness
        s.log("Preset " + name + " built only.");
        co_return LoadOutcome{};
    }
    const bool ready = co_await deps.control->untilKeyReady(op);
    if (!ready)   // EC-10: no item runs
        co_return stopped(plan->modelLine, "The " + plan->model.value + " did not report key readiness after it was built.");

    // 2. What the product does after the build (AC-46, AC-53).
    const ItemResult after = co_await rules.afterBuild(run);
    if (!after.ok) co_return stopped(plan->modelLine, after.reason);

    // 3. The items in file order (AC-14), each waited for (AC-56).
    const machine::Machine* m = s.machine();
    const machine::TickRate rate = m ? m->descriptor().tickRate : 1;
    const Ticks bound = static_cast<Ticks>(plan->header.timeoutSeconds) * rate;
    IdleWatch watch(rules.idle());
    bool atOnce = false;
    for (std::size_t i = 0; i < plan->items.size(); ++i) {
        const PlannedItem& entry = plan->items[i];
        Item& item = *entry.item;
        const bool last = i + 1 == plan->items.size();
        if (item.needsRunningMachine()) {   // AC-19: it waits while the machine is paused
            const bool runs = co_await run.untilRunning();
            if (!runs) co_return stopped(item.line, "The machine is paused.");
        }
        run.setFirstPressAtOnce(atOnce);
        run.setDeadline(bound);
        const ItemResult res = co_await item.carryOut(run);
        if (!res.ok) {   // AC-17: the machine stays as it is
            s.log("  line " + std::to_string(item.line) + " " + item.logText() + ": failed: " + res.reason);
            co_return stopped(item.line, res.reason);
        }
        s.log("  line " + std::to_string(item.line) + " " + item.logText() + ": done");
        atOnce = entry.skipWaitAfter;
        if (last) {
            if (!plan->letItFinish) break;   // AC-60: not waited for
        } else if (entry.skipWaitAfter || item.after() != After::Idle) {
            continue;   // AC-58; an item that has finished when carried out
        }
        // The implicit wait (AC-56), bounded by timeout: (AC-57); after the
        // last item only with let-it-finish:, without a bound (AC-59).
        const std::optional<Ticks> limit = last ? std::nullopt : std::optional<Ticks>(bound);
        const IdleOutcome w = co_await watch.wait(op, limit);
        if (w.noMachine) co_return stopped(item.line, sessions::kNoMachine);
        if (w.paused) co_return stopped(item.line, "The machine is paused.");
        if (w.boundReached)
            co_return stopped(item.line, "The " + item.logText() + " has not finished after " +
                                             std::to_string(plan->header.timeoutSeconds) + " s of emulated time (timeout:).");
        if (last) s.log("  let-it-finish: the machine is idle");
    }
    if (plan->items.empty() && plan->letItFinish) {   // let-it-finish: alone
        const IdleOutcome w = co_await watch.wait(op, std::nullopt);
        if (!w.idle) co_return stopped(plan->modelLine, w.paused ? "The machine is paused." : sessions::kNoMachine);
    }
    s.log("Preset " + name + " carried out to its end.");   // AC-18
    co_return LoadOutcome{};
}

} // namespace calcu::presets
