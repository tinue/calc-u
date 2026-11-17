#include "TICardReaderPlace.hpp"
#include "TIMachine.hpp"

using cu59::TIMagneticCardData;

namespace {

std::shared_ptr<const TIMagneticCardData> cardOf(const calcu::media::Content& c) {
    return std::dynamic_pointer_cast<const TIMagneticCardData>(c);
}

} // namespace

calcu::media::MediaPlace::Inserted TICardReaderPlace::insert(calcu::media::Content c, int which) {
    if (side() || (m_inserted && m_.m_hw.isCardPresent())) return Inserted{false, "A card is in the reader."};
    if (m_inserted) takeOut();   // given back by the core, not yet taken: keep what it wrote
    const auto card = cardOf(c);
    if (!card) return Inserted{false, "The card reader takes a magnetic card."};
    if (which < 0 || which > 1) return Inserted{false, "A magnetic card has the sides 0 and 1."};
    std::vector<std::uint8_t> bytes;
    const TIMagneticCardData::Side& s = card->sides[static_cast<std::size_t>(which)];
    if (const auto* b = std::get_if<std::vector<std::uint8_t>>(&s)) bytes = *b;
    else if (const auto* def = std::get_if<cu59::TICardSideDef>(&s)) bytes = cu59::buildCardBank(*def, m_.m_hw.partitionProgramRegs());
    // EC-2: a fresh side gives the ROM zeroes.
    m_.m_hw.insertCard(bytes.empty() ? nullptr : bytes.data(), bytes.size());
    m_content = std::move(c);
    m_side = which;
    m_inserted = true;
    return Inserted{};
}

void TICardReaderPlace::takeOut() {
    if (!m_inserted) return;
    std::vector<std::uint8_t> written = m_.m_hw.cardEject();
    m_inserted = false;
    const int passed = m_side.value_or(0);
    m_side.reset();
    GivenBack back;
    back.content = m_content;
    if (written.size() == cu59::kCardBankBytes) {   // D-8: a whole bank
        auto card = std::make_shared<TIMagneticCardData>();
        if (const auto old = cardOf(m_content)) *card = *old;
        card->sides[static_cast<std::size_t>(passed)] = std::move(written);
        card->lastWritten = passed;
        back.content = card;
        back.written = true;
        m_content = back.content;
        ++m_revision;
    }
    if (m_back && m_back->written && !back.written) back.written = true;   // an earlier write not yet taken
    m_back = std::move(back);
}

calcu::media::Content TICardReaderPlace::eject() {
    takeOut();
    return m_content;
}

std::optional<int> TICardReaderPlace::side() const {
    if (m_inserted && m_.m_hw.isCardPresent()) return m_side;
    return std::nullopt;
}

bool TICardReaderPlace::waitsForMedium() const {
    return m_.m_ranges.cardWait.contains(m_.m_hw.pc()) && m_.m_hw.isWaitingForCard();
}

std::optional<calcu::media::MediaPlace::GivenBack> TICardReaderPlace::takeGivenBack() {
    if (m_inserted && !m_.m_hw.isCardPresent()) takeOut();   // the core gave it back (CRD_OFF)
    return std::exchange(m_back, std::nullopt);
}

void TICardReaderPlace::beforeCpuReset() { takeOut(); }
