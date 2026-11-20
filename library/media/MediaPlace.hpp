#pragma once
#include "machine/Capability.hpp"
#include "machine/Ids.hpp"
#include "media/Kind.hpp"
#include <cstdint>
#include <optional>
#include <string>

// The media kind: two capability interfaces. A media place of a descriptor
// (MachineDescriptor::mediaPlaces, PeripheralDescriptor::mediaPlaces) has
// one MediaPlace, owned as its CapabilityEntry says. A memory module that is
// a medium is a peripheral in a module slot and offers a ModuleMedium; its
// write-protect switch is the controls kind's `write-protect`.

namespace calcu::media {

inline const machine::ControlId kWriteProtect{"write-protect"};

class MediaPlace : public machine::Capability {
public:
    static constexpr machine::KindId kKind{"media-place"};
    machine::KindId kind() const override { return kKind; }
    virtual machine::MediaPlaceId id() const = 0;   // its descriptor entry: kind, passage, sides
    struct Inserted {
        bool ok = true;
        std::string refusal;   // the content is the kind's; a refusal names why
    };
    virtual Inserted insert(Content, int side) = 0;   // in at once, no power cycle
    virtual Content eject() = 0;                      // what it holds now; nullptr when empty
    virtual std::optional<int> side() const = 0;      // std::nullopt: empty
    virtual Content content() const = 0;              // for saving
    virtual std::uint64_t revision() const = 0;       // changes with the content
    // The machine waits for a medium at this place now (the media state's
    // `waits`; the machine's own Machine::awaitsMedium() is the idle signal).
    virtual bool waitsForMedium() const = 0;
    virtual std::optional<bool> writeProtected() const { return std::nullopt; }   // nullopt: no switch
    virtual void setWriteProtected() {}               // on, never off
    // Pass-through: the medium has passed and is given back.
    struct GivenBack {
        Content content;
        bool written = false;
    };
    virtual std::optional<GivenBack> takeGivenBack() { return std::nullopt; }
    // Holding: the machine set the moment of a write (a tape: the motor stopped).
    virtual bool takeWriteNow() { return false; }
    // Holding: true when an instance is written only at the moments
    // takeWriteNow() sets and at the writes that do not wait, not half a
    // second after each change: a tape, whose recording is written each time
    // the motor stops and at eject.
    virtual bool writesWhenAsked() const { return false; }
};

class ModuleMedium : public machine::Capability {
public:
    static constexpr machine::KindId kKind{"module-medium"};
    machine::KindId kind() const override { return kKind; }
    virtual Content content() const = 0;
    virtual std::uint64_t revision() const = 0;
};

} // namespace calcu::media
