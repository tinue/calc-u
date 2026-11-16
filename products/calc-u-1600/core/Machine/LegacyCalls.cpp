#include "Machine/LegacyCalls.hpp"

#include "../Debug/MachineDebugTargets.hpp"
#include "../PC1600/PC1600PresetMedia.hpp"
#include "../PC1500/PC1500BasicLoader.hpp"
#include "../PC1500/PC1500BasicTyper.hpp"
#include "../PC1500/PC1500MachineCodeLoader.hpp"
#include "../PC1500/PC1500PresetLoader.hpp"
#include "../PC1600/PC1600BasicLoader.hpp"
#include "../PC1600/PC1600BasicTyper.hpp"
#include "../PC1600/PC1600MachineCodeLoader.hpp"
#include "../PC1600/PC1600PresetLoader.hpp"
#include "../PC1600/PC1600TypedInput.hpp"
#include "Machine/SharpCores.hpp"

using calcu::machine::Machine;

namespace sharp::legacy {

BasicLoadResult loadBasicProgram(Machine& m, const std::vector<std::uint8_t>& file) {
    if (PC1600Machine* p = pc1600Of(m)) return ::loadBasicProgram(*p, file);
    if (PC1500Machine* p = pc1500Of(m)) return ::loadBasicProgram(*p, file);
    BasicLoadResult r;
    r.error = "No machine is running.";
    return r;
}

bool loadMachineCode(Machine& m, int slot, std::uint32_t addr, const std::uint8_t* data, std::size_t len,
                     std::string* error, int bank) {
    if (PC1600Machine* p = pc1600Of(m)) return loadPC1600MachineCode(*p, slot, addr, data, len, error, bank);
    if (PC1500Machine* p = pc1500Of(m)) return loadPC1500MachineCode(*p, addr, data, len, error);
    return false;
}

machinecode::PC1600State machineCodeLoadState(Machine& m) {
    if (PC1600Machine* p = pc1600Of(m)) return pc1600LoadState(*p);
    return {};
}

bool userRam(Machine& m, std::uint32_t* start, std::uint32_t* end) {
    PC1500Machine* p = pc1500Of(m);
    if (!p) return false;
    pc1500UserRam(*p, start, end);
    return true;
}

std::unique_ptr<debug::DebugTarget> makeDebugTarget(Machine& m) {
    if (PC1600Machine* p = pc1600Of(m)) return debug::makeDebugTarget(*p);
    if (PC1500Machine* p = pc1500Of(m)) return debug::makeDebugTarget(*p);
    return nullptr;
}

debug::LoadResult loadProgram(Machine& m, const debug::LoadRequest& request, debug::SourceMap& map,
                              const debug::DebugTarget& target) {
    return debug::loadProgram(pc1500Of(m), pc1600Of(m), request, map, target);
}

job::Task<PresetLoadResult> applyPresetJob(Machine& m, PresetFile preset, PresetLogFn log, std::string traceDir,
                                           std::string moduleDir, PresetBootedFn onBooted,
                                           PresetRoms presetRoms, std::vector<std::string> extraModuleDirs,
                                           PresetArmedFn onArmed, PresetSaveAsFn onSaveAs) {
    PresetLoadResult result;
    if (PC1600Machine* p = pc1600Of(m)) {
        static_cast<PC1600InterfaceMachine&>(m).notePresetCe1600pVariant(
            calcu::machine::VariantId{preset.ce1600pRomVariant == "old" ? "Old" : "New"});
        result = co_await applyPC1600PresetJob(*p, std::move(preset), std::move(log), std::move(traceDir),
                                               std::move(moduleDir), std::move(onBooted), std::move(presetRoms),
                                               std::move(extraModuleDirs), std::move(onArmed), std::move(onSaveAs));
    } else if (PC1500Machine* p = pc1500Of(m)) {
        result = co_await applyPC1500PresetJob(*p, std::move(preset), std::move(log), std::move(traceDir),
                                               std::move(moduleDir), std::move(onBooted), std::move(presetRoms),
                                               std::move(extraModuleDirs), std::move(onArmed), std::move(onSaveAs));
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
        p.ce150 = c->ce150Card();
    } else if (PC1500Machine* c = pc1500Of(m)) {
        p.lh5801 = &c->cpu();
        p.ce158 = c->ce158Card();
        p.ce150 = c->ce150Card();
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
