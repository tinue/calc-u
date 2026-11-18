// TIWebPart: what the Web page draws, built and tested natively as the Web
// engine (frontends/web/bindings.cpp) builds the session: the shell's start
// (the model by its name, a bundled preset by its path; unknown ones), the
// settings in Calc-U-59.ini, cards in the save folder, a dropped file, and
// the topics the page reads.

#include "TestHarness.hpp"
#include "../app/TIMagneticCardKind.hpp"
#include "control/ControlState.hpp"
#include "debugger/RunState.hpp"
#include "media/MediaTopics.hpp"
#include "typing/TypingTopics.hpp"
#include "opening/OpenRequest.hpp"
#include "SessionRig.hpp"
#include "../app/TIWebPart.hpp"
#include "settings/AtomicFile.hpp"
#include "settings/HostDirs.hpp"
#include "settings/PlatformBase.hpp"
#include "settings/Paths.hpp"
#include "settings/SettingsFile.hpp"
#include "settings/Startup.hpp"
#include "shell/SettingsPart.hpp"
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
    std::unique_ptr<calcu::settings::Paths> paths;
    std::unique_ptr<calcu::shell::SettingsPart> settingsPart;
    std::unique_ptr<TIWebPart> web;
    WebPieces() {
        dirs.frontend = calcu::settings::FrontendKind::Web;
        dirs.settingsBase = (webFolder.path() / "settings").string();
        dirs.saveFolder = (webFolder.path() / "Calc-U-59").string();
        fs::create_directories(dirs.saveFolder);
        webFolder.write("presets/one.ti59", "format-version: 2\nmodel: TI-58\ndata:\n  05: 3.25\n");
        file = std::make_unique<calcu::settings::SettingsFile>(calcu::settings::SettingsFile::open(
            settings::settingsDecl(), calcu::settings::settingsFilePath(settings::settingsDecl(), dirs)));
        paths = std::make_unique<calcu::settings::Paths>(*file, dirs);
        calcu::shell::SettingsPartSetup ss;   // the shell's settings dialog
        ss.file = file.get();
        ss.paths = paths.get();
        settingsPart = std::make_unique<calcu::shell::SettingsPart>(std::move(ss));
        web = std::make_unique<TIWebPart>();
    }
};

// The Web engine's session, natively: the Web part behind the product's.
struct WebRig : WebPieces, testroms::SessionRig {
    explicit WebRig(bool started = true)
        : SessionRig(testroms::RigOptions{.model = settings::modelNamed(calcu::settings::startupModel(*file)).value_or(cu59::Model::TI59),
                                          .before = {settingsPart.get()}, .after = {web.get()},
                                          .saveFolder = dirs.saveFolder, .noTraceFolder = true, .settings = file.get(),
                                          .examplesFolder = (webFolder.path() / "presets").string(), .start = false}) {
        web->bind(part());
        if (started) {   // the page's start (the library's start)
            post(CommandName{"start"}, Value::object({}));
            until([this] { return session().machineSession().machine() && !busy(); });
        }
    }
    void send(const char* name, Value args = Value::object({})) {
        post(CommandName{name}, std::move(args));
        session().call(now);
    }
    void run(std::chrono::milliseconds d) { runFor(d); }
    Value state(const TopicName& t) const {
        const PayloadPtr* p = fe.mirror.state(t);
        return p && *p ? (*p)->toValue() : Value();
    }
    Value ask(const char* name, Value args = Value::object({})) {
        const CallId id = session().ask(me, Request{RequestName{name}, std::move(args)});
        const ReplyMessage* r = fe.reply(id);
        return r && r->done && r->result ? r->result->toValue() : Value();
    }
};

} // namespace

// The page's start is the library's `start {model, preset}`, the model by
// its name and the preset by its bundle path.
TEST(web_start_takes_the_model_by_name_and_a_bundled_preset) {
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

// A model or a preset the product does not have is a notice;
// the start goes on with the startup model's built-in default.
TEST(web_unknown_model_and_preset_are_notices) {
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

// Through the shell's SettingsPart (its dialog): a change is written into
// Calc-U-59.ini at once.
TEST(web_settings_in_the_ini_file) {
    WebRig r;
    r.send("set-setting", Value::object({{"section", "startup"}, {"key", "model"}, {"value", "TI-58C"}}));
    r.send("set-setting", Value::object({{"section", "trace"}, {"key", "max-file-size-mb"}, {"value", 7}}));
    r.send("set-setting", Value::object({{"section", "keyboard"}, {"key", "feedback"}, {"value", "click"}}));
    r.run(50ms);
    const auto ini = calcu::settings::readWholeFile(
        calcu::settings::settingsFilePath(settings::settingsDecl(), r.dirs)).value_or("");
    CHECK(ini.find("model = TI-58C") != std::string::npos);
    CHECK(ini.find("max-file-size-mb = 7") != std::string::npos);
    CHECK(ini.find("feedback = click") != std::string::npos);
    CHECK(r.fe.mirror.state(TopicName{"settings"}) != nullptr);
}

// The product's shell data for the page, per layout.
TEST(web_shell_product_per_layout) {
    WebRig r;
    const Value wide = r.ask("c59.shell-product", Value::object({{"layout", "wide"}, {"version", "3.0.0"}}));
    const Value* product = wide.find("product");
    CHECK(product && product->find("name") && *product->find("name")->string() == "Calc-U-59");
    CHECK(product && product->find("aboutLinks") && product->find("aboutLinks")->elements()->size() == 3);
    CHECK(product && product->find("copyScreen") && !*product->find("copyScreen")->boolean());
    const std::string wideSpec = toText(*wide.find("settingsSpec"));
    CHECK(wideSpec.find("max-file-size-mb") != std::string::npos);
    CHECK(wideSpec.find("Version 3.0.0") != std::string::npos);
    CHECK(wideSpec.find("feedback") == std::string::npos);
    const std::string portrait = toText(*r.ask("c59.shell-product", Value::object({{"layout", "portrait"}, {"version", "3.0.0"}}))
                                             .find("settingsSpec"));
    CHECK(portrait.find("feedback") != std::string::npos);
    CHECK(portrait.find("Click Sound") != std::string::npos);
    CHECK(portrait.find("max-file-size-mb") == std::string::npos);
}

// The faceplate of a model, the key rectangles and the card corners;
// the heatmap grid.
TEST(web_faceplate_map_and_heatmap_grid) {
    WebRig r;
    const Value m = r.ask("c59.faceplate-map", Value::object({{"model", "TI-59"}}));
    CHECK(m.find("map") && m.find("map")->find("keys")->elements()->size() == 45);
    CHECK(m.find("map") && m.find("map")->find("display"));
    CHECK(m.find("card") && m.find("canvas"));
    CHECK(m.find("keyRects") && m.find("keyRects")->elements()->size() == 45);
    CHECK(m.find("cardCorners") && m.find("cardCorners")->elements()->size() == 2);
    const Value h = r.ask("c59.heatmap-grid");
    CHECK(h.find("cols") && *h.find("cols")->integer() == 80);
    CHECK(h.find("colors") && h.find("colors")->elements()->size() == 6);
}

// The cards are the library's media of the persistent save folder; a
// dropped card file is imported into it and fitted.
TEST(web_cards_are_files_of_the_save_folder) {
    WebRig r;
    r.run(1s);
    r.send("media.start-new", Value::object({{"place", "card-reader"}, {"template", "implicit:Fresh card"}, {"name", "Mine"}}));
    CHECK(r.part().cardFitted());
    auto card = std::make_shared<TIMagneticCardData>();
    const std::string text = writeMagneticCardFile(*card, "Dropped", "", false, std::nullopt, "");
    auto upload = [&](const std::string& name) {   // the page's drop: the library's open, an upload
        calcu::opening::OpenRequest q;
        calcu::opening::OpenedFile f;
        f.name = name;
        f.bytes = std::vector<std::uint8_t>(text.begin(), text.end());
        q.files = {f};
        q.upload = true;
        r.send("open", q.toValue());
        r.run(200ms);
    };
    upload("Dropped.magnetcard.yaml");
    CHECK(fs::exists(fs::path(r.part().files().saveFolder()) / "Dropped.magnetcard.yaml"));
    const auto* media = r.part().mediaPart().bindingOf("card-reader");
    CHECK(media && media->name == "Dropped");
    upload("old.u59");
    CHECK(!fs::exists(fs::path(r.part().files().saveFolder()) / "old.u59"));   // a 2.3.0 card file (.u59) is ignored
}

TEST(web_the_page_topics_are_there) {
    WebRig r;
    r.run(1s);
    for (const auto* t : {&topics::kWebDisplay, &topics::kWebCueCard, &calcu::control::kControlStateTopic, &calcu::media::kMediaTopic,
                          &topics::kDebugView, &calcu::debugger::kRunStateTopic, &calcu::typing::kHeldKeysTopic,
                          &kBusyTopic})
        CHECK(r.fe.mirror.state(*t) != nullptr);
    // The page reads run-state, busy and held-keys itself; the PC-100C
    // and its TRACE from the control state (no c59.printer topic).
    CHECK(r.fe.mirror.state(TopicName{"c59.web.printer"}) == nullptr);
    CHECK(r.fe.mirror.state(TopicName{"c59.printer"}) == nullptr);
    CHECK(r.fe.mirror.state(TopicName{"c59.web.run-state"}) == nullptr);
    CHECK(r.fe.mirror.state(TopicName{"control-state"}) != nullptr);   // the model and the module
    CHECK(r.fe.mirror.state(TopicName{"c59.web.model"}) == nullptr);   // none: the control state has the model
    const Value display = r.state(topics::kWebDisplay);
    CHECK(display.bytes() && display.bytes()->size() == 36 * sizeof(float));
}

// The page's key grid and the host keys go
// through the library's typing; the key the page draws pressed is the one
// live input holds (the library's held-keys, its last `live` key). The key
// grid sends the library's screen-key-down/-up (CalculatorFace.jsx); there
// is no test-only `c59.web.key-button-*`.
TEST(web_keys_through_the_typing_engine) {
    WebRig r;
    r.run(1s);
    auto held = [&] {
        const Value v = r.state(calcu::typing::kHeldKeysTopic);
        const Value* live = v.find("live");
        if (!live || !live->elements()) return std::string("?");
        return live->elements()->empty() ? std::string() : *live->elements()->back().string();
    };
    for (const char* gone : {"c59.web.key-button-down", "c59.web.key-button-up"}) {
        const calcu::sessions::CallId id = r.post(CommandName{gone}, Value::object({{"row", 6}, {"col", 2}}));
        r.session().call(r.now);
        const auto* reply = r.reply(id);
        CHECK(reply != nullptr && !reply->done);   // no such command
    }
    CHECK_EQ(held(), std::string());
    r.send("screen-key-down", Value::object({{"key", "5"}, {"pointer", 2}}));
    CHECK_EQ(held(), std::string("5"));
    r.send("screen-key-up", Value::object({{"key", "5"}, {"pointer", 2}}));
    CHECK_EQ(held(), std::string());
    r.run(300ms);
    CHECK_EQ(testroms::displayText(r.machine()), std::string("5"));
    calcu::typing::HostKeyEvent e;
    e.physical = "KeyQ";
    e.carried = 'a';   // AZERTY: the key at Q carries A
    e.produced = 'a';
    r.send("host-key-down", calcu::typing::toValue(e));
    CHECK_EQ(held(), std::string("A"));
    r.send("focus-lost");   // released after its minimum
    r.run(300ms);
    CHECK_EQ(held(), std::string());
    CHECK(r.part().typing().engine().down().empty());
}
