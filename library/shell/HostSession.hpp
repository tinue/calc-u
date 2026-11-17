#pragma once
#include "pacing/WallClock.hpp"
#include "sessions/AppSession.hpp"
#include "sessions/SessionPart.hpp"
#include "settings/Declaration.hpp"
#include "settings/HostDirs.hpp"
#include "settings/Paths.hpp"
#include "settings/SettingsFile.hpp"
#include "shell/SettingsPart.hpp"
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// A host's side of a product's app session, written once (Part E
// simplification E-2, docs/reviews/e-simplification.md S-1): what every
// frontend host (Qt6, the Web engine, the Swift bridge) and the products'
// own session factories put around the product's part. HostSettings opens
// the product's settings file at the host's place (LIB-9 AC-5, AC-6) and
// keeps the places over it; makeHostSession() makes LIB-14's SettingsPart
// over them, puts it after the host's own parts in the product's
// sessionParts() (the one order of parts::LibraryParts), lets the session
// own what the host made, writes "Settings: <problem>" into LIB-3's log
// where the host says so and sends LIB-18's `start` where the host says so.
// Single-threaded: on the session's thread.

namespace calcu::shell {

// LIB-9: the product's settings file, read once from `filePath` (default:
// settingsFilePath(decl, dirs), AC-2), and the host's places over it. A file
// that cannot be read is in problem() (EC-6); the host logs it.
class HostSettings {
public:
    HostSettings(const settings::ProductDecl& decl, settings::HostDirs dirs,
                 std::optional<std::string> filePath = std::nullopt);
    HostSettings(const HostSettings&) = delete;
    HostSettings& operator=(const HostSettings&) = delete;

    settings::SettingsFile& file() { return file_; }
    const settings::SettingsFile& file() const { return file_; }
    const settings::Paths& paths() const { return paths_; }
    const std::string& problem() const { return file_.problem(); }

private:
    settings::SettingsFile file_;
    settings::Paths paths_;   // over file_
};

struct HostSessionSetup {
    // LIB-14's SettingsPart over the host's settings; null: none (a test's
    // session without the shell's dialog).
    HostSettings* settings = nullptr;
    std::function<void(std::string_view section, std::string_view key)> settingsChanged;   // SettingsPartSetup::changed
    std::vector<std::string> settingsAlsoShown;                                            // SettingsPartSetup::alsoShown
    // The host's own parts: `before` ahead of SettingsPart (the Web: the
    // shell's part), both after the control part; `after` behind the
    // product's (the Web part).
    std::vector<sessions::SessionPart*> before;
    std::vector<sessions::SessionPart*> after;
    pacing::WallClock clock;   // default: the steady clock
    std::function<std::chrono::system_clock::time_point()> hostTime;   // default: the system clock
    sessions::Hosting hosting = sessions::Hosting::Paced;
    std::optional<std::uint32_t> jitterSeed;
    // The product's part and what it and the host's parts need (settings,
    // places, the Web part): the session owns it and lets it go first, after
    // SettingsPart (AppSessionSetup::owned).
    std::shared_ptr<void> owned;
    // The Web engine: "Settings: <problem>" into LIB-3's log once the session
    // is made (the Qt6 hosts write it to their own log when they open the
    // file; the Swift bridge hands it to the app).
    bool logSettingsProblem = false;
    // LIB-18's `start`, sent once when the session is made (the Swift bridge,
    // the tests); the Qt6 hosts send it after their timer starts, the Web page
    // itself.
    bool start = false;
};

namespace detail {
std::unique_ptr<SettingsPart> settingsPartOf(HostSessionSetup&);
std::unique_ptr<sessions::AppSession> hostSessionOf(HostSessionSetup, std::vector<sessions::SessionPart*> parts,
                                                    std::unique_ptr<SettingsPart>);
} // namespace detail

// The session of `part` (owned through `setup.owned`): its
// sessionParts(host, after) with the host's parts and SettingsPart.
template <class Part> sessions::ProductSession<Part> makeHostSession(Part& part, HostSessionSetup setup) {
    std::unique_ptr<SettingsPart> settingsPart = detail::settingsPartOf(setup);
    std::vector<sessions::SessionPart*> host = setup.before;
    if (settingsPart) host.push_back(settingsPart.get());
    std::vector<sessions::SessionPart*> parts = part.sessionParts(std::move(host), setup.after);
    sessions::ProductSession<Part> out;
    out.part = &part;
    out.session = detail::hostSessionOf(std::move(setup), std::move(parts), std::move(settingsPart));
    return out;
}

} // namespace calcu::shell
