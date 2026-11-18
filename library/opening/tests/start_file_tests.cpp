// The file named at the start of a desktop program (startFile).
#include "TestHarness.hpp"
#include "opening/StartFile.hpp"

using calcu::opening::startFile;

TEST(start_file_is_last_file_argument) {
    const auto exists = [](const std::string& s) { return s == "/a/one.bas" || s == "/a/two.pc1500" || s == "/a/app"; };
    CHECK(!startFile({"/a/app"}, exists));
    CHECK(startFile({"/a/app", "/a/one.bas"}, exists) == std::optional<std::string>("/a/one.bas"));
    CHECK(startFile({"/a/app", "/a/one.bas", "/a/two.pc1500"}, exists) == std::optional<std::string>("/a/two.pc1500"));
    CHECK(startFile({"/a/app", "/a/one.bas", "-NSDocumentRevisionsDebugMode", "YES"}, exists) ==
          std::optional<std::string>("/a/one.bas"));
    CHECK(!startFile({"/a/app", "--dap", "4711", "/nowhere.bas"}, exists));
}
