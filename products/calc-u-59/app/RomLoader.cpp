#include "RomLoader.hpp"
#include "../core/TICatalogue.hpp"
#include "TextUtil.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>

namespace cu59 {

std::optional<std::vector<uint8_t>> parseModuleTxt(std::string_view text) {
    constexpr int kMaxBytes = 5000;
    std::vector<uint8_t> bytes(kMaxBytes, 0);
    int maxAddr = 0;
    bool inData = false;
    for (std::string_view line : splitLines(text)) {
        std::string_view s = trim(line);
        if (!inData) {
            if (startsWith(s, "---")) inData = true;
            continue;
        }
        // Some dumps include an "ADDR: BCD DATA" header; others start with rows.
        std::string upper = toUpper(s);
        if (startsWith(upper, "ADDR:") && upper.find("DATA") != std::string::npos) continue;
        if (s.empty()) continue;
        size_t colon = s.find(':');
        if (colon == std::string_view::npos) continue;
        auto addr = parseDec(trim(s.substr(0, colon)));
        if (!addr) continue;
        // BCD values: "21" means 0x21, not decimal 21. Unparseable tokens are dropped.
        int i = 0;
        for (std::string_view tok : splitWords(s.substr(colon + 1))) {
            auto v = parseHex(tok);
            if (!v || *v > 0xFF) continue;
            long idx = *addr + i++;
            if (idx >= 0 && idx < kMaxBytes) {
                bytes[static_cast<size_t>(idx)] = static_cast<uint8_t>(*v);
                maxAddr = std::max(maxAddr, static_cast<int>(idx) + 1);
            }
        }
    }
    if (maxAddr == 0) return std::nullopt;
    bytes.resize(static_cast<size_t>(maxAddr));
    return bytes;
}

namespace {

std::string readOrThrow(const std::string& name, const ResourceReader& read) {
    auto text = read(name);
    if (!text) throw RomError("ROM file not found: " + name);
    return *text;
}

} // namespace

std::vector<calcu::machine::RomImage> loadRomImages(Model model, const ResourceReader& read) {
    std::vector<calcu::machine::RomImage> images;
    auto add = [&](const char* name) {
        std::string text = readOrThrow(name, read);
        images.push_back({name, std::vector<uint8_t>(text.begin(), text.end())});
    };
    for (const char* name : info(model).romFiles) add(name);
    for (const char* name : info(model).constFiles) add(name);
    return images;
}

std::unique_ptr<calcu::machine::Machine> buildMachine(Model model, const ResourceReader& read,
                                                      std::shared_ptr<const calcu::machine::ProductData> content) {
    calcu::machine::BuildRequest request;
    request.model = modelId(model);
    request.roms = loadRomImages(model, read);
    request.content = std::move(content);
    TIMachineBuilder builder;
    calcu::machine::BuildResult r = calcu::machine::build(tiCatalogue(), builder, request);
    if (!r.machine) throw RomError(r.missingRoms.empty() ? r.refusal : "ROM file not found: " + r.missingRoms.front());
    return std::move(r.machine);
}

ResourceReader directoryReader(std::vector<std::string> dirs) {
    return [dirs = std::move(dirs)](const std::string& name) -> std::optional<std::string> {
        for (const auto& dir : dirs) {
            std::ifstream f(dir + "/" + name, std::ios::binary);
            if (!f) continue;
            std::ostringstream ss;
            ss << f.rdbuf();
            return ss.str();
        }
        return std::nullopt;
    };
}

std::optional<std::vector<uint8_t>> loadModule(const std::string& romFile, const ResourceReader& read) {
    auto text = read(romFile);
    if (!text) return std::nullopt;
    return parseModuleTxt(*text);
}

} // namespace cu59
