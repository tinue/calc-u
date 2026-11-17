#pragma once
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../core/Display/DisplayOutputText.hpp"
#include "../core/Machine/SharpBuilder.hpp"
#include "../core/Machine/SharpCatalogue.hpp"
#include "../core/Machine/SharpCores.hpp"
#include "../core/Machine/SharpProductKinds.hpp"
#include "BootRun.hpp"
#include "TestCards.hpp"
#include "TestRoms.hpp"
#include "TestHarness.hpp"

// Sharp machines built through the machine interface for the M-26 tests
// (sharp_machine_tests, descriptor_tests, peripherals_tests). Run from
// products/calc-u-1600 (CoreTests' working directory). The ROM images come
// from the ROM set of roms/ (testRomSet(), LIB-12); a helper returns null /
// false when a file of the full set is missing, so a test can SKIP.

namespace sharptest {

using namespace calcu::machine;

inline const char* cardsDir() { return "frontends/qt6/resources/cards"; }

// The images of a model's variant from the test ROM set; false with `error`
// without the set.
inline bool modelImages(const Catalogue& cat, const ModelId& model, const std::optional<VariantId>& v,
                        std::vector<RomImage>* out, std::string* error) {
    const MachineDescriptor* d = cat.model(model);
    if (!testRomSet() || !d) {
        if (error) *error = "no ROM set in roms/ (or no model " + model.value + ")";
        return false;
    }
    *out = testRomSet()->imagesFor(*d, v);
    return true;
}
// The images of a peripheral's variant: from the test ROM set, the host
// drive's from firmware/pc1600-hostdrive/ (roms.md Q-1).
inline bool peripheralImages(const Catalogue& cat, const PeripheralId& id, const std::optional<VariantId>& v,
                             std::vector<RomImage>* out, std::string* error) {
    const PeripheralDescriptor* d = cat.peripheral(id);
    if (!d) {
        if (error) *error = id.value + " is not a known peripheral";
        return false;
    }
    if (d->romsFor(v).empty()) return true;
    if (id.value == sharp::kHostDrive) {
        if (!testPresetRoms().hostDrive) {
            if (error) *error = "no host drive ROM";
            return false;
        }
        out->push_back({d->roms.at(0).fileName, *testPresetRoms().hostDrive});
        return true;
    }
    if (!testRomSet()) {
        if (error) *error = "no ROM set in roms/";
        return false;
    }
    *out = testRomSet()->imagesFor(*d, v);
    return true;
}

// The catalogue with the bundled module definitions added: the folder's,
// and the ROM set's (the CE-502B, LIB-12 AC-14).
inline Catalogue catalogueWithModules() {
    Catalogue c = sharp::sharpCatalogue();
    sharp::addModuleDefinitions(c, cardsDir());
    if (testRomSet()) sharp::addModuleDefinitions(c, *testRomSet());
    return c;
}

inline bool haveModelRoms(const char* model, const std::optional<VariantId>& v = std::nullopt) {
    std::vector<RomImage> images;
    std::string error;
    return modelImages(sharp::sharpCatalogue(), ModelId{model}, v, &images, &error);
}

inline PlacePath place(const char* p) { return PlacePath{{PlaceName{p}}}; }

// A peripheral request with its ROM images; `ok` false when one is missing.
inline PeripheralRequest request(const Catalogue& cat, const char* id, const char* where,
                                 std::optional<VariantId> variant = std::nullopt, bool* ok = nullptr) {
    PeripheralRequest r{place(where), PeripheralId{id}, std::move(variant), {}, nullptr, {}};
    std::string error;
    const bool got = peripheralImages(cat, r.peripheral, r.variant, &r.roms, &error);
    if (ok) *ok = got;
    return r;
}

inline PeripheralRequest hostDriveRequest(const Catalogue& cat, const std::string& folder) {
    PeripheralRequest r = request(cat, sharp::kHostDrive, sharp::kHostDrivePlace);
    r.content = std::make_shared<sharp::HostDriveRequest>(folder);
    return r;
}

// A module from its definition in the catalogue (built by the machine).
inline PeripheralRequest moduleRequest(const char* name, const char* where) {
    return PeripheralRequest{place(where), PeripheralId{name}, std::nullopt, {}, nullptr, {}};
}

// A built machine (not started), or null when its ROMs are missing.
inline std::unique_ptr<Machine> buildSharp(const Catalogue& cat, const char* model,
                                           std::optional<VariantId> variant = std::nullopt,
                                           std::vector<PeripheralRequest> peripherals = {}, std::string* refusal = nullptr) {
    BuildRequest req;
    req.model = ModelId{model};
    req.variant = std::move(variant);
    req.soundRate = sharp::kSoundRate;
    req.peripherals = std::move(peripherals);
    std::string error;
    if (!modelImages(cat, req.model, req.variant, &req.roms, &error)) return nullptr;
    sharp::SharpMachineBuilder builder;
    BuildResult r = build(cat, builder, req);
    if (refusal) *refusal = r.refusal;
    return std::move(r.machine);
}

// Cold boot and the boot run to the prompt.
inline void boot(Machine& m) {
    m.coldBoot();
    sharptest::bootRun(m);
}

inline void tap(Machine& m, const std::string& id);
inline void typeLine(Machine& m, const std::string& text, double settleSeconds = 1);
inline bool runUntilKeyReady(Machine& m, double capSeconds = 30);

// Built, cold booted, at the prompt. A PC-1500 with fresh RAM asks
// `NEW0? :CHECK` first: answered with CL, NEW0, ENTER, as the golden runs do.
inline std::unique_ptr<Machine> bootedSharp(const Catalogue& cat, const char* model,
                                            std::vector<PeripheralRequest> peripherals = {}) {
    auto m = buildSharp(cat, model, std::nullopt, std::move(peripherals));
    if (!m) return m;
    boot(*m);
    if (m->model().value != "PC-1600") {
        tap(*m, "cl");
        typeLine(*m, "new0");
        runUntilKeyReady(*m, 5);
    }
    return m;
}

inline Ticks frameTicks(const Machine& m) { return m.descriptor().tickRate / 60; }
inline Ticks secondsTicks(const Machine& m, double s) {
    return static_cast<Ticks>(static_cast<double>(m.descriptor().tickRate) * s);
}

// Advances a frame at a time until key readiness; false at the cap.
inline bool runUntilKeyReady(Machine& m, double capSeconds) {
    const Ticks cap = secondsTicks(m, capSeconds);
    Ticks spent = 0;
    while (!m.isKeyReady()) {
        if (spent >= cap) return false;
        spent += m.advance(frameTicks(m));
    }
    return true;
}

// Advances a frame at a time until `pred`; false at the cap.
template <class P> bool runUntil(Machine& m, P pred, double capSeconds) {
    const Ticks cap = secondsTicks(m, capSeconds);
    Ticks spent = 0;
    while (!pred()) {
        if (spent >= cap) return false;
        spent += m.advance(frameTicks(m));
    }
    return true;
}

// A tap of key `id`: 4 frames down, 4 frames up (the ROMs' key scan sees it
// on both models; the PC-1500's queue cadence).
inline void tap(Machine& m, const std::string& id) {
    m.keyDown(KeyId{id});
    m.advance(frameTicks(m) * 4);
    m.keyUp(KeyId{id});
    m.advance(frameTicks(m) * 4);
}

// Types `text` (lower-case letters, digits, space and = + * / ( ) . -),
// then `enter`.
inline void typeLine(Machine& m, const std::string& text, double settleSeconds) {
    for (char c : text) tap(m, c == ' ' ? std::string("space") : std::string(1, c));
    tap(m, "enter");
    // The line is stored or run: wait until the ROM takes keys again, as
    // the typers do, at most `settleSeconds`.
    m.advance(frameTicks(m) * 6);
    runUntilKeyReady(m, settleSeconds);
}

inline std::string screen(Machine& m) { return displayText(m).text(); }

inline bool indicatorOn(Machine& m, const char* name) {
    for (const auto& i : m.readDisplay().indicators)
        if (i.name == name) return i.on;
    return false;
}
// Taps MODE until the `RUN` or `PRO` indicator is lit (the PC-1500 is in
// PRO after NEW0, the PC-1600 in RUN after a cold boot).
inline void toMode(Machine& m, const char* indicator) {
    for (int i = 0; i < 3 && !indicatorOn(m, indicator); ++i) tap(m, "mode");
}
// Stores the BASIC line `line` (PRO mode) and starts the program (RUN mode).
inline void runProgram(Machine& m, const std::string& line) {
    toMode(m, "PRO");
    typeLine(m, line);
    toMode(m, "RUN");
    typeLine(m, "run", 0.1);
}

inline sharp::LoaderMemory& memoryOf(Machine& m) { return *calcu::machine::first<sharp::LoaderMemory>(m); }

// The card in a module slot (from 1) as plugged() shows it, or nullptr (the
// PC-1500 has one slot). Was sharp::legacy::moduleIn until M-51.
inline const ExpansionCard* moduleIn(Machine& m, int slot) {
    if (slot == 2 && sharp::pc1500Of(m)) return nullptr;
    sharp::SharpMachine* s = sharp::sharpOf(m);
    if (!s || (slot != 1 && slot != 2)) return nullptr;
    return s->moduleNow(PlaceName{slot == 2 ? sharp::kSlot2 : sharp::kSlot1});
}

// An address where a write lands in the module at `slot` (from 1), found by
// writing and restoring a byte every 0x100 bytes in the CPU's view; -1 if
// none.
inline long moduleAddress(Machine& m, int slot) {
    sharp::LoaderMemory& mem = memoryOf(m);
    const ExpansionCard* card = moduleIn(m, slot);
    if (!card) return -1;
    for (std::uint32_t a = 0; a < 0x10000; a += 0x100) {
        const std::uint16_t addr = static_cast<std::uint16_t>(a + 0x40);
        const std::uint8_t old = mem.peek(addr);
        const std::vector<std::uint8_t> before = card->debugImage();
        const std::uint8_t probe = static_cast<std::uint8_t>(old ^ 0x5A);
        if (!mem.writeBuiltIn(addr, &probe, 1).stored) continue;
        const bool inModule = card->debugImage() != before;
        mem.writeBuiltIn(addr, &old, 1);
        if (inModule) return addr;
    }
    return -1;
}

// Writes four bytes into the module at `slot` (from 1): on the PC-1500
// through the loader memory, on the PC-1600 through the slot's pins (its
// RAM is not in the Z-80's view at the prompt). True when the module's
// content changed.
inline bool scribbleModule(Machine& m, int slot) {
    const ExpansionCard* card = moduleIn(m, slot);
    if (!card) return false;
    const std::vector<std::uint8_t> before = card->debugImage();
    static std::uint8_t round = 0;   // another pattern each time, so a second write changes the content
    ++round;
    const std::uint8_t pattern[4] = {static_cast<std::uint8_t>(0xA5 ^ round), 0x5A, static_cast<std::uint8_t>(0xC3 + round), 0x3C};
    if (PC1600Machine* p = sharp::pc1600Of(m)) {
        for (int bank = 0; bank < 4; ++bank)
            for (std::uint32_t a = 0x8040; a < 0xC000; a += 0x400)
                if (p->debugSlotBusWritable(bank, static_cast<std::uint16_t>(a), 4)) {
                    p->debugWriteSlotBus(bank, static_cast<std::uint16_t>(a), pattern, 4);
                    if (card->debugImage() != before) return true;
                }
        return false;
    }
    const long at = moduleAddress(m, slot);
    return at >= 0 && memoryOf(m).writeBuiltIn(static_cast<std::uint16_t>(at), pattern, 4).stored && card->debugImage() != before;
}

inline bool hasKind(Machine& m, KindId k) { return !m.capabilities(k).empty(); }

// (Keep the Plugged alive while using the pointer.)
inline const PlugEntry* pluggedAt(const Plugged& p, const char* where) {
    for (const auto& e : p.byPlace)
        if (e.place == place(where)) return &e;
    return nullptr;
}

} // namespace sharptest
