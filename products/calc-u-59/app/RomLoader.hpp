#pragma once
#include "MachineModelInfo.hpp"
#include "machine/Build.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

// Reading the plaintext ROM dumps in roms/ (Datamath museum format: header
// block, a "---" separator line, then address-prefixed data rows). The
// calculator's ROM files go to the machine as RomImages, which the core
// decodes (core/TIRomText.hpp; LIB-1 D-4); module images are parsed here
// (C59-6, LIB-12). File access is the frontend's job: it passes a
// ResourceReader that returns a file's text by bare file name (Swift:
// Bundle.main; Qt: the :/roms/ qrc).

namespace cu59 {

struct RomError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

/// Returns the full text of a bundled resource by bare file name (e.g.
/// "TMC0582.txt"), or nullopt if it doesn't exist.
using ResourceReader = std::function<std::optional<std::string>(const std::string& fileName)>;

/// A ResourceReader over plain directories, searched in order — the layout of
/// roms/ (calculator/, then solid-state/) for tests and headless tools.
ResourceReader directoryReader(std::vector<std::string> dirs);

/// Parse a solid-state module TMC*.txt file: "dddd: bb bb ..." rows of BCD
/// bytes ("21" is 0x21). Returns bytes up to the highest address written
/// (max 5000), or nullopt if no data rows were found.
std::optional<std::vector<uint8_t>> parseModuleTxt(std::string_view text);

/// The model's five calculator ROM files (three instruction chips, two
/// CONST-K files) as RomImages for calcu::machine::build(). The names are
/// ModelInfo's (LIB-1's adapter until M-24). Throws RomError on a missing file.
std::vector<calcu::machine::RomImage> loadRomImages(Model model, const ResourceReader& read);

/// A machine of `model` built from its ROM files through the machine
/// interface (calcu::machine::build with tiCatalogue() and TIMachineBuilder),
/// in the state after a cold boot (C59-1 AC-7). `content`: a TI-58C's
/// constant memory (TIConstantMemory, C59-3 from M-54). Throws RomError when
/// a file is missing or cannot be decoded.
std::unique_ptr<calcu::machine::Machine> buildMachine(Model model, const ResourceReader& read,
                                                      std::shared_ptr<const calcu::machine::ProductData> content = nullptr);

/// Load a solid-state module image by ROM file name (cuecards.txt MODULE-ROM).
std::optional<std::vector<uint8_t>> loadModule(const std::string& romFile, const ResourceReader& read);

} // namespace cu59
