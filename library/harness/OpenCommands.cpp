// The harness command of LIB-13 (design §7, D-10; spec AC-9): `open <path>`,
// the session command `open` as a frontend sends it, with the file's bytes
// and its path (relative to the working directory), way `harness`. The reply
// is that of AC-3: what the file was opened as, or why it was refused.
#include "files/Files.hpp"
#include "harness/Commands.hpp"
#include "harness/HarnessHost.hpp"
#include "opening/OpenRequest.hpp"
#include <filesystem>
#include <fstream>
#include <iterator>

namespace calcu::harness {

namespace {

Reply openCmd(CommandContext& c) {
    if (c.line.argText.empty()) return refusedReply("usage: open <path>");
    std::error_code ec;
    const std::filesystem::path abs = std::filesystem::absolute(files::toPath(c.line.argText), ec);
    const std::filesystem::path p = ec ? files::toPath(c.line.argText) : abs;
    opening::OpenedFile f;
    f.name = files::fromPath(p.filename());
    f.path = files::fromPath(p);
    if (std::filesystem::is_regular_file(p, ec))
        if (auto bytes = files::readBytes(p)) f.bytes = std::move(*bytes);
    opening::OpenRequest req;
    req.files.push_back(std::move(f));
    req.way = opening::Way::Harness;
    const sessions::ReplyMessage r = c.host.command(sessions::Command{opening::kOpenCommand, req.toValue()});
    if (!r.done) return refusedReply(r.reason);
    const auto* o = sessions::payloadAs<opening::OpenReply>(r.result);
    if (!o) return doneReply();
    Value v = Value::object();
    v.set("as", std::string(opening::openedAsName(o->as)));
    v.set("file", o->file);
    v.set("detail", o->detail);
    std::string line = std::string(opening::openedAsName(o->as)) + " " + o->file;
    if (!o->detail.empty()) line += " " + o->detail;
    return doneReply(std::move(v), {line});
}

} // namespace

void addOpenCommand(CommandRegistry& r) { r.add("open", openCmd, "open <path>"); }

} // namespace calcu::harness
