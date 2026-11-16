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
// the save folder Calc-U-59/ (cards, ti58c.mem; LIB-15 AC-20). No trace
// folder: a trace is kept in memory and handed over with KeepFile when it
// ends (LIB-10 AC-26, D-10; the MEMFS /traces of LIB-9 A-3 went in M-53).

#include <chrono>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>

#include "../../app/ProductRoms.hpp"
#include "../../app/TIDebuggerSetup.hpp"
#include "../../app/TIPrinters.hpp"
#include "../../core/TICatalogue.hpp"
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
// (AppSessionSetup::owned) and lets it go first. The library's typing part
// goes before the product's (LIB-5); the PaperPart (LIB-8) and the
// DebuggerPart (LIB-10) after it, as makeAppSession has them (M-53); the Web
// part last.
struct WebParts {
    calcu::settings::SettingsFile file;
    s59::Settings settings{file};
    calcu::typing::TypingPart typing{cu59::typingProduct()};
    cu59::TILevelRules rules;
    std::unique_ptr<cu59::TIAppPart> app;
    calcu::paper::PaperPart paper{tiCatalogue(), cu59::tiPrinterPapers()};
    std::unique_ptr<calcu::debugger::DebuggerPart> debugger;
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
    setup.startModel = parts->settings.startupModel();
    setup.saveFolder = start.dirs.saveFolder;
    WebParts* p = parts.get();
    cu59::TIDebuggerOptions debug;
    debug.traceLimitMb = [p] { return static_cast<long>(p->settings.traceMaxFileSizeMB()); };
    debug.handOver = true;   // no trace folder: KeepFile (LIB-10 AC-26)
    debug.showPanel = [p](calcu::sessions::SessionContext&, const std::string& view) { p->app->showDebugPanelNamed(view); };
    parts->debugger = std::make_unique<calcu::debugger::DebuggerPart>(cu59::tiDebuggerSetup(parts->rules, std::move(debug)));
    parts->app = std::make_unique<cu59::TIAppPart>(std::move(setup));
    parts->app->setTyping(&parts->typing);
    parts->app->setLibraryParts(&parts->paper, parts->debugger.get(), &parts->rules);
    parts->web = std::make_unique<cu59::TIWebPart>(parts->settings, start.presetsDir);
    parts->web->bind(*parts->app);

    calcu::sessions::AppSessionSetup s;
    s.clock = &calcu::web::wallTimeNow;
    s.hostTime = [] { return std::chrono::system_clock::now(); };
    s.parts = {start.shellPart, &parts->typing, parts->app.get(), &parts->paper, parts->debugger.get(), parts->web.get()};
    const std::string problem = parts->file.problem();
    s.owned = std::move(parts);
    auto session = std::make_unique<calcu::sessions::AppSession>(std::move(s));
    if (!problem.empty()) session->context().log("Settings: " + problem);
    return session;
}

} // namespace

CALCU_WEB_PRODUCT(CalcU59Host, makeWebAppSession)
