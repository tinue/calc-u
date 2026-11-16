#include "TIPresetKeys.hpp"
#include "Bcd.hpp"
#include "CueCardYaml.hpp"
#include "../core/PC100C.hpp"
#include "../core/TICatalogue.hpp"
#include "formats/Flow.hpp"
#include "peripherals/Controls.hpp"
#include "presets/Engine.hpp"
#include "presets/Reader.hpp"
#include "sessions/LibraryTopics.hpp"
#include <map>
#include <memory>

namespace cu59 {

using calcu::formats::YamlNode;
using calcu::presets::After;
using calcu::presets::Item;
using calcu::presets::ItemResult;
using calcu::presets::ReadContext;
using calcu::presets::RunContext;
using calcu::sessions::Task;

bool isPresetFileName(std::string_view name) {
    const std::size_t dot = name.rfind('.');
    if (dot == std::string_view::npos) return false;
    const std::string ext = calcu::presets::lowerAscii(name.substr(dot + 1));
    return ext == "ti59" || ext == "ti58" || ext == "ti58c";
}

int lastStepOf(const calcu::machine::ModelId& m) { return m.value == "TI-59" ? 959 : 479; }
int dataRegistersOf(const calcu::machine::ModelId& m) { return m.value == "TI-59" ? 100 : 60; }
bool hasExtraRegisters(const calcu::machine::ModelId& m) { return m.value == "TI-58C"; }

ProgramMemory* programMemoryOf(calcu::machine::Machine& m) {
    for (const auto& e : m.capabilities(ProgramMemory::kKind))
        if (auto* k = calcu::machine::capabilityAs<ProgramMemory>(m, e.id)) return k;
    return nullptr;
}

ConstantMemory* constantMemoryOf(calcu::machine::Machine& m) {
    for (const auto& e : m.capabilities(ConstantMemory::kKind))
        if (auto* k = calcu::machine::capabilityAs<ConstantMemory>(m, e.id)) return k;
    return nullptr;
}

std::variant<int, std::string> partitionSteps(std::string_view value, int lastStep) {
    std::string_view v = value;
    while (!v.empty() && v.front() == ' ') v.remove_prefix(1);
    while (!v.empty() && v.back() == ' ') v.remove_suffix(1);
    const std::size_t dot = v.find('.');
    const std::string_view whole = v.substr(0, dot);
    const std::string_view rest = dot == std::string_view::npos ? std::string_view() : v.substr(dot + 1);
    auto digits = [](std::string_view s) {
        if (s.empty() || s.size() > 6) return false;
        for (char c : s)
            if (c < '0' || c > '9') return false;
        return true;
    };
    if (!digits(whole) || (dot != std::string_view::npos && !rest.empty() && !digits(rest)))
        return "partition: is the number of the last program step (479, or 479.59).";
    const int last = std::stoi(std::string(whole));
    if (last > lastStep)
        return "partition: " + std::string(whole) + " is beyond the last program step of this model, " +
               std::to_string(lastStep) + ".";
    const int steps = std::min(960, (last + 1 + 79) / 80 * 80);
    return steps;
}

std::variant<DataKey, std::string> dataKey(std::string_view key, const calcu::machine::ModelId& model) {
    DataKey k;
    std::string_view s = key;
    if (!s.empty() && (s[0] == 'H' || s[0] == 'h')) {
        k.extra = true;
        s.remove_prefix(1);
    }
    if (s.size() != 2 || s[0] < '0' || s[0] > '9' || s[1] < '0' || s[1] > '9')
        return "data: " + std::string(key) + " is no register; a register is NN (00 to 99)" +
               (hasExtraRegisters(model) ? std::string(" or H00 to H03") : std::string()) + ".";
    k.index = (s[0] - '0') * 10 + (s[1] - '0');
    if (k.extra) {
        if (!hasExtraRegisters(model)) return "data: " + std::string(key) + ": only the TI-58C has the registers H00 to H03.";
        if (k.index > 3) return "data: " + std::string(key) + ": the TI-58C's extra registers are H00 to H03.";
    } else if (k.index >= dataRegistersOf(model)) {
        return "data: " + std::string(key) + ": the " + model.value + " has the registers 00 to " +
               std::to_string(dataRegistersOf(model) - 1) + ".";
    }
    return k;
}

std::string registerText(const RegisterNibbles& n) {
    std::string mant;
    for (int i = 15; i >= 3; --i) mant += static_cast<char>('0' + (n[static_cast<std::size_t>(i)] % 10));
    if (mant.find_first_not_of('0') == std::string::npos) return "0";
    while (mant.size() > 1 && mant.back() == '0') mant.pop_back();
    const int exp = n[2] * 10 + n[1];
    std::string out = (n[0] & 2) ? "-" : "";
    out += mant.substr(0, 1);
    if (mant.size() > 1) out += "." + mant.substr(1);
    out += "e" + std::string((n[0] & 4) ? "-" : "") + std::to_string(exp);
    return out;
}

namespace {

int lineOf(const YamlNode& n, int fallback) { return n.line ? n.line : fallback; }

ItemResult noMemory() { return ItemResult{false, "The machine has no program memory.", {}}; }

// ── partition: (AC-2) ───────────────────────────────────────────────────
class PartitionItem final : public Item {
public:
    int steps = 0;
    Task<ItemResult> carryOut(RunContext& run) override {
        calcu::machine::Machine* m = run.machine();
        ProgramMemory* pm = m ? programMemoryOf(*m) : nullptr;
        if (!pm) co_return noMemory();
        pm->setPartition(steps);
        co_return ItemResult{};
    }
    After after() const override { return After::Own; }
    bool needsRunningMachine() const override { return true; }
    std::string logText() const override { return "partition: " + std::to_string(steps - 1); }
};

std::unique_ptr<Item> readPartition(const YamlNode& n, ReadContext& ctx) {
    if (!n.isScalar()) {
        ctx.error(n.line, "partition: is a value on the same line (partition: 479).");
        return nullptr;
    }
    auto r = partitionSteps(calcu::formats::scalarText(n), lastStepOf(ctx.model().model));
    if (auto* why = std::get_if<std::string>(&r)) {
        ctx.error(n.line, *why);
        return nullptr;
    }
    auto item = std::make_unique<PartitionItem>();
    item->steps = std::get<int>(r);
    return item;
}

// ── program: (AC-1, AC-3, EC-1) ─────────────────────────────────────────
class ProgramItem final : public Item {
public:
    std::vector<ProgramStep> steps;
    std::string what;
    Task<ItemResult> carryOut(RunContext& run) override {
        calcu::machine::Machine* m = run.machine();
        ProgramMemory* pm = m ? programMemoryOf(*m) : nullptr;
        if (!pm) co_return noMemory();
        for (const ProgramStep& s : steps) pm->writeStep(s.step, s.keycode);
        co_return ItemResult{};
    }
    After after() const override { return After::Own; }
    bool needsRunningMachine() const override { return true; }
    std::string logText() const override { return "program: " + what; }
};

std::unique_ptr<Item> readProgram(const YamlNode& n, ReadContext& ctx) {
    const int at = n.line;
    if (!n.isMap() || n.map.empty()) {
        ctx.error(at, "program: is a block with keycodes: | or file:.");
        return nullptr;
    }
    for (const auto& [k, v] : n.map)
        if (k != "keycodes" && k != "file") {
            ctx.error(lineOf(v, at), "program: has no key " + k + "; it takes keycodes: | or file:.");
            return nullptr;
        }
    const YamlNode* codes = n.find("keycodes");
    const YamlNode* file = n.find("file");
    if (!codes == !file || n.map.size() != 1) {
        ctx.error(at, "program: takes exactly one of keycodes: | and file:.");
        return nullptr;
    }
    const int lastStep = lastStepOf(ctx.model().model);
    auto item = std::make_unique<ProgramItem>();
    if (codes) {
        if (!codes->isScalar()) {
            ctx.error(lineOf(*codes, at), "program: keycodes: is a block (keycodes: |).");
            return nullptr;
        }
        auto r = readKeycodes(codes->scalar, lastStep);
        if (auto* e = std::get_if<KeycodeError>(&r)) {
            const std::optional<std::string> text = ctx.source().text();
            int line = codes->line;
            if (text) {
                const auto lines = blockLineNumbers(*text, codes->line, static_cast<std::size_t>(e->lineIndex) + 1);
                if (static_cast<std::size_t>(e->lineIndex) < lines.size()) line = lines[static_cast<std::size_t>(e->lineIndex)];
            }
            ctx.error(line, "program: " + e->message);
            return nullptr;
        }
        item->steps = std::get<std::vector<ProgramStep>>(std::move(r));
        item->what = "keycodes";
        return item;
    }
    const int fileLine = lineOf(*file, at);
    const std::string path = file->isScalar() ? calcu::formats::scalarText(*file) : std::string();
    if (path.empty()) {
        ctx.error(fileLine, "program: file: needs a path.");
        return nullptr;
    }
    auto bytes = ctx.readFile(path, fileLine);
    if (!bytes) return nullptr;
    auto r = readKeystrokeProgramFile(std::string(bytes->begin(), bytes->end()), lastStep);
    if (auto* why = std::get_if<std::string>(&r)) {
        ctx.error(fileLine, "program: " + path + ": " + *why);
        return nullptr;
    }
    item->steps = std::get<KeystrokeProgram>(r).steps;
    item->what = path;
    return item;
}

// ── data: (AC-4, EC-2, EC-3) ────────────────────────────────────────────
class DataItem final : public Item {
public:
    struct Entry {
        DataKey key;
        RegisterNibbles value;
    };
    std::vector<Entry> entries;
    Task<ItemResult> carryOut(RunContext& run) override {
        calcu::machine::Machine* m = run.machine();
        ProgramMemory* pm = m ? programMemoryOf(*m) : nullptr;
        if (!pm) co_return noMemory();
        for (const Entry& e : entries) {
            if (e.key.extra) pm->writeExtra(e.key.index, e.value);
            else pm->writeRegister(e.key.index, e.value);
        }
        co_return ItemResult{};
    }
    After after() const override { return After::Own; }
    bool needsRunningMachine() const override { return true; }
    std::string logText() const override { return "data: " + std::to_string(entries.size()) + " registers"; }
};

std::unique_ptr<Item> readData(const YamlNode& n, ReadContext& ctx) {
    const int at = n.line;
    if (!n.isMap() || n.map.empty()) {
        ctx.error(at, "data: is a mapping of registers, NN: value.");
        return nullptr;
    }
    auto item = std::make_unique<DataItem>();
    std::map<std::pair<bool, int>, int> seen;
    bool ok = true;
    for (const auto& [k, v] : n.map) {
        const int line = lineOf(v, at);
        auto key = dataKey(k, ctx.model().model);
        if (auto* why = std::get_if<std::string>(&key)) {
            ctx.error(line, *why);
            ok = false;
            continue;
        }
        const DataKey dk = std::get<DataKey>(key);
        if (!seen.emplace(std::make_pair(dk.extra, dk.index), line).second) {   // D-6
            ctx.error(line, "data: " + k + " is given twice.");
            ok = false;
            continue;
        }
        const auto value = v.isScalar() ? parseDecimal(calcu::formats::scalarText(v)) : std::nullopt;
        if (!value) {
            ctx.error(line, "data: " + k + ": the value is no number (a decimal number, an exponent allowed: -1.5e-3).");
            ok = false;
            continue;
        }
        item->entries.push_back({dk, *value});
    }
    if (!ok) return nullptr;
    return item;
}

// ── cue-card: (AC-5, AC-6, EC-4, EC-5) ──────────────────────────────────
class CueCardItem final : public Item {
public:
    CueCardContent card;
    std::function<void(calcu::sessions::SessionContext&, const CueCardContent&)> fit;
    Task<ItemResult> carryOut(RunContext& run) override {
        if (fit) fit(run.session(), card);
        co_return ItemResult{};
    }
    After after() const override { return After::TakenEffect; }
    bool needsRunningMachine() const override { return false; }
    std::string logText() const override { return "cue-card: " + card.title; }
};

// ── printer-trace: (LIB-6 AC-40) ────────────────────────────────────────
class PrinterTraceItem final : public Item {
public:
    bool on = false;
    Task<ItemResult> carryOut(RunContext& run) override {
        calcu::machine::Machine* m = run.machine();
        auto* c = m ? calcu::machine::capabilityAs<calcu::peripherals::Controls>(
                          *m, calcu::machine::CapabilityId{"printer/controls"})
                    : nullptr;
        if (!c) co_return ItemResult{false, "printer-trace: needs the PC-100C.", {}};
        c->set(PC100C::kTrace, on);
        co_return ItemResult{};
    }
    After after() const override { return After::TakenEffect; }
    bool needsRunningMachine() const override { return false; }
    std::string logText() const override { return std::string("printer-trace: ") + (on ? "on" : "off"); }
};

} // namespace

void addTIKeywords(calcu::presets::Vocabulary& v, const TIKeywordSetup& setup) {
    v.add(calcu::presets::SectionDef{"partition", readPartition});
    v.add(calcu::presets::SectionDef{"program", readProgram});
    v.add(calcu::presets::SectionDef{"data", readData});
    auto fit = setup.fitCueCard;
    v.add(calcu::presets::SectionDef{
        "cue-card", [fit](const YamlNode& n, ReadContext& ctx) -> std::unique_ptr<Item> {
            auto card = readStandAloneCueCard(n, [&ctx](int line, const std::string& m) { ctx.error(line, m); });
            if (!card) return nullptr;
            auto item = std::make_unique<CueCardItem>();
            item->card = *card;
            item->fit = fit;
            return item;
        }});
    v.add(calcu::presets::StepDef{
        "printer-trace", calcu::presets::StepDef::Value::Scalar,
        [](const YamlNode& n, ReadContext& ctx) -> std::unique_ptr<Item> {
            const std::string t = calcu::presets::lowerAscii(calcu::formats::scalarText(n));
            if (t != "on" && t != "off") {
                ctx.error(n.line, "printer-trace: is on or off.");
                return nullptr;
            }
            if (!ctx.plugged(kTIPrinterPlace, kPC100C)) {
                ctx.error(n.line, "printer-trace: needs printer: PC-100C.");
                return nullptr;
            }
            auto item = std::make_unique<PrinterTraceItem>();
            item->on = t == "on";
            return item;
        },
        true, false});
}

} // namespace cu59
