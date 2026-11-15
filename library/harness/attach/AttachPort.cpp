#include "harness/attach/AttachPort.hpp"
#include <cstdlib>

namespace calcu::harness {

namespace {

bool parsePort(const std::string& s, std::uint16_t& out) {
    if (s.empty() || s.size() > 5) return false;
    long v = 0;
    for (char c : s) {
        if (c < '0' || c > '9') return false;
        v = v * 10 + (c - '0');
    }
    if (v < 1 || v > 65535) return false;
    out = static_cast<std::uint16_t>(v);
    return true;
}

} // namespace

std::uint16_t attachPort(std::span<const std::string> arguments) {
    static const std::string kOpt = "--attach-port";
    std::uint16_t port = kDefaultAttachPort;
    for (std::size_t i = 0; i < arguments.size(); ++i) {
        const std::string& a = arguments[i];
        std::uint16_t p = 0;
        if (a == kOpt && i + 1 < arguments.size() && parsePort(arguments[i + 1], p)) port = p;
        else if (a.rfind(kOpt + "=", 0) == 0 && parsePort(a.substr(kOpt.size() + 1), p)) port = p;
    }
    return port;
}

} // namespace calcu::harness
