#include "harness/Commands.hpp"
#include "harness/Protocol.hpp"
#include <sstream>

namespace calcu::harness {

std::vector<std::string> words(const std::string& text) {
    std::vector<std::string> out;
    std::istringstream in(text);
    for (std::string w; in >> w;) out.push_back(w);
    return out;
}

Reply fromSessionReply(const sessions::ReplyMessage& r) { return r.done ? doneReply() : refusedReply(r.reason); }

void CommandRegistry::add(std::string name, Handler h, std::string usage) {
    usage_.push_back(usage);
    common_[std::move(name)] = Entry{std::move(h), std::move(usage)};
}

void CommandRegistry::addProduct(std::string word, std::string name, Handler h, std::string usage) {
    usage_.push_back(word + " " + usage);
    product_[std::move(word)][std::move(name)] = Entry{std::move(h), std::move(usage)};
}

const Handler* CommandRegistry::find(const std::vector<std::string>& path) const {
    if (path.size() == 1) {
        const auto it = common_.find(path[0]);
        return it == common_.end() ? nullptr : &it->second.handler;
    }
    if (path.size() == 2) {
        const auto p = product_.find(path[0]);
        if (p == product_.end()) return nullptr;
        const auto it = p->second.find(path[1]);
        return it == p->second.end() ? nullptr : &it->second.handler;
    }
    return nullptr;
}

bool CommandRegistry::isProductWord(const std::string& word) const { return product_.count(word) != 0; }
bool CommandRegistry::isCommand(const std::string& name) const { return common_.count(name) != 0; }
std::vector<std::string> CommandRegistry::usage() const { return usage_; }

// ── Protocol ────────────────────────────────────────────────────────────

std::string trimmed(std::string_view s) {
    while (!s.empty() && (s.back() == '\r' || s.back() == ' ' || s.back() == '\t')) s.remove_suffix(1);
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) s.remove_prefix(1);
    return std::string(s);
}

namespace {
// The first word and the rest after it, trimmed.
std::pair<std::string, std::string> firstWord(const std::string& s) {
    const auto end = s.find_first_of(" \t");
    if (end == std::string::npos) return {s, {}};
    return {s.substr(0, end), trimmed(std::string_view(s).substr(end))};
}
} // namespace

std::variant<CommandLine, std::string> parseLine(std::string_view line, const CommandRegistry& reg) {
    const std::string t = trimmed(line);
    CommandLine out;
    if (t.empty()) return out;
    auto [word, rest] = firstWord(t);
    if (reg.isProductWord(word)) {
        auto [name, args] = firstWord(rest);
        if (name.empty()) return std::string("unknown command " + word + " (a product command needs its name)");
        if (!reg.find({word, name})) return std::string("unknown command " + word + " " + name);
        out.path = {word, name};
        out.argText = args;
        return out;
    }
    if (!reg.isCommand(word)) return std::string("unknown command " + word);
    out.path = {word};
    out.argText = rest;
    return out;
}

std::vector<std::string> splitOneShot(std::span<const std::string> words) {
    std::vector<std::string> lines;
    std::string cur;
    bool any = false;
    for (const auto& w : words) {
        if (w == kThen) {
            lines.push_back(cur);
            cur.clear();
            any = false;
            continue;
        }
        if (any) cur += ' ';
        cur += w;
        any = true;
    }
    lines.push_back(cur);
    return lines;
}

} // namespace calcu::harness
