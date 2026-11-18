#pragma once
#include "opening/FileKind.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// Calc-U-59's file kinds of the library's `open`, asked in this order, each
// by its name alone (takesName is its rule):
//
//   asm-overlay           a name ending in .asm: read as by Select File
//   keystroke-program     .keystrokes.yaml: its program written into the
//                         running machine from step 000, without a cold boot
//   rom-module-file       .rom.yaml: refused, a module is taken by its ID only
//   constant-memory-file  ti58c.memory.yaml: refused, the product's own file
//
// The library decides presets (.ti59, .ti58, .ti58c, or a `model:` line)
// and media before them: a magnetic card (`.magnetcard.yaml`) is a medium of
// the library's media part, fitted into the TI-59's card reader by the
// library's MediaOpener (release 2.3.0's .u59 and .bin cards are no files of
// the product).

namespace cu59 {

class TIDebugPart;

/// The product's preset extensions.
std::vector<std::string> tiPresetExtensions();

/// The Qt6 program's arguments with its own options and their values taken
/// out (`--grab x`, `--after n`, `--pacing-probe s`), the
/// program's name kept first: what calcu::opening::startFile reads, so the
/// start file is the last argument that does not start with `-` and names an
/// existing file (as in Calc-U-1600), never a value of an option.
std::vector<std::string> programArguments(const std::vector<std::string>& argv);

class TIFileKinds {
public:
    explicit TIFileKinds(TIDebugPart& debug);
    ~TIFileKinds();
    TIFileKinds(const TIFileKinds&) = delete;
    TIFileKinds& operator=(const TIFileKinds&) = delete;
    /// In the order they are asked.
    std::vector<calcu::opening::FileKind*> kinds();

private:
    struct Kinds;
    std::unique_ptr<Kinds> m_kinds;
};

} // namespace cu59
