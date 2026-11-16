#pragma once
#include <string>
#include <vector>

#include "machine/Catalogue.hpp"
#include "media/Kind.hpp"

// Calc-U-1600's kinds of medium (LIB-7 design §10.3; spec AC-37, AC-41,
// AC-42, AC-46): the battery-backed module (`.card.yaml`, version 1, held by
// a module slot), the floppy disk (`.floppy.yaml`, version 2, the CE-1600F's
// place `floppy`, sides 0 and 1) and the cassette tape (`.tape.yaml`,
// version 1, with its WAV, in the tape folder; the cassette's place `tape`).
// Their codecs read and write the files (docs/formats/media.md §2, §3,
// docs/Memory-Card-Definition-Format.md); the content they give is
// core/Machine/SharpMediaPlaces.hpp's ModuleContent, FloppyContent,
// TapeContent, which the library hands through unread.

namespace shared {

inline constexpr const char* kModuleKind = "battery-backed-module";
inline constexpr const char* kFloppyKind = "floppy-disk";
inline constexpr const char* kTapeKind = "cassette";
inline constexpr const char* kTapeFileSuffix = ".tape.yaml";

// The three kinds. `bundledDir` holds the bundled templates (the module
// definitions, `blank` and `formatted` disks, the `empty` tape);
// `catalogue` gets the descriptor of a saved module the first time one is
// plugged in (a module request names a peripheral of the catalogue).
std::vector<calcu::media::KindDecl> mediaKinds(const std::string& bundledDir, calcu::machine::Catalogue& catalogue);

// The modules that are not media in a slot's picker (AC-8): the module's
// name, its description as the tooltip, in the catalogue's order.
calcu::media::ModuleListing moduleListing();

} // namespace shared
