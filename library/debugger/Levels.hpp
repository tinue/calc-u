#pragma once
#include "machine/DebugTarget.hpp"
#include "machine/Ids.hpp"
#include "machine/Machine.hpp"

// Levels: the product's rules for pause, step and pause-on-start. Every
// target has the level `cpu`; a product may declare `program`. The library names
// no processor and no model: what lands where, what one `program` step is
// and when a start fires come from here, reading InstructionEvent::marks.

namespace calcu::debugger {

using LevelName = machine::Id<struct LevelTag>;
inline const LevelName kCpu{"cpu"};
inline const LevelName kProgram{"program"};

enum class RestartKind { Reset, ColdBoot, Build };   // the session's Restart plus a new machine

class LevelRules {
public:
    virtual ~LevelRules() = default;
    virtual bool hasProgramLevel() const { return false; }
    bool hasLevel(const LevelName& l) const { return l == kCpu || (l == kProgram && hasProgramLevel()); }
    // A pause of the level lands at once, or after the instruction for which
    // pauseLandsAfter says so.
    virtual bool pauseLandsAtOnce(const LevelName&) const { return true; }
    virtual bool pauseLandsAfter(const LevelName&, const machine::InstructionEvent&) { return true; }
    virtual void pauseAsked(const LevelName&, machine::Machine&) {}   // e.g. the TI's count of 50000 starts
    // One step of `program` has been carried out after this instruction.
    virtual bool programStepDoneAfter(const machine::InstructionEvent&) { return true; }
    // A `cpu` step carries out an instruction that stays at its address
    // again, up to this many times (the TI: 32).
    virtual int cpuStepRepeats() const { return 0; }
    // Pause-on-start. The default for `cpu`: the first instruction
    // after which the machine is not key ready, once it was seen key ready
    // since arming; isKeyReady() is asked only while a start is armed. The
    // default for `program`: never.
    virtual void startArmed(const LevelName&, machine::Machine&);
    virtual bool startFiresAfter(const LevelName&, const machine::InstructionEvent&, const machine::Machine&);
    virtual bool startFiresAtRestart(const LevelName&, RestartKind) const { return false; }

private:
    bool seenReady_ = false;
};

// The Sharp machines' and the test machines' default: `cpu` only.
class CpuOnlyRules final : public LevelRules {};

} // namespace calcu::debugger
