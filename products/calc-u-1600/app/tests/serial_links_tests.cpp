// LIB-2 AC-33 through Calc-U-1600's machine session (LIB-2 design §6 P10;
// M-26): the host links of the serial ports, which the session holds until
// M-31 ("Product-held host links"). The cases need pseudo-terminals (macOS,
// Linux: the library's CALCU_HAS_PTY). SKIP without ROMs.

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

#include "TestSettings.hpp"
#include "TestHarness.hpp"
#include "app/MachineSession.hpp"

#if (defined(__APPLE__) || defined(__unix__)) && !defined(__EMSCRIPTEN__)
#define SERIAL_TESTS_HAVE_PTY 1
#else
#define SERIAL_TESTS_HAVE_PTY 0
#endif

namespace {

namespace fs = std::filesystem;
using shared::MachineSession;
using shared::Model;

struct QuietListener : MachineSession::Listener {
    std::string missing;
    void romMissing(const calcu::roms::RomsMissing& m) override { missing = calcu::roms::describe(m); }
};

bool romsPresent() {
    return std::ifstream("roms/PC-1500_A04.ROM").good() && std::ifstream("roms/PC1600-P0-B0-new.bin").good() &&
           std::ifstream("roms/CE-158.ROM").good();
}

struct Fixture {
    TestSettings testSettings;
    shared::Settings& settings = testSettings.settings;
    fs::path home = fs::temp_directory_path() / "calcu-serial-links-test";
    fs::path links = home / "links";
    shared::AppPaths paths{settings, testHostDirs(home.generic_string()), shared::BundleDirs{"roms", {}}};
    QuietListener listener;
    std::unique_ptr<MachineSession> session;
    Fixture() {
        fs::remove_all(home);
        fs::create_directories(links);
        settings.setSerialLinkDirOverride(links.generic_string());
        session = std::make_unique<MachineSession>(settings, paths, listener);
    }
    ~Fixture() {
        session.reset();
        fs::remove_all(home);
    }
    void run(double seconds) {
        const auto frame = static_cast<std::uint64_t>(session->clockHz() / 60.0);
        for (int i = 0; i < static_cast<int>(seconds * 60); ++i) session->advance(frame);
    }
};

bool linkExists(const fs::path& p) {
    std::error_code ec;
    return fs::is_symlink(p, ec) || fs::exists(p, ec);
}

#if SERIAL_TESTS_HAVE_PTY

TEST(lib2_ac33_pc1600_rs232c_file) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    Fixture f;
    f.session->switchModel(Model::PC1600);
    CHECK(f.listener.missing.empty());
    // At once when a PC-1600 exists, before anything uses the port.
    CHECK(!f.session->serialLinkStatus().empty());
    CHECK(linkExists(f.links / "calcu1600-rs232c.serial"));
    // No CE-158, no file of its own.
    CHECK(f.session->ce158SerialLinkStatus().empty());
    CHECK(!linkExists(f.links / "calcu1600-ce158.serial"));
}

TEST(lib2_ac33_ce158_file_on_first_use_all_models) {
    SKIP_UNLESS(romsPresent(), "no ROMs in roms/");
    for (Model model : {Model::PC1500, Model::PC1500A, Model::PC1600}) {
        Fixture f;
        f.session->switchModel(model);
        CHECK(f.session->ce158SerialLinkStatus().empty());
        f.session->powerCycleAround([&] { CHECK(f.session->attachCE158()); });
        f.run(1.0);
        f.session->pumpSerialLinks();
        const bool made = !f.session->ce158SerialLinkStatus().empty();
        if (!made) std::fprintf(stderr, "  model %d: no CE-158 serial port file\n", static_cast<int>(model));
        CHECK(made);
        CHECK(linkExists(f.links / "calcu1600-ce158.serial"));
    }
}

#endif

} // namespace
