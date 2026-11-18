#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "presets/Reader.hpp"
#include "presets/Vocabulary.hpp"
#include "startup/MachinePlan.hpp"

// ── Calc-U-1600's keywords of the preset format ────────────────────────
//
// Added through the place the preset loader gives a product
// (ProductRules::addKeywords); the loader itself knows none of them:
// - the section `program:`: a program file or the lines of
//   a BASIC listing, carried out by the product's loaders;
// - the header keys `host-drive:`, `slot-1-rom:`,
//   `slot-2-rom:`, `bus-rom:` and `debug:` (app/SharpDebugBlock).
// Every check that needs no machine is made when the preset is read, with
// the line. The host drive, the bus ROMs and the content of a slot
// ROM go into the machine plan through ReadContext::addPeripheral and
// setContentAt as they are read.

namespace cu1600 {

struct SharpKeywordSetup {
    // False where the frontend mounts no folder (the Web); unset: true.
    std::function<bool()> hostFolders;
};

void addSharpKeywords(calcu::presets::Vocabulary&, const SharpKeywordSetup&);

// The pairs of header keys that go together (`slot-N-rom:` and its
// module), at the line of the later key.
void checkSharpHeader(const calcu::presets::LoadPlan&, calcu::presets::ReadContext&);

} // namespace cu1600
