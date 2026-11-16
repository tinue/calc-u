#include "Machine/LegacyCalls.hpp"

#include "../Debug/MachineDebugTargets.hpp"
#include "../PC1600/PC1600PresetMedia.hpp"
#include "../PC1500/PC1500BasicTyper.hpp"
#include "../PC1500/PC1500PresetLoader.hpp"
#include "../PC1600/PC1600BasicTyper.hpp"
#include "../PC1600/PC1600PresetLoader.hpp"
#include "../PC1600/PC1600TypedInput.hpp"
#include "Machine/SharpCores.hpp"

using calcu::machine::Machine;

namespace sharp::legacy {

std::unique_ptr<debug::DebugTarget> makeDebugTarget(Machine& m) {
    if (PC1600Machine* p = pc1600Of(m)) return debug::makeDebugTarget(*p);
    if (PC1500Machine* p = pc1500Of(m)) return debug::makeDebugTarget(*p);
    return nullptr;
}

debug::LoadResult loadProgram(Machine& m, const debug::LoadRequest& request, debug::SourceMap& map,
                              const debug::DebugTarget& target) {
    return debug::loadProgram(m, request, map, target);
}

job::Task<PresetLoadResult> applyPresetJob(Machine& m, PresetFile preset, PresetLogFn log, std::string traceDir,
                                           std::string moduleDir, PresetBootedFn onBooted,
                                           PresetRoms presetRoms, std::vector<std::string> extraModuleDirs,
                                           PresetArmedFn onArmed, PresetSaveAsFn onSaveAs,
                                           calcu::sessions::OperationContext* ctx, calcu::typing::TypingEngine* engine) {
    PresetLoadResult result;
    const PresetTyping typing{ctx, engine, &m};
    if (PC1600Machine* p = pc1600Of(m)) {
        static_cast<PC1600InterfaceMachine&>(m).notePresetCe1600pVariant(
            calcu::machine::VariantId{preset.ce1600pRomVariant == "old" ? "Old" : "New"});
        result = co_await applyPC1600PresetJob(*p, std::move(preset), std::move(log), std::move(traceDir),
                                               std::move(moduleDir), std::move(onBooted), std::move(presetRoms),
                                               std::move(extraModuleDirs), std::move(onArmed), std::move(onSaveAs), typing);
    } else if (PC1500Machine* p = pc1500Of(m)) {
        result = co_await applyPC1500PresetJob(*p, std::move(preset), std::move(log), std::move(traceDir),
                                               std::move(moduleDir), std::move(onBooted), std::move(presetRoms),
                                               std::move(extraModuleDirs), std::move(onArmed), std::move(onSaveAs), typing);
    } else {
        result.error = "not a Sharp machine";
    }
    if (SharpMachine* s = sharpOf(m)) s->syncFromCore();
    co_return result;
}

void syncPlugged(Machine& m) {
    if (SharpMachine* s = sharpOf(m)) s->syncFromCore();
}

job::Task<void> runToPromptJob(Machine& m) {
    SharpMachine* s = sharpOf(m);
    if (!s) co_return;
    // One advance per phase: each phase is one run of the core, as today.
    while (s->powerCycleActive()) co_await job::run(m, s->powerCyclePhaseTicks());
    if (PC1600Machine* p = pc1600Of(m)) co_await runBootToPromptJob(*p);
    else if (PC1500Machine* p = pc1500Of(m)) co_await runBootToPromptJob(*p);
}

void runToPrompt(Machine& m) { job::runToCompletion(runToPromptJob(m)); }

const ExpansionCard* moduleIn(Machine& m, int slot) {
    // What the slot shows now: a module still waiting for the off point too.
    if (slot == 2 && pc1500Of(m)) return nullptr;   // the PC-1500 has one slot
    if (SharpMachine* s = sharpOf(m))
        return slot == 1 || slot == 2 ? s->moduleNow(calcu::machine::PlaceName{slot == 2 ? kSlot2 : kSlot1}) : nullptr;
    return nullptr;
}

void atOffPoint(Machine& m, std::function<void()> f) {
    if (SharpMachine* s = sharpOf(m)) s->atOffPoint(std::move(f));
    else f();
}

CoreParts coreParts(Machine& m) {
    CoreParts p;
    if (PC1600Machine* c = pc1600Of(m)) {
        p.sc7852 = &c->sc7852();
        p.lh5803 = &c->lh5803();
        p.bank = &c->bank();
        p.arbiter = &c->busArbiter();
        p.ce158 = c->ce158Card();
        p.ce1600p = c->ce1600pCard();
    } else if (PC1500Machine* c = pc1500Of(m)) {
        p.lh5801 = &c->cpu();
        p.ce158 = c->ce158Card();
    }
    return p;
}

void setYieldHook(Machine& m, std::function<void()> hook, std::uint64_t intervalTicks) {
    if (PC1600Machine* c = pc1600Of(m)) c->setYieldHook(std::move(hook), intervalTicks);
    else if (PC1500Machine* c = pc1500Of(m)) c->setYieldHook(std::move(hook), intervalTicks);
}

PresetMedia presetMedia(Machine& m, const PresetLoadResult& armed, const PresetSaveAsRequest& request) {
    if (PC1600Machine* c = pc1600Of(m)) return pc1600PresetMedia(*c, armed, request);
    PresetMedia media;
    if (PC1500Machine* c = pc1500Of(m))
        media.slots[0] = {c->expansionConnector().attachedCard(), armed.slot1ResolvedPath, armed.slot1RomFile};
    return media;
}

bool typeLine(Machine& m, const std::string& line, bool pressEnter, std::string* error) {
    if (PC1600Machine* c = pc1600Of(m)) return ::typeLine(*c, line, pressEnter, error);
    if (PC1500Machine* c = pc1500Of(m)) return ::typeLine(*c, line, pressEnter, error);
    return false;
}

} // namespace sharp::legacy
