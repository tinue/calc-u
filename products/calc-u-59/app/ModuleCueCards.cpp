#include "ModuleCueCards.hpp"
#include "CueCardYaml.hpp"
#include "formats/Flow.hpp"
#include "formats/Yaml.hpp"

namespace cu59 {

using calcu::formats::YamlNode;

std::map<int, CueCardContent> moduleCueCards(const calcu::roms::RomSet& roms, std::string_view moduleID,
                                             const std::function<void(const std::string&)>& problem) {
    std::map<int, CueCardContent> out;
    if (moduleID.empty()) return out;
    const std::string file = std::string(moduleID) + ".rom.yaml";
    auto fail = [&](const std::string& why) {
        if (problem) problem("The cue cards of " + file + " cannot be read: " + why);
        return std::map<int, CueCardContent>{};
    };
    const auto* bytes = roms.bytes(file);
    if (!bytes) return fail("the file is not in the ROM set.");
    YamlNode root;
    std::string error;
    if (!calcu::formats::parseYaml(std::string(bytes->begin(), bytes->end()), &root, &error)) return fail(error);
    const YamlNode* cards = root.find("cue-cards");
    if (!cards) return out;
    if (!cards->isSeq()) return fail("cue-cards: is a list.");
    for (const YamlNode& item : cards->seq) {
        if (!item.isMap()) return fail("line " + std::to_string(item.line) + ": a cue card is a block.");
        const YamlNode* program = item.find("program");
        int n = 0;
        try {
            n = program ? std::stoi(calcu::formats::scalarText(*program)) : 0;
        } catch (...) {
            n = 0;
        }
        if (n < 1 || n > 99) return fail("line " + std::to_string(item.line) + ": program: is 1 to 99.");
        YamlNode block = item;   // the cue card block without the module's keys
        std::string id;
        block.map.clear();
        for (const auto& [k, v] : item.map) {
            if (k == "program") continue;
            if (k == "id") {
                id = calcu::formats::scalarText(v);
                continue;
            }
            block.map.emplace_back(k, v);
        }
        std::string why;
        auto card = readStandAloneCueCard(block, [&](int line, const std::string& m) {
            if (why.empty()) why = "line " + std::to_string(line) + ": " + m;
        });
        if (!card) return fail(why);
        card->template_ = CueCardTemplate::SolidState;
        card->id = id;
        if (!out.emplace(n, *card).second) return fail("program " + std::to_string(n) + " has two cue cards.");
    }
    return out;
}

} // namespace cu59
