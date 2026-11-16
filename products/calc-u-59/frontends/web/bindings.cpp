// The Calc-U-59 Web engine (LIB-15 design §3.6, §4; M-34): the product's
// factory of the library's app session for the library's Web Worker host.
// All the rest is the library's (library/web/glue/: WorkerHost translates
// the session's messages to the page and back) and the app layer's
// (TIAppPart: the machine and the flows; TIWebPart: what the page draws).
// Built by frontends/web/build.sh (CMake target calcu59_web) into dist/wasm/.
//
// File system (the library's worker writes it before the session is made):
// /bundle the ROM full set and cuecards.txt (files.json; LIB-12: looked up
// once, here), /presets the bundled examples (examples/bundle.yaml and the
// files it lists; LIB-6 AC-31, AC-33: the bundle paths `examples:<path>`),
// /persist-calc-u-59 the persistent file system:
// settings/Calc-U-59/Calc-U-59.ini (LIB-9 AC-23) and the save folder
// Calc-U-59/ (cards, ti58c.memory.yaml; LIB-15 AC-20, C59-3 AC-13). No trace
// folder: a trace is kept in memory and handed over with KeepFile when it
// ends (LIB-10 AC-26, D-10; the MEMFS /traces of LIB-9 A-3 went in M-53).

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
// (AppSessionSetup::owned) and lets it go first. TIAppPart brings the
// library's parts (LIB-6 D-40: typing, peripherals, presets, startup, paper,
// debugger, in the library's order), the program memory commands and itself
// (sessionParts()); the shell's part goes first, the Web part last.
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

    cu59::TIAppSetup setup;
    setup.roms = roms.set;
    setup.cuecards = calcu::settings::readWholeFile(start.bundleDir + "/cuecards.txt").value_or("");
    setup.saveFolder = start.dirs.saveFolder;
    setup.settings = &parts->file;          // [startup] model, last-model, [preset] (LIB-18 AC-17)
    setup.examplesFolder = start.presetsDir;   // the bundle (LIB-15 AC-16)
    // No trace folder: a trace is handed over with KeepFile (LIB-10 AC-26);
    // [trace] max-file-size-mb sets its limit.
    setup.trace.maxBytes = static_cast<std::uint64_t>(parts->settings.traceMaxFileSizeMB()) * 1'000'000;
    parts->app = std::make_unique<cu59::TIAppPart>(std::move(setup));
    parts->web = std::make_unique<cu59::TIWebPart>(parts->settings);
    parts->web->bind(*parts->app);
    // LIB-13 AC-8: several files of the preset picker's upload become one folder here.
    parts->app->libraryParts().open().setup().uploadFolder = start.uploadsDir;

    calcu::sessions::AppSessionSetup s;
    s.clock = &calcu::web::wallTimeNow;
    s.hostTime = [] { return std::chrono::system_clock::now(); };
    // LIB-4's ControlPart (D-13): the control state with C59-4's product
    // entries (c59.asm-run, c59.trace, c59.calculator-tab; AC-25).
    s.parts = {start.shellPart, &parts->app->libraryParts().controlPart()};
    for (calcu::sessions::SessionPart* p : parts->app->sessionParts()) s.parts.push_back(p);
    s.parts.push_back(parts->web.get());
    const std::string problem = parts->file.problem();
    s.owned = std::move(parts);
    auto session = std::make_unique<calcu::sessions::AppSession>(std::move(s));
    if (!problem.empty()) session->context().log("Settings: " + problem);
    return session;
}

} // namespace

CALCU_WEB_PRODUCT(CalcU59Host, makeWebAppSession)
