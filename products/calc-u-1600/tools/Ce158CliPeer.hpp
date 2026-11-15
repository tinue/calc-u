#pragma once
// Shared by pc1500_cli / pc1600_cli: the CE-158's serial peer for a
// headless run (--ce158-pty / --ce158-rx / --ce158-rx-hold / --ce158-tx)
// and the end-of-run report (Centronics output, serial traffic).
//
// The peer works on the CE-158's serial port capability (the library's
// serial port kind, LIB-2 design §3.3): pump() moves the bytes between two
// advances, as a host link does (calcu::peripherals::HostLinks). With
// --ce158-pty the other side is a pseudo-terminal (calcu::peripherals::
// openHostLink, the successor of PtySerialLink).

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "../core/Connector/Ce158Card.hpp"
#include "../core/Machine/LegacyCalls.hpp"
#include "CliCommon.hpp"
#include "peripherals/HostLink.hpp"
#include "peripherals/SerialPort.hpp"

inline void printCe158Bytes(const char* title, const std::vector<uint8_t>& bytes) {
    std::printf("%s (%zu bytes):\n", title, bytes.size());
    std::printf("%s\n", ce158PrintableText(bytes.data(), bytes.size()).c_str());
}

class Ce158CliPeer {
public:
    static constexpr const char* kUsage = "--ce158-pty | --ce158-rx <file> [--ce158-rx-hold <n>] --ce158-tx <file>";

    /// Consumes argv[i] (and its value) if it is a --ce158-* option.
    bool parseArg(int argc, char** argv, int& i) {
        if (std::strcmp(argv[i], "--ce158-pty") == 0) { m_pty = true; return true; }
        if (i + 1 >= argc) return false;
        if (std::strcmp(argv[i], "--ce158-rx") == 0) { m_rxPath = argv[++i]; return true; }
        if (std::strcmp(argv[i], "--ce158-tx") == 0) { m_txPath = argv[++i]; return true; }
        if (std::strcmp(argv[i], "--ce158-rx-hold") == 0) { m_hold = std::strtol(argv[++i], nullptr, 10); return true; }
        return false;
    }

    /// Whether a peer was asked for (pump() then has work).
    bool active() const { return m_pty || !m_rxPath.empty() || !m_txPath.empty(); }

    /// Opens the peer (before the preset plugs the CE-158 in).
    bool open() {
        if (m_pty) {
            m_link = calcu::peripherals::openHostLink(std::nullopt, "calcu1600-ce158.serial");
            if (!m_link) {
                std::fprintf(stderr, "CE-158 PTY: no pseudo-terminal on this host\n");
                return false;
            }
            std::fprintf(stderr, "CE-158 serial port: %s\n", m_link->path().c_str());
        } else if (!m_rxPath.empty()) {
            if (!cli::readFile(m_rxPath, &m_rx)) {
                std::fprintf(stderr, "cannot read %s\n", m_rxPath.c_str());
                return false;
            }
        }
        return true;
    }

    /// Between two advances: the CE-158's sent bytes out, the peer's bytes in.
    void pump(cli::Machine& machine) {
        if (!active()) return;
        auto* port = cli::ofPeripheral<calcu::peripherals::SerialPort>(machine, sharp::kCE158);
        if (!port) return;
        uint8_t buf[1024];
        for (size_t n; (n = port->takeSent(buf)) > 0;) {
            if (m_link) m_link->write(std::span<const uint8_t>(buf, n));
            else m_tx.insert(m_tx.end(), buf, buf + n);
        }
        if (m_link) {
            for (size_t n; m_pending.size() < calcu::peripherals::SerialPortQueues::kReceiveLimit &&
                           (n = m_link->read(buf)) > 0;)
                m_pending.insert(m_pending.end(), buf, buf + n);
            m_pending.erase(m_pending.begin(),
                            m_pending.begin() + static_cast<std::ptrdiff_t>(port->putReceived(m_pending)));
            port->setPeerLines({true, true, m_link->peerOpen(), false});
            return;
        }
        if (m_rxPos >= m_rx.size()) return;
        // --ce158-rx-hold: n character times of silence from the port's first
        // use (at the baud the ROM set; 10 bits a character).
        if (m_hold > 0) {
            if (!port->used()) return;
            if (!m_holdFrom) m_holdFrom = machine.tickCount();
            const uint64_t baud = port->lineSettings().baud.value_or(300);
            const uint64_t charTicks = static_cast<uint64_t>(machine.descriptor().tickRate) * 10 / (baud ? baud : 300);
            if (machine.tickCount() - *m_holdFrom < static_cast<uint64_t>(m_hold) * charTicks) return;
        }
        m_rxPos += port->putReceived(std::span<const uint8_t>(m_rx.data() + m_rxPos, m_rx.size() - m_rxPos));
    }

    /// Prints the CE-158 summary.
    bool report(cli::Machine& machine) {
        pump(machine);
        bool ok = true;
        if (cli::plugged(machine, sharp::kCE158)) {
            const Ce158Card* card = sharp::legacy::coreParts(machine).ce158;
            std::printf("CE-158: attached, baud=%d, UART status=%02X\n", card->baudRate(), card->uartStatus());
            auto* printer = cli::ofPeripheral<sharp::Ce158PrinterPort>(machine, sharp::kCE158);
            printCe158Bytes("CE-158 Centronics output", printer->drain());
            if (!m_rxPath.empty())
                std::printf("CE-158 serial: received %zu of %zu scripted bytes\n", m_rxPos, m_rx.size());
            if (!m_txPath.empty() || !m_rxPath.empty()) printCe158Bytes("CE-158 serial output", m_tx);
            if (!m_txPath.empty() && !cli::writeFile(m_txPath, m_tx)) {
                std::fprintf(stderr, "cannot write %s\n", m_txPath.c_str());
                ok = false;
            }
        }
        return ok;
    }

private:
    bool m_pty = false;
    std::string m_rxPath, m_txPath;
    long m_hold = 0;
    std::unique_ptr<calcu::peripherals::HostLink> m_link;
    std::vector<uint8_t> m_pending;   // read from the PTY, not yet taken by the port
    std::vector<uint8_t> m_rx, m_tx;
    size_t m_rxPos = 0;
    std::optional<uint64_t> m_holdFrom;
};
