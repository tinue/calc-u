#pragma once
#include "machine/Machine.hpp"
#include "roms/RomSet.hpp"
#include "sessions/Operation.hpp"
#include "sessions/SessionPart.hpp"
#include "startup/MachinePlan.hpp"
#include "startup/ProductMachines.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <vector>

// Machine control (LIB-18 design §3.4; spec AC-1, AC-2, AC-7, AC-10 to
// AC-14, EC-3 to EC-5, EC-7): builds a plan into the session, changes a
// plug and runs what follows, runs the machine to key readiness, carries the
// plugs over to another variant, and keeps the last build. Single-threaded,
// on the session's thread; a machine changes only inside an operation.

namespace calcu::startup {

class MachineControl {
public:
    MachineControl(ProductMachines&, std::shared_ptr<const roms::RomSet>);

    // AC-1, AC-2: fills the ROM images from the RomSet, builds through
    // machine::build(catalogue, builder, request) and hands
    // the machine to the machine session with the model's profile and sound
    // rate; the old machine is destroyed with its memory. Before that:
    // MachineSession::dropInput() (AC-2, EC-4) and the commanded speed back
    // to regular (AC-2, D-11; outside an operation at once, else when it
    // ends). A build that is refused changes nothing and gives the refusal.
    std::optional<BuildRefusal> build(sessions::SessionContext&, MachinePlan, BuildOrigin);
    // The plan and origin of the last build that succeeded.
    const std::optional<BuiltWith>& lastBuild() const { return lastBuild_; }
    // AC-10: the request for a peripheral by place, ID and variant, from the
    // catalogue (no content: the peripheral's default).
    machine::PeripheralRequest requestFor(const machine::PlacePath&, const machine::PeripheralId&,
                                          std::optional<machine::VariantId>) const;
    // AC-12: the refusal the change would get, without it (canAttach,
    // Machine::canReplace); asked before an operation starts.
    std::optional<BuildRefusal> check(const machine::Machine&, const PlugChange&) const;
    // AC-10, AC-11, EC-5: the change and what follows, inside an operation.
    // A change the machine refuses (not asked with check() first) fails the
    // operation with the machine's reason.
    sessions::Task<void> change(sessions::OperationContext&, PlugChange);
    // AC-1, AC-10, AC-13, EC-3, EC-7: run pieces (OperationContext::runPiece)
    // until the machine reports key readiness, or until readinessBound(model)
    // of emulated time has run; then one log line, and the machine is left
    // as it is (D-4, D-5). True when it is ready.
    sessions::Task<bool> untilKeyReady(sessions::OperationContext&);
    // AC-7: the plan of the running machine in another variant: carried()
    // for each plug of Machine::plugged() in place order; no content.
    MachinePlan variantPlan(machine::Machine&, machine::VariantId);
    // LIB-7 (M-41; LIB-18 §3.4, LIB-7 S-7a): what a part adds to the request
    // of a plug variantPlan carries over (a module medium's content), after
    // ProductMachines::carried, in the order added.
    using Carry = std::function<void(machine::PeripheralRequest&, const machine::PlugEntry&, machine::Machine&)>;
    void addCarry(Carry c) { carries_.push_back(std::move(c)); }

    ProductMachines& product() { return product_; }
    bool hasRoms() const { return roms_ != nullptr; }
    // D-11: a build inside an operation asked for regular speed at its end.
    bool takeRegularSpeedDue() {
        const bool due = regularDue_;
        regularDue_ = false;
        return due;
    }

private:
    machine::PeripheralRequest withRoms(machine::PeripheralRequest) const;

    ProductMachines& product_;
    std::shared_ptr<const roms::RomSet> roms_;
    std::optional<BuiltWith> lastBuild_;
    bool regularDue_ = false;
    std::vector<Carry> carries_;
};

} // namespace calcu::startup
