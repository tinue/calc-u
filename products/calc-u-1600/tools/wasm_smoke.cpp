// WebAssembly smoke test for Core + Shared (tools/build_wasm_check.sh):
// built with Emscripten, run under node from the product folder (NODERAWFS,
// so roms/ and the bundled cards are read straight from disk). Starts the
// library's app session with Calc-U-1600's part in virtual-time hosting
// (LIB-3 design §7; the operations run in one go) and runs presets through
// its commands, then types through the input queue, and checks the LCD.
// Exit 0 = pass; 2 = no ROMs (skip).

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

#include "Machine/SharpRoms.hpp"
#include "app/SharpSession.hpp"
#include "sessions/Listener.hpp"
#include "app/tests/TestSettings.hpp"

namespace {

namespace fs = std::filesystem;
namespace sessions = calcu::sessions;

std::string screen(const sessions::Listener& host, const shared::SharpAppPart& part) {
    LcdText text;
    const calcu::machine::DisplayOutput* d = host.display();
    return d && part.lcdText(*d, &text) ? text.text() : std::string();
}

bool run(sessions::Listener& host, const sessions::CommandName& name, sessions::Value args = {}) {
    const auto r = host.send(name, std::move(args)).reply;   // virtual time: the operation ran to its end inside send()
    if (!r || !r->done) std::fprintf(stderr, "%s: %s\n", name.value.c_str(), r ? r->reason.c_str() : "no reply");
    return r && r->done;
}

} // namespace

int main() {
    if (!sharp::lookUpRoms(calcu::roms::DirectoryBundle("roms")).ok()) {
        std::printf("wasm_smoke: SKIP (not the full ROM set in roms/; run roms/fetch_roms.sh, run from the product folder)\n");
        return 2;
    }
    // A bundle folder like the app's: ROMs and the bundled cards/disks.
    const fs::path root = fs::temp_directory_path() / "calcu-wasm-smoke";
    fs::remove_all(root);
    fs::create_directories(root / "bundle");
    for (const auto& dir : {fs::path("roms"), fs::path("frontends/qt6/resources/cards")})
        for (const auto& f : fs::directory_iterator(dir))
            if (f.is_regular_file()) fs::copy_file(f.path(), root / "bundle" / f.path().filename());

    int failures = 0;
    {
        TestSettings testSettings;
        shared::Settings& settings = testSettings.settings;
        shared::AppPaths paths(settings, testHostDirs((root / "home").generic_string()),
                               shared::BundleDirs{(root / "bundle").generic_string(), {}});
        shared::SharpSessionOptions options;
        options.hosting = sessions::Hosting::VirtualTime;
        options.clock = [] { return calcu::pacing::WallTime{0}; };
        auto app = shared::makeSharpSession(settings, paths, std::move(options));
        shared::SharpAppPart& part = *app.part;
        sessions::Listener host;
        host.listen(*app.session, sessions::HostServices{{sessions::HostService::FolderPicker}});
        if (!run(host, shared::commands::kStart)) ++failures;

        struct Case {
            const char* preset;
            const char* expect;
        };
        for (const Case& c : {Case{"tests/fixtures/presets/startup-pc1600.pc1600", "Memory:"},
                              Case{"tests/fixtures/presets/hanoi.pc1600", ""},
                              Case{"tests/fixtures/presets/startup-pc1500a.pc1500a", ""}}) {
            const bool ok = run(host, shared::commands::kLoadPreset, sessions::Value::object({{"path", c.preset}}));
            const std::string lcd = screen(host, part);
            const bool seen = lcd.find(c.expect) != std::string::npos;
            std::printf("%s %s\n%s\n", ok && seen ? "ok  " : "FAIL", c.preset, lcd.c_str());
            if (!ok || !seen) ++failures;
        }
        // Typing through the library's typing engine (LIB-5), in emulated time.
        part.typingPart().engine().typeText("PRINT 6*7", true, false, calcu::typing::Origin::Typed);
        app.session->advanceUnpaced(static_cast<calcu::machine::Ticks>(4.0 * part.clockHz()));
        const bool typed = screen(host, part).find("42") != std::string::npos;
        std::printf("%s PRINT 6*7\n%s\n", typed ? "ok  " : "FAIL", screen(host, part).c_str());
        if (!typed) ++failures;
    }
    fs::remove_all(root);
    std::printf("wasm_smoke: %s\n", failures == 0 ? "all passed" : "FAILED");
    return failures == 0 ? 0 : 1;
}
