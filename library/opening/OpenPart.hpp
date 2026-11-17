#pragma once
#include "opening/FileKind.hpp"
#include "opening/OpenRequest.hpp"
#include "opening/Openers.hpp"
#include "sessions/SessionPart.hpp"
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

// The open part (LIB-13 design §3.5; spec AC-1, AC-3, AC-4, AC-7, AC-8,
// EC-2 to EC-4, EC-6, EC-7): the command `open`, the request
// `open-would-take`, the file kept while the session is busy. One per app
// session, after the parts of LIB-6 and LIB-7 it opens with.

namespace calcu::opening {

struct OpenSetup {
    std::string productName;                       // test 6's sentence
    PresetOpening* presets = nullptr;
    MediaOpening* media = nullptr;
    std::vector<FileKind*> kinds;                  // in the product's order
    std::map<Way, std::uint64_t> maxBytes;         // AC-10; a way not listed has no limit
    std::optional<std::string> uploadFolder;       // AC-8: where several files become one folder (Web: /uploads)
};

class OpenPart final : public sessions::SessionPart {
public:
    explicit OpenPart(OpenSetup);
    const OpenSetup& setup() const { return setup_; }

    void attach(sessions::SessionSetup&) override;
    void afterCall(sessions::SessionContext&, pacing::WallTime) override;   // opens the file kept (AC-7)

private:
    sessions::CommandOutcome open(const OpenRequest&, sessions::SessionContext&);
    sessions::CommandOutcome openOne(const OpenedFile&, const OpenRequest&, sessions::SessionContext&);
    sessions::CommandOutcome openMedium(const OpenedFile&, const OpenRequest&, sessions::SessionContext&);
    sessions::CommandOutcome openSeveral(const OpenRequest&, sessions::SessionContext&);

    OpenSetup setup_;
    std::optional<OpenRequest> kept_;
    int folders_ = 0;   // open-<n> of AC-8
};

} // namespace calcu::opening
