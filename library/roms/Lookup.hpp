#pragma once
#include "roms/Bundle.hpp"
#include "roms/FullSet.hpp"
#include "roms/RomReader.hpp"
#include "roms/RomSet.hpp"
#include <memory>
#include <string>
#include <vector>

// The one ROM lookup (LIB-12 design §3.5; AC-2 to AC-7; ADR 0035). A
// product calls it once at its start, through its lookUpRoms(): every file
// of the full set is read from the Bundle by exact name and checked by its
// amount (a binary file by its size in bytes, a text file by what the
// product's reader yields), then by the product's added checks. The md5 is
// checked by fetch_roms.sh at build time, never here (AC-3). Either every
// file is good and there is a RomSet, or nothing can be built and the
// result names every file that is missing or bad (AC-5). A second source of
// ROM files (a user's ROM folder, ADR 0035) would be added here only.

namespace calcu::roms {

struct RomsMissing {
    std::vector<FileProblem> files;   // one row per file (EC-1)
    std::string where;                // Bundle::where()
};

struct LookupResult {
    std::shared_ptr<const RomSet> set;   // exactly one of the two
    RomsMissing missing;
    bool ok() const { return set != nullptr; }
};

// `reader` may be null for a product without text files.
LookupResult lookUp(const FullSet&, const Bundle&, const ProductRomReader*);

// The message of AC-5, as text: names every file and its problem, says that
// the full set is needed, and how to get it in a local build. The Qt6
// dialogs, the Swift window, the Web page and the tools show it; the drawn
// form is LIB-14's and C59-8's.
std::string describe(const RomsMissing&);

} // namespace calcu::roms
