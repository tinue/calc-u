#pragma once
#include "roms/Lookup.hpp"
#include "roms/RomSet.hpp"
#include "sessions/Payload.hpp"
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

// The startup session part: the commands that
// change the machine and the start of a product, each an operation of the
// app session. PeripheralsPart comes before it in AppSessionSetup::parts, so
// that the real-time clocks get the host's time at a build and at the end of
// every operation.
//
// Commands (library names, no prefix): `start {model?, preset?}`,
// `model {name}`, `variant {name}`, `attach {place, peripheral, variant?}`,
// `detach {place}`, `reset`, `cold-boot`, `power-cycle`. `model`, `reset`
// and `cold-boot` end a running operation (only when the handler starts
// one); the others are refused while one runs, even where they would be
// Done.

namespace calcu::startup {

inline const sessions::CommandName kModelCommand{"model"};
inline const sessions::CommandName kVariantCommand{"variant"};
inline const sessions::CommandName kAttachCommand{"attach"};
inline const sessions::CommandName kDetachCommand{"detach"};
inline const sessions::CommandName kResetCommand{"reset"};
inline const sessions::CommandName kColdBootCommand{"cold-boot"};
inline const sessions::CommandName kPowerCycleCommand{"power-cycle"};
// `start` is sessions::kStartCommand.

inline constexpr const char* kStartedAlready = "The product has started already.";   // `start` is accepted once per app session

struct StartupSetup {
    MachineControl& control;              // the product's machines and the full ROM set
    settings::SettingsFile& settings;     // [startup] model, last-model; [preset]
    PresetLoader* loader = nullptr;       // the PresetsPart; none: [preset] is not read
    // The ROM lookup found no full set: the start is refused
    // with its text (the files that are missing), and the topic
    // `roms-missing` holds it for the frontends.
    std::optional<roms::RomsMissing> romsMissing;
};

// The topic `roms-missing` {text}, set only without the full set.
inline const sessions::TopicName kRomsMissingTopic{"roms-missing"};
struct RomsMissingState final : sessions::PayloadOf<RomsMissingState> {
    static constexpr sessions::PayloadKind kKind{"roms-missing"};
    roms::RomsMissing missing;
    std::string text;   // roms::describe(missing)
    bool same(const RomsMissingState& o) const { return text == o.text; }
    sessions::Value toValue() const override { return sessions::Value::object({{"text", text}}); }
};

class StartupPart final : public sessions::SessionPart {
public:
    explicit StartupPart(StartupSetup);
    MachineControl& control() { return setup_.control; }

    void attach(sessions::SessionSetup&) override;
    void machineChanged(sessions::SessionContext&) override;   // [startup] last-model
    void operationEnded(sessions::SessionContext&) override;   // regular speed after a build

private:
    enum class Restart { Reset, ColdBoot, PowerCycle };
    // The start for `model`: its startup preset, else its built-in default.
    sessions::Task<sessions::OperationResult> buildFor(sessions::OperationContext&, machine::ModelId model,
                                                       std::string preset, BuildOrigin origin);
    sessions::Task<sessions::OperationResult> switchVariant(sessions::OperationContext&, machine::VariantId);
    sessions::Task<sessions::OperationResult> changePlug(sessions::OperationContext&, PlugChange);
    sessions::Task<sessions::OperationResult> restart(sessions::OperationContext&, Restart);
    sessions::Task<sessions::OperationResult> builtInDefault(sessions::OperationContext&, machine::ModelId,
                                                             BuildOrigin);

    StartupSetup setup_;
    bool started_ = false;
};

} // namespace calcu::startup
