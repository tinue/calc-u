// The exit check of M-28 (LIB-11 design §6, T14; D-1): `calcu` drives the
// Sharp machine of the default start, the PC-1500A, headless through the
// library's app session (the PC-1600 follows with LIB-18's model switch;
// the product ACs of LIB-11, AC-18 to AC-20, are tested in M-51).
#include "../app/HarnessProduct.hpp"
#include "TestHarness.hpp"
#include "harness/Run.hpp"
#include <filesystem>
#include <fstream>
#include <memory>
#include "harness/Commands.hpp"
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

// LIB-18 design P16 (the PC-1600 of the M-28 exit check, LIB-11 D-1): the
// harness switches to the PC-1600 with LIB-18's `model`, and reads its
// display with the PC-1600's font.
TEST(harness_drives_a_pc1600) {
    SKIP_UNLESS(std::filesystem::exists("roms/PC1600-P0-B0-new.bin"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    auto product = shared::makeHarnessProduct();
    std::string error;
    auto server = Server::start(*product, {}, &error);
    CHECK(server != nullptr);
    if (!server) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return;
    }
    auto reply = [&](const std::string& l) { return server->line(l).text; };
    CHECK_EQ(reply("model PC-1600"), std::string("done\n"));
    const std::string lcd = reply("lcd");
    CHECK(lcd.rfind("done ", 0) == 0);
    CHECK(lcd.find(">") != std::string::npos);   // the PC-1600's prompt, read with its font
    CHECK(lcd.find("\nnot text: 0\n") != std::string::npos);
    CHECK(reply("capabilities").find("place host-drive empty\n") != std::string::npos);   // a PC-1600 place
    const auto* m = server->host().session().context().machine();
    CHECK(m && m->model().value == "PC-1600");
    server->finish();
}

TEST(harness_variant_switch_old) {
    SKIP_UNLESS(std::filesystem::exists("roms/PC1600-P0-B0-old.bin"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    auto product = shared::makeHarnessProduct();
    std::string error;
    auto server = Server::start(*product, {}, &error);
    CHECK(server != nullptr);
    if (!server) return;
    auto reply = [&](const std::string& l) { return server->line(l).text; };
    CHECK_EQ(reply("model PC-1600"), std::string("done\n"));
    CHECK_EQ(reply("variant old"), std::string("done\n"));   // any case (LIB-18 D-16)
    const auto* m = server->host().session().context().machine();
    CHECK(m && m->model().value == "PC-1600" && m->variant() && m->variant()->value == "Old");
    CHECK(reply("lcd").find(">") != std::string::npos);
    CHECK_EQ(reply("variant Old"), std::string("done\n"));   // the running variant: nothing to do
    server->finish();
}

// ---- C1600-2 AC-21 (design §3.7, T9): the loaders under `pc1600` ----

namespace {

std::unique_ptr<Server> loaderServer(std::unique_ptr<HarnessProduct>& product) {
    product = shared::makeHarnessProduct();
    std::string error;
    return Server::start(*product, {}, &error);
}

std::string scratch(const char* name, const std::string& bytes) {
    const std::filesystem::path p = std::filesystem::temp_directory_path() / name;
    std::ofstream out(p, std::ios::binary | std::ios::trunc);
    out << bytes;
    return p.generic_string();
}

} // namespace

TEST(c1600_2_ac21_listed_under_pc1600) {
    CommandRegistry r;
    auto product = shared::makeHarnessProduct();
    product->addCommands(r);
    for (const char* name : {"file-kind", "load-basic", "type-basic", "load-code"}) CHECK(r.find({"pc1600", name}) != nullptr);
    bool usage = false;
    for (const std::string& u : r.usage())
        if (u.rfind("pc1600 load-code", 0) == 0) usage = true;
    CHECK(usage);   // --help lists them
}

TEST(c1600_2_ac21_file_kind) {
    SKIP_UNLESS(std::filesystem::exists("roms/PC-1500_A04.ROM"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    std::unique_ptr<HarnessProduct> product;
    auto server = loaderServer(product);
    CHECK(server != nullptr);
    if (!server) return;
    const std::string bbin = server->line("pc1600 file-kind tests/fixtures/basic/lissajou-1500.bbin").text;
    CHECK(bbin.find("kind: basic-pc1500\n") != std::string::npos && bbin.find("name: LISSAJOU-1500\n") != std::string::npos);
    const std::string wav = server->line("pc1600 file-kind tests/fixtures/tape/pc1500_tape.wav").text;
    CHECK(wav.find("file 1: TAPEFIX") != std::string::npos);
    std::string ce158("\x01\x42" "COM", 5);
    ce158 += std::string(16, '\0');
    ce158 += std::string("\x7C\x01\x00\x01\x7C\x01\x9A\x9A", 8);
    const std::string code = scratch("c1600_2_code.bin", ce158);
    const std::string k = server->line("pc1600 file-kind " + code).text;
    CHECK(k.find("kind: code-lh5801\n") != std::string::npos && k.find("load: &7C01\n") != std::string::npos &&
          k.find("start: &7C01\n") != std::string::npos);
    CHECK(server->line("pc1600 file-kind /no/such/file").text.rfind("refused: ", 0) == 0);
    server->finish();
}

TEST(c1600_2_ac21_load_basic_with_n) {
    SKIP_UNLESS(std::filesystem::exists("roms/PC-1500_A04.ROM"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    std::unique_ptr<HarnessProduct> product;
    auto server = loaderServer(product);
    if (!server) return;
    server->line("key cl");
    server->line("type NEW0");
    server->line("wait 1s");
    CHECK_EQ(server->line("pc1600 load-basic tests/fixtures/tape/pc1500_tape.wav 1").text, std::string("done\n"));
    CHECK_EQ(server->line("pc1600 load-basic tests/fixtures/basic/lissajou-1500.bas").text, std::string("done\n"));
    CHECK(server->line("pc1600 load-basic tests/fixtures/tape/pc1500_tape.wav 2").text.rfind("refused: ", 0) == 0);
    server->finish();
}

TEST(c1600_2_ac21_type_basic_lines_not_stored) {
    SKIP_UNLESS(std::filesystem::exists("roms/PC-1500_A04.ROM"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    std::unique_ptr<HarnessProduct> product;
    auto server = loaderServer(product);
    if (!server) return;
    server->line("key cl");
    server->line("type NEW0");
    server->line("wait 1s");
    const std::string ok = scratch("c1600_2_ok.bas", "10 A=1\n20 END\n");
    CHECK_EQ(server->line("pc1600 type-basic " + ok).text, std::string("done\n"));
    const std::string bad = scratch("c1600_2_bad.bas", "30 B=2\nB=3\n");
    const std::string r = server->line("pc1600 type-basic " + bad).text;
    CHECK(r.rfind("refused: ", 0) == 0 && r.find("1 program line(s) not stored") != std::string::npos &&
          r.find("\"B=3\"") != std::string::npos);
    server->finish();
}

TEST(c1600_2_ac21_load_code_options_and_run) {
    SKIP_UNLESS(std::filesystem::exists("roms/PC1600-P0-B0-new.bin"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    std::unique_ptr<HarnessProduct> product;
    auto server = loaderServer(product);
    if (!server) return;
    CHECK_EQ(server->line("model PC-1600").text, std::string("done\n"));
    const std::string raw = scratch("c1600_2_raw.bin", std::string("\xC9\xC9\xC9\xC9", 4));
    CHECK(server->line("pc1600 load-code " + raw).text.find("needs an address") != std::string::npos);
    const std::string r = server->line("pc1600 load-code " + raw + " address=C100 length=2").text;
    CHECK(r.find("range: &C100-&C101\n") != std::string::npos && r.find("area: S0\n") != std::string::npos &&
          r.find("bank: 0\n") != std::string::npos && r.find("new: NEW \"S0:\",&102\n") != std::string::npos &&
          r.find("start: CALL &C100\n") != std::string::npos && r.find("started: no\n") != std::string::npos);
    CHECK(server->line("pc1600 load-code " + raw + " address=&4000 cpu=lh5803").text.find("refused: ") == std::string::npos);
    CHECK(server->line("pc1600 load-code " + raw + " address=&F800").text.find("warning: ") != std::string::npos);
    CHECK(server->line("pc1600 load-code " + raw + " cpu=6502").text.rfind("refused: ", 0) == 0);
    // `run` with a header that names an address to start at.
    const std::string hdr = scratch("c1600_2_hdr.bin",
                                    std::string("\xFF\x10\x00\x00\x10\x01\x00\x00\x00\xC1\x00\x00\xC1\x00\x00\x0F\xC9", 17));
    const std::string run = server->line("pc1600 load-code " + hdr + " run").text;
    CHECK(run.find("started: yes\n") != std::string::npos);
    CHECK(server->line("lcd").text.find("CALL &C100") != std::string::npos);
    server->finish();
}
