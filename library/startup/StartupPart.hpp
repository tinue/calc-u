#pragma once
#include "roms/RomSet.hpp"
#include "sessions/Messages.hpp"
#include "sessions/Operation.hpp"
#include "sessions/SessionPart.hpp"
#include "settings/SettingsFile.hpp"
#include "startup/MachineControl.hpp"
#include "startup/PresetLoader.hpp"
#include "startup/ProductMachines.hpp"
#include <memory>
#include <optional>
#include <string>

// LIB-18's session part (design §3.5; spec AC-3 to AC-16): the commands that
// change the machine and the start of a product, each an operation of the
// app session. PeripheralsPart comes before it in AppSessionSetup::parts, so
// that the real-time clocks get the host's time at a build and at the end of
// every operation (AC-1, AC-13; LIB-2 AC-23).
//
// Commands (library names, no prefix; D-2): `start {model?, preset?}`,
// `model {name}`, `variant {name}`, `attach {place, peripheral, variant?}`,
// `detach {place}`, `reset`, `cold-boot`, `power-cycle`. `model`, `reset`
// and `cold-boot` end a running operation (AC-16, D-6); the others are
// refused while one runs (D-7).

namespace calcu::startup {

inline const sessions::CommandName kModelCommand{"model"};
inline const sessions::CommandName kVariantCommand{"variant"};
inline const sessions::CommandName kAttachCommand{"attach"};
inline const sessions::CommandName kDetachCommand{"detach"};
inline const sessions::CommandName kResetCommand{"reset"};
inline const sessions::CommandName kColdBootCommand{"cold-boot"};
inline const sessions::CommandName kPowerCycleCommand{"power-cycle"};
// `start` is sessions::kStartCommand (LIB-15 N3).

inline constexpr const char* kStartedAlready = "The product has started already.";   // D-8

struct StartupSetup {
    ProductMachines& product;
    std::shared_ptr<const roms::RomSet> roms;   // the full set (LIB-12); the caller does not start without it
    settings::SettingsFile& settings;           // [startup] model, last-model; [preset]
    PresetLoader* loader = nullptr;             // nullptr until LIB-6 (D-9)
};

class StartupPart final : public sessions::SessionPart {
public:
    explicit StartupPart(StartupSetup);
    MachineControl& control() { return control_; }

    void attach(sessions::SessionSetup&) override;
    void machineChanged(sessions::SessionContext&) override;   // AC-9: [startup] last-model
    void operationEnded(sessions::SessionContext&) override;   // D-11: regular speed after a build

private:
    enum class Restart { Reset, ColdBoot, PowerCycle };
    // Step 2 and 3 of the start for `model` (AC-4 to AC-6, EC-1, EC-6).
    sessions::Task<sessions::OperationResult> buildFor(sessions::OperationContext&, machine::ModelId model,
                                                       std::string preset, BuildOrigin origin);
    sessions::Task<sessions::OperationResult> switchVariant(sessions::OperationContext&, machine::VariantId);
    sessions::Task<sessions::OperationResult> changePlug(sessions::OperationContext&, PlugChange);
    sessions::Task<sessions::OperationResult> restart(sessions::OperationContext&, Restart);
    sessions::Task<sessions::OperationResult> builtInDefault(sessions::OperationContext&, machine::ModelId,
                                                             BuildOrigin);

    StartupSetup setup_;
    MachineControl control_;
    bool started_ = false;
};

} // namespace calcu::startup
