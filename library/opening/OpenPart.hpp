#pragma once
#include "opening/FileKind.hpp"
#include "opening/OpenRequest.hpp"
#include "sessions/SessionPart.hpp"
#include "settings/SettingsFile.hpp"
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

// The open part: the command `open`, the request `open-would-take`, the
// file kept while the session is busy. One per app session, after the
// presets and media parts it opens with: a preset is the presets part's
// `load-preset` ({ref} from its folder, {content, name} as its content), a
// medium the media part's reads of opened files and `media.open-file`. A
// refusal of theirs becomes the open part's notice.
//
// `choose-startup-preset {model, ref | files: [{name, bytes}]}`: the
// settings dialog's "Choose…". The preset (a path; or, on the Web, chosen
// with its files, which all go into the file cache) is checked as the
// startup load reads it; only then it becomes [preset] <model>. A preset
// that does not pass is a notice with the reason, the setting unchanged.
// Never held back while an operation runs.

namespace calcu::presets {
class PresetsPart;
}
namespace calcu::media {
class MediaPart;
}

namespace calcu::opening {

struct OpenSetup {
    std::string productName;                       // test 6's sentence
    presets::PresetsPart* presets = nullptr;       // null: no presets are opened
    std::vector<std::string> presetExtensions;     // ".ti59", ".pc1500a": any case
    media::MediaPart* media = nullptr;             // null: no media are opened; its kinds' suffixes
    std::vector<FileKind*> kinds;                  // in the product's order
    std::map<Way, std::uint64_t> maxBytes;         // a way not listed has no limit
    std::optional<std::string> uploadFolder;       // where several files become one folder (Web: /uploads)
    std::optional<std::string> cacheFolder;        // the Web's file cache: every uploaded file is kept there
    settings::SettingsFile* settings = nullptr;    // [preset]; null: no choose-startup-preset
};

class OpenPart final : public sessions::SessionPart {
public:
    explicit OpenPart(OpenSetup);
    const OpenSetup& setup() const { return setup_; }

    void attach(sessions::SessionSetup&) override;
    void afterCall(sessions::SessionContext&, pacing::WallTime) override;   // opens the file kept

private:
    sessions::CommandOutcome open(const OpenRequest&, sessions::SessionContext&);
    sessions::CommandOutcome openOne(const OpenedFile&, const OpenRequest&, sessions::SessionContext&);
    sessions::CommandOutcome openMedium(const OpenedFile&, const OpenRequest&, sessions::SessionContext&);
    sessions::CommandOutcome openSeveral(const OpenRequest&, sessions::SessionContext&);
    sessions::CommandOutcome chooseStartup(const sessions::Value&, sessions::SessionContext&);
    void keep(const std::vector<OpenedFile>&, const OpenedFile* except, sessions::SessionContext&) const;
    const OpenedFile* presetAmong(const std::vector<OpenedFile>&) const;

    OpenSetup setup_;
    std::optional<OpenRequest> kept_;
    int folders_ = 0;   // the <n> of the next upload folder open-<n>
};

} // namespace calcu::opening
