#pragma once
#include "CueCard.hpp"
#include "roms/RomSet.hpp"
#include <functional>
#include <map>
#include <string>
#include <string_view>

// The module cue cards of a Solid State ROM module (C59-7 design §3.5; spec
// AC-22, AC-23; docs/formats/modules.md §2.3): the `cue-cards:` items of the
// module's `<ID>.rom.yaml` in the ROM set, each a cue card block plus
// `program` (1–99) and `id`, drawn as a module card (CueCardTemplate::
// SolidState). They replace 2.3.0's roms/solid-state/cuecards.txt (LIB-12
// AC-14). A file that cannot be read, or an item that breaks the block,
// gives none of its cards and the problem through `problem`.

namespace cu59 {

std::map<int, CueCardContent> moduleCueCards(const calcu::roms::RomSet&, std::string_view moduleID,
                                             const std::function<void(const std::string&)>& problem = {});

} // namespace cu59
