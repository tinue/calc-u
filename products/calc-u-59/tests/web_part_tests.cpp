// TIWebPart (LIB-15 59, M-34): what the Web page draws, built and tested
// natively as the Web engine (frontends/web/bindings.cpp) builds the session:
// the shell's start (N3: the model by its name, a bundled preset by its path;
// EC-4), the settings in Calc-U-59.ini (LIB-9 AC-23), cards in the save
// folder, a dropped file, and the topics the page reads (AC-22).

#include "TestHarness.hpp"
#include "SessionRig.hpp"
#include "../app/TIWebPart.hpp"
#include "settings/AtomicFile.hpp"
#include "settings/HostDirs.hpp"
#include "settings/PlatformBase.hpp"
#include "settings/SettingsFile.hpp"
#include <fstream>

using namespace cu59;
using namespace std::chrono_literals;
using namespace calcu::sessions;
namespace fs = std::filesystem;

namespace {

// What the Web engine makes before the session (bindings.cpp's
// makeWebAppSession): the folders, the settings file, the Web part.
struct WebPieces {
    calcu::testing::TempFolder webFolder{"calcu59-web"};
    calcu::settings::HostDirs dirs;
    std::unique_ptr<calcu::settings::SettingsFile> file;
    std::unique_ptr<settings::Settings> settings;
    std::unique_ptr<TIWebPart> web;
    WebPieces() {
        dirs.frontend = calcu::settings::FrontendKind::Web;
        dirs.settingsBase = (webFolder.path() / "settings").string();
        dirs.saveFolder = (webFolder.path() / "Calc-U-59").string();
        fs::create_directories(dirs.saveFolder);
        webFolder.write("presets/one.ti59", "MODEL: TI-58\nREGISTERS:\n05 = 3.25\n");
        file = std::make_unique<calcu::settings::SettingsFile>(calcu::settings::SettingsFile::open(
            settings::settingsDecl(), calcu::settings::settingsFilePath(settings::settingsDecl(), dirs)));
        settings = std::make_unique<settings::Settings>(*file);
        web = std::make_unique<TIWebPart>(*settings, (webFolder.path() / "presets").string());
    }
};

// The Web engine's session, natively: the Web part behind the product's.
struct WebRig : WebPieces, testroms::SessionRig {
    WebRig()
        : SessionRig(testroms::RigOptions{.model = settings->startupModel(), .after = {web.get()},
                                          .saveFolder = dirs.saveFolder, .noTraceFolder = true}) {
        web->bind(part());
    }
    ~WebRig() { web.reset(); }   // first: it tells the part it goes
    void send(const char* name, Value args = Value::object({})) {
        post(CommandName{name}, std::move(args));
        session().call(now);
    }
    void run(std::chrono::milliseconds d) { runFor(d); }
    Value state(const TopicName& t) const {
        const PayloadPtr* p = fe.mirror.state(t);
        return p && *p ? (*p)->toValue() : Value();
    }
};

} // namespace

TEST(lib15_ac16_start_takes_the_model_by_name_and_a_bundled_preset) {
    WebRig r;
    r.send("start", Value::object({{"model", "TI-58C"}, {"preset", ""}}));
    CHECK(r.part().model() == Model::TI58C);
    r.send("start", Value::object({{"model", ""}, {"preset", "one.ti59"}}));
    r.run(500ms);
    CHECK(r.part().model() == Model::TI58);
    CHECK_EQ(testroms::mem(r.part().emulator()->machine()).readDataReg(5), 3.25);
    CHECK(r.notices().empty());
}

TEST(lib15_ec4_unknown_model_and_preset_are_notices) {
    WebRig r;
    r.send("start", Value::object({{"model", "TI-99"}, {"preset", "../one.ti59"}}));
    r.run(100ms);
    CHECK(r.part().model() == Model::TI59);
    CHECK_EQ(r.notices().size(), size_t{2});
    if (r.notices().size() == 2) {
        CHECK(r.notices()[0].find("has no model TI-99") != std::string::npos);
        CHECK(r.notices()[1].find("../one.ti59 is not a bundled preset") != std::string::npos);
    }
}

TEST(lib9_ac23_web_settings_in_the_ini_file) {
    WebRig r;
    r.send("c59.web.set-startup-model", Value::object({{"model", 2}}));
    r.send("c59.web.set-trace-max-mb", Value::object({{"mb", 7}}));
    r.run(50ms);
    const auto ini = calcu::settings::readWholeFile(
        calcu::settings::settingsFilePath(settings::settingsDecl(), r.dirs)).value_or("");
    CHECK(ini.find("model = TI-58C") != std::string::npos);
    const Value s = r.state(topics::kWebSettings);
    CHECK(s.find("startupModel") && s.find("startupModel")->integer() && *s.find("startupModel")->integer() == 2);
    CHECK(s.find("traceMaxMB") && *s.find("traceMaxMB")->integer() == 7);
}

TEST(lib15_ac20_cards_are_files_of_the_save_folder) {
    WebRig r;
    r.run(1s);
    // A blank card fitted, written by the machine, listed by the card topic.
    r.send("c59.web.fit-blank-card", Value::object({{"name", "Mine.U59"}}));
    CHECK(r.part().cardFitted());
    r.send("c59.web.fit-blank-card", Value::object({{"name", "../x.U59"}}));   // never out of the folder
    // A dropped card joins the folder once fitted.
    const std::string text = "Calc-U-59 Card 2.0\n";
    r.send("c59.web.drop", Value::object({{"name", "notes.txt"}, {"bytes", Value(Value::Bytes(text.begin(), text.end()))}}));
    r.run(50ms);
    const Value card = r.state(topics::kWebCard);
    CHECK(card.find("fitted") && card.find("fitted")->boolean() && *card.find("fitted")->boolean());
}

TEST(lib15_ac22_the_page_topics_are_there) {
    WebRig r;
    r.run(1s);
    for (const auto* t : {&topics::kWebModel, &topics::kWebDisplay, &topics::kWebRunState, &topics::kWebPrinter,
                          &topics::kWebCard, &topics::kWebLog, &topics::kWebSettings})
        CHECK(r.fe.mirror.state(*t) != nullptr);
    const Value display = r.state(topics::kWebDisplay);
    CHECK(display.bytes() && display.bytes()->size() == 36 * sizeof(float));
    const Value model = r.state(topics::kWebModel);
    CHECK(model.find("moduleID") && model.find("moduleID")->string() && *model.find("moduleID")->string() == "ML");
}

// LIB-5 (AC-21, AC-24, AC-10): the page's key grid and the host keys go
// through the library's typing; the key the page draws pressed is the one
// live input holds (c59.web.run-state heldKey).
TEST(lib5_ac24_web_keys_through_the_typing_engine) {
    WebRig r;
    r.run(1s);
    auto held = [&] {
        const Value v = r.state(topics::kWebRunState);
        const Value* h = v.find("heldKey");
        return h && h->integer() ? *h->integer() : -2;
    };
    r.send("c59.web.key-button-down", Value::object({{"row", 6}, {"col", 2}}));   // 5
    CHECK_EQ(held(), std::int64_t(32));
    r.send("c59.web.key-button-up");
    CHECK_EQ(held(), std::int64_t(-1));
    r.run(300ms);
    CHECK_EQ(testroms::displayText(r.machine()), std::string("5"));
    calcu::typing::HostKeyEvent e;
    e.physical = "KeyQ";
    e.carried = 'a';   // AZERTY: the key at Q carries A
    e.produced = 'a';
    r.send("host-key-down", calcu::typing::toValue(e));
    CHECK_EQ(held(), std::int64_t(0));
    r.send("c59.web.blur");   // focus lost: released after its minimum
    r.run(300ms);
    CHECK_EQ(held(), std::int64_t(-1));
    CHECK(r.part().typing()->engine().down().empty());
}
