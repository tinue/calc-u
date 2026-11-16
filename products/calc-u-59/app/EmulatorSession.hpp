#pragma once
#include "FrameSampler.hpp"
#include "../core/TIAdapters.hpp"
#include "MachineModelInfo.hpp"
#include "machine/Machine.hpp"
#include "peripherals/Controls.hpp"
#include "roms/RomSet.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// The runtime around one running calculator that every frontend shares: the
// 60 Hz frame sampler and the PC-100C's controls (the debug panel's reads
// are TIDebugPart's since M-56). Since M-54 (LIB-18 §11.3, D-26) it neither builds nor owns
// the machine: LIB-18's machine control builds it and the library's machine
// session holds it; cu59::TIAppPart makes a new EmulatorSession around each
// new machine (machineChanged). It runs on the session's one thread
// (ADR 0009). See reference/SharedArchitecture.md.

namespace cu59 {

class EmulatorSession {
public:
    EmulatorSession(calcu::machine::Machine& machine, const calcu::roms::RomSet& roms);
    EmulatorSession(const EmulatorSession&) = delete;
    EmulatorSession& operator=(const EmulatorSession&) = delete;

    Model model() const { return m_model; }
    /// The machine and its adapters (C59-1's TI adapters).
    class Access {
    public:
        explicit Access(calcu::machine::Machine& m) : m_m(m) {}
        calcu::machine::Machine& machine() { return m_m; }
        TICardReaderAdapter& cardReader() { return *cardReaderOf(m_m); }
        TIMemoryAdapter& memory() { return *memoryOf(m_m); }

    private:
        calcu::machine::Machine& m_m;
    };
    Access access() const { return Access(m_machine); }
    calcu::machine::Machine& machine() { return m_machine; }
    /// The bytes of the module plugged into `slot-1` (empty if none), kept for
    /// the debugger (adapter until M-56, C59-6 design §5); refreshModule()
    /// reads them again after a plug change.
    const std::vector<uint8_t>& moduleImage() const { return m_moduleImage; }
    void refreshModule();
    /// The module in `slot-1` now (empty: none).
    std::string moduleID() const;
    bool printerPlugged() const;

    /// A control of the PC-100C: a key down or up, the switch on or off
    /// (C59-6 AC-5). False without a PC-100C.
    bool printerControl(const calcu::machine::ControlId& id, bool on);
    /// The switch's position; nullopt without a PC-100C.
    std::optional<bool> printerSwitch(const calcu::machine::ControlId& id) const;

    // ── Frames ───────────────────────────────────────────────────────────────
    FrameUpdate tick(bool cardSwiping) { return m_sampler.tick(m_machine, cardSwiping); }

private:
    const Model m_model;
    calcu::machine::Machine& m_machine;
    const calcu::roms::RomSet& m_roms;
    std::vector<uint8_t> m_moduleImage;
    FrameSampler m_sampler;
};

} // namespace cu59
