// The Calc-U-1600 Web engine (LIB-15 design §3.6, §4; M-31): the product's
// factory of the library's app session for the library's Web Worker host.
// All the rest is the library's (library/web/glue/: WorkerHost translates
// the session's messages to the page and back) and the app layer's
// (SharpAppPart: the machine, the operations; SharpWebPart: what the page
// draws). Built by frontends/web/build.sh (CMake target calcu_web) into
// dist/wasm/.

#include <filesystem>
#include <memory>
#include <string>

#include "app/AppPaths.hpp"
#include "app/Settings.hpp"
#include "app/SharpAppPart.hpp"
#include "app/SharpWebPart.hpp"
#include "settings/PlatformBase.hpp"
#include "settings/SettingsFile.hpp"
#include "web/glue/Bind.hpp"

namespace {

// The parts of one worker's session and what they need; the session owns it
// (AppSessionSetup::owned) and lets it go first. The Web part goes before the
// product's part, the settings last.
struct WebParts {
    calcu::settings::SettingsFile file;
    shared::Settings settings{file};
    std::unique_ptr<shared::AppPaths> paths;
    std::unique_ptr<shared::SharpAppPart> part;
    std::unique_ptr<shared::SharpWebPart> web;
    explicit WebParts(calcu::settings::SettingsFile f) : file(std::move(f)) {}
};

std::unique_ptr<calcu::sessions::AppSession> makeWebAppSession(const calcu::web::WebStart& start) {
    calcu::settings::HostDirs dirs = start.dirs;
    if (dirs.home.empty()) dirs.home = std::filesystem::path(dirs.saveFolder).parent_path().generic_string();   // /persist
    auto parts = std::make_shared<WebParts>(calcu::settings::SettingsFile::open(
        shared::settingsDecl(), calcu::settings::settingsFilePath(shared::settingsDecl(), dirs)));
    parts->paths = std::make_unique<shared::AppPaths>(parts->settings, dirs,
                                                      shared::BundleDirs{start.bundleDir, start.presetsDir});
    parts->part = std::make_unique<shared::SharpAppPart>(parts->settings, *parts->paths);
    parts->web = std::make_unique<shared::SharpWebPart>(*parts->part, parts->settings, *parts->paths, start.uploadsDir);

    calcu::sessions::AppSessionSetup setup;
    setup.clock = &calcu::web::wallTimeNow;
    setup.parts.push_back(start.shellPart);
    for (calcu::sessions::SessionPart* p : parts->part->sessionParts()) setup.parts.push_back(p);
    setup.parts.push_back(parts->web.get());
    const std::string problem = parts->file.problem();
    setup.owned = std::move(parts);
    auto session = std::make_unique<calcu::sessions::AppSession>(std::move(setup));
    if (!problem.empty()) session->context().log("Settings: " + problem);
    return session;
}

} // namespace

CALCU_WEB_PRODUCT(CalcU1600Host, makeWebAppSession)
