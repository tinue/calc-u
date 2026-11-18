#pragma once
#include "harness/Protocol.hpp"
#include "harness/Reply.hpp"
#include "presets/Vocabulary.hpp"
#include "sessions/Messages.hpp"
#include <functional>
#include <map>
#include <string>
#include <vector>

// Commands. The registry holds product commands under the product's word,
// common commands at the top level. Each area adds its commands from its
// own file (key, type: typing; preset, expect, screenshot, the bare wait:
// presets; ...).

namespace calcu::media {
class MediaPart;
}
namespace calcu::parts {
class LibraryParts;
}

namespace calcu::harness {

class HarnessHost;
class HarnessProduct;

struct CommandContext {
    HarnessHost& host;
    const CommandLine& line;
    bool quit = false;   // set by `quit`: the run ends after this reply
};

using Handler = std::function<Reply(CommandContext&)>;

class CommandRegistry {
public:
    void add(std::string name, Handler, std::string usage);   // a common command
    void addProduct(std::string productWord, std::string name, Handler, std::string usage);
    const Handler* find(const std::vector<std::string>& path) const;
    bool isProductWord(const std::string& word) const;
    bool isCommand(const std::string& name) const;
    std::vector<std::string> usage() const;   // for --help, in the order added

private:
    struct Entry {
        Handler handler;
        std::string usage;
    };
    std::map<std::string, Entry> common_;
    std::map<std::string, std::map<std::string, Entry>> product_;
    std::vector<std::string> usage_;
};

// The first commands: wait,
// key-down, key-up, display-text, display, state, capabilities, ticks, audio,
// state-save, state-restore, quit.
void addFirstCommands(CommandRegistry&);
// key, type (TypingCommands.cpp); called by addFirstCommands.
void addTypingCommands(CommandRegistry&);

// model, variant, attach, detach, reset, cold-boot and power-cycle at the
// top level, each sent as the session command of the StartupPart with the
// reply of its operation.
void addStartupCommands(CommandRegistry&);

// preset, check-preset, preset-list, full-speed,
// regular-speed, set-control, and every step of the vocabulary with
// StepDef::command (elapsed-time, expect, not-expect, screenshot, sync-clock,
// a product's printer-trace; the media steps), each sent as
// `run-step`. The bare `wait` is FirstCommands.cpp's.
// Null: for the usage text (--help), the steps every product has.
void addPresetCommands(CommandRegistry&, const presets::Vocabulary*);

// cut-paper and paper text, marks, png, copy, cut
// (PaperCommands.cpp), each the session command or request of PaperPart;
// refused in a product without printers.
void addPaperCommands(CommandRegistry&);

// The debugger's commands (DebugCommands.cpp), each the session command or
// request of the DebuggerPart with the setter `harness`; refused with "The
// machine offers no debug target." in a product without a DebuggerPart.
void addDebugCommands(CommandRegistry&);
// media, media import, media export, media delete
// (MediaCommands.cpp) on the run's media part; refused where the product
// has no kind of medium. The media steps come with addPresetCommands.
void addMediaCommands(CommandRegistry&, media::MediaPart&);
// `open <path>` (OpenCommands.cpp), the session command `open` with the
// file's bytes and path, way `harness`; the reply says what it was opened as.
void addOpenCommand(CommandRegistry&);

// Every command of `calcu serve` in its order (the first commands, the
// startup commands, the preset commands on the parts' vocabulary, paper,
// debugger, media on their media part, open, then the product's); the
// attached harness has the same, on the app's parts.
void addServeCommands(CommandRegistry&, parts::LibraryParts& parts, HarnessProduct& product);

// The words of a command's argument text, parted by white space.
std::vector<std::string> words(const std::string& text);
// A session's reply to a command as the harness's: done (nothing read) or
// refused with its reason.
Reply fromSessionReply(const sessions::ReplyMessage&);

} // namespace calcu::harness
