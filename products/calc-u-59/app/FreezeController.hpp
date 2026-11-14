#pragma once
#include "../core/TIAdapters.hpp"
#include <atomic>
#include <cstdint>
#include <optional>
#include <utility>

// The debugger's armed freezes, checked from the run loop:
//  - FREEZE ON START (CPU tab): fires when the CPU leaves the steady keyboard
//    scan loop — a key press, or a reset (PC jumps to 0000) while running.
//  - Freeze on the next step (CALCULATOR tab): fires at the next step
//    boundary, where a debugger STEP stops too.
// The two are mutually exclusive: arming one disarms the other. Arming happens
// on the UI thread, checking on the emulation thread: the run loop sets it as
// the machine's run-control hook (TIDebugTargetAdapter::setRunControl), which
// asks it after every instruction inside Machine::advance() (until LIB-10's run
// control, M-53).

namespace cu59 {

/// Which debug panel owns a freeze (RESUME/STEP are enabled only there).
enum class FreezeOwner { Cpu, Calculator };

class FreezeController final : public TIStepObserver {
public:

    /// FREEZE ON START. `currentPC` guards against an immediate re-trigger at 0000.
    void armScanLoopExit(uint16_t currentPC);
    /// Freeze before the next program byte executes (CALCULATOR FREEZE ON START).
    void armPcChange();
    void disarmScanLoopExit();
    void disarmPcChange();

    bool scanLoopExitArmed() const { return m_scanArmed.load(); }
    bool pcChangeArmed() const { return m_pcArmed.load(); }

    /// Per instruction, with the PC of the next one and whether it is a step
    /// boundary (a per-batch check lands hundreds of instructions late). The
    /// panel whose armed freeze fires; it disarms itself.
    std::optional<FreezeOwner> checkStep(uint16_t pc, bool stepBoundary, bool inScanLoop);

    /// TIStepObserver: checkStep; a freeze that fires stops the advance.
    bool stopAfter(uint16_t pc, bool stepBoundary, bool inScanLoop) override;
    /// The owner of the freeze that stopped the last advance, and clears it.
    std::optional<FreezeOwner> takeFired() { return std::exchange(m_fired, std::nullopt); }

private:
    std::optional<FreezeOwner> m_fired;   // emulation thread
    std::atomic<bool> m_scanArmed{false};
    std::atomic<bool> m_seenLoop{false};
    std::atomic<uint16_t> m_armPC{0xFFFF};
    std::atomic<bool> m_pcArmed{false};
};

} // namespace cu59
