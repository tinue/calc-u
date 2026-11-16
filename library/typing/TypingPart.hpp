#pragma once
#include "machine/Ids.hpp"
#include "sessions/AppSession.hpp"
#include "sessions/SessionPart.hpp"
#include "typing/KeyboardMap.hpp"
#include "typing/TypingEngine.hpp"
#include "typing/TypingProfile.hpp"
#include <cstdint>
#include <map>
#include <optional>
#include <string_view>
#include <vector>

// Typing in the app session (LIB-5 design §3.5; spec AC-15 to AC-19, AC-38,
// AC-39): the session part that holds the typing engine, adds it as a hook
// of the machine session, declares the commands, the topic `held-keys` and
// the request `typing` (TypingTopics.hpp), and sets the product's profile
// and map for each machine. It goes before the product's part in the
// session's parts (LIB-3 AC-12).

namespace calcu::typing {

// What a product hands the part: a profile and a keyboard map per model.
class TypingProduct {
public:
    virtual const TypingProfile* profile(const machine::ModelId&) const = 0;
    virtual const KeyboardMap* keyboardMap(const machine::ModelId&) const = 0;   // nullptr: no host keys
    // D-8: the product's models type text (`type` is declared).
    virtual bool typesText() const = 0;

protected:
    ~TypingProduct() = default;
};

// AC-20: whether the machine is paused for typing (a freeze); by default
// MachineSession::paused(). Calc-U-59's TI freeze until M-53 (TIFreezeProbe).
class PauseProbe {
public:
    virtual bool paused(sessions::SessionContext&) const = 0;

protected:
    ~PauseProbe() = default;
};

class TypingPart final : public sessions::SessionPart {
public:
    explicit TypingPart(const TypingProduct&, sessions::Hosting = sessions::Hosting::Paced);
    TypingPart(const TypingPart&) = delete;
    TypingPart& operator=(const TypingPart&) = delete;

    TypingEngine& engine() { return engine_; }
    const TypingEngine& engine() const { return engine_; }
    // The product's profiles, for a model before its machine is built (LIB-6:
    // `key` and `type` are checked when a preset is read).
    const TypingProduct& product() const { return product_; }
    void setPauseProbe(const PauseProbe* p) { probe_ = p; }

    // The presses of a `key` name on the current model (AC-15), or nullopt.
    std::optional<std::vector<Press>> pressesOf(std::string_view name) const;

    // SessionPart
    void attach(sessions::SessionSetup&) override;
    void beforeCall(sessions::SessionContext&, pacing::WallTime) override;
    void afterCall(sessions::SessionContext&, pacing::WallTime) override;
    void machineChanged(sessions::SessionContext&) override;

private:
    void sync();   // the machine and the pause, before anything else
    void heldChanged();
    void finished(Token, const TypeResult&);
    sessions::CommandOutcome deferred(const Queued&);

    const TypingProduct& product_;
    sessions::Hosting hosting_;
    TypingEngine engine_;
    const PauseProbe* probe_ = nullptr;
    sessions::SessionContext* ctx_ = nullptr;
    // The machine generation the engine was last set to: a second
    // machineChanged for the same machine (a session in virtual time tells
    // the parts after an operation that built it, when the engine has
    // followed it already) keeps what is typed (C1600-3: the DAP's set-up).
    std::optional<std::uint64_t> engineGeneration_;
    sessions::StateTopic* held_ = nullptr;
    std::map<Token, sessions::CallId> calls_;
    pacing::WallTime wall_{0};
};

} // namespace calcu::typing
