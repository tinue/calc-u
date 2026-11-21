#include "FrameSampler.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace cu59 {

double DisplayPov::Channel::step(double polled, double maxDuty, double seconds) {
    // Share: time-weighted means over the position's first polls, so it is
    // right at once, then running averages carried across dark phases. Time
    // weighting makes the ratio follow the integrated driven time, whatever
    // the poll length (a poll shorter than a sweep drives positions unevenly).
    if (polled > 0 && maxDuty >= kShareMinDuty) {
        shareSeconds += seconds;
        const double a = std::max(seconds / shareSeconds, 1.0 - std::exp(-seconds / kShareSeconds));
        duty += (polled - duty) * a;
        bestDuty += (maxDuty - bestDuty) * a;
    }
    const double d = std::min(1.0, polled / share() / kFullDuty);
    if (d >= 1.0) {               // driven through the whole poll: simply on
        dark = 0;
        level = 1.0;
        return level;
    }
    // Only the undriven part of the poll counts towards the hold; the
    // brightness never drops below what this poll's own duty shows.
    const double before = dark;
    dark += seconds * (1.0 - d);
    const double fading = dark - std::max(before, kHoldSeconds);
    if (fading > 0) level *= std::exp(-fading / kFadeSeconds);
    level = std::max(level, d);
    if (level < kCutoff) level = 0;
    return level;
}

void DisplayPov::update(const calcu::machine::DisplayOutput& out, double seconds) {
    const auto& positions = std::get<calcu::machine::SegmentFrame>(out.frame).positions;
    float digitDuty[12], dpDuty[12];
    for (int i = 0; i < 12; ++i) {
        const auto& p = positions[static_cast<size_t>(i)];
        digitDuty[i] = p.brightness.value_or(0.0f);
        dpDuty[i] = p.segmentBrightness.back().value_or(0.0f);
    }
    const double maxDuty = *std::max_element(std::begin(digitDuty), std::end(digitDuty));
    double best = 0;
    for (int i = 0; i < 12; ++i) {
        m_digit[i].step(digitDuty[i], maxDuty, seconds);
        m_dp[i].step(dpDuty[i], maxDuty, seconds);
        if (m_digit[i].level > 0) best = std::max(best, m_digit[i].share());
    }
    if (best > 0) m_bestShare = best;
}

bool LedFrame::operator==(const LedFrame& o) const {
    return std::memcmp(segments, o.segments, sizeof segments) == 0 &&
           std::memcmp(digitLevel, o.digitLevel, sizeof digitLevel) == 0 &&
           std::memcmp(dpLevel, o.dpLevel, sizeof dpLevel) == 0;
}

FrameUpdate FrameSampler::tick(calcu::machine::Machine& m) {
    FrameUpdate u;

    u.display = m.readDisplay();
    // The poll's length: what ran since the previous tick. With nothing run
    // since (a frozen machine), the core repeats its last snapshot, which a
    // pause may have cut off mid-sweep: the levels stay as they were, so a
    // frozen display keeps what it showed.
    const calcu::machine::Ticks now = m.tickCount();
    if (now != m_lastTicks) {
        m_pov.update(u.display, static_cast<double>(now - m_lastTicks) / m.descriptor().tickRate);
    }
    m_lastTicks = now;
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
