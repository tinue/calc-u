#pragma once
#include "machine/Capability.hpp"
#include "machine/Ids.hpp"
#include "media/Kind.hpp"
#include <cstdint>
#include <optional>
#include <string>

// The media kind (LIB-7 design §3.3; spec AC-6; capabilities.md, the media
// kind; D-1): two capability interfaces. A media place of a descriptor
// (MachineDescriptor::mediaPlaces, PeripheralDescriptor::mediaPlaces) has
// one MediaPlace, owned as its CapabilityEntry says. A memory module that is
// a medium is a peripheral in a module slot and offers a ModuleMedium; its
// write-protect switch is the controls kind's `write-protect` (LIB-2 D-9).

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
    virtual Inserted insert(Content, int side) = 0;   // AC-10: in at once, no power cycle
    virtual Content eject() = 0;                      // what it holds now; nullptr when empty
    virtual std::optional<int> side() const = 0;      // std::nullopt: empty
    virtual Content content() const = 0;              // for saving
    virtual std::uint64_t revision() const = 0;       // changes with the content (AC-11)
    virtual bool waitsForMedium() const = 0;          // LIB-3 AC-22
    virtual std::optional<bool> writeProtected() const { return std::nullopt; }   // nullopt: no switch
    virtual void setWriteProtected() {}               // AC-19: on, never off
    // Pass-through: the medium has passed and is given back (AC-12).
    struct GivenBack {
        Content content;
        bool written = false;
    };
    virtual std::optional<GivenBack> takeGivenBack() { return std::nullopt; }
    // Holding: the machine set the moment of a write (a tape: the motor stopped, D-11).
    virtual bool takeWriteNow() { return false; }
    // Holding: true when an instance is written only at the moments
    // takeWriteNow() sets and at the writes that do not wait (A2, A5), not
    // half a second after each change (A1): a tape, whose recording is
    // written each time the motor stops and at eject (M-47; Calc-U-1600
    // AC-42, D-11).
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
