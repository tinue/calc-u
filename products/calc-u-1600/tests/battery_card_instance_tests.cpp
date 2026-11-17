// Headless C++ tests for Core/Connector/BatteryCardInstance.hpp -- the
// addressed-hex writer + battery-card instance splice logic used by the
// Qt6 prototype's "Name & Save" / autosave flow. Same no-framework,
// assert-and-tally style as memory_card_tests.cpp.
// Build & run: see tools/run_tests.sh

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "TempFolder.hpp"
#include "../core/Connector/BatteryCardInstance.hpp"
#include "../core/Connector/FloppyImageFile.hpp"
#include "../core/Connector/MemoryCardCatalog.hpp"
#include "Files.hpp"
#include "../core/Connector/MemoryCardDefinition.hpp"
#include "../core/Connector/SoftwareDefinedCard.hpp"

#include <filesystem>
#include "TestHarness.hpp"

namespace {

std::vector<uint8_t> uniformBank(size_t n, uint8_t v) { return std::vector<uint8_t>(n, v); }

TEST(test_addressed_hex_roundtrip_uniform) {
    auto bank = uniformBank(0x4000, 0xFF);
    auto lines = formatAddressedHexLines(bank);
    CHECK(lines.size() == 1);  // whole bank collapses to one run line

    std::string text;
    for (const auto& l : lines) { text += l; text += "\n"; }
    std::vector<uint8_t> out;
    std::string err;
    CHECK(mcd_detail::parseAddressedHex(text, static_cast<uint32_t>(bank.size()), &out, &err));
    CHECK(out == bank);
}

TEST(test_addressed_hex_roundtrip_mixed) {
    std::vector<uint8_t> bank(0x100, 0);
    for (size_t i = 0; i < bank.size(); ++i) bank[i] = static_cast<uint8_t>(i * 7 + 3);
    auto lines = formatAddressedHexLines(bank);
    CHECK(lines.size() == bank.size() / 16);  // no run collapsing possible

    std::string text;
    for (const auto& l : lines) { text += l; text += "\n"; }
    std::vector<uint8_t> out;
    std::string err;
    CHECK(mcd_detail::parseAddressedHex(text, static_cast<uint32_t>(bank.size()), &out, &err));
    CHECK(out == bank);
}

TEST(test_addressed_hex_roundtrip_uniform_run_plus_tail) {
    // A long uniform run followed by a differing tail -- the run must
    // collapse exactly up to the first row that isn't uniform.
    std::vector<uint8_t> bank(0x40, 0x00);
    for (size_t i = 0x30; i < bank.size(); ++i) bank[i] = static_cast<uint8_t>(i);
    auto lines = formatAddressedHexLines(bank);
    // Rows 0x00,0x10,0x20 uniform-collapse into one run line; row 0x30 is
    // the mixed tail -> one more line.
    CHECK(lines.size() == 2);

    std::string text;
    for (const auto& l : lines) { text += l; text += "\n"; }
    std::vector<uint8_t> out;
    std::string err;
    CHECK(mcd_detail::parseAddressedHex(text, static_cast<uint32_t>(bank.size()), &out, &err));
    CHECK(out == bank);
}

TEST(test_addressed_hex_single_differing_byte) {
    std::vector<uint8_t> bank(0x20, 0xAA);
    bank[5] = 0x11;
    auto lines = formatAddressedHexLines(bank);
    CHECK(lines.size() == 2);  // row 0 (mixed), row 1 (uniform)

    std::string text;
    for (const auto& l : lines) { text += l; text += "\n"; }
    std::vector<uint8_t> out;
    std::string err;
    CHECK(mcd_detail::parseAddressedHex(text, static_cast<uint32_t>(bank.size()), &out, &err));
    CHECK(out == bank);
}

TEST(test_battery_card_block_keeps_uniform_banks) {
    // A uniform bank must still be written: dropped, it would reload at the
    // region's power-up-fill (0xFF here), not the 0x00 it held.
    std::vector<uint8_t> image;
    auto zeroBank = uniformBank(0x100, 0x00);
    std::vector<uint8_t> mixedBank(0x100, 0);
    mixedBank[10] = 0x42;
    image.insert(image.end(), zeroBank.begin(), zeroBank.end());
    image.insert(image.end(), mixedBank.begin(), mixedBank.end());

    auto lines = formatBatteryCardInitialContentBlock(2, image);
    bool sawBank0 = false;
    bool sawBank1 = false;
    for (const auto& l : lines) {
        if (l.find("- bank: 0") != std::string::npos) sawBank0 = true;
        if (l.find("- bank: 1") != std::string::npos) sawBank1 = true;
    }
    CHECK(sawBank0);
    CHECK(sawBank1);

    std::string yaml =
        "format-version: 1\nmodule-name: X\n"
        "compatible-hosts: [PC-1500]\n"
        "definition-terminology: PC-1500\n"
        "regions:\n"
        "  - name: sram\n"
        "    content: { kind: regular, writable: true, power-up-fill: 0xFF }\n"
        "    addressing: { chip-select: Y0 }\n"
        "    banking:\n"
        "      latch: { type: trigger-based, trigger: { pin: 16 }, source-domain: address, sampled-lines: [A0] }\n"
        "      bank-count: 2\n"
        "      bank-size: 0x0100\n"
        "      bank-window: { chip-select: Y0, span: 0x0100 }\n";
    for (const auto& l : lines) { yaml += l; yaml += "\n"; }
    MemoryCardDefinition def;
    std::string err;
    CHECK(parseMemoryCardDefinition(yaml, &def, &err));
    if (!err.empty()) std::fprintf(stderr, "  (parse error: %s)\n", err.c_str());
    if (!def.regions.empty()) {
        auto it = def.regions[0].initialContentByBank.find(0);
        CHECK(it != def.regions[0].initialContentByBank.end());
        if (it != def.regions[0].initialContentByBank.end()) CHECK(it->second == zeroBank);
    }
}

// Minimal single-region, single-bank battery-style module definition, for
// splice/parse round-tripping without depending on a real bundled file.
// Real battery cards (CE-1638/CE-1601M/superram) are always *banked* --
// initial-content:'s `bank:` field is only valid on a banked region -- so
// this uses a trivial trigger-based banking with bank-count: 1 to match
// that shape rather than an unbanked region.
const char* kSyntheticCardYaml =
    "format-version: 1\nmodule-name: Synthetic\n"
    "compatible-hosts: [PC-1500]\n"
    "definition-terminology: PC-1500\n"
    "battery: true\n"
    "regions:\n"
    "  - name: sram\n"
    "    content: { kind: regular, writable: true, power-up-fill: 0xFF }\n"
    "    addressing: { chip-select: Y0 }\n"
    "    banking:\n"
    "      latch: { type: trigger-based, trigger: { pin: 16 }, source-domain: address, sampled-lines: [A0] }\n"
    "      bank-count: 1\n"
    "      bank-size: 0x0100\n"
    "      bank-window: { chip-select: Y0, span: 0x0100 }\n";

TEST(test_battery_card_block_never_empty_when_all_banks_uniform) {
    // A freshly-attached, never-written-to card: every bank sits at its
    // power-up-fill value. Core's parseInitialContent() rejects an empty
    // 'blocks' list, so the writer must emit explicit blocks here too.
    std::vector<uint8_t> image(2 * 0x100, 0xFF);
    auto lines = formatBatteryCardInitialContentBlock(2, image);
    bool sawBankEntry = false;
    for (const auto& l : lines)
        if (l.find("- bank:") != std::string::npos) sawBankEntry = true;
    CHECK(sawBankEntry);

    // The resulting block must parse back through the real Core parser,
    // which is exactly what enforces the *.module.yaml non-empty-blocks
    // requirement.
    std::string yaml =
        "format-version: 1\nmodule-name: X\n"
        "compatible-hosts: [PC-1500]\n"
        "definition-terminology: PC-1500\n"
        "regions:\n"
        "  - name: sram\n"
        "    content: { kind: regular, writable: true, power-up-fill: 0xFF }\n"
        "    addressing: { chip-select: Y0 }\n"
        "    banking:\n"
        "      latch: { type: trigger-based, trigger: { pin: 16 }, source-domain: address, sampled-lines: [A0] }\n"
        "      bank-count: 2\n"
        "      bank-size: 0x0100\n"
        "      bank-window: { chip-select: Y0, span: 0x0100 }\n";
    for (const auto& l : lines) { yaml += l; yaml += "\n"; }
    MemoryCardDefinition def;
    std::string err;
    CHECK(parseMemoryCardDefinition(yaml, &def, &err));
    if (!err.empty()) std::fprintf(stderr, "  (parse error: %s)\n", err.c_str());
}

TEST(test_battery_card_block_unbanked_region_omits_bank_key) {
    // An unbanked battery card (e.g. CE-1600M: "banking: none",
    // capacity: 0x8000) reports debugBankCount() == -1 (ExpansionCard's
    // "no bank concept" convention) -- formatBatteryCardInitialContentBlock
    // must then omit the 'bank:' key entirely (Core's parseInitialContent
    // rejects 'bank' on an unbanked region's blocks).
    std::vector<uint8_t> image(0x100, 0);
    image[3] = 0x99;
    auto lines = formatBatteryCardInitialContentBlock(-1, image);
    for (const auto& l : lines) CHECK(l.find("bank:") == std::string::npos);
    bool sawOffset = false;
    for (const auto& l : lines)
        if (l.find("- offset: 0x0000") != std::string::npos) sawOffset = true;
    CHECK(sawOffset);

    const char* unbankedCard =
        "format-version: 1\nmodule-name: X\n"
        "compatible-hosts: [PC-1500]\n"
        "definition-terminology: PC-1500\n"
        "regions:\n"
        "  - name: sram\n"
        "    capacity: 0x0100\n"
        "    banking: none\n"
        "    content: { kind: regular, writable: true, power-up-fill: 0xFF }\n"
        "    addressing: { chip-select: Y0, span: 0x0100 }\n";
    std::string yaml = unbankedCard;
    for (const auto& l : lines) { yaml += l; yaml += "\n"; }
    MemoryCardDefinition def;
    std::string err;
    CHECK(parseMemoryCardDefinition(yaml, &def, &err));
    if (!err.empty()) std::fprintf(stderr, "  (parse error: %s)\n", err.c_str());
    if (!def.regions.empty()) {
        auto it = def.regions[0].initialContentByBank.find(0);
        CHECK(it != def.regions[0].initialContentByBank.end());
        if (it != def.regions[0].initialContentByBank.end()) CHECK(it->second == image);
    }
}

TEST(test_splice_first_save_inserts_header_and_content) {
    std::vector<uint8_t> image(0x100, 0);
    image[3] = 0x99;
    auto contentLines = formatBatteryCardInitialContentBlock(1, image);

    std::string spliced;
    std::string err;
    CHECK(spliceBatteryCardInstance(kSyntheticCardYaml, "MySave", "Synthetic", contentLines, &spliced,
                                    &err, "2026-01-01T00:00:00Z"));
    CHECK(spliced.find("# Calc-U-1600 memory module instance -- generated from \"Synthetic\".") !=
          std::string::npos);
    CHECK(spliced.find("# created: 2026-01-01T00:00:00Z") != std::string::npos);
    CHECK(spliced.find("\nsaved: 2026-01-01T00:00:00Z\n") != std::string::npos);
    CHECK(spliced.find("module-name: \"MySave\"") != std::string::npos);
    CHECK(spliced.find("initial-content:") != std::string::npos);

    MemoryCardDefinition def;
    CHECK(parseMemoryCardDefinition(spliced, &def, &err));
    CHECK(def.moduleName == "MySave");
    CHECK(def.regions.size() == 1);
    auto it = def.regions[0].initialContentByBank.find(0);
    CHECK(it != def.regions[0].initialContentByBank.end());
    if (it != def.regions[0].initialContentByBank.end()) CHECK(it->second == image);
}

TEST(test_splice_resave_preserves_created_updates_last_saved) {
    std::vector<uint8_t> image1(0x100, 0);
    auto contentLines1 = formatBatteryCardInitialContentBlock(1, image1);
    std::string firstSave;
    std::string err;
    CHECK(spliceBatteryCardInstance(kSyntheticCardYaml, "MySave", "Synthetic", contentLines1, &firstSave,
                                    &err, "2026-01-01T00:00:00Z"));

    std::vector<uint8_t> image2(0x100, 0);
    image2[7] = 0x55;
    auto contentLines2 = formatBatteryCardInitialContentBlock(1, image2);
    std::string resaved;
    CHECK(spliceBatteryCardInstance(firstSave, "MySave", "MySave", contentLines2, &resaved, &err,
                                    "2026-01-02T00:00:00Z"));

    CHECK(resaved.find("# created: 2026-01-01T00:00:00Z") != std::string::npos);
    // The origin names the template, not the instance it was re-saved from.
    CHECK(resaved.find("generated from \"Synthetic\".") != std::string::npos);
    CHECK(resaved.find("\nsaved: 2026-01-02T00:00:00Z\n") != std::string::npos);
    // Exactly one header block -- no accumulation across re-saves.
    size_t firstPos = resaved.find("# Calc-U-1600 memory module instance");
    size_t secondPos = resaved.find("# Calc-U-1600 memory module instance", firstPos + 1);
    CHECK(secondPos == std::string::npos);

    MemoryCardDefinition def;
    CHECK(parseMemoryCardDefinition(resaved, &def, &err));
    auto it = def.regions[0].initialContentByBank.find(0);
    CHECK(it != def.regions[0].initialContentByBank.end());
    if (it != def.regions[0].initialContentByBank.end()) CHECK(it->second == image2);
}

TEST(test_card_template_key) {
    std::string err;
    MemoryCardDefinition def;
    CHECK(parseMemoryCardDefinition(kSyntheticCardYaml, &def, &err));
    CHECK(!def.isTemplate);
    const std::string templ = std::string("template: true\n") + kSyntheticCardYaml;
    MemoryCardDefinition templDef;
    CHECK(parseMemoryCardDefinition(templ, &templDef, &err));
    CHECK(templDef.isTemplate);
    MemoryCardDefinition bad;
    CHECK(!parseMemoryCardDefinition(std::string("template: maybe\n") + kSyntheticCardYaml, &bad, &err));
}

// Saving from a template yields an instance: the `template:` line is gone.
TEST(test_splice_drops_template_key) {
    const std::string templ = std::string("template: true\n") + kSyntheticCardYaml;
    std::vector<uint8_t> image(0x100, 0);
    std::string spliced, err;
    CHECK(spliceBatteryCardInstance(templ, "MySave", "Synthetic", formatBatteryCardInitialContentBlock(1, image),
                                    &spliced, &err, "2026-01-01T00:00:00Z"));
    CHECK(spliced.find("template:") == std::string::npos);
    MemoryCardDefinition def;
    CHECK(parseMemoryCardDefinition(spliced, &def, &err));
    CHECK(!def.isTemplate);
}

// A protected save moves the card's switch (every `write-protect:` line,
// keeping indentation and comment); without the option the file's position
// stays; a card without the key can't be saved protected.
TEST(test_splice_write_protect) {
    const std::string templ =
        "format-version: 1\nmodule-name: \"CE-161\"\ntemplate: true\nbattery: true\n"
        "compatible-hosts: [PC-1500]\ndefinition-terminology: PC-1500\nregions:\n"
        "  - name: r\n    capacity: 0x10\n    banking: none\n    content:\n      kind: regular\n"
        "      write-protect: unprotected   # the Set Switch\n"
        "    addressing: { chip-select: Y0, span: 0x10 }\n";
    const std::vector<std::string> block = formatBatteryCardInitialContentBlock(-1, std::vector<uint8_t>(0x10, 0x55));
    std::string spliced, err;
    CHECK(spliceBatteryCardInstance(templ, "Rom", "CE-161", block, &spliced, &err, "2026-11-09T00:00:00Z", false,
                                    true));
    CHECK(spliced.find("      write-protect: protected   # the Set Switch\n") != std::string::npos);
    MemoryCardDefinition def;
    CHECK(parseMemoryCardDefinition(spliced, &def, &err));
    CHECK(!def.regions.empty() && def.regions[0].contentForBank(0).writeProtected);

    std::string resaved;  // an autosave of the instance keeps the switch where it is
    CHECK(spliceBatteryCardInstance(spliced, "Rom", "Rom", block, &resaved, &err, "2026-01-01T00:00:00Z"));
    CHECK(resaved.find("write-protect: protected") != std::string::npos);

    CHECK(!spliceBatteryCardInstance(std::string("template: true\n") + kSyntheticCardYaml, "X", "Synthetic",
                                     formatBatteryCardInitialContentBlock(1, std::vector<uint8_t>(0x100, 0)),
                                     &spliced, &err, "2026-11-09T00:00:00Z", false, true));
    CHECK(err == "the card has no write-protect switch");
}

// Every bundled card and floppy declares itself a template -- the app never
// writes to them (and the bundle may be read-only anyway).
TEST(test_bundled_files_are_templates) {
    const char* dir = "frontends/qt6/resources/cards";
    std::string err;
    const auto cards = scanMemoryCardDirectory(dir, &err);
    SKIP_UNLESS(!cards.empty(), (std::string(dir) + " not found").c_str());
    CHECK(err.empty());
    for (const auto& e : cards) {
        if (!e.isTemplate) std::fprintf(stderr, "  not a template: %s\n", e.filePath.c_str());
        CHECK(e.isTemplate);
    }
    int disks = 0;
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        const std::string path = entry.path().string();
        if (path.size() < 12 || path.compare(path.size() - 12, 12, kFloppyFileSuffix) != 0) continue;
        ++disks;
        FloppyFile disk;
        CHECK(parseFloppyFile(calcu::testing::readText(entry.path()), &disk, &err));
        if (!disk.isTemplate) std::fprintf(stderr, "  not a template: %s\n", path.c_str());
        CHECK(disk.isTemplate);
    }
    CHECK(disks > 0);
}

// A saved floppy is an instance: formatFloppyFile() never writes `template:`.
TEST(test_saved_floppy_is_instance) {
    const std::vector<uint8_t> image(CE1600FCard::kImageSize, 0xE5);
    FloppyFile disk;
    std::string err;
    CHECK(parseFloppyFile(formatFloppyFile("Mine", image, false, "", "2026-01-01T00:00:00Z"), &disk, &err));
    CHECK(disk.diskName == "Mine" && !disk.isTemplate);
}

TEST(test_splice_fails_without_module_name) {
    const char* noModuleName =
        "compatible-hosts: [PC-1500]\n"
        "definition-terminology: PC-1500\n"
        "regions:\n"
        "  - name: sram\n";
    std::string outText, err;
    CHECK(!spliceBatteryCardInstance(noModuleName, "X", "X", {}, &outText, &err, "2026-01-01T00:00:00Z"));
    CHECK(!err.empty());
}

TEST(test_splice_fails_without_regions) {
    const char* noRegions =
        "format-version: 1\nmodule-name: X\n"
        "compatible-hosts: [PC-1500]\n"
        "definition-terminology: PC-1500\n";
    std::string outText, err;
    CHECK(!spliceBatteryCardInstance(noRegions, "X", "X", {}, &outText, &err, "2026-01-01T00:00:00Z"));
    CHECK(!err.empty());
}

TEST(test_splice_roundtrip_through_real_bundled_file_if_present) {
    const char* path = "frontends/qt6/resources/cards/ce1638.module.yaml";
    std::ifstream probe(path);
    SKIP_UNLESS(probe, (std::string(path) + " not found").c_str());
    std::stringstream ss;
    ss << probe.rdbuf();
    const std::string sourceText = ss.str();

    MemoryCardDefinition orig;
    std::string err;
    CHECK(parseMemoryCardDefinition(sourceText, &orig, &err));
    if (orig.regions.empty()) return;

    const int bankCount = static_cast<int>(orig.regions[0].banking.bankCount);
    const size_t bankSize = orig.regions[0].banking.bankSize;
    std::vector<uint8_t> image(bankSize * static_cast<size_t>(bankCount), 0);
    if (!image.empty()) image[0] = 0x7E;  // ensure at least one non-uniform bank

    auto contentLines = formatBatteryCardInitialContentBlock(bankCount, image);
    std::string spliced;
    CHECK(spliceBatteryCardInstance(sourceText, "MyCE1638", orig.moduleName, contentLines, &spliced, &err, "2026-01-01T00:00:00Z"));

    MemoryCardDefinition parsed;
    CHECK(parseMemoryCardDefinition(spliced, &parsed, &err));
    CHECK(parsed.moduleName == "MyCE1638");
    CHECK(!parsed.isTemplate);
    CHECK(spliced.find("\nformat-version: 1\n") != std::string::npos);
}


}  // namespace
