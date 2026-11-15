// The inputs of the JS mirror (LIB-14 design §6, §8 "Value form as the
// contract"): library/shell/tests/inputs.txt holds the Values that
// TestShellPart.hpp builds from LIB-4's ControlState and LIB-6's
// PresetListReply, one per line as `<name> <json>`, for
// library/web/tests/shell_model.test.js. This case fails when a payload's
// toValue() changes, so both model sets keep reading the same form.
// CALCU_SHELL_INPUTS=1 writes the file anew.
#include "TestHarness.hpp"
#include "TestShellPart.hpp"
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#ifndef CALCU_SHELL_FIXTURES
#error "CALCU_SHELL_FIXTURES is the folder of inputs.txt (library/CMakeLists.txt)"
#endif

using namespace calcu;
namespace st = calcu::shell::test;

namespace {

std::string fixtureText() {
    std::vector<std::pair<std::string, sessions::Value>> values;
    values.emplace_back("modelA", st::controlValue("Model-A"));
    values.emplace_back("modelB", st::controlValue("Model-B"));
    auto aux = st::controlState();
    st::plug(aux, "aux", "test-plotter");
    values.emplace_back("auxPlotter", aux.toValue());
    auto printer = st::controlState();
    st::plug(printer, "printer", "test-dotter");
    values.emplace_back("printerDotter", printer.toValue());
    auto unavailable = st::controlState();
    unavailable.models[1].available = st::unavailable("Model-B needs a module.");
    unavailable.actions[0].available = st::unavailable("Reset is not possible now.");
    unavailable.places[2].fitting[0].available = st::unavailable("TD-20 does not fit now.");
    values.emplace_back("unavailable", unavailable.toValue());
    auto oneSlot = st::controlState();
    oneSlot.places.erase(oneSlot.places.begin() + 1);
    values.emplace_back("oneSlot", oneSlot.toValue());
    auto noColdBoot = st::controlState();
    noColdBoot.actions.erase(noColdBoot.actions.begin() + 1);
    values.emplace_back("noColdBoot", noColdBoot.toValue());
    values.emplace_back("presetList", st::presetList());
    values.emplace_back("media", st::mediaWithKind());
    std::string out =
        "# The inputs of library/web/tests/shell_model.test.js, written by\n"
        "# library/shell/tests/inputs_fixture_tests.cpp (CALCU_SHELL_INPUTS=1). Do not edit.\n";
    for (const auto& [name, v] : values) out += name + " " + sessions::toText(v) + "\n";
    return out;
}

} // namespace

TEST(inputs_fixture_matches_payloads) {
    const std::string path = std::string(CALCU_SHELL_FIXTURES) + "/inputs.txt";
    const std::string text = fixtureText();
    if (const char* w = std::getenv("CALCU_SHELL_INPUTS"); w && std::string(w) == "1") {
        std::ofstream(path, std::ios::binary) << text;
    }
    std::ifstream in(path, std::ios::binary);
    std::stringstream got;
    got << in.rdbuf();
    CHECK(got.str() == text);
}
