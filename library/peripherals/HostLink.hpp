#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>

// A host link (see the glossary):
// what connects a machine's serial port to the user's computer. On macOS and
// Linux it is a pseudo-terminal whose serial port file, a symbolic link with
// a fixed name in the folder for serial links, any serial program
// can open with any settings (PtyHostLink.cpp). Elsewhere there is none (NoHostLink.cpp).
//
// Threads: a pseudo-terminal link runs a reader and a writer
// thread of its own; its rings are the only place that synchronizes. The
// members below are called from the session's thread.

namespace calcu::peripherals {

class HostLink {
public:
    virtual ~HostLink() = default;
    // What the program on the user's computer sent, oldest first; how many.
    virtual std::size_t read(std::span<std::uint8_t> out) = 0;
    // Never blocks; dropped while no program reads.
    virtual void write(std::span<const std::uint8_t> in) = 0;
    virtual bool peerOpen() const = 0;   // a program has the file open (→ PeerLines::dcd)
    // The serial port file, or the raw terminal device when none could be
    // made (no folder).
    virtual std::string path() const = 0;
    // The serial port file into another folder; true when
    // it was made there.
    virtual bool moveTo(const std::optional<std::string>& folder) = 0;
};

// A pseudo-terminal with its serial port file `<folder>/<fileName>`; without
// a folder, only the raw device. nullptr where there are no pseudo-terminals
// (Windows, the Web; CALCU_HAS_PTY unset) or when none could be opened.
std::unique_ptr<HostLink> openHostLink(const std::optional<std::string>& folder, const std::string& fileName);

} // namespace calcu::peripherals
