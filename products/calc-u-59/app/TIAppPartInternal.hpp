#pragma once
#include "TIAppPart.hpp"
#include "../core/TIAdapters.hpp"

// The nested helper of cu59::TIAppPart, shared by TIAppPart.cpp (the session
// plumbing) and TIAppPartFlows.cpp (the product's flows). Not for other users.

namespace cu59 {

// TICardWaitAdapter: the TI-59 needs a card now (LIB-17 AC-16).
struct TIAppPart::Hooks final : calcu::sessions::WaitProbe {
    TIAppPart& part;
    explicit Hooks(TIAppPart& p) : part(p) {}
    bool waitsForMedium(const calcu::machine::Machine& m) const override {
        const auto* r = cardReaderOf(const_cast<calcu::machine::Machine&>(m));
        return r && r->isInCardWait() && r->isWaitingForCard();   // the idle loop polls the switch too
    }
};

} // namespace cu59
