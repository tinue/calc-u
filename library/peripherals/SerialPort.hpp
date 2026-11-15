#pragma once
#include "machine/Capability.hpp"
#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <span>

// The generic kind serial port (LIB-2 design §3.3; spec AC-17, EC-7;
// capabilities.md C-1, I-3): the bytes a machine sends, taken out, and the
// bytes it is to receive, put in, in order; the line settings the machine
// has set as far as the model has them. The capability reaches no host: a
// host link (HostLinks.hpp) does that.

namespace calcu::peripherals {

// What the machine has set; a field the model does not have stays empty.
struct LineSettings {
    std::optional<std::uint32_t> baud;
    std::optional<int> bitsPerChar;
    std::optional<bool> dtr, rts;
    bool operator==(const LineSettings&) const = default;
};

// The lines the other side drives. A pseudo-terminal carries none, so a host
// link reports CTS and DSR on and DCD while a program has the file open.
struct PeerLines {
    bool cts = true, dsr = true, dcd = true, ri = false;
    bool operator==(const PeerLines&) const = default;
};

class SerialPort : public machine::Capability {
public:
    static constexpr machine::KindId kKind{"serial-port"};
    machine::KindId kind() const override { return kKind; }

    virtual std::size_t takeSent(std::span<std::uint8_t> out) = 0;          // oldest first; how many
    virtual std::size_t putReceived(std::span<const std::uint8_t> in) = 0;  // how many it took; the rest stays with the caller
    virtual LineSettings lineSettings() const = 0;
    virtual bool used() const = 0;   // the machine has used the port since it exists (AC-33; D-5)
    virtual void setPeerLines(const PeerLines&) = 0;
};

// A ready-made implementation for cores: two queues, no thread, no lock
// (I-3). The sent queue has no limit, so a machine that sends is never held
// up (EC-7; D-8); the received queue takes up to kReceiveLimit bytes.
class SerialPortQueues final : public SerialPort {
public:
    static constexpr std::size_t kReceiveLimit = 8192;   // today's kRxHighWater (Calc-U-1600 PtySerialLink)

    std::size_t takeSent(std::span<std::uint8_t> out) override;
    std::size_t putReceived(std::span<const std::uint8_t> in) override;
    LineSettings lineSettings() const override { return line_; }
    bool used() const override { return used_; }
    void setPeerLines(const PeerLines& p) override { peer_ = p; }

    // The core's side, called by the UART at its character time.
    void machineSends(std::uint8_t b) {
        used_ = true;
        sent_.push_back(b);
    }
    bool machineReceives(std::uint8_t& out);
    void machineSetsLine(const LineSettings& l) { line_ = l; }
    PeerLines peerLines() const { return peer_; }
    void markUsed() { used_ = true; }

    std::size_t sentWaiting() const { return sent_.size(); }
    std::size_t receivedWaiting() const { return received_.size(); }

private:
    std::deque<std::uint8_t> sent_, received_;
    LineSettings line_;
    PeerLines peer_;
    bool used_ = false;
};

} // namespace calcu::peripherals
