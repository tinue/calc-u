// TIWebPart (LIB-15 59, M-34): what the Web page draws, built and tested
// natively as the Web engine (frontends/web/bindings.cpp) builds the session:
// the shell's start (N3: the model by its name, a bundled preset by its path;
// EC-4), the settings in Calc-U-59.ini (LIB-9 AC-23), cards in the save
// folder, a dropped file, and the topics the page reads (AC-22).

#include "TestHarness.hpp"
#include "AppSessionRig.hpp"
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

// The Web engine's session (bindings.cpp's makeWebAppSession), natively.
struct WebRig : MessageSink {
    testroms::TempFolder folder;
    calcu::settings::HostDirs dirs;
    std::unique_ptr<calcu::settings::SettingsFile> file;
    std::unique_ptr<settings::Settings> settings;
    std::unique_ptr<TIWebPart> web;
    calcu::sessions::ProductSession<TIAppPart> app;
    calcu::pacing::WallTime now{std::chrono::hours(1)};
    TopicMirror mirror;
    std::vector<std::string> notices;
    ListenerId me = 0;

    WebRig() {
        dirs.frontend = calcu::settings::FrontendKind::Web;
        dirs.settingsBase = (folder.path / "settings").string();
        dirs.saveFolder = (folder.path / "Calc-U-59").string();
        fs::create_directories(dirs.saveFolder);
        fs::create_directories(folder.path / "presets");
        std::ofstream(folder.path / "presets" / "one.ti59") << "MODEL: TI-58\nREGISTERS:\n05 = 3.25\n";
        file = std::make_unique<calcu::settings::SettingsFile>(calcu::settings::SettingsFile::open(
            settings::settingsDecl(), calcu::settings::settingsFilePath(settings::settingsDecl(), dirs)));
        settings = std::make_unique<settings::Settings>(*file);
        web = std::make_unique<TIWebPart>(*settings, (folder.path / "presets").string(), folder.path / "traces");
        TIAppSetup s;
        s.roms = testroms::romSet();
        s.cuecards = testroms::catalog();
        s.startModel = settings->startupModel();
        s.saveFolder = dirs.saveFolder;
        s.trace = {folder.path / "traces", 1'000'000};
        app = makeAppSession(std::move(s), [this] { return now += 1ms; }, 59u, {}, {web.get()});
        web->bind(*app.part);
        me = app.session->listen(*this, {});
    }
    ~WebRig() {
        web.reset();   // first: it tells the part it goes
        app = {};
    }
    void deliver(const Message& m) override {
        mirror.deliver(m);
        if (const auto* s = std::get_if<StreamMessage>(&m); s && s->topic == kNoticesTopic)
            for (const auto& n : s->appended)
                if (const auto* x = payloadAs<Notice>(n)) notices.push_back(x->sentence);
    }
    void send(const char* name, Value args = Value::object({})) {
        app.session->send(me, Command{CommandName{name}, std::move(args)});
        app.session->call(now);
    }
    void run(std::chrono::milliseconds d) {
        for (auto end = now + d; now < end;) app.session->call(now += std::chrono::microseconds(16'667));
    }
    Value state(const TopicName& t) const {
        const PayloadPtr* p = mirror.state(t);
        return p && *p ? (*p)->toValue() : Value();
    }
    TIAppPart& part() { return *app.part; }
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
    CHECK(r.notices.empty());
}

TEST(lib15_ec4_unknown_model_and_preset_are_notices) {
    WebRig r;
    r.send("start", Value::object({{"model", "TI-99"}, {"preset", "../one.ti59"}}));
    r.run(100ms);
    CHECK(r.part().model() == Model::TI59);
    CHECK_EQ(r.notices.size(), size_t{2});
    if (r.notices.size() == 2) {
        CHECK(r.notices[0].find("has no model TI-99") != std::string::npos);
        CHECK(r.notices[1].find("../one.ti59 is not a bundled preset") != std::string::npos);
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
                          &topics::kWebCard, &topics::kWebLog, &topics::kWebSettings, &topics::kWebTraces})
        CHECK(r.mirror.state(*t) != nullptr);
    const Value display = r.state(topics::kWebDisplay);
    CHECK(display.bytes() && display.bytes()->size() == 36 * sizeof(float));
    const Value model = r.state(topics::kWebModel);
    CHECK(model.find("moduleID") && model.find("moduleID")->string() && *model.find("moduleID")->string() == "ML");
}
