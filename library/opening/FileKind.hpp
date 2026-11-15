#pragma once
#include "opening/OpenRequest.hpp"
#include "sessions/SessionPart.hpp"
#include <string_view>

// A product's file kind (LIB-13 design §3.3; spec AC-1 test 4, AC-6;
// frontend.md SP-3, I-4): one sort of file a product opens besides presets
// and media (a BASIC program, an ASM overlay). The product gives its kinds
// in the order they are asked; what `open` does is the product's.

namespace calcu::opening {

class FileKind {
public:
    virtual std::string_view name() const = 0;                     // "basic-program": in the reply
    virtual bool takesName(std::string_view lowerName) const = 0;  // AC-6: by the name alone, for a drag
    virtual bool takes(const OpenedFile&) const = 0;               // AC-1 test 4: by its own rule
    // A kind may refuse what it takes, with its own sentence; the open part
    // adds the notice.
    virtual sessions::CommandOutcome open(const OpenedFile&, Way, sessions::SessionContext&) = 0;

protected:
    ~FileKind() = default;
};

} // namespace calcu::opening
