#include "settings/IniDocument.hpp"
#include "settings/Declaration.hpp"

namespace calcu::settings {

namespace {

std::string_view trim(std::string_view s) {
    const char* ws = " \t\r";  // a lone CR ends a last line without LF
    const auto b = s.find_first_not_of(ws);
    if (b == std::string_view::npos) return {};
    const auto e = s.find_last_not_of(ws);
    return s.substr(b, e - b + 1);
}

// Well-formed UTF-8: no overlong forms, no surrogates, nothing above U+10FFFF.
bool validUtf8(std::string_view s) {
    std::size_t i = 0;
    while (i < s.size()) {
        const auto c = static_cast<unsigned char>(s[i]);
        if (c < 0x80) { ++i; continue; }
        int n = 0;
        unsigned cp = 0;
        if (c >= 0xC2 && c <= 0xDF) { n = 1; cp = c & 0x1F; }
        else if (c >= 0xE0 && c <= 0xEF) { n = 2; cp = c & 0x0F; }
        else if (c >= 0xF0 && c <= 0xF4) { n = 3; cp = c & 0x07; }
        else return false;
        for (int k = 1; k <= n; ++k) {
            if (i + static_cast<std::size_t>(k) >= s.size()) return false;
            const auto cc = static_cast<unsigned char>(s[i + static_cast<std::size_t>(k)]);
            if ((cc & 0xC0) != 0x80) return false;
            cp = (cp << 6) | (cc & 0x3F);
        }
        if ((n == 2 && cp < 0x800) || (n == 3 && (cp < 0x10000 || cp > 0x10FFFF))) return false;
        if (cp >= 0xD800 && cp <= 0xDFFF) return false;
        i += static_cast<std::size_t>(n) + 1;
    }
    return true;
}

} // namespace

ParseResult IniDocument::parse(std::string_view bytes) {
    ParseResult result;
    if (bytes.size() >= 3 && bytes.substr(0, 3) == "\xEF\xBB\xBF") {
        result.problem = "the file starts with a byte order mark";
        return result;
    }
    if (!validUtf8(bytes)) {
        result.problem = "the file is not UTF-8";
        return result;
    }
    IniDocument doc;
    bool firstEnding = true;
    std::string section;
    std::size_t pos = 0;
    int lineNo = 0;
    while (pos < bytes.size()) {
        ++lineNo;
        const auto nl = bytes.find('\n', pos);
        Line line;
        std::string_view raw;
        if (nl == std::string_view::npos) {
            raw = bytes.substr(pos);
            pos = bytes.size();
        } else {
            raw = bytes.substr(pos, nl - pos);
            line.ending = "\n";
            pos = nl + 1;
        }
        if (!line.ending.empty() && !raw.empty() && raw.back() == '\r') {
            raw.remove_suffix(1);
            line.ending = "\r\n";
        }
        if (firstEnding && !line.ending.empty()) {
            doc.newline_ = line.ending;
            firstEnding = false;
        }
        line.raw = std::string(raw);
        const std::string_view t = trim(raw);
        if (t.empty()) {
            line.kind = Kind::Blank;
        } else if (t[0] == '#' || t[0] == ';') {
            line.kind = Kind::Comment;
        } else if (t[0] == '[') {
            if (t.back() != ']' || t.size() < 3 || trim(t.substr(1, t.size() - 2)).empty()) {
                result.problem = "line " + std::to_string(lineNo) + " is not a [section] header";
                result.line = lineNo;
                return result;
            }
            line.kind = Kind::Section;
            section = lowerAscii(trim(t.substr(1, t.size() - 2)));
        } else {
            const auto eq = t.find('=');
            if (eq == std::string_view::npos || trim(t.substr(0, eq)).empty()) {
                result.problem = "line " + std::to_string(lineNo) + " is neither a [section] nor a key = value line";
                result.line = lineNo;
                return result;
            }
            line.kind = Kind::Key;
            line.key = lowerAscii(trim(t.substr(0, eq)));
            line.value = std::string(trim(t.substr(eq + 1)));
        }
        line.section = section;
        doc.lines_.push_back(std::move(line));
    }
    result.doc = std::move(doc);
    return result;
}

std::optional<std::string> IniDocument::get(std::string_view section, std::string_view key) const {
    const std::string s = lowerAscii(section), k = lowerAscii(key);
    for (auto it = lines_.rbegin(); it != lines_.rend(); ++it)
        if (it->kind == Kind::Key && it->section == s && it->key == k) return it->value;
    return std::nullopt;
}

void IniDocument::insert(std::size_t at, Line line) {
    // The line before the new one needs an ending of its own.
    if (at > 0 && lines_[at - 1].ending.empty()) lines_[at - 1].ending = newline_;
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(at), std::move(line));
}

void IniDocument::set(std::string_view section, std::string_view key, std::string_view value) {
    const std::string s = lowerAscii(section), k = lowerAscii(key);
    const auto formatted = [&](std::string_view name) {
        return value.empty() ? std::string(name) + " =" : std::string(name) + " = " + std::string(value);
    };
    // The line that counts: changed in place, keeping the key's spelling.
    for (auto it = lines_.rbegin(); it != lines_.rend(); ++it) {
        if (it->kind == Kind::Key && it->section == s && it->key == k) {
            const std::string_view raw = it->raw;
            const auto b = raw.find_first_not_of(" \t");
            const auto eq = raw.find('=');
            const std::string indent(raw.substr(0, b));
            it->raw = indent + formatted(trim(raw.substr(b, eq - b)));
            it->value = std::string(value);
            return;
        }
    }
    Line line;
    line.kind = Kind::Key;
    line.section = s;
    line.key = k;
    line.value = std::string(value);
    line.raw = formatted(key);
    line.ending = newline_;
    // After the last key line or header of the section's last block.
    std::optional<std::size_t> after;
    for (std::size_t i = 0; i < lines_.size(); ++i) {
        const Line& l = lines_[i];
        if (l.section == s && (l.kind == Kind::Key || l.kind == Kind::Section)) after = i;
    }
    if (after) {
        insert(*after + 1, std::move(line));
        return;
    }
    if (s.empty()) {
        // A first top-level key: before the first section, after the leading comments.
        std::size_t at = 0;
        while (at < lines_.size() && lines_[at].kind == Kind::Comment) ++at;
        insert(at, std::move(line));
        return;
    }
    // A new section at the end, after a blank line.
    if (!lines_.empty()) {
        if (lines_.back().ending.empty()) lines_.back().ending = newline_;
        if (lines_.back().kind != Kind::Blank) {
            Line blank;
            blank.ending = newline_;
            blank.section = lines_.back().section;
            lines_.push_back(std::move(blank));
        }
    }
    Line header;
    header.kind = Kind::Section;
    header.section = s;
    header.raw = "[" + std::string(section) + "]";
    header.ending = newline_;
    lines_.push_back(std::move(header));
    lines_.push_back(std::move(line));
}

std::string IniDocument::text() const {
    std::string out;
    for (const Line& l : lines_) {
        out += l.raw;
        out += l.ending;
    }
    return out;
}

} // namespace calcu::settings
