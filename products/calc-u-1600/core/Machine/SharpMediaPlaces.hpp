#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../Connector/ExpansionCard.hpp"
#include "../Connector/MemoryCardDefinition.hpp"
#include "../Tape/TapeDeck.hpp"
#include "machine/ProductData.hpp"
#include "media/MediaPlace.hpp"

// Calc-U-1600's media places and module media (LIB-7 design §10.4; spec
// AC-38, AC-39, AC-42; capabilities.md, the media kind): the CE-1600F drive
// and the cassette of a CE-150 or CE-1600P are holding media places
// (`calcu::media::MediaPlace`), a battery-backed module in a slot is a
// module medium (`calcu::media::ModuleMedium`). The content they take and
// give is one of the three content types below, which the product's codecs
// (app/SharpMediaKinds) read from and write to the files; the library
// carries it unread (AC-46).

namespace sharp {

// A battery-backed module: its definition, whose initial content is the
// module's content (what its file holds, or the RAM as the machine left
// it). A module request may carry it; moduleCardOf builds the card from it.
class ModuleContent final : public calcu::machine::ProductData {
public:
    explicit ModuleContent(MemoryCardDefinition d) : definition(std::move(d)) {}
    MemoryCardDefinition definition;
    bool writeProtected() const;
};

// A floppy disk: both sides, CE1600FCard::kImageSize bytes, side 0 first.
class FloppyContent final : public calcu::machine::ProductData {
public:
    explicit FloppyContent(std::vector<std::uint8_t> i) : image(std::move(i)) {}
    std::vector<std::uint8_t> image;
};

// A cassette tape: the bytes of its sound file (WAV). A tape without a
// sample is blank: inserted, it records (today's "new tape to record");
// one with samples plays.
class TapeContent final : public calcu::machine::ProductData {
public:
    explicit TapeContent(std::vector<std::uint8_t> w) : wav(std::move(w)) {}
    std::vector<std::uint8_t> wav;
};

// `def` with `image` (every region's backing store in definition order, as
// ExpansionCard::debugImage() gives it) as its initial content, and the
// write-protect switch in that position.
MemoryCardDefinition definitionWithContent(const MemoryCardDefinition& def, const std::vector<std::uint8_t>& image,
                                           bool writeProtected);

// The CE-1600F's disk drive: the media place `floppy` of the CE-1600P.
class FloppyPlace final : public calcu::media::MediaPlace {
public:
    struct Drive {   // the core's CE-1600F, through the machine
        std::function<bool()> attached, hasDisk, motorOn;
        std::function<bool(const std::uint8_t*, std::size_t)> load;
        std::function<void()> eject;
        std::function<std::vector<std::uint8_t>()> image;
        std::function<std::uint64_t()> revision;
        std::function<int()> side;
        std::function<void(int)> setSide;
    };
    explicit FloppyPlace(Drive d) : m_drive(std::move(d)) {}
    calcu::machine::MediaPlaceId id() const override { return calcu::machine::MediaPlaceId{"floppy"}; }
    Inserted insert(calcu::media::Content, int side) override;
    calcu::media::Content eject() override;
    std::optional<int> side() const override;
    calcu::media::Content content() const override;
    std::uint64_t revision() const override { return m_drive.revision(); }
    bool waitsForMedium() const override { return false; }

private:
    Drive m_drive;
};

// The cassette recorder of a CE-150 or CE-1600P: the media place `cassette`.
// Recording onto a blank tape keeps the sound in the place; it is written
// each time the motor stops and when the tape leaves (writesWhenAsked, D-11).
class CassettePlace final : public calcu::media::MediaPlace {
public:
    explicit CassettePlace(TapeDeck& deck) : m_deck(deck) {}
    calcu::machine::MediaPlaceId id() const override { return calcu::machine::MediaPlaceId{"cassette"}; }
    Inserted insert(calcu::media::Content, int side) override;
    calcu::media::Content eject() override;
    std::optional<int> side() const override;
    calcu::media::Content content() const override;
    std::uint64_t revision() const override;
    bool waitsForMedium() const override { return false; }
    bool takeWriteNow() override { return m_deck.takeStopped(); }
    bool writesWhenAsked() const override { return true; }

private:
    TapeDeck& m_deck;
    std::shared_ptr<const TapeContent> m_played;   // the tape in play mode, as it came
};

// A battery-backed module in a slot (LIB-7 D-1): its content and revision.
class SlotModuleMedium final : public calcu::media::ModuleMedium {
public:
    explicit SlotModuleMedium(std::function<const ExpansionCard*()> card) : m_card(std::move(card)) {}
    calcu::media::Content content() const override;
    std::uint64_t revision() const override;
    // A battery-backed software-defined module is in the slot now.
    bool present() const;

private:
    std::function<const ExpansionCard*()> m_card;
};

} // namespace sharp
