// The Calc-U-59 Web engine (LIB-15 design §3.6, §4; M-34): the product's
// factory of the library's app session for the library's Web Worker host.
// All the rest is the library's (library/web/glue/: WorkerHost translates
// the session's messages to the page and back) and the app layer's
// (TIAppPart: the machine and the flows; TIWebPart: what the page draws).
// Built by frontends/web/build.sh (CMake target calcu59_web) into dist/wasm/.
//
// File system (the library's worker writes it before the session is made):
// /bundle the ROM full set and cuecards.txt (files.json; LIB-12: looked up
// once, here), /presets the bundled presets, /persist-calc-u-59 the
// persistent file system: settings/Calc-U-59/Calc-U-59.ini (LIB-9 AC-23) and
// the save folder Calc-U-59/ (cards, ti58c.mem; LIB-15 AC-20). Traces go to
// MEMFS /traces (LIB-9 A-3, until M-53).

#include <chrono>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>

#include "../../app/ProductRoms.hpp"
#include "../../app/Settings.hpp"
#include "../../app/TIAppPart.hpp"
#include "../../app/TIWebPart.hpp"
#include "roms/Bundle.hpp"
#include "settings/AtomicFile.hpp"
#include "settings/PlatformBase.hpp"
#include "settings/SettingsFile.hpp"
#include "web/glue/Bind.hpp"

namespace {

namespace s59 = cu59::settings;

// The parts of one worker's session and what they need; the session owns it
// (AppSessionSetup::owned) and lets it go first. The Web part goes before the
// product's part (it tells the part), the settings last.
struct WebParts {
    calcu::settings::SettingsFile file;
    s59::Settings settings{file};
    std::unique_ptr<cu59::TIAppPart> app;
    std::unique_ptr<cu59::TIWebPart> web;
    explicit WebParts(calcu::settings::SettingsFile f) : file(std::move(f)) {}
};

std::unique_ptr<calcu::sessions::AppSession> makeWebAppSession(const calcu::web::WebStart& start) {
    // LIB-12: the full set or no session; the worker shows why (LIB-15 AC-3).
    auto roms = cu59::lookUpRoms(calcu::roms::DirectoryBundle(start.bundleDir));
    if (!roms.ok()) throw std::runtime_error(calcu::roms::describe(roms.missing));

    auto parts = std::make_shared<WebParts>(calcu::settings::SettingsFile::open(
        s59::settingsDecl(), calcu::settings::settingsFilePath(s59::settingsDecl(), start.dirs)));
    std::error_code ec;
    std::filesystem::create_directories(start.dirs.saveFolder, ec);
    const std::filesystem::path traces = "/traces";

    cu59::TIAppSetup setup;
    setup.roms = roms.set;
    setup.cuecards = calcu::settings::readWholeFile(start.bundleDir + "/cuecards.txt").value_or("");
    setup.startModel = parts->settings.startupModel();
    setup.saveFolder = start.dirs.saveFolder;
    setup.trace = {traces, static_cast<uint64_t>(parts->settings.traceMaxFileSizeMB()) * 1'000'000};
    parts->app = std::make_unique<cu59::TIAppPart>(std::move(setup));
    parts->web = std::make_unique<cu59::TIWebPart>(parts->settings, start.presetsDir, traces);
    parts->web->bind(*parts->app);

    calcu::sessions::AppSessionSetup s;
    s.clock = &calcu::web::wallTimeNow;
    s.hostTime = [] { return std::chrono::system_clock::now(); };
    s.parts = {start.shellPart, parts->app.get(), parts->web.get()};
    const std::string problem = parts->file.problem();
    s.owned = std::move(parts);
    auto session = std::make_unique<calcu::sessions::AppSession>(std::move(s));
    if (!problem.empty()) session->context().log("Settings: " + problem);
    return session;
}

} // namespace

CALCU_WEB_PRODUCT(CalcU59Host, makeWebAppSession)
