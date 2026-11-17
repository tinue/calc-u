#pragma once
#include "opening/FileKind.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// Calc-U-59's file kinds of LIB-13's `open` (LIB-13 design §10.4; spec
// AC-11, AC-13, AC-23; C59-4 AC-19), asked in this order (D-16), each by its
// name alone (takesName is its rule, D-20):
//
//   asm-overlay           a name ending in .asm: read as by Select File (C59-4)
//   keystroke-program     .keystrokes.yaml: its program written into the
//                         running machine from step 000, without a cold boot
//   rom-module-file       .rom.yaml: refused, a module is taken by its ID only
//   constant-memory-file  ti58c.memory.yaml: refused, the product's own file
//
// The library decides presets (.ti59, .ti58, .ti58c, or a `model:` line)
// and media before them: a magnetic card (`.magnetcard.yaml`) is LIB-7's
// medium since M-55, fitted into the TI-59's card reader by the library's
// MediaOpener (the interim `magnetic-card` kind of M-56 for 2.3.0's .u59 and
// .bin went with it; those are no files of the product, C59-7 AC-4).

namespace cu59 {

class TIDebugPart;

/// The product's preset extensions (AC-11).
std::vector<std::string> tiPresetExtensions();

/// The Qt6 program's arguments with its own options and their values taken
/// out (`--grab x`, `--after n`, `--pacing-probe s`; LIB-13 D-18), the
/// program's name kept first: what calcu::opening::startFile reads, so the
/// start file is the last argument that does not start with `-` and names an
/// existing file (ledger 59-47), never a value of an option.
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
