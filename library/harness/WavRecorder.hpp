#pragma once
#include <cstdint>
#include <fstream>
#include <span>
#include <string>

// A 16-bit mono PCM WAV file (LIB-11 design §2; spec AC-9 `audio`, EC-3).
// The header is written at open with sizes of zero and again at close, so
// the file is complete once close() has run.

namespace calcu::harness {

class WavRecorder {
public:
    WavRecorder() = default;
    ~WavRecorder() { close(); }
    WavRecorder(const WavRecorder&) = delete;
    WavRecorder& operator=(const WavRecorder&) = delete;

    // False with `error` set when the file cannot be written.
    bool open(const std::string& path, int sampleRate, std::string* error);
    void write(std::span<const std::int16_t> mono);
    void close();
    bool isOpen() const { return out_.is_open(); }
    std::uint64_t samples() const { return samples_; }

private:
    void header();
    std::ofstream out_;
    int rate_ = 0;
    std::uint64_t samples_ = 0;
};

} // namespace calcu::harness
