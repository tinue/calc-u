#pragma once
#include "media/Kind.hpp"
#include "media/MediaPart.hpp"
#include "presets/Reader.hpp"
#include "presets/Vocabulary.hpp"
#include <memory>
#include <string>

// Media in presets (docs/formats/preset.md): the slot keys' resolver, the
// in-file media of a read, and on the presets' place for each kind its insert
// and arm steps (named by the product), and once the library's steps `eject:`
// and `saveas:`. Every reference is resolved when the preset is read; the
// steps are also harness commands.

namespace calcu::media {

// The in-file media of one read, kept with the read (ReadContext::keep).
struct InFileSet final : presets::ProductValue {
    InFileDefs defs;
};
inline constexpr const char* kInFileKey = "calcu.media.in-file";

void addMediaToPresets(presets::Vocabulary&, MediaPart&);

// For a product's header key (Calc-U-59's `magnetic-cards:`): an in-file
// medium of `kind` defined by the preset being read. The library knows only
// its name and content.
void defineInFile(presets::ReadContext&, const KindDecl&, std::string name, Content);

} // namespace calcu::media
