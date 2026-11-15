#include "peripherals/PeripheralsPart.hpp"
#include "peripherals/Controls.hpp"
#include "peripherals/RealTimeClock.hpp"
#include "sessions/Args.hpp"
#include "sessions/LibraryTopics.hpp"
#include <cctype>

namespace calcu::peripherals {

namespace {
std::string lower(std::string_view s) {
    std::string out(s);
    for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}
} // namespace

PeripheralsPart::PeripheralsPart(HostLinkRules rules, std::optional<std::string> folder, HostLinkFactory factory)
    : links_(std::move(rules), std::move(folder), std::move(factory)) {}

void PeripheralsPart::attach(sessions::SessionSetup& s) {
    s.command(kSetControlCommand, [](const sessions::Value& args, sessions::SessionContext& c) -> sessions::CommandOutcome {
        machine::Machine* m = c.machine();
        if (!m) return sessions::Refused{sessions::kNoMachine};
        const std::string peripheral = sessions::textArg(args, "peripheral");
        const std::string place = sessions::textArg(args, "place");
        const std::string control = sessions::textArg(args, "control");
        const bool on = sessions::flagArg(args, "on");
        // The plugged peripheral by its ID in any case, at the place when one is named.
        const machine::PlugEntry* at = nullptr;
        const machine::Plugged plugged = m->plugged();
        for (const auto& e : plugged.byPlace)
            if (lower(e.peripheral.value) == lower(peripheral) && (place.empty() || machine::placeText(e.place) == place)) {
                at = &e;
                break;
            }
        if (!at) return sessions::Refused{"No " + peripheral + " is plugged in."};
        Controls* controls = controlsCapability(*m, machine::PeripheralOwner{at->peripheral, at->place});
        if (!controls) return sessions::Refused{"The " + at->peripheral.value + " has no controls."};
        for (const auto& info : controls->list()) {
            if (lower(info.id.value) != lower(control)) continue;
            const ControlCall r = info.form == machine::ControlForm::Switch ? controls->set(info.id, on)
                                  : on                                       ? controls->press(info.id)
                                                                             : controls->release(info.id);
            if (r == ControlCall::Done) return sessions::Done{};
            break;
        }
        return sessions::Refused{"The " + at->peripheral.value + " has no control " + control + "."};   // EC-6
    });
}

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
