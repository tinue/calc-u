#pragma once
#include "FrameSampler.hpp"
#include "../core/TIDebugKinds.hpp"
#include "../core/TIProductKinds.hpp"
#include "machine/Machine.hpp"
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
    /// The machine (its capabilities: the product kinds of core/TIProductKinds.hpp).
    class Access {
    public:
        explicit Access(calcu::machine::Machine& m) : m_m(m) {}
        calcu::machine::Machine& machine() { return m_m; }

    private:
        calcu::machine::Machine& m_m;
    };
    Access access() const { return Access(m_machine); }
    calcu::machine::Machine& machine() { return m_machine; }
    /// The module in `slot-1` now (empty: none).
    std::string moduleID() const;
    bool printerPlugged() const;

    /// The switch's position; nullopt without a PC-100C (it is set through
    /// LIB-2's set-control, C59-6 AC-5).
    std::optional<bool> printerSwitch(const calcu::machine::ControlId& id) const;

    // ── Frames ───────────────────────────────────────────────────────────────
    FrameUpdate tick() { return m_sampler.tick(m_machine); }

private:
    const Model m_model;
    calcu::machine::Machine& m_machine;
    FrameSampler m_sampler;
};

} // namespace cu59
