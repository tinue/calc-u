#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "Tape/TapeDeck.hpp"
#include "app/ControlState.hpp"
#include "roms/Lookup.hpp"
#include "sessions/Messages.hpp"
#include "sessions/Payload.hpp"

// Calc-U-1600's topics, commands and requests on the library's app session
// (LIB-3 design §4, §5: the interim product topics; spec AC-38). Each is
// carried by the library unread (AC-12) and re-pointed at its owner's topic
// in the owner's step: `c1600.control-state` LIB-4 1600 (M-49);
// `c1600.machine` LIB-4 / LIB-18 1600 (M-49 / M-46); `c1600.tapes` LIB-7
// 1600 (M-47); `c1600.serial-links` LIB-2 1600 (M-47). `roms-missing` is
// LIB-12's topic (LIB-12 design §3.5); its payload lives here until a
// second product sends it.

namespace shared {

namespace topics {
inline const calcu::sessions::TopicName kControlState{"c1600.control-state"};
inline const calcu::sessions::TopicName kMachine{"c1600.machine"};
inline const calcu::sessions::TopicName kTapes{"c1600.tapes"};
inline const calcu::sessions::TopicName kSerialLinks{"c1600.serial-links"};
inline const calcu::sessions::TopicName kRomsMissing{"roms-missing"};
} // namespace topics

// The commands. Those that start an operation are marked (op); `start`,
// `c1600.reset` and `c1600.pick-model` stop a running one (LIB-18 AC-16,
// ledger 1600-9). Arguments in braces.
namespace commands {
inline const calcu::sessions::CommandName kStart{"start"};                       // (op) {model?, preset?} LIB-15 N3
inline const calcu::sessions::CommandName kLoadPreset{"c1600.load-preset"};      // (op) {path, armOnly?}
inline const calcu::sessions::CommandName kLoadDefaultPreset{"c1600.load-default-preset"};   // (op) {path, model}
inline const calcu::sessions::CommandName kDefaultPreset{"c1600.default-preset"};            // (op)
inline const calcu::sessions::CommandName kReset{"c1600.reset"};                 // (op) {all}
inline const calcu::sessions::CommandName kPickModel{"c1600.pick-model"};        // (op) {model}
inline const calcu::sessions::CommandName kSelectModule{"c1600.select-module"};  // (op) {slot, name}
inline const calcu::sessions::CommandName kPC1500Rom{"c1600.pc1500-rom"};        // (op) {revision: A01|A03|A04}
inline const calcu::sessions::CommandName kPC1600Rom{"c1600.pc1600-rom"};        // (op) {version: old|new}
inline const calcu::sessions::CommandName kCE1600PRom{"c1600.ce1600p-rom"};      // (op) {version: old|new}
inline const calcu::sessions::CommandName kTogglePeripheral{"c1600.toggle-peripheral"};   // (op) {peripheral}
inline const calcu::sessions::CommandName kMountHostDrive{"c1600.mount-host-drive"};      // (op) {dir}
inline const calcu::sessions::CommandName kLoadBasic{"c1600.load-basic"};        // {bytes, path}
inline const calcu::sessions::CommandName kLoadMachineCode{"c1600.load-machine-code"};    // {bytes, addr, slot, bank}
} // namespace commands

// What the pickers, menus and panels offer now (ControlState).
struct ControlStatePayload final : calcu::sessions::PayloadOf<ControlStatePayload> {
    static constexpr calcu::sessions::PayloadKind kKind{"c1600.control-state"};
    explicit ControlStatePayload(ControlState s) : state(std::move(s)) {}
    ControlState state;
    bool same(const ControlStatePayload& o) const { return state == o.state; }
    calcu::sessions::Value toValue() const override;
};

// The machine: its model and how often it was rebuilt (0.8.1's
// machineReplaced, modelSwitched) and a module bay changed a slot
// (moduleChanged).
struct MachinePayload final : calcu::sessions::PayloadOf<MachinePayload> {
    static constexpr calcu::sessions::PayloadKind kKind{"c1600.machine"};
    Model model = Model::PC1500A;
    std::uint64_t builds = 0;          // grows with every machine built
    std::uint64_t modelSwitches = 0;   // grows with every model switch and preset load
    std::vector<std::uint64_t> moduleChanges = {0, 0, 0};   // per slot (1, 2)
    bool same(const MachinePayload& o) const {
        return model == o.model && builds == o.builds && modelSwitches == o.modelSwitches &&
               moduleChanges == o.moduleChanges;
    }
    calcu::sessions::Value toValue() const override;
};

// The tape deck and the tape list (0.8.1's tapesChanged, tapeStatus).
struct TapesPayload final : calcu::sessions::PayloadOf<TapesPayload> {
    static constexpr calcu::sessions::PayloadKind kKind{"c1600.tapes"};
    TapeDeck::Status status;
    std::uint64_t listChanges = 0;
    bool same(const TapesPayload& o) const {
        return status.mode == o.status.mode && status.motor == o.status.motor && status.position == o.status.position &&
               status.length == o.status.length && status.path == o.status.path && listChanges == o.listChanges;
    }
    calcu::sessions::Value toValue() const override;
};

// The serial port files (0.8.1's serialLinksRelinked; LIB-2 AC-33).
struct SerialLinksPayload final : calcu::sessions::PayloadOf<SerialLinksPayload> {
    static constexpr calcu::sessions::PayloadKind kKind{"c1600.serial-links"};
    std::string rs232c, ce158;   // the path a client opens, empty while there is none
    std::uint64_t moves = 0;
    bool same(const SerialLinksPayload& o) const { return rs232c == o.rs232c && ce158 == o.ce158 && moves == o.moves; }
    calcu::sessions::Value toValue() const override;
};

// A file of the ROM full set is missing or bad: no machine (LIB-12 AC-5).
struct RomsMissingPayload final : calcu::sessions::PayloadOf<RomsMissingPayload> {
    static constexpr calcu::sessions::PayloadKind kKind{"roms-missing"};
    explicit RomsMissingPayload(calcu::roms::RomsMissing m) : missing(std::move(m)) {}
    calcu::roms::RomsMissing missing;
    bool same(const RomsMissingPayload& o) const { return calcu::roms::describe(missing) == calcu::roms::describe(o.missing); }
    calcu::sessions::Value toValue() const override;
};

} // namespace shared
