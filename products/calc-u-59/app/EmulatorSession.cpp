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

EmulatorSession::EmulatorSession(calcu::machine::Machine& machine, const calcu::roms::RomSet& roms)
    : m_model(modelFrom(machine.model()).value_or(Model::TI59)), m_machine(machine), m_roms(roms) {
    refreshModule();
}

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

void EmulatorSession::refreshModule() {
    m_moduleImage.clear();
    const std::string id = moduleID();
    if (id.empty()) return;
    if (auto file = moduleRomFile(id)) m_moduleImage = cu59::moduleImage(m_roms, *file).value_or(std::vector<uint8_t>{});
}

bool EmulatorSession::printerControl(const calcu::machine::ControlId& id, bool on) {
    auto* c = calcu::machine::capabilityAs<calcu::peripherals::Controls>(m_machine, kControls);
    if (!c) return false;
    for (const auto& info : c->list()) {
        if (info.id != id) continue;
        if (info.form == calcu::machine::ControlForm::Switch) return c->set(id, on) == calcu::peripherals::ControlCall::Done;
        return (on ? c->press(id) : c->release(id)) == calcu::peripherals::ControlCall::Done;
    }
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
