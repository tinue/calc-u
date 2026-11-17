#include "harness/WavRecorder.hpp"
#include "files/Files.hpp"
#include <filesystem>

namespace calcu::harness {

namespace {
void u32(std::ofstream& o, std::uint32_t v) {
    const char b[4] = {static_cast<char>(v & 0xFF), static_cast<char>((v >> 8) & 0xFF),
                       static_cast<char>((v >> 16) & 0xFF), static_cast<char>((v >> 24) & 0xFF)};
    o.write(b, 4);
}
void u16(std::ofstream& o, std::uint16_t v) {
    const char b[2] = {static_cast<char>(v & 0xFF), static_cast<char>((v >> 8) & 0xFF)};
    o.write(b, 2);
}
} // namespace

bool WavRecorder::open(const std::string& path, int sampleRate, std::string* error) {
    close();
    out_.open(files::toPath(path), std::ios::binary | std::ios::trunc);
    if (!out_) {
        *error = "Cannot write the file " + path + ".";
        return false;
    }
    rate_ = sampleRate;
    samples_ = 0;
    header();
    return true;
}

void WavRecorder::header() {
    const std::uint32_t data = static_cast<std::uint32_t>(samples_ * 2);
    out_.seekp(0);
    out_.write("RIFF", 4);
    u32(out_, 36 + data);
    out_.write("WAVE", 4);
    out_.write("fmt ", 4);
    u32(out_, 16);
    u16(out_, 1);   // PCM
    u16(out_, 1);   // mono
    u32(out_, static_cast<std::uint32_t>(rate_));
    u32(out_, static_cast<std::uint32_t>(rate_) * 2);
    u16(out_, 2);
    u16(out_, 16);
    out_.write("data", 4);
    u32(out_, data);
}

void WavRecorder::write(std::span<const std::int16_t> mono) {
    if (!out_.is_open()) return;
    out_.seekp(0, std::ios::end);
    for (std::int16_t s : mono) u16(out_, static_cast<std::uint16_t>(s));
    samples_ += mono.size();
}

void WavRecorder::close() {
    if (!out_.is_open()) return;
    header();
    out_.close();
}

} // namespace calcu::harness
