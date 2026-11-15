// The media commands (LIB-7 design §3.11; spec AC-26, AC-28; LIB-11 AC-8,
// AC-22): `media` (the topic `media` as `state` gives it), `media import
// <file>`, `media export <kind> <name> <path>` and `media delete <kind>
// <name>`. The media steps (`test-disk { name: Work }`, `eject disk`,
// `saveas { … }`) are LIB-6's preset commands (StepDef::command), sent as
// `run-step`. A product without kinds of medium has no topic `media`: the
// commands are refused.
#include "harness/Commands.hpp"
#include "harness/HarnessHost.hpp"
#include "harness/Product.hpp"
#include "media/MediaPart.hpp"
#include "media/Shelf.hpp"
#include "settings/AtomicFile.hpp"
#include <sstream>

namespace calcu::harness {

namespace {

std::vector<std::string> words(const std::string& text) {
    std::vector<std::string> out;
    std::istringstream in(text);
    for (std::string w; in >> w;) out.push_back(w);
    return out;
}

std::string joined(const std::vector<std::string>& w, std::size_t from, std::size_t to) {
    std::string out;
    for (std::size_t i = from; i < to && i < w.size(); ++i) out += (out.empty() ? "" : " ") + w[i];
    return out;
}

Reply fromCommand(const sessions::ReplyMessage& r) { return r.done ? doneReply() : refusedReply(r.reason); }

Reply mediaCmd(CommandContext& c, media::MediaPart* part) {
    static const char* kUsage = "usage: media | media import <file> | media export <kind> <name> <path> | media delete <kind> <name>";
    if (!part) return refusedReply(std::string(c.host.product().name()) + " has no media.");
    const auto w = words(c.line.argText);
    if (w.empty()) {   // AC-7: the topic, read again first
        c.host.command(sessions::Command{media::kRefreshCommand, {}});
        const auto t = c.host.readTopic(media::kMediaTopic.value);
        if (!t || !t->state) return refusedReply("The session gave no media.");
        Value v = Value::object();
        v.set("topic", media::kMediaTopic.value);
        v.set("shape", "state");
        v.set("state", fromSession(t->state->toValue()));
        return doneReply(std::move(v), {payloadText(t->state)});
    }
    if (w[0] == "import" && w.size() >= 2) {   // AC-30
        sessions::Value::Array paths{sessions::Value(joined(w, 1, w.size()))};
        return fromCommand(c.host.command(
            sessions::Command{media::kImportCommand, sessions::Value::object({{"paths", sessions::Value(std::move(paths))}})}));
    }
    if (w[0] == "delete" && w.size() >= 3)   // AC-31: the command is the confirmation
        return fromCommand(c.host.command(sessions::Command{
            media::kDeleteCommand, sessions::Value::object({{"kind", w[1]}, {"name", joined(w, 2, w.size())}, {"confirmed", true}})}));
    if (w[0] == "export" && w.size() >= 4) {   // AC-31, D-13: its files side by side
        const media::KindDecl* k = part->kind(w[1]);
        if (!k) return refusedReply("There is no kind of medium " + w[1] + ".");
        std::string reason;
        const auto files = part->exportFiles(*k, joined(w, 2, w.size() - 1), &reason);
        if (!files) return refusedReply(reason);
        const std::string path = w.back();
        const std::string dir = media::folderOfPath(path);
        for (std::size_t i = 0; i < files->size(); ++i) {
            const std::string target = i == 0 ? path : media::joinFile(dir, (*files)[i].relativePath);
            const auto r = settings::writeFileAtomically(target, (*files)[i].bytes);
            if (!r.ok) return refusedReply("Cannot write " + target + ": " + r.problem);
        }
        Value v = Value::object();
        v.set("path", path);
        return doneReply(std::move(v));
    }
    return refusedReply(kUsage);
}

} // namespace

void addMediaCommands(CommandRegistry& r, media::MediaPart* part) {
    r.add("media", [part](CommandContext& c) { return mediaCmd(c, part); },
          "media [import|export|delete …]  the media topic; import a file; export or delete a saved medium");
}

} // namespace calcu::harness
