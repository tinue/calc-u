#include "peripherals/PeripheralsPart.hpp"
#include "peripherals/RealTimeClock.hpp"

namespace calcu::peripherals {

PeripheralsPart::PeripheralsPart(HostLinkRules rules, std::optional<std::string> folder, HostLinkFactory factory)
    : links_(std::move(rules), std::move(folder), std::move(factory)) {}

void PeripheralsPart::pump(sessions::SessionContext& c) {
    if (machine::Machine* m = c.machine()) links_.pump(*m);
    else links_.machineGone();
}

void PeripheralsPart::beforeCall(sessions::SessionContext& c, pacing::WallTime) { pump(c); }

void PeripheralsPart::afterCall(sessions::SessionContext& c, pacing::WallTime) { pump(c); }

void PeripheralsPart::machineChanged(sessions::SessionContext& c) {
    machine::Machine* m = c.machine();
    if (!m) {
        links_.machineGone();
        return;
    }
    links_.machineChanged(*m);
    setClocksFromHost(*m, hostDateTime(c.hostTime()));
}

void PeripheralsPart::operationEnded(sessions::SessionContext& c) {
    if (machine::Machine* m = c.machine()) setClocksFromHost(*m, hostDateTime(c.hostTime()));
}

} // namespace calcu::peripherals
