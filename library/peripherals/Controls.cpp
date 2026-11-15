#include "peripherals/Controls.hpp"

namespace calcu::peripherals {

ControlTable::ControlTable(std::vector<machine::ControlDescriptor> controls, OnChange onChange)
    : onChange_(std::move(onChange)) {
    for (const auto& c : controls) controls_.push_back(ControlInfo{c.id, c.form, false});
}

ControlCall ControlTable::change(const machine::ControlId& id, machine::ControlForm form, bool on) {
    for (auto& c : controls_) {
        if (c.id != id) continue;
        if (c.form != form) return ControlCall::WrongForm;
        if (c.on != on) {
            c.on = on;
            if (onChange_) onChange_(id, on);
        }
        return ControlCall::Done;
    }
    return ControlCall::UnknownControl;
}

void ControlTable::restore(const std::vector<machine::SwitchPosition>& positions) {
    for (const auto& p : positions)
        for (auto& c : controls_)
            if (c.id == p.id && c.form == machine::ControlForm::Switch) c.on = p.on;
}

bool ControlTable::on(const machine::ControlId& id) const {
    for (const auto& c : controls_)
        if (c.id == id) return c.on;
    return false;
}

Controls* controlsCapability(machine::Machine& m, const machine::Owner& owner) {
    for (const auto& e : m.capabilities(Controls::kKind))
        if (e.owner == owner) return machine::capabilityAs<Controls>(m, e.id);
    return nullptr;
}

std::vector<ControlInfo> controlsOf(machine::Machine& m, const machine::Owner& owner) {
    Controls* c = controlsCapability(m, owner);
    return c ? c->list() : std::vector<ControlInfo>{};
}

} // namespace calcu::peripherals
