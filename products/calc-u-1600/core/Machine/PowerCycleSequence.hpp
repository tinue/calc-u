#pragma once
#include <algorithm>
#include <cstdint>

// The power cycle of a Sharp machine as a sequence over the advances that
// follow (C1600-1 design §3.3; AC-6, AC-7): today's MachineSession sequence
// (0.8.1 `app/MachineSession.cpp:244-257`), moved into the core. No clock,
// no thread: SharpMachine::advance() runs it in chunks that end on its
// phase changes, and each phase is one run of the core, as today's job ran
// one `job::run` per phase.
//
//   1. `off` down for kKeyHoldFrames frames, then released;
//   2. up to kPowerOffTimeoutFrames frames, one at a time, until the machine
//      is off (checked before each frame);
//   3. the off point: the changes that wait for it are made (D-2);
//   4. `on` down for kKeyHoldFrames frames, then released.
// Running to the prompt is the caller's (LIB-18; until M-46
// LegacyCalls::runToPromptJob).

namespace sharp {

class PowerCycleSequence {
public:
    // ~4 frames at 60 Hz: long enough for the ROM's key scan to see a press.
    static constexpr int kKeyHoldFrames = 4;
    // 3 s of emulated time: don't hang if the power-down never completes.
    static constexpr int kPowerOffTimeoutFrames = 180;

    // What the sequence does to the machine at its phase changes.
    class Actions {
    public:
        virtual ~Actions() = default;
        virtual void offKey(bool down) = 0;
        virtual void onKey(bool down) = 0;
        virtual bool switchedOff() const = 0;
        virtual void atOffPoint() = 0;
    };

    // frame = tick rate / 60, as today (`clockHz() / 60` truncated).
    void start(std::uint64_t frame, Actions& a) {
        m_frame = frame;
        m_phase = Phase::OffHeld;
        m_remaining = frame * kKeyHoldFrames;
        a.offKey(true);
    }
    bool active() const { return m_phase != Phase::Idle; }
    // Ticks to the next phase change (0 when idle).
    std::uint64_t remaining() const { return m_remaining; }

    // `ran` ticks of the current phase have run; at its end the next phase
    // starts (possibly several, when no ticks are needed in between).
    void consumed(std::uint64_t ran, Actions& a) {
        if (!active()) return;
        m_remaining -= std::min(ran, m_remaining);
        if (m_remaining > 0) return;
        switch (m_phase) {
            case Phase::OffHeld:
                a.offKey(false);
                m_phase = Phase::WaitOff;
                m_frames = 0;
                if (a.switchedOff()) changeAndPressOn(a);
                else m_remaining = m_frame;
                break;
            case Phase::WaitOff:
                if (++m_frames >= kPowerOffTimeoutFrames || a.switchedOff()) changeAndPressOn(a);
                else m_remaining = m_frame;
                break;
            case Phase::OnHeld:
                a.onKey(false);
                m_phase = Phase::Idle;
                break;
            case Phase::Idle: break;
        }
    }

    // Abandons a sequence under way (a reset, a cold boot); the keys it
    // holds are released.
    void cancel(Actions& a) {
        if (m_phase == Phase::OffHeld) a.offKey(false);
        if (m_phase == Phase::OnHeld) a.onKey(false);
        m_phase = Phase::Idle;
        m_remaining = 0;
    }

private:
    enum class Phase { Idle, OffHeld, WaitOff, OnHeld };
    void changeAndPressOn(Actions& a) {
        a.atOffPoint();
        a.onKey(true);
        m_phase = Phase::OnHeld;
        m_remaining = m_frame * kKeyHoldFrames;
    }

    Phase m_phase = Phase::Idle;
    std::uint64_t m_frame = 0;
    std::uint64_t m_remaining = 0;
    int m_frames = 0;
};

} // namespace sharp
