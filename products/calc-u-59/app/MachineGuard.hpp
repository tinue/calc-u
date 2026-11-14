#pragma once
#include "../core/TIAdapters.hpp"
#include "machine/Machine.hpp"
#include <mutex>

// The app-layer lock of C59-1 (design §5, D-1): the core holds no lock any
// more (ADR 0009), but until the session runs on one thread three threads
// call the machine — the runner (EmulationRunner), the UI thread and the key
// sequencer's lanes. MachineGuard serialises them: the runner holds it per
// advance and per debug step, every other call on the machine or an adapter
// holds it per call (an Access lives to the end of the full expression, or of
// the scope that keeps it). Callbacks (freezeFired, batchDone) run outside it.
// With Threading::Cooperative (Web) it is never contended.
//
// Recursive, so that a function holding an Access may call one that takes it
// again. Removed in M-33 for the Qt6 and Swift paths (LIB-3 59 / C59-5: the
// session on one thread) and at the latest in M-34 with its last user, the
// Web's cooperative EmulatorSession (LIB-15 59).

namespace cu59 {

class MachineGuard {
public:
    explicit MachineGuard(calcu::machine::Machine& m) : m_machine(m) {}

    /// The machine and its adapters, under the lock for this object's lifetime.
    class Access {
    public:
        calcu::machine::Machine& machine() { return m_m; }
        TIPeripheralsAdapter& peripherals() { return *peripheralsOf(m_m); }
        TICardReaderAdapter& cardReader() { return *cardReaderOf(m_m); }
        TIMemoryAdapter& memory() { return *memoryOf(m_m); }
        TIDebugTargetAdapter& debugTarget() { return *debugTargetOf(m_m); }
        TIDebugPanelAdapter& debugPanel() { return *debugPanelOf(m_m); }
        TIPacingHints& pacingHints() { return *pacingHintsOf(m_m); }

    private:
        friend class MachineGuard;
        Access(std::recursive_mutex& mutex, calcu::machine::Machine& m) : m_lock(mutex), m_m(m) {}
        std::unique_lock<std::recursive_mutex> m_lock;
        calcu::machine::Machine& m_m;
    };

    Access access() { return Access(m_mutex, m_machine); }

private:
    std::recursive_mutex m_mutex;
    calcu::machine::Machine& m_machine;
};

} // namespace cu59
