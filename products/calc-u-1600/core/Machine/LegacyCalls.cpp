#include "Machine/LegacyCalls.hpp"

#include "../PC1500/PC1500BasicTyper.hpp"
#include "../PC1600/PC1600BasicTyper.hpp"
#include "../PC1600/PC1600TypedInput.hpp"
#include "Machine/SharpCores.hpp"

using calcu::machine::Machine;

namespace sharp::legacy {

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

bool typeLine(Machine& m, const std::string& line, bool pressEnter, std::string* error) {
    if (PC1600Machine* c = pc1600Of(m)) return ::typeLine(*c, line, pressEnter, error);
    if (PC1500Machine* c = pc1500Of(m)) return ::typeLine(*c, line, pressEnter, error);
    return false;
}

} // namespace sharp::legacy
