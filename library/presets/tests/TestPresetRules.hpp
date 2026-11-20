#pragma once
#include "TestMachines.hpp"
#include "TestProduct.hpp"
#include "formats/Number.hpp"
#include "harness/DisplayText.hpp"
#include "peripherals/Controls.hpp"
#include "presets/Engine.hpp"
#include "presets/Expect.hpp"
#include "presets/ProductRules.hpp"
#include "presets/Reader.hpp"
#include "presets/Vocabulary.hpp"
#include <map>
#include <memory>
#include <string>
#include <vector>

// The product rules and keywords built for the tests:
//   idle      test-seg: key ready for 2 ms, asked every 0.1 ms (10 ticks);
//             test-dot: key ready for 20 frames, asked every frame.
//   after the build: counted; a test may give a step of its own (a pause).
//   picture   the display output as bytes (no PNG).
//   compare   the text of the harness test product's display rule.
//   markers   sync-clock is for test-dot only; printer-trace for test-seg
//             only; test-undefined is [undefined] everywhere.
// Keywords added as a product adds its own:
//   test-header:   a header key with a block { level: <number>, note: <text>, flag: <bool> }
//   test-memory:   a section: { at: <number>, bytes: | <hex> } or
//                  { at: <number>, file: <path> } into test.memory
//   test-panel:    a frontend step: shown where the rules have a panel
//   printer-trace: on|off: the test printer's `trace` switch; needs the
//                  printer
//   test-undefined: a step the format has not defined yet

namespace calcu::presets::test {

namespace mt = calcu::machine::test;
using formats::YamlNode;
using sessions::Task;

// The display as text by the harness test product's rules.
inline harness::DisplayText testDisplayText(const machine::DisplayOutput& o, const machine::MachineDescriptor& d) {
    static constexpr std::uint8_t kDigits[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
    harness::DisplayText t;
    if (const auto* f = std::get_if<machine::DotMatrixFrame>(&o.frame))
        t = harness::readDotMatrix(*f, harness::test::testFont(), o.indicators, d.display.indicators);
    else {
        const auto& g = std::get<machine::SegmentGeometry>(d.display.geometry);
        harness::SegmentRule rule = [](const machine::SegmentPosition& p, const machine::SegmentLayout&) {
            std::uint8_t bits = 0;
            for (std::size_t s = 0; s < 7 && s < p.lit.size(); ++s)
                if (p.lit[s]) bits = static_cast<std::uint8_t>(bits | (1u << s));
            std::string x;
            for (int i = 0; i < 10; ++i)
                if (bits == kDigits[i]) x = std::string(1, static_cast<char>('0' + i));
            if (bits != 0 && x.empty()) x = "?";
            if (p.lit.size() > 7 && p.lit[7]) x += ".";
            return x;
        };
        t = harness::readSegments(std::get<machine::SegmentFrame>(o.frame), g.layout, rule, false, o.indicators,
                                  d.display.indicators);
    }
    t.on = o.on;
    return t;
}

inline std::vector<std::uint8_t> displayBytes(const machine::DisplayOutput& o) {
    std::vector<std::uint8_t> out{static_cast<std::uint8_t>(o.on ? 1 : 0)};
    if (const auto* f = std::get_if<machine::SegmentFrame>(&o.frame))
        for (const auto& p : f->positions)
            for (bool b : p.lit) out.push_back(b ? 1 : 0);
    if (const auto* f = std::get_if<machine::DotMatrixFrame>(&o.frame)) out.insert(out.end(), f->dots.begin(), f->dots.end());
    return out;
}

struct TestHeader final : ProductValue {
    std::int64_t level = 0;
    std::string note;
    bool flag = false;
};

class TestModelRules final : public ModelRules {
public:
    explicit TestModelRules(bool dot) {
        if (dot) {
            rule.stretch = machine::Seconds(20.0 / 60.0);
            rule.sample = mt::kDotRate / 60;
            marks["printer-trace"] = Applies::NotForThisModel;
        } else {
            rule.stretch = machine::Seconds(0.002);
            rule.sample = 10;
            marks["sync-clock"] = Applies::NotForThisModel;
        }
        marks["test-undefined"] = Applies::Undefined;
    }
    IdleRule idle() const override {
        ++idleAsked;
        return rule;
    }
    Task<ItemResult> afterBuild(RunContext& r) override {
        ++afterBuilds;
        if (after) co_return co_await after(r);
        co_return ItemResult{};
    }
    std::optional<std::vector<std::uint8_t>> screenshot(machine::Machine&, const machine::DisplayOutput& o) override {
        ++pictures;
        return displayBytes(o);
    }
    ExpectReading compare(machine::Machine& m, const machine::DisplayOutput& o, const ExpectText& e) override {
        ExpectReading r;
        const harness::DisplayText t = testDisplayText(o, m.descriptor());
        if (!o.on) {
            r.displayOff = true;
            r.shows = "nothing (the display is off)";
            return r;
        }
        std::string all;
        for (const auto& row : t.rows) {
            all += (all.empty() ? "" : "\n") + row;
            r.shows += (r.shows.empty() ? "" : " | ") + row;
        }
        r.found = foundInLines(all, e);
        return r;
    }
    std::optional<std::string> checkExpect(const ExpectText& e, std::string_view keyword) const override {
        return expectCheck ? expectCheck(e, keyword) : std::nullopt;
    }
    Applies applies(std::string_view k) const override {
        const auto it = marks.find(std::string(k));
        return it == marks.end() ? Applies::Yes : it->second;
    }
    PlaceKeyRule placeKey(const machine::PlaceName& p) const override {
        const auto it = places.find(p.value);
        return it == places.end() ? PlaceKeyRule{} : it->second;
    }

    IdleRule rule;
    std::function<Task<ItemResult>(RunContext&)> after;
    std::map<std::string, Applies> marks;
    std::map<std::string, PlaceKeyRule> places;
    std::function<std::optional<std::string>(const ExpectText&, std::string_view)> expectCheck;   // unset: every text
    mutable int idleAsked = 0;
    int afterBuilds = 0, pictures = 0;
};

// test-memory: written into the machine's test.memory (After::Own).
class TestMemoryItem final : public Item {
public:
    TestMemoryItem(std::int64_t at, std::vector<std::uint8_t> b) : at_(at), bytes_(std::move(b)) {}
    Task<ItemResult> carryOut(RunContext& run) override {
        auto* m = static_cast<mt::TestMachine*>(run.machine());
        if (!m || !m->memory()) co_return ItemResult{false, "There is no test memory.", {}};
        auto& mem = m->memory()->bytes;
        if (static_cast<std::size_t>(at_) + bytes_.size() > mem.size())
            co_return ItemResult{false, "The bytes do not fit the test memory.", {}};
        std::copy(bytes_.begin(), bytes_.end(), mem.begin() + at_);
        co_return ItemResult{};
    }
    After after() const override { return After::Own; }
    bool needsRunningMachine() const override { return true; }
    std::string logText() const override { return "test-memory: " + std::to_string(bytes_.size()) + " bytes"; }

private:
    std::int64_t at_;
    std::vector<std::uint8_t> bytes_;
};

class TestPresetRules final : public ProductRules {
public:
    TestPresetRules() : seg(false), dot(true) {}
    std::string_view productName() const override { return "Calc-U-Test"; }
    ModelRules& forModel(const machine::ModelId& m) override { return m == mt::kDot ? static_cast<ModelRules&>(dot) : seg; }

    void addKeywords(Vocabulary& v) override {
        v.add(HeaderKeyDef{"test-header", [](const YamlNode& n, int line, ReadContext& ctx) {
                               auto h = std::make_shared<TestHeader>();
                               if (!n.isMap()) {
                                   ctx.error(line, "test-header: is a block with level: and note:.");
                                   return;
                               }
                               for (const auto& [k, x] : n.map) {
                                   const int at = x.line ? x.line : line;
                                   std::string s, e;
                                   x.asString(&s, &e);
                                   if (k == "level") {
                                       const auto l = formats::parseNumber(s);
                                       if (!l) ctx.error(at, "level: is a number.");
                                       else h->level = *l;
                                   } else if (k == "note") {
                                       h->note = s;
                                   } else if (k == "flag") {
                                       if (!x.asBool(&h->flag, &e)) ctx.error(at, "flag: is true or false.");
                                   } else {
                                       ctx.error(at, "Unknown key " + k + " in test-header:.");   // every level
                                   }
                               }
                               ctx.keep("test-header", h);
                           }});
        v.add(SectionDef{"test-memory", [](const YamlNode& n, ReadContext& ctx) -> std::unique_ptr<Item> {
                             if (!n.isMap()) {
                                 ctx.error(n.line, "test-memory: is a block with at: and bytes: or file:.");
                                 return nullptr;
                             }
                             std::int64_t at = 0;
                             std::vector<std::uint8_t> bytes;
                             bool ok = true;
                             for (const auto& [k, x] : n.map) {
                                 const int line = x.line ? x.line : n.line;
                                 std::string s, e;
                                 x.asString(&s, &e);
                                 if (k == "at") {
                                     const auto a = formats::parseNumber(s);
                                     if (!a || *a < 0) {
                                         ctx.error(line, "at: is a number.");
                                         ok = false;
                                     } else at = *a;
                                 } else if (k == "bytes") {
                                     std::string hex;
                                     for (char c : x.scalar)
                                         if (c != ' ' && c != '\n') hex += c;
                                     for (std::size_t i = 0; i + 1 < hex.size(); i += 2) {
                                         const auto b = formats::parseNumber("0x" + hex.substr(i, 2));
                                         if (!b) {
                                             ctx.error(line, "bytes: are hex pairs.");
                                             return nullptr;
                                         }
                                         bytes.push_back(static_cast<std::uint8_t>(*b));
                                     }
                                 } else if (k == "file") {
                                     auto b = ctx.readFile(s, line);
                                     if (!b) ok = false;
                                     else bytes = std::move(*b);
                                 } else {
                                     ctx.error(line, "Unknown key " + k + " in test-memory:.");
                                     ok = false;
                                 }
                             }
                             if (!ok) return nullptr;
                             return std::make_unique<TestMemoryItem>(at, std::move(bytes));
                         }});
        struct PanelItem final : Item {
            TestPresetRules* rules;
            std::string name;
            Task<ItemResult> carryOut(RunContext&) override {
                if (rules->panel) rules->shown.push_back(name);   // else passed over
                co_return ItemResult{};
            }
            After after() const override { return After::TakenEffect; }
            bool needsRunningMachine() const override { return false; }
            std::string logText() const override { return "test-panel: " + name; }
        };
        v.add(StepDef{"test-panel", StepDef::Value::Scalar,
                      [this](const YamlNode& n, ReadContext&) -> std::unique_ptr<Item> {
                          auto p = std::make_unique<PanelItem>();
                          p->rules = this;
                          n.asString(&p->name, &p->name);
                          return p;
                      },
                      false, true});
        struct TraceItem final : Item {
            bool on = false;
            Task<ItemResult> carryOut(RunContext& run) override {
                machine::Machine* m = run.machine();
                const machine::PeripheralOwner owner{mt::kPrinter, machine::PlacePath{{machine::PlaceName{"printer"}}}};
                peripherals::Controls* c = m ? peripherals::controlsCapability(*m, owner) : nullptr;
                if (!c) co_return ItemResult{false, "There is no test printer.", {}};
                c->set(machine::ControlId{"trace"}, on);
                co_return ItemResult{};
            }
            After after() const override { return After::TakenEffect; }
            bool needsRunningMachine() const override { return false; }
            std::string logText() const override { return on ? "printer-trace: on" : "printer-trace: off"; }
        };
        v.add(StepDef{"printer-trace", StepDef::Value::Scalar,
                      [](const YamlNode& n, ReadContext& ctx) -> std::unique_ptr<Item> {
                          std::string s, e;
                          n.asString(&s, &e);
                          if (s != "on" && s != "off") {
                              ctx.error(n.line, "printer-trace: takes on or off.");
                              return nullptr;
                          }
                          if (!ctx.plugged(machine::PlaceName{"printer"}, mt::kPrinter)) {
                              ctx.error(n.line, "printer-trace: needs printer: test-printer.");
                              return nullptr;
                          }
                          auto t = std::make_unique<TraceItem>();
                          t->on = s == "on";
                          return t;
                      },
                      true, false});
        struct NothingItem final : Item {
            Task<ItemResult> carryOut(RunContext&) override { co_return ItemResult{}; }
            After after() const override { return After::TakenEffect; }
            bool needsRunningMachine() const override { return false; }
            std::string logText() const override { return "test-undefined"; }
        };
        v.add(StepDef{"test-undefined", StepDef::Value::None,
                      [](const YamlNode&, ReadContext&) -> std::unique_ptr<Item> { return std::make_unique<NothingItem>(); },
                      false, false});
    }

    TestModelRules seg, dot;
    bool panel = false;
    std::vector<std::string> shown;
};

} // namespace calcu::presets::test
