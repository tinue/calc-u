// LIB-11 design T9 to T12, T18: sound, the run folder and settings, the
// clock, determinism, the machine state (AC-6, AC-8, AC-9 audio, AC-21,
// AC-24; EC-1, EC-3, EC-6).
#include "Files.hpp"
#include "TempFolder.hpp"
#include "TestHarness.hpp"
#include "TestProduct.hpp"
#include "harness/Run.hpp"
#include "peripherals/RealTimeClock.hpp"
#include "settings/Paths.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

using namespace calcu::harness;
using namespace calcu::harness::test;
namespace fs = std::filesystem;
namespace mt = calcu::machine::test;
using calcu::testing::TempFolder;

namespace {
std::uint32_t u32At(const std::string& s, std::size_t at) {
    std::uint32_t v = 0;
    for (int i = 3; i >= 0; --i) v = (v << 8) | static_cast<unsigned char>(s[at + std::size_t(i)]);
    return v;
}
bool inside(const std::string& path, const std::string& folder) { return path.rfind(folder, 0) == 0; }
std::string str(const fs::path& p) { return p.string(); }
} // namespace

// ── Sound (T9) ──────────────────────────────────────────────────────────

TEST(ac9_audio_records_all_sound_also_of_an_operation) {
    TempFolder out;
    const std::string wav = str(out.path() / "run.wav");
    {
        TestRun run({}, mt::kDot);   // 8000 samples a second
        CHECK(run.server != nullptr);
        if (!run.server) return;
        run.line("wait 2ms");   // before audio: not in the file
        CHECK_EQ(run.line("audio " + wav), std::string("done\n"));
        CHECK_EQ(run.line("test operation 10000"), std::string("done\n"));   // 10 ms in one go
        run.line("wait 5ms");
        run.server->finish();
    }
    const std::string bytes = calcu::testing::readText(wav);
    CHECK_EQ(bytes.size(), std::size_t{44 + 2 * 120});   // 15 ms at 8000 a second
    if (bytes.size() < 44) return;
    CHECK(bytes.substr(0, 4) == "RIFF" && bytes.substr(8, 4) == "WAVE" && bytes.substr(36, 4) == "data");
    CHECK_EQ(u32At(bytes, 24), std::uint32_t{8000});
}

TEST(ec3_wav_complete_when_input_ends) {
    TempFolder out;
    const std::string first = str(out.path() / "a.wav"), second = str(out.path() / "b.wav");
    TestRun run({}, mt::kDot);
    CHECK(run.server != nullptr);
    if (!run.server) return;
    std::vector<std::string> in = {"audio " + first, "wait 1ms", "audio " + second, "wait 2ms"};
    std::size_t at = 0;
    run.server->serve([&]() -> std::optional<std::string> { return at < in.size() ? std::optional(in[at++]) : std::nullopt; },
                      [](const std::string&) {});
    // The header's sizes are those of the data, in both files (D-8: a second
    // audio completes the first file).
    for (const auto& [path, samples] : {std::pair{first, 8u}, std::pair{second, 16u}}) {
        const std::string b = calcu::testing::readText(path);
        CHECK_EQ(b.size(), std::size_t{44 + 2 * samples});
        if (b.size() < 44) continue;
        CHECK_EQ(u32At(b, 4), std::uint32_t(36 + 2 * samples));
        CHECK_EQ(u32At(b, 40), std::uint32_t(2 * samples));
    }
}

TEST(ec6_audio_on_machine_without_sound_is_refused) {
    TempFolder out;
    TestRun run;   // test-seg has no sound
    CHECK(run.server != nullptr);
    if (!run.server) return;
    CHECK_EQ(run.line("audio " + str(out.path() / "x.wav")), std::string("refused: This machine has no sound.\n"));
    CHECK(!fs::exists(out.path() / "x.wav"));
}

// ── The run folder, settings and ROMs (T10) ─────────────────────────────

TEST(ac8_defaults_and_no_settings_file_read_or_written) {
    TestRun run;
    CHECK(run.server != nullptr);
    if (!run.server) return;
    const RunSetup& s = run.server->setup();
    CHECK(s.settings().outcome() == calcu::settings::Outcome::DefaultsOnly);
    CHECK(!s.settings().writable());
    CHECK_EQ(s.settings().text("startup", "model"), std::string("last"));   // the default
    // The host folders a feature would use are inside the run folder; no
    // settings file is there after a run.
    const auto& d = s.hostDirs();
    CHECK(inside(d.home, s.runFolder()) && inside(d.settingsBase, s.runFolder()));
    const calcu::settings::Paths paths(s.settings(), d);
    CHECK(inside(paths.settingsFile(), s.runFolder()));
    run.line("wait 1ms");
    CHECK(!fs::exists(paths.settingsFile()));
}

TEST(ac8_run_folder_empty_removed_and_in_no_reply) {
    TestRun run;
    CHECK(run.server != nullptr);
    if (!run.server) return;
    const std::string save = run.server->setup().saveFolder(), folder = run.server->setup().runFolder();
    CHECK(fs::is_directory(save) && fs::is_empty(save));
    CHECK(inside(save, folder));
    const std::string reply = run.line("test fail"), notices = run.line("state notices");
    CHECK(reply.find("<save folder>/missing.txt") != std::string::npos);
    CHECK(notices.find("<save folder>/missing.txt") != std::string::npos);
    CHECK(reply.find(save) == std::string::npos && notices.find(save) == std::string::npos);
    run.server->finish();
    CHECK(!fs::exists(folder));
}

TEST(ac8_traces_and_screenshots_go_to_the_run_folder) {
    TestRun run;
    CHECK(run.server != nullptr);
    if (!run.server) return;
    const RunSetup& s = run.server->setup();
    CHECK_EQ(s.traceFolder(), s.saveFolder());
    CHECK_EQ(s.screenshotFolder(), s.saveFolder());
    CHECK(inside(s.traceFolder(), s.runFolder()));
}

TEST(ac8_options_name_settings_save_and_output_folders) {
    TempFolder mine;
    const fs::path ini = mine.path() / "Calc-U-Test.ini", save = mine.path() / "save", output = mine.path() / "out";
    { std::ofstream(ini) << "format-version = 1\n[startup]\nmodel = test-dot\n"; }
    const std::string before = calcu::testing::readText(ini);
    RunOptions o;
    o.settingsFile = str(ini);
    o.saveFolder = str(save);
    o.outputFolder = str(output);
    {
        TestRun run(o);
        CHECK(run.server != nullptr);
        if (!run.server) return;
        CHECK(run.server->setup().settings().outcome() == calcu::settings::Outcome::Read);
        CHECK(run.machine().model() == mt::kDot);   // [startup] model of the named file
        CHECK_EQ(fs::path(run.server->setup().saveFolder()), fs::absolute(save));
        CHECK_EQ(fs::path(run.server->setup().traceFolder()), fs::absolute(output));
        CHECK_EQ(fs::path(run.server->setup().screenshotFolder()), fs::absolute(output));
        CHECK(run.line("test fail").find("<save folder>/missing.txt") != std::string::npos);
        { std::ofstream(save / "kept.txt") << "mine"; }
    }
    // The named folders are the agent's: kept, with what is in them. The
    // named settings file is used as a product uses it: the session records
    // the last used model there (LIB-18 AC-9), and nothing else changes.
    CHECK(fs::exists(save / "kept.txt") && fs::is_directory(output));
    const std::string after = calcu::testing::readText(ini);
    CHECK(after.rfind(before, 0) == 0 || after.find("model = test-dot\n") != std::string::npos);
    CHECK(after.find("last-model = test-dot") != std::string::npos);
}

// LIB-11 AC-2 with LIB-18 (design L8): the run starts with the machine the
// product starts with under the default settings: `last` and nothing
// recorded gives the fallback model, built by LIB-18's start, at key readiness.
TEST(ac2_run_starts_with_the_startup_model_under_default_settings) {
    TestRun seg;
    CHECK(seg.server != nullptr);
    if (!seg.server) return;
    CHECK(seg.machine().model() == mt::kSeg);
    CHECK(seg.machine().isKeyReady());
    CHECK(seg.machine().tickCount() >= mt::kStartUpTicks);
    CHECK_EQ(seg.server->setup().settings().text("startup", "last-model"), std::string("test-seg"));   // kept for the run
    CHECK_EQ(seg.line("model test-seg"), std::string("done\n"));   // the one machine: nothing to do
    TestRun dot({}, mt::kDot);   // another fallback model in the declaration
    CHECK(dot.server && dot.machine().model() == mt::kDot && dot.machine().isKeyReady());
}

TEST(ac8_bundled_roms_through_the_lookup) {
    TestRun run({}, mt::kDot);
    CHECK(run.server != nullptr);
    if (!run.server) return;
    // Every file of the full set was read once at the start (LIB-12 AC-6):
    // the machine runs with the bundle gone.
    std::error_code ec;
    for (const auto& e : fs::directory_iterator(run.product.romFolder().path())) fs::remove(e.path(), ec);
    CHECK(run.line("wait 1ms").rfind("done 2\n", 0) == 0);
    CHECK(!run.product.lookUpRoms().ok());   // a new start would not find them
}

TEST(ec1_missing_rom_ends_at_start_naming_files) {
    TestProduct p;
    p.romFolder().remove("test-seg.rom");
    p.romFolder().remove("test-dot-v2.rom");
    std::string error;
    auto s = Server::start(p, {}, &error);
    CHECK(s == nullptr);
    CHECK(error.find("test-seg.rom") != std::string::npos);
    CHECK(error.find("test-dot-v2.rom") != std::string::npos);
}

// ── The clock (T11) ─────────────────────────────────────────────────────

TEST(ac21_fixed_instant_by_default_at_build_and_after_operation) {
    TestRun run({}, mt::kDot);   // test-dot's component rtc
    CHECK(run.server != nullptr);
    if (!run.server) return;
    auto* clock = run.machine().clock();
    CHECK(clock != nullptr);
    if (!clock) return;
    using calcu::peripherals::ClockFields;
    const ClockFields fixed{std::nullopt, 1, 1, 6, 0, 0, 0};   // 2000-01-01, a Saturday, 00:00:00
    CHECK(clock->read() == fixed);
    const int sets = clock->setCalls;
    run.line("wait 3s");
    CHECK(clock->read().second == std::optional<int>(3));   // the clock runs on ticks
    run.line("test operation 1000000");
    CHECK_EQ(clock->setCalls, sets + 1);
    CHECK(clock->read() == fixed);   // handed the same instant again
}

TEST(ac21_clock_option_names_instant) {
    RunOptions o;
    o.clock = ClockMode::Given;
    o.given = *parseDateTime("2026-11-15T13:45:30");
    TestRun run(o, mt::kDot);
    CHECK(run.server != nullptr);
    if (!run.server) return;
    using calcu::peripherals::ClockFields;
    CHECK(run.machine().clock()->read() == (ClockFields{std::nullopt, 11, 15, 0, 13, 45, 30}));   // a Sunday
    CHECK(!parseDateTime("2026-11-15 13:45:30") && !parseDateTime("2026-13-01T00:00:00") && !parseDateTime("host"));
}

TEST(ac21_clock_host_hands_host_time) {
    RunOptions o;
    o.clock = ClockMode::Host;
    const auto before = calcu::peripherals::hostDateTime(std::chrono::system_clock::now());
    TestRun run(o, mt::kDot);
    const auto after = calcu::peripherals::hostDateTime(std::chrono::system_clock::now());
    CHECK(run.server != nullptr);
    if (!run.server) return;
    const auto f = run.machine().clock()->read();
    CHECK((f.day == before.day && f.month == before.month) || (f.day == after.day && f.month == after.month));
    CHECK((f.hour == before.hour && f.minute == before.minute) || (f.hour == after.hour && f.minute == after.minute));
}

// ── Determinism (T12) ───────────────────────────────────────────────────

namespace {
std::vector<std::string> script(bool json) {
    RunOptions o;
    o.json = json;
    TestRun run(o, mt::kDot);
    std::vector<std::string> out;
    if (!run.server) return out;
    for (const char* l : {"ticks", "wait 1.5s", "key-down a", "wait 20ms", "lcd", "display", "key-up a", "state busy",
                          "capabilities", "test operation 123457", "ticks", "state test.count", "test fail",
                          "state notices", "state log", "key-down zz", "wait 2", "lcd"})
        out.push_back(run.line(l));
    return out;
}
} // namespace

TEST(ac6_two_runs_give_the_same_replies) {
    for (bool json : {false, true}) {
        const auto a = script(json);
        std::this_thread::sleep_for(std::chrono::milliseconds(30));   // wall time passes between the runs
        const auto b = script(json);
        CHECK(!a.empty());
        CHECK(a == b);
    }
}

TEST(ac6_no_pacer_and_no_wall_clock) {
    TestRun run;
    CHECK(run.server != nullptr);
    if (!run.server) return;
    const int reads = run.server->wallClockReads();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    // Wall time passing moves nothing: emulated time passes only by commands.
    // The start's one run piece (LIB-18 D-4), nothing more.
    CHECK_EQ(run.line("ticks"), std::string("done 2\nticks: 1667\nseconds: 0.016670000\n"));
    run.line("wait 10ms");
    run.line("lcd");
    CHECK_EQ(run.machine().tickCount(), std::uint64_t{2667});
    // The session is hosted in virtual time: no host call reads the clock
    // for a deadline (the pacer only notes it when it starts afresh).
    CHECK_EQ(run.server->wallClockReads(), reads);
}

// ── The machine state (T18) ─────────────────────────────────────────────

namespace {
std::vector<std::string> after(TestRun& run) {
    std::vector<std::string> out;
    for (const char* l : {"wait 10ms", "key-down 2", "wait 5ms", "lcd", "display", "ticks", "key-up 2", "wait 1ms", "lcd"})
        out.push_back(run.line(l));
    return out;
}
} // namespace

TEST(ac24_restore_continues_identically) {
    TempFolder f;
    const std::string file = str(f.path() / "state.bin");
    TestRun run;   // test-seg offers the kind
    CHECK(run.server != nullptr);
    if (!run.server) return;
    run.line("wait 7ms");
    run.line("key-down 1");
    CHECK_EQ(run.line("state-save " + file), std::string("done\n"));
    const auto first = after(run);
    CHECK_EQ(run.line("state-restore " + file), std::string("done\n"));
    CHECK(after(run) == first);
    // In another run as well.
    TestRun other;
    CHECK(other.server != nullptr);
    if (!other.server) return;
    CHECK_EQ(other.line("state-restore " + file), std::string("done\n"));
    CHECK(after(other) == first);
}

TEST(ac24_restore_refused_for_other_configuration_changes_nothing) {
    TempFolder f;
    const std::string file = str(f.path() / "state.bin");
    {
        TestRun run;
        CHECK(run.server && run.line("state-save " + file) == "done\n");
    }
    // Another product version.
    TestRun later;
    later.product.version_ = "2.0-test";
    later.server.reset();
    later.server = Server::start(later.product, {}, &later.error);
    CHECK(later.server != nullptr);
    if (!later.server) return;
    later.line("wait 1ms");
    const std::string before = later.line("ticks");
    CHECK(later.line("state-restore " + file).rfind("refused: The file is of another product version", 0) == 0);
    CHECK_EQ(later.line("ticks"), before);
    // Another model in the header.
    std::string bytes = calcu::testing::readText(file);
    const auto at = bytes.find("model: test-seg");
    CHECK(at != std::string::npos);
    if (at == std::string::npos) return;
    bytes.replace(at, 15, "model: test-dox");
    { std::ofstream(file, std::ios::binary) << bytes; }
    TestRun same;
    CHECK(same.server && same.line("state-restore " + file).rfind("refused: ", 0) == 0);
    { std::ofstream(file, std::ios::binary) << "not a state"; }
    CHECK(same.line("state-restore " + file) == "refused: The file holds no machine state.\n");
}

TEST(ac24_refused_without_the_kind) {
    TempFolder f;
    TestRun run({}, mt::kDot);   // test-dot does not offer it
    CHECK(run.server != nullptr);
    if (!run.server) return;
    CHECK_EQ(run.line("state-save " + str(f.path() / "s.bin")),
             std::string("refused: This machine does not offer its machine state.\n"));
    CHECK(!fs::exists(f.path() / "s.bin"));
}

TEST(ac24_host_files_not_in_state) {
    TempFolder f;
    const std::string file = str(f.path() / "state.bin");
    TestRun run;
    CHECK(run.server != nullptr);
    if (!run.server) return;
    { std::ofstream(fs::path(run.server->setup().saveFolder()) / "host-file.txt") << "of the host"; }
    run.line("state-save " + file);
    const std::string bytes = calcu::testing::readText(file);
    CHECK(bytes.rfind("calcu-machine-state 1\nproduct: Calc-U-Test\nversion: 1.0-test\nmodel: test-seg\nvariant: -\n\n", 0) == 0);
    CHECK(bytes.find("host-file") == std::string::npos && bytes.find("of the host") == std::string::npos);
    CHECK(bytes.find(run.server->setup().saveFolder()) == std::string::npos);
}
