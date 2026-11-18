#pragma once
#include "CueCard.hpp"
#include "formats/Yaml.hpp"
#include <functional>
#include <optional>
#include <string>

// The stand-alone cue card block of docs/formats/cue-card.md §3, §4: a
// preset's `cue-card:` section and the `cue-card` of the constant memory
// file. The keys `title`,
// `row-1`, `row-2`, `row-2-r`, the three alignments, `style`,
// `pencil-color`, the labels `A`–`E` and `A'`–`E'` (`A′` accepted, any case);
// any other key (`id`, `Template`, `Banks`, `model`, …) is refused. A
// stand-alone card always draws as the plain card (CueCardTemplate::CueCard).

namespace cu59 {

/// Each error with its line; nullopt when any was given.
using CueCardError = std::function<void(int line, const std::string& message)>;
std::optional<CueCardContent> readStandAloneCueCard(const calcu::formats::YamlNode& block, const CueCardError& error);

/// The block's lines (`key: value`), each indented by `indent` spaces; empty
/// keys and defaults left out. Values that need it are quoted.
std::string writeStandAloneCueCard(const CueCardContent&, int indent);

} // namespace cu59
