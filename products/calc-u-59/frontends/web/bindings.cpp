// The Calc-U-59 Web engine: the product's
// factory of the library's app session for the library's Web Worker host.
// All the rest is the library's (library/web/glue/: WorkerHost translates
// the session's messages to the page and back; shell/HostSession: the
// settings file, the shell's SettingsPart, the order) and the app layer's
// (TIAppPart: the machine and the flows; TIWebPart: what the page draws).
// Built by frontends/web/build.sh (CMake target calcu59_web) into dist/wasm/.
//
// File system (the library's worker writes it before the session is made):
// /bundle the ROM full set (files.json; looked up once, here; the
// cue cards are the module files'), /presets the bundled examples (examples/bundle.yaml and the
// files it lists; the bundle paths `examples:<path>`),
// /persist-calc-u-59 the persistent file system:
// settings/Calc-U-59/Calc-U-59.ini and the save folder
// Calc-U-59/ (the magnetic cards of the library's media part;
// ti58c.memory.yaml). No trace
// folder: a trace is kept in memory and handed over with KeepFile when it
// ends.

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>

#include "../../app/ProductRoms.hpp"
#include "../../app/Settings.hpp"
#include "../../app/TIAppPart.hpp"
#include "../../app/TIWebPart.hpp"
#include "roms/Bundle.hpp"
#include "shell/HostSession.hpp"
#include "web/glue/Bind.hpp"

namespace {

namespace s59 = cu59::settings;

// The parts of one worker's session and what they need; the session owns it
// (AppSessionSetup::owned) and lets it go first. TIAppPart brings every
// part in the library's one order (sessionParts()): the control part, the
// shell's part and its SettingsPart (the `settings` topic and
// `set-setting` of the shell's dialog), the library's parts, the program
// memory commands and itself; the Web part last.
struct WebParts {
    calcu::shell::HostSettings settings;
    std::unique_ptr<cu59::TIAppPart> app;
    std::unique_ptr<cu59::TIWebPart> web;
    explicit WebParts(const calcu::settings::HostDirs& dirs) : settings(s59::settingsDecl(), dirs) {}
};

std::unique_ptr<calcu::sessions::AppSession> makeWebAppSession(const calcu::web::WebStart& start) {
    auto parts = std::make_shared<WebParts>(start.placesDirs());
    std::error_code ec;
    std::filesystem::create_directories(start.dirs.saveFolder, ec);

    cu59::TIAppSetup setup;
    // Without the full set the session
    // still starts, with no machine; the shell shows the topic
    // `roms-missing` in place of the faceplate.
    setup.roms = cu59::lookUpRoms(calcu::roms::DirectoryBundle(start.bundleDir));
    setup.settings = &parts->settings.file();   // [startup] model, last-model, [preset]
    // The save folder; no trace folder and no folder for screenshots: a
    // trace and a preset's screenshot are handed over with KeepFile, as a
    // download,
    // [trace] max-file-size-mb sets a trace's limit.
    setup.paths = &parts->settings.paths();
    setup.examplesFolder = start.presetsDir;   // the bundle
    setup.uploadFolder = start.uploadsDir;     // several files of the preset picker's upload
    parts->app = std::make_unique<cu59::TIAppPart>(std::move(setup));
    parts->web = std::make_unique<cu59::TIWebPart>();
    parts->web->bind(*parts->app);

    // The one order (parts::LibraryParts): the library's ControlPart (the
    // control state with the product entries c59.asm-run and
    // c59.calculator-tab), the shell's part and SettingsPart, the
    // library's and the product's, the Web part last.
    calcu::shell::HostSessionSetup host;
    host.settings = &parts->settings;
    host.before = {start.shellPart};
    host.after = {parts->web.get()};
    host.clock = &calcu::web::wallTimeNow;
    host.hostTime = [] { return std::chrono::system_clock::now(); };
    host.logSettingsProblem = true;
    cu59::TIAppPart& app = *parts->app;
    host.owned = std::move(parts);
    return calcu::shell::makeHostSession(app, std::move(host)).session;
}

} // namespace

CALCU_WEB_PRODUCT(CalcU59Host, makeWebAppSession)
