#pragma once
#include "startup/MachineControl.hpp"
#include "startup/PresetLoader.hpp"
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// The stand-in for LIB-6's loader (LIB-18 design §2; spec: "the Library
// column shows the criteria that use a startup preset with a stand-in for the
// loader"). A preset is a name with a scripted plan: it builds the plan
// through MachineControl (BuildOrigin::StartupPreset) and runs `steps` ticks,
// or cannot be read, or fails after building. A name it does not know is a
// file that is gone.

namespace calcu::startup::test {

class TestPresetLoader final : public PresetLoader {
public:
    enum class Script { Build, Outdated, FailAfterBuild };
    struct Preset {
        MachinePlan plan;
        Script script = Script::Build;
        machine::Ticks steps = 1'000;   // what its steps run
    };

    sessions::Task<PresetLoad> load(std::string ref, std::optional<machine::ModelId> forModel, MachineControl& control,
                                    sessions::OperationContext& c) override {
        calls.emplace_back(ref, forModel);
        const auto it = presets.find(ref);
        if (it == presets.end()) co_return PresetLoad{PresetLoad::Outcome::CannotLoad, "the file " + ref + " is gone"};
        const Preset p = it->second;
        if (p.script == Script::Outdated)
            co_return PresetLoad{PresetLoad::Outcome::CannotLoad, "the file " + ref + " is outdated"};
        if (forModel && p.plan.model != *forModel)
            co_return PresetLoad{PresetLoad::Outcome::CannotLoad, "it is for the " + p.plan.model.value};
        if (auto refused = control.build(c.session(), p.plan, BuildOrigin::StartupPreset))
            co_return PresetLoad{PresetLoad::Outcome::CannotLoad, refused->reason};
        co_await c.run(p.steps);
        if (p.script == Script::FailAfterBuild)
            co_return PresetLoad{PresetLoad::Outcome::FailedWhileRunning, "the tape broke"};
        co_return PresetLoad{};
    }

    std::map<std::string, Preset> presets;
    std::vector<std::pair<std::string, std::optional<machine::ModelId>>> calls;
};

} // namespace calcu::startup::test
