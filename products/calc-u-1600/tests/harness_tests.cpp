// `calcu` drives the Sharp machine of the default start, the PC-1500A,
// headless through the library's app session (the PC-1600 follows with the
// model switch; the product commands are tested in tests/harness/).
#include "../app/HarnessProduct.hpp"
#include "../app/Settings.hpp"
#include "parts/LibraryParts.hpp"
#include "TestHarness.hpp"
#include "harness/Run.hpp"
#include "TempFolder.hpp"
#include <cstdio>
#include <memory>
#include <filesystem>
#include <fstream>
#include <memory>
#include "harness/Commands.hpp"
#include <string>

using namespace calcu::harness;

// A bundled preset, the harness `wait`
// without a duration, `expect` and `screenshot` on a PC-1600.
TEST(preset_wait_expect_screenshot_on_pc1600) {
    SKIP_UNLESS(std::filesystem::exists("roms/PC1600-P0-B0-new.bin"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    calcu::testing::TempFolder out("calcu1600-preset-wait");
    auto product = cu1600::makeHarnessProduct();
    RunOptions o;
    o.outputFolder = out.str();
    std::string error;
    auto server = Server::start(*product, o, &error);
    CHECK(server != nullptr);
    if (!server) return;
    auto reply = [&](const std::string& l) { return server->line(l).text; };
    CHECK(reply("preset examples:basic/hanoi.pc1600").rfind("done", 0) == 0);
    // `wait` returns once the program waits for the user: past the title,
    // shown during WAIT 100 delays, at the first question.
    CHECK(reply("wait").rfind("done", 0) == 0);
    CHECK(reply("expect Tower Height").rfind("done", 0) == 0);
    CHECK(reply("expect Not on the screen").rfind("refused: ", 0) == 0);
    CHECK(reply("screenshot hanoi.png").rfind("done", 0) == 0);
    CHECK(std::filesystem::exists(out.path() / "hanoi.png"));
    server->finish();
}

// The harness entry builds with SharpProductMachines (the built-in default
// of each model).
TEST(harness_starts_with_the_product_machines) {
    std::string error;
    auto run = RunSetup::make({}, cu1600::settingsDecl(), &error);
    CHECK(run != nullptr);
    if (!run) return;
    auto product = cu1600::makeHarnessProduct();
    product->prepare(*run);   // the app layer of a run: its machines
    calcu::startup::ProductMachines& m = product->libraryParts().control().product();
    for (const char* model : {"PC-1500", "PC-1500A", "PC-1600"}) {
        const calcu::startup::MachinePlan plan = m.builtInDefault(calcu::machine::ModelId{model});
        CHECK(plan.model.value == model && !plan.variant && plan.peripherals.empty());
        CHECK(m.readinessBound(calcu::machine::ModelId{model}).count() == (std::string(model) == "PC-1600" ? 27.0 : 7.0));
    }
}

TEST(harness_drives_a_sharp_machine) {
    SKIP_UNLESS(std::filesystem::exists("roms/PC-1500_A04.ROM"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    auto product = cu1600::makeHarnessProduct();
    std::string error;
    auto server = Server::start(*product, {}, &error);
    CHECK(server != nullptr);
    if (!server) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return;
    }
    auto reply = [&](const std::string& l) { return server->line(l).text; };
    CHECK(reply("wait 2s").rfind("done 2\nticks: ", 0) == 0);
    const std::string boot = reply("display-text");
    CHECK(boot.find("NEW0?") != std::string::npos);   // a fresh RAM asks first
    CHECK(boot.find("\nnot text: 0\n") != std::string::npos);
    for (const char* k : {"cl", "n", "e", "w", "0", "enter"}) {
        CHECK_EQ(reply(std::string("key-down ") + k), std::string("done\n"));
        reply("wait 70ms");
        reply(std::string("key-up ") + k);
        reply("wait 70ms");
    }
    reply("wait 1s");
    const std::string after = reply("display-text");
    CHECK(after.find("NEW0?") == std::string::npos);
    CHECK(after.find("indicators:") != std::string::npos);
    CHECK(reply("ticks").rfind("done 2\nticks: ", 0) == 0);
    CHECK(reply("display").rfind("done 10\n", 0) == 0);   // 7 rows of dots, indicators, names, on
    server->finish();
}

// The harness switches to the PC-1600 with `model`, and reads its
// display with the PC-1600's font.
TEST(harness_drives_a_pc1600) {
    SKIP_UNLESS(std::filesystem::exists("roms/PC1600-P0-B0-new.bin"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    auto product = cu1600::makeHarnessProduct();
    std::string error;
    auto server = Server::start(*product, {}, &error);
    CHECK(server != nullptr);
    if (!server) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return;
    }
    auto reply = [&](const std::string& l) { return server->line(l).text; };
    CHECK_EQ(reply("model PC-1600"), std::string("done\n"));
    const std::string lcd = reply("display-text");
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
    auto product = cu1600::makeHarnessProduct();
    std::string error;
    auto server = Server::start(*product, {}, &error);
    CHECK(server != nullptr);
    if (!server) return;
    auto reply = [&](const std::string& l) { return server->line(l).text; };
    CHECK_EQ(reply("model PC-1600"), std::string("done\n"));
    CHECK_EQ(reply("variant old"), std::string("done\n"));   // any case
    const auto* m = server->host().session().context().machine();
    CHECK(m && m->model().value == "PC-1600" && m->variant() && m->variant()->value == "Old");
    CHECK(reply("display-text").find(">") != std::string::npos);
    CHECK_EQ(reply("variant Old"), std::string("done\n"));   // the running variant: nothing to do
    server->finish();
}

// The harness's paper
// commands on the CE-150 of a PC-1500A.
namespace {
struct PaperHarness {
    std::unique_ptr<HarnessProduct> product = cu1600::makeHarnessProduct();
    std::unique_ptr<Server> server;
    PaperHarness() {
        std::string error;
        server = Server::start(*product, {}, &error);
        if (!server) std::fprintf(stderr, "%s\n", error.c_str());
    }
    std::string reply(const std::string& l) { return server->line(l).text; }
    // Clears the fresh RAM's NEW0? question, plugs the CE-150 in and prints.
    void printOnCe150(const std::string& line) {
        reply("key cl");
        reply("type NEW0");
        reply("wait 1s");
        reply("attach printer CE-150");
        reply("type " + line);
        reply("wait 3s");
    }
};
} // namespace

TEST(harness_paper_marks_of_the_ce150) {
    SKIP_UNLESS(std::filesystem::exists("roms/CE-150.ROM"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    PaperHarness h;
    CHECK(h.server != nullptr);
    if (!h.server) return;
    h.printOnCe150("LPRINT 123");
    const std::string marks = h.reply("paper marks CE-150");
    CHECK(marks.rfind("done ", 0) == 0);
    CHECK(marks.find("\nreach ") != std::string::npos);
    CHECK(marks.find("\nstroke ") != std::string::npos);
    CHECK_EQ(h.reply("paper text CE-150"), std::string("refused: The CE-150 has no text layer.\n"));
    h.server->finish();
}

TEST(harness_cut_paper_and_png) {
    SKIP_UNLESS(std::filesystem::exists("roms/CE-150.ROM"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    calcu::testing::TempFolder t("calcu-1600-paper-png");
    PaperHarness h;
    CHECK(h.server != nullptr);
    if (!h.server) return;
    h.printOnCe150("LPRINT 1");
    const std::string png = (t.path() / "ce150.png").string();
    CHECK_EQ(h.reply("paper png " + png), std::string("done\n"));
    CHECK(std::filesystem::exists(png) && std::filesystem::file_size(png) > 100);
    CHECK_EQ(h.reply("cut-paper"), std::string("done\n"));
    CHECK_EQ(h.reply("paper png " + png), std::string("refused: The paper is empty.\n"));
    h.server->finish();
}

// The harness's run
// control and memory commands on the Sharp targets.
TEST(harness_pause_step_resume) {
    SKIP_UNLESS(std::filesystem::exists("roms/PC1600-P0-B0-new.bin"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    PaperHarness h;
    CHECK(h.server != nullptr);
    if (!h.server) return;
    CHECK_EQ(h.reply("model PC-1600"), std::string("done\n"));
    CHECK(h.reply("pause").find("paused, owner cpu, reason pause, target SC7852") != std::string::npos);
    const std::string t0 = h.reply("ticks");
    h.reply("wait 100ms");
    CHECK_EQ(h.reply("ticks"), t0);   // paused: no emulated time passes
    CHECK(h.reply("step { level: cpu, count: 3 }").find("reason step") != std::string::npos);
    CHECK(h.reply("step-over").find("reason step") != std::string::npos);
    CHECK(h.reply("resume").find("running") != std::string::npos);
    h.reply("wait 100ms");
    CHECK(h.reply("ticks") != t0);
    h.server->finish();
}

TEST(harness_breakpoint_and_watch) {
    SKIP_UNLESS(std::filesystem::exists("roms/PC-1500_A04.ROM"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    PaperHarness h;
    CHECK(h.server != nullptr);
    if (!h.server) return;
    h.reply("wait 1s");
    // The PC-1500A's ROM goes through its keyboard scan all the time.
    const std::string hist = h.reply("pause");
    CHECK(hist.find("paused") != std::string::npos);
    const std::string regs = h.reply("registers");
    const std::size_t at = regs.find("p = 0x");
    CHECK(at != std::string::npos);
    const std::string pc = regs.substr(at + 4, 6);
    CHECK(h.reply("break LH5801 " + pc).rfind("done", 0) == 0);
    CHECK(h.reply("resume").find("running") != std::string::npos);
    h.reply("wait 1s");
    CHECK(h.reply("state run-state").find("breakpoint") != std::string::npos);
    CHECK(h.reply("break off").rfind("done", 0) == 0);
    // A write watch on the RAM the ROM keeps writing while idle.
    CHECK(h.reply("watch LH5801 me0 0x7600 0x600 write").rfind("done", 0) == 0);
    CHECK(h.reply("resume").find("running") != std::string::npos);
    h.reply("wait 1s");
    CHECK(h.reply("state run-state").find("watch") != std::string::npos);
    CHECK(h.reply("watch off").rfind("done", 0) == 0);
    h.server->finish();
}

TEST(harness_peek_bank_and_vbank) {
    SKIP_UNLESS(std::filesystem::exists("roms/PC1600-P0-B0-new.bin"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    PaperHarness h;
    CHECK(h.server != nullptr);
    if (!h.server) return;
    CHECK_EQ(h.reply("model PC-1600"), std::string("done\n"));
    CHECK(h.reply("peek SC7852 memory 0x8000 2 bank=6").rfind("done 2\n0x8000: 0x", 0) == 0);
    CHECK(h.reply("peek SC7852 io-written 0x31").rfind("done 1\n0x31: 0x", 0) == 0);
    CHECK(h.reply("peek SC7852 io 0x31").rfind("done 1\n0x31: 0x", 0) == 0);
    CHECK(h.reply("peek LH5803 me1-written 0x0000").find("none") != std::string::npos);
    CHECK(h.reply("peek SC7852 memory 0x8000 1 bank=2 vbank=1").rfind("done 1", 0) == 0);
    h.server->finish();
}

// A poke on io is the SC7852's OUT: OUT (58H),3EH switches IC2's display
// off, and `display` shows it at once, without a wait.
TEST(harness_poke_io_is_an_out) {
    SKIP_UNLESS(std::filesystem::exists("roms/PC1600-P0-B0-new.bin"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    PaperHarness h;
    CHECK(h.server != nullptr);
    if (!h.server) return;
    CHECK_EQ(h.reply("model PC-1600"), std::string("done\n"));
    h.reply("wait");
    CHECK_EQ(h.reply("poke SC7852 io 0x58 0x3E"), std::string("done\n"));
    const std::string d = h.reply("display");
    CHECK(d.find("\non: true\nparts: IC2 off, IC3 on\n") != std::string::npos);
    CHECK_EQ(h.reply("peek SC7852 io-written 0x58"), std::string("done 1\n0x58: 0x3e\n"));
    CHECK(h.reply("poke SC7852 io-written 0x58 0x3F").rfind("refused: ", 0) == 0);
    h.server->finish();
}

// ---- The loaders under `c1600` ----

namespace {

std::unique_ptr<Server> loaderServer(std::unique_ptr<HarnessProduct>& product) {
    product = cu1600::makeHarnessProduct();
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

TEST(listed_under_c1600) {
    CommandRegistry r;
    auto product = cu1600::makeHarnessProduct();
    product->addCommands(r);
    for (const char* name : {"file-kind", "load-basic", "type-basic", "load-code"}) CHECK(r.find({"c1600", name}) != nullptr);
    bool usage = false;
    for (const std::string& u : r.usage())
        if (u.rfind("c1600 load-code", 0) == 0) usage = true;
    CHECK(usage);   // --help lists them
}

TEST(loader_server_file_kind) {
    SKIP_UNLESS(std::filesystem::exists("roms/PC-1500_A04.ROM"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    std::unique_ptr<HarnessProduct> product;
    auto server = loaderServer(product);
    CHECK(server != nullptr);
    if (!server) return;
    const std::string bbin = server->line("c1600 file-kind tests/fixtures/basic/lissajou-1500.bbin").text;
    CHECK(bbin.find("kind: basic-pc1500\n") != std::string::npos && bbin.find("name: LISSAJOU-1500\n") != std::string::npos);
    const std::string wav = server->line("c1600 file-kind tests/fixtures/tape/pc1500_tape.wav").text;
    CHECK(wav.find("file 1: TAPEFIX") != std::string::npos);
    std::string ce158("\x01\x42" "COM", 5);
    ce158 += std::string(16, '\0');
    ce158 += std::string("\x7C\x01\x00\x01\x7C\x01\x9A\x9A", 8);
    const std::string code = scratch("loader_code.bin", ce158);
    const std::string k = server->line("c1600 file-kind " + code).text;
    CHECK(k.find("kind: code-lh5801\n") != std::string::npos && k.find("load: &7C01\n") != std::string::npos &&
          k.find("start: &7C01\n") != std::string::npos);
    CHECK(server->line("c1600 file-kind /no/such/file").text.rfind("refused: ", 0) == 0);
    server->finish();
}

TEST(load_basic_with_n) {
    SKIP_UNLESS(std::filesystem::exists("roms/PC-1500_A04.ROM"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    std::unique_ptr<HarnessProduct> product;
    auto server = loaderServer(product);
    if (!server) return;
    server->line("key cl");
    server->line("type NEW0");
    server->line("wait 1s");
    CHECK_EQ(server->line("c1600 load-basic tests/fixtures/tape/pc1500_tape.wav 1").text, std::string("done\n"));
    CHECK_EQ(server->line("c1600 load-basic tests/fixtures/basic/lissajou-1500.bas").text, std::string("done\n"));
    CHECK(server->line("c1600 load-basic tests/fixtures/tape/pc1500_tape.wav 2").text.rfind("refused: ", 0) == 0);
    server->finish();
}

TEST(type_basic_lines_not_stored) {
    SKIP_UNLESS(std::filesystem::exists("roms/PC-1500_A04.ROM"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    std::unique_ptr<HarnessProduct> product;
    auto server = loaderServer(product);
    if (!server) return;
    server->line("key cl");
    server->line("type NEW0");
    server->line("wait 1s");
    const std::string ok = scratch("loader_ok.bas", "10 A=1\n20 END\n");
    CHECK_EQ(server->line("c1600 type-basic " + ok).text, std::string("done\n"));
    const std::string bad = scratch("loader_bad.bas", "30 B=2\nB=3\n");
    const std::string r = server->line("c1600 type-basic " + bad).text;
    CHECK(r.rfind("refused: ", 0) == 0 && r.find("1 program line(s) not stored") != std::string::npos &&
          r.find("\"B=3\"") != std::string::npos);
    server->finish();
}

TEST(load_code_options_and_run) {
    SKIP_UNLESS(std::filesystem::exists("roms/PC1600-P0-B0-new.bin"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    std::unique_ptr<HarnessProduct> product;
    auto server = loaderServer(product);
    if (!server) return;
    CHECK_EQ(server->line("model PC-1600").text, std::string("done\n"));
    const std::string raw = scratch("loader_raw.bin", std::string("\xC9\xC9\xC9\xC9", 4));
    CHECK(server->line("c1600 load-code " + raw).text.find("needs an address") != std::string::npos);
    const std::string r = server->line("c1600 load-code " + raw + " address=C100 length=2").text;
    CHECK(r.find("range: &C100-&C101\n") != std::string::npos && r.find("area: S0\n") != std::string::npos &&
          r.find("bank: 0\n") != std::string::npos && r.find("new: NEW \"S0:\",&102\n") != std::string::npos &&
          r.find("start: CALL &C100\n") != std::string::npos && r.find("started: no\n") != std::string::npos);
    CHECK(server->line("c1600 load-code " + raw + " address=&4000 cpu=lh5803").text.find("refused: ") == std::string::npos);
    CHECK(server->line("c1600 load-code " + raw + " address=&F800").text.find("warning: ") != std::string::npos);
    CHECK(server->line("c1600 load-code " + raw + " cpu=6502").text.rfind("refused: ", 0) == 0);
    // `run` with a header that names an address to start at.
    const std::string hdr = scratch("loader_hdr.bin",
                                    std::string("\xFF\x10\x00\x00\x10\x01\x00\x00\x00\xC1\x00\x00\xC1\x00\x00\x0F\xC9", 17));
    const std::string run = server->line("c1600 load-code " + hdr + " run").text;
    CHECK(run.find("started: yes\n") != std::string::npos);
    CHECK(server->line("display-text").text.find("CALL &C100") != std::string::npos);
    server->finish();
}

// ---- `open` in the harness loads at once ----

TEST(harness_open_loads_first_wav_file) {
    SKIP_UNLESS(std::filesystem::exists("roms/PC-1500_A04.ROM"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    std::unique_ptr<HarnessProduct> product;
    auto server = loaderServer(product);
    if (!server) return;
    server->line("key cl");
    server->line("type NEW0");
    server->line("wait 1s");
    const std::string r = server->line("open tests/fixtures/tape/pc1500_tape.wav").text;
    CHECK_EQ(r, std::string("done 1\nfile-kind pc1500_tape.wav basic-program\n"));
    server->finish();
}

TEST(harness_open_refuses_headerless) {
    SKIP_UNLESS(std::filesystem::exists("roms/PC-1500_A04.ROM"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    std::unique_ptr<HarnessProduct> product;
    auto server = loaderServer(product);
    if (!server) return;
    static const char kBlock[] = "\x21\x00\x80\x06\x10\x7E\xFE\x20\x28\x01\x23\x10\xF8\xCD\x00\x10\xC9";
    std::string code;
    for (int i = 0; i < 8; i++) code += std::string(kBlock, 17);
    const std::string bin = scratch("open_headerless.bin", code);
    const std::string r = server->line("open " + bin).text;
    CHECK(r.rfind("refused: open_headerless.bin: ", 0) == 0 && r.find("needs an address") != std::string::npos);
    // `.rom` is no machine-code name.
    const std::string rom = scratch("open_headerless.rom", code);
    CHECK_EQ(server->line("open " + rom).text, std::string("refused: Calc-U-1600 cannot open open_headerless.rom.\n"));
    server->finish();
}

TEST(harness_open_preset_bas_and_headered_bin) {
    SKIP_UNLESS(std::filesystem::exists("roms/PC-1500_A04.ROM"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    std::unique_ptr<HarnessProduct> product;
    auto server = loaderServer(product);
    if (!server) return;
    server->line("key cl");
    server->line("type NEW0");
    server->line("wait 1s");
    CHECK_EQ(server->line("open tests/fixtures/basic/lissajou-1500.bas").text,
             std::string("done 1\nfile-kind lissajou-1500.bas basic-program\n"));
    std::string ce158("\x01\x42" "COM", 5);
    ce158 += std::string(16, '\0');
    ce158 += std::string("\x7C\x01\x00\x01\x7C\x01\x9A\x9A", 8);
    const std::string code = scratch("open_code.bin", ce158);
    CHECK_EQ(server->line("open " + code).text, std::string("done 1\nfile-kind open_code.bin machine-code-program\n"));
    CHECK_EQ(server->line("open examples/startup/default-pc1500a.pc1500a").text,
             std::string("done 1\npreset default-pc1500a.pc1500a\n"));
    server->finish();
}

// `state control-state` in the
// harness has the paper panel owned by the plotter and the product entries.
TEST(harness_state_control_state_has_paper_panel_and_product_entries) {
    SKIP_UNLESS(std::filesystem::exists("roms/PC-1500_A04.ROM"), "the ROMs are not fetched (roms/fetch_roms.sh)");
    auto product = cu1600::makeHarnessProduct();
    std::string error;
    auto server = Server::start(*product, {}, &error);
    CHECK(server != nullptr);
    if (!server) return;
    CHECK(server->line("attach printer CE-150").text.rfind("done", 0) == 0);
    const std::string s = server->line("state control-state").text;
    CHECK(s.find("\"panels\":[{\"kind\":\"paper\",\"owner\":{\"place\":\"printer\",\"peripheral\":\"CE-150\"") != std::string::npos);
    CHECK(s.find("c1600.disk-controls") != std::string::npos && s.find("c1600.ce158-printer-panel") != std::string::npos);
    if (s.find("c1600.disk-controls") == std::string::npos) std::fprintf(stderr, "%s\n", s.c_str());
    server->finish();
}
