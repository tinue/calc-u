#pragma once
#include "presets/Vocabulary.hpp"

// The header key `magnetic-cards:` of a TI-59 preset (C59-7 design §3.4;
// spec AC-18 to AC-20; docs/formats/preset.md §3.1): a list of in-file
// cards, each `card-name`, optional `description` and `cue-card`, and
// `sides` with `0`, `1` or both; a side has `bank` (1–4, required),
// `protected` (default false), `partition` (default: the calculator's when
// the side goes in), `program: |` in the keycode notation of §4.1 (bare
// keycodes from the bank's first step) and `data:` in §4.2's notation. Each
// item goes into the library's in-file store (media::defineInFile) as a
// TIMagneticCardData of TICardSideDef sides; what follows (`card: { mem: }`,
// `arm:`, `saveas: { card: }`, the store kept until the next load or model
// switch) is the library's (LIB-7 §3.8). Contents outside the bank, two
// items with one `card-name` and an unknown key refuse the preset with the
// line. The TI rules mark it [59: TI-59] (app/TIPresetRules).

namespace cu59 {

void addMagneticCardsKey(calcu::presets::Vocabulary&);

} // namespace cu59
