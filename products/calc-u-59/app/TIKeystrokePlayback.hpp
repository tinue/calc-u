#pragma once
#include "StateFile.hpp"
#include <chrono>
#include <cstdint>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// KEYSTROKES playback of state-file format 1 (LIB-5 adapter, design §4, §5;
// removed in M-54 with LIB-6 59): the script lane of 2.3.0's KeySequencer
// without its key handling. A `Key` event is handed to the library's typing
// engine as one press of origin Preset (held 100 ms of emulated time and
// until the ROM has read it, at key readiness, behaviour change 59-23), and
// playback waits until the engine is idle again (the key up and the machine
// ready). `Wait:` keeps 2.3.0's wall time (format 2's `wait:` is emulated
// time, LIB-6). A reload or an InsertCard waits for the scan loop, hands the
// work to the host and waits until the host has finished it; ArmCard only
// queues its card. A GUI control (PrinterTrace:, Freeze:, …) goes to the host
// at once and playback waits until it is done. While the debugger is frozen
// reloads and inserted cards wait (the engine's presses wait for a paused
// machine by themselves); directives go on, so Step: and Resume: work.
//
// No threads: advance(now) runs whatever is due, on the session's thread
// (TIAppPart's slice hook at the machine session's slice ends).

namespace cu59 {

class TIKeystrokePlayback {
public:
    struct Host {
        virtual ~Host() = default;
        /// The press of the key at this matrix code, queued in the typing
        /// engine (origin Preset); false when it could not be (no machine).
        virtual bool typeKey(uint8_t matrixCode) = 0;
        /// The typing engine has nothing down and nothing waiting.
        virtual bool typingIdle() = 0;
        /// The machine reports key readiness (Machine::isKeyReady, C59-1 AC-18).
        virtual bool isKeyReady() = 0;
        virtual bool isFrozen() = 0;
        virtual void setFullSpeed(bool on) = 0;
        virtual void scriptedTrace(const std::optional<std::string>& fileName) = 0;  ///< nullopt = stop
        virtual void zeroElapseTime() = 0;
        virtual void reportElapseTime() = 0;
        /// Apply a reload stage (the calculator is idle); hostPending() until done.
        virtual void applyReload(const std::shared_ptr<const ReloadStage>& stage) = 0;
        /// Queue the card of an ArmCard / InsertCard event; an InsertCard card
        /// keeps hostPending() true until it has passed through the reader.
        virtual void queueCard(const KeystrokeEvent& event) = 0;
        /// A GUI control (PrinterTrace … Resume); hostPending() until done.
        virtual void control(const KeystrokeEvent& event) = 0;
        /// A reload, an inserted card or a control is not done yet.
        virtual bool hostPending() = 0;
        virtual void playbackStarted() = 0;
        /// Always called at the end of a script, even if cancelled: the host
        /// drops full speed and closes a scripted trace left open.
        virtual void playbackFinished() = 0;
    };

    explicit TIKeystrokePlayback(Host& host) : m_host(host) {}
    TIKeystrokePlayback(const TIKeystrokePlayback&) = delete;
    TIKeystrokePlayback& operator=(const TIKeystrokePlayback&) = delete;

    /// Play a KEYSTROKES sequence (queued after a script still running).
    void play(std::vector<KeystrokeEvent> events);
    /// Cancel playback (a key the engine holds still comes up).
    void cancel();
    bool isPlaying() const { return m_playing; }
    /// A script plays, or one is queued.
    bool pending() const { return m_current.has_value() || !m_scripts.empty(); }

    /// Run what is due at `now`.
    void advance(std::chrono::steady_clock::time_point now);

private:
    using Clock = std::chrono::steady_clock;
    struct Script {
        enum class Phase { Next, Typing, Sleeping, WaitingToApply, WaitingHost };
        std::vector<KeystrokeEvent> events;
        uint64_t generation = 0;
        size_t next = 0;
        Phase phase = Phase::Next;
        Clock::time_point wake{};
        bool started = false;
    };
    /// Run the script until it waits; false when it has finished.
    bool step(Script& s, Clock::time_point now);
    bool cancelled(const Script& s) const { return s.generation != m_generation; }

    Host& m_host;
    bool m_playing = false;
    uint64_t m_generation = 0;
    std::deque<Script> m_scripts;
    std::optional<Script> m_current;
    Clock::time_point m_now{};
};

} // namespace cu59
