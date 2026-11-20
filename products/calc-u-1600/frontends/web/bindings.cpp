// The Calc-U-1600 Web engine: the product's
// factory of the library's app session for the library's Web Worker host.
// All the rest is the library's (library/web/glue/: WorkerHost translates
// the session's messages to the page and back; shell/HostSession: the
// settings file, the library's SettingsPart, the order) and the app layer's
// (SharpAppPart: the machine, the operations; SharpWebPart: what only the
// page asks for). The page is put together from the React set's shell
// so the session also has the library's ControlPart (with the product's
// rules: no host folder in a browser) and its SettingsPart. Built by frontends/web/build.sh (CMake target calcu1600_web)
// into dist/wasm/.

#include <memory>
#include <string>

#include "app/AppPaths.hpp"
#include "app/Settings.hpp"
#include "app/SharpAppPart.hpp"
#include "app/SharpWebPart.hpp"
#include "shell/HostSession.hpp"
#include "web/glue/Bind.hpp"

namespace {

// The parts of one worker's session and what they need; the session owns it
// (AppSessionSetup::owned) and lets it go first.
struct WebParts {
    calcu::shell::HostSettings settings;
    cu1600::Settings typed{settings.file()};
    std::unique_ptr<cu1600::AppPaths> paths;
    std::unique_ptr<cu1600::SharpAppPart> part;
    std::unique_ptr<cu1600::SharpWebPart> web;
    explicit WebParts(const calcu::settings::HostDirs& dirs) : settings(cu1600::settingsDecl(), dirs) {}
};

std::unique_ptr<calcu::sessions::AppSession> makeWebAppSession(const calcu::web::WebStart& start) {
    const calcu::settings::HostDirs dirs = start.placesDirs();
    auto parts = std::make_shared<WebParts>(dirs);
    parts->paths = std::make_unique<cu1600::AppPaths>(parts->typed, dirs,
                                                      cu1600::BundleDirs{start.bundleDir, start.presetsDir});
    cu1600::SharpAppPartOptions options;
    options.canMountHostFolder = false;
    options.uploadFolder = start.uploadsDir;   // several files of Load Preset…
    parts->part = std::make_unique<cu1600::SharpAppPart>(parts->typed, *parts->paths, std::move(options));
    parts->web = std::make_unique<cu1600::SharpWebPart>(*parts->part, start.uploadsDir);

    // The one order (parts::LibraryParts): the control part, the shell's and
    // SettingsPart, the library's and the product's, the Web part last.
    calcu::shell::HostSessionSetup host;
    host.settings = &parts->settings;
    host.before = {start.shellPart};
    host.after = {parts->web.get()};
    host.clock = &calcu::web::wallTimeNow;
    host.logSettingsProblem = true;
    cu1600::SharpAppPart& part = *parts->part;
    host.owned = std::move(parts);
    return calcu::shell::makeHostSession(part, std::move(host)).session;
}

} // namespace

CALCU_WEB_PRODUCT(CalcU1600Host, makeWebAppSession)
