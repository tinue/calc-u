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

// Carrying a preset out (LIB-6 design §3.6; spec AC-10 to AC-19, AC-55 to
// AC-61, EC-2 to EC-4, EC-6, EC-10): the build, the items in file order, the
// implicit wait, `skip-wait:`, `let-it-finish:`, `timeout:`, the end at
// regular speed. One operation of the app session from the command until
// its last item has been carried out (AC-12). It reads no wall clock; the
// only wall time is OperationContext::hostWait (`wait:` while paused).

namespace calcu::presets {

// The baseline of `elapsed-time:` (D-10): the tick of the last `start` on
// the machine of that generation; none: from tick 0 of the machine.
struct ElapsedBase {
    std::uint64_t generation = 0;
    std::optional<machine::Ticks> tick;
};

// What the engine works with: the part's (PresetsPart).
struct EngineDeps {
    startup::MachineControl* control = nullptr;   // LIB-18's
    typing::TypingPart* typing = nullptr;         // LIB-5's
    std::function<std::optional<std::string>()> screenshotFolder;   // LIB-9's; the harness: the run's
    ElapsedBase* elapsed = nullptr;                                 // `elapsed-time: start` (D-10)
};

struct LoadOptions {
    bool buildOnly = false;   // AC-55
    bool startup = false;     // through PresetLoader::load: BuildOrigin::StartupPreset (D-24)
};

struct LoadOutcome {
    enum class Kind { Done, NotBuilt, Failed } kind = Kind::Done;   // NotBuilt: the build was refused (AC-52)
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
    // The item before was followed by `skip-wait:` (AC-58; LIB-5 AC-14).
    bool firstPressAtOnce() const { return firstPressAtOnce_; }
    void setFirstPressAtOnce(bool b) { firstPressAtOnce_ = b; }
    // `timeout:` in ticks, for an item that waits itself (After::Own; AC-57).
    std::optional<machine::Ticks> deadline() const { return deadline_; }
    void setDeadline(std::optional<machine::Ticks> d) { deadline_ = d; }
    sessions::Task<bool> untilRunning();   // AC-19
    void log(std::string line) { op_.session().log(std::move(line)); }
    typing::TypingEngine* typing() { return deps_.typing ? &deps_.typing->engine() : nullptr; }
    // The display as it is at this moment (AC-25, AC-24).
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
};

sessions::Task<LoadOutcome> carryOut(sessions::OperationContext&, std::shared_ptr<const LoadPlan>, LoadOptions,
                                     EngineDeps&);

// The machine plan of a load plan (LIB-18 §3.1).
startup::MachinePlan machinePlan(const LoadPlan&);

// `m:ss.mmm`, the minutes on beyond 59 (AC-23).
std::string elapsedText(machine::Ticks ticks, machine::TickRate rate);

} // namespace calcu::presets
