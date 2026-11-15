#pragma once
#include <cstdint>

#include "SerialLink.hpp"
#include "peripherals/SerialPort.hpp"

// A serial port of a Sharp machine as the library's serial port kind
// (LIB-2 design §2, §4; AC-17, AC-33): the core's SerialLink, the UART's
// peer (the PC-1600's TC8576F, the CE-158's UART), over two queues
// (calcu::peripherals::SerialPortQueues). No host, no thread, no lock: a
// host link (calcu::peripherals::HostLinks) moves the bytes between two
// advances. The port counts as used from the UART's first poll, send or
// status call, as the former LazySerialLink opened its terminal (LIB-2 D-5).
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
        l.bitsPerChar = bitsPerChar;
        m_queues.machineSetsLine(l);
    }

private:
    calcu::peripherals::SerialPortQueues m_queues;
};
