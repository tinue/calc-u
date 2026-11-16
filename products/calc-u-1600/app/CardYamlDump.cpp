#include "app/CardYamlDump.hpp"

#include <cstdint>

#include "Connector/BatteryCardInstance.hpp"
#include "Connector/ExpansionCard.hpp"
#include "Debug/Inspect/HexDump.hpp"
#include "app/SharpAppPart.hpp"
#include "app/Text.hpp"

namespace shared {

std::vector<std::string> cardYamlDump(SharpAppPart& app) {
    using inspect::fmt;
    const bool isPC1600 = app.currentModel() == Model::PC1600;
    std::vector<std::string> chunks;
    for (int slot = 1; slot <= app.slotCount(); ++slot) {
        int bankCount = 0;
        const std::vector<std::uint8_t> image = app.debugSlotCardImage(slot, &bankCount);
        // bankCount <= 0 is an unbanked card (e.g. CE-1600M): one implicit
        // bank, written without a `bank:` key.
        if (image.empty() || (bankCount > 0 && image.size() % static_cast<std::size_t>(bankCount) != 0)) continue;
        const calcu::media::Binding* bound = app.mediaPart().bindingOf(slot == 2 ? "slot-2" : "slot-1");
        const ExpansionCard* card = app.slotCard(slot);
        const std::string name = bound ? bound->name : card ? card->moduleName() : std::string();
        const std::string label = name.empty() ? "attached module" : name;
        const std::string slotLabel = isPC1600 ? fmt(" (Slot %d)", slot) : "";
        const std::string layout =
            bankCount > 0 ? fmt("%d bank(s) x %s", bankCount,
                                inspect::sizeLabel(static_cast<std::uint32_t>(image.size() / bankCount)).c_str())
                          : fmt("unbanked, %s", inspect::sizeLabel(static_cast<std::uint32_t>(image.size())).c_str());
        chunks.push_back(fmt("\xE2\x94\x80\xE2\x94\x80 Module card dump: %s%s, %s -- paste under the region's "
                             "initial-content: \xE2\x94\x80\xE2\x94\x80",
                             label.c_str(), slotLabel.c_str(), layout.c_str()));
        chunks.push_back(joinLines(formatBatteryCardInitialContentBlock(bankCount, image)));
    }
    if (chunks.empty())
        chunks.push_back("\xE2\x94\x80\xE2\x94\x80 Module card dump: no card attached, or it has no dumpable RAM "
                         "\xE2\x94\x80\xE2\x94\x80");
    return chunks;
}

} // namespace shared
