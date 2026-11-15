#pragma once
#include "harness/MachineState.hpp"
#include "sessions/Payload.hpp"
#include "sessions/SessionPart.hpp"
#include "sessions/Topics.hpp"
#include <cstdint>
#include <string>
#include <vector>

// The commands of the kind machine state on the app session (LIB-11 design
// T18; spec AC-24): `machine-state.save` gives the state with the header the
// library writes; `machine-state.restore {bytes}` checks the header against
// the machine now (product, version, model, variant, plugged peripherals)
// and refuses a file of another configuration before the machine sees it.

namespace calcu::harness {

inline const sessions::CommandName kStateSaveCommand{"machine-state.save"};
inline const sessions::CommandName kStateRestoreCommand{"machine-state.restore"};

struct StateBytes final : sessions::PayloadOf<StateBytes> {
    static constexpr sessions::PayloadKind kKind{"machine-state"};
    explicit StateBytes(std::vector<std::uint8_t> b) : bytes(std::move(b)) {}
    std::vector<std::uint8_t> bytes;
    bool same(const StateBytes& o) const { return bytes == o.bytes; }
    sessions::Value toValue() const override { return sessions::Value(bytes); }
};

class MachineStatePart final : public sessions::SessionPart {
public:
    MachineStatePart(std::string product, std::string version)
        : product_(std::move(product)), version_(std::move(version)) {}
    void attach(sessions::SessionSetup&) override;

    // The header for the machine now (one line per field, then an empty line).
    std::string header(const machine::Machine&) const;

private:
    std::string product_, version_;
    sessions::StateTopic* display_ = nullptr;
};

} // namespace calcu::harness
