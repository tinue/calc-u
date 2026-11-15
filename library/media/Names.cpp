#include "media/Names.hpp"

namespace calcu::media {

NameCheck checkName(std::string_view raw) {
    NameCheck out;
    bool space = false;
    for (char c : raw) {
        if (c == ' ') {
            space = !out.name.empty();
            continue;
        }
        if (space) out.name += ' ';
        space = false;
        out.name += c;
    }
    if (out.name.empty()) {
        out.refusal = "A name cannot be empty.";
        return out;
    }
    for (char c : out.name) {
        const auto u = static_cast<unsigned char>(c);
        if (u < 0x20 || u == 0x7F) {
            out.refusal = "A name cannot hold a control character.";
            return out;
        }
        if (std::string_view("/\\:*?\"<>|").find(c) != std::string_view::npos) {
            out.refusal = std::string("A name cannot hold the character ") + c + ".";
            return out;
        }
    }
    return out;
}

std::string foldName(std::string_view s) {
    std::string out(s);
    for (char& c : out)
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    return out;
}

bool sameName(std::string_view a, std::string_view b) { return foldName(a) == foldName(b); }

std::string fileNameFor(const std::string& name, const KindDecl& k) { return name + k.suffix; }

std::string numberedFileName(const std::string& name, const KindDecl& k,
                             const std::function<bool(const std::string&)>& taken) {
    for (int n = 1;; ++n) {
        const std::string f = name + "-" + std::to_string(n) + k.suffix;
        if (!taken(f)) return f;
    }
}

bool hasSuffix(std::string_view f, const KindDecl& k) {
    return !k.suffix.empty() && f.size() > k.suffix.size() &&
           foldName(f.substr(f.size() - k.suffix.size())) == foldName(k.suffix);
}

std::string stemOf(std::string_view f, const KindDecl& k) {
    return std::string(hasSuffix(f, k) ? f.substr(0, f.size() - k.suffix.size()) : f);
}

std::string nameOf(const MediumInfo& info, std::string_view fileName, const KindDecl& k) {
    if (!info.name.empty()) return info.name;
    return stemOf(baseName(fileName), k);
}

std::string baseName(std::string_view path) {
    const std::size_t at = path.find_last_of("/\\");
    return std::string(at == std::string_view::npos ? path : path.substr(at + 1));
}

} // namespace calcu::media
