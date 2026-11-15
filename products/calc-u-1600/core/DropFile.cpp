#include "DropFile.hpp"

#include <cctype>

#include "ProgramFile.hpp"
#include "opening/Classify.hpp"

namespace dropfile {

namespace {

// NUL-free UTF-8 with a line that starts with `model:` in column 0 --
// indented or commented-out keys don't count. Adapter (LIB-13 design §5,
// M-42): the rule is the library's; this file goes in M-48.
bool looksLikePreset(const std::vector<uint8_t>& bytes) { return calcu::opening::looksLikePreset(bytes); }

bool codeFileName(const std::string& fileName) {
    const size_t dot = fileName.rfind('.');
    if (dot == std::string::npos) return false;
    std::string ext = fileName.substr(dot + 1);
    for (char& ch : ext) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    return ext == "bin" || ext == "rom";
}

}  // namespace

Target classify(const std::vector<uint8_t>& bytes, const std::string& fileName) {
    if (looksLikePreset(bytes)) return Target::Preset;
    using programfile::Kind;
    const programfile::ProgramFile f = programfile::classify(bytes);
    switch (f.kind) {
        case Kind::BasicListing:
        case Kind::BasicPC1500:
        case Kind::BasicPC1600: return Target::BasicProgram;
        case Kind::CodeLH5801:
        case Kind::CodeZ80: return Target::MachineCode;
        case Kind::Headerless:
            return f.looksLikeCode && codeFileName(fileName) ? Target::MachineCode : Target::None;
        case Kind::Empty: return Target::None;
        // A cassette WAV that can't be loaded goes to the BASIC loader, which
        // says why; other files go nowhere.
        case Kind::Other: return f.fromTape ? Target::BasicProgram : Target::None;
    }
    return Target::None;
}

}  // namespace dropfile
