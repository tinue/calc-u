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

// Machine control: builds a plan into the session, changes a
// plug and runs what follows, runs the machine to key readiness, carries the
// plugs over to another variant, and keeps the last build. Single-threaded,
// on the session's thread; a machine changes only inside an operation.

namespace calcu::startup {

class MachineControl {
public:
    MachineControl(ProductMachines&, std::shared_ptr<const roms::RomSet>);

    // Fills the ROM images from the RomSet, builds through
    // machine::build(catalogue, builder, request) and hands
    // the machine to the machine session with the model's profile and sound
    // rate; the old machine is destroyed with its memory. Before that:
    // MachineSession::dropInput() and the commanded speed back to regular
    // (full speed does not carry over to a new machine; outside an operation
    // at once, else when it ends). A build that is refused changes nothing and gives the refusal.
    std::optional<BuildRefusal> build(sessions::SessionContext&, MachinePlan, BuildOrigin);
    // The plan and origin of the last build that succeeded.
    const std::optional<BuiltWith>& lastBuild() const { return lastBuild_; }
    // The request for a peripheral by place, ID and variant, from the
    // catalogue (no content: the peripheral's default), with what the product
    // adds to an attach (ProductMachines::attaching).
    machine::PeripheralRequest requestFor(const machine::PlacePath&, const machine::PeripheralId&,
                                          std::optional<machine::VariantId>) const;
    // The refusal the change would get, without it (canAttach,
    // Machine::canReplace); asked before an operation starts.
    std::optional<BuildRefusal> check(const machine::Machine&, const PlugChange&) const;
    // The change and what follows, inside an operation.
    // A change the machine refuses (not asked with check() first) fails the
    // operation with the machine's reason.
    sessions::Task<void> change(sessions::OperationContext&, PlugChange);
    // Runs pieces (OperationContext::runPiece; no finer wait)
    // until the machine reports key readiness, or until readinessBound(model)
    // of emulated time has run; then one log line, and the machine is left
    // as it is. True when it is ready.
    sessions::Task<bool> untilKeyReady(sessions::OperationContext&);
    // The plan of the running machine in another variant: carried()
    // for each plug of Machine::plugged() in place order; no content.
    MachinePlan variantPlan(machine::Machine&, machine::VariantId);
    // What a part (the media part) adds to the request
    // of a plug variantPlan carries over (a module medium's content), after
    // ProductMachines::carried, in the order added.
    using Carry = std::function<void(machine::PeripheralRequest&, const machine::PlugEntry&, machine::Machine&)>;
    void addCarry(Carry c) { carries_.push_back(std::move(c)); }

    ProductMachines& product() { return product_; }
    bool hasRoms() const { return roms_ != nullptr; }
    // A build inside an operation asked for regular speed at its end.
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
