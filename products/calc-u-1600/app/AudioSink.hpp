#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "sessions/Messages.hpp"

namespace cu1600 {

// The host's audio output, push mode, mono int16 at the rate the session's
// SoundStart names (Qt: QAudioSink).
class AudioSink {
public:
    virtual ~AudioSink() = default;
    // Opens the default output with a buffer of `bufferMs`; false if there
    // is none or it can't be opened (the sink reports why, once).
    virtual bool start(int sampleRate, int bufferMs) = 0;
    virtual void stop() = 0;
    // Samples the sink can take right now.
    virtual std::size_t room() const = 0;
    virtual void write(const std::int16_t* samples, std::size_t count) = 0;
    // True when the sink played out everything it had (an audible gap).
    virtual bool ranDry() const = 0;
};

// ── SoundFeed: the session's sound messages into an AudioSink ───────────
//
// What is heard is the app session's decision: it sends SoundStart on the first sound after silence (then
// 40 ms of priming silence), SoundSamples at regular speed only, and
// SoundStop after a second of silence. The sink side keeps the carry-over:
// samples the sink had no room for wait for the next message, up to a
// small cap, so latency can't build up (the sink's room() cannot be read
// across a thread boundary, so this stays on the frontend's side).
class SoundFeed {
public:
    using Warn = std::function<void(const std::string&)>;
    SoundFeed(AudioSink& sink, Warn warn);
    ~SoundFeed();

    // A SoundStart, SoundSamples or SoundStop; other messages are ignored.
    void deliver(const calcu::sessions::Message& message);
    // Starts and stops the sink once: the first sink start in a process
    // initializes the platform audio backend, which can block (~0.5 s on
    // macOS); call this right after startup instead of at the first BEEP.
    void warmUp(int sampleRate);
    bool open() const { return m_open; }

    static constexpr int kSinkBufferMs = 100;

private:
    void feed(const std::int16_t* samples, std::size_t n);
    void stopSink();

    AudioSink& m_sink;
    Warn m_warn;
    bool m_open = false;
    bool m_justStarted = false;
    int m_rate = 0;
    std::vector<std::int16_t> m_pending;   // given but not yet accepted by the sink
};

} // namespace cu1600
