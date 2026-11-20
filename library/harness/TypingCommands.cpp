// The harness commands `key` and `type`: the session commands of the typing
// part, whose replies come when the keys are up; the harness lets emulated time pass until then,
// at most an hour (a program that never comes back to its key scan would
// otherwise hang the harness). On a model without characters `type` is an
// unknown command. Neither waits until the machine is idle: that is `wait`.
#include "formats/Duration.hpp"
#include "harness/Commands.hpp"
#include "harness/HarnessHost.hpp"
#include "typing/TypingTopics.hpp"

namespace calcu::harness {

namespace {

constexpr const char* kNotTaken = "The keys were not taken within an hour.";

Reply fromSessionReply(const sessions::ReplyMessage& r) {
    if (!r.done) return refusedReply(r.reason);
    return doneReply();
}

Reply keyCmd(CommandContext& c) {
    if (c.line.argText.empty()) return refusedReply("usage: key <key ID> ... [hold=<duration>]");
    sessions::Value::Array names;
    std::optional<double> hold;
    std::string word;
    for (char ch : c.line.argText + " ") {
        if (ch != ' ' && ch != '\t') {
            word += ch;
            continue;
        }
        if (word.rfind("hold=", 0) == 0) {   // every press held exactly this long
            std::string error;
            const auto us = formats::parseDuration(word.substr(5), &error);
            if (!us) return refusedReply("hold= takes a duration: " + word.substr(5) + ".");
            hold = static_cast<double>(*us) / 1e6;
        } else if (!word.empty()) {
            names.push_back(sessions::Value(word));
        }
        word.clear();
    }
    sessions::Value a = hold ? sessions::Value::object({{"names", sessions::Value(std::move(names))}, {"hold", *hold}})
                             : sessions::Value::object({{"names", sessions::Value(std::move(names))}});
    return fromSessionReply(c.host.command(sessions::Command{typing::kKey, std::move(a)}, kNotTaken));
}

Reply typeCmd(CommandContext& c) {
    sessions::Value a = sessions::Value::object({{"text", c.line.argText}});
    return fromSessionReply(c.host.command(sessions::Command{typing::kType, std::move(a)}, kNotTaken));
}

} // namespace

void addTypingCommands(CommandRegistry& r) {
    r.add("key", keyCmd, "key <key ID> ... [hold=<d>] press the keys in turn; replies when the last is up");
    r.add("type", typeCmd, "type <text>              type the text and ENTER (Sharp); replies when ENTER is up");
}

} // namespace calcu::harness
