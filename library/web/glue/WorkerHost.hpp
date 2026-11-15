#pragma once
#include "sessions/AppSession.hpp"
#include "sessions/Messages.hpp"
#include "sessions/Payload.hpp"
#include "sessions/SessionPart.hpp"
#include "settings/HostDirs.hpp"
#include <emscripten/val.h>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

// The Web Worker's host of one app session (LIB-15 design §3.2, §3.6; spec
// AC-1, AC-2, AC-4): it builds the session through the product's factory,
// listens to it with the services the page states, passes the page's
// messages in and the session's out, and makes the host call. It only
// translates (frontend.md I-3; ADR 0014): it reads no topic and decides no
// rule. Single-threaded, as the worker is (ADR 0009).

namespace calcu::web {

// The persistent file system's mode (design §3.3), from storage.js.
enum class StorageMode { Kept, SecondCopy, Unreadable, Unavailable };
const char* storageModeName(StorageMode);

inline const sessions::TopicName kStartTopic{"web.start"};       // {state: "running" | "failed", failure}
inline const sessions::TopicName kStorageTopic{"web.storage"};   // {mode, sentence}

// What the worker passes in (design §3.2).
struct WebStart {
    std::string product;                 // as the glossary writes it: "Calc-U-1600"
    settings::HostDirs dirs;             // FrontendKind::Web, settingsBase, saveFolder (§3.3)
    std::string bundleDir = "/bundle", presetsDir = "/presets", uploadsDir = "/uploads";
    StorageMode storage = StorageMode::Kept;
    std::string storageSentence;         // sentences.js; empty when kept
    // The shell's own part (web.start, web.storage): the factory lists it
    // first among the session's parts.
    sessions::SessionPart* shellPart = nullptr;
};

// The session and what it needs to live: the product's parts and settings
// are kept in `keep`, which is destroyed after the session.
struct WebSession {
    std::shared_ptr<void> keep;
    std::unique_ptr<sessions::AppSession> session;
};
using AppSessionFactory = std::function<WebSession(const WebStart&)>;   // Hosting::Paced

// The worker's wall clock (performance.now()), the one the host call passes:
// a factory gives it to AppSessionSetup::clock.
pacing::WallTime wallTimeNow();

class WorkerHost final : public sessions::MessageSink {
public:
    // `start`: {product, dirs: {settingsBase, saveFolder, home}, storage:
    // {mode, sentence}, post: function(message, transfer)}.
    WorkerHost(const AppSessionFactory&, emscripten::val start);
    ~WorkerHost();
    WorkerHost(const WorkerHost&) = delete;
    WorkerHost& operator=(const WorkerHost&) = delete;

    // Why the session could not be built; empty when it runs.
    std::string failure() const { return failure_; }
    void receive(emscripten::val message);   // listen, command, request, answer
    void call();                             // the host call, with the wall time now
    void deliver(const sessions::Message&) override;

private:
    class ShellPart;
    struct Held {
        emscripten::val message, transfer;
    };

    void post(emscripten::val message, emscripten::val transfer);
    void flushHeld(sessions::CallId sessionCall, double pageCall);
    void answerFilePicker(sessions::CallId ask, emscripten::val message);

    std::unique_ptr<ShellPart> part_;
    WebStart start_;
    WebSession app_;
    emscripten::val post_;
    std::string failure_;
    sessions::ListenerId listener_ = 0;
    bool listening_ = false;
    bool holding_ = false;
    std::vector<Held> held_;
    std::map<sessions::CallId, double> pageCalls_;   // the session's call id → the page's
    std::set<sessions::CallId> filePickerAsks_;
};

} // namespace calcu::web
