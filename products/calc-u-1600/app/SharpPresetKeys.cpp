#include "app/SharpPresetKeys.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <variant>

#include "BasicTyping.hpp"
#include "Connector/MemoryCardDefinition.hpp"
#include "Loaders.hpp"
#include "Machine/SharpCatalogue.hpp"
#include "Machine/SharpMachine.hpp"
#include "app/SharpDebugBlock.hpp"
#include "formats/Flow.hpp"
#include "formats/Number.hpp"
#include "presets/Engine.hpp"
#include "sessions/LibraryTopics.hpp"
#include "typing/Awaitable.hpp"

namespace shared {

namespace {

using calcu::formats::YamlNode;
using calcu::presets::After;
using calcu::presets::Item;
using calcu::presets::ItemResult;
using calcu::presets::ReadContext;
using calcu::presets::RunContext;
using calcu::sessions::Task;

constexpr const char* kHostDriveKey = "host-drive";
constexpr const char* kBusRomKey = "bus-rom";

int lineOf(const YamlNode& n, int fallback) { return n.line ? n.line : fallback; }

std::string text(const YamlNode& n) { return calcu::formats::scalarText(n); }

std::string fileNameOf(const std::string& path) {
    const std::size_t slash = path.find_last_of("/\\:");
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

// A key given twice inside one block (AC-1: "A field given twice refuses").
bool twice(const YamlNode& block, ReadContext& ctx, const std::string& owner, int line) {
    std::set<std::string> seen;
    bool found = false;
    for (const auto& [k, v] : block.map)
        if (!seen.insert(k).second) {
            ctx.error(lineOf(v, line), owner + ": " + k + ": is given twice.");
            found = true;
        }
    return found;
}

// ---- program: (AC-1 to AC-5) ----

enum class ProgramWay { LoadBasic, TypeIn, Code };

class ProgramItem final : public Item {
public:
    ProgramWay way = ProgramWay::LoadBasic;
    std::vector<std::uint8_t> bytes;   // LoadBasic, Code
    std::string listing;               // TypeIn
    std::string name;                  // the file as written, or "text"
    sharp::loaders::CodeRequest request;

    Task<ItemResult> carryOut(RunContext& run) override {
        calcu::machine::Machine* m = run.machine();
        if (!m) co_return ItemResult{false, calcu::sessions::kNoMachine, {}};
        switch (way) {
            case ProgramWay::LoadBasic: {
                const sharp::loaders::BasicLoaded r = sharp::loaders::loadBasic(*m, bytes);
                if (!r.ok) co_return failed(sharp::loaders::refusal(name, r.refusal));
                co_return ItemResult{};
            }
            case ProgramWay::Code: {
                auto loaded = sharp::loaders::loadCode(*m, bytes, request);
                if (auto* why = std::get_if<std::string>(&loaded)) co_return failed(sharp::loaders::refusal(name, *why));
                const sharp::loaders::CodeLoaded& code = std::get<sharp::loaders::CodeLoaded>(loaded);
                run.log("  program: " + code.summary);
                if (!code.headerStart) co_return ItemResult{};
                // Auto-run (preset.md §4.1): the starting command; the
                // implicit wait lets the code return (EC-3, LIB-6 AC-57).
                calcu::typing::TypingEngine* engine = run.typing();
                if (!engine) co_return failed("This product types no keys.");
                const std::string command = code.advice.callCommand;
                const calcu::typing::TypeResult t = co_await calcu::typing::typeTextAndWait(
                    run.op(), *engine, command, true, {calcu::typing::Origin::Preset, run.firstPressAtOnce()});
                if (!t.done) co_return failed(t.reason);
                run.log("  program: started with " + command);
                co_return ItemResult{};
            }
            case ProgramWay::TypeIn: {
                calcu::typing::TypingEngine* engine = run.typing();
                if (!engine) co_return failed("This product types no keys.");
                const BasicTypeResult r = co_await sharp::loaders::typeProgram(run.op(), *engine, *m, listing);
                if (r.ok) co_return ItemResult{};
                // AC-4: no line stored, or the lines that were not.
                std::string reason = r.error;
                if (!r.rejectedLines.empty()) {
                    reason += ":";
                    for (std::size_t i = 0; i < r.rejectedLines.size(); ++i)
                        reason += (i == 0 ? " \"" : ", \"") + r.rejectedLines[i] + "\"";
                }
                if (!reason.empty() && reason.back() != '.') reason += '.';
                if (!reason.empty()) reason[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(reason[0])));
                co_return failed(reason);
            }
        }
        co_return ItemResult{};
    }
    After after() const override { return After::Idle; }
    bool needsRunningMachine() const override { return true; }
    std::string logText() const override {
        if (name.empty()) return "program: text";
        return "program: " + name;
    }

private:
    static ItemResult failed(std::string why) { return ItemResult{false, std::move(why), {}}; }
};

// AC-5: the lines of a block scalar as LIB-6 AC-1 takes them: empty lines
// and lines that start with `#` left out; the rest typed as it is.
std::string programLines(const std::string& block) {
    std::istringstream in(block);
    std::string out;
    for (std::string line; std::getline(in, line);) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const std::size_t first = line.find_first_not_of(' ');
        if (first == std::string::npos || line[first] == '#') continue;
        out += line + "\n";
    }
    return out;
}

std::unique_ptr<Item> readProgram(const YamlNode& n, ReadContext& ctx) {
    const int at = n.line;
    if (!n.isMap() || n.map.empty()) {   // EC-2
        ctx.error(at, "program: is a block with file: or text:.");
        return nullptr;
    }
    if (twice(n, ctx, "program", at)) return nullptr;
    const YamlNode* file = n.find("file");
    const YamlNode* textNode = n.find("text");
    const YamlNode* typed = n.find("typed");
    const YamlNode* address = n.find("address");
    const YamlNode* length = n.find("length");
    bool ok = true;
    for (const auto& [k, v] : n.map)
        if (k != "file" && k != "text" && k != "typed" && k != "address" && k != "length") {   // EC-2
            ctx.error(lineOf(v, at), "program: has no field " + k + "; its fields are file, text, typed, address and length.");
            ok = false;
        }
    if (!ok) return nullptr;
    if (!file == !textNode) {
        ctx.error(at, "program: takes exactly one of file: and text:.");
        return nullptr;
    }
    auto item = std::make_unique<ProgramItem>();
    if (textNode) {
        if (typed || address || length) {
            ctx.error(at, "program: text: is typed in; typed:, address: and length: go with file:.");
            return nullptr;
        }
        const std::string lines = textNode->isScalar() ? programLines(textNode->scalar) : std::string();
        if (lines.empty()) {   // EC-1
            ctx.error(lineOf(*textNode, at), "program: text: needs the lines of a BASIC program below it.");
            return nullptr;
        }
        item->way = ProgramWay::TypeIn;
        item->listing = lines;
        return item;
    }
    // file: (AC-2): read and its kind told now.
    const int fileLine = lineOf(*file, at);
    const std::string path = file->isScalar() ? text(*file) : std::string();
    if (path.empty()) {
        ctx.error(fileLine, "program: file: needs a path.");
        return nullptr;
    }
    bool typeIn = false;
    if (typed) {
        const std::string t = typed->isScalar() ? text(*typed) : std::string();
        if (t != "true" && t != "false") {
            ctx.error(lineOf(*typed, at), "program: typed: is true or false.");
            return nullptr;
        }
        typeIn = t == "true";
    }
    if (address) {
        const auto a = address->isScalar() ? calcu::formats::parseNumber(text(*address)) : std::nullopt;
        if (!a || *a < 0 || *a > 0xFFFF) {
            ctx.error(lineOf(*address, at), "program: address: is a number from 0 to &FFFF.");
            return nullptr;
        }
        item->request.address = static_cast<std::uint32_t>(*a);
    }
    if (length) {
        const auto l = length->isScalar() ? calcu::formats::parseNumber(text(*length)) : std::nullopt;
        if (!l || *l < 0) {
            ctx.error(lineOf(*length, at), "program: length: is a number of bytes.");
            return nullptr;
        }
        item->request.length = static_cast<std::size_t>(*l);
    }
    auto bytes = ctx.readFile(path, fileLine);   // EC-5: without a folder a relative path is refused
    if (!bytes) return nullptr;
    const std::string name = fileNameOf(path);
    item->name = path;
    std::string why;
    auto image = sharp::loaders::image(*bytes, 0, &why);   // a WAV: its first file
    if (!image) {
        ctx.error(fileLine, "program: " + name + ": " + why);
        return nullptr;
    }
    const sharp::loaders::FileKind kind = sharp::loaders::fileKind(*image);
    using programfile::Kind;
    switch (kind.kind) {
        case Kind::BasicListing:
            item->way = typeIn ? ProgramWay::TypeIn : ProgramWay::LoadBasic;
            break;
        case Kind::BasicPC1500:
        case Kind::BasicPC1600:
            if (typeIn) {
                ctx.error(lineOf(*typed, at), "program: " + name + " is tokenized BASIC; only a listing can be typed in (typed: true).");
                return nullptr;
            }
            item->way = ProgramWay::LoadBasic;
            break;
        case Kind::CodeLH5801:
        case Kind::CodeZ80:
        case Kind::Headerless:
            if (typeIn) {
                ctx.error(lineOf(*typed, at), "program: " + name + " is machine code; only a BASIC listing can be typed in (typed: true).");
                return nullptr;
            }
            item->way = ProgramWay::Code;
            break;
        case Kind::Empty:
            ctx.error(fileLine, "program: " + name + " is empty.");
            return nullptr;
        case Kind::Other:
            ctx.error(fileLine, "program: " + name + " is " + (kind.words.empty() ? std::string("no program") : kind.words) +
                                    ", not a program.");
            return nullptr;
    }
    if (item->way != ProgramWay::Code && (address || length)) {
        ctx.error(lineOf(address ? *address : *length, at),
                  "program: address: and length: go with machine code; " + name + " is BASIC.");
        return nullptr;
    }
    if (item->way == ProgramWay::TypeIn) item->listing = programLines(std::string(image->begin(), image->end()));
    else item->bytes = std::move(*image);
    return item;
}

// ---- host-drive: (AC-6, AC-7) ----

struct HostDriveValue final : calcu::presets::ProductValue {
    std::string folder;
};

// `{ <key>: <text> }` with only that key; the text, or nullopt with the error.
std::optional<std::string> referenceOf(const YamlNode& n, int line, const char* owner, const char* key, ReadContext& ctx) {
    if (!n.isMap() || n.map.size() != 1 || !n.find(key)) {
        ctx.error(line, std::string(owner) + ": is { " + key + ": <path> }.");
        return std::nullopt;
    }
    const std::string t = text(*n.find(key));
    if (t.empty()) {
        ctx.error(line, std::string(owner) + ": " + key + ": needs a path.");
        return std::nullopt;
    }
    return t;
}

// ---- slot-N-rom: (AC-8) ----

struct SlotRomValue final : calcu::presets::ProductValue {
    std::string slot;   // slot-1, slot-2
    std::string path;
    std::vector<std::uint8_t> bytes;
    int line = 0;
};

// ---- bus-rom: (AC-9, AC-10) ----

struct BusRomList final : calcu::presets::ProductValue {
    std::vector<sharp::BusRomRequest> roms;   // in list order
};

void readBusRoms(const YamlNode& n, int line, ReadContext& ctx) {
    const bool pc1600 = ctx.model().model.value == sharp::kPC1600;
    if (!n.isSeq() || n.seq.empty()) {
        ctx.error(line, "bus-rom: is a list of ROMs, each - file: with bank: or address:.");
        return;
    }
    auto list = std::make_shared<BusRomList>();
    bool ok = true;
    auto fail = [&](int at, const std::string& what) {
        ctx.error(at, "bus-rom: " + what);
        ok = false;
    };
    for (const YamlNode& item : n.seq) {
        const int at = lineOf(item, line);
        if (!item.isMap() || !item.find("file")) {
            fail(at, "each ROM needs file:.");
            continue;
        }
        bool itemOk = true;
        for (const auto& [k, v] : item.map)
            if (k != "file" && k != "bank" && k != "address" && k != "me" && k != "pv" && k != "pu") {
                fail(lineOf(v, at), "a ROM has no key " + k + "; its keys are file, bank, address, me, pv and pu.");
                itemOk = false;
            }
        if (!itemOk) continue;
        auto number = [&](const char* key, std::int64_t lo, std::int64_t hi, const std::string& expected) -> std::optional<std::int64_t> {
            const YamlNode* v = item.find(key);
            const auto x = v && v->isScalar() ? calcu::formats::parseNumber(text(*v)) : std::nullopt;
            if (!x || *x < lo || *x > hi) {
                fail(v ? lineOf(*v, at) : at, std::string(key) + ": is " + expected + ".");
                return std::nullopt;
            }
            return x;
        };
        const bool hasBank = item.find("bank"), hasAddress = item.find("address");
        if (hasBank == hasAddress) {
            fail(at, "give either bank: (the PC-1600's system bus) or address:.");
            continue;
        }
        sharp::BusRomRequest rom;
        std::size_t room = 0;
        if (hasBank) {
            if (!pc1600) {   // EC-6
                fail(lineOf(*item.find("bank"), at), "the " + ctx.model().model.value + " takes address:, not bank: (a bank is the PC-1600's).");
                continue;
            }
            if (item.find("me") || item.find("pv") || item.find("pu")) {
                fail(at, "me:, pv: and pu: go with address:, not bank:.");
                continue;
            }
            const auto b = number("bank", 4, 7, "a bank from 4 to 7");
            if (!b) continue;
            rom.bank = static_cast<int>(*b);
            room = 0x4000;
        } else {
            const auto a = number("address", 0, 0xFFFF, "an address from &0000 to &FFFF");
            if (!a) continue;
            rom.address = static_cast<std::uint16_t>(*a);
            room = 0x10000u - rom.address;
            bool flagsOk = true;
            for (const char* f : {"me", "pv", "pu"}) {
                if (!item.find(f)) continue;
                const auto v = number(f, 0, 1, "0 or 1");
                if (!v) {
                    flagsOk = false;
                    continue;
                }
                if (std::string(f) == "me") rom.me1 = *v == 1;
                else if (std::string(f) == "pv") rom.pv = static_cast<int>(*v);
                else rom.pu = static_cast<int>(*v);
            }
            if (!flagsOk) continue;
        }
        const YamlNode& file = *item.find("file");
        const std::string path = text(file);
        auto bytes = ctx.readFile(path, lineOf(file, at));
        if (!bytes) {
            ok = false;
            continue;
        }
        if (bytes->empty() || bytes->size() > room) {
            fail(lineOf(file, at), path + " has " + std::to_string(bytes->size()) + " bytes; " +
                                       (hasBank ? "a bank" : "the room from its address to &FFFF") + " holds 1 to " +
                                       std::to_string(room) + ".");
            continue;
        }
        rom.bytes = std::move(*bytes);
        rom.name = fileNameOf(path);
        list->roms.push_back(std::move(rom));
    }
    if (ok) ctx.keep(kBusRomKey, list);
}

} // namespace

void addSharpKeywords(calcu::presets::Vocabulary& v, const SharpKeywordSetup& setup) {
    v.add(calcu::presets::SectionDef{"program", readProgram});

    const bool hostFolders = setup.hostFolders;
    v.add(calcu::presets::HeaderKeyDef{
        kHostDriveKey,
        [hostFolders](const YamlNode& n, int line, ReadContext& ctx) {
            const auto folder = referenceOf(n, line, kHostDriveKey, "folder", ctx);
            if (!folder) return;
            if (!hostFolders) {   // AC-7
                ctx.error(line, "The host drive is not available in this frontend.");
                return;
            }
            const std::string resolved = ctx.source().resolvedName(*folder);
            const std::filesystem::path p(std::u8string(resolved.begin(), resolved.end()));
            if (!p.is_absolute()) {   // EC-5
                ctx.error(line, "The preset " + ctx.source().name() + " was handed over without its folder, so the folder " +
                                    *folder + " it names cannot be used.");
                return;
            }
            std::error_code ec;
            if (!std::filesystem::is_directory(p, ec)) {   // AC-6, row 1600-11
                ctx.error(line, "host-drive: the folder " + resolved + " does not exist.");
                return;
            }
            auto value = std::make_shared<HostDriveValue>();
            value->folder = resolved;
            ctx.keep(kHostDriveKey, value);
        },
        false});

    for (const char* slot : {sharp::kSlot1, sharp::kSlot2}) {
        const std::string key = std::string(slot) + "-rom";
        v.add(calcu::presets::HeaderKeyDef{
            key,
            [key, slot = std::string(slot)](const YamlNode& n, int line, ReadContext& ctx) {
                const auto path = referenceOf(n, line, key.c_str(), "file", ctx);
                if (!path) return;
                auto bytes = ctx.readFile(*path, line);
                if (!bytes) return;
                auto value = std::make_shared<SlotRomValue>();
                value->slot = slot;
                value->path = *path;
                value->bytes = std::move(*bytes);
                value->line = line;
                ctx.keep(key, value);
            },
            false});
    }

    v.add(calcu::presets::HeaderKeyDef{kBusRomKey, readBusRoms, false});

    v.add(calcu::presets::HeaderKeyDef{
        kDebugKey,
        [](const YamlNode& n, int line, ReadContext& ctx) {
            if (auto block = readDebugBlock(n, line, ctx)) ctx.keep(kDebugKey, block);
        },
        false});
}

void checkSharpHeader(const calcu::presets::LoadPlan& plan, ReadContext& ctx) {
    for (const char* slot : {sharp::kSlot1, sharp::kSlot2}) {
        const std::string key = std::string(slot) + "-rom";
        const auto* rom = dynamic_cast<const SlotRomValue*>(ctx.kept(key).get());
        if (!rom) continue;
        const calcu::presets::PlaceRequest* module = nullptr;
        for (const auto& r : plan.places)
            if (r.request.place.steps.size() == 1 && r.request.place.steps[0].value == slot) module = &r;
        if (!module) {
            ctx.error(rom->line, key + ": needs a module in " + slot + ".");
            continue;
        }
        const auto* d = ctx.catalogue().peripheral(module->request.peripheral);
        const auto* def = d ? dynamic_cast<const sharp::ModuleDefinitionData*>(d->productData.get()) : nullptr;
        if (!def) {
            ctx.error(std::max(rom->line, module->line), key + ": the module in " + slot + " is no ROM module.");
            continue;
        }
        MemoryCardDefinition copy = def->definition;
        std::string why;
        if (!replaceRomContent(&copy, rom->bytes, &why))
            ctx.error(std::max(rom->line, module->line), key + ": " + rom->path + ": " + why + ".");
    }
}

void amendSharpPlan(const calcu::presets::LoadPlan& plan, calcu::startup::MachinePlan& out) {
    using calcu::machine::PeripheralId;
    using calcu::machine::PeripheralRequest;
    using calcu::machine::PlaceName;
    using calcu::machine::PlacePath;
    auto kept = [&](const std::string& key) -> const calcu::presets::ProductValue* {
        const auto it = plan.productHeader.find(key);
        return it == plan.productHeader.end() ? nullptr : it->second.get();
    };
    for (const char* slot : {sharp::kSlot1, sharp::kSlot2})
        if (const auto* rom = dynamic_cast<const SlotRomValue*>(kept(std::string(slot) + "-rom")))
            for (PeripheralRequest& r : out.peripherals)
                if (r.place.steps.size() == 1 && r.place.steps[0].value == slot && !r.content)
                    r.content = std::make_shared<sharp::SlotRomRequest>(rom->bytes);
    if (const auto* hd = dynamic_cast<const HostDriveValue*>(kept(kHostDriveKey))) {
        PeripheralRequest r;
        r.place = PlacePath{{PlaceName{sharp::kHostDrivePlace}}};
        r.peripheral = PeripheralId{sharp::kHostDrive};
        r.content = std::make_shared<sharp::HostDriveRequest>(hd->folder);
        out.peripherals.push_back(std::move(r));
    }
    if (const auto* list = dynamic_cast<const BusRomList*>(kept(kBusRomKey)))
        for (const sharp::BusRomRequest& rom : list->roms) {
            PeripheralRequest r;
            r.place = PlacePath{{PlaceName{sharp::kBusRomPlace}}};
            r.peripheral = PeripheralId{sharp::kBusRom};
            r.content = std::make_shared<sharp::BusRomRequest>(rom);
            out.peripherals.push_back(std::move(r));
        }
}

} // namespace shared
