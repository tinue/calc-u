#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "machine/Machine.hpp"
#include "peripherals/SerialPort.hpp"
#include "sessions/MachineSession.hpp"
#include "sessions/SessionPart.hpp"

// The serial ports of `pc1600 serial <port> …` (C1600-5 AC-7 to AC-9, EC-3;
// design §3.4, D-4): per port (`ce158`, the CE-158's; `rs232c`, the
// PC-1600's own) a feed, a capture file and what the report counts. Today's
// Ce158CliPeer (tools/Ce158CliPeer.hpp at the pin) pumped between slices of
// its own; this part paces the feed in the machine session's slice hook, in
// emulated ticks, so a run repeats exactly (LIB-11 AC-6).
//
// - feed: the file's bytes are offered to the port one in each character
//   time (ten bits at the speed the port is set to, 300 baud while it has
//   none), after `hold` character times of silence from the feed; a second
//   feed takes the place of what was left.
// - The bytes the port sends are taken out of it at every host call (and
//   before the machine goes), unless the port has a host link (link), which
//   takes them then: they go to the capture file, if one is named, and are
//   kept for the report from the machine's build on.
// Single-threaded, on the session's thread. The harness's part: a frontend
// has its ports' host links instead.

namespace shared {

class SerialScriptsPart final : public calcu::sessions::SessionPart, public calcu::sessions::SliceHook {
public:
    // Whether the port of that word has a host link now (the link takes its
    // bytes; SharpAppPart::serialLinked).
    explicit SerialScriptsPart(std::function<bool(const std::string& port)> linked);

    // The serial port capability of `port` ("ce158", "rs232c") on the running
    // machine, or nullptr (EC-3).
    calcu::peripherals::SerialPort* port(const std::string& port) const;

    void feed(const std::string& port, std::vector<std::uint8_t> bytes, std::uint64_t hold);
    // The file is made empty now; nullopt, or the reason it cannot be written.
    std::optional<std::string> capture(const std::string& port, const std::string& path);

    struct Report {
        std::uint32_t baud = 0;   // 0: not set
        std::optional<int> bitsPerChar;
        bool used = false;
        std::size_t fed = 0, taken = 0;   // of the feed: given, taken by the port
        std::vector<std::uint8_t> sent;   // since the machine was built
        std::vector<std::uint8_t> printer;   // ce158: the bytes sent to its printer's port, since the build
    };
    Report report(const std::string& port);
    bool attached() const { return ctx_ != nullptr; }

    // ---- SessionPart ----
    void attach(calcu::sessions::SessionSetup&) override;
    void afterCall(calcu::sessions::SessionContext&, calcu::pacing::WallTime) override;
    void beforeMachineChange(calcu::sessions::SessionContext&) override;
    void machineChanged(calcu::sessions::SessionContext&) override;
    void closing(calcu::sessions::SessionContext&) override;

    // ---- SliceHook: the feed's bytes at their character times ----
    calcu::machine::Ticks nextDue(calcu::machine::Ticks now) const override;
    void act(calcu::machine::Machine&, calcu::machine::Ticks now, calcu::pacing::WallTime due) override;

private:
    struct Port {
        std::vector<std::uint8_t> feed;
        std::size_t taken = 0;                 // of feed
        calcu::machine::Ticks due = 0;         // the next character time of the feed
        std::string capturePath;
        std::vector<std::uint8_t> sent;        // since the machine was built
        std::vector<std::uint8_t> printer;     // ce158: its printer's port (c1600.ce158-printer-port)
    };
    calcu::machine::Ticks charTicks(const calcu::machine::Machine&, calcu::peripherals::SerialPort&) const;
    void drain();   // every port's sent bytes out, unless it is linked
    void append(const std::string& path, const std::uint8_t* bytes, std::size_t n);

    std::function<bool(const std::string&)> linked_;
    calcu::sessions::SessionContext* ctx_ = nullptr;
    std::uint64_t generation_ = 0;
    std::map<std::string, Port> ports_;
};

} // namespace shared
