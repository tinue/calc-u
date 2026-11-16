#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "../Connector/ExpansionCard.hpp"
#include "../Debug/ProgramLoader.hpp"
#include "../Job.hpp"
#include "sessions/Operation.hpp"
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
class Ce150Card;
class Ce158Card;
class CE1600PCard;

namespace sharp::legacy {

// ---- Build & Load (M-50, C1600-3 on C1600-2's loader) ----
// The listing is verified through the machine's debug targets (thread n:
// target n - 1).
debug::LoadResult loadProgram(calcu::machine::Machine&, const debug::LoadRequest&, debug::SourceMap&);

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
    Ce150Card* ce150 = nullptr;               // the CE-150's card, for the tools' event log (M-51)
};
CoreParts coreParts(calcu::machine::Machine&);
// The core's yield hook (PC1500Machine/PC1600Machine::setYieldHook): called
// inside a long run every `intervalTicks`, e.g. to drain the sound while a
// preset's single long run goes on; an empty function removes it.
void setYieldHook(calcu::machine::Machine&, std::function<void()> hook, std::uint64_t intervalTicks);
// The model's typer: types `line`, optionally ENTER (typeLine of
// PC1500BasicTyper / PC1600BasicTyper; M-36 for the app's typing).
bool typeLine(calcu::machine::Machine&, const std::string& line, bool pressEnter, std::string* error);

} // namespace sharp::legacy
