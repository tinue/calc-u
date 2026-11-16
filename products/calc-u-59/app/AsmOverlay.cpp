#include "AsmOverlay.hpp"
#include "TextUtil.hpp"

namespace cu59 {

namespace {

// Lines ending with LF or CRLF (a lone CR is part of its line's text).
std::vector<std::string_view> lines(std::string_view text) {
    std::vector<std::string_view> out;
    size_t start = 0;
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] != '\n') continue;
        size_t end = i;
        if (end > start && text[end - 1] == '\r') --end;
        out.push_back(text.substr(start, end - start));
        start = i + 1;
    }
    if (start < text.size()) {
        std::string_view last = text.substr(start);
        if (!last.empty() && last.back() == '\r') last.remove_suffix(1);
        out.push_back(last);
    }
    return out;
}

bool blank(char c) { return c == ' ' || c == '\t'; }

std::string_view trimBlanks(std::string_view s) {
    while (!s.empty() && blank(s.front())) s.remove_prefix(1);
    while (!s.empty() && blank(s.back())) s.remove_suffix(1);
    return s;
}

} // namespace

std::variant<std::vector<std::uint16_t>, std::string> readAsmOverlay(std::string_view name, std::string_view text) {
    const std::string file(name);
    const std::vector<std::string_view> all = lines(text);
    if (all.empty() || all.front() != kAsmOverlayFirstLine)
        return file + " is an ASM overlay in an older format. This release reads the ASM overlay format version 2.0, "
                      "described in the help's reference chapter.";
    size_t hex = all.size();
    for (size_t i = 1; i < all.size(); ++i)
        if (toUpper(trimBlanks(all[i])) == "HEX:") {
            hex = i;
            break;
        }
    if (hex == all.size()) return file + " has no HEX: line.";

    std::vector<std::uint16_t> words;
    for (size_t i = hex + 1; i < all.size(); ++i) {
        std::string_view line = all[i];
        if (const size_t c = line.find(';'); c != std::string_view::npos) line = line.substr(0, c);
        const std::string where = file + ", line " + std::to_string(i + 1) + ": ";
        size_t p = 0;
        while (p < line.size()) {
            if (blank(line[p])) {
                ++p;
                continue;
            }
            size_t e = p;
            while (e < line.size() && !blank(line[e])) ++e;
            const std::string_view word = line.substr(p, e - p);
            p = e;
            std::string_view run = word;
            if (run.size() > 2 && run[0] == '0' && (run[1] == 'x' || run[1] == 'X')) run.remove_prefix(2);
            bool digits = !run.empty();
            for (char ch : run) digits = digits && hexDigit(ch) >= 0;
            if (!digits) return where + std::string(word) + " is neither hexadecimal digits nor a comment.";
            if (run.size() % 4 != 0) return where + std::string(word) + " is not a multiple of four hexadecimal digits.";
            for (size_t k = 0; k < run.size(); k += 4) {
                const std::string_view four = run.substr(k, 4);
                const auto v = parseHex(four);
                if (!v || *v > 0x1FFF) return where + std::string(four) + " is above 1FFF.";
                words.push_back(static_cast<std::uint16_t>(*v));
            }
        }
    }
    if (words.empty()) return file + " has no instructions after HEX:.";
    if (words.size() > 2048)
        return file + " holds " + std::to_string(words.size()) + " instructions; at most 2048 fit from 1800 to 1FFF.";
    return words;
}

} // namespace cu59
