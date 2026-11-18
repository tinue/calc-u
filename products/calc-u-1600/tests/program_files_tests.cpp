// What a file is: sharp::loaders::fileKind and image over the one classifier
// (core/ProgramFile, SharpDataExchange's rules), and the empty and damaged
// files every loader refuses the same way. The classifier's own cases stay
// in program_file_tests.cpp.

#include <cstdint>
#include <fstream>
#include <iterator>
#include <string>
#include <variant>
#include <vector>

#include "../core/Loaders.hpp"
#include "../core/Machine/SharpLoaderMemory.hpp"
#include "support/PC1500BasicTyper.hpp"
#include "../core/PC1500/PC1500Machine.hpp"
#include "../core/ProgramFile.hpp"
#include "LoaderTestSupport.hpp"
#include "TestHarness.hpp"

namespace {

using programfile::Kind;
using sharp::loaders::fileKind;

const std::vector<uint8_t> kCode = {0x3E, 0x41, 0xC9, 0x00, 0x11};
const std::vector<uint8_t> kBasic = {0x00, 0x0A, 0x03, 0xF1, 0x8E, 0x0D};   // 10 END

std::vector<uint8_t> ce158(const std::vector<uint8_t>& payload, uint8_t type, uint16_t load = 0, uint16_t autorun = 0) {
    std::vector<uint8_t> f(27, 0x00);
    f[0] = 0x01;
    f[1] = type;
    f[2] = 'C'; f[3] = 'O'; f[4] = 'M';
    const char* name = "SAMPLE";
    for (int i = 0; name[i]; i++) f[5 + i] = static_cast<uint8_t>(name[i]);
    const uint16_t wire = static_cast<uint16_t>(payload.size() - 1);
    for (auto [at, v] : {std::pair<int, uint16_t>{0x15, load}, {0x17, wire}, {0x19, autorun}}) {
        f[at] = static_cast<uint8_t>(v >> 8);
        f[at + 1] = static_cast<uint8_t>(v & 0xFF);
    }
    f.insert(f.end(), payload.begin(), payload.end());
    return f;
}

std::vector<uint8_t> pc1600(const std::vector<uint8_t>& payload, uint8_t type, uint32_t load = 0, uint32_t autorun = 0) {
    std::vector<uint8_t> f = {0xFF, 0x10, 0x00, 0x00, type};
    for (uint32_t v : {static_cast<uint32_t>(payload.size()), load, autorun}) {
        f.push_back(static_cast<uint8_t>(v & 0xFF));
        f.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
        f.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
    }
    f.push_back(0x00);
    f.push_back(0x0F);
    f.insert(f.end(), payload.begin(), payload.end());
    return f;
}

std::vector<uint8_t> text(const std::string& s) { return std::vector<uint8_t>(s.begin(), s.end()); }

std::vector<uint8_t> z80Code() {   // SharpDataExchange's Z80 sample (src/cpu_guess.rs)
    static const uint8_t kBlock[] = {0x21, 0x00, 0x80, 0x06, 0x10, 0x7E, 0xFE, 0x20, 0x28,
                                     0x01, 0x23, 0x10, 0xF8, 0xCD, 0x00, 0x10, 0xC9};
    std::vector<uint8_t> out;
    for (int i = 0; i < 8; i++) out.insert(out.end(), std::begin(kBlock), std::end(kBlock));
    return out;
}

// The fixture tape twice in one WAV: its data chunk's samples doubled.
std::vector<uint8_t> doubledTape(const std::vector<uint8_t>& wav) {
    for (std::size_t i = 12; i + 8 <= wav.size();) {
        const std::uint32_t size = wav[i + 4] | wav[i + 5] << 8 | wav[i + 6] << 16 | std::uint32_t(wav[i + 7]) << 24;
        if (std::equal(wav.begin() + i, wav.begin() + i + 4, "data")) {
            std::vector<std::uint8_t> out(wav.begin(), wav.begin() + i + 8 + size);
            out.insert(out.end(), wav.begin() + i + 8, wav.begin() + i + 8 + size);
            const std::uint32_t doubled = size * 2, riff = static_cast<std::uint32_t>(out.size() - 8);
            for (int b = 0; b < 4; ++b) {
                out[i + 4 + b] = static_cast<std::uint8_t>(doubled >> (8 * b));
                out[4 + b] = static_cast<std::uint8_t>(riff >> (8 * b));
            }
            return out;
        }
        i += 8 + size + (size & 1);
    }
    return {};
}

std::vector<uint8_t> silentWav() {
    std::vector<uint8_t> w = {'R', 'I', 'F', 'F', 0, 0, 0, 0, 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ',
                              16, 0, 0, 0, 1, 0, 1, 0, 0x40, 0x1F, 0, 0, 0x40, 0x1F, 0, 0, 1, 0, 8, 0,
                              'd', 'a', 't', 'a', 0x40, 0x1F, 0, 0};
    w.insert(w.end(), 8000, 0x80);
    return w;
}

TEST(every_kind_by_content_not_name) {
    CHECK(fileKind({}).kind == Kind::Empty);
    CHECK(fileKind(text("10 PRINT \"HI\"\n20 END\n")).kind == Kind::BasicListing);
    CHECK(fileKind(ce158(kBasic, 0x40)).kind == Kind::BasicPC1500);
    CHECK(fileKind(pc1600(kBasic, 0x21)).kind == Kind::BasicPC1600);
    CHECK(fileKind(ce158(kCode, 0x42, 0x40C5)).kind == Kind::CodeLH5801);
    CHECK(fileKind(pc1600(kCode, 0x10, 0xC0C5)).kind == Kind::CodeZ80);
    CHECK(fileKind(kCode).kind == Kind::Headerless);
    // fileKind takes the bytes alone: there is no name to go by.
}

TEST(headerless_looks_like_lh5801_or_z80) {
    const sharp::loaders::FileKind z = fileKind(z80Code());
    CHECK(z.kind == Kind::Headerless && z.looksLike && *z.looksLike == machinecode::Cpu::Z80);
    CHECK(!fileKind(kCode).looksLike);   // too short to guess
    CHECK(!fileKind(pc1600(z80Code(), 0x10, 0xC0C5)).looksLike);   // a header: no guess
}

TEST(other_with_words) {
    const sharp::loaders::FileKind t = fileKind(text("Just some notes,\nnot a program.\n"));
    CHECK(t.kind == Kind::Other && t.words == "plain text");
}

TEST(ce158_header_fields) {
    const sharp::loaders::FileKind k = fileKind(ce158(kCode, 0x42, 0x40C5, 0x40C7));
    CHECK(k.header.has_value());
    if (!k.header) return;
    CHECK(k.header->name == "SAMPLE");
    CHECK(k.header->loadAddress == 0x40C5 && k.header->bank == 0);
    CHECK(k.header->length == kCode.size());
    CHECK(k.header->start && *k.header->start == 0x40C7);
}

TEST(pc1600_header_fields_with_bank) {
    const sharp::loaders::FileKind k = fileKind(pc1600(kCode, 0x10, 0x02'8100, 0x02'8105));
    CHECK(k.header.has_value());
    if (!k.header) return;
    CHECK(k.header->loadAddress == 0x8100 && k.header->bank == 2);
    CHECK(k.header->length == kCode.size());
    CHECK(k.header->start && (*k.header->start & 0xFFFF) == 0x8105);
}

TEST(start_zero_or_ffff_none) {
    for (uint16_t s : {uint16_t{0}, uint16_t{0xFFFF}}) {
        const auto k = fileKind(ce158(kCode, 0x42, 0x40C5, s));
        CHECK(k.header && !k.header->start);
        const auto p = fileKind(pc1600(kCode, 0x10, 0xC0C5, s));
        CHECK(p.header && !p.header->start);
    }
}

TEST(wav_files_in_tape_order_with_names) {
    const std::vector<uint8_t> wav = loadertest::fileBytes("tests/fixtures/tape/pc1500_tape.wav");
    SKIP_UNLESS(!wav.empty(), "tests/fixtures/tape/pc1500_tape.wav missing");
    const sharp::loaders::FileKind one = fileKind(wav);
    CHECK(one.kind == Kind::BasicPC1500 && one.tapeFiles.size() == 1);
    CHECK(!one.tapeFiles.empty() && one.tapeFiles[0].rfind("TAPEFIX", 0) == 0);
    const std::vector<uint8_t> img = loadertest::fileBytes("tests/fixtures/tape/pc1500_tape.img");
    const auto first = sharp::loaders::image(wav, 1, nullptr);
    CHECK(first.has_value() && programfile::classify(*first).payload == img);   // a file like any other

    const std::vector<uint8_t> two = doubledTape(wav);
    const sharp::loaders::FileKind k = fileKind(two);
    CHECK(k.tapeFiles.size() == 2);
    CHECK(sharp::loaders::image(two, 2, nullptr).has_value());
    std::string why;
    CHECK(!sharp::loaders::image(two, 3, &why).has_value() && why.find("2 files") != std::string::npos);
}

TEST(wav_none_readable_is_other_with_words) {
    const sharp::loaders::FileKind k = fileKind(silentWav());
    CHECK(k.kind == Kind::Other && k.tapeFiles.empty());
    CHECK(k.words.find("a cassette recording without a file that can be read") != std::string::npos);
}

// Every loader asks the classifier first: the same sentence from each.
TEST(empty_refused_as_empty_by_every_loader) {
    PC1500Machine m;
    SKIP_UNLESS(m.loadROMFile("roms/PC-1500_A04.ROM"), "roms/PC-1500_A04.ROM not found");
    sharp::PC1500LoaderMemory mem(m);
    CHECK(sharp::loaders::loadBasic(mem, {}).refusal == "The file is empty.");
    const auto code = sharp::loaders::loadCode(mem, {}, {});
    CHECK(std::holds_alternative<std::string>(code) && std::get<std::string>(code) == "The file is empty.");
    sharp::loaders::CodeRequest at;
    at.address = 0x7C01;   // headerless with an address: still empty, not "no code"
    const auto withAddress = sharp::loaders::loadCode(mem, {}, at);
    CHECK(std::holds_alternative<std::string>(withAddress) && std::get<std::string>(withAddress) == "The file is empty.");
}

TEST(partial_header_refused_saying_so) {
    auto cut = ce158(kCode, 0x42);
    cut.resize(20);
    const sharp::loaders::FileKind k = fileKind(cut);
    CHECK(k.damaged);
    PC1500Machine m;
    SKIP_UNLESS(m.loadROMFile("roms/PC-1500_A04.ROM"), "roms/PC-1500_A04.ROM not found");
    sharp::PC1500LoaderMemory mem(m);
    const std::string sentence = "The file starts like a CE-158 or PC-1600 header, but has no complete header of a known type.";
    CHECK(sharp::loaders::loadBasic(mem, cut).refusal == sentence);
    const auto code = sharp::loaders::loadCode(mem, cut, {});
    CHECK(std::holds_alternative<std::string>(code) && std::get<std::string>(code) == sentence);
}

TEST(wav_first_file_by_default) {
    const std::vector<uint8_t> wav = loadertest::fileBytes("tests/fixtures/tape/pc1500_tape.wav");
    SKIP_UNLESS(!wav.empty(), "tests/fixtures/tape/pc1500_tape.wav missing");
    const std::vector<uint8_t> two = doubledTape(wav);
    const auto def = sharp::loaders::image(two, 0, nullptr);
    const auto first = sharp::loaders::image(two, 1, nullptr);
    CHECK(def && first && *def == *first);
    // Not a WAV: the bytes themselves.
    CHECK(sharp::loaders::image(kCode, 0, nullptr) == std::optional<std::vector<uint8_t>>(kCode));
}

} // namespace
