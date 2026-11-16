// TIWebPart (LIB-15 59, M-34): what the Web page draws, built and tested
// natively as the Web engine (frontends/web/bindings.cpp) builds the session:
// the shell's start (N3: the model by its name, a bundled preset by its path;
// EC-4), the settings in Calc-U-59.ini (LIB-9 AC-23), cards in the save
// folder, a dropped file, and the topics the page reads (AC-22).

#include "TestHarness.hpp"
#include "../app/TIMagneticCardKind.hpp"
#include "media/MediaTopics.hpp"
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
        webFolder.write("presets/one.ti59", "format-version: 2\nmodel: TI-58\ndata:\n  05: 3.25\n");
        file = std::make_unique<calcu::settings::SettingsFile>(calcu::settings::SettingsFile::open(
            settings::settingsDecl(), calcu::settings::settingsFilePath(settings::settingsDecl(), dirs)));
        settings = std::make_unique<settings::Settings>(*file);
        web = std::make_unique<TIWebPart>(*settings);
    }
};

// The Web engine's session, natively: the Web part behind the product's.
struct WebRig : WebPieces, testroms::SessionRig {
    explicit WebRig(bool started = true)
        : SessionRig(testroms::RigOptions{.model = WebPieces::settings->startupModel(), .after = {web.get()},
                                          .saveFolder = dirs.saveFolder, .noTraceFolder = true, .settings = file.get(),
                                          .examplesFolder = (webFolder.path() / "presets").string(), .start = false}) {
        web->bind(part());
        if (started) {   // the page's start (LIB-18's, LIB-15 N3)
            post(CommandName{"start"}, Value::object({}));
            until([this] { return session().machineSession().machine() && !busy(); });
        }
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

// LIB-15 AC-16 (LIB-6 §11.6 P11): the page's start is LIB-18's `start
// {model, preset}`, the model by its name and the preset by its bundle path.
TEST(lib15_ac16_start_takes_the_model_by_name_and_a_bundled_preset) {
    {
        WebRig r(false);
        r.send("start", Value::object({{"model", "TI-58C"}, {"preset", ""}}));
        r.run(2s);
        CHECK(r.part().model() == Model::TI58C);
    }
    WebRig r(false);
    r.send("start", Value::object({{"model", ""}, {"preset", "examples:one.ti59"}}));
    r.run(2s);
    CHECK(r.part().model() == Model::TI58);
    CHECK_EQ(testroms::mem(r.part().emulator()->machine()).readDataReg(5), 3.25);
    CHECK(r.notices().empty());
}

// LIB-15 EC-4: a model or a preset the product does not have is a notice;
// the start goes on with the startup model's built-in default.
TEST(lib15_ec4_unknown_model_and_preset_are_notices) {
    WebRig r(false);
    r.send("start", Value::object({{"model", "TI-99"}, {"preset", "examples:../one.ti59"}}));
    r.run(2s);
    CHECK(r.part().model() == Model::TI59);
    CHECK_EQ(r.notices().size(), size_t{2});
    if (r.notices().size() == 2) {
        CHECK(r.notices()[0].find("no model \"TI-99\"") != std::string::npos);
        CHECK(r.notices()[1].find("could not be loaded") != std::string::npos);
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

// LIB-15 AC-20 (M-55, LIB-7 59): the cards are the library's media of the
// persistent save folder; a dropped card file is imported into it and fitted.
TEST(lib15_ac20_cards_are_files_of_the_save_folder) {
    WebRig r;
    r.run(1s);
    r.send("media.start-new", Value::object({{"place", "card-reader"}, {"template", "implicit:Fresh card"}, {"name", "Mine"}}));
    CHECK(r.part().cardFitted());
    auto card = std::make_shared<TIMagneticCardData>();
    const std::string text = writeMagneticCardFile(*card, "Dropped", "", false, std::nullopt, "");
    r.send("c59.web.drop", Value::object({{"name", "Dropped.magnetcard.yaml"}, {"bytes", Value(Value::Bytes(text.begin(), text.end()))}}));
    r.run(50ms);
    CHECK(fs::exists(fs::path(r.part().files().saveFolder()) / "Dropped.magnetcard.yaml"));
    const auto* media = r.part().mediaPart()->bindingOf("card-reader");
    CHECK(media && media->name == "Dropped");
    r.send("c59.web.drop", Value::object({{"name", "old.u59"}, {"bytes", Value(Value::Bytes(text.begin(), text.end()))}}));
    CHECK(!fs::exists(fs::path(r.part().files().saveFolder()) / "old.u59"));   // C59-7 AC-4: no card
}

TEST(lib15_ac22_the_page_topics_are_there) {
    WebRig r;
    r.run(1s);
    for (const auto* t : {&topics::kWebModel, &topics::kWebDisplay, &topics::kWebRunState, &topics::kWebPrinter,
                          &calcu::media::kMediaTopic, &topics::kWebLog, &topics::kWebSettings})
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
