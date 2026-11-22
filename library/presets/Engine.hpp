#pragma once
#include "machine/Display.hpp"
#include "machine/Machine.hpp"
#include "presets/ProductRules.hpp"
#include "presets/Reader.hpp"
#include "sessions/Operation.hpp"
#include "sessions/SessionPart.hpp"
#include "startup/MachineControl.hpp"
#include "typing/TypingPart.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <string>

// Carrying a preset out: the build, the items in file order, the implicit
// wait, `skip-wait:`, `let-it-finish:`, `timeout:`, the end at regular speed.
// One operation of the app session from the command until its last item has
// been carried out. It reads no wall clock; the only wall time is
// OperationContext::hostWait (`wait:` while paused).

namespace calcu::presets {

// The baseline of `elapsed-time:`: the tick of the last `start` on the
// machine of that generation; none: from tick 0 of the machine the preset
// built.
struct ElapsedBase {
    std::uint64_t generation = 0;
    std::optional<machine::Ticks> tick;
};

// What the engine works with: the part's (PresetsPart).
struct EngineDeps {
    startup::MachineControl* control = nullptr;   // startup's
    typing::TypingPart* typing = nullptr;         // typing's
    std::function<std::optional<std::string>()> screenshotFolder;   // the settings' Paths; the harness: the run's
    ElapsedBase* elapsed = nullptr;                                 // `elapsed-time: start`
};

struct LoadOptions {
    bool buildOnly = false;   // build the machine, carry out no item
    bool startup = false;     // through PresetLoader::load: BuildOrigin::StartupPreset
};

struct LoadOutcome {
    enum class Kind { Done, NotBuilt, Failed } kind = Kind::Done;   // NotBuilt: the build was refused
    std::string message;
};

// What an item sees while it is carried out.
class RunContext {
public:
    RunContext(sessions::OperationContext&, const LoadPlan*, ModelRules&, EngineDeps&);
    sessions::OperationContext& op() { return op_; }
    sessions::SessionContext& session() { return op_.session(); }
    machine::Machine* machine() { return op_.session().machine(); }
    const LoadPlan* plan() const { return plan_; }   // nullptr for a step on its own (run-step)
    ModelRules& rules() { return rules_; }
    // The item before was followed by `skip-wait:`.
    bool firstPressAtOnce() const { return firstPressAtOnce_; }
    void setFirstPressAtOnce(bool b) { firstPressAtOnce_ = b; }
    // `timeout:` in ticks, for an item that waits itself (After::Own).
    std::optional<machine::Ticks> deadline() const { return deadline_; }
    void setDeadline(std::optional<machine::Ticks> d) { deadline_ = d; }
    // The last item of a load without `let-it-finish:`: an item that
    // waits itself does not wait (Calc-U-59: a `card:` the ROM has not
    // taken yet stays at the reader).
    bool notWaitedFor() const { return notWaitedFor_; }
    void setNotWaitedFor(bool b) { notWaitedFor_ = b; }
    sessions::Task<bool> untilRunning();   // while paused, in the host's time
    // What a step gives on purpose (`elapsed-time: report`): at every level.
    void log(std::string line) { op_.session().log(std::move(line)); }
    // How the preset is carried out: at Info.
    void progress(std::string line) { op_.session().logAt(sessions::LogLevel::Info, std::move(line)); }
    typing::TypingEngine* typing() { return deps_.typing ? &deps_.typing->engine() : nullptr; }
    // The display as it is at this moment.
    machine::DisplayOutput display();
    std::optional<std::string> screenshotFolder() const {
        return deps_.screenshotFolder ? deps_.screenshotFolder() : std::nullopt;
    }
    ElapsedBase& elapsed();

private:
    sessions::OperationContext& op_;
    const LoadPlan* plan_;
    ModelRules& rules_;
    EngineDeps& deps_;
    bool firstPressAtOnce_ = false;
    std::optional<machine::Ticks> deadline_;
    bool notWaitedFor_ = false;
};

sessions::Task<LoadOutcome> carryOut(sessions::OperationContext&, std::shared_ptr<const LoadPlan>, LoadOptions,
                                     EngineDeps&);

// The machine plan of a load plan.
startup::MachinePlan machinePlan(const LoadPlan&);

// `m:ss.mmm`, the minutes on beyond 59.
std::string elapsedText(machine::Ticks ticks, machine::TickRate rate);

} // namespace calcu::presets
