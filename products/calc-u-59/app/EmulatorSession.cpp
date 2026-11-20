#include "EmulatorSession.hpp"
#include "ModuleCatalog.hpp"
#include "ProductRoms.hpp"
#include "../core/TICatalogue.hpp"

namespace cu59 {

namespace {
const calcu::machine::PlacePath kSlot{{kTIModulePlace}};
const calcu::machine::PlacePath kPrinter{{kTIPrinterPlace}};
const calcu::machine::CapabilityId kControls{"printer/controls"};
} // namespace

EmulatorSession::EmulatorSession(calcu::machine::Machine& machine, const calcu::roms::RomSet&)
    : m_model(machine.model()), m_machine(machine) {}

std::string EmulatorSession::moduleID() const {
    for (const auto& e : m_machine.plugged().byPlace)
        if (e.place == kSlot) return e.peripheral.value;
    return {};
}

bool EmulatorSession::printerPlugged() const {
    for (const auto& e : m_machine.plugged().byPlace)
        if (e.place == kPrinter) return true;
    return false;
}

std::optional<bool> EmulatorSession::printerSwitch(const calcu::machine::ControlId& id) const {
    auto* c = calcu::machine::capabilityAs<calcu::peripherals::Controls>(m_machine, kControls);
    if (!c) return std::nullopt;
    for (const auto& info : c->list())
        if (info.id == id) return info.on;
    return std::nullopt;
}

} // namespace cu59
