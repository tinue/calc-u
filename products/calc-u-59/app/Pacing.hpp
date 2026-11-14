#pragma once
#include "../core/TICatalogue.hpp"
#include "FreezeController.hpp"
#include "MachineModelInfo.hpp"
#include "machine/Machine.hpp"
#include <cstdint>
#include <random>

// Real-time pacing of the run loop (reference/SharedArchitecture.md § "Emulation
// Loop and Timing"): 20 ms batches of the model's instruction rate, cycle-
// weighted — an IDLE instruction counts 4 (1 on the TI-58C, which runs at
// constant speed) — with the overshoot carried into the next batch so the
// long-term rate stays exact. A batch is one Machine::advance() in ticks (32
// per weighted cycle, C59-1 AC-1). Goes in M-33 (LIB-17 59). Each batch ends at a random offset
// from its 20 ms grid point (BatchJitter). The loop sleeps out the rest of
// each 20 ms unless full speed is on.

namespace cu59 {

inline constexpr double kBatchSeconds = 0.020;

/// Weighted cycles per batch: the instruction rate (tick rate / 32) × 20 ms
/// (≈ 284 on the TI-59).
inline int32_t batchTargetCycles(Model model) {
    return static_cast<int32_t>(tiCatalogue().model(modelId(model))->tickRate / 32.0 * kBatchSeconds);
}

/// Ticks per weighted cycle (C59-1 AC-1).
inline constexpr int32_t kTicksPerCycle = 32;

/// Batch targets whose ends fall at a random offset 0…target/2 past their
/// grid point (n × target): batch n gets target + rₙ − rₙ₋₁, so the offsets
/// never accumulate and the long-term rate stays exact. The frontends sample
/// the CPU between batches; with a fixed target, a ROM loop whose length
/// divides it (the TI-58's 80-step error-blink dark pass in 240) is caught at
/// the same instruction every frame and the CPU panel stands still.
class BatchJitter {
public:
    BatchJitter(int32_t target, uint32_t seed) : m_target(target), m_rng(seed), m_offset(0, target / 2) {}
    /// Weighted cycles for the next batch.
    int32_t next() {
        int32_t offset = m_offset(m_rng);
        int32_t cycles = m_target + offset - m_last;
        m_last = offset;
        return cycles;
    }
    /// Back to the grid point: the run loop restarts with no carry.
    void reset() { m_last = 0; }

private:
    const int32_t m_target;
    std::minstd_rand m_rng;
    std::uniform_int_distribution<int32_t> m_offset;
    int32_t m_last = 0;
};

struct BatchResult {
    enum class Stop { None, Breakpoint, Freeze };
    Stop stop = Stop::None;
    FreezeOwner freezeOwner = FreezeOwner::Cpu;  ///< for Stop::Freeze
    /// An instruction this batch was a confirmed spin-wait retrying a keyboard
    /// scan or card-switch poll (TIPacingHints) — real-world state that full
    /// speed must not race through. A printer BUSY wait does not set it.
    bool polledHardware = false;
};

/// Run one batch of `targetTicks`, continuing from `carry` (the previous
/// batch's overshoot, in ticks): one Machine::advance(). Stops early on a
/// breakpoint or an armed freeze firing, which `freeze`, set as the machine's
/// run-control hook, decides inside the advance (C59-1 design §4; LIB-17 D-7).
BatchResult runBatch(calcu::machine::Machine& m, int32_t& carry, int32_t targetTicks, FreezeController& freeze);

/// Whether to sleep out the rest of the batch. Full speed skips it — except
/// while the CPU sits in the scan/key-detect range or the batch polled
/// hardware, which always run at regular speed: racing them would execute an
/// unbounded number of idle iterations before anything notices the CPU is
/// idle (and inflate elapsed-time measurements across that window).
bool shouldSleep(bool fullSpeed, bool inIdleAndKeyDetect, bool polledHardware);

} // namespace cu59
