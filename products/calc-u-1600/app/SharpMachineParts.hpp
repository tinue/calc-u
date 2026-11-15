#pragma once
#include <cstdint>

#include "machine/Machine.hpp"
#include "sessions/MachineSession.hpp"

// The adapter around the library's machine session for Calc-U-1600 (LIB-3
// design §2, §5): today's debugger slice as its advance gate. (Typing is
// the library's typing engine since M-36: app/SharpTyping.) Single-threaded, on the session's
// thread; the frontend calls them directly until their steps.

namespace shared {

// ── SharpDebuggerGate (adapter until M-45, LIB-10 1600) ──────────────────
//
// 0.8.1's `Listener::runDebuggerSlice` and `machineAboutToChange` /
// `machineReplaced` towards the Qt DebugController: same thread, called
// directly. With a debugger attached, every advance of the machine
// session goes through its run control (pause, stepping, breakpoints).
class SharpDebugger {
public:
    virtual ~SharpDebugger() = default;
    virtual void machineAboutToChange() {}
    virtual void machineReplaced() {}
    // True when the debugger ran the slice itself (it may run less: paused).
    virtual bool runDebuggerSlice(std::uint64_t /*ticks*/) { return false; }
};

class SharpDebuggerGate final : public calcu::sessions::AdvanceGate {
public:
    void setDebugger(SharpDebugger* d) { m_debugger = d; }
    SharpDebugger* debugger() const { return m_debugger; }
    calcu::machine::Ticks advance(calcu::machine::Machine&, calcu::machine::Ticks atLeast, bool& stoppedEarly) override;

private:
    SharpDebugger* m_debugger = nullptr;
};

} // namespace shared
