#include "TestMachines.hpp"
#include <algorithm>
#include <cstring>
#include <variant>

namespace calcu::machine::test {

namespace {
const std::string kMd5 = "0123456789abcdef0123456789abcdef";   // test data, no real dump

RomEntry rom(const char* name, std::uint64_t amount, RomEncoding enc = RomEncoding::Binary,
             ContentUnit unit = ContentUnit::Bytes) {
    return RomEntry{name, enc, amount, unit, kMd5};
}

std::vector<KeyDescriptor> keys(std::initializer_list<const char*> ids) {
    std::vector<KeyDescriptor> out;
    for (const char* id : ids) out.push_back(KeyDescriptor{KeyId{id}, {}});
    return out;
}

MachineDescriptor segModel() {
    MachineDescriptor d;
    d.model = kSeg;
    d.family = ModelFamilyId{"test-family"};
    d.roms = {rom("test-seg.rom", 2048)};
    d.tickRate = kSegRate;
    SegmentLayout layout;
    for (const char* s : {"A", "B", "C", "D", "E", "F", "G"}) layout.segments.push_back({s, false});
    layout.segments.push_back({"DP", true});
    d.display = DisplayDescriptor{SegmentGeometry{3, layout, true}, {IndicatorDef{"C", true}}};
    d.hasSound = false;
    d.keys = keys({"0", "1", "2", "3", "clr"});
    d.keys.back().otherSpellings = {"CLR", "clear"};
    d.connectors = {
        ConnectorDescriptor{ConnectorId{"slot"}, "8-contact edge",
                            {PinOutDescriptor{PinOutId{"test.slot"}, PinOutLevel::Functional, Takes::One,
                                              AfterChange::PowerCycle}}},
        ConnectorDescriptor{ConnectorId{"bus"}, "11-pad",
                            {PinOutDescriptor{PinOutId{"test.bus"}, PinOutLevel::Functional, Takes::Chain,
                                              AfterChange::Nothing}}},
    };
    d.places = {
        PlaceDescriptor{PlaceName{"slot-1"}, ConnectorId{"slot"}, PinOutId{"test.slot"}, 1},
        PlaceDescriptor{PlaceName{"printer"}, ConnectorId{"bus"}, PinOutId{"test.bus"}, 1},
        PlaceDescriptor{PlaceName{"aux"}, ConnectorId{"bus"}, PinOutId{"test.bus"}, 1},
    };
    d.components = {
        ComponentDescriptor{ComponentId{"cpu"}, "test CPU", Depth::Separate, std::nullopt},
        ComponentDescriptor{ComponentId{"rom"}, "test ROM", Depth::Folded, ComponentId{"cpu"}},
        ComponentDescriptor{ComponentId{"slot"}, "module slot", Depth::Functional, std::nullopt},
    };
    d.mediaPlaces = {MediaPlaceDescriptor{MediaPlaceId{"reader"}, "test-card", MediaPassage::PassThrough, {0, 1}}};
    d.debugTargets = {DebugTargetId{"test-cpu"}};
    return d;
}

MachineDescriptor dotModel() {
    MachineDescriptor d;
    d.model = kDot;
    d.family = ModelFamilyId{"test-dot-family"};
    d.variants = {
        VariantDescriptor{VariantId{"V1"}, {rom("test-dot-v1.rom", 4096)}},
        VariantDescriptor{VariantId{"V2"},
                          {rom("test-dot-v2.rom", 4096), rom("test-dot-font.txt", 64, RomEncoding::Text, ContentUnit::Rows)}},
    };
    d.defaultVariant = VariantId{"V1"};
    d.tickRate = kDotRate;
    d.display = DisplayDescriptor{DotMatrixGeometry{12, 8, CellGrouping{6, 8, 2, 1}},
                                  {IndicatorDef{"RUN", false}, IndicatorDef{"BUSY", false}}};
    d.hasSound = true;
    d.keys = keys({"a", "b", "c", "off", "on"});
    d.components = {
        ComponentDescriptor{ComponentId{"main"}, "main CPU", Depth::Separate, std::nullopt},
        ComponentDescriptor{ComponentId{"inner"}, "inner block on its own clock", Depth::Separate, std::nullopt},
    };
    d.mediaPlaces = {MediaPlaceDescriptor{MediaPlaceId{"drive"}, "test-disk", MediaPassage::Holding, {0}}};
    d.debugTargets = {DebugTargetId{"main"}, DebugTargetId{"inner"}};
    return d;
}

std::vector<PeripheralDescriptor> peripherals() {
    PeripheralDescriptor printer;
    printer.id = kPrinter;
    printer.name = "Test printer";
    printer.fits = {PinOutId{"test.bus"}};
    printer.controls = {ControlDescriptor{ControlId{"online"}, ControlForm::Switch}};
    printer.components = {ComponentDescriptor{ComponentId{"mech"}, "printer mechanism", Depth::Functional, std::nullopt}};

    PeripheralDescriptor module;
    module.id = kModule;
    module.name = "Test module";
    module.variants = {VariantDescriptor{VariantId{"4k"}, {rom("test-module-4k.rom", 4096)}},
                       VariantDescriptor{VariantId{"8k"}, {rom("test-module-8k.rom", 8192)}}};
    module.defaultVariant = VariantId{"4k"};
    module.fits = {PinOutId{"test.slot"}, PinOutId{"test.bus"}};
    module.components = {ComponentDescriptor{ComponentId{"rom"}, "module ROM", Depth::Separate, std::nullopt}};
    module.productData = std::make_shared<TestModuleDefinition>("name: test-module\n");
    return {printer, module};
}

std::vector<RomImage> imagesFor(const std::vector<RomEntry>& entries) {
    std::vector<RomImage> out;
    for (const auto& e : entries) out.push_back(RomImage{e.fileName, {0x01, 0x02, 0x03}});
    return out;
}

// Seven-segment patterns of 0-9, bits A (0) to G (6).
constexpr std::uint8_t kDigits[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};

constexpr std::int16_t kLoud = 8'000;
constexpr std::uint64_t kToneHz = 1'000;
} // namespace

Catalogue makeTestCatalogue() { return Catalogue({segModel(), dotModel()}, peripherals()); }

const Catalogue& testCatalogue() {
    static const Catalogue c = makeTestCatalogue();
    return c;
}

BuildRequest requestFor(const Catalogue& cat, const ModelId& model, std::optional<VariantId> variant,
                        std::vector<PeripheralRequest> peripherals, int soundRate) {
    BuildRequest r;
    r.model = model;
    r.variant = variant;
    if (const MachineDescriptor* d = cat.model(model)) r.roms = imagesFor(d->romsFor(variant));
    r.peripherals = std::move(peripherals);
    r.soundRate = soundRate;
    return r;
}

PeripheralRequest peripheralAt(const Catalogue& cat, const std::string& place, const PeripheralId& id,
                               std::optional<VariantId> variant) {
    PeripheralRequest r;
    r.place = PlacePath{{PlaceName{place}}};
    r.peripheral = id;
    r.variant = variant;
    if (const PeripheralDescriptor* d = cat.peripheral(id)) {
        r.roms = imagesFor(d->romsFor(variant));
        r.content = d->productData;
    }
    return r;
}

BuildResult TestBuilder::build(const BuildRequest& req, const Catalogue& cat) {
    const MachineDescriptor* d = cat.model(req.model);
    if (!d) return BuildResult{nullptr, {}, "unknown model"};
    auto m = std::make_unique<TestMachine>(*d, req.variant, req.soundRate);
    if (auto t = std::dynamic_pointer_cast<const TestSegTiming>(req.content); t && req.model == kSeg)
        m->setTiming(std::move(t));
    std::vector<PlaceName> taken;
    for (const auto& p : req.peripherals) {
        if (p.place.steps.size() != 1) return BuildResult{nullptr, {}, "no such place"};
        const auto info = cat.place(req.model, p.place.steps[0]);
        if (!info) return BuildResult{nullptr, {}, "no such place"};
        if (std::find(info->fitting.begin(), info->fitting.end(), p.peripheral) == info->fitting.end())
            return BuildResult{nullptr, {}, "does not fit"};
        if (std::count(taken.begin(), taken.end(), p.place.steps[0]) >= info->place->takesAtOnce)
            return BuildResult{nullptr, {}, "place taken"};
        taken.push_back(p.place.steps[0]);
        m->plug(PlugEntry{p.place, p.peripheral, p.variant});
    }
    return BuildResult{std::move(m), {}, {}};
}

std::unique_ptr<TestMachine> buildTestMachine(const ModelId& model, std::optional<VariantId> variant,
                                              std::vector<PeripheralRequest> peripherals, int soundRate) {
    TestBuilder builder;
    const Catalogue& cat = testCatalogue();
    BuildResult r = build(cat, builder, requestFor(cat, model, variant, std::move(peripherals), soundRate));
    return std::unique_ptr<TestMachine>(static_cast<TestMachine*>(r.machine.release()));
}

std::unique_ptr<TestMachine> buildTimedTestSeg(Ticks instructionLength, std::uint64_t loopLength) {
    TestBuilder builder;
    const Catalogue& cat = testCatalogue();
    BuildRequest req = requestFor(cat, kSeg);
    req.content = std::make_shared<TestSegTiming>(instructionLength, loopLength);
    BuildResult r = build(cat, builder, req);
    return std::unique_ptr<TestMachine>(static_cast<TestMachine*>(r.machine.release()));
}

Ticks runUntilKeyReady(Machine& m) {
    Ticks ran = 0;
    while (!m.isKeyReady()) ran += m.advance(1);
    return ran;
}

bool sameDisplay(const DisplayOutput& a, const DisplayOutput& b) {
    if (a.on != b.on || a.indicators.size() != b.indicators.size() || a.frame.index() != b.frame.index()) return false;
    for (std::size_t i = 0; i < a.indicators.size(); ++i)
        if (a.indicators[i].name != b.indicators[i].name || a.indicators[i].on != b.indicators[i].on ||
            a.indicators[i].brightness != b.indicators[i].brightness)
            return false;
    if (const auto* sa = std::get_if<SegmentFrame>(&a.frame)) {
        const auto& sb = std::get<SegmentFrame>(b.frame);
        if (sa->positions.size() != sb.positions.size()) return false;
        for (std::size_t i = 0; i < sa->positions.size(); ++i)
            if (sa->positions[i].lit != sb.positions[i].lit || sa->positions[i].brightness != sb.positions[i].brightness ||
                sa->positions[i].segmentBrightness != sb.positions[i].segmentBrightness)
                return false;
        return true;
    }
    const auto& da = std::get<DotMatrixFrame>(a.frame);
    const auto& db = std::get<DotMatrixFrame>(b.frame);
    return da.width == db.width && da.height == db.height && da.dots == db.dots;
}

// --- TestMachine -------------------------------------------------------------

TestMachine::TestMachine(const MachineDescriptor& d, std::optional<VariantId> v, int soundRate)
    : desc_(d), variant_(std::move(v)), soundRate_(soundRate) {}

bool TestMachine::isDot() const { return std::holds_alternative<DotMatrixGeometry>(desc_.display.geometry); }

bool TestMachine::hasPrinter() const {
    return std::any_of(plugged_.begin(), plugged_.end(), [](const PlugEntry& e) { return e.peripheral == kPrinter; });
}

Ticks TestMachine::advance(Ticks atLeast) {
    const bool printer = hasPrinter();
    Ticks ran = 0;
    while (ran < atLeast) {
        const Ticks len = isDot() ? 4 : timing_ ? timing_->instructionLength : (generic_.instructions % 2 == 0 ? 3 : 5);
        ran += len;
        ++generic_.instructions;
        if (printer) ++printerCounter_.count;
        if (isDot()) {   // the inner block: 3 of its cycles per 8 ticks
            innerRest_ += 3 * len;
            innerCycles_ += innerRest_ / 8;
            innerRest_ %= 8;
        }
    }
    ticks_ += ran;
    sinceStart_ += ran;
    if (desc_.hasSound && soundRate_ > 0) {
        // Sample n stands at tick n * rate / soundRate; its value is a square
        // wave of that tick alone, so the slicing of the advances cannot show.
        const std::uint64_t rate = desc_.tickRate, sr = static_cast<std::uint64_t>(soundRate_);
        for (; samplesMade_ * rate / sr < ticks_; ++samplesMade_) {
            const std::uint64_t at = samplesMade_ * rate / sr;
            sound_.push_back((at * 2 * kToneHz / rate) % 2 ? kLoud : static_cast<std::int16_t>(-kLoud));
        }
    }
    return ran;
}

void TestMachine::reset() { startUp(); }

void TestMachine::powerCycle() {
    displayOn_ = true;
    startUp();
}

void TestMachine::coldBoot() {
    presses_ = 0;
    displayOn_ = true;
    startUp();
}

KeyCall TestMachine::keyDown(const KeyId& k) {
    if (!desc_.hasKey(k)) return KeyCall::UnknownKey;
    if (std::find(keysDown_.begin(), keysDown_.end(), k) != keysDown_.end()) return KeyCall::Done;
    keysDown_.push_back(k);
    events_.push_back(KeyEvent{k, true, ticks_});
    ++presses_;
    if (k == KeyId{"off"}) displayOn_ = false;
    if (k == KeyId{"on"}) displayOn_ = true;
    return KeyCall::Done;
}

KeyCall TestMachine::keyUp(const KeyId& k) {
    if (!desc_.hasKey(k)) return KeyCall::UnknownKey;
    auto it = std::find(keysDown_.begin(), keysDown_.end(), k);
    if (it == keysDown_.end()) return KeyCall::Done;
    keysDown_.erase(it);
    events_.push_back(KeyEvent{k, false, ticks_});
    return KeyCall::Done;
}

DisplayOutput TestMachine::readDisplay() {
    DisplayOutput out;
    const bool anyDown = !keysDown_.empty();
    if (isDot()) {
        const auto& g = std::get<DotMatrixGeometry>(desc_.display.geometry);
        DotMatrixFrame f{g.width, g.height, std::vector<std::uint8_t>(std::size_t(g.width * g.height), 0)};
        out.on = displayOn_;
        if (displayOn_) {
            f.dots[(ticks_ / 16) % std::uint64_t(g.width)] = 1;                       // row 0: moves with the ticks
            f.dots[3 * g.width + int(presses_ % std::uint64_t(g.width))] = 1;         // row 3: the presses
            for (int i = 0; i < int(keysDown_.size()) && i < g.width; ++i) f.dots[7 * g.width + i] = 1;   // row 7: keys down
        }
        out.frame = std::move(f);
        out.indicators = {IndicatorOutput{desc_.display.indicators[0].name, displayOn_ && !isKeyReady(), std::nullopt},
                          IndicatorOutput{desc_.display.indicators[1].name, displayOn_ && anyDown, std::nullopt}};
        return out;
    }
    const auto& g = std::get<SegmentGeometry>(desc_.display.geometry);
    const std::size_t n = g.layout.segments.size();
    const std::uint64_t digits[3] = {presses_ % 10, (ticks_ / 1000) % 10, keysDown_.size() % 10};
    SegmentFrame f;
    for (int pos = 0; pos < g.positions; ++pos) {
        SegmentPosition p{std::vector<bool>(n, false), 0.0f, std::vector<std::optional<float>>(n)};
        for (std::size_t s = 0; s < 7; ++s) p.lit[s] = (kDigits[digits[pos]] >> s) & 1;
        p.brightness = 1.0f;
        const bool dp = pos == 0 && anyDown;
        p.lit[7] = dp;
        p.segmentBrightness[7] = dp ? 0.5f : 0.0f;
        f.positions.push_back(std::move(p));
    }
    out.frame = std::move(f);
    const bool c = isKeyReady();
    out.indicators = {IndicatorOutput{desc_.display.indicators[0].name, c, c ? 1.0f : 0.0f}};
    return out;
}

std::size_t TestMachine::takeSound(std::span<std::int16_t> out) {
    const std::size_t n = std::min(out.size(), sound_.size());
    std::copy_n(sound_.begin(), n, out.begin());
    sound_.erase(sound_.begin(), sound_.begin() + std::ptrdiff_t(n));
    return n;
}

Plugged TestMachine::plugged() const {
    Plugged p;
    p.byPlace = plugged_;
    for (const auto& c : desc_.connectors) {
        ConnectorPlugs cp{PlacePath{}, c.id, {}};
        for (const auto& e : plugged_)
            if (const PlaceDescriptor* pl = desc_.place(e.place.steps[0]); pl && pl->connector == c.id)
                cp.plugged.push_back(e);
        p.byConnector.push_back(std::move(cp));
    }
    return p;
}

std::vector<CapabilityEntry> TestMachine::capabilities() const {
    const ComponentId cpu{isDot() ? "main" : "cpu"};
    std::vector<CapabilityEntry> out{
        CapabilityEntry{TestGeneric::kKind, CapabilityId{cpu.value + "/" + std::string(TestGeneric::kKind.value)},
                        ComponentOwner{cpu}}};
    for (const auto& e : plugged_)
        if (e.peripheral == kPrinter)
            out.push_back(CapabilityEntry{
                TestCounter::kKind, CapabilityId{e.place.steps[0].value + "/" + std::string(TestCounter::kKind.value)},
                PeripheralOwner{kPrinter, e.place}});
    return out;
}

Capability* TestMachine::capability(const CapabilityId& id) {
    for (const auto& e : capabilities())
        if (e.id == id) return e.kind == TestCounter::kKind ? static_cast<Capability*>(&printerCounter_) : &generic_;
    return nullptr;
}

} // namespace calcu::machine::test
