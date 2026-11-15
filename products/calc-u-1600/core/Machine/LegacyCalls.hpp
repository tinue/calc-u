#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "../Basic/BasicLoadResults.hpp"
#include "../Connector/ExpansionCard.hpp"
#include "../Debug/ProgramLoader.hpp"
#include "../Job.hpp"
#include "../MachineCodeFile.hpp"
#include "../Preset/PresetRunner.hpp"
#include "machine/Machine.hpp"

// Adapters (C1600-1 design §5): core functions on calcu::machine::Machine&
// for the callers whose rows come later. Each reaches the old class inside
// the core (SharpCores.hpp) and goes in the step named. No file outside the
// core needs a machine class for them; this header includes none.

class LH5801;
class LH5803;
class SC7852;
class PC1600Bank;
class PC1600BusArbiter;
class Ce158Card;
class CE1600PCard;
// Not included: PresetMedia.hpp has a member `slots`, which Qt's macro of
// that name breaks in the Qt frontend's sources that include this header.
struct PresetMedia;

namespace sharp::legacy {

// ---- Loaders (M-44, C1600-2) ----
BasicLoadResult loadBasicProgram(calcu::machine::Machine&, const std::vector<std::uint8_t>& file);
// PC-1600: `slot`, `bank` as loadPC1600MachineCode(); the PC-1500 ignores them.
bool loadMachineCode(calcu::machine::Machine&, int slot, std::uint32_t addr, const std::uint8_t* data, std::size_t len,
                     std::string* error, int bank);
machinecode::PC1600State machineCodeLoadState(calcu::machine::Machine&);   // PC-1600 only; {} otherwise
// The PC-1500's user RAM; false on the PC-1600.
bool userRam(calcu::machine::Machine&, std::uint32_t* start, std::uint32_t* end);

// ---- Debugger (M-45, LIB-10 1600) ----
std::unique_ptr<debug::DebugTarget> makeDebugTarget(calcu::machine::Machine&);
debug::LoadResult loadProgram(calcu::machine::Machine&, const debug::LoadRequest&, debug::SourceMap&,
                              const debug::DebugTarget&);

// ---- Presets (M-46, LIB-6 1600 and C1600-4) ----
// The model's preset loader on a machine just built for the preset; what it
// plugs in is read back into plugged() when it is done.
job::Task<PresetLoadResult> applyPresetJob(calcu::machine::Machine&, PresetFile preset, PresetLogFn log,
                                           std::string traceDir, std::string moduleDir, PresetBootedFn onBooted,
                                           PresetRoms presetRoms, std::vector<std::string> extraModuleDirs,
                                           PresetArmedFn onArmed, PresetSaveAsFn onSaveAs);
// What the preset loader plugged in so far, read back into plugged() (from
// its onArmed, so that the armed machine's peripherals -- a plotter's paper
// -- are seen while the preset still runs; LIB-3 AC-35, M-31).
void syncPlugged(calcu::machine::Machine&);

// ---- Running to the prompt (M-46, LIB-18 1600) ----
// A power cycle under way runs to its end, a phase at a time; then the
// model's boot run (runBootToPromptJob of both typers).
job::Task<void> runToPromptJob(calcu::machine::Machine&);
void runToPrompt(calcu::machine::Machine&);

// ---- Media (M-47, LIB-7 1600) ----
// The card in a module slot (from 1), or nullptr.
const ExpansionCard* moduleIn(calcu::machine::Machine&, int slot);
// `f` runs at the off point of the coming power cycle, after the changes
// that wait for it (an attach on a running machine, design D-2), or now if
// none waits: the disk goes into a CE-1600F plugged in then, as today.
void atOffPoint(calcu::machine::Machine&, std::function<void()> f);

// ---- Today's tools (M-51: pc1500_cli, pc1600_cli, the probes and the
// loader matrices go; C1600-1 design §7) ----
// The parts of the core the tools read per instruction: the CPUs, the
// PC-1600's banks and bus arbiter, the CE-158 card. Null where the model
// has none (or the CE-158 is not plugged in).
struct CoreParts {
    LH5801* lh5801 = nullptr;                 // PC-1500, PC-1500A
    SC7852* sc7852 = nullptr;                 // PC-1600
    LH5803* lh5803 = nullptr;                 // PC-1600
    const PC1600Bank* bank = nullptr;         // PC-1600
    const PC1600BusArbiter* arbiter = nullptr;   // PC-1600
    Ce158Card* ce158 = nullptr;
    CE1600PCard* ce1600p = nullptr;           // PC-1600, also during a preset load
};
CoreParts coreParts(calcu::machine::Machine&);
// The core's yield hook (PC1500Machine/PC1600Machine::setYieldHook): called
// inside a long run every `intervalTicks`, e.g. to drain the sound while a
// preset's single long run goes on; an empty function removes it.
void setYieldHook(calcu::machine::Machine&, std::function<void()> hook, std::uint64_t intervalTicks);
// What a preset's `saveas:` saves from the machine (savePresetMedia()).
PresetMedia presetMedia(calcu::machine::Machine&, const PresetLoadResult& armed, const PresetSaveAsRequest& request);
// The model's typer: types `line`, optionally ENTER (typeLine of
// PC1500BasicTyper / PC1600BasicTyper; M-36 for the app's typing).
bool typeLine(calcu::machine::Machine&, const std::string& line, bool pressEnter, std::string* error);

} // namespace sharp::legacy
