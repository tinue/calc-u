#pragma once
#include "../core/TICatalogue.hpp"
#include "startup/ProductMachines.hpp"
#include <functional>
#include <memory>

// What Calc-U-59 hands LIB-18 (LIB-18 design §11.2, §11.3; spec AC-17 to
// AC-19, D-25): the TI catalogue and builder, the built-in default of each
// model (the Master Library module in `slot-1`, no printer: ADR 0021, row
// 59-10), the TI pacing profile, no sound (LIB-3 AC-33), the readiness bound
// of 25 s (D-30). A TI-58C's built-in default carries its constant memory as
// build content (AC-19; C59-3 AC-7: ConstantMemoryKeeper::contentForBuild).
// The app layer and the harness entry use the same class.

namespace cu59 {

class TIProductMachines final : public calcu::startup::ProductMachines {
public:
    using ContentForBuild = std::function<std::shared_ptr<const calcu::machine::ProductData>()>;
    /// `constantMemory`: what a TI-58C built without a preset gets; unset: zeroed.
    explicit TIProductMachines(ContentForBuild constantMemory = {}) : m_constantMemory(std::move(constantMemory)) {}

    const calcu::machine::Catalogue& catalogue() const override { return tiCatalogue(); }
    calcu::machine::MachineBuilder& builder() override { return m_builder; }
    calcu::startup::MachinePlan builtInDefault(const calcu::machine::ModelId&) override;
    calcu::pacing::PacingProfile pacingProfile(const calcu::machine::ModelId&) const override;
    int soundRate(const calcu::machine::ModelId&) const override { return 0; }
    calcu::machine::Seconds readinessBound(const calcu::machine::ModelId&) const override {
        return calcu::machine::Seconds(25.0);
    }
    void setConstantMemory(ContentForBuild f) { m_constantMemory = std::move(f); }

private:
    TIMachineBuilder m_builder;
    ContentForBuild m_constantMemory;
};

/// The module the built-in default puts into `slot-1` (ADR 0021).
inline constexpr const char* kDefaultModule = "ML";

} // namespace cu59
