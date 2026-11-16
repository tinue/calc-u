#pragma once
#include "../core/TIAdapters.hpp"
#include "DisplayPov.hpp"
#include "machine/Machine.hpp"
#include <string>
#include <vector>

// What the UI picks up from the machine on each 60 Hz frame (the paper is
// LIB-8's PaperPart since M-53):
// core log messages, whether the card reader auto-ejected, and the display —
// each position's last glyph with its brightness as the eye sees it
// (DisplayPov) — plus the smoothed "C" (calculating) indicator.

namespace cu59 {

/// The display as drawn: glyphs and per-position brightness 0–255.
struct LedFrame {
    uint8_t segments[12]{};   ///< last driven glyph per position: A–G as bits 0–6 (SevenSegment.hpp)
    uint8_t digitLevel[12]{}; ///< 0 = dark … 255 = fully lit
    uint8_t dpLevel[12]{};    ///< decimal point of each position
    bool operator==(const LedFrame& o) const;
    bool operator!=(const LedFrame& o) const { return !(*this == o); }
};

struct FrameUpdate {
    calcu::machine::DisplayOutput display;  ///< the machine's display output (C59-1 AC-13)
    LedFrame led;
    double calcIndicatorOpacity = 0;      ///< 0–1, see FrameSampler
    /// The card was passing and the core ejected it (at CRD_OFF, as the real
    /// card leaves the reader after each pass): the frontend saves written data.
    bool cardAutoEjected = false;
};

class FrameSampler {
public:
    /// cardSwiping: the frontend's card
    /// state. The poll covers the emulated time since the previous tick (tick
    /// count difference over the tick rate). The only reader of the machine's
    /// display output (C59-1 OQ-7).
    FrameUpdate tick(calcu::machine::Machine& m, bool cardSwiping);

    /// Forget the indicator history (new machine).
    void reset();

    /// Advance the C indicator by one frame with the core's mean drive (the
    /// indicator's brightness × 16: 0–16); returns the new opacity.
    double updateCalcIndicator(float drive);

private:
    double m_calcOpacity = 0;
    int m_zeroFrames = 0;
    DisplayPov m_pov;
    uint8_t m_lastSegments[12]{};          ///< per position, the glyph of its last driven poll
    calcu::machine::Ticks m_lastTicks = 0;  ///< tick count at the previous tick
    double m_lastSeconds = 0;              ///< the previous poll's length (no instruction since: again)
};

} // namespace cu59
