#include "peripherals/RealTimeClock.hpp"
#include <ctime>

namespace calcu::peripherals {

HostDateTime hostDateTime(std::chrono::system_clock::time_point t) {
    std::time_t now = std::chrono::system_clock::to_time_t(t);
    // to_time_t may round rather than truncate: take the sub-second phase
    // relative to the time_t it returned, borrowing a second if it rounded
    // up, so it stays in 0-999.
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t - std::chrono::system_clock::from_time_t(now)).count();
    if (ms < 0) {
        now -= 1;
        ms += 1000;
    }
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    return HostDateTime{local.tm_year + 1900, local.tm_mon + 1, local.tm_mday, local.tm_wday,
                        local.tm_hour,        local.tm_min,     local.tm_sec,  static_cast<int>(ms)};
}

namespace {

ClockFields fieldsOf(const HostDateTime& h) {
    return ClockFields{h.year, h.month, h.day, h.weekday, h.hour, h.minute, h.second};
}

} // namespace

void setClocksFromHost(machine::Machine& m, const HostDateTime& h) {
    const ClockFields f = fieldsOf(h);
    for (const auto& e : m.capabilities(RealTimeClock::kKind))
        if (auto* c = machine::capabilityAs<RealTimeClock>(m, e.id)) c->set(f, h.millisecond);
}

} // namespace calcu::peripherals
