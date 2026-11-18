// Help and About in the model; the drawn dialog, the log and a help address
// that cannot be opened are the Qt6 cases of
// library/qt6/tests/shell_widgets_tests.cpp.
#include "TestHarness.hpp"
#include "TestShellPart.hpp"
#include "shell/MenuModel.hpp"
#include "shell/ShellProduct.hpp"
#include <string>

using namespace calcu;
using namespace calcu::shell;
namespace st = calcu::shell::test;

TEST(help_opens_product_address) {
    const auto p = st::product();
    const auto ms = menus(st::inputs(), p, Traits{});
    const MenuItem* help = findItem(ms, actions::kHelp);
    CHECK(help != nullptr);
    CHECK_EQ(help->label, std::string("Test-Product Help"));
    CHECK(help->command.value.empty());   // a shell action: it opens p.helpUrl
    CHECK_EQ(p.helpUrl, std::string("https://calc-u.org/test/"));
    // Help works also when the product could not start.
    CHECK(findItem(menus(ShellInputs{}, p, Traits{}), actions::kHelp) != nullptr);
}

TEST(about_shows_product_texts) {
    auto p = st::product();
    AboutModel a = about(p);
    CHECK_EQ(a.title, std::string("About Test-Product"));
    CHECK_EQ(a.name, std::string("Test-Product"));
    CHECK_EQ(a.version, std::string("Version 1.2.3 (abc123)"));
    CHECK_EQ(a.lines.size(), std::size_t{2});
    CHECK_EQ(a.lines[0], std::string("A product made for the tests."));
    CHECK_EQ(a.links.size(), std::size_t{2});
    CHECK_EQ(a.links[1].second, std::string("https://example.org/privacy"));
    p.aboutLabel = "About Test-Product…";   // a product's label with an ellipsis
    const auto ms = menus(st::inputs(), p, Traits{});
    CHECK_EQ(findItem(ms, actions::kAbout)->label, std::string("About Test-Product…"));
    CHECK_EQ(about(p).title, std::string("About Test-Product"));
    p.build.clear();
    CHECK_EQ(about(p).version, std::string("Version 1.2.3"));
}
