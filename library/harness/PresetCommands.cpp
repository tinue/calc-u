// The harness commands of LIB-6 (design §3.9; spec AC-34): one vocabulary
// with the preset steps (D-15). `key` and `type` are LIB-5's commands, the
// bare `wait` is FirstCommands.cpp's.
#include "harness/Commands.hpp"
#include "harness/HarnessHost.hpp"
#include "harness/Product.hpp"
#include "peripherals/PeripheralsPart.hpp"
#include "presets/PresetsPart.hpp"
#include "sessions/LibraryTopics.hpp"
#include <filesystem>

namespace calcu::harness {

namespace {

constexpr const char* kNoPresets = "This build reads no presets of format 2 yet.";

Reply fromSessionReply(const sessions::ReplyMessage& r) {
    if (!r.done) return refusedReply(r.reason);
    return doneReply();
}

std::vector<std::string> words(const std::string& s) {
    std::vector<std::string> out;
    std::string w;
    for (char c : s + " ") {
        if (c == ' ' || c == '\t') {
            if (!w.empty()) out.push_back(w);
            w.clear();
        } else {
            w += c;
        }
    }
    return out;
}

// A path relative to the working directory, or a bundle path as written.
std::string refOf(const std::string& written) {
    if (written.rfind("examples:", 0) == 0) return written;
    const std::filesystem::path p(std::u8string(written.begin(), written.end()));
    std::error_code ec;
    const std::u8string abs = std::filesystem::absolute(p, ec).generic_u8string();
    return ec ? written : std::string(abs.begin(), abs.end());
}

bool hasPresets(CommandContext& c) { return c.host.product().presetRules() != nullptr; }

Reply presetCmd(CommandContext& c) {
    if (c.line.argText.empty()) return refusedReply("usage: preset <path or examples:path>");
    if (!hasPresets(c)) return refusedReply(kNoPresets);
    sessions::Value a = sessions::Value::object({{"ref", refOf(c.line.argText)}});
    return fromSessionReply(c.host.command(sessions::Command{presets::kLoadPresetCommand, std::move(a)}));
}

Reply checkPresetCmd(CommandContext& c) {
    const auto files = words(c.line.argText);
    if (files.empty()) return refusedReply("usage: check-preset <file> [<file> ...]");
    if (!hasPresets(c)) return refusedReply(kNoPresets);
    Value result = Value::array();
    std::vector<std::string> lines;
    bool failed = false;
    for (const auto& f : files) {
        const sessions::ReplyMessage r =
            c.host.ask(sessions::Request{presets::kCheckPresetRequest, sessions::Value::object({{"ref", refOf(f)}})});
        Value x = Value::object();
        x.set("file", f);
        x.set("ok", r.done);
        x.set("error", r.done ? Value() : Value(r.reason));
        result.push(std::move(x));
        lines.push_back(r.done ? "ok " + f : "FAIL " + r.reason);
        failed = failed || !r.done;
    }
    Reply reply = doneReply(std::move(result), std::move(lines));
    reply.failsRun = failed;   // the run ends as failed when a file failed
    return reply;
}

Reply presetListCmd(CommandContext& c) {
    if (!hasPresets(c)) return refusedReply(kNoPresets);
    const sessions::ReplyMessage r = c.host.ask(sessions::Request{presets::kPresetListRequest, {}});
    if (!r.done) return refusedReply(r.reason);
    const auto* l = sessions::payloadAs<presets::PresetListReply>(r.result);
    if (!l) return refusedReply("The session gave no preset list.");
    std::vector<std::string> lines;
    for (const auto& e : l->entries) {
        const std::string kind =
            e.kind == presets::PresetListEntry::Kind::Startup ? std::string("startup") : "example (" + e.group + ")";
        lines.push_back(e.bundlePath + " | " + e.model.value + " | " + kind + " | " + e.name +
                        (e.description.empty() ? std::string() : " | " + e.description));
    }
    return doneReply(fromSession(l->toValue()), std::move(lines));
}

Handler speedCmd(bool full) {
    return [full](CommandContext& c) {
        return fromSessionReply(
            c.host.command(sessions::Command{sessions::kSpeedCommand, sessions::Value::object({{"full", full}})}));
    };
}

Reply switchCmd(CommandContext& c) {
    const auto w = words(c.line.argText);
    if (w.size() != 3 || (w[2] != "on" && w[2] != "off"))
        return refusedReply("usage: switch <peripheral> <control> on|off");
    sessions::Value a = sessions::Value::object({{"peripheral", w[0]}, {"control", w[1]}, {"on", w[2] == "on"}});
    return fromSessionReply(c.host.command(sessions::Command{peripherals::kSetControlCommand, std::move(a)}));
}

// A step of the vocabulary as a command: `run-step {verb, value}`.
Handler stepCmd(std::string verb) {
    return [verb](CommandContext& c) {
        if (!hasPresets(c)) return refusedReply(kNoPresets);
        sessions::Value a = sessions::Value::object({{"verb", verb}, {"value", c.line.argText}});
        const sessions::ReplyMessage r = c.host.command(sessions::Command{presets::kRunStepCommand, std::move(a)});
        if (!r.done) return refusedReply(r.reason);
        if (r.result) {   // a step's result: `elapsed-time report`
            const sessions::Value v = r.result->toValue();
            const sessions::Value* text = v.find("text");
            if (text && text->string()) {
                Value x = Value::object();
                x.set("text", *text->string());
                return doneReply(std::move(x), {*text->string()});
            }
        }
        return doneReply();
    };
}

std::string usageOf(const presets::StepDef& d) {
    std::string u = d.verb;
    switch (d.value) {
    case presets::StepDef::Value::None: break;
    case presets::StepDef::Value::Raw: u += " <text>"; break;
    default: u += " <value>"; break;
    }
    u.resize(std::max<std::size_t>(u.size() + 1, 25), ' ');
    return u + "the preset step " + d.verb + ":";
}

} // namespace

void addPresetCommands(CommandRegistry& r, const presets::Vocabulary* v) {
    r.add("preset", presetCmd, "preset <path>            load a preset (or examples:<path>, a bundled one)");
    r.add("check-preset", checkPresetCmd, "check-preset <file> ...  read and check presets, build nothing");
    r.add("preset-list", presetListCmd, "preset-list              the bundled presets");
    r.add("full-speed", speedCmd(true), "full-speed               the machine at full speed");
    r.add("regular-speed", speedCmd(false), "regular-speed            the machine at regular speed");
    r.add("switch", switchCmd, "switch <p> <control> on|off  a switch or key of a peripheral");
    if (v) {
        for (const auto& d : v->steps())
            if (d.command && !r.isCommand(d.verb)) r.add(d.verb, stepCmd(d.verb), usageOf(d));
        return;
    }
    for (const char* verb : {"elapsed-time", "expect", "not-expect", "screenshot", "syncclock"})
        if (!r.isCommand(verb)) r.add(verb, stepCmd(verb), std::string(verb) + " ...  a preset step");
}

} // namespace calcu::harness
