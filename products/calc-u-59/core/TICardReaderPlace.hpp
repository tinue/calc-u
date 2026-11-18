#pragma once
#include "TIMagneticCard.hpp"
#include "media/MediaPlace.hpp"
#include <cstdint>
#include <optional>

// The TI-59's card reader as a media place: pass-through, the sides
// 0 and 1, the content a TIMagneticCardData. A side that goes in passes once:
// the ROM reads it or writes a bank, and the core gives the card back by
// itself (CRD_OFF); a card the ROM does not take at once stays at the reader
// with the card switch closed until it does. What the ROM
// does with a partition that is not its own, a protected bank or an empty
// side is the ROM's. The TI-58 and TI-58C have none.

class TIMachine;

class TICardReaderPlace final : public calcu::media::MediaPlace {
public:
    explicit TICardReaderPlace(TIMachine& m) : m_(m) {}
    calcu::machine::MediaPlaceId id() const override { return calcu::machine::MediaPlaceId{"card-reader"}; }
    /// Refused while a card passes; else the side's bytes go in: the bank, a
    /// definition built with its partition or the machine's now, or zeroes
    /// for a fresh side.
    Inserted insert(calcu::media::Content, int side) override;
    /// Eject Card: a whole bank the ROM wrote is kept and reported by the
    /// next takeGivenBack; returns the card.
    calcu::media::Content eject() override;
    std::optional<int> side() const override;
    calcu::media::Content content() const override { return m_content; }
    std::uint64_t revision() const override { return m_revision; }
    /// The ROM waits in the card wait of `n Write`, `n INV Write`; not while it only waits for a key.
    bool waitsForMedium() const override;
    std::optional<GivenBack> takeGivenBack() override;
    /// Before the processor reset (a Reset, a power cycle, a Cold boot): a card
    /// in the reader is taken out as by Eject Card.
    void beforeCpuReset();

    /// The card in the reader: its side and whether it is in;
    /// its bytes and position are the processor's card buffers. A card
    /// restored in the reader belongs to no binding: it is a card of
    /// its own, which the pass gives back with what it wrote.
    template <class Io>
    void stateIo(Io& io) {
        io.begin("CRD ");
        int side = m_side.value_or(-1);
        io(side)(m_inserted);
        if constexpr (Io::kRestoring) {
            m_side = side >= 0 ? std::optional<int>(side) : std::nullopt;
            m_back.reset();
            if (m_inserted) m_content = std::make_shared<cu59::TIMagneticCardData>();
            ++m_revision;
        }
        io.end();
    }

private:
    /// The card out of the reader, the written bank (all 246 bytes) kept.
    void takeOut();
    TIMachine& m_;
    calcu::media::Content m_content;
    std::optional<int> m_side;           ///< in the reader now
    bool m_inserted = false;
    std::optional<GivenBack> m_back;     ///< given back, not yet taken
    std::uint64_t m_revision = 0;
};
