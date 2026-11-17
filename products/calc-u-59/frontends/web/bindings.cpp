// The Calc-U-59 Web engine (LIB-15 design §3.6, §4; M-34): the product's
// factory of the library's app session for the library's Web Worker host.
// All the rest is the library's (library/web/glue/: WorkerHost translates
// the session's messages to the page and back; shell/HostSession: the
// settings file, LIB-14's SettingsPart, the order) and the app layer's
// (TIAppPart: the machine and the flows; TIWebPart: what the page draws).
// Built by frontends/web/build.sh (CMake target calcu59_web) into dist/wasm/.
//
// File system (the library's worker writes it before the session is made):
// /bundle the ROM full set (files.json; LIB-12: looked up once, here; the
// cue cards are the module files', C59-7 AC-23), /presets the bundled examples (examples/bundle.yaml and the
// files it lists; LIB-6 AC-31, AC-33: the bundle paths `examples:<path>`),
// /persist-calc-u-59 the persistent file system:
// settings/Calc-U-59/Calc-U-59.ini (LIB-9 AC-23) and the save folder
// Calc-U-59/ (the magnetic cards of the library's media part, LIB-7 59;
// ti58c.memory.yaml; LIB-15 AC-20, C59-3 AC-13). No trace
// folder: a trace is kept in memory and handed over with KeepFile when it
// ends (LIB-10 AC-26, D-10; the MEMFS /traces of LIB-9 A-3 went in M-53).

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
// shell's part and LIB-14's SettingsPart (the `settings` topic and
// `set-setting` of the shell's dialog, M-57), the library's parts (LIB-6
// D-40), the program memory commands and itself; the Web part last.
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
    // LIB-12 AC-5, LIB-14 AC-19 (M-57): without the full set the session
    // still starts, with no machine; the shell shows the topic
    // `roms-missing` in place of the faceplate.
    setup.roms = cu59::lookUpRoms(calcu::roms::DirectoryBundle(start.bundleDir));
    setup.settings = &parts->settings.file();   // [startup] model, last-model, [preset] (LIB-18 AC-17)
    // The save folder; no trace folder: a trace is handed over with KeepFile
    // (LIB-10 AC-26), [trace] max-file-size-mb sets its limit.
    setup.paths = &parts->settings.paths();
    setup.screenshotFolder = start.dirs.saveFolder;   // a preset's screenshot: written there
    setup.examplesFolder = start.presetsDir;   // the bundle (LIB-15 AC-16)
    setup.uploadFolder = start.uploadsDir;     // LIB-13 AC-8: several files of the preset picker's upload
    parts->app = std::make_unique<cu59::TIAppPart>(std::move(setup));
    parts->web = std::make_unique<cu59::TIWebPart>();
    parts->web->bind(*parts->app);

    // The one order (parts::LibraryParts): LIB-4's ControlPart (D-13: the
    // control state with C59-4's product entries c59.asm-run, c59.trace,
    // c59.calculator-tab; AC-25), the shell's part and SettingsPart, the
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
