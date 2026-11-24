#include "TempFolder.hpp"
#include "TestHarness.hpp"
#include "applog/AppLogFile.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

// The app log: its levels, the line format, the file with its header and
// its rotation.

using namespace calcu::applog;
using calcu::testing::TempFolder;

namespace {
std::string read(const std::string& path) {
    std::ifstream in(std::filesystem::path(path), std::ios::binary);
    std::ostringstream s;
    s << in.rdbuf();
    return s.str();
}
std::size_t count(const std::string& text, std::string_view part) {
    std::size_t n = 0;
    for (std::size_t at = text.find(part); at != std::string::npos; at = text.find(part, at + 1)) ++n;
    return n;
}
} // namespace

TEST(app_log_level_names_and_settings) {
    for (AppLogLevel l : {AppLogLevel::Error, AppLogLevel::Warn, AppLogLevel::Info, AppLogLevel::Debug})
        CHECK(appLogLevelNamed(appLogLevelName(l)) == l);
    CHECK(!appLogLevelNamed("off"));
    CHECK(!appLogLevelNamed("WARN"));
    CHECK(appLogLevelOfSetting("debug") == AppLogLevel::Debug);
    CHECK(appLogLevelOfSetting("default") == defaultAppLogLevel());
}

TEST(app_log_keeps_its_level_and_the_ones_above) {
    MemoryAppLog log(AppLogLevel::Warn);
    log.error("e");
    log.warn("w");
    log.info("i");
    log.debug("d");
    CHECK_EQ(log.lines().size(), std::size_t(2));
    CHECK(log.has(AppLogLevel::Error, "e") && log.has(AppLogLevel::Warn, "w"));
    log.setLevel(AppLogLevel::Debug);
    log.debug("d");
    CHECK(log.has(AppLogLevel::Debug, "d"));
    log.setLevel(AppLogLevel::Error);
    log.warn("w2");
    CHECK(!log.has(AppLogLevel::Warn, "w2"));
}

TEST(app_log_line_format) {
    const std::string line = formatLine(std::chrono::system_clock::now(), AppLogLevel::Warn, "Copy failed: no\nclipboard");
    // 2026-11-24T10:15:03.123 WARN  Copy failed: no
    //   clipboard
    CHECK_EQ(line.size() > 24, true);
    CHECK_EQ(line[10], 'T');
    CHECK_EQ(line[19], '.');
    CHECK_EQ(line.substr(23, 7), std::string(" WARN  "));
    CHECK(line.find("Copy failed: no\n  clipboard") != std::string::npos);
    CHECK(formatLine(std::chrono::system_clock::now(), AppLogLevel::Debug, "x").find(" DEBUG x") != std::string::npos);
}

TEST(app_log_file_header_lines_and_mirror) {
    TempFolder dir;
    const std::string path = dir / "Logs/Product/Product.log";   // the folders are made
    std::string mirrored;
    {
        AppLogFile log(path, "Product 1.0 (abc) macOS", AppLogLevel::Info,
                       [&](AppLogLevel, const std::string& line) { mirrored += line + "\n"; });
        CHECK(log.problem().empty());
        log.info("Saved the module \"A\".");
        log.debug("left out");
    }
    const std::string text = read(path);
    CHECK(text.find("INFO  Product 1.0 (abc) macOS\n") != std::string::npos);
    CHECK(text.find("INFO  Saved the module \"A\".\n") != std::string::npos);
    CHECK(text.find("left out") == std::string::npos);
    CHECK_EQ(mirrored, text);
    {
        AppLogFile again(path, "Product 1.0 (abc) macOS", AppLogLevel::Warn);   // a second run appends
    }
    CHECK_EQ(count(read(path), "Product 1.0 (abc) macOS"), std::size_t(2));
}

TEST(app_log_file_rotates_at_start) {
    TempFolder dir;
    const std::string path = dir / "P.log";
    dir.write("P.1.log", "the oldest run\n");
    dir.write("P.log", std::string(200, 'x') + "\n");
    { AppLogFile log(path, "run", AppLogLevel::Warn, {}, /*rotateAt=*/100); }
    CHECK(read(dir / "P.1.log").find(std::string(200, 'x')) != std::string::npos);   // the older file replaced
    CHECK(read(dir / "P.1.log").find("the oldest run") == std::string::npos);
    CHECK(read(path).find(std::string(200, 'x')) == std::string::npos);
    CHECK(read(path).find("INFO  run") != std::string::npos);
    { AppLogFile log(path, "small", AppLogLevel::Warn, {}, /*rotateAt=*/100'000); }
    CHECK(read(path).find("INFO  small") != std::string::npos);   // under the limit: appended
    CHECK(read(path).find("INFO  run") != std::string::npos);
}

TEST(app_log_file_that_cannot_be_written_says_why_and_still_mirrors) {
    TempFolder dir;
    dir.write("taken", "a file where the log's folder would be");
    int mirrored = 0;
    AppLogFile log(dir / "taken/P.log", "run", AppLogLevel::Warn, [&](AppLogLevel, const std::string&) { ++mirrored; });
    CHECK(!log.problem().empty());
    CHECK(log.problem().find("cannot be written") != std::string::npos);
    log.warn("still seen");
    CHECK_EQ(mirrored, 2);   // the header and the line
}
