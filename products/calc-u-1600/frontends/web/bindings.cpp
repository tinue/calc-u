// The Calc-U-1600 Web engine (LIB-15 design §3.6, §4; M-31): the product's
// factory of the library's app session for the library's Web Worker host.
// All the rest is the library's (library/web/glue/: WorkerHost translates
// the session's messages to the page and back) and the app layer's
// (SharpAppPart: the machine, the operations; SharpWebPart: what the page
// draws). Built by frontends/web/build.sh (CMake target calcu_web) into
// dist/wasm/.

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

// What the session of one worker keeps alive: destroyed after the session
// (WebSession), so the parts let go of it first.
struct WebKeep {
    calcu::settings::SettingsFile file;
    shared::Settings settings{file};
    std::unique_ptr<shared::AppPaths> paths;
    std::unique_ptr<shared::SharpAppPart> part;
    std::unique_ptr<shared::SharpWebPart> web;
    explicit WebKeep(calcu::settings::SettingsFile f) : file(std::move(f)) {}
    ~WebKeep() {
        if (part) part->forgetSession();   // the session is gone already
    }
};

calcu::web::WebSession makeWebAppSession(const calcu::web::WebStart& start) {
    calcu::settings::HostDirs dirs = start.dirs;
    if (dirs.home.empty()) dirs.home = dirs.saveFolder;
    auto keep = std::make_shared<WebKeep>(calcu::settings::SettingsFile::open(
        shared::settingsDecl(), calcu::settings::settingsFilePath(shared::settingsDecl(), dirs)));
    keep->paths = std::make_unique<shared::AppPaths>(keep->settings, dirs,
                                                     shared::BundleDirs{start.bundleDir, start.presetsDir});
    keep->part = std::make_unique<shared::SharpAppPart>(keep->settings, *keep->paths);
    keep->web = std::make_unique<shared::SharpWebPart>(*keep->part, keep->settings, *keep->paths, start.uploadsDir);

    calcu::sessions::AppSessionSetup setup;
    setup.clock = &calcu::web::wallTimeNow;
    setup.parts.push_back(start.shellPart);
    for (calcu::sessions::SessionPart* p : keep->part->sessionParts()) setup.parts.push_back(p);
    setup.parts.push_back(keep->web.get());
    calcu::web::WebSession out;
    out.session = std::make_unique<calcu::sessions::AppSession>(std::move(setup));
    keep->part->bind(*out.session);
    if (!keep->file.problem().empty()) out.session->context().log("Settings: " + keep->file.problem());
    out.keep = std::move(keep);
    return out;
}

} // namespace

CALCU_WEB_PRODUCT(CalcU1600Host, makeWebAppSession)
