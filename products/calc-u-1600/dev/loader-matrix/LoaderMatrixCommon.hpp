#pragma once
// Shared by pc1500_loader_matrix and pc1600_loader_matrix: the machine built
// through the machine interface (C1600-1 design §7), the preset applied by
// the core's loader (sharp::legacy::applyPresetJob until M-46), and the
// serial peer that sends what `sde put` sends, on a serial port capability.

#include <cstdint>
#include <fstream>
#include <iterator>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "../../core/Display/DisplayOutputText.hpp"
#include "../../core/Machine/LegacyCalls.hpp"
#include "../../core/Preset/PresetFile.hpp"
#include "../../tools/CliCommon.hpp"
#include "peripherals/SerialPort.hpp"

namespace matrix {

using calcu::machine::Machine;

inline bool readFile(const std::string& path, std::vector<uint8_t>* out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    out->assign(std::istreambuf_iterator<char>(in), {});
    return true;
}

// The display as one line of text, rows joined with '|'.
inline std::string lcd(Machine& m) {
    std::string t = displayText(m).plainText();
    for (char& c : t) if (c == '\n') c = '|';
    return t;
}

// The serial port owned by component `component` ("rs232c") or by the
// peripheral `peripheral` ("CE-158"); taking it gives its UART the peer.
inline calcu::peripherals::SerialPortQueues* serialPort(Machine& m, const char* component, const char* peripheral) {
    for (const auto& e : m.capabilities(calcu::peripherals::SerialPort::kKind)) {
        const auto* c = std::get_if<calcu::machine::ComponentOwner>(&e.owner);
        const auto* p = std::get_if<calcu::machine::PeripheralOwner>(&e.owner);
        if ((c && component && c->component.value == component) || (p && peripheral && p->peripheral.value == peripheral))
            // Calc-U-1600's ports are SerialPortQueues (core/Serial/SerialPortCapability.hpp).
            return static_cast<calcu::peripherals::SerialPortQueues*>(
                calcu::machine::capabilityAs<calcu::peripherals::SerialPort>(m, e.id));
    }
    return nullptr;
}

// What `sde put` does: the header at full speed, HEADER_PAUSE, then the
// payload with `gapChars` idle character times after each byte. pump() is
// called once per character time, as the UART polled its peer: a byte goes
// in when the UART has taken the last one.
struct SdeFeeder {
    std::vector<uint8_t> rx;
    size_t headerLen = 0;
    size_t pos = 0;
    long pauseChars = 0;   // the header pause, in character times
    long gapChars = 0;
    long gapLeft = 0;
    bool armed = false;

    void pump(calcu::peripherals::SerialPortQueues& q) {
        if (!armed || pos >= rx.size()) return;
        if (pos < headerLen) {
            pos += q.putReceived(std::span<const uint8_t>(rx.data() + pos, headerLen - pos));
            return;
        }
        if (q.receivedWaiting() > 0) return;
        if (pauseChars > 0) { --pauseChars; return; }
        if (gapLeft > 0) { --gapLeft; return; }
        pos += q.putReceived(std::span<const uint8_t>(rx.data() + pos, 1));
        gapLeft = gapChars;
    }
    bool done(const calcu::peripherals::SerialPortQueues* q) const {
        return pos >= rx.size() && (!q || q->receivedWaiting() == 0);
    }
};

// Builds the preset's model and applies the preset. `takeRs232c`: the
// PC-1600's RS-232C gets its peer before the preset runs (the old
// harnesses' setSerialLink()).
inline std::unique_ptr<Machine> applyPreset(const std::string& presetPath, bool takeRs232c, PresetLoadResult* res) {
    PresetFile preset;
    std::string err;
    if (!parsePresetFile(presetPath, &preset, &err)) {
        res->ok = false;
        res->error = "preset: " + err;
        return nullptr;
    }
    std::optional<std::string> variant;
    std::string model;
    if (preset.isPC1600()) {
        model = sharp::kPC1600;
        variant = preset.romVariant == "old" ? "Old" : "New";
    } else {
        model = preset.variant == PC1500Variant::PC1500A ? sharp::kPC1500A : sharp::kPC1500;
    }
    std::unique_ptr<Machine> m = cli::buildMachine(model, variant, "roms", &err);
    if (!m) {
        res->ok = false;
        res->error = "ROM set: " + err;
        return nullptr;
    }
    if (takeRs232c) serialPort(*m, "rs232c", nullptr);
    *res = job::runToCompletion(sharp::legacy::applyPresetJob(*m, preset, {}, ".", "frontends/qt6/resources/cards", {},
                                                              cli::presetRoms("roms", {}), {}, {}, {}));
    return m;
}

} // namespace matrix
