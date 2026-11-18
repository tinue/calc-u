#pragma once
#include <cstdint>
#include <optional>

#include "SerialLink.hpp"
#include "peripherals/SerialPort.hpp"

// A serial port of a Sharp machine as the library's serial port kind:
// the core's SerialLink, the UART's
// peer (the PC-1600's TC8576F, the CE-158's UART), over two queues
// (calcu::peripherals::SerialPortQueues). No host, no thread, no lock: a
// host link (calcu::peripherals::HostLinks) moves the bytes between two
// advances. The port counts as used from the UART's first poll, send or
// status call.
class SerialPortCapability final : public SerialLink {
public:
    calcu::peripherals::SerialPortQueues& port() { return m_queues; }
    const calcu::peripherals::SerialPortQueues& port() const { return m_queues; }

    bool poll(uint8_t& out) override {
        m_queues.markUsed();
        return m_queues.machineReceives(out);
    }
    void send(uint8_t byte) override { m_queues.machineSends(byte); }
    void setControl(bool dtr, bool rts) override {
        m_queues.markUsed();
        calcu::peripherals::LineSettings l = m_queues.lineSettings();
        l.dtr = dtr;
        l.rts = rts;
        m_queues.machineSetsLine(l);
    }
    void getStatus(Lines& in) override {
        m_queues.markUsed();
        const calcu::peripherals::PeerLines p = m_queues.peerLines();
        in.cts = p.cts;
        in.dsr = p.dsr;
        in.dcd = p.dcd;
        in.ri = p.ri;
    }
    void onBaud(uint32_t baud, int bitsPerChar) override {
        calcu::peripherals::LineSettings l = m_queues.lineSettings();
        l.baud = baud;
        if (bitsPerChar > 0) l.bitsPerChar = bitsPerChar;   // 0: the model does not say (the CE-158)
        m_queues.machineSetsLine(l);
    }

    /// The machine state (Machine/StateRecords.hpp): the
    /// line settings the machine has set, the peer's lines, whether the
    /// port was used. The bytes in the queues are on their way to or from
    /// the host and stay with it, as a serial port file does.
    template <class Io>
    void stateIo(Io& io) {
        calcu::peripherals::LineSettings l = m_queues.lineSettings();
        calcu::peripherals::PeerLines p = m_queues.peerLines();
        bool used = m_queues.used();
        optionalIo(io, l.baud);
        optionalIo(io, l.bitsPerChar);
        optionalIo(io, l.dtr);
        optionalIo(io, l.rts);
        io(p.cts)(p.dsr)(p.dcd)(p.ri)(used);
        if constexpr (Io::kRestoring) {
            m_queues.machineSetsLine(l);
            m_queues.setPeerLines(p);
            if (used) m_queues.markUsed();
        }
    }

private:
    calcu::peripherals::SerialPortQueues m_queues;

    template <class Io, class T>
    static void optionalIo(Io& io, std::optional<T>& v) {
        bool has = v.has_value();
        T value = has ? *v : T{};
        io(has)(value);
        if constexpr (Io::kRestoring) v = has ? std::optional<T>(value) : std::nullopt;
    }
};
