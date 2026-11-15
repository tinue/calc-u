#pragma once
#include "media/Kind.hpp"
#include "media/MediaPart.hpp"
#include "presets/Reader.hpp"
#include "presets/Vocabulary.hpp"
#include <memory>
#include <string>

// Media in presets (LIB-7 design §3.8; spec AC-21 to AC-27, EC-5 to EC-7;
// LIB-6 AC-3, AC-29): the slot keys' resolver, the in-file media of a read,
// and on LIB-6's place for each kind its insert and arm steps, and once the
// steps `eject:` and `saveas:` (D-20). Every reference is resolved when the
// preset is read (AC-23); the steps are also harness commands (LIB-6 §3.9).

namespace calcu::media {

// The in-file media of one read (AC-22), kept with the read (ReadContext::keep).
struct InFileSet final : presets::ProductValue {
    InFileDefs defs;
};
inline constexpr const char* kInFileKey = "calcu.media.in-file";

void addMediaToPresets(presets::Vocabulary&, MediaPart&);

// For a product's header key (`magnetic-cards:`, C59-7): an in-file medium of
// `kind` defined by the preset being read. The library knows only its name
// and content.
void defineInFile(presets::ReadContext&, const KindDecl&, std::string name, Content);

} // namespace calcu::media
