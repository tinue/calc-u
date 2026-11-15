#include "Machine/LegacyCalls.hpp"

#include "../Debug/MachineDebugTargets.hpp"
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

KeyLatches readLatches(Machine& m) {
    if (PC1600Machine* p = pc1600Of(m)) return pc1600ReadLatches(*p);
    return KeyLatches{};
}

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
                                           std::vector<std::string> romDirs, std::vector<std::string> extraModuleDirs,
                                           PresetArmedFn onArmed, PresetSaveAsFn onSaveAs) {
    PresetLoadResult result;
    if (PC1600Machine* p = pc1600Of(m)) {
        static_cast<PC1600InterfaceMachine&>(m).notePresetCe1600pVariant(
            calcu::machine::VariantId{preset.ce1600pRomVariant == "old" ? "Old" : "New"});
        result = co_await applyPC1600PresetJob(*p, std::move(preset), std::move(log), std::move(traceDir),
                                               std::move(moduleDir), std::move(onBooted), std::move(romDirs),
                                               std::move(extraModuleDirs), std::move(onArmed), std::move(onSaveAs));
    } else if (PC1500Machine* p = pc1500Of(m)) {
        result = co_await applyPC1500PresetJob(*p, std::move(preset), std::move(log), std::move(traceDir),
                                               std::move(moduleDir), std::move(onBooted), std::move(romDirs),
                                               std::move(extraModuleDirs), std::move(onArmed), std::move(onSaveAs));
    } else {
        result.error = "not a Sharp machine";
    }
    if (SharpMachine* s = sharpOf(m)) s->syncFromCore();
    co_return result;
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
    if (PC1600Machine* p = pc1600Of(m)) return slot == 1 || slot == 2 ? p->memory().slotCard(slot) : nullptr;
    if (PC1500Machine* p = pc1500Of(m)) return slot == 1 ? p->memory().expansionConnector().attachedCard() : nullptr;
    return nullptr;
}

void atOffPoint(Machine& m, std::function<void()> f) {
    if (SharpMachine* s = sharpOf(m)) s->atOffPoint(std::move(f));
    else f();
}

} // namespace sharp::legacy
