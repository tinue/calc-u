#include "Keycodes.hpp"
#include "formats/Flow.hpp"
#include "formats/Yaml.hpp"
#include <sstream>

namespace cu59 {

namespace {

bool allDigits(std::string_view s) {
    if (s.empty()) return false;
    for (char c : s)
        if (c < '0' || c > '9') return false;
    return true;
}

std::string uncommented(std::string_view line) {
    for (std::size_t i = 0; i < line.size(); ++i)
        if (line[i] == '#' && (i == 0 || line[i - 1] == ' ')) return std::string(line.substr(0, i));
    return std::string(line);
}

std::vector<std::string> tokens(const std::string& line) {
    std::vector<std::string> out;
    std::istringstream in(line);
    for (std::string t; in >> t;) out.push_back(t);
    return out;
}

std::string stepText(int step) {
    std::string s = std::to_string(step);
    while (s.size() < 3) s = "0" + s;
    return s;
}

} // namespace

std::variant<std::vector<ProgramStep>, KeycodeError> readKeycodes(std::string_view text, int lastStep) {
    std::vector<ProgramStep> out;
    int next = 0;
    int index = 0;
    std::size_t pos = 0;
    while (pos <= text.size()) {
        const std::size_t nl = text.find('\n', pos);
        std::string_view raw = text.substr(pos, nl == std::string_view::npos ? std::string_view::npos : nl - pos);
        pos = nl == std::string_view::npos ? text.size() + 1 : nl + 1;
        if (!raw.empty() && raw.back() == '\r') raw.remove_suffix(1);
        const std::vector<std::string> t = tokens(uncommented(raw));
        const int at = index++;
        if (t.empty()) continue;
        auto fail = [&](std::string m) { return KeycodeError{at, std::move(m)}; };
        if (t[0].size() >= 3 && allDigits(t[0])) {   // listing
            const int step = std::stoi(t[0]);
            if (step > lastStep)
                return fail("the step " + t[0] + " is beyond the last step, " + stepText(lastStep) + ".");
            if (t.size() < 2 || t[1].size() != 2 || !allDigits(t[1]))
                return fail("the step " + t[0] + " needs one keycode of two digits after it.");
            out.push_back({step, static_cast<std::uint8_t>(std::stoi(t[1]))});
            next = step + 1;
            continue;
        }
        for (const std::string& k : t) {   // compact
            if (k.size() != 2 || !allDigits(k))
                return fail("\"" + k + "\" is no keycode; a line of keycodes holds only two-digit keycodes (00 to 99).");
            if (next > lastStep)
                return fail("the keycodes go beyond the last step, " + stepText(lastStep) + ".");
            out.push_back({next++, static_cast<std::uint8_t>(std::stoi(k))});
        }
    }
    return out;
}

std::variant<KeystrokeProgram, std::string> readKeystrokeProgramFile(std::string_view text, int lastStep) {
    using calcu::formats::YamlNode;
    YamlNode root;
    std::string error;
    if (!calcu::formats::parseYaml(std::string(text), &root, &error)) return error;
    if (!root.isMap()) return std::string("A keystroke program file is a mapping of format, format-version, program-name and keycodes.");
    KeystrokeProgram p;
    for (const auto& [k, v] : root.map)
        if (k != "format" && k != "format-version" && k != "program-name" && k != "description" && k != "keycodes")
            return "line " + std::to_string(v.line) + ": unknown key " + k + ".";
    const YamlNode* format = root.find("format");
    if (!format || calcu::formats::scalarText(*format) != "ti-keystroke-program")
        return std::string("format: must be ti-keystroke-program.");
    const YamlNode* version = root.find("format-version");
    if (!version || calcu::formats::scalarText(*version) != "1")
        return std::string("format-version: must be 1.");
    const YamlNode* name = root.find("program-name");
    if (!name || !name->isScalar() || calcu::formats::scalarText(*name).empty())
        return std::string("program-name: is required.");
    p.name = calcu::formats::scalarText(*name);
    if (const YamlNode* d = root.find("description")) p.description = calcu::formats::scalarText(*d);
    const YamlNode* codes = root.find("keycodes");
    if (!codes || !codes->isScalar()) return std::string("keycodes: is required, as a block (keycodes: |).");
    auto r = readKeycodes(codes->scalar, lastStep);
    if (auto* e = std::get_if<KeycodeError>(&r)) {
        const std::vector<int> lines = blockLineNumbers(text, codes->line, static_cast<std::size_t>(e->lineIndex) + 1);
        const int line = static_cast<std::size_t>(e->lineIndex) < lines.size() ? lines[static_cast<std::size_t>(e->lineIndex)] : codes->line;
        return "line " + std::to_string(line) + ": " + e->message;
    }
    p.steps = std::get<std::vector<ProgramStep>>(std::move(r));
    return p;
}

std::vector<int> blockLineNumbers(std::string_view fileText, int firstLine, std::size_t count) {
    std::vector<int> out;
    int lineNo = 0;
    std::size_t pos = 0;
    while (pos <= fileText.size() && out.size() < count) {
        const std::size_t nl = fileText.find('\n', pos);
        std::string_view raw = fileText.substr(pos, nl == std::string_view::npos ? std::string_view::npos : nl - pos);
        pos = nl == std::string_view::npos ? fileText.size() + 1 : nl + 1;
        ++lineNo;
        if (lineNo < firstLine) continue;
        const std::string content = uncommented(raw);
        if (content.find_first_not_of(" \r") == std::string::npos) continue;   // blank or comment only
        out.push_back(lineNo);
    }
    return out;
}

} // namespace cu59
