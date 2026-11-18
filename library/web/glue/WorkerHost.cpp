#include "web/glue/WorkerHost.hpp"
#include "web/glue/MessageCodec.hpp"
#include <emscripten.h>
#include <exception>
#include <filesystem>
#include <fstream>
#include <system_error>

using emscripten::val;
namespace s = calcu::sessions;

namespace calcu::web {

const char* storageModeName(StorageMode m) {
    switch (m) {
    case StorageMode::Kept: return "kept";
    case StorageMode::SecondCopy: return "second-copy";
    case StorageMode::Unreadable: return "unreadable";
    case StorageMode::Unavailable: return "unavailable";
    }
    return "kept";
}

namespace {

StorageMode storageModeNamed(const std::string& name) {
    if (name == "second-copy") return StorageMode::SecondCopy;
    if (name == "unreadable") return StorageMode::Unreadable;
    if (name == "unavailable") return StorageMode::Unavailable;
    return StorageMode::Kept;
}

std::string text(const val& o, const char* key) {
    if (o.isUndefined() || o.isNull()) return {};
    const val v = o[key];
    return v.isString() ? v.as<std::string>() : std::string();
}

struct StartState final : s::PayloadOf<StartState> {
    static constexpr s::PayloadKind kKind{"web.start"};
    StartState(std::string st, std::string f) : state(std::move(st)), failure(std::move(f)) {}
    std::string state, failure;
    bool same(const StartState& o) const { return state == o.state && failure == o.failure; }
    s::Value toValue() const override { return s::Value::object({{"state", state}, {"failure", failure}}); }
};

struct StorageState final : s::PayloadOf<StorageState> {
    static constexpr s::PayloadKind kKind{"web.storage"};
    StorageState(StorageMode m, std::string st) : mode(m), sentence(std::move(st)) {}
    StorageMode mode;
    std::string sentence;
    bool same(const StorageState& o) const { return mode == o.mode && sentence == o.sentence; }
    s::Value toValue() const override {
        return s::Value::object({{"mode", storageModeName(mode)}, {"sentence", sentence}});
    }
};

} // namespace

// The shell's topics: declared in the session like any
// feature's; the storage sentence also goes once into the notices.
class WorkerHost::ShellPart final : public s::SessionPart {
public:
    void attach(s::SessionSetup& setup) override {
        start = &setup.stateTopic(kStartTopic);
        storage = &setup.stateTopic(kStorageTopic);
    }
    void afterCall(s::SessionContext& c, pacing::WallTime) override {
        if (!pendingNotice.empty()) c.notice(std::move(pendingNotice));
        pendingNotice.clear();
    }
    s::StateTopic* start = nullptr;
    s::StateTopic* storage = nullptr;
    std::string pendingNotice;
};

WorkerHost::WorkerHost(const AppSessionFactory& factory, val start)
    : part_(std::make_unique<ShellPart>()), post_(start["post"]) {
    start_.product = text(start, "product");
    const val dirs = start["dirs"];
    start_.dirs.frontend = settings::FrontendKind::Web;
    start_.dirs.settingsBase = text(dirs, "settingsBase");
    start_.dirs.saveFolder = text(dirs, "saveFolder");
    start_.dirs.home = text(dirs, "home");
    const val storage = start["storage"];
    start_.storage = storageModeNamed(text(storage, "mode"));
    start_.storageSentence = text(storage, "sentence");
    start_.shellPart = part_.get();
    try {
        session_ = factory(start_);
    } catch (const std::exception& e) {
        failure_ = e.what();
    } catch (...) {
        failure_ = "The session could not be built.";
    }
    if (failure_.empty() && !session_) failure_ = "The session could not be built.";
    if (!failure_.empty()) return;
    part_->storage->set(std::make_shared<StorageState>(start_.storage, start_.storageSentence));
    part_->start->set(std::make_shared<StartState>("running", ""));
    part_->pendingNotice = start_.storageSentence;
}

WorkerHost::~WorkerHost() = default;

// A message to the page; a reply carries the page's id of its call (0: the
// page wanted no reply, it keeps the session's).
void WorkerHost::post(const s::Message& m, double pageCall) {
    val transfer = val::array();
    val message = encode(m, transfer);
    if (const auto* ask = std::get_if<s::ServiceAsk>(&m); ask && ask->service == s::HostService::FilePicker)
        filePickerAsks_.insert(ask->ask);
    if (pageCall != 0 && std::holds_alternative<s::ReplyMessage>(m)) message.set("call", pageCall);
    post_(message, transfer);
}

void WorkerHost::receive(val message) {
    if (!session_) return;
    const std::string kind = text(message, "kind");
    const val callVal = message["call"];
    const double pageCall = callVal.isNumber() ? callVal.as<double>() : 0;

    if (kind == "listen") {
        if (listener_.listening()) return;
        s::HostServices services;
        const val names = message["args"].isUndefined() ? val::undefined() : message["args"]["services"];
        if (!names.isUndefined() && !names.isNull()) {
            const int n = names["length"].as<int>();
            for (int i = 0; i < n; ++i)
                if (names[i].isString())
                    if (const auto svc = sessions::serviceNamed(names[i].as<std::string>())) services.offered.insert(*svc);
        }
        listener_.onMessage = [this](const s::Message& m) { post(m); };
        listener_.listen(*session_, std::move(services));
        return;
    }
    if (!listener_.listening()) return;
    if (kind == "command" || kind == "request") {
        // One reply per call, at once or at the operation's end.
        auto reply = [this, pageCall](const s::ReplyMessage& r) { post(r, pageCall); };
        if (kind == "command") listener_.send(s::CommandName{text(message, "name")}, fromJs(message["args"]), reply);
        else listener_.ask(s::RequestName{text(message, "name")}, fromJs(message["args"]), reply);
        return;
    }
    if (kind == "answer") {
        const auto ask = static_cast<s::CallId>(pageCall);
        if (filePickerAsks_.erase(ask)) {
            answerFilePicker(ask, message);
            return;
        }
        s::ServiceAnswer a;
        a.given = message["done"].isTrue();
        a.reason = text(message, "reason");
        a.result = fromJs(message["value"]);
        session_->answer(ask, std::move(a));
        return;
    }
    // A message the glue cannot decode: refused with a reply when it waits for one.
    if (pageCall != 0) {
        val reply = val::object();
        reply.set("kind", "reply");
        reply.set("call", pageCall);
        reply.set("done", false);
        reply.set("reason", "Unknown message: " + kind + ".");
        reply.set("value", val::null());
        post_(reply, val::array());
    }
}

// The picked files' bytes go to the uploads folder; the session gets their
// paths.
void WorkerHost::answerFilePicker(s::CallId ask, val message) {
    s::ServiceAnswer a;
    a.given = message["done"].isTrue();
    a.reason = text(message, "reason");
    if (a.given) {
        s::Value::Array paths;
        const s::Value picked = fromJs(message["value"]);
        std::error_code ec;
        std::filesystem::create_directories(start_.uploadsDir, ec);
        if (const auto* files = picked.elements())
            for (const s::Value& f : *files) {
                const s::Value* name = f.find("name");
                const s::Value* bytes = f.find("bytes");
                if (!name || !name->string() || !bytes || !bytes->bytes()) continue;
                const std::string file = std::filesystem::path(*name->string()).filename().string();
                const std::string path = start_.uploadsDir + "/" + file;
                std::ofstream out(path, std::ios::binary | std::ios::trunc);
                out.write(reinterpret_cast<const char*>(bytes->bytes()->data()),
                          static_cast<std::streamsize>(bytes->bytes()->size()));
                if (out) paths.push_back(s::Value(path));
            }
        a.result = s::Value(std::move(paths));
    }
    session_->answer(ask, std::move(a));
}

pacing::WallTime wallTimeNow() {
    return std::chrono::duration_cast<pacing::WallTime>(std::chrono::duration<double, std::milli>(emscripten_get_now()));
}

void WorkerHost::call() {
    if (session_) session_->call(wallTimeNow());
}

} // namespace calcu::web
