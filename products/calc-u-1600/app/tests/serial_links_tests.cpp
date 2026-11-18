// The host links of the serial ports through the library's app session,
// held by the library's peripherals part. The cases need pseudo-terminals (macOS,
// Linux: the library's CALCU_HAS_PTY). SKIP without ROMs.

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

#include "TestSettings.hpp"
#include "TestHarness.hpp"
#include "SessionFixture.hpp"

#if (defined(__APPLE__) || defined(__unix__)) && !defined(__EMSCRIPTEN__)
#define SERIAL_TESTS_HAVE_PTY 1
#else
#define SERIAL_TESTS_HAVE_PTY 0
#endif

namespace {

namespace fs = std::filesystem;
using Model = calcu::machine::ModelId;

struct Fixture : sessionfixture::SessionFixture {
    fs::path links = root / "links";
    Fixture() : SessionFixture("calcu-serial-links-test") {
        fs::create_directories(links);
        settings.file().set("serial", "link-folder", links.generic_string());
    }
};

bool linkExists(const fs::path& p) {
    std::error_code ec;
    return fs::is_symlink(p, ec) || fs::exists(p, ec);
}

#if SERIAL_TESTS_HAVE_PTY

TEST(pc1600_rs232c_file) {
    SKIP_UNLESS(fullRomSetPresent(), "no ROMs in roms/");
    Fixture f;
    CHECK(f.start("PC-1600"));
    // At once when a PC-1600 exists, before anything uses the port.
    CHECK(!f.part().serialLinkStatus().empty());
    CHECK(linkExists(f.links / "calcu1600-rs232c.serial"));
    // No CE-158, no file of its own.
    CHECK(f.part().ce158SerialLinkStatus().empty());
    CHECK(!linkExists(f.links / "calcu1600-ce158.serial"));
}

TEST(ce158_file_on_first_use_all_models) {
    SKIP_UNLESS(fullRomSetPresent(), "no ROMs in roms/");
    for (Model model : {calcu::machine::ModelId{"PC-1500"}, calcu::machine::ModelId{"PC-1500A"}, calcu::machine::ModelId{"PC-1600"}}) {
        Fixture f;
        CHECK(f.start(model.value.c_str()));
        CHECK(f.part().ce158SerialLinkStatus().empty());
        CHECK(f.toggle("CE-158"));
        f.runFor(1.0);
        const bool made = !f.part().ce158SerialLinkStatus().empty();
        if (!made) std::fprintf(stderr, "  model %s: no CE-158 serial port file\n", model.value.c_str());
        CHECK(made);
        CHECK(linkExists(f.links / "calcu1600-ce158.serial"));
    }
}

#endif

} // namespace
