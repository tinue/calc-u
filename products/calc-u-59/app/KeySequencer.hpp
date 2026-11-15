#pragma once
#include "MachineModelInfo.hpp"
#include "StateFile.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

// Timed key input: KEYSTROKES playback and the physical keyboard.
//
// A tap waits until the CPU is in the steady keyboard scan loop, holds the key
// 100 ms — longer than the ROM's worst-case debounce (~3 sweeps to arm and
// confirm, ideas/KeypressLatch.md) yet short enough that a repeat-while-held
// key (Adv) fires only a couple of times at regular speed — then releases it
// and waits for the scan loop again, so no keystroke is lost to a
// still-computing calculator or one still in its key-release gate.
//
// Physical keys follow the hardware's level semantics: a single code goes down
// on key-down and up on key-up, but never earlier than 100 ms after it went
// down (a faster release would be discarded as bounce). A two-code binding
// (2nd + key, e.g. Shift+A for A′) is played as two taps instead, since the ROM
// must see 2nd released and rescanned before the second key.
//
// A reload or an InsertCard waits for the scan loop like a key, hands the work
// to the host and waits until the host has finished it; ArmCard only queues
// its card. A GUI control (PrinterTrace:, Freeze:, …) goes to the host at once
// and playback waits until it is done. While the debugger is frozen, keys,
// reloads and inserted cards wait until it runs again (a key pressed into a
// frozen machine would be lost); directives go on, so Step: and Resume: work.
//
// Playback and keyboard taps run on two independent lanes, as two separate
// tasks would: a key press cancels earlier keyboard taps, never a running
// script. Each script or tap is a resumable task — a small state machine that
// says when it next wants to run. There are no threads: advance(now) runs
// whatever is due, on the caller's thread (the session's: the machine
// session's slice ends, LIB-3 59's TIKeySequencerHook, or the Web's
// cooperative pump).

namespace cu59 {

class KeySequencer {
public:
    struct Host {
        virtual ~Host() = default;
        virtual void pressKey(uint8_t matrixCode) = 0;
        virtual void releaseKey(uint8_t matrixCode) = 0;
        /// The machine reports key readiness (Machine::isKeyReady, C59-1 AC-18).
        virtual bool isKeyReady() = 0;
        virtual bool isFrozen() = 0;
        /// The key shown pressed (row×5 + col, 0-based), -1 for none.
        virtual void heldKeyChanged(int highlightID) = 0;
        // KEYSTROKES playback
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

    static constexpr std::chrono::milliseconds kHold{100};
    static constexpr std::chrono::microseconds kScanPoll{1500};

    explicit KeySequencer(Host& host);
    ~KeySequencer();
    KeySequencer(const KeySequencer&) = delete;
    KeySequencer& operator=(const KeySequencer&) = delete;

    /// Play a KEYSTROKES sequence (queued after a script still running).
    void play(std::vector<KeystrokeEvent> events);

    /// Cancel playback (a key being tapped is still released).
    void cancel();

    bool isPlaying() const { return m_playing.load(); }
    /// A script plays, or one is queued.
    bool scriptPending() const { return m_playing.load() || m_script.busy(); }

    // Physical keyboard
    void keyDown(const std::vector<uint8_t>& codes);
    void keyUp();
    /// Force-release whatever is held (focus lost, view torn down).
    void releaseAll();

    /// Run the lanes and deferred releases due at `now`.
    void advance(std::chrono::steady_clock::time_point now);

    /// row×5 + col identifier the keyboard view highlights for a matrix code.
    static int highlightID(uint8_t matrixCode) { return (matrixCode / 10 - 1) * 5 + (matrixCode % 10 - 1); }

private:
    using Clock = std::chrono::steady_clock;

    /// One script or keyboard tap sequence, resumable: step() runs it until it
    /// has to wait, and says until when.
    struct Task {
        enum class Phase {
            Next,            ///< run the next event
            WaitingToPress,  ///< poll every kScanPoll until the CPU is in the scan loop, then press
            Holding,         ///< key down until `wake` (a cancel does not cut it short)
            WaitingIdle,     ///< key up; poll every kScanPoll until the CPU is in the scan loop
            Sleeping,        ///< Wait until `wake` (a cancel ends it)
            WaitingToApply,  ///< poll until the CPU is in the scan loop, then run the reload / InsertCard
            WaitingHost,     ///< poll until the host has finished it (a cancel ends it)
        };
        std::vector<KeystrokeEvent> events;  ///< a keyboard tap sequence holds Key events only
        bool script = false;
        uint64_t generation = 0;
        size_t next = 0;
        Phase phase = Phase::Next;
        Clock::time_point wake{};
        uint8_t code = 0;                    ///< the key being tapped
    };

    /// A serial queue of tasks; cancel() bumps the generation, which drops
    /// queued tasks and ends a waiting one. The owner calls advance().
    class Lane {
    public:
        explicit Lane(KeySequencer& owner);
        ~Lane();
        void post(Task task);
        void cancel();
        bool cancelled(uint64_t generation) const { return m_generation.load() != generation; }
        bool busy() const {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_current.has_value() || !m_tasks.empty();
        }
        /// Run what is due at `now`; when the task in progress next wants to
        /// run, nullopt when the lane is idle.
        std::optional<Clock::time_point> advance(Clock::time_point now);
    private:
        bool takeNext(Task& task);  // under m_mutex
        KeySequencer& m_owner;
        std::atomic<uint64_t> m_generation{0};
        mutable std::mutex m_mutex;
        std::deque<Task> m_tasks;
        std::optional<Task> m_current;  ///< the task in progress
    };

    /// Run `task` at `now` until it waits: when it next wants to run, or
    /// nullopt when it has finished.
    std::optional<Clock::time_point> step(Lane& lane, Task& task, Clock::time_point now);
    void finish(Task& task);
    void releaseAt(uint8_t code, Clock::time_point when);
    /// Release every key due by `now`.
    void releaseDue(Clock::time_point now);
    Clock::time_point now() const { return m_now; }

    Host& m_host;
    std::atomic<bool> m_playing{false};
    Clock::time_point m_now{};  ///< the last advance() time

    // Physical keyboard: the single code held down, and since when.
    std::mutex m_keyMutex;
    std::optional<uint8_t> m_heldCode;
    Clock::time_point m_heldSince;

    // Deferred releases.
    std::vector<std::pair<Clock::time_point, uint8_t>> m_releases;

    Lane m_script;
    Lane m_keys;
};

} // namespace cu59
