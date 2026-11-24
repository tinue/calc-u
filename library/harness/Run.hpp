#pragma once
#include "applog/AppLog.hpp"
#include "harness/Commands.hpp"
#include "harness/HarnessHost.hpp"
#include "harness/Product.hpp"
#include "harness/ServeAddOn.hpp"
#include "parts/LibraryParts.hpp"
#include "sessions/AppSession.hpp"
#include "settings/HostDirs.hpp"
#include "settings/Paths.hpp"
#include "settings/SettingsFile.hpp"
#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// One run of the harness: its folders and settings (RunSetup) and its session and
// commands (Server). The serve loop works on a line reader and a writer;
// no stdio inside (the CLI's main() hands them in).

namespace calcu::harness {

// A local date and time, in no time zone.
struct LocalDateTime {
    int year = 2000, month = 1, day = 1, hour = 0, minute = 0, second = 0;
    bool operator==(const LocalDateTime&) const = default;
};
// `YYYY-MM-DDThh:mm:ss`, or std::nullopt.
std::optional<LocalDateTime> parseDateTime(std::string_view);

enum class ClockMode { Fixed, Given, Host };

struct RunOptions {
    bool json = false;                          // --json
    std::optional<std::string> settingsFile;    // open it; else defaults, nothing read or written
    std::optional<std::string> saveFolder;      // else an empty one made for the run
    std::optional<std::string> outputFolder;    // traces and screenshots; else the save folder
    ClockMode clock = ClockMode::Fixed;         // 2000-01-01T00:00:00 unless named
    LocalDateTime given;                        // ClockMode::Given
    // The app log (how the run went, not the calculator's `log`): its lines
    // at this level and above go to `appLogOut` ("app-log warn: <text>");
    // without `appLogOut` they are dropped.
    applog::AppLogLevel appLogLevel = applog::AppLogLevel::Warn;   // --app-log-level
    std::function<void(const std::string&)> appLogOut;
};

// The run folder, the settings and the clock.
class RunSetup {
public:
    static std::unique_ptr<RunSetup> make(const RunOptions&, const settings::ProductDecl&, std::string* error);
    ~RunSetup();   // removes the run folder
    RunSetup(const RunSetup&) = delete;
    RunSetup& operator=(const RunSetup&) = delete;

    const RunOptions& options() const { return options_; }
    const std::string& saveFolder() const { return saveFolder_; }
    const std::string& traceFolder() const { return outputFolder_; }
    const std::string& screenshotFolder() const { return outputFolder_; }
    const std::string& runFolder() const { return runFolder_; }
    const settings::SettingsFile& settings() const { return *settings_; }
    settings::SettingsFile& settings() { return *settings_; }   // the startup part writes last-model
    // The host folders a feature hands to settings::Paths: the save folder,
    // the output folder for traces and screenshots, the rest inside the
    // run folder, so nothing of the user's is reached
    // (FrontendKind::Given), and the places on them and the run's settings.
    const settings::HostDirs& hostDirs() const { return dirs_; }
    const settings::Paths& paths() const { return *paths_; }
    std::function<std::chrono::system_clock::time_point()> hostTime() const;
    bool clockFollowsHost() const;
    // The text with the save folder's path written `<save folder>`, and the
    // run folder's `<run folder>` (the path appears in no reply).
    std::string scrub(std::string) const;

private:
    RunSetup() = default;
    RunOptions options_;
    std::string runFolder_, saveFolder_, outputFolder_;
    std::unique_ptr<settings::SettingsFile> settings_;
    settings::HostDirs dirs_;
    std::unique_ptr<settings::Paths> paths_;
};

// What carries the session's machine state for state-save and
// state-restore: a session part with the commands of the kind.
class MachineStatePart;

class Server {
public:
    // The ROM full set; then the startup part's start (`start`, the startup
    // model under the run's settings, to key readiness). Null with `error` set.
    static std::unique_ptr<Server> start(HarnessProduct&, const RunOptions&, std::string* error);
    ~Server();
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    struct Result {
        std::string text;   // the reply as written, its final newline included; empty after `quit`
        bool done = true;
        bool quit = false;
    };
    // One line: its reply (an empty line gets none).
    Result line(std::string_view);
    // One reply per line until `quit` or the end of the input.
    // Returns the exit status (0).
    int serve(const std::function<std::optional<std::string>()>& readLine,
              const std::function<void(const std::string&)>& write);
    // `serve` with a product's add-ons started
    // (with serveLoop()): standard input is read by a thread of its own; its
    // lines and the add-ons' tasks are taken in order on this thread; while
    // none waits and an add-on wants time, emulated time passes unpaced in
    // pieces of 1/60 s. Without add-ons it is serve().
    int serveWith(const std::vector<ServeAddOn*>& addOns, std::function<std::optional<std::string>()> readLine,
                  const std::function<void(const std::string&)>& write);
    // What an add-on gets at its start: the loop's inbox and its end.
    ServeLoop serveLoop();
    // The lines in order until the first refusal; 0 or 1.
    int oneShot(const std::vector<std::string>& lines, const std::function<void(const std::string&)>& write);
    // Completes the files of the run and removes the run folder.
    void finish();

    HarnessHost& host() { return *host_; }
    RunSetup& setup() { return *setup_; }
    CommandRegistry& commands() { return registry_; }
    // How often the harness's wall clock was read for a deadline:
    // it returns 0 every time, so nothing is paced by it.
    int wallClockReads() const { return *clockReads_; }

private:
    Server() = default;
    HarnessProduct* product_ = nullptr;
    std::unique_ptr<RunSetup> setup_;
    std::shared_ptr<int> clockReads_ = std::make_shared<int>(0);
    std::unique_ptr<applog::AppLog> appLog_;   // before the session, which writes to it
    std::unique_ptr<MachineStatePart> state_;
    std::unique_ptr<sessions::AppSession> session_;
    std::unique_ptr<HarnessHost> host_;
    CommandRegistry registry_;
    bool finished_ = false;
    struct Inbox;
    std::shared_ptr<Inbox> inbox_;
};

// The products of a build matched by name or product word, any case.
HarnessProduct* findProduct(const std::vector<std::unique_ptr<HarnessProduct>>&, std::string_view name);

} // namespace calcu::harness
