#include "ModuleFile.hpp"
#include "formats/Yaml.hpp"
#include <optional>

using calcu::formats::YamlNode;

namespace {

int hexDigit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

std::optional<std::uint8_t> hexByte(std::string_view s) {
    if (s.size() != 2 || hexDigit(s[0]) < 0 || hexDigit(s[1]) < 0) return std::nullopt;
    return static_cast<std::uint8_t>(hexDigit(s[0]) * 16 + hexDigit(s[1]));
}

std::vector<std::string_view> lines(std::string_view text) {
    std::vector<std::string_view> out;
    size_t start = 0;
    for (size_t i = 0; i <= text.size(); ++i)
        if (i == text.size() || text[i] == '\n') {
            out.push_back(text.substr(start, i - start));
            start = i + 1;
        }
    return out;
}

std::vector<std::string_view> words(std::string_view s) {
    std::vector<std::string_view> out;
    size_t start = 0;
    for (size_t i = 0; i <= s.size(); ++i)
        if (i == s.size() || s[i] == ' ' || s[i] == '\t') {
            if (i > start) out.push_back(s.substr(start, i - start));
            start = i + 1;
        }
    return out;
}

// docs/formats/README.md §1.1, `addressed-hex`: `$XXXX: <1–16 byte pairs>`, or
// `$XXXX: XX...` repeating XX up to the next line's address (or the end);
// the lines cover 0 … size − 1 exactly, in address order.
bool addressedHex(std::string_view block, size_t size, std::vector<std::uint8_t>& out, std::string& error) {
    out.clear();
    std::optional<std::uint8_t> run;
    for (std::string_view line : lines(block)) {
        auto w = words(line);
        if (w.empty()) continue;
        std::string_view a = w[0];
        if (a.size() < 3 || a.front() != '$' || a.back() != ':') {
            error = "rom.bytes: expected \"$XXXX:\" at the start of \"" + std::string(line) + "\"";
            return false;
        }
        unsigned long addr = 0;
        for (char c : a.substr(1, a.size() - 2)) {
            if (hexDigit(c) < 0) {
                error = "rom.bytes: bad address " + std::string(a);
                return false;
            }
            addr = addr * 16 + static_cast<unsigned long>(hexDigit(c));
        }
        if (run) {
            if (addr < out.size()) {
                error = "rom.bytes: address " + std::string(a) + " overlaps the run before it";
                return false;
            }
            out.resize(addr, *run);
            run.reset();
        }
        if (addr != out.size()) {
            error = "rom.bytes: address " + std::string(a) + (addr < out.size() ? " overlaps" : " leaves a gap");
            return false;
        }
        if (w.size() == 2 && w[1].size() == 5 && w[1].substr(2) == "...") {
            run = hexByte(w[1].substr(0, 2));
            if (!run) {
                error = "rom.bytes: bad run " + std::string(w[1]);
                return false;
            }
            continue;
        }
        if (w.size() < 2 || w.size() > 17) {
            error = "rom.bytes: a line holds 1 to 16 bytes, at " + std::string(a);
            return false;
        }
        for (size_t i = 1; i < w.size(); ++i) {
            auto b = hexByte(w[i]);
            if (!b) {
                error = "rom.bytes: bad byte \"" + std::string(w[i]) + "\" at " + std::string(a);
                return false;
            }
            out.push_back(*b);
        }
    }
    if (run) out.resize(size > out.size() ? size : out.size() + 1, *run);
    if (out.size() != size) {
        error = "rom.bytes covers " + std::to_string(out.size()) + " bytes, rom.size is " + std::to_string(size);
        return false;
    }
    return true;
}

// `hex`: byte pairs, whitespace and newlines ignored.
bool plainHex(std::string_view block, size_t size, std::vector<std::uint8_t>& out, std::string& error) {
    out.clear();
    int high = -1;
    for (char c : block) {
        if (c == ' ' || c == '\t' || c == '\n') continue;
        int d = hexDigit(c);
        if (d < 0) {
            error = std::string("rom.bytes: not a hex digit: ") + c;
            return false;
        }
        if (high < 0) high = d;
        else {
            out.push_back(static_cast<std::uint8_t>(high * 16 + d));
            high = -1;
        }
    }
    if (high >= 0 || out.size() != size) {
        error = "rom.bytes covers " + std::to_string(out.size()) + " bytes, rom.size is " + std::to_string(size);
        return false;
    }
    return true;
}

// `base64`: whitespace and newlines ignored.
bool base64(std::string_view block, size_t size, std::vector<std::uint8_t>& out, std::string& error) {
    out.clear();
    auto value = [](char c) -> int {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return c - 'a' + 26;
        if (c >= '0' && c <= '9') return c - '0' + 52;
        if (c == '+') return 62;
        if (c == '/') return 63;
        return -1;
    };
    unsigned bits = 0;
    int count = 0;
    for (char c : block) {
        if (c == ' ' || c == '\t' || c == '\n') continue;
        if (c == '=') break;
        int v = value(c);
        if (v < 0) {
            error = std::string("rom.bytes: not a base64 character: ") + c;
            return false;
        }
        bits = (bits << 6) | static_cast<unsigned>(v);
        count += 6;
        if (count >= 8) {
            count -= 8;
            out.push_back(static_cast<std::uint8_t>((bits >> count) & 0xFF));
        }
    }
    if (out.size() != size) {
        error = "rom.bytes covers " + std::to_string(out.size()) + " bytes, rom.size is " + std::to_string(size);
        return false;
    }
    return true;
}

bool scalar(const YamlNode& root, const char* key, std::string& out, std::string& error) {
    const YamlNode* n = root.find(key);
    if (!n) {
        error = std::string("missing '") + key + "'";
        return false;
    }
    return n->asString(&out, &error);
}

} // namespace

TIModuleFile readModuleFile(std::string_view text, std::string& error) {
    error.clear();
    // \r\n and \r become \n (LIB-12 EC-2), before the YAML reader sees the text.
    std::string normalised;
    normalised.reserve(text.size());
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\r') {
            normalised += '\n';
            if (i + 1 < text.size() && text[i + 1] == '\n') ++i;
        } else {
            normalised += text[i];
        }
    }
    YamlNode root;
    if (!calcu::formats::parseYaml(normalised, &root, &error)) return {};
    if (!root.isMap()) {
        error = "not a module file";
        return {};
    }
    if (!calcu::formats::requireFormatVersion(root, 1, "solid-state-rom-module", &error)) return {};
    TIModuleFile out;
    std::string format;
    if (!scalar(root, "format", format, error)) return {};
    if (format != "solid-state-rom-module") {
        error = "format is '" + format + "', not 'solid-state-rom-module'";
        return {};
    }
    if (!scalar(root, "module-name", out.moduleName, error)) return {};
    if (!scalar(root, "description", out.description, error)) return {};
    // Optional (docs/formats/modules.md §2.4): the title and the picker's order.
    if (root.find("title") && !scalar(root, "title", out.title, error)) return {};
    if (const YamlNode* sort = root.find("menu-sort")) {
        long v = 0;
        if (!sort->asInt(&v, &error)) return {};
        out.menuSort = static_cast<int>(v);
    }
    const YamlNode* rom = root.find("rom");
    if (!rom || !rom->isMap()) {
        error = "missing 'rom'";
        return {};
    }
    long size = 0;
    const YamlNode* sizeNode = rom->find("size");
    if (!sizeNode) {
        error = "missing 'rom.size'";
        return {};
    }
    if (!sizeNode->asInt(&size, &error)) return {};
    if (size < 1 || static_cast<unsigned long>(size) > kModuleRomMaxBytes) {
        error = "rom.size " + std::to_string(size) + " is not 1 to 5000";
        return {};
    }
    std::string encoding, bytes;
    if (!scalar(*rom, "encoding", encoding, error)) return {};
    if (!scalar(*rom, "bytes", bytes, error)) return {};
    const size_t n = static_cast<size_t>(size);
    bool ok = false;
    if (encoding == "addressed-hex") ok = addressedHex(bytes, n, out.rom, error);
    else if (encoding == "hex") ok = plainHex(bytes, n, out.rom, error);
    else if (encoding == "base64") ok = base64(bytes, n, out.rom, error);
    else error = "rom.encoding '" + encoding + "' is not addressed-hex, hex or base64";
    if (!ok) return {};
    return out;
}
