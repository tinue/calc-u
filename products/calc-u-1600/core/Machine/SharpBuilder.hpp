#pragma once
#include "machine/Build.hpp"

// Builds the Sharp machines (C1600-1 design §3.1): calcu::machine::build(
// sharpCatalogue(), builder, request). The machine gets the ROM images the
// descriptor names, then the request's peripherals in request order (the
// order on the 60-pin chain, AC-17). It runs nothing: the caller makes the
// first operation (a cold boot). A sound rate other than 48,000 is refused:
// both cores fix it (AC-14, design D-6).

namespace sharp {

inline constexpr int kSoundRate = 48000;

class SharpMachineBuilder final : public calcu::machine::MachineBuilder {
public:
    calcu::machine::BuildResult build(const calcu::machine::BuildRequest&, const calcu::machine::Catalogue&) override;
};

} // namespace sharp
