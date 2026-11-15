#include "TestProduct.hpp"
#include "TestKinds.hpp"
#include "TestPresetRules.hpp"
#include "TestDebugParts.hpp"
#include "formats/Flow.hpp"
#include "roms/FullSet.hpp"
#include "sessions/LibraryTopics.hpp"
#include "sessions/Operation.hpp"
#include "settings/Startup.hpp"
#include "typing/TypingPart.hpp"
#include <algorithm>
#include <cstdlib>

namespace calcu::harness::test {

using namespace calcu::machine;
using namespace calcu::sessions;

namespace {
constexpr std::uint8_t kDigits[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};

struct Count final : PayloadOf<Count> {
    static constexpr PayloadKind kKind{"test.count"};
    explicit Count(std::int64_t n) : n(n) {}
    std::int64_t n;
    bool same(const Count& o) const { return n == o.n; }
    sessions::Value toValue() const override { return sessions::Value::object({{"operations", n}}); }
};

const TopicName kCountTopic{"test.count"};
const CommandName kOperation{"test.operation"};
const CommandName kFail{"test.fail"};

Task<OperationResult> runFor(OperationContext& c, Ticks n) {
    co_await c.run(n);
    co_return OperationResult{};
}

// harness Value of a parsed step value.
harness::Value valueOf(const formats::YamlNode& n) {
    switch (n.type) {
    case formats::YamlNode::Type::Null: return {};
    case formats::YamlNode::Type::Scalar: return formats::scalarText(n);
    case formats::YamlNode::Type::Sequence: {
        harness::Value a = harness::Value::array();
        for (const auto& e : n.seq) a.push(valueOf(e));
        return a;
    }
    case formats::YamlNode::Type::Map: break;
    }
    harness::Value o = harness::Value::object();
    for (const auto& [k, v] : n.map) o.set(k, valueOf(v));
    return o;
}
} // namespace

struct TestProduct::Part final : SessionPart {
    StateTopic* count = nullptr;
    std::int64_t operations = 0;
    void attach(SessionSetup& s) override {
        count = &s.stateTopic(kCountTopic);
        count->set(std::make_shared<Count>(0));
        s.command(kOperation, [this](const sessions::Value& a, SessionContext&) -> CommandOutcome {
            const sessions::Value* t = a.find("ticks");
            if (!t || !t->integer() || *t->integer() < 0) return Refused{"The operation needs ticks."};
            const Ticks n = static_cast<Ticks>(*t->integer());
            count->set(std::make_shared<Count>(++operations));
            return StartOperation{[n](OperationContext& c) { return runFor(c, n); }};
        });
        s.command(kFail, [](const sessions::Value& a, SessionContext& c) -> CommandOutcome {
            const sessions::Value* f = a.find("file");
            const std::string file = f && f->string() ? *f->string() : std::string("?");
            c.notice("Cannot read " + file + ".");
            return Refused{"Cannot read " + file + "."};
        });
    }
};

// LIB-5: typing on the test machines.
struct TestProduct::Typing final : typing::TypingProduct, typing::PauseProbe {
    TestProduct& product;
    typing::TypingProfile seg, dot;
    typing::TypingPart part;
    explicit Typing(TestProduct& p) : product(p), seg(profileFor(machine::test::kSegRate, '3', "clr")),
                                      dot(profileFor(machine::test::kDotRate, 'c', "on")),
                                      part(*this, sessions::Hosting::VirtualTime) {
        part.setPauseProbe(this);
    }
    static typing::TypingProfile profileFor(TickRate rate, char last, const char* enter) {
        typing::TypingProfile p;
        p.minHold = typing::ticksOf(0.010, rate);
        p.typed = typing::Cadence{typing::ticksOf(0.010, rate), typing::ticksOf(0.005, rate)};
        p.readinessPoll = std::max<Ticks>(1, rate / 1000);
        const char first = last == 'c' ? 'a' : '0';
        p.charKeys = [first, last](char32_t c, const typing::TypingSupport*) -> std::optional<std::vector<typing::Press>> {
            if (c < static_cast<char32_t>(first) || c > static_cast<char32_t>(last)) return std::nullopt;
            return std::vector<typing::Press>{typing::Press{KeyId{std::string(1, static_cast<char>(c))}}};
        };
        p.enter = KeyId{enter};
        return p;
    }
    const typing::TypingProfile* profile(const ModelId& m) const override {
        return m == machine::test::kDot ? &dot : &seg;
    }
    const typing::KeyboardMap* keyboardMap(const ModelId&) const override { return nullptr; }
    bool typesText() const override { return true; }
    bool paused(SessionContext& c) const override { return product.typingPaused || c.machineSession().paused(); }
};

// LIB-10: the debugger with the test rules; its trace folder and limit are
// the run's (Run.cpp). test-dot stands for a model without the run-control
// steps (the Sharp models, AC-19).
struct TestProduct::Debug {
    debugger::test::TestLevelRules rules;
    debugger::test::TestSymbols symbols;
    debugger::DebuggerPart part;
    Debug() : part(setup()) {}
    debugger::DebuggerSetup setup() {
        debugger::DebuggerSetup d;
        d.rules = &rules;
        d.trace.recorder = [](Machine& m) -> std::unique_ptr<debugger::TraceRecorder> {
            auto* tm = dynamic_cast<machine::test::TestMachine*>(&m);
            return tm && tm->cpu() ? std::make_unique<debugger::test::TestRecorder>(*tm) : nullptr;
        };
        d.symbolReaders = {&symbols};
        d.runControlSteps = [](const ModelId& m) { return m != machine::test::kDot; };
        d.panelViews = [](const ModelId& m) {
            return m == machine::test::kDot ? std::vector<std::string>{} : std::vector<std::string>{"cpu", "log"};
        };
        return d;
    }
};

GlyphFont testFont() {
    GlyphFont f;
    f.cellWidth = 6;
    f.cellHeight = 8;
    f.glyphs = {Glyph{"A", {0x7E, 0x11, 0x11, 0x11, 0x7E, 0x00, 0, 0}},
                Glyph{"B", {0x7F, 0x49, 0x49, 0x49, 0x36, 0x00, 0, 0}},
                Glyph{"C", {0x3E, 0x41, 0x41, 0x41, 0x22, 0x00, 0, 0}}};
    return f;
}

TestProduct::TestProduct()
    : part_(std::make_unique<Part>()), typing_(std::make_unique<Typing>(*this)),
      paper_(std::make_unique<paper::PaperPart>(
          machine::test::paperTestCatalogue(),
          std::vector<paper::PrinterPaper>{{machine::test::kDotter, machine::test::td20Description()},
                                           {machine::test::kPlotter, machine::test::tp4Description()}})),
      debug_(std::make_unique<Debug>()), presetRules_(std::make_unique<presets::test::TestPresetRules>()) {
    roms::test::writeFullSet(roms_, roms::fullSet(catalogue()));
    // LIB-6: the bundled presets of the test product.
    presetFolder_.write("bundle.yaml", "format: example-bundle\nformat-version: 1\n"
                                       "startup:\n  - preset: { file: startup/seg.test-seg }\n"
                                       "groups:\n  - group: Basic\n    examples:\n"
                                       "      - preset: { file: basic/two.test-seg }\n");
    presetFolder_.write("startup/seg.test-seg", "format-version: 2\nmodel: test-seg\npreset-name: Seg\n");
    presetFolder_.write("basic/two.test-seg", "format-version: 2\nmodel: test-seg\npreset-name: Two\n"
                                              "description: Presses 2\nkeys:\n  - key: 2\n");
    bundle_ = std::make_unique<presets::DirectoryPresetBundle>(presetFolder_.path());
}

presets::ProductRules* TestProduct::presetRules() { return presetRules_.get(); }

typing::TypingPart* TestProduct::typingPart() { return &typing_->part; }

TestProduct::~TestProduct() = default;

const settings::ProductDecl& TestProduct::settingsDecl() const {
    // One declaration per product, as SettingsFile keeps a pointer to it; its
    // fallback model is the one the tests start with (LIB-11 AC-2).
    decl_.productName = "Calc-U-Test";
    decl_.models = {"test-seg", "test-dot"};
    decl_.fallbackModel = startModel ? startModel->value : "test-seg";
    decl_.keys = {settings::family::startupModel(), settings::family::startupLastModel(),
                  settings::family::presetFor("test-seg"), settings::family::presetFor("test-dot")};
    return decl_;
}

roms::LookupResult TestProduct::lookUpRoms() const {
    static const roms::FullSet set = roms::fullSet(machine::test::testCatalogue());
    return roms::lookUp(set, roms::DirectoryBundle(roms_.path()), &reader_);
}

std::vector<media::KindDecl> TestProduct::mediaKinds() {
    if (!media) return {};
    builder_.inner.media = true;   // the machines offer their media places
    // The bundled media are always there (LIB-7 AC-28): a disk template.
    mediaFolder_.write("disks/Formatted.testdisk.yaml", "format: test-disk\nformat-version: 2\nname: Formatted\ncontent: \n");
    return media::test::testKinds(media::test::TestKindFolders{mediaFolder_ / "disks", "", ""});
}

BuildResult TestProduct::Builder::build(const BuildRequest& req, const Catalogue& cat) {
    BuildResult r = inner.build(req, cat);
    if (r.machine && req.model == machine::test::kSeg)
        static_cast<machine::test::TestMachine&>(*r.machine).offerState(true);
    if (r.machine) {   // LIB-10
        machine::test::TestProgram p = debugger::test::defaultProgram();
        p.bare = bare;
        static_cast<machine::test::TestMachine&>(*r.machine).enableCpu(p);
    }
    return r;
}

DisplayText TestProduct::readDisplay(const DisplayOutput& o, const MachineDescriptor& d) {
    if (const auto* f = std::get_if<DotMatrixFrame>(&o.frame)) return readDotMatrix(*f, testFont(), o.indicators, d.display.indicators);
    const auto& g = std::get<SegmentGeometry>(d.display.geometry);
    SegmentRule rule = [](const SegmentPosition& p, const SegmentLayout&) {
        std::uint8_t bits = 0;
        for (std::size_t s = 0; s < 7 && s < p.lit.size(); ++s)
            if (p.lit[s]) bits = static_cast<std::uint8_t>(bits | (1u << s));
        std::string t;
        for (int i = 0; i < 10; ++i)
            if (bits == kDigits[i]) t = std::string(1, static_cast<char>('0' + i));
        if (bits != 0 && t.empty()) t = "?";
        if (p.lit.size() > 7 && p.lit[7]) t += ".";
        return t;
    };
    return readSegments(std::get<SegmentFrame>(o.frame), g.layout, rule, false, o.indicators, d.display.indicators);
}

void TestProduct::addCommands(CommandRegistry& r) {
    r.addProduct("test", "mapping", [](CommandContext& c) {
        formats::YamlNode n;
        std::string error;
        if (!formats::parseValue(c.line.argText, &n, &error)) return refusedReply(error);
        harness::Value v = valueOf(n);
        return doneReply(v, {writeJson(v)});
    }, "mapping <value>          echoes the value it was given");
    r.addProduct("test", "operation", [](CommandContext& c) {
        sessions::Value a = sessions::Value::object({{"ticks", static_cast<long long>(std::strtoll(c.line.argText.c_str(), nullptr, 10))}});
        const ReplyMessage m = c.host.command(Command{kOperation, std::move(a)});
        if (!m.done) return refusedReply(m.reason);
        return doneReply();
    }, "operation <ticks>        an operation of that many ticks");
    r.addProduct("test", "fail", [](CommandContext& c) {
        sessions::Value a = sessions::Value::object({{"file", c.host.setup().saveFolder() + "/missing.txt"}});
        const ReplyMessage m = c.host.command(Command{kFail, std::move(a)});
        return m.done ? doneReply() : refusedReply(m.reason);
    }, "fail                     refused, with a notice naming a file in the save folder");
    r.addProduct("test", "count", [](CommandContext& c) {
        auto t = c.host.readTopic("test.count");
        const auto* n = t && t->state ? payloadAs<Count>(t->state) : nullptr;
        const std::int64_t v = n ? n->n : 0;
        return doneReply(harness::Value(static_cast<long long>(v)), {"operations: " + std::to_string(v)});
    }, "count                    the operations run");
}

std::vector<SessionPart*> TestProduct::sessionParts() {
    return {&typing_->part, part_.get(), paper_.get(), &debug_->part};
}

debugger::DebuggerPart* TestProduct::debuggerPart() { return &debug_->part; }

std::unique_ptr<HarnessProduct> makeTestProduct() { return std::make_unique<TestProduct>(); }

} // namespace calcu::harness::test
