#pragma once
#include <QObject>
#include <cstdint>

#include "app/AudioSink.hpp"

class QAudioSink;
class QIODevice;

// The host's default audio output as a cu1600::AudioSink (a QAudioSink):
// the app session decides what is heard and
// MachineController's SoundFeed (app/AudioSink.hpp) writes it.
// Must outlive the session.
class AudioOutput : public QObject, private cu1600::AudioSink {
    Q_OBJECT
public:
    explicit AudioOutput(QObject* parent = nullptr);

    cu1600::AudioSink& sink() { return *this; }

private:
    // cu1600::AudioSink
    bool start(int sampleRate, int bufferMs) override;
    void stop() override;
    std::size_t room() const override;
    void write(const std::int16_t* samples, std::size_t count) override;
    bool ranDry() const override;

    QAudioSink* m_sink = nullptr;
    QIODevice* m_io = nullptr;  // m_sink's push device while playing
    bool m_warned = false;      // no device / open failure logged once
};
