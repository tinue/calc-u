// The Calc-U-1600 Web engine (LIB-15 design §3.6, §4; M-31): the product's
// factory of the library's app session for the library's Web Worker host.
// All the rest is the library's (library/web/glue/: WorkerHost translates
// the session's messages to the page and back) and the app layer's
// (SharpAppPart: the machine, the operations; SharpWebPart: what only the
// page asks for). The page is put together from the React set's shell
// (LIB-14 Part D), so the session also has LIB-4's ControlPart (with the
// product's rules: no host folder in a browser, AC-18) and LIB-14's
// SettingsPart. Built by frontends/web/build.sh (CMake target calcu_web)
// into dist/wasm/.

#include <filesystem>
#include <memory>
#include <string>

#include "app/AppPaths.hpp"
#include "app/Settings.hpp"
#include "app/SharpAppPart.hpp"
#include "app/SharpWebPart.hpp"
#include "settings/PlatformBase.hpp"
#include "settings/SettingsFile.hpp"
#include "shell/SettingsPart.hpp"
#include "web/glue/Bind.hpp"

namespace {

// The parts of one worker's session and what they need; the session owns it
// (AppSessionSetup::owned) and lets it go first.
struct WebParts {
    calcu::settings::SettingsFile file;
    shared::Settings settings{file};
    std::unique_ptr<shared::AppPaths> paths;
    std::unique_ptr<shared::SharpAppPart> part;
    std::unique_ptr<shared::SharpWebPart> web;
    std::unique_ptr<calcu::shell::SettingsPart> settingsPart;
    explicit WebParts(calcu::settings::SettingsFile f) : file(std::move(f)) {}
};

std::unique_ptr<calcu::sessions::AppSession> makeWebAppSession(const calcu::web::WebStart& start) {
    calcu::settings::HostDirs dirs = start.dirs;
    if (dirs.home.empty()) dirs.home = std::filesystem::path(dirs.saveFolder).parent_path().generic_string();   // /persist
    auto parts = std::make_shared<WebParts>(calcu::settings::SettingsFile::open(
        shared::settingsDecl(), calcu::settings::settingsFilePath(shared::settingsDecl(), dirs)));
    parts->paths = std::make_unique<shared::AppPaths>(parts->settings, dirs,
                                                      shared::BundleDirs{start.bundleDir, start.presetsDir});
    parts->part = std::make_unique<shared::SharpAppPart>(parts->settings, *parts->paths,
                                                         shared::SharpAppPartOptions{/*canMountHostFolder=*/false});
    parts->web = std::make_unique<shared::SharpWebPart>(*parts->part, start.uploadsDir);
    calcu::shell::SettingsPartSetup ss;
    ss.file = &parts->file;
    ss.paths = &parts->paths->places();
    parts->settingsPart = std::make_unique<calcu::shell::SettingsPart>(std::move(ss));

    calcu::sessions::AppSessionSetup setup;
    setup.clock = &calcu::web::wallTimeNow;
    // The one order (parts::LibraryParts): the control part, the shell's and
    // SettingsPart, the library's and the product's, the Web part last.
    setup.parts = parts->part->sessionParts({start.shellPart, parts->settingsPart.get()}, {parts->web.get()});
    const std::string problem = parts->file.problem();
    setup.owned = std::move(parts);
    auto session = std::make_unique<calcu::sessions::AppSession>(std::move(setup));
    if (!problem.empty()) session->context().log("Settings: " + problem);
    return session;
}

} // namespace

CALCU_WEB_PRODUCT(CalcU1600Host, makeWebAppSession)
