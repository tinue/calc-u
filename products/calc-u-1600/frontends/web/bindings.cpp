// The Calc-U-1600 Web engine (LIB-15 design §3.6, §4; M-31): the product's
// factory of the library's app session for the library's Web Worker host.
// All the rest is the library's (library/web/glue/: WorkerHost translates
// the session's messages to the page and back; shell/HostSession: the
// settings file, LIB-14's SettingsPart, the order) and the app layer's
// (SharpAppPart: the machine, the operations; SharpWebPart: what only the
// page asks for). The page is put together from the React set's shell
// (LIB-14 Part D), so the session also has LIB-4's ControlPart (with the
// product's rules: no host folder in a browser, AC-18) and LIB-14's
// SettingsPart. Built by frontends/web/build.sh (CMake target calcu_web)
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
    shared::Settings typed{settings.file()};
    std::unique_ptr<shared::AppPaths> paths;
    std::unique_ptr<shared::SharpAppPart> part;
    std::unique_ptr<shared::SharpWebPart> web;
    explicit WebParts(const calcu::settings::HostDirs& dirs) : settings(shared::settingsDecl(), dirs) {}
};

std::unique_ptr<calcu::sessions::AppSession> makeWebAppSession(const calcu::web::WebStart& start) {
    const calcu::settings::HostDirs dirs = start.placesDirs();
    auto parts = std::make_shared<WebParts>(dirs);
    parts->paths = std::make_unique<shared::AppPaths>(parts->typed, dirs,
                                                      shared::BundleDirs{start.bundleDir, start.presetsDir});
    parts->part = std::make_unique<shared::SharpAppPart>(parts->typed, *parts->paths,
                                                         shared::SharpAppPartOptions{/*canMountHostFolder=*/false});
    parts->web = std::make_unique<shared::SharpWebPart>(*parts->part, start.uploadsDir);

    // The one order (parts::LibraryParts): the control part, the shell's and
    // SettingsPart, the library's and the product's, the Web part last.
    calcu::shell::HostSessionSetup host;
    host.settings = &parts->settings;
    host.before = {start.shellPart};
    host.after = {parts->web.get()};
    host.clock = &calcu::web::wallTimeNow;
    host.logSettingsProblem = true;
    shared::SharpAppPart& part = *parts->part;
    host.owned = std::move(parts);
    return calcu::shell::makeHostSession(part, std::move(host)).session;
}

} // namespace

CALCU_WEB_PRODUCT(CalcU1600Host, makeWebAppSession)
