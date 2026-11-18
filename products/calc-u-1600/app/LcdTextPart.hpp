#pragma once
#include <optional>

#include "Display/LcdText.hpp"
#include "harness/DisplayText.hpp"
#include "machine/Descriptor.hpp"
#include "machine/Display.hpp"
#include "sessions/SessionPart.hpp"

// The harness's `display-text` on a Sharp machine: the font of the model's ROM, read when the machine changes,
// and the cursor the ROM reports through its RAM, read at each flush; the
// cell under the cursor reads as the character the ROM saved there, as
// `--lcd-text` of the old CLIs did.

namespace cu1600 {

class LcdTextPart final : public calcu::sessions::SessionPart {
public:
    void attach(calcu::sessions::SessionSetup&) override {}
    void machineChanged(calcu::sessions::SessionContext&) override;
    void beforeFlush(calcu::sessions::SessionContext&) override;

    calcu::harness::DisplayText read(const calcu::machine::DisplayOutput&, const calcu::machine::MachineDescriptor&) const;

private:
    LcdFont font_;
    calcu::harness::GlyphFont glyphs_;
    std::optional<LcdCursor> cursor_;
};

} // namespace cu1600
