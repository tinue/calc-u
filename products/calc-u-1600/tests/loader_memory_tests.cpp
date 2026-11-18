// The product kind "loader memory access"
// (c1600.loader-memory) on every Sharp model, through the machine interface.

#include <cstdint>
#include <string>
#include <vector>

#include "SharpTestSupport.hpp"
#include "TestHarness.hpp"

namespace {

using namespace sharptest;
using sharp::LoaderMemory;

bool haveAll() { return haveModelRoms("PC-1500") && haveModelRoms("PC-1500A") && haveModelRoms("PC-1600"); }

TEST(listed_by_every_sharp_model) {
    SKIP_UNLESS(haveAll(), "Sharp ROMs missing");
    const Catalogue& c = sharp::sharpCatalogue();
    for (const char* model : {"PC-1500", "PC-1500A", "PC-1600"}) {
        auto m = buildSharp(c, model);
        CHECK(m != nullptr);
        if (!m) continue;
        const auto e = m->capabilities(LoaderMemory::kKind);
        CHECK_EQ(e.size(), std::size_t{1});
        // Owned by the machine itself (its main CPU).
        if (!e.empty()) CHECK(std::holds_alternative<calcu::machine::ComponentOwner>(e[0].owner));
        CHECK(calcu::machine::first<LoaderMemory>(*m) != nullptr);
    }
}

TEST(peek_without_effect) {
    SKIP_UNLESS(haveAll(), "Sharp ROMs missing");
    for (const char* model : {"PC-1500A", "PC-1600"}) {
        auto m = bootedSharp(sharp::sharpCatalogue(), model);
        CHECK(m != nullptr);
        if (!m) continue;
        LoaderMemory& mem = memoryOf(*m);
        const Ticks t = m->tickCount();
        const std::vector<std::uint8_t> ram = mem.internalRam();
        std::uint32_t sum = 0;
        for (std::uint32_t a = 0; a < 0x10000; ++a) sum += mem.peek(static_cast<std::uint16_t>(a));
        (void)sum;
        CHECK(m->tickCount() == t);
        CHECK(mem.internalRam() == ram);
    }
}

TEST(peek_lh5803_view_pc1600) {
    SKIP_UNLESS(haveAll(), "Sharp ROMs missing");
    auto m = bootedSharp(sharp::sharpCatalogue(), "PC-1600");
    CHECK(m != nullptr);
    if (!m) return;
    LoaderMemory& mem = memoryOf(*m);
    const std::uint8_t b = 0x5A;
    CHECK(mem.writeBuiltIn(0xE123, &b, 1).stored);
    // The LH5803's &6123 is the Z-80's &E123 (the internal RAM).
    CHECK(mem.peek(0x6123, LoaderMemory::Processor::Lh5803) == 0x5A);
    CHECK(mem.peek(0xE123) == 0x5A);
}

TEST(mode_and_area_pc1600) {
    SKIP_UNLESS(haveAll(), "Sharp ROMs missing");
    auto p = bootedSharp(sharp::sharpCatalogue(), "PC-1500A");
    CHECK(p && memoryOf(*p).mode() == LoaderMemory::Mode::None && memoryOf(*p).programArea() == 0);
    auto m = bootedSharp(sharp::sharpCatalogue(), "PC-1600");
    CHECK(m != nullptr);
    if (!m) return;
    CHECK(memoryOf(*m).mode() == LoaderMemory::Mode::Mode0);
    CHECK(memoryOf(*m).programArea() == 0);
    tap(*m, "mode");   // PRO, as a preset does before MODE1
    typeLine(*m, "new", 3);
    typeLine(*m, "mode1", 5);
    CHECK(memoryOf(*m).mode() == LoaderMemory::Mode::Mode1);
}

TEST(write_built_in_and_slot_any_bank_switched_in) {
    SKIP_UNLESS(haveAll(), "Sharp ROMs missing");
    const Catalogue c = catalogueWithModules();
    auto m = bootedSharp(c, "PC-1600", {moduleRequest("CE-1600M", "slot-1")});
    CHECK(m != nullptr);
    if (!m) return;
    LoaderMemory& mem = memoryOf(*m);
    const std::uint8_t pattern[4] = {0x11, 0x22, 0x33, 0x44};
    CHECK(mem.writeBuiltIn(0xD000, pattern, 4).stored);
    CHECK(mem.peek(0xD000) == 0x11 && mem.peek(0xD003) == 0x44);
    // Both banks of the 32 KB module, whichever the window shows now.
    for (int bank : {0, 1}) {
        CHECK(mem.writeSlot(bank, 0x9000, pattern, 4).stored);
        const auto image = sharptest::moduleIn(*m, 1)->debugImage();
        const std::size_t off = static_cast<std::size_t>(bank) * 0x4000 + 0x1000;
        CHECK(image.size() > off + 3 && image[off] == 0x11 && image[off + 3] == 0x44);
    }
    // The PC-1500: its ME0 RAM, no slot writes.
    auto p = bootedSharp(sharp::sharpCatalogue(), "PC-1500A");
    CHECK(p != nullptr);
    if (!p) return;
    CHECK(memoryOf(*p).writeBuiltIn(0x4800, pattern, 4).stored);
    CHECK(memoryOf(*p).peek(0x4803) == 0x44);
    CHECK(!memoryOf(*p).writeSlot(0, 0x8000, pattern, 4).stored);
}

TEST(slot_ram_and_module_size_banks) {
    SKIP_UNLESS(haveAll(), "Sharp ROMs missing");
    const Catalogue c = catalogueWithModules();
    auto m = bootedSharp(c, "PC-1600", {moduleRequest("CE-1600M", "slot-1")});
    CHECK(m != nullptr);
    if (!m) return;
    LoaderMemory& mem = memoryOf(*m);
    const LoaderMemory::Module one = mem.module(1), two = mem.module(2);
    CHECK(one.present && one.size == 0x8000);
    CHECK(!two.present && two.size == 0);
    CHECK(mem.slotRam(0, 0x8000, 0x4000) && mem.slotRam(1, 0x8000, 0x4000));
    CHECK(!mem.slotRam(2, 0x8000, 1) && !mem.slotRam(3, 0x8000, 1));
}

TEST(write_not_stored_says_so_writes_nothing) {
    SKIP_UNLESS(haveAll(), "Sharp ROMs missing");
    auto p = bootedSharp(sharp::sharpCatalogue(), "PC-1500A");
    CHECK(p != nullptr);
    if (!p) return;
    LoaderMemory& mem = memoryOf(*p);
    const std::uint8_t pattern[4] = {0x11, 0x22, 0x33, 0x44};
    // Two RAM bytes and two past the end of RAM: none is written.
    std::uint16_t edge = 0;
    for (std::uint32_t a = 0x4000; a < 0xC000; a += 0x100) {
        const std::uint8_t old = mem.peek(static_cast<std::uint16_t>(a));
        if (!mem.writeBuiltIn(static_cast<std::uint16_t>(a), &old, 1).stored) {
            edge = static_cast<std::uint16_t>(a);
            break;
        }
    }
    SKIP_UNLESS(edge != 0, "no RAM edge found");
    const std::uint16_t start = static_cast<std::uint16_t>(edge - 2);
    const std::uint8_t b0 = mem.peek(start), b1 = mem.peek(static_cast<std::uint16_t>(start + 1));
    const LoaderMemory::Write w = mem.writeBuiltIn(start, pattern, 4);
    CHECK(!w.stored && w.firstMiss == edge);
    CHECK(mem.peek(start) == b0 && mem.peek(static_cast<std::uint16_t>(start + 1)) == b1);

    auto m = bootedSharp(sharp::sharpCatalogue(), "PC-1600");
    CHECK(m != nullptr);
    if (!m) return;
    const LoaderMemory::Write s = memoryOf(*m).writeSlot(0, 0x8000, pattern, 4);   // no module
    CHECK(!s.stored && s.firstMiss == 0x8000);
    CHECK(!memoryOf(*m).writeBuiltIn(0x8000, pattern, 4).stored);   // not the built-in RAM
}

} // namespace
