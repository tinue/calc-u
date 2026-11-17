#pragma once
#include "harness/Protocol.hpp"
#include "harness/Reply.hpp"
#include "presets/Vocabulary.hpp"
#include "sessions/Messages.hpp"
#include <functional>
#include <map>
#include <string>
#include <vector>

// Commands (LIB-11 design §3.3; spec AC-9, AC-13). The registry is the place
// of AC-13: product commands under the product's word, common commands at
// the top level. Each later feature adds its commands in its own step, from
// its own area (key, type: LIB-5; preset, expect, screenshot, the bare
// wait: LIB-6; ...).

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
    void addProduct(std::string productWord, std::string name, Handler, std::string usage);   // AC-13
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

// The first commands of AC-9 and the commands of AC-23 and AC-24: wait,
// key-down, key-up, lcd, display, state, capabilities, ticks, audio,
// state-save, state-restore, quit.
void addFirstCommands(CommandRegistry&);
// LIB-5: key, type (TypingCommands.cpp); called by addFirstCommands.
void addTypingCommands(CommandRegistry&);

// LIB-18 AC-15 (design §3.6): model, variant, attach, detach, reset,
// cold-boot and power-cycle at the top level, each sent as the session
// command of LIB-18 with the reply of its operation.
void addStartupCommands(CommandRegistry&);

// LIB-6 AC-34 (design §3.9): preset, check-preset, preset-list, full-speed,
// regular-speed, switch, and every step of the vocabulary with
// StepDef::command (elapsed-time, expect, not-expect, screenshot, syncclock,
// a product's printer-trace; LIB-7's media steps from M-41), each sent as
// `run-step`. The bare `wait` is FirstCommands.cpp's.
// Null: for the usage text (--help), the steps every product has.
void addPresetCommands(CommandRegistry&, const presets::Vocabulary*);

// LIB-8 AC-23 (design §2): cut-paper and paper text, marks, png, copy, cut
// (PaperCommands.cpp), each the session command or request of PaperPart;
// refused in a product without printers.
void addPaperCommands(CommandRegistry&);

// LIB-10 AC-20, AC-50, AC-51 (design §3.9): the debugger's commands
// (DebugCommands.cpp), each the session command or request of LIB-10's
// DebuggerPart with the setter `harness`; refused with "The machine offers
// no debug target." in a product without a DebuggerPart.
void addDebugCommands(CommandRegistry&);
// LIB-7 AC-28 (design §3.11): media, media import, media export, media delete
// (MediaCommands.cpp) on the run's media part; refused where the product
// has no kind of medium. The media steps come with addPresetCommands.
void addMediaCommands(CommandRegistry&, media::MediaPart&);
// LIB-13 AC-9 (design §7, D-10): `open <path>` (OpenCommands.cpp), the
// session command `open` with the file's bytes and path, way `harness`; the
// reply says what it was opened as (AC-3).
void addOpenCommand(CommandRegistry&);

// D-27: every command of `calcu serve` in its order (the first commands,
// LIB-18's, LIB-6's on the parts' vocabulary, LIB-8's, LIB-10's, LIB-7's on
// their media part, LIB-13's, then the product's); the attached harness has
// the same, on the app's parts.
void addServeCommands(CommandRegistry&, parts::LibraryParts& parts, HarnessProduct& product);

// The words of a command's argument text, parted by white space.
std::vector<std::string> words(const std::string& text);
// A session's reply to a command as the harness's: done (nothing read) or
// refused with its reason.
Reply fromSessionReply(const sessions::ReplyMessage&);

} // namespace calcu::harness
