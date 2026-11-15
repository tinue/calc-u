#pragma once
#include <cstdint>
#include <span>
#include <string>

// The port of harness attach (C59-5 AC-10; design §3.2): the launch argument
// `--attach-port N`, else 59059. Several simulators share the Mac's loopback,
// so each needs a port of its own (first step 4). Compiled always: the
// client (`calcu attach`) uses it too.

namespace calcu::harness {

inline constexpr std::uint16_t kDefaultAttachPort = 59059;   // C59-5 decision log, 2026-11-14

// "--attach-port N" (or "--attach-port=N") anywhere in `arguments`; a value
// that is no port (1 to 65535) is ignored.
std::uint16_t attachPort(std::span<const std::string> arguments);

} // namespace calcu::harness
