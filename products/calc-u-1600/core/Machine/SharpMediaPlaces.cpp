#include "Machine/SharpMediaPlaces.hpp"

#include <string>

#include "../Audio/WavFile.hpp"
#include "../Connector/CE1600FCard.hpp"
#include "../Connector/SoftwareDefinedCard.hpp"

namespace sharp {

using calcu::media::Content;

bool ModuleContent::writeProtected() const {
    for (const Region& r : definition.regions)
        for (std::uint32_t b = 0; b < r.banking.bankCount; ++b)
            if (r.contentForBank(b).hasWriteProtect && r.contentForBank(b).writeProtected) return true;
    return false;
}

MemoryCardDefinition definitionWithContent(const MemoryCardDefinition& def, const std::vector<std::uint8_t>& image,
                                           bool writeProtected) {
    MemoryCardDefinition out = def;
    std::size_t offset = 0;
    for (Region& r : out.regions) {
        const std::uint32_t banks = r.banking.bankCount ? r.banking.bankCount : 1;
        const std::size_t size = r.banking.bankSize ? r.banking.bankSize : r.capacity;
        for (std::uint32_t b = 0; b < banks; ++b) {
            const std::size_t from = offset + std::size_t(b) * size;
            if (from + size > image.size()) break;
            r.initialContentByBank[b] = std::vector<std::uint8_t>(image.begin() + static_cast<std::ptrdiff_t>(from),
                                                                  image.begin() + static_cast<std::ptrdiff_t>(from + size));
        }
        offset += r.capacity;
        if (r.content.hasWriteProtect) r.content.writeProtected = writeProtected;
        for (BankContentRange& c : r.contentByBank)
            if (c.content.hasWriteProtect) c.content.writeProtected = writeProtected;
    }
    return out;
}

// ── The CE-1600F ────────────────────────────────────────────────────────

calcu::media::MediaPlace::Inserted FloppyPlace::insert(Content c, int side) {
    const auto* disk = dynamic_cast<const FloppyContent*>(c.get());
    if (!disk) return {false, "That is not a floppy disk."};
    if (!m_drive.attached()) return {false, "There is no CE-1600F."};
    if (disk->image.size() != CE1600FCard::kImageSize) return {false, "The floppy disk has the wrong size."};
    if (!m_drive.load(disk->image.data(), disk->image.size())) return {false, "The CE-1600F did not take the disk."};
    m_drive.setSide(side);
    return {};
}

Content FloppyPlace::eject() {
    Content held = content();
    m_drive.eject();
    return held;
}

std::optional<int> FloppyPlace::side() const {
    if (!m_drive.attached() || !m_drive.hasDisk()) return std::nullopt;
    return m_drive.side();
}

Content FloppyPlace::content() const {
    if (!m_drive.attached() || !m_drive.hasDisk()) return nullptr;
    return std::make_shared<FloppyContent>(m_drive.image());
}

// ── The cassette ────────────────────────────────────────────────────────

calcu::media::MediaPlace::Inserted CassettePlace::insert(Content c, int) {
    auto tape = std::dynamic_pointer_cast<const TapeContent>(c);
    if (!tape) return {false, "That is not a cassette tape."};
    WavData wav;
    std::string error;
    if (!readWavBytes(tape->wav, wav, error)) return {false, "The tape's sound cannot be read: " + error + "."};
    if (wav.samples.empty()) {
        m_deck.armRecord({});   // a blank tape records; the place keeps the sound
        m_played.reset();
        return {};
    }
    m_deck.loadForPlay(wav.samples, wav.sampleRate);
    m_played = std::move(tape);
    return {};
}

Content CassettePlace::eject() {
    Content held = content();
    m_deck.eject();
    m_played.reset();
    return held;
}

std::optional<int> CassettePlace::side() const {
    if (m_deck.mode() == TapeDeck::Mode::Empty) return std::nullopt;
    return 0;
}

Content CassettePlace::content() const {
    switch (m_deck.mode()) {
        case TapeDeck::Mode::Empty: return nullptr;
        case TapeDeck::Mode::Play: return m_played;
        case TapeDeck::Mode::Record: break;
    }
    return std::make_shared<TapeContent>(wavMono16Bytes(m_deck.recording(), TapeDeck::kRecordSampleRate));
}

std::uint64_t CassettePlace::revision() const {
    return m_deck.mode() == TapeDeck::Mode::Record ? m_deck.recording().size() : 0;
}

// ── A battery-backed module ─────────────────────────────────────────────

namespace {

const SoftwareDefinedCard* batteryCard(const ExpansionCard* c) {
    const auto* sd = dynamic_cast<const SoftwareDefinedCard*>(c);
    return sd && sd->definition().battery ? sd : nullptr;
}

} // namespace

bool SlotModuleMedium::present() const { return batteryCard(m_card()) != nullptr; }

Content SlotModuleMedium::content() const {
    const SoftwareDefinedCard* sd = batteryCard(m_card());
    if (!sd) return nullptr;
    return std::make_shared<ModuleContent>(definitionWithContent(sd->definition(), sd->debugImage(), sd->writeProtected()));
}

std::uint64_t SlotModuleMedium::revision() const {
    const ExpansionCard* c = m_card();
    return c ? c->contentRevision() : 0;
}

} // namespace sharp
