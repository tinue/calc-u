#pragma once
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

// One run of the harness (LIB-11 design §3.4, §3.8; spec AC-2, AC-3, AC-6,
// AC-8, AC-21): its folders and settings (RunSetup) and its session and
// commands (Server). The serve loop works on a line reader and a writer;
// no stdio inside (the CLI's main() hands them in).

namespace calcu::harness {

// A local date and time, in no time zone (AC-21; D-4).
struct LocalDateTime {
    int year = 2000, month = 1, day = 1, hour = 0, minute = 0, second = 0;
    bool operator==(const LocalDateTime&) const = default;
};
// `YYYY-MM-DDThh:mm:ss`, or std::nullopt.
std::optional<LocalDateTime> parseDateTime(std::string_view);
// The instant at which the host's local date and time is `t`.
std::chrono::system_clock::time_point localInstant(const LocalDateTime& t);

enum class ClockMode { Fixed, Given, Host };

struct RunOptions {
    bool json = false;                          // AC-4
    std::optional<std::string> settingsFile;    // AC-8: open it; else defaults, nothing read or written
    std::optional<std::string> saveFolder;      // AC-8: else an empty one made for the run
    std::optional<std::string> outputFolder;    // AC-8: traces and screenshots; else the save folder
    ClockMode clock = ClockMode::Fixed;         // AC-21: 2000-01-01T00:00:00 unless named
    LocalDateTime given;                        // ClockMode::Given
};

// The run folder, the settings and the clock (AC-8, AC-21).
class RunSetup {
public:
    static std::unique_ptr<RunSetup> make(const RunOptions&, const settings::ProductDecl&, std::string* error);
    ~RunSetup();   // removes the run folder (AC-8)
    RunSetup(const RunSetup&) = delete;
    RunSetup& operator=(const RunSetup&) = delete;

    const RunOptions& options() const { return options_; }
    const std::string& saveFolder() const { return saveFolder_; }
    const std::string& traceFolder() const { return outputFolder_; }
    const std::string& screenshotFolder() const { return outputFolder_; }
    const std::string& runFolder() const { return runFolder_; }
    const settings::SettingsFile& settings() const { return *settings_; }
    settings::SettingsFile& settings() { return *settings_; }   // LIB-18 AC-9 writes last-model
    // The host folders a feature hands to settings::Paths: all inside the
    // run folder, so nothing of the user's is reached (LIB-9 AC-18; D-11).
    const settings::HostDirs& hostDirs() const { return dirs_; }
    std::function<std::chrono::system_clock::time_point()> hostTime() const;
    // The text with the save folder's path written `<save folder>`, and the
    // run folder's `<run folder>` (AC-8: the path appears in no reply).
    std::string scrub(std::string) const;
    // LIB-11 D-6, D-11: what every product's library parts take from the run:
    // its settings, typing in emulated time, its home, its folders for
    // screenshots (AC-8) and media (the save folder as the run passes it;
    // LIB-7 §3.11); the product adds its own data.
    parts::LibraryPartsSetup partsSetup(const machine::Catalogue&, startup::ProductMachines&,
                                        const roms::LookupResult&);

private:
    RunSetup() = default;
    RunOptions options_;
    std::string runFolder_, saveFolder_, outputFolder_;
    std::unique_ptr<settings::SettingsFile> settings_;
    settings::HostDirs dirs_;
    std::unique_ptr<settings::Paths> mediaPaths_;   // partsSetup
};

// What carries the session's machine state for state-save and
// state-restore (AC-24): a session part with the commands of the kind.
class MachineStatePart;

class Server {
public:
    // EC-1: the ROM full set; then LIB-18's start (`start`, the startup
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
    // AC-3, EC-3: one reply per line until `quit` or the end of the input.
    // Returns the exit status (0).
    int serve(const std::function<std::optional<std::string>()>& readLine,
              const std::function<void(const std::string&)>& write);
    // C1600-3 AC-22 (design §3.5): `serve` with a product's add-ons started
    // (with serveLoop()): standard input is read by a thread of its own; its
    // lines and the add-ons' tasks are taken in order on this thread; while
    // none waits and an add-on wants time, emulated time passes unpaced in
    // pieces of 1/60 s. Without add-ons it is serve().
    int serveWith(const std::vector<ServeAddOn*>& addOns, std::function<std::optional<std::string>()> readLine,
                  const std::function<void(const std::string&)>& write);
    // What an add-on gets at its start: the loop's inbox and its end.
    ServeLoop serveLoop();
    // AC-2: the lines in order until the first refusal; 0 or 1.
    int oneShot(const std::vector<std::string>& lines, const std::function<void(const std::string&)>& write);
    // Completes the files of the run and removes the run folder (EC-3, AC-8).
    void finish();

    HarnessHost& host() { return *host_; }
    RunSetup& setup() { return *setup_; }
    CommandRegistry& commands() { return registry_; }
    // How often the harness's wall clock was read for a deadline (AC-6):
    // it returns 0 every time, so nothing is paced by it.
    int wallClockReads() const { return *clockReads_; }

private:
    Server() = default;
    HarnessProduct* product_ = nullptr;
    std::unique_ptr<RunSetup> setup_;
    std::shared_ptr<int> clockReads_ = std::make_shared<int>(0);
    std::unique_ptr<MachineStatePart> state_;
    std::unique_ptr<sessions::AppSession> session_;
    std::unique_ptr<HarnessHost> host_;
    CommandRegistry registry_;
    bool finished_ = false;
    struct Inbox;
    std::shared_ptr<Inbox> inbox_;
};

// The products of a build matched by name or short form, any case (AC-1).
HarnessProduct* findProduct(const std::vector<std::unique_ptr<HarnessProduct>>&, std::string_view name);

} // namespace calcu::harness
