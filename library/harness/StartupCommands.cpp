// LIB-18's harness commands (LIB-18 design §3.6; spec AC-15, OQ-5): at the
// top level, each sent as the session command of LIB-18 (StartupPart) with
// the reply of its operation, run in one go (LIB-11 AC-7).
#include "harness/Commands.hpp"
#include "harness/HarnessHost.hpp"
#include "startup/StartupPart.hpp"
#include <string>

namespace calcu::harness {

using sessions::Command;
using sessions::CommandName;

namespace {

Reply sent(CommandContext& c, const CommandName& name, sessions::Value args) {
    const sessions::ReplyMessage r = c.host.command(Command{name, std::move(args)});
    return r.done ? doneReply() : refusedReply(r.reason);
}

Reply needs(const char* usage) { return refusedReply(std::string("usage: ") + usage); }

// The first word and the rest after it, trimmed.
std::pair<std::string, std::string> split(const std::string& s) {
    const auto end = s.find_first_of(" \t");
    if (end == std::string::npos) return {s, {}};
    return {s.substr(0, end), trimmed(std::string_view(s).substr(end))};
}

Handler named(const CommandName& name, const char* usage) {
    return [name, usage](CommandContext& c) {
        if (c.line.argText.empty()) return needs(usage);
        return sent(c, name, sessions::Value::object({{"name", c.line.argText}}));
    };
}

Handler bare(const CommandName& name) {
    return [name](CommandContext& c) { return sent(c, name, sessions::Value()); };
}

// `attach <place> <peripheral>[:<variant>]`: the place as placeText writes
// it (`a/jack`), the variant as in a preset (`CE-1600P:old`).
Reply attachCmd(CommandContext& c) {
    const char* usage = "attach <place> <peripheral>[:<variant>]";
    const auto [place, rest] = split(c.line.argText);
    if (place.empty() || rest.empty() || rest.find_first_of(" \t") != std::string::npos) return needs(usage);
    const auto colon = rest.find(':');
    sessions::Value::Object args{{"place", place}, {"peripheral", rest.substr(0, colon)}};
    if (colon != std::string::npos) args.push_back({"variant", rest.substr(colon + 1)});
    return sent(c, startup::kAttachCommand, sessions::Value(std::move(args)));
}

Reply detachCmd(CommandContext& c) {
    if (c.line.argText.empty()) return needs("detach <place>");
    return sent(c, startup::kDetachCommand, sessions::Value::object({{"place", c.line.argText}}));
}

} // namespace

void addStartupCommands(CommandRegistry& r) {
    r.add("model", named(startup::kModelCommand, "model <name>"), "model <name>             switch to another model");
    r.add("variant", named(startup::kVariantCommand, "variant <name>"),
          "variant <name>           switch to another variant of the model");
    r.add("attach", attachCmd, "attach <place> <peripheral>[:<variant>]  plug a peripheral in");
    r.add("detach", detachCmd, "detach <place>           take out what is in the place");
    r.add("reset", bare(startup::kResetCommand), "reset                    Reset");
    r.add("cold-boot", bare(startup::kColdBootCommand), "cold-boot                Cold boot: all memory empty");
    r.add("power-cycle", bare(startup::kPowerCycleCommand), "power-cycle              switch the machine off and on");
}

} // namespace calcu::harness
