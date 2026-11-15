// Shared/ModuleBay, Shared/FloppyBay, Shared/TapeBay against the real ROMs
// and the bundled cards/disks (Qt6/resources/cards), in a throwaway bundle
// and save folder. SKIP without ROMs.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

#include "SessionFixture.hpp"
#include "Machine/SharpProductKinds.hpp"
#include "app/DirectoryScanCache.hpp"
#include "app/FloppyBay.hpp"
#include "app/ModuleBay.hpp"
#include "app/TapeBay.hpp"
#include "TestHarness.hpp"

namespace {

namespace fs = std::filesystem;
using shared::Model;
using calcu::sessions::Value;
using sessionfixture::SessionFixture;
namespace c = shared::commands;

bool contains(const std::vector<shared::ModuleBay::ModuleEntry>& v, const std::string& name) {
    for (const auto& e : v)
        if (e.moduleName == name) return true;
    return false;
}

bool contains(const std::vector<shared::FloppyBay::DiskEntry>& v, const std::string& name) {
    for (const auto& e : v)
        if (e.diskName == name) return true;
    return false;
}

// A listed disk's `description:`, "" if it isn't listed.
std::string diskDescription(const shared::FloppyBay& bay, const std::string& name) {
    const auto lists = bay.diskLists();
    for (const auto* entries : {&lists.templates, &lists.instances})
        for (const auto& e : *entries)
            if (e.diskName == name) return e.description;
    return {};
}

Value toggle(const char* p) { return Value::object({{"peripheral", p}}); }

TEST(test_module_bay) {
    SKIP_UNLESS(std::ifstream("roms/PC1600-P0-B0-new.bin").good(), "no ROMs in roms/");
    SessionFixture f("calcu-shared-bay-test");
    f.open();
    shared::ModuleBay* modules = &f.part().modules();
    const auto lists = modules->moduleLists(CardHost::PC1600Slot1);
    CHECK(contains(lists.templates, "CE-1600M"));
    for (const auto& e : lists.templates)
        if (e.moduleName == "CE-1600M") CHECK(e.description == "32 KB battery-backed RAM");
    CHECK(!contains(lists.templates, "CE-1601M"));  // Slot 2 only
    CHECK(lists.instances.empty());

    modules->selectModule(1, "CE-1600M");
    CHECK(f.start("PC-1600"));
    CHECK(f.notices().empty());
    CHECK(f.part().slotCard(1) && f.part().slotCard(1)->moduleName() == "CE-1600M");
    CHECK(modules->canNameAndSave(1));
    CHECK(!modules->slotHasInstanceFile(1));

    std::string error;
    CHECK(!modules->nameAndSave(1, "CE-1600M", &error));  // a template's name
    CHECK(modules->nameAndSave(1, "My RAM", &error));
    const std::string saved = f.paths->instancePathFor("My RAM");
    CHECK(fs::exists(saved));
    CHECK(modules->selectedModuleName(1) == "My RAM");
    CHECK(modules->slotHasInstanceFile(1));
    CHECK(!modules->canNameAndSave(1));
    CHECK(contains(modules->moduleLists(CardHost::PC1600Slot1).instances, "My RAM"));
    CHECK(!modules->nameAndSave(1, "Other", &error));  // already saved: autosave

    // A rebuild re-attaches the saved instance from its file.
    CHECK(f.ok(c::kPC1600Rom, Value::object({{"version", "old"}})));
    CHECK(f.part().slotCard(1) && f.part().slotCard(1)->moduleName() == "My RAM");
    // A model switch clears the slots.
    CHECK(f.ok(c::kPickModel, Value::object({{"model", "PC-1500A"}})));
    CHECK(modules->selectedModuleName(1).empty());
}

TEST(test_floppy_bay) {
    SKIP_UNLESS(std::ifstream("roms/PC1600-P0-B0-new.bin").good(), "no ROMs in roms/");
    SessionFixture f("calcu-shared-bay-test");
    f.open();
    shared::FloppyBay* floppy = &f.part().floppy();
    const auto lists = floppy->diskLists();
    CHECK(contains(lists.templates, "Formatted"));
    CHECK(diskDescription(*floppy, "Formatted") == "Formatted, both sides empty");
    CHECK(f.start("PC-1600"));
    floppy->selectDisk("Formatted");  // no drive yet: selection only
    CHECK(floppy->selectedDiskName() == "Formatted");
    CHECK(f.ok(c::kTogglePeripheral, toggle("CE-1600P")));  // inserts the selected disk
    sharp::FloppyDriveAccess* m = f.part().floppyDrive();
    CHECK(m && m->hasDisk());
    if (!m) return;
    CHECK(floppy->canNameAndSave());
    floppy->toggleSide();
    CHECK(floppy->side() == 1);

    std::string error;
    CHECK(floppy->nameAndSave("Work", &error));
    const std::string saved = f.paths->floppyInstancePathFor("Work");
    CHECK(fs::exists(saved));
    CHECK(!floppy->canNameAndSave());
    CHECK(diskDescription(*floppy, "Work") == "Formatted, both sides empty");  // kept from the template

    // The drive writes: autosaved once the debounce has passed, not before.
    fs::remove(saved);
    std::vector<std::uint8_t> image = m->diskImage();
    image[100] ^= 0xFF;
    m->loadImage(image.data(), image.size());
    floppy->tick(10.0);
    floppy->tick(10.3);
    CHECK(!fs::exists(saved));
    floppy->tick(10.6);
    CHECK(fs::exists(saved));
    CHECK(diskDescription(*floppy, "Work") == "Formatted, both sides empty");  // and by the autosave

    // A detach of the CE-1600P saves the disk first, without waiting for the
    // debounce. A CE-150 no longer takes its place: the printer place holds
    // one (LIB-2 AC-29).
    fs::remove(saved);
    image[100] ^= 0xFF;
    m->loadImage(image.data(), image.size());
    floppy->tick(20.0);
    std::string why;
    CHECK(!f.part().attachCE150(&why));
    CHECK(!why.empty());
    CHECK(f.part().ce1600pAttached());
    CHECK(f.ok(c::kTogglePeripheral, toggle("CE-1600P")));
    CHECK(!f.part().ce1600pAttached());
    CHECK(fs::exists(saved));
}

TEST(test_tape_bay) {
    SKIP_UNLESS(std::ifstream("roms/PC1600-P0-B0-new.bin").good(), "no ROMs in roms/");
    SessionFixture f("calcu-shared-bay-test");
    CHECK(f.start("PC-1500A"));
    int changed = 0;
    shared::TapeBay tapes(f.part(), *f.paths, shared::TapeBay::Events{[&] { ++changed; }, {}});
    CHECK(tapes.tapeNames().empty());
    std::ofstream(f.paths->tapePathFor("b side")) << "x";
    std::ofstream(f.paths->tapePathFor("A side")) << "x";
    const auto names = tapes.tapeNames();
    CHECK(names.size() == 2 && names[0] == "A side" && names[1] == "b side");
    CHECK(tapes.exists("A side"));
    { std::string why; CHECK(!tapes.recordNew("   ", &why) && !why.empty()); }
    CHECK(tapes.recordNew(" New ", nullptr));
    CHECK(tapes.currentName() == "New");
    std::string error;
    CHECK(tapes.selectForPlay("", &error));
    CHECK(tapes.currentName().empty());
}

} // namespace

// DirectoryScanCache: one parse while the files stay put; a new, grown or
// removed file makes the next get() parse again.
TEST(test_directory_scan_cache) {
    SKIP_UNLESS(std::ifstream("roms/PC1600-P0-B0-new.bin").good(), "no ROMs in roms/");
    const fs::path dir = fs::temp_directory_path() / "calcu-scan-cache-test";
    fs::remove_all(dir);
    fs::create_directories(dir);
    int scans = 0;
    auto scan = [&](const std::string& d) {
        ++scans;
        std::vector<std::string> names;
        for (const auto& e : fs::directory_iterator(d)) names.push_back(e.path().filename().string());
        return names;
    };
    shared::DirectoryScanCache<std::string> cache;
    CHECK(cache.get(dir.string(), scan).empty() && scans == 1);
    CHECK(cache.get(dir.string(), scan).empty() && scans == 1);
    std::ofstream(dir / "a.card.yaml") << "x";
    CHECK(cache.get(dir.string(), scan).size() == 1 && scans == 2);
    CHECK(cache.get(dir.string(), scan).size() == 1 && scans == 2);
    std::ofstream(dir / "a.card.yaml", std::ios::app) << "more";
    cache.get(dir.string(), scan);
    CHECK(scans == 3);
    fs::remove(dir / "a.card.yaml");
    CHECK(cache.get(dir.string(), scan).empty() && scans == 4);
    fs::remove_all(dir);
}
