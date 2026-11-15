// The exit check of M-28 (LIB-11 design §6, T14; D-1): `calcu` drives the
// Sharp machine of the default start, the PC-1500A, headless through the
// library's app session (the PC-1600 follows with LIB-18's model switch;
// the product ACs of LIB-11, AC-18 to AC-20, are tested in M-51).
#include "../app/HarnessProduct.hpp"
#include "TestHarness.hpp"
#include "harness/Run.hpp"
#include <filesystem>
#include <string>

using namespace calcu::harness;

TEST(harness_drives_a_sharp_machine) {
    SKIP_UNLESS(std::filesystem::exists("roms/PC-1500_A04.ROM"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    auto product = shared::makeHarnessProduct();
    std::string error;
    auto server = Server::start(*product, {}, &error);
    CHECK(server != nullptr);
    if (!server) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return;
    }
    auto reply = [&](const std::string& l) { return server->line(l).text; };
    CHECK(reply("wait 2s").rfind("done 2\nticks: ", 0) == 0);
    const std::string boot = reply("lcd");
    CHECK(boot.find("NEW0?") != std::string::npos);   // a fresh RAM asks first
    CHECK(boot.find("\nnot text: 0\n") != std::string::npos);
    for (const char* k : {"cl", "n", "e", "w", "0", "enter"}) {
        CHECK_EQ(reply(std::string("key-down ") + k), std::string("done\n"));
        reply("wait 70ms");
        reply(std::string("key-up ") + k);
        reply("wait 70ms");
    }
    reply("wait 1s");
    const std::string after = reply("lcd");
    CHECK(after.find("NEW0?") == std::string::npos);
    CHECK(after.find("indicators:") != std::string::npos);
    CHECK(reply("ticks").rfind("done 2\nticks: ", 0) == 0);
    CHECK(reply("display").rfind("done 10\n", 0) == 0);   // 7 rows of dots, indicators, names, on
    server->finish();
}
