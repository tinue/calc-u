// Headless C++ tests for the universal software-defined memory module:
//   * core/Connector/MemoryCardDefinition.hpp -- parse + validate + resolve
//   * core/Connector/SoftwareDefinedCard.hpp  -- the ExpansionCard itself
//   * the `slot-N:` preset keys and the OUT (28H) -> Slot 2 route
//
// Same no-framework, assert-and-tally style as lh5801_tests.cpp.
// Build & run: see tools/run_tests.sh
//
// The card-definition inputs here are inline strings, not the files under
// examples/memory/memory-cards/ (those are demo assets that get renamed/edited);
// the one file-based test SKIPs if its example is absent.

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>


#include "TempFolder.hpp"
#include "../core/Connector/MemoryCardCatalog.hpp"
#include "../core/Connector/MemoryCardDefinition.hpp"
#include "../core/Connector/SoftwareDefinedCard.hpp"
#include "../core/Machine/SharpCatalogue.hpp"
#include "../core/PC1600/PC1600Machine.hpp"
#include "support/Typing.hpp"
#include "TestRoms.hpp"
#include "TestRoms.hpp"
#include "TestCards.hpp"
#include "TestHarness.hpp"

namespace {

// ── inline module definitions (CE-155 / CE-1600M / CE-1601M shapes) ───────

const char* kCe155Yaml =
    "format-version: 1\nmodule-name: CE-155\n"
    "compatible-hosts: [PC-1500, PC-1500A, PC-1600-Slot-1]\n"
    "definition-terminology: PC-1500\n"
    "regions:\n"
    "  - name: sram-8k\n"
    "    capacity: 0x2000\n"
    "    banking: none\n"
    "    content: { kind: regular, writable: true, power-up-fill: 0xFF }\n"
    "    addressing:\n"
    "      any-of:\n"
    "        - all-of:\n"
    "            - chip-select: Y0\n"
    "            - address-bits: { A13: 1, A12: 1, A11: 1 }\n"
    "          span: 0x0800\n"
    "          maps-to: 0x0000\n"
    "        - { chip-select: S1, span: 0x0800, maps-to: 0x0800 }\n"
    "        - { chip-select: S2, span: 0x0800, maps-to: 0x1000 }\n"
    "        - { chip-select: S3, span: 0x0800, maps-to: 0x1800 }\n";

const char* kCe1600mYaml =
    "format-version: 1\nmodule-name: CE-1600M\n"
    "compatible-hosts: [PC-1600-Slot-1, PC-1600-Slot-2]\n"
    "definition-terminology: PC-1600-Slot-1\n"
    "regions:\n"
    "  - name: sram-32k\n"
    "    capacity: 0x8000\n"
    "    banking: none\n"
    "    pc1600-module-class: plain-ram-no-header\n"
    "    content: { kind: regular, writable: true, power-up-fill: 0xFF }\n"
    "    addressing:\n"
    "      any-of:\n"
    "        - all-of: [ { chip-select: RAM2 }, { signal-negated: PVOUT } ]\n"
    "          span: 0x4000\n"
    "          maps-to: 0x0000\n"
    "        - all-of: [ { chip-select: RAM2 }, { signal: PVOUT } ]\n"
    "          span: 0x4000\n"
    "          maps-to: 0x4000\n";

const char* kCe1601mYaml =
    "format-version: 1\nmodule-name: CE-1601M\n"
    "compatible-hosts: [PC-1600-Slot-2]\n"
    "definition-terminology: PC-1600-Slot-2\n"
    "regions:\n"
    "  - name: vram-64k\n"
    "    pc1600-module-class: plain-ram-no-header\n"
    "    content: { kind: regular, writable: true, power-up-fill: 0xFF }\n"
    "    addressing: { chip-select: RAM1 }\n"
    "    banking:\n"
    "      latch:\n"
    "        type: trigger-based\n"
    "        trigger: { io-port: 0x28 }\n"
    "        sampled-lines: [D0, D1, D2]\n"
    "        source-domain: data\n"
    "      bank-count: 2\n"
    "      bank-size: 0x8000\n"
    "      bank-window:\n"
    "        any-of:\n"
    "          - all-of: [ { chip-select: RAM1 }, { signal-negated: PVOUT } ]\n"
    "            span: 0x4000\n"
    "            maps-to: 0x0000\n"
    "          - all-of: [ { chip-select: RAM1 }, { signal: PVOUT } ]\n"
    "            span: 0x4000\n"
    "            maps-to: 0x4000\n";

// A CE-1601M-class card with a 4-bit vertical-bank latch (D0-D3, not
// D0-D2) and all 16 x 32 KB banks fitted = 512 KB: the latch width is the
// definition's, not the host's. Same trigger-based OUT (28H) / data-domain
// latch, same PVOUT half-select nesting.
const char* kFourBitLatchYaml =
    "format-version: 1\nmodule-name: four-bit-latch\n"
    "compatible-hosts: [PC-1600-Slot-2]\n"
    "definition-terminology: PC-1600-Slot-2\n"
    "regions:\n"
    "  - name: vram-512k\n"
    "    pc1600-module-class: plain-ram-no-header\n"
    "    content: { kind: regular, writable: true, power-up-fill: 0xFF }\n"
    "    addressing: { chip-select: RAM1 }\n"
    "    banking:\n"
    "      latch:\n"
    "        type: trigger-based\n"
    "        trigger: { io-port: 0x28 }\n"
    "        sampled-lines: [D0, D1, D2, D3]\n"
    "        source-domain: data\n"
    "      bank-count: 16\n"
    "      bank-size: 0x8000\n"
    "      bank-window:\n"
    "        any-of:\n"
    "          - all-of: [ { chip-select: RAM1 }, { signal-negated: PVOUT } ]\n"
    "            span: 0x4000\n"
    "            maps-to: 0x0000\n"
    "          - all-of: [ { chip-select: RAM1 }, { signal: PVOUT } ]\n"
    "            span: 0x4000\n"
    "            maps-to: 0x4000\n";

// Minimal by-bank fixture exercising the flash engine: banks 0-1 Regular,
// banks 2-3 Flash, sized small (0x100/bank, 0x40 sectors) for cheap tests.
// Same trigger shape as CE-1638/CE-163F (pin 18, address domain), scaled
// down -- not modeling a real product.
const char* kFlashByBankYaml =
    "format-version: 1\nmodule-name: T-FLASH\n"
    "compatible-hosts: [PC-1500]\n"
    "definition-terminology: PC-1500\n"
    "regions:\n"
    "  - name: r\n"
    "    addressing: { chip-select: Y0 }\n"
    "    banking:\n"
    "      latch:\n"
    "        type: trigger-based\n"
    "        trigger: { pin: 18 }\n"
    "        sampled-lines: [A0, A1]\n"
    "        source-domain: address\n"
    "      bank-count: 4\n"
    "      bank-size: 0x100\n"
    "      bank-window: { chip-select: Y0, span: 0x100 }\n"
    "    content:\n"
    "      by-bank:\n"
    "        - { banks: \"0-1\", kind: regular, writable: true, power-up-fill: 0xFF }\n"
    "        - banks: \"2-3\"\n"
    "          kind: flash\n"
    "          power-up-fill: 0xAA\n"
    "          protocol:\n"
    "            unlock-sequence:\n"
    "              - { address: 0x55, data: 0xAA }\n"
    "              - { address: 0x2A, data: 0x55 }\n"
    "            byte-program-command: 0xA0\n"
    "            erase-setup-command: 0x80\n"
    "            sector-erase-command: 0x30\n"
    "            chip-erase-command: 0x10\n"
    "            reset-command: 0xF0\n"
    "            sector-size: 0x40\n";

std::unique_ptr<SoftwareDefinedCard> buildCard(const char* yaml, CardHost host) {
    MemoryCardDefinition def;
    std::string err;
    if (!parseMemoryCardDefinition(yaml, &def, &err)) {
        std::fprintf(stderr, "  buildCard parse error: %s\n", err.c_str());
        return nullptr;
    }
    if (!def.compatibleWith(host)) return nullptr;
    return std::make_unique<SoftwareDefinedCard>(std::move(def));
}

// ── definition parse / validation ───────────────────────────────────────

TEST(test_inline_definitions_parse) {
    for (const char* y : {kCe155Yaml, kCe1600mYaml, kCe1601mYaml}) {
        MemoryCardDefinition def;
        std::string err;
        bool ok = parseMemoryCardDefinition(y, &def, &err);
        if (!ok) std::fprintf(stderr, "  (%s)\n", err.c_str());
        CHECK(ok);
    }
}

TEST(test_card_description) {
    MemoryCardDefinition def;
    std::string err;
    CHECK(parseMemoryCardDefinition(kCe155Yaml, &def, &err));
    CHECK(def.description.empty());
    std::string y = kCe155Yaml;
    y.insert(y.find("compatible-hosts"), "description: \"8 KB static RAM\"\n");
    MemoryCardDefinition described;
    CHECK(parseMemoryCardDefinition(y, &described, &err));
    CHECK(described.description == "8 KB static RAM");
}

TEST(test_ce155_definition_shape) {
    MemoryCardDefinition def;
    std::string err;
    CHECK(parseMemoryCardDefinition(kCe155Yaml, &def, &err));
    CHECK(def.terminology == CardHost::PC1500);
    CHECK(def.compatibleWith(CardHost::PC1500) && def.compatibleWith(CardHost::PC1600Slot1));
    CHECK(def.regions.size() == 1);
    const Region& r = def.regions[0];
    CHECK(!r.banked && r.capacity == 0x2000);
    CHECK(r.addressing.groups.size() == 4);
    const EnableGroup& y0 = r.addressing.groups[0];
    CHECK(y0.requireHigh.size() == 1 && y0.requireHigh[0] == 4);
    CHECK(y0.addrBits.size() == 3);
    CHECK(y0.span == 0x800 && y0.mapsTo == 0);
    CHECK(r.addressing.groups[1].requireHigh.size() == 1 &&
          r.addressing.groups[1].requireHigh[0] == 16);
    CHECK(r.addressing.groups[3].mapsTo == 0x1800);
}

// Same shape as kFlashByBankYaml (bank-size 0x100, banks 0-1 Regular /
// 2-3 Flash) plus `initial-content` on bank 0 (addressed-hex) and bank 2
// (plain hex, partial coverage -- the rest keeps the flash power-up-fill).
const char* kInitialContentYaml =
    "format-version: 1\nmodule-name: T-INIT\n"
    "compatible-hosts: [PC-1500]\n"
    "definition-terminology: PC-1500\n"
    "regions:\n"
    "  - name: r\n"
    "    addressing: { chip-select: Y0 }\n"
    "    banking:\n"
    "      latch:\n"
    "        type: trigger-based\n"
    "        trigger: { pin: 18 }\n"
    "        sampled-lines: [A0, A1]\n"
    "        source-domain: address\n"
    "      bank-count: 4\n"
    "      bank-size: 0x100\n"
    "      bank-window: { chip-select: Y0, span: 0x100 }\n"
    "    content:\n"
    "      by-bank:\n"
    "        - { banks: \"0-1\", kind: regular, writable: true, power-up-fill: 0xFF }\n"
    "        - banks: \"2-3\"\n"
    "          kind: flash\n"
    "          power-up-fill: 0xAA\n"
    "          protocol:\n"
    "            unlock-sequence:\n"
    "              - { address: 0x55, data: 0xAA }\n"
    "              - { address: 0x2A, data: 0x55 }\n"
    "            byte-program-command: 0xA0\n"
    "            erase-setup-command: 0x80\n"
    "            sector-erase-command: 0x30\n"
    "            chip-erase-command: 0x10\n"
    "            reset-command: 0xF0\n"
    "            sector-size: 0x40\n"
    "    initial-content:\n"
    "      blocks:\n"
    "        - bank: 0\n"
    "          offset: 0x0000\n"
    "          encoding: addressed-hex\n"
    "          bytes: |\n"
    "            $0000: 00...\n"
    "            $0010: 11 22 33 44 55 66 77 88  99 AA BB CC DD EE FF 00\n"
    "            $0020: FF...\n"
    "        - bank: 2\n"
    "          offset: 0x0010\n"
    "          encoding: hex\n"
    "          bytes: |\n"
    "            AA BB CC\n";

bool rejects(const std::string& text, std::string* err) {
    MemoryCardDefinition def;
    err->clear();
    return !parseMemoryCardDefinition(text, &def, err) && !err->empty();
}

const char* kMinPrefix =
    "format-version: 1\nmodule-name: T\n"
    "compatible-hosts: [PC-1500]\n"
    "definition-terminology: PC-1500\n"
    "regions:\n";

TEST(test_reject_unknown_top_key) {
    std::string err;
    CHECK(rejects(std::string(kMinPrefix) +
                      "  - name: r\n    capacity: 0x800\n    banking: none\n"
                      "    content: regular\n    addressing: { chip-select: Y0, span: 0x800 }\n"
                      "bogus-key: 1\n",
                  &err));
}

// A card without `format-version`, or with one this build doesn't know, is
// refused -- and the version is checked before the key list, so a newer
// file with new keys still says which version it is.
TEST(test_reject_missing_or_unknown_format_version) {
    const std::string body = std::string(kMinPrefix) +
                             "  - name: r\n    capacity: 0x800\n    banking: none\n"
                             "    content: regular\n    addressing: { chip-select: Y0, span: 0x800 }\n";
    std::string err;
    MemoryCardDefinition def;
    CHECK(parseMemoryCardDefinition(body, &def, &err));

    std::string noVersion = body;
    noVersion.erase(0, std::string("format-version: 1\n").size());
    CHECK(rejects(noVersion, &err));
    CHECK(err == "missing 'format-version'");

    std::string v2 = body;
    v2.replace(0, std::string("format-version: 1").size(), "format-version: 2");
    CHECK(rejects(v2 + "new-key: 1\n", &err));
    CHECK(err == "unsupported module definition format-version 2 (this build reads 1)");
}

TEST(test_reject_terminology_not_in_hosts) {
    std::string err;
    CHECK(rejects("format-version: 1\nmodule-name: T\n"
                  "compatible-hosts: [PC-1500]\n"
                  "definition-terminology: PC-1500A\n"
                  "regions:\n"
                  "  - name: r\n    capacity: 0x800\n    banking: none\n"
                  "    content: regular\n    addressing: { chip-select: Y0, span: 0x800 }\n",
                  &err));
}

TEST(test_reject_tiling_gap_and_overlap) {
    std::string err;
    CHECK(rejects(std::string(kMinPrefix) +
                      "  - name: r\n    capacity: 0x1000\n    banking: none\n    content: regular\n"
                      "    addressing:\n      any-of:\n"
                      "        - { chip-select: S1, span: 0x400, maps-to: 0x000 }\n"
                      "        - { chip-select: S2, span: 0x400, maps-to: 0x800 }\n",
                  &err));
    CHECK(rejects(std::string(kMinPrefix) +
                      "  - name: r\n    capacity: 0x800\n    banking: none\n    content: regular\n"
                      "    addressing:\n      any-of:\n"
                      "        - { chip-select: S1, span: 0x800, maps-to: 0x000 }\n"
                      "        - { chip-select: S2, span: 0x800, maps-to: 0x400 }\n",
                  &err));
}

TEST(test_reject_rom_flash_bybank_linebased) {
    std::string err;
    // A ROM range with no initial-content has no bytes at all.
    CHECK(rejects(std::string(kMinPrefix) +
                      "  - name: r\n    capacity: 0x800\n    banking: none\n    content: rom\n"
                      "    addressing: { chip-select: Y0, span: 0x800 }\n",
                  &err));
    CHECK(err.find("initial-content") != std::string::npos);
    CHECK(rejects(std::string(kMinPrefix) +
                      "  - name: r\n    capacity: 0x800\n    banking: none\n    content: flash\n"
                      "    addressing: { chip-select: Y0, span: 0x800 }\n",
                  &err));
    CHECK(rejects(std::string(kMinPrefix) +
                      "  - name: r\n    banking:\n"
                      "        latch: { type: line-based, lines: [K0], source-domain: address-select }\n"
                      "        bank-count: 2\n        bank-size: 0x800\n"
                      "        bank-window: { chip-select: Y0, span: 0x800 }\n"
                      "    content: regular\n    addressing: { chip-select: Y0 }\n",
                  &err));
}

TEST(test_reject_bybank_shapes) {
    std::string err;
    // by-bank on an unbanked region.
    CHECK(rejects(std::string(kMinPrefix) +
                      "  - name: r\n    capacity: 0x800\n    banking: none\n"
                      "    content: { by-bank: [ { banks: \"0\", kind: regular } ] }\n"
                      "    addressing: { chip-select: Y0, span: 0x800 }\n",
                  &err));
    // Gap: banks 0-2 of a 4-bank region, nothing covers bank 3.
    CHECK(rejects(std::string(kMinPrefix) +
                      "  - name: r\n"
                      "    addressing: { chip-select: Y0 }\n"
                      "    banking:\n"
                      "        latch: { type: trigger-based, trigger: { pin: 18 },\n"
                      "                 sampled-lines: [A0, A1], source-domain: address }\n"
                      "        bank-count: 4\n        bank-size: 0x100\n"
                      "        bank-window: { chip-select: Y0, span: 0x100 }\n"
                      "    content: { by-bank: [ { banks: \"0-2\", kind: regular } ] }\n",
                  &err));
    // Overlap: banks 0-2 and 2-3 both claim bank 2.
    CHECK(rejects(std::string(kMinPrefix) +
                      "  - name: r\n"
                      "    addressing: { chip-select: Y0 }\n"
                      "    banking:\n"
                      "        latch: { type: trigger-based, trigger: { pin: 18 },\n"
                      "                 sampled-lines: [A0, A1], source-domain: address }\n"
                      "        bank-count: 4\n        bank-size: 0x100\n"
                      "        bank-window: { chip-select: Y0, span: 0x100 }\n"
                      "    content:\n"
                      "      by-bank:\n"
                      "        - { banks: \"0-2\", kind: regular }\n"
                      "        - { banks: \"2-3\", kind: regular }\n",
                  &err));
    // Out of range: bank-count is 4 (0-3), a range names bank 4.
    CHECK(rejects(std::string(kMinPrefix) +
                      "  - name: r\n"
                      "    addressing: { chip-select: Y0 }\n"
                      "    banking:\n"
                      "        latch: { type: trigger-based, trigger: { pin: 18 },\n"
                      "                 sampled-lines: [A0, A1], source-domain: address }\n"
                      "        bank-count: 4\n        bank-size: 0x100\n"
                      "        bank-window: { chip-select: Y0, span: 0x100 }\n"
                      "    content: { by-bank: [ { banks: \"0-4\", kind: regular } ] }\n",
                  &err));
    // Flash content still needs a protocol, even inside a by-bank entry.
    CHECK(rejects(std::string(kMinPrefix) +
                      "  - name: r\n"
                      "    addressing: { chip-select: Y0 }\n"
                      "    banking:\n"
                      "        latch: { type: trigger-based, trigger: { pin: 18 },\n"
                      "                 sampled-lines: [A0, A1], source-domain: address }\n"
                      "        bank-count: 4\n        bank-size: 0x100\n"
                      "        bank-window: { chip-select: Y0, span: 0x100 }\n"
                      "    content: { by-bank: [ { banks: \"0-3\", kind: flash } ] }\n",
                  &err));
}

TEST(test_reject_unbanked_without_capacity) {
    std::string err;
    CHECK(rejects(std::string(kMinPrefix) +
                      "  - name: r\n    banking: none\n    content: regular\n"
                      "    addressing: { chip-select: Y0, span: 0x800 }\n",
                  &err));
}

// ── compatibility gate ─────────────────────────────────────────────────

TEST(test_compat_gate_inline) {
    CHECK(buildCard(kCe1600mYaml, CardHost::PC1600Slot1) != nullptr);
    CHECK(buildCard(kCe1600mYaml, CardHost::PC1500) == nullptr);  // not in compatible-hosts
}

TEST(test_compat_gate_via_file_if_present) {
    const char* path = "frontends/qt6/resources/cards/ce155.module.yaml";
    std::ifstream probe(path);
    SKIP_UNLESS(probe, (std::string(path) + " not found").c_str());
    std::string err;
    CHECK(makeSoftwareDefinedCard(path, CardHost::PC1500, &err) != nullptr);
    err.clear();
    CHECK(makeSoftwareDefinedCard(path, CardHost::PC1600Slot2, &err) == nullptr &&
          err.find("not compatible") != std::string::npos);
}

TEST(test_superram_r4_card_yaml_if_present) {
    const char* path = "frontends/qt6/resources/cards/superRAMr4.module.yaml";
    std::ifstream probe(path);
    SKIP_UNLESS(probe, (std::string(path) + " not found").c_str());
    // Slot 2 only.
    for (CardHost h : {CardHost::PC1500, CardHost::PC1500A, CardHost::PC1600Slot1}) {
        std::string err;
        CHECK(makeSoftwareDefinedCard(path, h, &err) == nullptr);
    }

    std::string err;
    auto card = makeSoftwareDefinedCard(path, CardHost::PC1600Slot2, &err);
    CHECK(card != nullptr);
    if (!card) return;
    CHECK(card->debugBankCount() == 32);   // 1 MB: a 5-bit (D0-D4) latch

    auto* sd = static_cast<SoftwareDefinedCard*>(card.get());
    PinState sel;
    sel.ioWrite = true;
    sel.forWrite = true;
    sel.address = 0x28;
    CHECK(sd->respondsToWrite(sel, 31));  // D4..D0 -> bank 31
    CHECK(sd->currentBank() == 31);

    PinState w;
    w.pin[4] = true;   // RAM1
    w.forWrite = true;
    w.address = 0x8000;
    CHECK(sd->respondsToWrite(w, 0x42));
    uint8_t v = 0xAA;
    CHECK(sd->respondsToRead(w, v) && v == 0x42);

    sd->respondsToWrite(sel, 0);
    CHECK(sd->respondsToRead(w, v) && v == 0x00);  // blank: bank 0 untouched
}

TEST(test_256k_custom_card_yaml_if_present) {
    const char* path = "frontends/qt6/resources/cards/256k-custom.module.yaml";
    std::ifstream probe(path);
    SKIP_UNLESS(probe, (std::string(path) + " not found").c_str());
    std::string err;
    CHECK(makeSoftwareDefinedCard(path, CardHost::PC1600Slot1, &err) == nullptr);   // Slot 2 only
    auto card = makeSoftwareDefinedCard(path, CardHost::PC1600Slot2, &err);
    CHECK(card != nullptr);
    if (!card) return;
    CHECK(card->debugBankCount() == 8);   // 256 KB: a 3-bit (D0-D2) latch

    // Initialised as INIT"S2:","F" then "M" leave it: bank 0's cleared
    // header (expansion memory), bank 1 the RAM disk's boot sector.
    auto* sd = static_cast<SoftwareDefinedCard*>(card.get());
    PinState sel;
    sel.ioWrite = true;
    sel.forWrite = true;
    sel.address = 0x28;
    PinState r;
    r.pin[4] = true;   // RAM1
    r.address = 0x8000;
    uint8_t v = 0;
    CHECK(sd->respondsToWrite(sel, 0));
    CHECK(sd->respondsToRead(r, v) && v == 0xFF);
    CHECK(sd->respondsToWrite(sel, 1));
    CHECK(sd->respondsToRead(r, v) && v == 0x55);
}

// ── `rom` content ────────────────────────────────────────────────────

// A 16-byte ROM on Y0; `bytes` is its initial-content (plain hex).
std::string romYaml(const std::string& bytes) {
    return std::string(kMinPrefix) +
           "  - name: r\n    capacity: 0x10\n    banking: none\n    content: rom\n"
           "    addressing: { chip-select: Y0, span: 0x10 }\n"
           "    initial-content:\n      blocks:\n        - offset: 0\n          encoding: hex\n"
           "          bytes: \"" + bytes + "\"\n";
}

TEST(test_rom_reads_and_ignores_every_write) {
    auto card = buildCard(romYaml("55 00 00 C5 40 00 00 FF 01 02 03 04 05 06 07 08").c_str(), CardHost::PC1500);
    CHECK(card != nullptr);
    if (!card) return;
    CHECK(card->definition().isRom());
    PinState p;
    p.pin[4] = true;
    p.address = 0x0003;
    uint8_t v = 0;
    CHECK(card->respondsToRead(p, v) && v == 0xC5);

    // Guest-CPU store: claimed (not open bus), but the ROM keeps its byte.
    PinState w = p;
    w.forWrite = true;
    CHECK(card->respondsToWrite(w, 0x11));
    CHECK(card->respondsToRead(p, v) && v == 0xC5);
    // A host poke can't write a mask ROM either.
    w.direct = true;
    CHECK(card->respondsToWrite(w, 0x22));
    CHECK(card->respondsToRead(p, v) && v == 0xC5);
    CHECK(card->debugImage()[3] == 0xC5);
}

TEST(test_rom_needs_every_byte_covered) {
    std::string err;
    CHECK(rejects(romYaml("55 00 00 C5 40 00 00 FF"), &err));  // 8 of 16 bytes
    CHECK(err.find("0x8") != std::string::npos);
}

// A preset's `slot-N-rom:` file replaces the ROM of a ROM module: the card
// file's placeholder (`$0000: FF...`) gives way to the file's bytes, a
// rebuilt file is picked up by the next build of the card, and only a
// single-region ROM module of exactly the file's size takes it.
TEST(test_rom_replaced_from_a_file) {
    const std::string placeholder = std::string(kMinPrefix) +
                                    "  - name: r\n    capacity: 0x10\n    banking: none\n    content: rom\n"
                                    "    addressing: { chip-select: Y0, span: 0x10 }\n"
                                    "    initial-content:\n      blocks:\n        - offset: 0\n"
                                    "          encoding: addressed-hex\n          bytes: |\n            $0000: FF...\n";
    MemoryCardDefinition def;
    std::string err;
    CHECK(parseMemoryCardDefinition(placeholder, &def, &err));
    std::vector<uint8_t> rom(16, 0xEE);
    rom[0] = 0x55;
    CHECK(replaceRomContent(&def, rom, &err));
    CHECK(def.regions[0].initialContentByBank[0] == rom);

    // Through the card factory, read on the bus.
    calcu::testing::TempFolder tmp("calcu1600-card-rom");
    const std::string dir = tmp.str();
    const std::string cardPath = std::string(dir) + "/rom.module.yaml";
    std::ofstream(cardPath) << placeholder;
    PinState p;
    p.pin[4] = true;
    uint8_t v = 0;
    auto card = makeSoftwareDefinedCard(cardPath, CardHost::PC1500, &err, &rom);
    CHECK(card && card->respondsToRead(p, v) && v == 0x55);
    card = makeSoftwareDefinedCard(cardPath, CardHost::PC1500, &err);  // no file: the placeholder
    CHECK(card && card->respondsToRead(p, v) && v == 0xFF);
    std::filesystem::remove_all(dir);

    // A file of the wrong size is refused, shorter or longer.
    MemoryCardDefinition def2;
    CHECK(parseMemoryCardDefinition(placeholder, &def2, &err));
    CHECK(!replaceRomContent(&def2, std::vector<uint8_t>(8, 0), &err));
    CHECK(err == "8 bytes, the module's ROM is 16 bytes");
    CHECK(!replaceRomContent(&def2, std::vector<uint8_t>(17, 0), &err));

    // Not a ROM module: RAM, a ROM/RAM by-bank split, two regions.
    const auto refuses = [&](const std::string& yaml) {
        MemoryCardDefinition d;
        std::string e;
        if (!parseMemoryCardDefinition(yaml, &d, &e)) return false;
        return !replaceRomContent(&d, std::vector<uint8_t>(16, 0), &e) &&
               e.find("is not a ROM module") != std::string::npos;
    };
    CHECK(refuses(std::string(kMinPrefix) +
                  "  - name: r\n    capacity: 0x10\n    banking: none\n    content: regular\n"
                  "    addressing: { chip-select: Y0, span: 0x10 }\n"));
    CHECK(refuses(std::string(kMinPrefix) +
                  "  - name: r\n"
                  "    banking:\n"
                  "      latch: { type: trigger-based, trigger: { pin: 18 }, sampled-lines: [A0], source-domain: address }\n"
                  "      bank-count: 2\n      bank-size: 0x8\n"
                  "      bank-window: { chip-select: Y0, span: 0x8 }\n"
                  "    content:\n      by-bank:\n"
                  "        - { banks: \"0\", kind: rom }\n"
                  "        - { banks: \"1\", kind: regular }\n"
                  "    addressing: { chip-select: Y0 }\n"
                  "    initial-content:\n      blocks:\n        - bank: 0\n          offset: 0\n          encoding: hex\n"
                  "          bytes: \"A0 A1 A2 A3 A4 A5 A6 A7\"\n"));
    CHECK(refuses(std::string(kMinPrefix) +
                  "  - name: a\n    capacity: 0x8\n    banking: none\n    content: rom\n"
                  "    addressing: { chip-select: S1, span: 0x8 }\n"
                  "    initial-content:\n      blocks:\n        - offset: 0\n"
                  "          encoding: addressed-hex\n          bytes: |\n            $0000: FF...\n"
                  "  - name: b\n    capacity: 0x8\n    banking: none\n    content: rom\n"
                  "    addressing: { chip-select: S2, span: 0x8 }\n"
                  "    initial-content:\n      blocks:\n        - offset: 0\n"
                  "          encoding: addressed-hex\n          bytes: |\n            $0000: FF...\n"));

    // `encoding: file` is gone: an unknown key, then an unknown encoding.
    CHECK(rejects(std::string(kMinPrefix) +
                      "  - name: r\n    capacity: 0x10\n    banking: none\n    content: rom\n"
                      "    addressing: { chip-select: Y0, span: 0x10 }\n"
                      "    initial-content:\n      blocks:\n        - offset: 0\n          encoding: file\n",
                  &err));
    CHECK(err.find("unknown encoding 'file'") != std::string::npos);
}

// ROM and RAM banks in one region: the ROM banks stay read-only, the RAM
// bank takes writes, and the card isn't a ROM module as a whole.
TEST(test_rom_by_bank_mixed_with_regular) {
    const std::string yaml = std::string(kMinPrefix) +
        "  - name: r\n"
        "    banking:\n"
        "      latch: { type: trigger-based, trigger: { pin: 18 }, sampled-lines: [A0], source-domain: address }\n"
        "      bank-count: 2\n      bank-size: 0x10\n"
        "      bank-window: { chip-select: Y0, span: 0x10 }\n"
        "    content:\n      by-bank:\n"
        "        - { banks: \"0\", kind: rom }\n"
        "        - { banks: \"1\", kind: regular }\n"
        "    addressing: { chip-select: Y0 }\n"
        "    initial-content:\n      blocks:\n        - bank: 0\n          offset: 0\n          encoding: hex\n"
        "          bytes: \"A0 A1 A2 A3 A4 A5 A6 A7 A8 A9 AA AB AC AD AE AF\"\n";
    auto card = buildCard(yaml.c_str(), CardHost::PC1500);
    CHECK(card != nullptr);
    if (!card) return;
    CHECK(!card->definition().isRom());
    const Region& r = card->definition().regions[0];
    CHECK(r.contentForBank(0).kind == ContentKind::Rom);
    CHECK(r.contentForBank(1).kind == ContentKind::Regular);
    CHECK(card->debugImage()[0x02] == 0xA2);
}

// ── CE-1601M trigger-based banking ─────────────────────────────────────

TEST(test_ce1601m_trigger_banking_direct) {
    auto sd = buildCard(kCe1601mYaml, CardHost::PC1600Slot2);
    CHECK(sd != nullptr);
    if (!sd) return;

    auto out28 = [&](uint8_t v) {
        PinState p;
        p.ioWrite = true;
        p.forWrite = true;
        p.address = 0x28;
        sd->respondsToWrite(p, v);
    };
    auto mem = [&](uint16_t addr, bool pvout) {
        PinState p;
        p.address = addr;
        p.pin[4] = true;
        p.pin[5] = pvout;
        return p;
    };

    out28(1);
    CHECK(sd->currentBank() == 1);
    PinState w = mem(0x8000, false);
    w.forWrite = true;
    CHECK(sd->respondsToWrite(w, 0x5A));
    uint8_t v = 0;
    CHECK(sd->respondsToRead(mem(0x8000, false), v) && v == 0x5A);

    out28(0);
    CHECK(sd->currentBank() == 0);
    CHECK(sd->respondsToRead(mem(0x8000, false), v) && v == 0xFF);

    PinState wh = mem(0x8000, true);
    wh.forWrite = true;
    CHECK(sd->respondsToWrite(wh, 0x24));
    CHECK(sd->respondsToRead(mem(0x8000, true), v) && v == 0x24);
    CHECK(sd->respondsToRead(mem(0x8000, false), v) && v == 0xFF);  // low half untouched
}

// debugBankCount(): the GUI resource-inventory's "N-way Port 28H paging"
// label reads this alongside debugCurrentBank() -- confirms it reports the
// definition's bank-count (kCe1601mYaml: 2) and is stable across latch
// writes (unlike debugCurrentBank(), which tracks the live selection).
TEST(test_debug_bank_count) {
    auto sd = buildCard(kCe1601mYaml, CardHost::PC1600Slot2);
    CHECK(sd != nullptr);
    if (!sd) return;
    CHECK(sd->debugBankCount() == 2);
    CHECK(sd->debugCurrentBank() == 0);

    PinState p;
    p.ioWrite = true;
    p.forWrite = true;
    p.address = 0x28;
    sd->respondsToWrite(p, 1);
    CHECK(sd->debugCurrentBank() == 1);
    CHECK(sd->debugBankCount() == 2);  // unchanged -- total count, not the live pick

    // An unbanked card (CE-1600M) has no bank concept: both report -1.
    auto plain = buildCard(kCe1600mYaml, CardHost::PC1600Slot1);
    CHECK(plain != nullptr);
    if (plain) {
        CHECK(plain->debugBankCount() == -1);
        CHECK(plain->debugCurrentBank() == -1);
    }
}

// ── CE-1601M mechanism with a 4-bit (D0-D3) vertical-bank latch ─────────

TEST(test_four_bit_vertical_latch_direct) {
    // Slot 2 only -- same gate as the CE-1601M.
    CHECK(buildCard(kFourBitLatchYaml, CardHost::PC1600Slot1) == nullptr);
    CHECK(buildCard(kFourBitLatchYaml, CardHost::PC1500) == nullptr);

    auto sd = buildCard(kFourBitLatchYaml, CardHost::PC1600Slot2);
    CHECK(sd != nullptr);
    if (!sd) return;

    CHECK(sd->debugBankCount() == 16);  // 16 x 32 KB = 512 KB

    auto out28 = [&](uint8_t v) {
        PinState p;
        p.ioWrite = true;
        p.forWrite = true;
        p.address = 0x28;
        sd->respondsToWrite(p, v);
    };
    auto mem = [&](uint16_t addr, bool pvout) {
        PinState p;
        p.address = addr;
        p.pin[4] = true;   // RAM1 enable
        p.pin[5] = pvout;  // PVOUT half-select
        return p;
    };

    // D3 is honoured: bank 15 is reachable (a CE-1601M would mask to D0-D2
    // and land on bank 7 instead).
    out28(15);
    CHECK(sd->currentBank() == 15);
    PinState w15 = mem(0x8000, false);
    w15.forWrite = true;
    CHECK(sd->respondsToWrite(w15, 0x5A));
    uint8_t v = 0;
    CHECK(sd->respondsToRead(mem(0x8000, false), v) && v == 0x5A);

    // A distinct high bank keeps its own bytes...
    out28(8);
    CHECK(sd->currentBank() == 8);
    CHECK(sd->respondsToRead(mem(0x8000, false), v) && v == 0xFF);
    PinState w8 = mem(0x8000, false);
    w8.forWrite = true;
    CHECK(sd->respondsToWrite(w8, 0x24));

    // ...and PVOUT still splits each 32 KB bank into two 16 KB halves.
    PinState w8hi = mem(0x8000, true);
    w8hi.forWrite = true;
    CHECK(sd->respondsToWrite(w8hi, 0x99));
    CHECK(sd->respondsToRead(mem(0x8000, true), v) && v == 0x99);
    CHECK(sd->respondsToRead(mem(0x8000, false), v) && v == 0x24);  // low half intact

    // Back to bank 15 -- its cell survived the excursion.
    out28(15);
    CHECK(sd->respondsToRead(mem(0x8000, false), v) && v == 0x5A);
}

// End to end through PC1600Memory: OUT (28H) forwards the whole byte
// (including D3) to the Slot 2 card's latch.
TEST(test_four_bit_vertical_latch_through_pc1600) {
    PC1600Bank bank;
    PC1600Memory mem(bank);
    mem.attachSlot2Card(buildCard(kFourBitLatchYaml, CardHost::PC1600Slot2));

    mem.writeIO(0x31, static_cast<uint8_t>(2 << 4));  // page-C bank 2 -> Slot 2, PVOUT low
    mem.writeIO(0x28, 0);
    mem.write(0x8000, 0xA1);
    CHECK(mem.read(0x8000) == 0xA1);

    mem.writeIO(0x28, 13);                            // vertical bank 13 -- needs D3+D2
    CHECK(bank.slot2VerticalBank() == 13);
    CHECK(mem.read(0x8000) == 0xFF);                  // fresh chip
    mem.write(0x8000, 0xB2);
    CHECK(mem.read(0x8000) == 0xB2);

    mem.writeIO(0x28, 0);
    CHECK(mem.read(0x8000) == 0xA1);                  // bank 0 cell untouched
}

// ── flash content / by-bank split ───────────────────────────────────────

TEST(test_flash_sector_and_chip_erase) {
    auto sd = buildCard(kFlashByBankYaml, CardHost::PC1500);
    CHECK(sd != nullptr);
    if (!sd) return;

    auto selectBank = [&](int bank) {
        PinState p;
        p.forWrite = true;
        p.pin[18] = true;
        p.address = uint16_t(bank);
        sd->respondsToWrite(p, 0);
    };
    auto write = [&](uint16_t off, uint8_t data) {
        PinState p;
        p.pin[4] = true;
        p.forWrite = true;
        p.address = off;
        return sd->respondsToWrite(p, data);
    };
    auto read = [&](uint16_t off) {
        uint8_t v = 0;
        PinState p;
        p.pin[4] = true;
        p.address = off;
        sd->respondsToRead(p, v);
        return v;
    };
    auto programByte = [&](uint16_t off, uint8_t data) {
        write(0x55, 0xAA);
        write(0x2A, 0x55);
        write(0x55, 0xA0);
        write(off, data);
    };
    auto directWrite = [&](uint16_t off, uint8_t data) {
        PinState p;
        p.pin[4] = true;
        p.forWrite = true;
        p.direct = true;
        p.address = off;
        return sd->respondsToWrite(p, data);
    };

    selectBank(2);
    directWrite(0x00, 0x11);  // poke bypasses the decoder entirely
    CHECK(read(0x00) == 0x11);

    programByte(0x00, 0x00);  // clear the whole first byte
    CHECK(read(0x00) == 0x00);

    // Sector erase (sector-size 0x40) clears only the current bank's sector.
    write(0x55, 0xAA);
    write(0x2A, 0x55);
    write(0x55, 0x80);  // erase-setup
    write(0x55, 0xAA);
    write(0x2A, 0x55);
    write(0x30, 0x30);  // sector-erase-command, sector containing 0x30
    CHECK(read(0x00) == 0xFF);

    // Chip erase clears every flash bank in the region, not just the
    // current one ("clears the whole Flash content-range").
    selectBank(2);
    programByte(0x00, 0x00);
    selectBank(3);
    programByte(0x00, 0x00);
    CHECK(read(0x00) == 0x00);
    selectBank(2);
    write(0x55, 0xAA);
    write(0x2A, 0x55);
    write(0x55, 0x80);
    write(0x55, 0xAA);
    write(0x2A, 0x55);
    write(0x55, 0x10);  // chip-erase-command
    CHECK(read(0x00) == 0xFF);
    selectBank(3);
    CHECK(read(0x00) == 0xFF);  // bank 3 cleared too
}

// ── initial-content ──────────────────────────────────────────────────

TEST(test_initial_content_addressed_hex_and_hex) {
    auto sd = buildCard(kInitialContentYaml, CardHost::PC1500);
    CHECK(sd != nullptr);
    if (!sd) return;

    auto selectBank = [&](int bank) {
        PinState p;
        p.forWrite = true;
        p.pin[18] = true;
        p.address = uint16_t(bank);
        sd->respondsToWrite(p, 0);
    };
    auto read = [&](uint16_t off) {
        uint8_t v = 0;
        PinState p;
        p.pin[4] = true;
        p.address = off;
        sd->respondsToRead(p, v);
        return v;
    };

    // Bank 0: addressed-hex -- $0000-$000F run of 00, $0010-$001F literal,
    // $0020-$00FF run of FF.
    selectBank(0);
    CHECK(read(0x0000) == 0x00);
    CHECK(read(0x000F) == 0x00);
    CHECK(read(0x0010) == 0x11);
    CHECK(read(0x001B) == 0xCC);
    CHECK(read(0x001F) == 0x00);
    CHECK(read(0x0020) == 0xFF);
    CHECK(read(0x00FF) == 0xFF);

    // Bank 2: FLASH, power-up-fill 0xAA, plain 'hex' block only covers
    // offset 0x10-0x12 -- everything else keeps the flash fill.
    selectBank(2);
    CHECK(read(0x0000) == 0xAA);
    CHECK(read(0x000F) == 0xAA);
    CHECK(read(0x0010) == 0xAA);  // 0xAA byte, indistinguishable from fill here
    CHECK(read(0x0011) == 0xBB);
    CHECK(read(0x0012) == 0xCC);
    CHECK(read(0x0013) == 0xAA);

    // Bank 3: FLASH, no initial-content block at all -- untouched fill.
    selectBank(3);
    CHECK(read(0x0000) == 0xAA);
}

TEST(test_reject_initial_content_addressed_hex_gap_or_overlap) {
    std::string err;
    // Gap: $0000-$000F run, then $0020 directly (skips $0010-$001F).
    CHECK(rejects(std::string(kMinPrefix) +
                      "  - name: r\n"
                      "    addressing: { chip-select: Y0 }\n"
                      "    banking:\n"
                      "        latch: { type: trigger-based, trigger: { pin: 18 },\n"
                      "                 sampled-lines: [A0], source-domain: address }\n"
                      "        bank-count: 2\n        bank-size: 0x20\n"
                      "        bank-window: { chip-select: Y0, span: 0x20 }\n"
                      "    content: { by-bank: [ { banks: \"0-1\", kind: regular } ] }\n"
                      "    initial-content:\n"
                      "      blocks:\n"
                      "        - bank: 0\n          offset: 0\n          encoding: addressed-hex\n"
                      "          bytes: |\n            $0000: 00...\n            $0010: 00...\n",
                  &err));

    // Overlap between two blocks on the same bank.
    CHECK(rejects(std::string(kMinPrefix) +
                      "  - name: r\n"
                      "    addressing: { chip-select: Y0 }\n"
                      "    banking:\n"
                      "        latch: { type: trigger-based, trigger: { pin: 18 },\n"
                      "                 sampled-lines: [A0], source-domain: address }\n"
                      "        bank-count: 2\n        bank-size: 0x20\n"
                      "        bank-window: { chip-select: Y0, span: 0x20 }\n"
                      "    content: { by-bank: [ { banks: \"0-1\", kind: regular } ] }\n"
                      "    initial-content:\n"
                      "      blocks:\n"
                      "        - bank: 0\n          offset: 0\n          encoding: hex\n"
                      "          bytes: |\n            00 11 22\n"
                      "        - bank: 0\n          offset: 2\n          encoding: hex\n"
                      "          bytes: |\n            33 44\n",
                  &err));
}

TEST(test_reject_initial_content_missing_bank_on_banked_region) {
    std::string err;
    CHECK(rejects(std::string(kMinPrefix) +
                      "  - name: r\n"
                      "    addressing: { chip-select: Y0 }\n"
                      "    banking:\n"
                      "        latch: { type: trigger-based, trigger: { pin: 18 },\n"
                      "                 sampled-lines: [A0], source-domain: address }\n"
                      "        bank-count: 2\n        bank-size: 0x20\n"
                      "        bank-window: { chip-select: Y0, span: 0x20 }\n"
                      "    content: { by-bank: [ { banks: \"0-1\", kind: regular } ] }\n"
                      "    initial-content:\n"
                      "      blocks:\n"
                      "        - offset: 0\n          encoding: hex\n          bytes: |\n            00\n",
                  &err));
}

// ── end to end ─────────────────────────────────────────────────────────

std::string makeScratchCardDir(const calcu::testing::TempFolder& tmp);  // defined below, in the catalogue section

// The SLOT2MAP gate-array remap (Port 3CH b5:b4): the firmware can make the
// Slot 2 RAM chip-select assert for a bank-1 access outside the ordinary
// page-C window -- the "(S2:) at Bank 1" path the RAM-disk file system uses.
// SLOT2MAP ROM routine: PC1600-P0-B0-new.bin 0A6DH.
TEST(test_ce1601m_slot2map_remap) {
    PC1600Bank bank;
    PC1600Memory mem(bank);
    mem.attachSlot2Card(buildCard(kCe1601mYaml, CardHost::PC1600Slot2));

    // Reference bytes, written through the ordinary (mode 0) route:
    // page-C bank 2 = Slot 2 PVOUT-low, bank 3 = PVOUT-high, vertical bank 0.
    mem.writeIO(0x28, 0);
    mem.writeIO(0x3C, 0x00);              // SLOT2MAP A=0 (default)
    mem.writeIO(0x31, 2 << 4);            // page-C bank 2
    mem.write(0x8000, 0xA1);
    mem.write(0xBFFF, 0xA2);              // top of the low 16K half
    mem.writeIO(0x31, 3 << 4);            // page-C bank 3
    mem.write(0x8000, 0xB1);             // low edge of the high 16K half

    // Mode 1 (A=1): Slot 2's low 16K half also answers at page-C bank 1,
    // which the ordinary decode would give to Slot 1.
    mem.writeIO(0x3C, 0x20);
    CHECK(bank.slot2MapMode() == 1);
    mem.writeIO(0x31, 1 << 4);            // page-C bank 1
    CHECK(mem.read(0x8000) == 0xA1);      // same module cell as mode-0 page-C bank 2
    CHECK(mem.read(0xBFFF) == 0xA2);
    mem.write(0x9000, 0x77);              // a fresh write through the remap
    mem.writeIO(0x3C, 0x00);
    mem.writeIO(0x31, 2 << 4);
    CHECK(mem.read(0x9000) == 0x77);      // landed in the module, visible via the normal route

    // Mode 2 (A=2): Slot 2's low 16K half at page-B bank 1 (4000-7FFF),
    // its high 16K half at page-A bank 1 (0000-3FFF).
    mem.writeIO(0x3C, 0x10);
    CHECK(bank.slot2MapMode() == 2);
    mem.writeIO(0x31, 1 << 1);            // page-B bank 1 (Port 31H b3:b1 = 001)
    CHECK(mem.read(0x4000) == 0xA1);      // low half, offset 0 -- the same cell again
    CHECK(mem.read(0x7FFF) == 0xA2);
    mem.writeIO(0x31, 0x01);              // page-A bank 1 (Port 31H b0 = 1)
    CHECK(mem.read(0x0000) == 0xB1);      // the HIGH half this time
    mem.write(0x0001, 0xC3);
    mem.writeIO(0x3C, 0x00);              // read the high-half write back the ordinary way
    mem.writeIO(0x31, 3 << 4);
    CHECK(mem.read(0x8001) == 0xC3);

    // A CE-1601M-class card's OUT (28H) vertical-bank latch still applies
    // under a remap -- it's the card's own state, not derived from the
    // remapped address.
    mem.writeIO(0x3C, 0x20);             // mode 1
    mem.writeIO(0x31, 1 << 4);
    mem.writeIO(0x28, 1);                 // vertical bank 1 -- the other 32K chip
    CHECK(mem.read(0x8000) == 0xFF);      // blank
    mem.write(0x8000, 0xEE);
    mem.writeIO(0x28, 0);
    CHECK(mem.read(0x8000) == 0xA1);      // vbank 0 cell intact
    mem.writeIO(0x28, 1);
    CHECK(mem.read(0x8000) == 0xEE);

    // Mode 0: the remap is inert -- page-C bank 1 is Slot 1 (empty here),
    // and page-A/B bank 1 don't reach Slot 2 at all.
    mem.writeIO(0x28, 0);
    mem.writeIO(0x3C, 0x00);
    mem.writeIO(0x31, 1 << 4);            // page-C bank 1 -> Slot 1
    CHECK(mem.read(0x8000) == 0xFF);
    mem.writeIO(0x31, 1 << 1);            // page-B bank 1
    CHECK(mem.read(0x4000) == 0xFF);
    mem.writeIO(0x31, 0x01);              // page-A bank 1
    CHECK(mem.read(0x0000) == 0xFF);      // no ROM loaded -> open bus, not Slot 2
}

// contentRevision() moves only when the stored content changes: a guest
// write of a new value, a loader's slotBusWrite(); not a write of the value
// already there (what the GUI's card autosave relies on to skip a copy).
TEST(test_card_content_revision) {
    PC1600Bank bank;
    PC1600Memory mem(bank);
    CHECK(mem.slot1CardRevision() == 0); // empty slot
    mem.attachSlot1Card(plainRamCard(0x8000));
    mem.writeIO(0x31, 0 << 4); // page-C bank 0
    mem.write(0x8000, 0x42);
    const uint64_t r1 = mem.slot1CardRevision();
    CHECK(r1 != 0);
    mem.write(0x8000, 0x42); // same value: no change
    CHECK(mem.slot1CardRevision() == r1);
    mem.write(0x8001, 0x43);
    const uint64_t r2 = mem.slot1CardRevision();
    CHECK(r2 != r1);
    const uint8_t b = 0x44;
    CHECK(mem.slotBusWrite(0, 0x8010, &b, 1));
    CHECK(mem.slot1CardRevision() != r2);
}

// SLOT2MAP with an empty Slot 2: the redirect must be inert, never divert a
// page-A/B ROM fetch into nothing.
TEST(test_slot2map_remap_no_card_is_inert) {
    PC1600Bank bank;
    PC1600Memory mem(bank);
    std::vector<uint8_t> lo(0x4000, 0x11), hi(0x4000, 0x22);
    CHECK(mem.loadBank0(lo.data(), lo.size(), hi.data(), hi.size()));

    mem.writeIO(0x3C, 0x10);              // SLOT2MAP A=2, but no Slot 2 card
    mem.writeIO(0x31, 0x01);              // page-A bank 1
    CHECK(mem.read(0x0000) == 0x11);      // still the page-A ROM
    mem.write(0x0000, 0x99);
    CHECK(mem.read(0x0000) == 0x11);      // ROM ignores the write
    CHECK(!mem.isWritable(0x0000));
}

// The SLOT1MAP gate-array remap (Port 3CH b2): when armed, Slot 1's high
// 16K half (beta, normally page-C bank 1) also answers at page-B bank 1
// (4000-7FFF) -- a mirror, not a move (TRM 0196H entry + diagram).
TEST(test_slot1map_remap) {
    PC1600Bank bank;
    PC1600Memory mem(bank);
    mem.attachSlot1Card(plainRamCard(0x8000)); // 32K unbanked: bank 0 = alpha, bank 1 = beta

    // Reference bytes via the ordinary (SLOT1MAP=0) route.
    mem.writeIO(0x3C, 0x00);
    mem.writeIO(0x31, 0 << 4);            // page-C bank 0 (alpha)
    mem.write(0x8000, 0xA1);
    mem.writeIO(0x31, 1 << 4);            // page-C bank 1 (beta)
    mem.write(0x8000, 0xB1);
    mem.write(0xBFFF, 0xB2);

    // SLOT1MAP=1: beta also answers at page-B bank 1.
    mem.writeIO(0x3C, 0x04);
    CHECK(bank.slot1MapActive());
    mem.writeIO(0x31, 1 << 1);            // page-B bank 1 (Port 31H b3:b1 = 001)
    CHECK(mem.read(0x4000) == 0xB1);      // same module cell as beta's page-C bank 1
    CHECK(mem.read(0x7FFF) == 0xB2);
    mem.write(0x4001, 0xC3);              // a fresh write through the mirror
    mem.writeIO(0x3C, 0x00);
    mem.writeIO(0x31, 1 << 4);            // read it back through the ordinary route
    CHECK(mem.read(0x8001) == 0xC3);

    // alpha (bank 0) never mirrors, and the mirror is gated on page-B
    // actually selecting bank 1 -- neither reaches Slot 1 here.
    mem.writeIO(0x3C, 0x04);
    mem.writeIO(0x31, 0x00);              // page-B bank 0, not bank 1
    CHECK(mem.read(0x4000) == 0xFF);      // open bus: SLOT1MAP armed but ungated
}

// SLOT1MAP with an empty Slot 1: the redirect must be inert.
TEST(test_slot1map_remap_no_card_is_inert) {
    PC1600Bank bank;
    PC1600Memory mem(bank);

    mem.writeIO(0x3C, 0x04);              // SLOT1MAP=1, but no Slot 1 card
    mem.writeIO(0x31, 1 << 1);            // page-B bank 1
    CHECK(mem.read(0x4000) == 0xFF);      // open bus, not diverted into nothing
    mem.write(0x4000, 0x99);
    CHECK(!mem.isWritable(0x4000));
}

// SLOT1MAP and SLOT2MAP mode 2 both mirror onto page-B bank 1 -- the one
// window they can collide on (two different physical cards). Arbitrated
// "last call wins": whichever field's value most recently changed owns the
// window; an exact tie (both changed in the same Port 3CH write) favors
// SLOT2MAP. See PC1600Bank::slot1MapWinsTie().
TEST(test_slot1map_slot2map_collision_last_call_wins) {
    PC1600Bank bank;
    PC1600Memory mem(bank);
    mem.attachSlot1Card(plainRamCard(0x8000));
    mem.attachSlot2Card(plainRamCard(0x8000));

    // Reference bytes: Slot 1's beta half, Slot 2's low half.
    mem.writeIO(0x3C, 0x00);
    mem.writeIO(0x31, 1 << 4);            // page-C bank 1 -> Slot 1 beta
    mem.write(0x8000, 0xB1);
    mem.writeIO(0x31, 2 << 4);            // page-C bank 2 -> Slot 2 low half
    mem.write(0x8000, 0xA1);

    mem.writeIO(0x31, 1 << 1);            // page-B bank 1, for the rest of this test

    // Only SLOT2MAP armed: Slot 2 answers.
    mem.writeIO(0x3C, 0x10);              // SLOT2MAP A=2
    CHECK(mem.read(0x4000) == 0xA1);

    // Exact tie -- both fields go from inactive to active in the very same
    // Port 3CH write (neither was already at this value beforehand):
    // SLOT2MAP wins (the documented fallback).
    mem.writeIO(0x3C, 0x00);
    mem.writeIO(0x3C, 0x14);              // SLOT1MAP=1 + SLOT2MAP A=2, one write
    CHECK(mem.read(0x4000) == 0xA1);

    // SLOT2MAP armed first, SLOT1MAP added in a later write (SLOT2MAP's own
    // field unchanged by it): SLOT1MAP is the more recent change and wins.
    mem.writeIO(0x3C, 0x00);
    mem.writeIO(0x3C, 0x10);              // SLOT2MAP A=2 alone
    mem.writeIO(0x3C, 0x14);              // SLOT1MAP=1 added
    CHECK(mem.read(0x4000) == 0xB1);

    // SLOT1MAP armed first, SLOT2MAP added later (SLOT1MAP's own bit
    // unchanged by it): SLOT2MAP is now the more recent change and wins.
    mem.writeIO(0x3C, 0x00);
    mem.writeIO(0x3C, 0x04);              // SLOT1MAP=1 alone
    mem.writeIO(0x3C, 0x14);              // SLOT2MAP A=2 added
    CHECK(mem.read(0x4000) == 0xA1);
}

// ── catalogue: scan a directory + resolve modulespec by module-name ────

// A throwaway directory of .module.yaml files for the catalogue tests, in `tmp`.
std::string makeScratchCardDir(const calcu::testing::TempFolder& tmp) {
    const std::string dir = tmp.str();
    auto put = [&](const char* file, const char* body) {
        std::ofstream(dir + "/" + file) << body;
    };
    put("ce155.module.yaml", kCe155Yaml);
    put("ce1600m.module.yaml", kCe1600mYaml);
    put("ce1601m.module.yaml", kCe1601mYaml);
    // Not a card file -- must be ignored by the ".module.yaml" tail match.
    put("notes.yaml", "hello: world\n");
    // Malformed card -- must be skipped, not abort the scan.
    put("broken.module.yaml", "format-version: 1\nmodule-name: BROKEN\nbogus-key: 1\n");
    return dir;
}

TEST(test_catalog_scan_lists_and_sorts) {
    const calcu::testing::TempFolder dirFolder("calcu1600-cards");
    const std::string dir = makeScratchCardDir(dirFolder);
    std::string err;
    auto entries = scanMemoryCardDirectory(dir, &err);
    CHECK(entries.size() == 3);  // ce155 / ce1600m / ce1601m; notes.yaml + broken skipped
    if (entries.size() == 3) {
        CHECK(entries[0].moduleName == "CE-155");     // sorted by module-name
        CHECK(entries[1].moduleName == "CE-1600M");
        CHECK(entries[2].moduleName == "CE-1601M");
        CHECK(entries[0].compatibleWith(CardHost::PC1500));
        CHECK(!entries[0].compatibleWith(CardHost::PC1600Slot2));
        CHECK(entries[2].compatibleWith(CardHost::PC1600Slot2));
    }
    CHECK(err.find("broken.module.yaml") != std::string::npos);  // reported, not fatal
}

TEST(test_catalog_scan_missing_dir) {
    std::string err;
    auto entries = scanMemoryCardDirectory("/no/such/module/dir", &err);
    CHECK(entries.empty());
    CHECK(!err.empty());
}

// The definitions that do not read are left out and named, each with its
// reason, for the app log; so is a folder that cannot be read.
TEST(module_definitions_problems_named) {
    const calcu::testing::TempFolder dirFolder("calcu1600-cards");
    const std::string dir = makeScratchCardDir(dirFolder);
    calcu::machine::Catalogue c = sharp::sharpCatalogue();
    std::vector<std::string> problems;
    CHECK_EQ(sharp::addModuleDefinitions(c, dir, &problems), 3);
    CHECK_EQ(problems.size(), std::size_t{1});
    CHECK(!problems.empty() && problems[0].find("The module definitions in " + dir + ": broken.module.yaml: ") == 0);
    CHECK(!c.peripheral(calcu::machine::PeripheralId{"BROKEN"}));
    problems.clear();
    CHECK_EQ(sharp::addModuleDefinitions(c, "/no/such/module/dir", &problems), 0);
    CHECK_EQ(problems.size(), std::size_t{1});
    CHECK(!problems.empty() && problems[0].find("/no/such/module/dir") != std::string::npos);
}

}  // namespace

// Without `power-up-fill`, regular RAM powers up 0x00 (CMOS RAM after a
// power loss) and flash powers up erased (0xFF).
TEST(test_power_up_fill_defaults_per_kind) {
    std::string yaml = kFlashByBankYaml;
    for (const char* line : {", power-up-fill: 0xFF", "          power-up-fill: 0xAA\n"}) {
        const size_t at = yaml.find(line);
        CHECK(at != std::string::npos);
        if (at != std::string::npos) yaml.erase(at, std::strlen(line));
    }
    MemoryCardDefinition def;
    std::string err;
    CHECK(parseMemoryCardDefinition(yaml, &def, &err));
    if (def.regions.size() != 1) return;
    CHECK(def.regions[0].contentForBank(0).powerUpFill == 0x00);
    CHECK(def.regions[0].contentForBank(2).powerUpFill == 0xFF);
}

// ── review fixes ─────────────────────────────────────────────────────────

TEST(test_yaml_apostrophe_inside_plain_scalar) {
    calcu::formats::YamlNode n;
    std::string err;
    CHECK(calcu::formats::parseYaml("name: Martin's CE-1600M  # my card\nlist: [Bob's, x]\nq: 'a # b'\n", &n, &err));
    std::string s;
    CHECK(n.find("name") && n.find("name")->asString(&s, &err) && s == "Martin's CE-1600M");
    const calcu::formats::YamlNode* l = n.find("list");
    CHECK(l && l->isSeq() && l->seq.size() == 2 && l->seq[0].scalar == "Bob's");
    CHECK(n.find("q") && n.find("q")->asString(&s, &err) && s == "a # b");
}

TEST(test_reject_unknown_key_inside_all_of) {
    std::string err;
    CHECK(rejects(std::string(kMinPrefix) +
                      "  - name: r\n    capacity: 0x800\n    banking: none\n    content: regular\n"
                      "    addressing:\n"
                      "      all-of: [{chip-selct: Y0}, {address-bits: {A13: 1}}]\n"
                      "      span: 0x800\n",
                  &err));
    CHECK(err.find("chip-selct") != std::string::npos);
}

// `write-protect:` is the module's switch position (CE-161): a scalar,
// part of the file. While protected, every write into the RAM is refused --
// host pokes (`direct`) too -- and setWriteProtected() moves it at runtime.
TEST(test_write_protect_switch) {
    auto yamlWith = [](const std::string& wp) {
        return std::string(kMinPrefix) +
               "  - name: r\n    capacity: 0x800\n    banking: none\n"
               "    content: { kind: regular, write-protect: " + wp + " }\n"
               "    addressing: { chip-select: Y0, span: 0x800 }\n";
    };
    MemoryCardDefinition def;
    std::string err;
    CHECK(parseMemoryCardDefinition(yamlWith("protected"), &def, &err));
    CHECK(def.regions.size() == 1 && def.regions[0].contentForBank(0).hasWriteProtect &&
          def.regions[0].contentForBank(0).writeProtected);
    CHECK(rejects(yamlWith("on"), &err));
    CHECK(err.find("protected|unprotected") != std::string::npos);
    CHECK(rejects(yamlWith("{ default: protected, persisted: true }"), &err));  // a map is refused

    auto sd = buildCard(yamlWith("unprotected").c_str(), CardHost::PC1500);
    CHECK(sd != nullptr);
    if (!sd) return;
    PinState p;
    p.forWrite = true;
    p.pin[4] = true;  // Y0
    p.address = 0x0010;
    CHECK(sd->hasWriteProtectSwitch() && !sd->writeProtected());
    WriteResult w = sd->respondsToWrite(p, 0x5A);
    CHECK(w.claimed && w.stored);
    sd->setWriteProtected(true);
    w = sd->respondsToWrite(p, 0xA5);
    CHECK(w.claimed && !w.stored);
    p.direct = true;  // a host poke can't get past the switch either
    w = sd->respondsToWrite(p, 0xA5);
    CHECK(w.claimed && !w.stored);
    uint8_t v = 0;
    PinState r;
    r.pin[4] = true;
    r.address = 0x0010;
    CHECK(sd->respondsToRead(r, v) && v == 0x5A);
    CHECK(!sd->debugMemories().empty() && sd->debugMemories()[0].kind == CardMemory::Kind::Rom);

    auto plain = buildCard((std::string(kMinPrefix) +
                            "  - name: r\n    capacity: 0x800\n    banking: none\n    content: regular\n"
                            "    addressing: { chip-select: Y0, span: 0x800 }\n").c_str(),
                           CardHost::PC1500);
    CHECK(plain && !plain->hasWriteProtectSwitch());
    if (plain) {
        plain->setWriteProtected(true);  // no switch: ignored
        CHECK(!plain->writeProtected());
    }
}

// A memory-range window need be neither a power of two nor aligned: its
// offset counts from `from`.
TEST(test_memory_range_offset_counts_from_range_start) {
    const std::string yaml = std::string(kMinPrefix) +
        "  - name: r\n    capacity: 0x1800\n    banking: none\n"
        "    content: { kind: regular, writable: true, power-up-fill: 0x00 }\n"
        "    addressing: { memory-range: { from: 0x4800, to: 0x5FFF } }\n"
        "    initial-content:\n"
        "      blocks:\n"
        "        - { offset: 0x0000, encoding: hex, bytes: \"5A\" }\n";
    auto sd = buildCard(yaml.c_str(), CardHost::PC1500);
    CHECK(sd != nullptr);
    if (!sd) return;
    auto write = [&](uint16_t a, uint8_t d) {
        PinState p;
        p.forWrite = true;
        p.address = a;
        return sd->respondsToWrite(p, d);
    };
    auto read = [&](uint16_t a) {
        uint8_t v = 0;
        PinState p;
        p.address = a;
        CHECK(sd->respondsToRead(p, v));
        return v;
    };
    CHECK(read(0x4800) == 0x5A);  // initial-content offset 0 lands at `from`
    CHECK(write(0x5000, 0x11));
    CHECK(write(0x5800, 0x22));
    CHECK(read(0x5000) == 0x11);  // no longer aliased with 0x5800
    CHECK(read(0x5800) == 0x22);
    CHECK(read(0x5FFF) == 0x00);
}

const char* kUnbankedFlashYaml =
    "format-version: 1\nmodule-name: T-FLASH-U\n"
    "compatible-hosts: [PC-1500]\n"
    "definition-terminology: PC-1500\n"
    "regions:\n"
    "  - name: r\n"
    "    capacity: 0x200\n"
    "    banking: none\n"
    "    addressing: { chip-select: Y0, span: 0x200 }\n"
    "    content:\n"
    "      kind: flash\n"
    "      protocol:\n"
    "        unlock-sequence:\n"
    "          - { address: 0x55, data: 0xAA }\n"
    "          - { address: 0x2A, data: 0x55 }\n"
    "        byte-program-command: 0xA0\n"
    "        erase-setup-command: 0x80\n"
    "        sector-erase-command: 0x30\n"
    "        chip-erase-command: 0x10\n"
    "        reset-command: 0xF0\n"
    "        sector-size: 0x100\n";

TEST(test_flash_unbanked_chip_and_sector_erase) {
    auto sd = buildCard(kUnbankedFlashYaml, CardHost::PC1500);
    CHECK(sd != nullptr);
    if (!sd) return;
    auto write = [&](uint16_t off, uint8_t data, bool direct = false) {
        PinState p;
        p.pin[4] = true;
        p.forWrite = true;
        p.direct = direct;
        p.address = off;
        return sd->respondsToWrite(p, data);
    };
    auto read = [&](uint16_t off) {
        uint8_t v = 0;
        PinState p;
        p.pin[4] = true;
        p.address = off;
        sd->respondsToRead(p, v);
        return v;
    };
    auto eraseCommand = [&](uint16_t off, uint8_t cmd) {
        write(0x55, 0xAA);
        write(0x2A, 0x55);
        write(0x55, 0x80);
        write(0x55, 0xAA);
        write(0x2A, 0x55);
        write(off, cmd);
    };

    write(0x000, 0x00, true);
    write(0x1FF, 0x00, true);
    eraseCommand(0x155, 0x30);  // sector erase: 0x100..0x1FF only
    CHECK(read(0x000) == 0x00);
    CHECK(read(0x1FF) == 0xFF);

    write(0x1FF, 0x00, true);
    eraseCommand(0x55, 0x10);   // chip erase clears the whole region
    CHECK(read(0x000) == 0xFF);
    CHECK(read(0x1FF) == 0xFF);
}

TEST(test_reject_unbanked_flash_sector_not_dividing_capacity) {
    // capacity 0x180 (tiled 0x100 + 0x80) with sector-size 0x100: a sector
    // erase at offset 0x100 would run past the backing store.
    std::string yaml = kUnbankedFlashYaml;
    const std::string cap = "capacity: 0x200";
    yaml.replace(yaml.find(cap), cap.size(), "capacity: 0x180");
    const std::string addr = "addressing: { chip-select: Y0, span: 0x200 }";
    yaml.replace(yaml.find(addr), addr.size(),
                 "addressing:\n"
                 "      any-of:\n"
                 "        - { chip-select: Y0, span: 0x100 }\n"
                 "        - { chip-select: S1, span: 0x80 }");
    std::string err;
    CHECK(rejects(yaml, &err));
    CHECK(err.find("sector-size") != std::string::npos);
}
