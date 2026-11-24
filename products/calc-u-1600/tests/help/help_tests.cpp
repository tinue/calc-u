// What the help tool cannot read itself.
// The settings keys the product declares reach `tools/help` through
// help/settings-keys.txt, which this test compares with settingsDecl()
// (CALCU1600_HELP_KEYS=1 writes it anew); the words a shared part may not
// use (help/help.yaml's `names:`) cover the catalogue and the bundled
// module definitions. A program of its own (CalcU1600HelpTests), so that
// CalcU1600CoreTests' Frequent budget stays as it is.

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "TestHarness.hpp"
#include "../../app/Settings.hpp"
#include "Machine/SharpCatalogue.hpp"
#include "Machine/SharpRoms.hpp"
#include "roms/Bundle.hpp"
#include "settings/Declaration.hpp"
#include "testing/SettingsKeysFile.hpp"

namespace {

namespace cs = calcu::settings;

const char* kKeysFile = "help/settings-keys.txt";
const char* kManifest = "help/help.yaml";

std::string readFile(const char* path) {
    std::ifstream in(path, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

std::string keysFileText() {
    return calcu::testing::settingsKeysFileText(cu1600::settingsDecl(), "CalcU1600HelpTests with CALCU1600_HELP_KEYS=1");
}

// The `names:` list of help/help.yaml (one flow sequence on its line).
std::set<std::string> manifestNames() {
    std::istringstream in(readFile(kManifest));
    std::string line;
    std::set<std::string> names;
    while (std::getline(in, line)) {
        if (line.rfind("names:", 0) != 0) continue;
        const auto open = line.find('[');
        const auto close = line.find(']');
        if (open == std::string::npos || close == std::string::npos) break;
        std::istringstream items(line.substr(open + 1, close - open - 1));
        std::string item;
        while (std::getline(items, item, ',')) {
            const auto b = item.find_first_not_of(" \"");
            const auto e = item.find_last_not_of(" \"");
            if (b != std::string::npos) names.insert(item.substr(b, e - b + 1));
        }
    }
    return names;
}

} // namespace

TEST(settings_keys_file_matches_declaration) {
    const std::string want = keysFileText();
    if (const char* write = std::getenv("CALCU1600_HELP_KEYS"); write && std::string(write) == "1") {
        std::ofstream(kKeysFile, std::ios::binary) << want;
    }
    const std::string have = readFile(kKeysFile);
    if (have != want) std::printf("%s differs from settingsDecl(); expected:\n%s", kKeysFile, want.c_str());
    CHECK(have == want);
}

TEST(every_declared_key_is_in_the_keys_file) {
    // Every key of the declaration shows up: by its name, or by its
    // section's general form and its name as a product line.
    const std::string text = keysFileText();
    for (const cs::KeyDecl& d : cu1600::settingsDecl().keys) {
        const std::string name = "[" + d.section + "] " + d.key;
        CHECK(text.find(" " + name + "\n") != std::string::npos);
    }
    CHECK(text.find("product [debug] dap-port\n") != std::string::npos);
    CHECK(text.find("family [startup] model\n") != std::string::npos);
    CHECK(text.find("family [preset] <model>\nproduct [preset] PC-1500\n") != std::string::npos);
}

TEST(help_names_cover_the_catalogue) {
    const std::set<std::string> names = manifestNames();
    CHECK(names.count("Calc-U-1600") == 1);
    calcu::machine::Catalogue catalogue = sharp::sharpCatalogue();
    sharp::addModuleDefinitions(catalogue, "frontends/qt6/resources/cards");
    const auto roms = sharp::lookUpRoms(calcu::roms::DirectoryBundle("roms")).set;
    if (roms) sharp::addModuleDefinitions(catalogue, *roms);  // the CE-502B
    for (const auto& m : catalogue.models()) {
        if (!names.count(m.model.value)) std::printf("help.yaml names: lacks the model %s\n", m.model.value.c_str());
        CHECK(names.count(m.model.value) == 1);
    }
    for (const auto& p : catalogue.peripherals()) {
        const std::string& name = p.name.empty() ? p.id.value : p.name;
        if (!names.count(name)) std::printf("help.yaml names: lacks the peripheral %s\n", name.c_str());
        CHECK(names.count(name) == 1);
    }
}
