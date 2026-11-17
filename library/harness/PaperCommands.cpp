// The paper commands (LIB-8 design §2, §3.5; spec AC-23, EC-2, EC-6, EC-8):
// `cut-paper` and `paper text`, `paper marks`, `paper png <path>`,
// `paper copy`, `paper cut`, each with an owner's name as its last value;
// without it the printer that is plugged in. Each is the session command or
// request of LIB-8's PaperPart; a product without printers has none of them
// (EC-2: no paper topic) and they are refused. The paper topic itself is read
// with `state paper <owner>`.
#include "harness/Commands.hpp"
#include "harness/HarnessHost.hpp"
#include "harness/Product.hpp"
#include "paper/PaperTopics.hpp"
#include <cerrno>
#include <cstring>
#include <fstream>
#include <sstream>

namespace calcu::harness {

namespace {

sessions::Value ownerArgs(const std::vector<std::string>& w, std::size_t at) {
    sessions::Value::Object o;
    if (w.size() > at) o.push_back({"owner", w[at]});
    return sessions::Value(std::move(o));
}

std::optional<Reply> noPaper(CommandContext& c) {
    if (c.host.readTopic(paper::kPapersTopic.value)) return std::nullopt;
    return refusedReply(std::string(c.host.product().name()) + " has no printer.");
}

Reply linesReply(const sessions::ReplyMessage& r) {
    if (!r.done) return refusedReply(r.reason);
    std::vector<std::string> lines;
    Value arr = Value::array();
    const sessions::Value v = r.result ? r.result->toValue() : sessions::Value();
    if (const sessions::Value* l = v.find("lines"); l && l->elements())
        for (const auto& e : *l->elements())
            if (e.string()) {
                lines.push_back(*e.string());
                arr.push(*e.string());
            }
    return doneReply(std::move(arr), std::move(lines));
}

Reply cutPaperCmd(CommandContext& c) {
    if (auto r = noPaper(c)) return *r;
    const auto w = words(c.line.argText);
    if (w.size() > 1) return refusedReply("usage: cut-paper [<printer>]");
    sessions::Value a = ownerArgs(w, 0);
    sessions::Value::Object o = *a.members();
    o.push_back({"copy", false});
    return fromSessionReply(c.host.command(sessions::Command{paper::kPaperCutCommand, sessions::Value(std::move(o))}));
}

Reply writePng(CommandContext& c, const std::string& path, const sessions::Value& owner) {
    const sessions::ReplyMessage r = c.host.ask(sessions::Request{paper::kPaperPictureRequest, owner});
    if (!r.done) return refusedReply(r.reason);
    const sessions::Value v = r.result ? r.result->toValue() : sessions::Value();
    const sessions::Value* bytes = v.find("bytes");
    if (!bytes || !bytes->bytes()) return refusedReply("The session gave no picture.");
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) return refusedReply("Cannot write " + path + ": " + std::strerror(errno) + ".");   // EC-6
    out.write(reinterpret_cast<const char*>(bytes->bytes()->data()), static_cast<std::streamsize>(bytes->bytes()->size()));
    out.close();
    if (!out) return refusedReply("Cannot write " + path + ".");
    Value result = Value::object();
    result.set("path", path);
    if (const sessions::Value* wv = v.find("width"); wv && wv->integer()) result.set("width", static_cast<long long>(*wv->integer()));
    if (const sessions::Value* hv = v.find("height"); hv && hv->integer()) result.set("height", static_cast<long long>(*hv->integer()));
    return doneReply(std::move(result));
}

Reply paperCmd(CommandContext& c) {
    static const char* kUsage = "usage: paper text|marks|png <path>|copy|cut [<printer>]";
    if (auto r = noPaper(c)) return *r;
    const auto w = words(c.line.argText);
    if (w.empty()) return refusedReply(kUsage);
    const std::string& what = w[0];
    if (what == "text" || what == "marks") {
        if (w.size() > 2) return refusedReply(kUsage);
        const auto& name = what == "text" ? paper::kPaperTextRequest : paper::kPaperMarksRequest;
        return linesReply(c.host.ask(sessions::Request{name, ownerArgs(w, 1)}));
    }
    if (what == "copy") {
        if (w.size() > 2) return refusedReply(kUsage);
        return fromSessionReply(c.host.command(sessions::Command{paper::kPaperCopyCommand, ownerArgs(w, 1)}));
    }
    if (what == "cut") {
        if (w.size() > 2) return refusedReply(kUsage);
        sessions::Value::Object o = *ownerArgs(w, 1).members();
        o.push_back({"copy", true});
        return fromSessionReply(c.host.command(sessions::Command{paper::kPaperCutCommand, sessions::Value(std::move(o))}));
    }
    if (what == "png") {
        if (w.size() < 2 || w.size() > 3) return refusedReply(kUsage);
        return writePng(c, w[1], ownerArgs(w, 2));
    }
    return refusedReply(kUsage);
}

} // namespace

void addPaperCommands(CommandRegistry& r) {
    r.add("cut-paper", cutPaperCmd, "cut-paper [<printer>]    cut the paper (of the printer plugged in), without a copy");
    r.add("paper", paperCmd, "paper <what> [<printer>]  text, marks, png <path>, copy or cut of the paper");
}

} // namespace calcu::harness
