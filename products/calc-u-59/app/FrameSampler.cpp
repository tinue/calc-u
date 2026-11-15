#include "FrameSampler.hpp"
#include "../core/PrinterOutput.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace cu59 {

bool LedFrame::operator==(const LedFrame& o) const {
    return std::memcmp(segments, o.segments, sizeof segments) == 0 &&
           std::memcmp(digitLevel, o.digitLevel, sizeof digitLevel) == 0 &&
           std::memcmp(dpLevel, o.dpLevel, sizeof dpLevel) == 0;
}

FrameUpdate FrameSampler::tick(calcu::machine::Machine& m, int debugLevel, bool cardSwiping) {
    FrameUpdate u;
    u.cardAutoEjected = cardSwiping && !cardReaderOf(m)->isCardPresent();
    // The paper of the PC-100C, if one is plugged in (C59-6 design §3.5).
    auto* paper = calcu::machine::capabilityAs<PrinterOutput>(m, calcu::machine::CapabilityId{"printer/c59.printer-output"});
    if (paper) u.printerLines = paper->takeLines();
    if (debugLevel > 0) {
        for (const auto& e : debugPanelOf(m)->drainDebugEvents())
            if (e.level <= debugLevel) u.logLines.emplace_back(e.msg);
    }
    if (paper) u.printerCodeLines = paper->takeCodeLines();

    u.display = m.readDisplay();
    // The poll's length: what ran since the previous tick. With nothing run
    // since, the core repeats its last snapshot, and the poll counts as long
    // as the previous one (as 2.3.0's pollTicks did).
    const calcu::machine::Ticks now = m.tickCount();
    if (now != m_lastTicks) m_lastSeconds = static_cast<double>(now - m_lastTicks) / m.descriptor().tickRate;
    m_lastTicks = now;
    m_pov.update(u.display, m_lastSeconds);
    auto level = [](float b) { return static_cast<uint8_t>(std::lround(b * 255.0f)); };
    const auto& positions = std::get<calcu::machine::SegmentFrame>(u.display.frame).positions;
    for (int i = 0; i < 12; ++i) {
        const auto& p = positions[static_cast<size_t>(i)];
        if (p.brightness.value_or(0.0f) > 0) {
            uint8_t bits = 0;
            for (int s = 0; s < 7; ++s)
                if (p.lit[static_cast<size_t>(s)]) bits |= static_cast<uint8_t>(1u << s);
            m_lastSegments[i] = bits;
        }
        u.led.segments[i] = m_lastSegments[i];
        u.led.digitLevel[i] = level(m_pov.digit(i));
        u.led.dpLevel[i] = level(m_pov.dp(i));
    }

    const auto& c = u.display.indicators.front();
    u.calcIndicatorOpacity = updateCalcIndicator(c.on ? c.brightness.value_or(0.0f) * 16.0f : 0.0f);
    return u;
}

void FrameSampler::reset() {
    m_calcOpacity = 0;
    m_zeroFrames = 0;
    m_pov = {};
    std::fill(std::begin(m_lastSegments), std::end(m_lastSegments), 0);
}

// The C indicator follows the core's mean drive (the indicator's brightness × 16):
// its brightness changes at once, as on a real TI-59, whose C visibly jumps
// while a program runs and between the phases of a blink. The one exception is
// 60 Hz aliasing: the ROM's IDLE display scan lasts ~4.5 ms (16 IDLE steps ×
// 281 µs), so one 16.7 ms poll can see a single zero-drive frame although the
// real dark gap is imperceptible. Hence:
//   target 0, first frame   → hold (aliasing artefact)
//   target 0, frame 2+      → decay fast (a real dark phase: error blink, …)
//   otherwise               → the target
// An opacity is not linear in light, hence the display gamma: drive 1 (one bit,
// the dark phase of a blink) gives 0.65, drive 0.09 (its lit phase) a very weak
// but visible C, and several bits at once reach full brightness.
double FrameSampler::updateCalcIndicator(float drive) {
    const double target = std::min(1.0, std::pow(static_cast<double>(drive), 1.0 / 2.2) * 0.65);
    if (target < 0.001) {
        ++m_zeroFrames;
        if (m_zeroFrames >= 2) {
            double decay = std::min(0.65, 0.35 * (m_zeroFrames - 1));
            m_calcOpacity = std::max(0.0, m_calcOpacity - decay);
        }
    } else {
        m_zeroFrames = 0;
        m_calcOpacity = target;
    }
    return m_calcOpacity;
}

} // namespace cu59
