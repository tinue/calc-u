#include "presets/PresetsPart.hpp"
#include "formats/Flow.hpp"
#include "presets/IdleWatch.hpp"
#include "sessions/Args.hpp"
#include "sessions/LibraryTopics.hpp"
#include "sessions/MachineSession.hpp"
#include <memory>

namespace calcu::presets {

using sessions::CommandOutcome;
using sessions::Done;
using sessions::OperationContext;
using sessions::OperationResult;
using sessions::Refused;
using sessions::SessionContext;
using sessions::StartOperation;
using sessions::Task;
using sessions::Value;

sessions::Value PresetListReply::toValue() const {
    Value::Array a;
    for (const auto& e : entries) {
        Value::Array files;
        for (const auto& f : e.files) files.push_back(Value(f));
        a.push_back(Value::object({{"bundlePath", e.bundlePath},
                                   {"model", e.model.value},
                                   {"name", e.name},
                                   {"description", e.description},
                                   {"kind", e.kind == PresetListEntry::Kind::Startup ? "startup" : "example"},
                                   {"group", e.group},
                                   {"files", Value(std::move(files))}}));
    }
    return Value::object({{"entries", Value(std::move(a))}});
}

PresetsPart::PresetsPart(PresetsSetup s) : setup_(std::move(s)) {
    addLibraryKeywords(vocabulary_);
    if (setup_.rules) setup_.rules->addKeywords(vocabulary_);
}

ReadSetup PresetsPart::readSetup() const {
    return ReadSetup{vocabulary_, *setup_.rules, *setup_.catalogue, setup_.typing ? &setup_.typing->product() : nullptr};
}

EngineDeps& PresetsPart::deps() {
    deps_.control = setup_.control;
    deps_.typing = setup_.typing;
    deps_.screenshotFolder = setup_.screenshotFolder;
    deps_.elapsed = &elapsed_;
    return deps_;
}

std::unique_ptr<PresetSource> PresetsPart::sourceOf(const Value& args) const {
    if (const Value* content = args.find("content"); content && content->string()) {   // EC-5, AC-42
        std::string name = sessions::textArg(args, "name");
        if (name.empty()) name = "the preset";
        return contentSource(*content->string(), std::move(name), setup_.home);
    }
    return sourceFor(sessions::textArg(args, "ref"), setup_.bundle, setup_.home);
}

Task<OperationResult> PresetsPart::runLoad(OperationContext& op, std::shared_ptr<const LoadPlan> plan, LoadOptions o) {
    regularDue_ = true;
    const LoadOutcome out = co_await carryOut(op, std::move(plan), o, deps());
    co_return OperationResult{out.kind == LoadOutcome::Kind::Done, out.message};
}

Task<startup::PresetLoad> PresetsPart::load(std::string ref, std::optional<machine::ModelId> forModel,
                                            startup::MachineControl& control, OperationContext& op) {
    using startup::PresetLoad;
    const auto source = sourceFor(ref, setup_.bundle, setup_.home);
    auto read = readPreset(*source, readSetup());
    if (auto* e = std::get_if<ReadError>(&read)) co_return PresetLoad{PresetLoad::Outcome::CannotLoad, e->message};
    auto plan = std::make_shared<LoadPlan>(std::get<LoadPlan>(std::move(read)));
    if (forModel && plan->model != *forModel)
        co_return PresetLoad{PresetLoad::Outcome::CannotLoad,
                             source->name() + " is a preset for the " + plan->model.value};
    if (!setup_.control) setup_.control = &control;
    regularDue_ = true;
    const LoadOutcome out = co_await carryOut(op, plan, LoadOptions{false, true}, deps());
    switch (out.kind) {
    case LoadOutcome::Kind::NotBuilt: co_return PresetLoad{PresetLoad::Outcome::CannotLoad, out.message};
    case LoadOutcome::Kind::Failed: co_return PresetLoad{PresetLoad::Outcome::FailedWhileRunning, out.message};
    case LoadOutcome::Kind::Done: break;
    }
    co_return PresetLoad{};
}

Task<OperationResult> PresetsPart::loadWithin(OperationContext& op, std::string ref, bool buildOnly,
                                             std::optional<machine::ModelId> forModel) {
    if (!setup_.rules || !setup_.catalogue) co_return OperationResult{false, "This build reads no presets of format 2 yet."};
    const auto source = sourceFor(ref, setup_.bundle, setup_.home);
    auto read = readPreset(*source, readSetup());
    if (auto* e = std::get_if<ReadError>(&read)) co_return OperationResult{false, e->message};
    auto plan = std::make_shared<const LoadPlan>(std::get<LoadPlan>(std::move(read)));
    if (forModel && plan->model != *forModel)
        co_return OperationResult{false, source->name() + " is a preset for the " + plan->model.value};
    const LoadOptions options{buildOnly, false};
    OperationResult r = co_await runLoad(op, plan, options);
    co_return r;
}

namespace {

Task<OperationResult> runItem(OperationContext& op, std::shared_ptr<Item> item, ModelRules* rules, EngineDeps* deps) {
    RunContext run(op, nullptr, *rules, *deps);
    if (item->needsRunningMachine()) {
        const bool runs = co_await run.untilRunning();
        if (!runs) co_return OperationResult{false, "The machine is paused."};
    }
    const ItemResult r = co_await item->carryOut(run);
    co_return OperationResult{r.ok, r.reason};
}

Task<OperationResult> waitIdle(OperationContext& op, IdleRule rule) {
    const machine::Machine* m = op.session().machine();
    if (!m) co_return OperationResult{false, sessions::kNoMachine};
    const machine::Ticks hour = static_cast<machine::Ticks>(m->descriptor().tickRate) * 3600;
    const IdleOutcome w = co_await IdleWatch(std::move(rule)).wait(op, hour);   // AC-34
    if (w.noMachine) co_return OperationResult{false, sessions::kNoMachine};
    if (w.paused) co_return OperationResult{false, "The machine is paused."};
    co_return OperationResult{};
}

} // namespace

CommandOutcome PresetsPart::loadPreset(const Value& args, SessionContext& c) {
    readRefusal_ = false;
    if (c.busy()) return Refused{sessions::kAnotherOperationRuns};
    if (!setup_.rules || !setup_.catalogue) return Refused{"This build reads no presets of format 2 yet."};
    const auto source = sourceOf(args);
    auto read = readPreset(*source, readSetup());
    if (auto* e = std::get_if<ReadError>(&read)) {
        readRefusal_ = true;
        return Refused{e->message};
    }
    auto plan = std::make_shared<const LoadPlan>(std::get<LoadPlan>(std::move(read)));
    const LoadOptions o{sessions::flagArg(args, "buildOnly"), sessions::textArg(args, "cause") == "startup"};
    return StartOperation{[this, plan, o](OperationContext& op) { return runLoad(op, plan, o); }};
}

void PresetsPart::attach(sessions::SessionSetup& s) {
    for (const auto& d : vocabulary_.duplicates())
        s.context().log("The preset keyword " + d + " was added twice; the second is not used.");

    // AC-6, D-6: read and checked whole before the operation starts.
    s.command(kLoadPresetCommand, [this](const Value& args, SessionContext& c) -> CommandOutcome {
        CommandOutcome o = loadPreset(args, c);
        if (auto* r = std::get_if<Refused>(&o); r && readRefusal_) c.notice(r->reason);
        return o;
    });

    // One step against the running machine (the harness commands, D-15).
    s.command(kRunStepCommand, [this](const Value& args, SessionContext& c) -> CommandOutcome {
        if (c.busy()) return Refused{sessions::kAnotherOperationRuns};
        machine::Machine* m = c.machine();
        if (!m) return Refused{sessions::kNoMachine};
        if (!setup_.rules || !setup_.catalogue) return Refused{"This build reads no presets of format 2 yet."};
        const std::string verb = sessions::textArg(args, "verb");
        const std::string text = sessions::textArg(args, "value");
        formats::YamlNode node;
        const StepDef* def = vocabulary_.step(verb);
        if (def && def->value == StepDef::Value::Raw) {
            node.type = text.empty() ? formats::YamlNode::Type::Null : formats::YamlNode::Type::Scalar;
            node.scalar = text;
            node.rawLine = true;
        } else {
            std::string error;
            if (!formats::parseValue(text, &node, &error)) return Refused{error};
        }
        node.line = 1;
        std::string error;
        std::unique_ptr<Item> read = readStep(verb, node, readSetup(), *m, &error);
        if (!read) return Refused{error};
        std::shared_ptr<Item> item(std::move(read));
        ModelRules* rules = &setup_.rules->forModel(m->model());
        if (!item->needsRunningMachine() && item->after() == After::TakenEffect) {   // done at once, with its result
            OperationContext op(c);
            RunContext run(op, nullptr, *rules, deps());
            const ItemResult r = sessions::runToCompletion(item->carryOut(run));
            if (!r.ok) return Refused{r.reason};
            if (r.result.empty()) return Done{};
            return Done{sessions::valuePayload(kStepResultKind, Value::object({{"text", r.result}}))};
        }
        return StartOperation{[this, item, rules](OperationContext& op) { return runItem(op, item, rules, &deps()); }};
    });

    s.command(kWaitIdleCommand, [this](const Value&, SessionContext& c) -> CommandOutcome {
        if (c.busy()) return Refused{sessions::kAnotherOperationRuns};
        const machine::Machine* m = c.machine();
        if (!m) return Refused{sessions::kNoMachine};
        if (!setup_.rules) return Refused{"This build reads no presets of format 2 yet."};
        IdleRule rule = setup_.rules->forModel(m->model()).idle();
        return StartOperation{[rule](OperationContext& op) { return waitIdle(op, rule); }};
    });

    s.request(kCheckPresetRequest, [this](const Value& args, SessionContext&) {
        if (!setup_.rules || !setup_.catalogue) return sessions::replyRefused("This build reads no presets of format 2 yet.");
        const auto source = sourceOf(args);
        auto read = readPreset(*source, readSetup());
        if (auto* e = std::get_if<ReadError>(&read)) return sessions::replyRefused(e->message);
        return sessions::replyGiven(sessions::valuePayload("check-preset", Value::object({{"ok", true}})));
    });

    s.request(kPresetListRequest, [this](const Value&, SessionContext& c) {
        if (!setup_.rules || !setup_.catalogue || !setup_.bundle)
            return sessions::replyRefused("This product bundles no presets of format 2 yet.");
        std::vector<std::string> left;
        auto reply = std::make_shared<PresetListReply>();
        reply->entries = presetList(*setup_.bundle, *setup_.rules, *setup_.catalogue, &left);
        for (auto& l : left) c.log(std::move(l));   // EC-9
        return sessions::replyGiven(std::move(reply));
    });
}

void PresetsPart::operationEnded(SessionContext& c) {
    if (!regularDue_) return;
    regularDue_ = false;
    c.setCommandedSpeed(pacing::Speed::Regular);   // AC-22, AC-60, D-19
}

} // namespace calcu::presets
