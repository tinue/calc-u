#pragma once

// The parts of an app's session the attached harness's commands need
// (LIB-14 design Part D, D-27): a frontend that has none leaves them null;
// the preset steps and `media` then say so. In a header of its own so that
// a host (qt6/SessionHost.hpp) names it in builds without attach.

namespace calcu::presets {
class Vocabulary;
}
namespace calcu::media {
class MediaPart;
}

namespace calcu::harness {

struct SessionParts {
    const presets::Vocabulary* vocabulary = nullptr;   // the PresetsPart's
    media::MediaPart* media = nullptr;
};

} // namespace calcu::harness
