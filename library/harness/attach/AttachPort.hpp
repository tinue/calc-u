#pragma once
#include <cstdint>
#include <span>
#include <string>

// The port of harness attach: the launch argument `--attach-port N`, else
// 59059. Several simulators share the Mac's loopback, so each needs a port of
// its own. Compiled always: the
// client (`calcu attach`) uses it too.

namespace calcu::harness {

inline constexpr std::uint16_t kDefaultAttachPort = 59059;

// "--attach-port N" (or "--attach-port=N") anywhere in `arguments`; a value
// that is no port (1 to 65535) is ignored.
std::uint16_t attachPort(std::span<const std::string> arguments);

} // namespace calcu::harness
