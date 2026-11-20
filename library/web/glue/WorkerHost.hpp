#pragma once
#include "sessions/AppSession.hpp"
#include "sessions/Listener.hpp"
#include "sessions/Messages.hpp"
#include "sessions/Payload.hpp"
#include "sessions/SessionPart.hpp"
#include "settings/HostDirs.hpp"
#include <emscripten/val.h>
#include <filesystem>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

// The Web Worker's host of one app session: it builds the session through the product's factory,
// listens to it with the services the page states, passes the page's
// messages in and the session's out, and makes the host call. It only
// translates: it reads no topic and decides no rule. Single-threaded, as the
// worker is.

namespace calcu::web {

// The persistent file system's mode, from storage.js.
enum class StorageMode { Kept, SecondCopy, Unreadable, Unavailable };

inline const sessions::TopicName kStartTopic{"web.start"};       // {state: "running" | "failed", failure}
inline const sessions::TopicName kStorageTopic{"web.storage"};   // {mode, sentence}

// What the worker passes in.
struct WebStart {
    std::string product;                 // as the glossary writes it: "Calc-U-1600"
    settings::HostDirs dirs;             // FrontendKind::Web, settingsBase, saveFolder
    std::string bundleDir = "/bundle", presetsDir = "/presets", uploadsDir = "/uploads";
    StorageMode storage = StorageMode::Kept;
    std::string storageSentence;         // sentences.js; empty when kept
    // The shell's own part (web.start, web.storage): the factory lists it
    // first among the session's parts.
    sessions::SessionPart* shellPart = nullptr;

    // The host folders of the product's places: `dirs`, home the save
    // folder's parent (/persist) when the page gives none.
    settings::HostDirs placesDirs() const {
        settings::HostDirs d = dirs;
        if (d.home.empty()) d.home = std::filesystem::path(d.saveFolder).parent_path().generic_string();
        return d;
    }
};

// The product's session (Hosting::Paced); it owns the product's parts and
// what they need (AppSessionSetup::owned).
using AppSessionFactory = std::function<std::unique_ptr<sessions::AppSession>(const WebStart&)>;

// The worker's wall clock (performance.now()), the one the host call passes:
// a factory gives it to AppSessionSetup::clock.
pacing::WallTime wallTimeNow();

class WorkerHost final {
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

private:
    class ShellPart;

    void post(const sessions::Message&, double pageCall = 0);
    void answerFilePicker(sessions::CallId ask, emscripten::val message);

    std::unique_ptr<ShellPart> part_;
    WebStart start_;
    std::unique_ptr<sessions::AppSession> session_;
    sessions::Listener listener_;   // stops listening before the session goes
    emscripten::val post_;
    std::string failure_;
    std::set<sessions::CallId> filePickerAsks_;
};

} // namespace calcu::web
