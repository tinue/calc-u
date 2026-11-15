#pragma once
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <string>

#include "KeyPaste.hpp"
#include "app/HostKeyMap.hpp"
#include "machine/Machine.hpp"
#include "sessions/MachineSession.hpp"

// The adapters around the library's machine session for Calc-U-1600 (LIB-3
// design §2, §5): today's typing as the session's input queue, and today's
// debugger slice as its advance gate. Single-threaded, on the session's
// thread; the frontend calls them directly until their steps.

namespace shared {

// ── SharpTypingHook (adapter until M-36, LIB-5) ──────────────────────────
//
// The keys and typing of 0.8.1's MachineSession (`KeyPaste`, the timed
// taps, the 60 Hz paste frames, hostKeyDown) as the machine session's
// SliceHook: the session cuts its advances at nextDue() and calls act()
// there, so the feeder acts at frame boundaries of emulated time and a
// timed tap fires at its tick. Times are the machine's tick count.
//
// A user's key (a faceplate key, a host key, a paste) is dropped while an
// operation runs (behaviour change 1600-9; LIB-3 AC-16): `busy` says so.
// A release still goes through, and the debugger's typeCommand() too.
class SharpTypingHook final : public calcu::sessions::SliceHook {
public:
    SharpTypingHook(calcu::sessions::MachineSession& ms, std::function<bool()> busy);

    // SliceHook
    calcu::machine::Ticks nextDue(calcu::machine::Ticks now) const override;
    void act(calcu::machine::Machine&, calcu::machine::Ticks now, calcu::pacing::WallTime due) override;

    // The machine went away (a rebuild, a reset): what it typed is dropped.
    void machineGone();

    // ---- Host keyboard ----
    // A host key resolved to a calculator key (HostKeyMap). Stops a paste,
    // then types it the way the model needs: on the PC-1600 an accented
    // character (or any key while one still types) through the live queue,
    // a shifted character as a Shift tap sequence, a plain `physical` key
    // as a real press, a committed character as a tap; on the PC-1500 the
    // cursor keys pressed for real (the ROM's own auto-repeat), everything
    // else through the keystroke queue. True if it pressed a key the
    // frontend must release with hostKeyUp() when the host key comes up.
    bool hostKeyDown(const ResolvedKey& key, bool physical);
    void hostKeyUp(const std::string& baseKey) { releaseKey(baseKey); }
    // A bare Shift press and release: one calculator SHIFT tap.
    void hostShiftTap();

    // ---- Keys and typing ----
    // A frontend's own key by key ID (C1600-1 AC-9, AC-10). A release that
    // comes before kMinKeyHoldFrames (2) of emulated time since the press is
    // deferred until then, so the ROM's key scan always sees the key; a
    // press while such a release is still to come waits for it.
    void pressKey(const std::string& name);
    void releaseKey(const std::string& name);
    void setOnKeyPressed(bool pressed);
    // PC-1600: SHIFT tap, 100 ms, then `baseName` tapped; 30 ms holds.
    void tapShiftedKey(const std::string& baseName);
    // One tap of `name`: the PC-1500's keystroke queue, a 30 ms press on
    // the PC-1600.
    void tapKey(const std::string& name);

    // Edit > Paste Text: types `text` (UTF-8) at the ROM's own keystroke
    // cadence (core/KeyPaste.hpp). Appends to a paste in progress.
    void pasteText(const std::string& text);
    // The debugger's auto-start: `line`, then ENTER (not a user key).
    void typeCommand(const std::string& line);
    bool pasteActive() const { return m_paste.active() && !m_liveTyping; }
    void cancelPaste();
    bool liveTypingActive() const { return m_paste.active() && m_liveTyping; }
    // Nothing still to type or to release.
    bool idle() const { return !m_paste.active() && m_timedKeys.empty(); }

private:
    calcu::machine::Machine* machine() const { return m_ms.machine(); }
    bool isPC1600() const;
    double clockHz() const;
    calcu::machine::Ticks now() const;
    bool userKeysDropped() const { return m_busy && m_busy(); }

    void enqueueKey(const std::string& name);
    void enqueueShiftedKey(const std::string& baseName);
    void interruptPaste();
    void typeLiveStep(const PasteStep& step);
    void enqueueTyped(const std::string& text, bool pressEnter);
    void pasteOnFrame();
    void startFrames();

    struct TimedKey {
        calcu::machine::Ticks due;
        bool press;
        std::string name;
    };
    void scheduleKey(double afterSeconds, bool press, const std::string& name);
    void scheduleKeyAt(calcu::machine::Ticks due, bool press, const std::string& name);
    void fireDueKeys(calcu::machine::Ticks now);
    calcu::machine::Ticks minKeyHoldTicks() const;
    calcu::machine::Ticks frameTicks() const;
    void machinePressKey(const std::string& name);
    void machineReleaseKey(const std::string& name);

    calcu::sessions::MachineSession& m_ms;
    std::function<bool()> m_busy;
    KeyPasteFeeder m_paste;
    bool m_liveTyping = false;               // m_paste holds live keystrokes, not a paste
    calcu::machine::Ticks m_nextFrame = 0;   // the paste feeder's next frame boundary
    std::deque<TimedKey> m_timedKeys;        // in due order
    std::map<std::string, calcu::machine::Ticks> m_keyPressedAt;
};

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
