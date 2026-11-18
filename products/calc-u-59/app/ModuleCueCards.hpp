#pragma once
#include "CueCard.hpp"
#include "roms/RomSet.hpp"
#include <functional>
#include <map>
#include <string>
#include <string_view>

// The module cue cards of a Solid State ROM module
// (docs/formats/modules.md §2.3): the `cue-cards:` items of the
// module's `<ID>.rom.yaml` in the ROM set, each a cue card block plus
// `program` (1–99) and `id`, drawn as a module card (CueCardTemplate::
// SolidState). A file that cannot be read, or an item that breaks the block,
// gives none of its cards and the problem through `problem`.

namespace cu59 {

std::map<int, CueCardContent> moduleCueCards(const calcu::roms::RomSet&, std::string_view moduleID,
                                             const std::function<void(const std::string&)>& problem = {});
/// The same from a module file's text (`file` names it in a problem).
std::map<int, CueCardContent> moduleCueCardsOf(std::string_view text, const std::string& file,
                                               const std::function<void(const std::string&)>& problem = {});

} // namespace cu59
