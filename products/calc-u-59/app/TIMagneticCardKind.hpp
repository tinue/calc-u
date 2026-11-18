#pragma once
#include "CueCard.hpp"
#include "../core/TIMagneticCard.hpp"
#include "machine/ProductData.hpp"
#include "media/Kind.hpp"
#include <memory>
#include <optional>
#include <string>

// Calc-U-59's one kind of medium, the magnetic card: the KindDecl
// `magnetic-card` and its codec for magnetic card format 3
// (`.magnetcard.yaml`, docs/formats/media.md §1). The content is a
// TIMagneticCardData (core/TIMagneticCard.hpp) whose label is a TICardLabel.
// Outdated: a format version below 3. A `.u59` or `.bin` card of release
// 2.3.0 is not of the kind's suffix: nothing lists, reads or changes it.

namespace cu59 {

/// The card's name, description and cue card, carried through a pass by the
/// core unread, so the card given back keeps them.
struct TICardLabel final : calcu::machine::ProductData {
    std::string name, description;
    std::optional<CueCardContent> cueCard;
    bool isTemplate = false;
};

inline constexpr const char* kMagneticCardKind = "magnetic-card";
inline constexpr const char* kFreshCardTemplate = "Fresh card";
inline constexpr int kMagneticCardFormatVersion = 3;

/// The kind (one for the process); its codec lives as long.
const calcu::media::KindDecl& magneticCardKind();

/// The card a content holds, or nullptr.
std::shared_ptr<const TIMagneticCardData> magneticCardOf(const calcu::media::Content&);
/// Its label, or nullptr.
const TICardLabel* cardLabelOf(const TIMagneticCardData&);

/// The cue card a fitted card shows: its own, or a plain card titled
/// with `name`, never the preset's `cue-card:`; a magnetic card's frame, the banks of its sides
/// in the corners.
CueCardContent fittedCardCue(const TIMagneticCardData* card, const std::string& name);

/// The file's text for a card (format 3, the key order of media.md §1.1);
/// the default partition of an in-file side that never passed is the one
/// of its definition, else the TI-59's at power-on (60 program registers).
std::string writeMagneticCardFile(const TIMagneticCardData& card, const std::string& name, const std::string& description,
                                  bool isTemplate, const std::optional<CueCardContent>& cueCard, const std::string& savedAt);

} // namespace cu59
