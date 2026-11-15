#pragma once
#include "harness/Protocol.hpp"
#include "harness/Reply.hpp"
#include "presets/Vocabulary.hpp"
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

namespace calcu::harness {

class HarnessHost;

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
// `run-step`. Without a vocabulary (a product without preset rules yet) the
// commands are refused. The bare `wait` is FirstCommands.cpp's.
void addPresetCommands(CommandRegistry&, const presets::Vocabulary*);

// LIB-8 AC-23 (design §2): cut-paper and paper text, marks, png, copy, cut
// (PaperCommands.cpp), each the session command or request of PaperPart;
// refused in a product without printers.
void addPaperCommands(CommandRegistry&);

// LIB-7 AC-28 (design §3.11): media, media import, media export, media delete
// (MediaCommands.cpp) on the run's media part; refused without one. The
// media steps come with addPresetCommands.
void addMediaCommands(CommandRegistry&, media::MediaPart*);

} // namespace calcu::harness
