// The harness command of LIB-13 (design §7, D-10; spec AC-9): `open <path>`,
// the session command `open` as a frontend sends it, with the file's bytes
// and its path (relative to the working directory), way `harness`. The reply
// is that of AC-3: what the file was opened as, or why it was refused.
#include "harness/Commands.hpp"
#include "harness/HarnessHost.hpp"
#include "opening/OpenRequest.hpp"
#include <filesystem>
#include <fstream>
#include <iterator>

namespace calcu::harness {

namespace {

std::filesystem::path pathOf(const std::string& s) { return std::filesystem::path(std::u8string(s.begin(), s.end())); }
std::string textOf(const std::filesystem::path& p) {
    const std::u8string u = p.generic_u8string();
    return std::string(u.begin(), u.end());
}

Reply openCmd(CommandContext& c) {
    if (c.line.argText.empty()) return refusedReply("usage: open <path>");
    std::error_code ec;
    const std::filesystem::path abs = std::filesystem::absolute(pathOf(c.line.argText), ec);
    const std::filesystem::path p = ec ? pathOf(c.line.argText) : abs;
    opening::OpenedFile f;
    f.name = textOf(p.filename());
    f.path = textOf(p);
    if (std::ifstream in(p, std::ios::binary); in && std::filesystem::is_regular_file(p, ec))
        f.bytes = std::vector<std::uint8_t>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
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
