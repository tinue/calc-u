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
#include "../KeyPaste.hpp"
#include "../MachineCodeFile.hpp"
#include "../Preset/PresetRunner.hpp"
#include "machine/Machine.hpp"

// Adapters (C1600-1 design §5): core functions on calcu::machine::Machine&
// for the callers whose rows come later. Each reaches the old class inside
// the core (SharpCores.hpp) and goes in the step named. No file outside the
// core needs a machine class for them; this header includes none.

namespace sharp::legacy {

// ---- Typing support (M-36, LIB-5 AC-35) ----
// The PC-1600's SHIFT and KBII latches; none on the PC-1500.
KeyLatches readLatches(calcu::machine::Machine&);

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
                                           std::vector<std::string> romDirs, std::vector<std::string> extraModuleDirs,
                                           PresetArmedFn onArmed, PresetSaveAsFn onSaveAs);

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

} // namespace sharp::legacy
