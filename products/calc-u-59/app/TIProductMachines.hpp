#pragma once
#include "../core/TICatalogue.hpp"
#include "startup/ProductMachines.hpp"
#include <functional>
#include <memory>

// What Calc-U-59 hands the library's machine control and startup: the TI
// catalogue and builder, the built-in default of each model (the Master
// Library module in `slot-1`, no printer, as the TI-59 came out of the box),
// the TI pacing profile, no sound, the readiness bound of 25 s. A TI-58C's
// built-in default carries its constant memory as build content
// (ConstantMemoryKeeper::contentForBuild). A PC-100C attached gets TRACE at
// the position the app session keeps (through a detach and attach, like the
// real switch; TIAppPart).
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
    /// TRACE's kept position, put into a PC-100C attached; unset: off.
    void setKeptTrace(std::function<bool()> f) { m_keptTrace = std::move(f); }
    void attaching(calcu::machine::PeripheralRequest& request) override;

private:
    TIMachineBuilder m_builder;
    ContentForBuild m_constantMemory;
    std::function<bool()> m_keptTrace;
};

/// The module the built-in default puts into `slot-1`.
inline constexpr const char* kDefaultModule = "ML";

} // namespace cu59
