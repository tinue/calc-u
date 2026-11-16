#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "presets/Reader.hpp"
#include "presets/Vocabulary.hpp"
#include "startup/MachinePlan.hpp"

// ── Calc-U-1600's keywords of the preset format (C1600-4 design §3) ─────
//
// Added through the place LIB-6 gives a product (ProductRules::addKeywords,
// AC-29); the loader itself knows none of them:
// - the section `program:` (AC-1 to AC-5): a program file or the lines of
//   a BASIC listing, carried out by C1600-2's loaders;
// - the header keys `host-drive:` (AC-6, AC-7), `slot-1-rom:`,
//   `slot-2-rom:` (AC-8), `bus-rom:` (AC-9, AC-10) and `debug:` (AC-11,
//   AC-12; app/SharpDebugBlock).
// Every check that needs no machine is made when the preset is read, with
// the line (AC-14). The host drive, the bus ROMs and the content of a slot
// ROM go into the machine plan through ReadContext::addPeripheral and
// setContentAt as they are read (D-2; LIB-6 D-33).

namespace shared {

struct SharpKeywordSetup {
    // AC-7: false where the frontend mounts no folder (the Web); unset: true.
    std::function<bool()> hostFolders;
};

void addSharpKeywords(calcu::presets::Vocabulary&, const SharpKeywordSetup&);

// D-32: the pairs of header keys that go together (`slot-N-rom:` and its
// module; AC-8), at the line of the later key.
void checkSharpHeader(const calcu::presets::LoadPlan&, calcu::presets::ReadContext&);

} // namespace shared
