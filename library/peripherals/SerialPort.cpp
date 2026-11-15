#include "peripherals/SerialPort.hpp"
#include <algorithm>

namespace calcu::peripherals {

std::size_t SerialPortQueues::takeSent(std::span<std::uint8_t> out) {
    const std::size_t n = std::min(out.size(), sent_.size());
    std::copy_n(sent_.begin(), n, out.begin());
    sent_.erase(sent_.begin(), sent_.begin() + static_cast<std::ptrdiff_t>(n));
    return n;
}

std::size_t SerialPortQueues::putReceived(std::span<const std::uint8_t> in) {
    const std::size_t room = kReceiveLimit - std::min(kReceiveLimit, received_.size());
    const std::size_t n = std::min(room, in.size());
    received_.insert(received_.end(), in.begin(), in.begin() + static_cast<std::ptrdiff_t>(n));
    return n;
}

bool SerialPortQueues::machineReceives(std::uint8_t& out) {
    if (received_.empty()) return false;
    out = received_.front();
    received_.pop_front();
    return true;
}

} // namespace calcu::peripherals
