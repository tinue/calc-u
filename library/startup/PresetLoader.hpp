#pragma once
#include "machine/Ids.hpp"
#include "sessions/Operation.hpp"
#include <optional>
#include <string>

// The seam LIB-6 fills (LIB-18 design §3.3; spec AC-4, AC-6, EC-1, EC-2,
// EC-6). Until LIB-6 (M-38) no product has a loader; the tests use a
// stand-in (startup/tests/TestPresetLoader.hpp).

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
    // (EC-1). Otherwise it builds the preset's machine through
    // control.build(plan, StartupPreset), from a cold boot (ADR 0016), and
    // runs its steps; a failure from then on is FailedWhileRunning and leaves
    // the machine as far as it got (EC-6).
    virtual sessions::Task<PresetLoad> load(std::string ref, std::optional<machine::ModelId> forModel,
                                            MachineControl& control, sessions::OperationContext&) = 0;

protected:
    ~PresetLoader() = default;
};

} // namespace calcu::startup
