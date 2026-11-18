#pragma once
#include "opening/Openers.hpp"
#include <string>
#include <vector>

// The two seams of Openers.hpp over the library's parts: the presets part's
// `load-preset` ({ref} from its folder, {content, name} as its content) and
// the media part (its reads of opened files and the
// command `media.open-file`). Only this file depends on their form.

namespace calcu::presets {
class PresetsPart;
}
namespace calcu::media {
class MediaPart;
}

namespace calcu::opening {

class PresetsOpener final : public PresetOpening {
public:
    // `extensions`: the product's preset extensions (".ti59"), any case.
    PresetsOpener(presets::PresetsPart&, std::vector<std::string> extensions);
    std::vector<std::string> extensions() const override { return extensions_; }
    sessions::CommandOutcome loadFile(const std::string& path, sessions::SessionContext&) override;
    sessions::CommandOutcome loadContent(const OpenedFile&, sessions::SessionContext&) override;

private:
    presets::PresetsPart* part_;
    std::vector<std::string> extensions_;
};

class MediaOpener final : public MediaOpening {
public:
    explicit MediaOpener(media::MediaPart&);
    std::vector<std::string> suffixes() const override;
    MediumFile read(const OpenedFile&) const override;
    std::vector<std::string> placesFor(const MediumFile&, const OpenedFile&, const machine::Machine&) const override;
    std::vector<std::string> placesInCatalogue(const MediumFile&) const override;
    std::optional<std::string> refusesNow(const std::string& place, const machine::Machine&) const override;
    sessions::CommandOutcome useWhereItLies(const OpenedFile&, const std::string& place, sessions::SessionContext&) override;
    sessions::CommandOutcome importAndUse(const OpenedFile&, const std::optional<std::string>& place, bool confirmed,
                                          sessions::SessionContext&) override;

private:
    media::MediaPart* part_;
};

} // namespace calcu::opening
