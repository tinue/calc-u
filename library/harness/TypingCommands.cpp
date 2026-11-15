// The harness commands `key` and `type` (LIB-5 design §3.5; spec AC-15 to
// AC-17; D-5, D-8): the session commands of the typing part, whose replies
// come when the keys are up; the harness lets emulated time pass until then,
// at most an hour. Neither waits until the machine is idle: that is `wait`.
#include "harness/Commands.hpp"
#include "harness/HarnessHost.hpp"
#include "typing/TypingTopics.hpp"

namespace calcu::harness {

namespace {

constexpr const char* kNotTaken = "The keys were not taken within an hour.";   // D-5

Reply fromSessionReply(const sessions::ReplyMessage& r) {
    if (!r.done) return refusedReply(r.reason);
    return doneReply();
}

Reply keyCmd(CommandContext& c) {
    if (c.line.argText.empty()) return refusedReply("usage: key <name> [<name> ...]");
    sessions::Value::Array names;
    std::string word;
    for (char ch : c.line.argText + " ") {
        if (ch == ' ' || ch == '\t') {
            if (!word.empty()) names.push_back(sessions::Value(word));
            word.clear();
        } else {
            word += ch;
        }
    }
    sessions::Value a = sessions::Value::object({{"names", sessions::Value(std::move(names))}});
    return fromSessionReply(c.host.command(sessions::Command{typing::kKey, std::move(a)}, kNotTaken));
}

Reply typeCmd(CommandContext& c) {
    sessions::Value a = sessions::Value::object({{"text", c.line.argText}});
    return fromSessionReply(c.host.command(sessions::Command{typing::kType, std::move(a)}, kNotTaken));
}

} // namespace

void addTypingCommands(CommandRegistry& r) {
    r.add("key", keyCmd, "key <name> [<name> ...]  press the keys in turn; replies when the last is up");
    r.add("type", typeCmd, "type <text>              type the text and ENTER (Sharp); replies when ENTER is up");
}

} // namespace calcu::harness
