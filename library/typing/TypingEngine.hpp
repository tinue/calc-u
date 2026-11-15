#pragma once
#include "machine/Machine.hpp"
#include "pacing/WallClock.hpp"
#include "sessions/MachineSession.hpp"
#include "typing/HostKey.hpp"
#include "typing/KeyboardMap.hpp"
#include "typing/TypingProfile.hpp"
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// The typing engine (LIB-5 design §3.3; spec AC-1 to AC-13, AC-18, AC-19,
// AC-38, AC-39; EC-1 to EC-6): host keys, on-screen keys and text turned
// into key down and key up on a machine, by the model's typing profile and
// keyboard map. It is the machine session's input queue (a SliceHook): the
// session cuts its advances at nextDue() and calls act() there, so every
// hold, gap and wait is counted in the machine's ticks (TR; typing.md I-1).
//
// Single-threaded, on the session's thread; it reads no clock (E14). The
// one wall-time rule, the rest of a hold on a paused machine (AC-20), uses
// the time of the host call it is handed (hostCall).
//
// Keys put down by the raw `key-down` command are not the engine's (AC-39):
// it neither waits for them nor releases them.

namespace calcu::typing {

class TypingSupport;

// Where a press comes from: live input (a host key, an on-screen key, text
// the host's input method committed), typed text and commands (Paste Text,
// `key`, `type`, the debugger's line), a preset (LIB-6; Calc-U-59's
// KEYSTROKES playback until M-54).
enum class Origin { Live, Typed, Preset };

using Token = std::uint64_t;   // one `typeKeys` or `typeText`; 0 for none

struct TypeResult {
    bool done = true;
    std::string reason;   // when not done
};

inline constexpr const char* kStoppedByKey = "Stopped by a key.";                  // D-9
inline constexpr const char* kStartedOver = "The machine started over.";           // E12
inline constexpr const char* kFocusWasLost = "The focus was lost.";                 // E10

struct Queued {
    Token token = 0;
    std::optional<std::string> refused;   // nothing was queued
};

// The keys the engine puts down and lets up (the manual trace of Calc-U-59
// until M-53, D-13; tests).
class KeyObserver {
public:
    virtual void keyChanged(const machine::KeyId&, bool down, machine::Ticks) = 0;

protected:
    ~KeyObserver() = default;
};

class TypingEngine final : public sessions::SliceHook {
public:
    // The machine session it serves: before each act and drop the engine
    // checks the session's machine generation and calls `resync` when the
    // machine has changed since setMachine (the part sets the new one).
    void bind(sessions::MachineSession&, std::function<void()> resync);
    // A new or replaced machine (EC-5): everything it held is forgotten,
    // nothing waits, a pending token is not done.
    void setMachine(machine::Machine*, const TypingProfile*, const KeyboardMap*);
    machine::Machine* machine() const { return machine_; }
    const TypingProfile* profile() const { return profile_; }
    const KeyboardMap* keyboardMap() const { return map_; }

    // ── Live input ──
    void hostKeyDown(const HostKeyEvent&);
    void hostKeyUp(const std::string& physical);
    void hostText(std::u32string_view);   // EC-4
    void screenKeyDown(const machine::KeyId&, std::uint32_t pointer);
    void screenKeyUp(const machine::KeyId&, std::uint32_t pointer);
    void focusLost();   // AC-10

    // ── Typed text and commands ──
    void paste(std::string_view utf8);   // AC-12, AC-34: first line, unknown left out, no ENTER, appended
    // AC-15: the presses in turn. firstPressAtOnce: LIB-6 AC-58 (after `skip-wait:`).
    Queued typeKeys(std::vector<Press>, Origin, bool firstPressAtOnce = false);
    // AC-16: the text (and ENTER); with refuseUnknown a character without a
    // key refuses it all, else it is left out.
    Queued typeText(std::string_view utf8, bool enter, bool refuseUnknown, Origin, bool firstPressAtOnce = false);
    // The token's result, once its last key is up or it was dropped; taken once.
    std::optional<TypeResult> takeResult(Token);
    void setOnFinished(std::function<void(Token, const TypeResult&)> f) { onFinished_ = std::move(f); }
    void setOnHeldChanged(std::function<void()> f) { onHeld_ = std::move(f); }
    void setObserver(KeyObserver* o) { observer_ = o; }

    // EC-5 (LIB-18 §3.4, D-12): every key the engine holds up now, nothing waits.
    void drop() override;
    bool idle() const;              // nothing down, nothing waiting, the machine ready again where the profile asks
    std::size_t waiting() const { return queue_.size(); }
    std::vector<machine::KeyId> liveHeld() const;   // AC-19
    std::vector<machine::KeyId> down() const;       // every key the engine holds down

    // AC-20: the machine is paused (a freeze, LIB-10); a held key's rest of
    // the hold passes in the host's time, in virtual time at once.
    void setPaused(bool paused, pacing::WallTime now);
    void setVirtualTime(bool v) { virtual_ = v; }
    void hostCall(pacing::WallTime now);

    // SliceHook
    machine::Ticks nextDue(machine::Ticks now) const override;
    void act(machine::Machine&, machine::Ticks now, pacing::WallTime) override;

private:
    struct Item {
        bool isChar = false;
        Press press;
        char32_t ch = 0;
        Origin origin = Origin::Live;
        bool liveSingle = false;            // a live key: no readiness wait, kept by later live input
        std::vector<std::string> holders;   // liveSingle: who holds it; empty: one press of minHold
        bool firstOfStep = false;
        bool atOnce = false;
        std::optional<machine::Ticks> boundAt;   // AC-14: starts all the same at this tick
        Token token = 0;
        bool last = false;                  // the token's last press
    };
    struct Down {
        machine::KeyId key;
        machine::Ticks at = 0;
        machine::Ticks hold = 0;
        std::vector<std::string> holders;
        bool liveSingle = false;
        bool live = false;   // shown in held-keys
        bool releaseAsked = false;
        std::optional<std::uint64_t> samplesAt;
        Token token = 0;
        bool last = false;
        std::optional<pacing::WallTime> wallDue;   // paused: comes up at this host time
    };

    machine::Ticks now() const { return machine_ ? machine_->tickCount() : 0; }
    bool current();   // the machine is still the one set; resyncs if not
    TypingSupport* support() const;
    void run(machine::Ticks now);       // runOnce, then the next poll
    void runOnce(machine::Ticks now);   // releases, then the next press
    bool waitsForReadiness(const Item&, machine::Ticks now) const;
    bool polling(machine::Ticks now) const;
    bool releaseDue(const Down&, machine::Ticks now) const;
    void release(std::size_t index, machine::Ticks now);
    void requestRelease(Down&);
    bool blocked(machine::Ticks now, bool forLive) const;
    bool startFront(machine::Ticks now);
    void keyDown(const machine::KeyId&, machine::Ticks now);
    void dropWaiting(const char* reason, bool keepLiveSingles);
    void finish(Token, TypeResult);
    void heldChanged();
    void liveDown(const machine::KeyId&, const std::string& holder);
    void liveUp(const std::string& holder);
    void queueLiveSequence(const std::vector<Press>&);
    Token nextToken() { return ++tokens_; }
    machine::Ticks effHold(machine::Ticks h) const;

    sessions::MachineSession* ms_ = nullptr;
    std::function<void()> resync_;
    std::uint64_t gen_ = 0;
    machine::Machine* machine_ = nullptr;
    const TypingProfile* profile_ = nullptr;
    const KeyboardMap* map_ = nullptr;
    std::vector<Down> down_;
    std::deque<Item> queue_;
    machine::Ticks gapUntil_ = 0;
    machine::Ticks pollAt_ = sessions::kNever;
    bool settling_ = false;   // waitsForReadinessAfterRelease: not ready again yet
    bool paused_ = false;
    bool virtual_ = false;
    pacing::WallTime wall_{0};
    Token tokens_ = 0;
    std::map<Token, TypeResult> results_;
    std::vector<Token> open_;   // tokens not finished
    std::function<void(Token, const TypeResult&)> onFinished_;
    std::function<void()> onHeld_;
    KeyObserver* observer_ = nullptr;
    std::vector<machine::KeyId> lastLive_;
};

// UTF-8 to code points (a malformed byte is left out), and back.
std::u32string decodeUtf8(std::string_view);
std::string encodeUtf8(char32_t);

} // namespace calcu::typing
