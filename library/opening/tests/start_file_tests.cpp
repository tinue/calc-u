// LIB-13 T16-5 (AC-21; AC-14 asks the same of Calc-U-59): the file named at
// the start of a desktop program (startFile).
#include "TestHarness.hpp"
#include "opening/StartFile.hpp"

using calcu::opening::startFile;

TEST(lib13_ac21_start_file_is_last_file_argument) {
    const auto exists = [](const std::string& s) { return s == "/a/one.bas" || s == "/a/two.pc1500" || s == "/a/app"; };
    CHECK(!startFile({"/a/app"}, exists));
    CHECK(startFile({"/a/app", "/a/one.bas"}, exists) == std::optional<std::string>("/a/one.bas"));
    CHECK(startFile({"/a/app", "/a/one.bas", "/a/two.pc1500"}, exists) == std::optional<std::string>("/a/two.pc1500"));
    CHECK(startFile({"/a/app", "/a/one.bas", "-NSDocumentRevisionsDebugMode", "YES"}, exists) ==
          std::optional<std::string>("/a/one.bas"));
    CHECK(!startFile({"/a/app", "--dap", "4711", "/nowhere.bas"}, exists));
}
