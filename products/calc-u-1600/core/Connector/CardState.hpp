#pragma once
#include <string>

#include "../Machine/StateRecords.hpp"
#include "ExpansionCard.hpp"

// One card's record of the machine state (C1600-5 AC-13): whether a card
// is there, then its own fields (CardBase::saveState). A state saved with
// a card where this machine has none, or the other way round, is refused.

namespace sharp {

template <class Io>
void cardStateIo(Io& io, const char (&tag)[5], CardBase* card) {
    io.begin(tag);
    bool present = card != nullptr;
    io(present);
    if constexpr (Io::kRestoring) {
        if (present != (card != nullptr))
            io.fail(std::string("the state was saved with another set of peripherals (") + tag + ")");
        else if (card)
            card->restoreState(io);
    } else {
        if (card) card->saveState(io);
    }
    io.end();
}

} // namespace sharp
