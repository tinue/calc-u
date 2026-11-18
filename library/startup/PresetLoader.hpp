#pragma once
#include "machine/Ids.hpp"
#include "sessions/Operation.hpp"
#include <optional>
#include <string>

// The seam the presets part (PresetsPart) fills: it loads a startup preset.
// The tests use a stand-in (startup/tests/TestPresetLoader.hpp).

namespace calcu::startup {

class MachineControl;

struct PresetLoad {
    enum class Outcome { Built, CannotLoad, FailedWhileRunning };
    Outcome outcome = Outcome::Built;
    std::string reason;   // an English sentence (CannotLoad, FailedWhileRunning)
};

class PresetLoader {
public:
    // Reads the preset `ref` (a bundled path or the [preset] value). On a read
    // error, an outdated file or a preset for another model than `forModel`
    // (when given), or a build that is refused: CannotLoad, nothing built
    // Otherwise it builds the preset's machine through
    // control.build(plan, StartupPreset), from a cold boot (a preset always cold boots), and
    // runs its steps; a failure from then on is FailedWhileRunning and leaves
    // the machine as far as it got.
    virtual sessions::Task<PresetLoad> load(std::string ref, std::optional<machine::ModelId> forModel,
                                            MachineControl& control, sessions::OperationContext&) = 0;

protected:
    ~PresetLoader() = default;
};

} // namespace calcu::startup
