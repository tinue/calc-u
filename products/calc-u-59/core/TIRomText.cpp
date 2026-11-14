#include "TIRomText.hpp"
#include <optional>

namespace {

// ASCII versions of app/TextUtil.hpp's helpers (the dumps are ASCII; their md5
// values are known, LIB-12 AC-9).
std::vector<std::string_view> splitLines(std::string_view text) {
    std::vector<std::string_view> out;
    size_t start = 0;
    for (size_t i = 0; i < text.size(); ++i)
        if (text[i] == '\n' || text[i] == '\r') {
            out.push_back(text.substr(start, i - start));
            start = i + 1;
        }
    out.push_back(text.substr(start));
    return out;
}
std::string_view trim(std::string_view s) {
    size_t b = 0, e = s.size();
    while (b < e && (s[b] == ' ' || s[b] == '\t')) ++b;
    while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t')) --e;
    return s.substr(b, e - b);
}
std::vector<std::string_view> splitWords(std::string_view s) {
    std::vector<std::string_view> out;
    size_t start = 0;
    for (size_t i = 0; i <= s.size(); ++i)
        if (i == s.size() || s[i] == ' ') {
            if (i > start) out.push_back(s.substr(start, i - start));
            start = i + 1;
        }
    return out;
}
bool startsWith(std::string_view s, std::string_view p) { return s.substr(0, p.size()) == p; }
int hexDigit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}
std::optional<unsigned long> parseHex(std::string_view s) {
    if (!s.empty() && s[0] == '+') s.remove_prefix(1);
    if (s.empty() || s.size() > 8) return std::nullopt;
    unsigned long v = 0;
    for (char c : s) {
        int d = hexDigit(c);
        if (d < 0) return std::nullopt;
        v = v * 16 + static_cast<unsigned long>(d);
    }
    return v;
}
bool isDec(std::string_view s) {
    if (!s.empty() && (s[0] == '+' || s[0] == '-')) s.remove_prefix(1);
    if (s.empty() || s.size() > 18) return false;
    for (char c : s)
        if (c < '0' || c > '9') return false;
    return true;
}
std::string hex4(uint16_t v) {
    static const char* digits = "0123456789ABCDEF";
    std::string s;
    for (int sh = 12; sh >= 0; sh -= 4) s += digits[(v >> sh) & 0xF];
    return s;
}

} // namespace

std::vector<uint16_t> decodeRomText(std::string_view text, std::string& error) {
    std::vector<uint16_t> words;
    bool inData = false;
    for (std::string_view line : splitLines(text)) {
        std::string_view s = trim(line);
        if (!inData) {
            if (startsWith(s, "---")) inData = true;
            continue;
        }
        if (s.empty()) continue;
        size_t colon = s.find(':');
        if (colon == std::string_view::npos) continue;
        std::string_view addr = trim(s.substr(0, colon));
        if (addr.size() != 4 || !parseHex(addr)) continue;
        for (std::string_view w : splitWords(s.substr(colon + 1))) {
            auto v = parseHex(w);
            if (!v || *v > 0xFFFF) {
                error = "invalid hex word: " + std::string(w);
                return {};
            }
            words.push_back(static_cast<uint16_t>(*v & 0x1FFF));
        }
    }
    return words;
}

std::vector<std::vector<uint8_t>> decodeConstantsText(std::string_view text, std::string& error) {
    std::vector<std::vector<uint8_t>> rows;
    bool inNumber = false;
    for (std::string_view line : splitLines(text)) {
        std::string_view s = trim(line);
        if (s.find("CONSTANT ROM (NUMBER)") != std::string_view::npos) {
            inNumber = true;
            continue;
        }
        if (startsWith(s, "---")) {
            if (inNumber) break;
            continue;
        }
        if (!inNumber || s.empty()) continue;
        size_t colon = s.find(':');
        if (colon == std::string_view::npos) continue;
        std::string_view addr = trim(s.substr(0, colon));
        if (!isDec(addr)) continue;
        std::string_view hex = trim(s.substr(colon + 1));
        if (hex.size() != 16) continue;
        std::vector<uint8_t> nibbles;
        for (auto it = hex.rbegin(); it != hex.rend(); ++it) {
            int n = hexDigit(*it);
            if (n < 0) {
                error = std::string("invalid hex nibble: ") + *it;
                return {};
            }
            nibbles.push_back(static_cast<uint8_t>(n));
        }
        rows.push_back(std::move(nibbles));
    }
    return rows;
}

TIRoms decodeRomSet(MachineVariant v, const std::vector<std::string_view>& romTexts,
                    const std::vector<std::string_view>& constantTexts, std::string& error) {
    constexpr size_t kWords = 6144, kRows = 64;
    TIRoms out;
    for (auto text : romTexts) {
        auto part = decodeRomText(text, error);
        if (!error.empty()) return {};
        out.words.insert(out.words.end(), part.begin(), part.end());
    }
    if (out.words.size() != kWords) {
        error = "wrong ROM word count: " + std::to_string(out.words.size());
        return {};
    }
    if (hasLargeMemory(v)) {
        // Sentinels from the known-good TI-59 ROM image: words[0] = 0x0A01 (the
        // first instruction, CLR IDL); words[6143] = 0x1987 (the last). A
        // mismatch means the file is corrupt, truncated or the wrong variant.
        if (out.words[0] != 0x0A01) error = "words[0] expected 0x0A01, got 0x" + hex4(out.words[0]);
        else if (out.words[6143] != 0x1987) error = "words[6143] expected 0x1987, got 0x" + hex4(out.words[6143]);
        if (!error.empty()) return {};
    }
    size_t rows = 0;
    for (auto text : constantTexts) {
        for (const auto& r : decodeConstantsText(text, error)) {
            out.constants.insert(out.constants.end(), r.begin(), r.end());
            ++rows;
        }
        if (!error.empty()) return {};
    }
    if (rows != kRows) {
        error = "expected 64 constant rows, got " + std::to_string(rows);
        return {};
    }
    return out;
}
