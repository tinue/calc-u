#pragma once
#include "../core/TIAdapters.hpp"
#include "MachineModelInfo.hpp"
#include "StateFile.hpp"
#include <optional>
#include <string>

// Writes a parsed state file into a machine: partition, program and
// registers (see reference/SharedArchitecture.md § "State File Loading"). The
// frontend owns everything around it — stopping its run loop first, model
// switches, module and printer changes, the cue card, KEYSTROKES playback.

namespace cu59 {

/// Why a file can't be applied to `model`, or nullopt. Only an explicit
/// partition beyond 479 on the 60-register models is rejected.
std::optional<std::string> stateFilePartitionError(const LoadStateResult& r, Model model);

/// Full load: reset, run the ROM's power-on routine to idle, set the
/// partition (default 239 on the TI-58/58C when the file gives none), clear
/// RAM (keeping the TI-58C's extra registers, whose RAM[60] the ROM needs),
/// write the program zero-padded to the model's physical capacity, then the
/// registers. Run on the emulation thread with the run loop stopped.
void applyState(calcu::machine::Machine& m, const LoadStateResult& r, Model model);

/// A reload during KEYSTROKES playback, as if the user keyed the changes in:
/// the partition if the stage sets one, then only the listed program steps
/// and registers — everything else stays.
void applyReload(calcu::machine::Machine& m, const ReloadStage& stage, Model model);

/// A fresh machine's power-on. TI-58C: run the ROM's startup once, then
/// restore its constant memory from `savedRam` (if given), so startup code
/// can't overwrite the restored registers. Other models: nothing to do.
/// 2.3.0's order, kept until C59-3 hands the constant memory at build (M-54,
/// ledger row 59-15).
void powerOn(calcu::machine::Machine& m, Model model, const std::vector<uint8_t>* savedRam);

// The Reset button and Clean are Machine::reset() and coldBoot() (C59-1
// AC-6): they return at once and are over when the machine reports key
// readiness again.

} // namespace cu59
