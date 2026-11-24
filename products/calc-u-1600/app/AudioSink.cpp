#include "app/AudioSink.hpp"

#include <algorithm>
#include <variant>

namespace cu1600 {

SoundFeed::SoundFeed(AudioSink& sink, Gap gap) : m_sink(sink), m_gap(std::move(gap)) {}

SoundFeed::~SoundFeed() { stopSink(); }

void SoundFeed::warmUp(int sampleRate) {
    if (m_open) return;
    if (m_sink.start(sampleRate, kSinkBufferMs)) m_sink.stop();
}

void SoundFeed::deliver(const calcu::sessions::Message& message) {
    using namespace calcu::sessions;
    if (const auto* start = std::get_if<SoundStart>(&message)) {
        stopSink();
        m_rate = start->sampleRate;
        m_open = m_rate > 0 && m_sink.start(m_rate, kSinkBufferMs);
        m_justStarted = true;
    } else if (const auto* s = std::get_if<SoundSamples>(&message)) {
        if (m_open) feed(s->mono.data(), s->mono.size());
    } else if (std::holds_alternative<SoundStop>(message)) {
        stopSink();
    }
}

void SoundFeed::feed(const std::int16_t* samples, std::size_t n) {
    // Audio the sink had no room for is carried to the next message (the
    // device consumes in hardware-period chunks). Once a stall pushes that
    // backlog past the cap, the oldest is dropped back down to the trim
    // level rather than just to the cap: production and consumption run at
    // the same rate, so a backlog never drains by itself.
    const std::size_t maxPending = static_cast<std::size_t>(m_rate) * 50 / 1000;
    const std::size_t trimPending = static_cast<std::size_t>(m_rate) * 15 / 1000;

    // The sink went idle = it played out everything it had (a sink started
    // just now is idle only because it has no data yet).
    if (!m_justStarted && m_sink.ranDry() && n > 0)
        m_gap("AudioOutput: sink ran dry (emulation behind audio clock) -- audible gap");
    m_justStarted = false;
    m_pending.insert(m_pending.end(), samples, samples + n);
    const std::size_t count = std::min(m_sink.room(), m_pending.size());
    if (count > 0) {
        m_sink.write(m_pending.data(), count);
        m_pending.erase(m_pending.begin(), m_pending.begin() + static_cast<std::ptrdiff_t>(count));
    }
    if (m_pending.size() > maxPending) {
        m_gap("AudioOutput: backlog " + std::to_string(m_pending.size() * 1000 / static_cast<std::size_t>(m_rate)) +
               " ms beyond the sink buffer -- audible cut");
        m_pending.erase(m_pending.begin(), m_pending.end() - static_cast<std::ptrdiff_t>(trimPending));
    }
}

void SoundFeed::stopSink() {
    if (!m_open) return;
    m_sink.stop();
    m_open = false;
    m_pending.clear();
}

} // namespace cu1600
