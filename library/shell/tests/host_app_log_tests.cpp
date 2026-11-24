#include "TempFolder.hpp"
#include "TestHarness.hpp"
#include "shell/HostAppLog.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// The native hosts' app log: the run's level from the launch arguments, the
// header, the file and the mirror.

using namespace calcu;
using applog::AppLogLevel;
using calcu::testing::TempFolder;

namespace {
std::string read(const std::string& path) {
    std::ifstream in(std::filesystem::path(path), std::ios::binary);
    std::ostringstream s;
    s << in.rdbuf();
    return s.str();
}
} // namespace

TEST(host_app_log_takes_the_run_level_of_its_arguments) {
    TempFolder dir;
    std::vector<std::string> mirrored;
    shell::HostAppLog log(dir / "P.log", "P 1.0 (abc)", {"app", "--app-log-level", "debug"},
                          [&](AppLogLevel, const std::string& line) { mirrored.push_back(line); });
    CHECK(log.runLevel() == AppLogLevel::Debug);
    CHECK(log.level() == AppLogLevel::Debug);
    log.debug("detail");
    CHECK_EQ(mirrored.size(), std::size_t(2));   // the header and the line
    CHECK(read(dir / "P.log").find("INFO  P 1.0 (abc)\n") != std::string::npos);
    CHECK(read(dir / "P.log").find("DEBUG detail\n") != std::string::npos);
}

TEST(host_app_log_without_a_run_level_starts_at_the_default) {
    TempFolder dir;
    shell::HostAppLog log(dir / "P.log", "P", {"app"}, {});
    CHECK(!log.runLevel());
    CHECK(log.level() == applog::defaultAppLogLevel());
    log.setLevel(AppLogLevel::Warn);
    log.info("left out");
    CHECK(read(dir / "P.log").find("left out") == std::string::npos);
}

TEST(host_app_log_says_why_a_level_or_a_file_does_not_hold) {
    TempFolder dir;
    dir.write("taken", "a file where the log's folder would be");
    std::vector<std::pair<AppLogLevel, std::string>> mirrored;
    shell::HostAppLog log(dir / "taken/P.log", "P", {"app", "--app-log-level", "loud"},
                          [&](AppLogLevel l, const std::string& line) { mirrored.emplace_back(l, line); });
    CHECK(!log.runLevel());
    CHECK_EQ(mirrored.size(), std::size_t(3));   // the header, the file's problem, the bad level
    CHECK(mirrored[1].first == AppLogLevel::Error && mirrored[1].second.find("cannot be written") != std::string::npos);
    CHECK(mirrored[2].first == AppLogLevel::Warn &&
          mirrored[2].second.find("--app-log-level takes error, warn, info or debug, not loud.") != std::string::npos);
}
