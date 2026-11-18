#pragma once
// A test frontend for Calc-U-1600 on the library's app session (LIB-3
// AC-29's kind): the product's session (makeSharpSession), a scripted wall
// clock, a Listener that keeps the topics, the replies by call, and helpers
// that run a command to its end one host call at a time. ROMs and the bundled cards are
// copied into a temporary bundle folder of the fixture's own (library/testing/
// TempFolder: a unique name, so parallel runs do not collide); nothing of the
// user's is touched.

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Connector/ExpansionCard.hpp"
#include "TempFolder.hpp"
#include "TestFrontend.hpp"
#include "TestSettings.hpp"
#include "app/SharpSession.hpp"
#include "media/MediaTopics.hpp"
#include "presets/PresetsPart.hpp"
#include "startup/StartupPart.hpp"
#include "sessions/Listener.hpp"
#include "sessions/LibraryTopics.hpp"
#include "typing/HostKey.hpp"
#include "typing/TypingTopics.hpp"

namespace sessionfixture {

namespace fs = std::filesystem;
namespace sessions = calcu::sessions;

inline bool romsPresent() { return fullRomSetPresent(); }

struct SessionFixture {
    calcu::testing::TempFolder folder;
    fs::path root = folder.path();
    TestSettings testSettings;
    cu1600::Settings& settings = testSettings.settings;
    std::unique_ptr<cu1600::AppPaths> paths;
    calcu::pacing::WallTime now{1'000'000'000'000LL};
    sessions::ProductSession<cu1600::SharpAppPart> app;
    sessions::Listener listener;               // stops listening before the session goes
    std::vector<sessions::Message> messages;   // every message, in order
    std::map<sessions::CallId, sessions::ReplyMessage> replies;   // not taken yet
    std::chrono::system_clock::time_point fixedTime{};
    bool fixedHostTime = false;
    bool hostDirectories = true;
    sessions::HostServices services;
    int calls = 0;
    // Wall time each read of the clock takes: the host's work inside a call
    // (an operation at full speed runs until its allowance is used up).
    // Without it, a call at full speed (an operation) would never see its
    // allowance pass on this clock: the fixture sets 100 us by default.
    std::chrono::nanoseconds readCost{100'000};

    explicit SessionFixture(const char* name = "calcu-session-test") : folder(name) {
        const fs::path bundle = root / "bundle";
        fs::create_directories(bundle);
        for (const auto& dir : {fs::path("roms"), fs::path("frontends/qt6/resources/cards")})
            for (const auto& f : fs::directory_iterator(dir))
                if (f.is_regular_file()) fs::copy_file(f.path(), bundle / f.path().filename());
        // The host drive's own ROM, bundled beside the set (roms.md Q-1).
        const fs::path hostDrive = "firmware/pc1600-hostdrive/PC1600-P1-B7-HOSTDRIVE.bin";
        if (fs::exists(hostDrive)) fs::copy_file(hostDrive, bundle / hostDrive.filename());
        paths = std::make_unique<cu1600::AppPaths>(
            settings, testHostDirs((root / "home").generic_string()),
            cu1600::BundleDirs{bundle.generic_string(), fs::absolute("examples").generic_string()});
    }
    ~SessionFixture() {
        listener.stop();
        app = {};
    }

    // Builds the session (not yet started).
    void open() {
        cu1600::SharpSessionOptions o;
        o.clock = [this] {
            now += readCost;
            return now;
        };
        if (fixedHostTime) {
            const auto t = fixedTime;
            o.hostTime = [t] { return t; };
        }
        o.canMountHostFolder = hostDirectories;   // LIB-4 AC-18
        app = cu1600::makeSharpSession(settings, *paths, std::move(o));
        listener.onMessage = [this](const sessions::Message& m) {
            messages.push_back(m);
            if (const auto* r = std::get_if<sessions::ReplyMessage>(&m)) replies[r->call] = *r;
        };
        sessions::HostServices offered = services;
        if (hostDirectories) offered.offered.insert(sessions::HostService::FolderPicker);   // File ▸ Host Drive…
        listener.listen(*app.session, std::move(offered));
    }

    cu1600::SharpAppPart& part() { return *app.part; }
    sessions::AppSession& session() { return *app.session; }
    bool busy() const { return app.session->busy(); }

    // A command; its reply, once it came, from takeReply().
    sessions::CallId send(const sessions::CommandName& name, sessions::Value args = {}) {
        auto sent = listener.send(name, std::move(args));
        if (sent.reply) {
            messages.push_back(*sent.reply);
            replies[sent.call] = *sent.reply;
        }
        return sent.call;
    }
    std::optional<sessions::ReplyMessage> takeReply(sessions::CallId call) {
        const auto it = replies.find(call);
        if (it == replies.end()) return std::nullopt;
        sessions::ReplyMessage r = std::move(it->second);
        replies.erase(it);
        return r;
    }

    // One host call, `ms` of wall time after the last.
    void call(double ms = 1000.0 / 60) {
        now += std::chrono::nanoseconds(static_cast<std::int64_t>(ms * 1e6));
        session().call(now);
        ++calls;
    }
    // A command run to its end, one host call at a time.
    sessions::ReplyMessage run(const sessions::CommandName& name, sessions::Value args = {}) {
        return listener.runToEnd(name, std::move(args), [this] { call(); });
    }
    bool ok(const sessions::CommandName& name, sessions::Value args = {}) {
        const sessions::ReplyMessage r = run(name, std::move(args));
        if (!r.done) std::fprintf(stderr, "  %s refused: %s\n", name.value.c_str(), r.reason.c_str());
        return r.done;
    }
    // Opens the session and starts `model` (a settings name: "PC-1500A").
    bool start(const char* model = nullptr) {
        if (model) settings.setStartupModelPreference(model);
        if (!app.session) open();
        return ok(cu1600::commands::kStart);
    }
    // ---- LIB-18's machine control and LIB-6's load, as the frontends send them ----
    bool model(const char* name) { return ok(calcu::startup::kModelCommand, sessions::Value::object({{"name", name}})); }
    bool variant(const char* name) { return ok(calcu::startup::kVariantCommand, sessions::Value::object({{"name", name}})); }
    bool reset() { return ok(calcu::startup::kResetCommand); }
    bool coldBoot() { return ok(calcu::startup::kColdBootCommand); }
    // The place of a peripheral, as the menus attach it.
    static const char* placeOf(const std::string& peripheral) {
        if (peripheral == "CE-158") return "interface";
        if (peripheral == "host-drive") return "host-drive";
        return "printer";
    }
    bool attach(const char* peripheral, const char* variant = nullptr) {
        sessions::Value::Object o{{"place", sessions::Value(placeOf(peripheral))},
                                  {"peripheral", sessions::Value(peripheral)}};
        if (variant) o.push_back({"variant", sessions::Value(variant)});
        return ok(calcu::startup::kAttachCommand, sessions::Value(std::move(o)));
    }
    bool detach(const char* peripheral) {
        return ok(calcu::startup::kDetachCommand, sessions::Value::object({{"place", placeOf(peripheral)}}));
    }
    // The control bar's button: attach, or detach what is plugged in.
    bool toggle(const char* peripheral) {
        const calcu::machine::Machine* m = part().machine();
        bool in = false;
        if (m)
            for (const auto& e : m->plugged().byPlace) in = in || e.peripheral.value == peripheral;
        return in ? detach(peripheral) : attach(peripheral);
    }
    // ---- LIB-7's media: the topic `media` and its commands, as the control bar sends them ----
    std::shared_ptr<const calcu::media::MediaState> media() { return part().mediaPart().state(session().context()); }
    const calcu::media::PlaceState* mediaPlace(const std::string& place) {
        static std::shared_ptr<const calcu::media::MediaState> kept;   // the state the pointer is into
        kept = media();
        for (const auto& p : kept->places)
            if (p.place == place) return &p;
        return nullptr;
    }
    // The picker entry of `place` labelled `label` ("" the empty one); "?" when none.
    std::string entry(const std::string& place, const std::string& label) {
        const calcu::media::PlaceState* p = mediaPlace(place);
        if (!p) return "?";
        for (const auto& e : p->picker)
            if (e.label == label) return e.id;
        return "?";
    }
    sessions::ReplyMessage choose(const std::string& place, const std::string& label) {
        return run(calcu::media::kChooseCommand, sessions::Value::object({{"place", place}, {"entry", entry(place, label)}}));
    }
    sessions::ReplyMessage nameAndSave(const std::string& place, const std::string& name, bool protect = false,
                                       bool confirmed = false) {
        return run(calcu::media::kNameAndSaveCommand,
                   sessions::Value::object({{"place", place}, {"name", name}, {"protect", protect}, {"confirmed", confirmed}}));
    }
    // What the media part holds in `place` (nullptr: empty).
    const calcu::media::Binding* held(const std::string& place) { return part().mediaPart().bindingOf(place); }
    // The save folder and the tape folder of the session.
    std::string saveFolder() const { return paths->places().saveFolder(); }
    std::string cassetteFolder() const { return paths->places().cassetteFolder(); }
    sessions::ReplyMessage loadPreset(const std::string& ref) {
        return run(calcu::presets::kLoadPresetCommand, sessions::Value::object({{"ref", ref}}));
    }
    // `seconds` of wall time at regular speed.
    void runFor(double seconds) {
        const int n = static_cast<int>(seconds * 60);
        for (int i = 0; i < n; ++i) call();
    }
    // Until the typing engine has typed everything, at most `capSeconds`.
    bool runUntilTyped(double capSeconds) {
        const int n = static_cast<int>(capSeconds * 60);
        for (int i = 0; i < n; ++i) {
            if (part().typingPart().engine().idle()) return true;
            call();
        }
        return part().typingPart().engine().idle();
    }
    // ---- Typing (LIB-5): the typing part's commands, as a frontend sends them ----
    sessions::CallId screenDown(const std::string& key, int pointer = 0) {
        return send(calcu::typing::kScreenKeyDown, sessions::Value::object({{"key", key}, {"pointer", pointer}}));
    }
    sessions::CallId screenUp(const std::string& key, int pointer = 0) {
        return send(calcu::typing::kScreenKeyUp, sessions::Value::object({{"key", key}, {"pointer", pointer}}));
    }
    // A tap: down and up at once (held its minimum by the engine).
    void tap(const std::string& key) {
        screenDown(key);
        screenUp(key);
    }
    sessions::CallId hostDown(const calcu::typing::HostKeyEvent& e) { return send(calcu::typing::kHostKeyDown, calcu::typing::toValue(e)); }
    sessions::CallId hostUp(const std::string& physical) {
        return send(calcu::typing::kHostKeyUp, sessions::Value::object({{"physical", physical}}));
    }
    sessions::CallId paste(const std::string& text) { return send(calcu::typing::kPaste, sessions::Value::object({{"text", text}})); }
    // The debugger's line (MachineController::typeCommand): the text and ENTER.
    void typeLine(const std::string& line) {
        part().typingPart().engine().typeText(line, true, false, calcu::typing::Origin::Typed);
    }
    calcu::typing::TypingEngine& engine() { return part().typingPart().engine(); }
    std::string screen() {
        LcdText text;
        const calcu::machine::DisplayOutput* d = listener.display();
        return d && part().lcdText(*d, &text) ? text.text() : std::string();
    }
    template <class P> const P* state(const sessions::TopicName& t) const { return listener.state<P>(t); }
    std::vector<std::string> notices() { return sessions::test::noticeSentences(listener.mirror()); }
};

} // namespace sessionfixture
