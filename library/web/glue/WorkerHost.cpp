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

// The shell's topics (design §9 N2): declared in the session like any
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
        app_ = factory(start_);
    } catch (const std::exception& e) {
        failure_ = e.what();
    } catch (...) {
        failure_ = "The session could not be built.";
    }
    if (failure_.empty() && !app_.session) failure_ = "The session could not be built.";
    if (!failure_.empty()) return;
    part_->storage->set(std::make_shared<StorageState>(start_.storage, start_.storageSentence));
    part_->start->set(std::make_shared<StartState>("running", ""));
    part_->pendingNotice = start_.storageSentence;
}

WorkerHost::~WorkerHost() {
    if (app_.session && listening_) app_.session->stopListening(listener_);
}

void WorkerHost::post(val message, val transfer) { post_(message, transfer); }

void WorkerHost::deliver(const s::Message& m) {
    val transfer = val::array();
    val message = encode(m, transfer);
    if (const auto* ask = std::get_if<s::ServiceAsk>(&m); ask && ask->service == s::HostService::FilePicker)
        filePickerAsks_.insert(ask->ask);
    if (const auto* reply = std::get_if<s::ReplyMessage>(&m)) {
        if (holding_) {   // the page's id is known only when send() returns
            held_.push_back(Held{message, transfer});
            return;
        }
        const auto it = pageCalls_.find(reply->call);
        if (it != pageCalls_.end()) {
            message.set("call", it->second);
            pageCalls_.erase(it);   // one reply per call (LIB-3 AC-8, AC-13)
        }
    }
    if (holding_) held_.push_back(Held{message, transfer});
    else post(message, transfer);
}

void WorkerHost::flushHeld(s::CallId sessionCall, double pageCall) {
    holding_ = false;
    bool replied = false;
    std::vector<Held> held;
    held.swap(held_);
    for (Held& h : held) {
        if (h.message["kind"].as<std::string>() == "reply") {
            const auto call = static_cast<s::CallId>(h.message["call"].as<double>());
            if (call == sessionCall) {
                h.message.set("call", pageCall);
                replied = true;
            } else if (const auto it = pageCalls_.find(call); it != pageCalls_.end()) {
                h.message.set("call", it->second);
                pageCalls_.erase(it);
            }
        }
        post(h.message, h.transfer);
    }
    if (!replied && pageCall != 0) pageCalls_[sessionCall] = pageCall;   // an operation replies when it ends (0: no reply wanted)
}

void WorkerHost::receive(val message) {
    if (!app_.session) return;
    const std::string kind = text(message, "kind");
    const val callVal = message["call"];
    const double pageCall = callVal.isNumber() ? callVal.as<double>() : 0;

    if (kind == "listen") {
        if (listening_) return;
        s::HostServices services;
        const val names = message["args"].isUndefined() ? val::undefined() : message["args"]["services"];
        if (!names.isUndefined() && !names.isNull()) {
            const int n = names["length"].as<int>();
            for (int i = 0; i < n; ++i)
                if (names[i].isString())
                    if (const auto svc = serviceNamed(names[i].as<std::string>())) services.offered.insert(*svc);
        }
        listening_ = true;
        listener_ = app_.session->listen(*this, std::move(services));
        return;
    }
    if (!listening_) return;
    if (kind == "command" || kind == "request") {
        holding_ = true;
        const s::CallId id = kind == "command"
                                 ? app_.session->send(listener_, s::Command{s::CommandName{text(message, "name")},
                                                                            fromJs(message["args"])})
                                 : app_.session->ask(listener_, s::Request{s::RequestName{text(message, "name")},
                                                                           fromJs(message["args"])});
        flushHeld(id, pageCall);
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
        app_.session->answer(ask, std::move(a));
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
        post(reply, val::array());
    }
}

// The picked files' bytes go to the uploads folder; the session gets their
// paths (design §3.1, §9 N4).
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
    app_.session->answer(ask, std::move(a));
}

pacing::WallTime wallTimeNow() {
    return std::chrono::duration_cast<pacing::WallTime>(std::chrono::duration<double, std::milli>(emscripten_get_now()));
}

void WorkerHost::call() {
    if (app_.session) app_.session->call(wallTimeNow());
}

} // namespace calcu::web
