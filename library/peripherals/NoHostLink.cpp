// Compiled where there are no pseudo-terminals (CALCU_HAS_PTY unset:
// Windows, Emscripten). A serial port then has no host link and what the
// machine sends goes nowhere (LIB-2 EC-5).
#include "peripherals/HostLink.hpp"

namespace calcu::peripherals {

std::unique_ptr<HostLink> openHostLink(const std::optional<std::string>&, const std::string&) { return nullptr; }

} // namespace calcu::peripherals
