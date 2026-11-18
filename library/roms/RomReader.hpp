#pragma once
#include "machine/Descriptor.hpp"
#include "roms/FullSet.hpp"
#include <cstdint>
#include <span>
#include <string>
#include <vector>

// What a product adds to the lookup: the reader of its text ROM files and the checks of its full set
// beyond the amount of each file. Binary files need no reader.

namespace calcu::roms {

class RomSet;

// What is wrong with one file of the full set. Defined here because
// a product's checkSet() reports in it.
enum class Problem { Missing, WrongAmount, Unreadable, ProductCheck, Catalogue };
struct FileProblem {
    std::string fileName;
    Problem problem;
    std::string detail;               // for the message; may be empty
    std::vector<NeededBy> neededBy;   // filled by lookUp() when the reader leaves it empty
};

class ProductRomReader {
public:
    virtual ~ProductRomReader() = default;
    struct Decoded {
        std::uint64_t amount = 0;   // what the file yields, in the entry's unit
        std::string error;          // set: the reader cannot read it
    };
    // Text entries only. The bytes are the file as bundled; line endings are
    // the reader's business.
    virtual Decoded decode(const machine::RomEntry&, std::span<const std::uint8_t> bytes) const = 0;
    // The product's added checks; run only when every file passed its own.
    virtual std::vector<FileProblem> checkSet(const RomSet&) const { return {}; }
};

} // namespace calcu::roms
