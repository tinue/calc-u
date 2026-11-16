#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#include "Machine/SharpBuilder.hpp"
#include "machine/Catalogue.hpp"
#include "startup/ProductMachines.hpp"

// ── SharpProductMachines: what Calc-U-1600 hands LIB-18 ─────────────────
//
// LIB-18 design §10.2, §10.3 (spec AC-25 to AC-28): the catalogue with the
// module definitions, the builder, the built-in default of each model (its
// default variant, nothing plugged in), the Sharp pacing profile, the sound
// rate, the readiness bounds (7 s for the PC-1500 and PC-1500A, 27 s for the
// PC-1600), the host drive's ROM (bundled outside the ROM set, roms.md Q-1)
// and what a variant switch carries over: each plug's place, peripheral and
// variant, the host drive's folder and, until M-47, a module's card from the
// module bay (adapter "bays on machineChanged", LIB-6 §10.5).

namespace shared {

class SharpProductMachines final : public calcu::startup::ProductMachines {
public:
    // `catalogue` outlives this; `hostDriveRom` may be null (no host drive).
    SharpProductMachines(const calcu::machine::Catalogue& catalogue,
                         std::shared_ptr<const std::vector<std::uint8_t>> hostDriveRom);

    const calcu::machine::Catalogue& catalogue() const override { return catalogue_; }
    calcu::machine::MachineBuilder& builder() override { return builder_; }
    calcu::startup::MachinePlan builtInDefault(const calcu::machine::ModelId&) override;
    calcu::pacing::PacingProfile pacingProfile(const calcu::machine::ModelId&) const override;
    int soundRate(const calcu::machine::ModelId&) const override;
    calcu::machine::Seconds readinessBound(const calcu::machine::ModelId&) const override;
    calcu::machine::PeripheralRequest carried(const calcu::machine::PlugEntry&, calcu::machine::Machine&) override;
    void addRoms(calcu::machine::PeripheralRequest&) const override;

private:
    const calcu::machine::Catalogue& catalogue_;
    sharp::SharpMachineBuilder builder_;
    std::shared_ptr<const std::vector<std::uint8_t>> hostDriveRom_;
};

} // namespace shared
