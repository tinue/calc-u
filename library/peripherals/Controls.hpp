#pragma once
#include "machine/Attach.hpp"
#include "machine/Capability.hpp"
#include "machine/Descriptor.hpp"
#include "machine/Machine.hpp"
#include <functional>
#include <vector>

// The generic kind controls (LIB-2 design §3.2; spec AC-14 to AC-16, EC-6;
// capabilities.md C-1): the keys and switches of a peripheral or component
// that are not machine keys. A control is named by its owner (the
// capability's) and its ID, unique within that owner. What a control does,
// and when, is its owner's.

namespace calcu::peripherals {

// key: on = down; switch: on.
struct ControlInfo {
    machine::ControlId id;
    machine::ControlForm form;
    bool on = false;
    bool operator==(const ControlInfo&) const = default;
};

// Refused (UnknownControl, WrongForm): nothing changes (EC-6). WrongForm is a
// switch pressed or a key set.
enum class ControlCall { Done, UnknownControl, WrongForm };

class Controls : public machine::Capability {
public:
    static constexpr machine::KindId kKind{"controls"};
    machine::KindId kind() const override { return kKind; }

    virtual std::vector<ControlInfo> list() const = 0;               // in the descriptor's order (AC-14)
    virtual ControlCall press(const machine::ControlId&) = 0;        // a key, at the current tick count (AC-15)
    virtual ControlCall release(const machine::ControlId&) = 0;
    virtual ControlCall set(const machine::ControlId&, bool on) = 0;  // a switch (AC-16)
};

// A ready-made implementation for cores: the controls of a descriptor, all
// up and off at first. OnChange is the owner's reaction, called on every
// change of a key or a switch (not on a press of a key that is down).
class ControlTable final : public Controls {
public:
    using OnChange = std::function<void(const machine::ControlId&, bool on)>;
    ControlTable(std::vector<machine::ControlDescriptor>, OnChange);

    std::vector<ControlInfo> list() const override { return controls_; }
    ControlCall press(const machine::ControlId& id) override { return change(id, machine::ControlForm::Key, true); }
    ControlCall release(const machine::ControlId& id) override { return change(id, machine::ControlForm::Key, false); }
    ControlCall set(const machine::ControlId& id, bool on) override {
        return change(id, machine::ControlForm::Switch, on);
    }

    // At plug: the switch positions of the request (D-3); without the owner's
    // reaction, since they are where the peripheral starts. Unknown IDs and
    // keys are ignored.
    void restore(const std::vector<machine::SwitchPosition>&);
    // The state of a control; false for an unknown ID.
    bool on(const machine::ControlId&) const;

private:
    ControlCall change(const machine::ControlId&, machine::ControlForm, bool on);

    std::vector<ControlInfo> controls_;
    OnChange onChange_;
};

// The controls an owner has in the machine now; empty if it has none
// (LIB-4 AC-21 reads them from a const machine).
std::vector<ControlInfo> controlsOf(const machine::Machine&, const machine::Owner&);
// The controls capability of an owner, or nullptr.
Controls* controlsCapability(machine::Machine&, const machine::Owner&);

} // namespace calcu::peripherals
