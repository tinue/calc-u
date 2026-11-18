#pragma once
#include "machine/Build.hpp"

// Builds the Sharp machines: calcu::machine::build(
// sharpCatalogue(), builder, request). The machine gets the ROM images the
// descriptor names, then the request's peripherals in request order (the
// order on the 60-pin chain), the bus ROMs last. It
// runs nothing and leaves the machine as a cold boot does. A sound rate other than 48,000 is refused:
// both cores fix it.

namespace sharp {

inline constexpr int kSoundRate = 48000;

class SharpMachineBuilder final : public calcu::machine::MachineBuilder {
public:
    calcu::machine::BuildResult build(const calcu::machine::BuildRequest&, const calcu::machine::Catalogue&) override;
};

} // namespace sharp
