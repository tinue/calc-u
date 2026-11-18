#pragma once
#include "machine/Names.hpp"
#include "opening/OpenRequest.hpp"
#include "sessions/SessionPart.hpp"
#include <string_view>

// A product's file kind (test 4 of Classify.hpp): one sort of file a product opens besides presets
// and media (a BASIC program, an ASM overlay). The product gives its kinds
// in the order they are asked; what `open` does is the product's.

namespace calcu::opening {

class FileKind {
public:
    virtual std::string_view name() const = 0;                     // "basic-program": in the reply
    virtual bool takesName(std::string_view lowerName) const = 0;  // by the name alone, for a drag
    // Test 4: by its own rule; by default by the name, as takesName.
    virtual bool takes(const OpenedFile& f) const { return takesName(machine::lowerAscii(f.name)); }
    // A kind may refuse what it takes, with its own sentence; the open part
    // adds the notice.
    virtual sessions::CommandOutcome open(const OpenedFile&, Way, sessions::SessionContext&) = 0;

protected:
    ~FileKind() = default;
};

} // namespace calcu::opening
