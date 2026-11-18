#include "opening/Classify.hpp"
#include "machine/Names.hpp"
#include "opening/FileKind.hpp"
#include "opening/OpenPart.hpp"
#include <cstring>

namespace calcu::opening {

std::string fileNameOf(const OpenedFile& f) {
    if (!f.name.empty()) return f.name;
    const std::string& p = f.path ? *f.path : std::string();
    const std::size_t slash = p.find_last_of("/\\");
    return slash == std::string::npos ? p : p.substr(slash + 1);
}

bool endsWith(std::string_view s, std::string_view suffix) {
    return !suffix.empty() && s.size() >= suffix.size() && s.substr(s.size() - suffix.size()) == suffix;
}

std::string sizeText(std::uint64_t n) {
    constexpr std::uint64_t kKiB = 1024, kMiB = 1024 * 1024;
    if (n >= kMiB && n % kMiB == 0) return std::to_string(n / kMiB) + " MiB";
    if (n >= kKiB && n % kKiB == 0) return std::to_string(n / kKiB) + " KiB";
    return std::to_string(n) + " bytes";
}

namespace {

// Well-formed UTF-8: no overlong forms, no surrogates, nothing above U+10FFFF.
bool validUtf8(const std::uint8_t* p, std::size_t n) {
    for (std::size_t i = 0; i < n;) {
        const std::uint8_t c = p[i];
        std::size_t len = 0;
        std::uint32_t cp = 0;
        if (c < 0x80) {
            ++i;
            continue;
        }
        if ((c & 0xE0) == 0xC0) {
            len = 2;
            cp = c & 0x1Fu;
        } else if ((c & 0xF0) == 0xE0) {
            len = 3;
            cp = c & 0x0Fu;
        } else if ((c & 0xF8) == 0xF0) {
            len = 4;
            cp = c & 0x07u;
        } else {
            return false;
        }
        if (i + len > n) return false;
        for (std::size_t k = 1; k < len; ++k) {
            if ((p[i + k] & 0xC0) != 0x80) return false;
            cp = (cp << 6) | (p[i + k] & 0x3Fu);
        }
        if ((len == 2 && cp < 0x80) || (len == 3 && cp < 0x800) || (len == 4 && cp < 0x10000) || cp > 0x10FFFF ||
            (cp >= 0xD800 && cp <= 0xDFFF))
            return false;
        i += len;
    }
    return true;
}

bool hasAny(std::string_view lowerName, const std::vector<std::string>& suffixes) {
    for (const auto& s : suffixes)
        if (endsWith(lowerName, machine::lowerAscii(s))) return true;
    return false;
}

std::optional<std::uint64_t> limitOf(Way w, const OpenSetup& s) {
    const auto it = s.maxBytes.find(w);
    if (it == s.maxBytes.end()) return std::nullopt;
    return it->second;
}

} // namespace

bool looksLikePreset(const std::vector<std::uint8_t>& bytes) {
    const std::uint8_t* p = bytes.data();
    std::size_t n = bytes.size();
    if (n >= 3 && p[0] == 0xEF && p[1] == 0xBB && p[2] == 0xBF) {
        p += 3;
        n -= 3;
    }
    static const char kKey[] = "model:";
    const std::size_t keyLen = sizeof(kKey) - 1;
    for (std::size_t line = 0; line < n;) {
        // The line search first: a binary rarely has one, so it ends early.
        if (n - line >= keyLen && std::memcmp(p + line, kKey, keyLen) == 0)
            return std::memchr(p, 0, n) == nullptr && validUtf8(p, n);
        const void* nl = std::memchr(p + line, '\n', n - line);
        if (nl == nullptr) break;
        line = static_cast<std::size_t>(static_cast<const std::uint8_t*>(nl) - p) + 1;
    }
    return false;
}

Classified classify(const OpenedFile& f, Way way, const OpenSetup& s) {
    Classified out;
    const std::string name = fileNameOf(f);
    const std::string lower = machine::lowerAscii(name);
    if (!f.bytes) {   // 1a
        out.refusal = name + " could not be read.";
        return out;
    }
    if (const auto limit = limitOf(way, s); limit && f.bytes->size() > *limit) {   // 1b
        out.refusal = name + " is larger than " + sizeText(*limit) + " and is not opened.";
        return out;
    }
    if (!name.empty() && name[0] == '.') {   // 1c
        out.refusal = name + " is a hidden file and is not opened.";
        return out;
    }
    if (s.presets && hasAny(lower, s.presets->extensions())) {   // 2: whatever it holds
        out.what = Classified::What::Preset;
        return out;
    }
    if (s.media && hasAny(lower, s.media->suffixes())) {   // 3
        out.what = Classified::What::Medium;
        return out;
    }
    for (FileKind* k : s.kinds)   // 4, in the product's order
        if (k && k->takes(f)) {
            out.what = Classified::What::Kind;
            out.kind = k;
            return out;
        }
    if (s.presets && looksLikePreset(*f.bytes)) {   // 5
        out.what = Classified::What::Preset;
        return out;
    }
    out.refusal = s.productName + " cannot open " + name + ".";   // 6
    return out;
}

bool wouldTake(std::string_view name, std::optional<std::uint64_t> size, const OpenSetup& s) {
    if (const auto limit = limitOf(Way::Drop, s); limit && size && *size > *limit) return false;   // 1b
    if (name.empty() || name[0] == '.') return false;                                           // 1c
    const std::string lower = machine::lowerAscii(name);
    if (s.presets && hasAny(lower, s.presets->extensions())) return true;   // 2
    if (s.media && hasAny(lower, s.media->suffixes())) return true;         // 3
    for (FileKind* k : s.kinds)
        if (k && k->takesName(lower)) return true;
    return false;
}

} // namespace calcu::opening
