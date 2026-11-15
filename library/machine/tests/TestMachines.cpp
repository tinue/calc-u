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
    d.connectors = {
        ConnectorDescriptor{ConnectorId{"chain"}, "test 60-pin",
                            {PinOutDescriptor{PinOutId{"test.chain"}, PinOutLevel::ContactLevel, Takes::Chain,
                                              AfterChange::PowerCycle}}},
        ConnectorDescriptor{ConnectorId{"card"}, "two-slot card edge",
                            {PinOutDescriptor{PinOutId{"test.card"}, PinOutLevel::Functional, Takes::One,
                                              AfterChange::PowerCycle}}},
    };
    d.places = {
        PlaceDescriptor{PlaceName{"a"}, ConnectorId{"chain"}, PinOutId{"test.chain"}, 1},
        PlaceDescriptor{PlaceName{"b"}, ConnectorId{"chain"}, PinOutId{"test.chain"}, 1},
        PlaceDescriptor{PlaceName{"card-1"}, ConnectorId{"card"}, PinOutId{"test.card"}, 1},
        PlaceDescriptor{PlaceName{"card-2"}, ConnectorId{"card"}, PinOutId{"test.card"}, 1},
    };
    d.components = {
        ComponentDescriptor{ComponentId{"main"}, "main CPU", Depth::Separate, std::nullopt},
        ComponentDescriptor{ComponentId{"inner"}, "inner block on its own clock", Depth::Separate, std::nullopt},
        ComponentDescriptor{ComponentId{"rtc"}, "real-time clock", Depth::Functional, std::nullopt},
    };
    d.mediaPlaces = {MediaPlaceDescriptor{MediaPlaceId{"drive"}, "test-disk", MediaPassage::Holding, {0}}};
    d.debugTargets = {DebugTargetId{"main"}, DebugTargetId{"inner"}};
    return d;
}

std::vector<PeripheralDescriptor> peripherals() {
    PeripheralDescriptor printer;
    printer.id = kPrinter;
    printer.name = "Test printer";
    printer.fits = {PinOutId{"test.bus"}, PinOutId{"test.chain"}, PinOutId{"test.jack"}};
    printer.controls = {ControlDescriptor{ControlId{"online"}, ControlForm::Switch},
                        ControlDescriptor{ControlId{"feed"}, ControlForm::Key},
                        ControlDescriptor{ControlId{"trace"}, ControlForm::Switch}};
    printer.components = {ComponentDescriptor{ComponentId{"mech"}, "printer mechanism", Depth::Functional, std::nullopt}};

    PeripheralDescriptor module;
    module.id = kModule;
    module.name = "Test module";
    module.variants = {VariantDescriptor{VariantId{"4k"}, {rom("test-module-4k.rom", 4096)}},
                       VariantDescriptor{VariantId{"8k"}, {rom("test-module-8k.rom", 8192)}}};
    module.defaultVariant = VariantId{"4k"};
    module.fits = {PinOutId{"test.slot"}, PinOutId{"test.bus"}, PinOutId{"test.card"}};
    module.components = {ComponentDescriptor{ComponentId{"rom"}, "module ROM", Depth::Separate, std::nullopt}};
    module.productData = std::make_shared<TestModuleDefinition>("name: test-module\n");

    PeripheralDescriptor hub;
    hub.id = kHub;
    hub.name = "Test hub";
    hub.fits = {PinOutId{"test.chain"}};
    hub.connectors = {ConnectorDescriptor{ConnectorId{"jack"}, "test jack",
                                          {PinOutDescriptor{PinOutId{"test.jack"}, PinOutLevel::Functional,
                                                            Takes::Chain, AfterChange::Nothing}}}};
    hub.places = {PlaceDescriptor{PlaceName{"jack"}, ConnectorId{"jack"}, PinOutId{"test.jack"}, 2}};
    hub.mediaPlaces = {MediaPlaceDescriptor{MediaPlaceId{"hub-disk"}, "test-disk", MediaPassage::Holding, {0}}};   // LIB-4 EC-4

    PeripheralDescriptor uart;
    uart.id = kUart;
    uart.name = "Test UART";
    uart.fits = {PinOutId{"test.bus"}, PinOutId{"test.chain"}};
    return {printer, module, hub, uart};
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
    if (auto s = std::dynamic_pointer_cast<const TestStartUp>(req.content)) m->setStartUp(s->ticks);
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
        m->plugAtBuild(p, cat);
    }
    ++builds;
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
    : desc_(d), variant_(std::move(v)), soundRate_(soundRate) {
    if (isDot()) clock_ = std::make_unique<TestClock>(d.tickRate);
}

bool TestMachine::isDot() const { return std::holds_alternative<DotMatrixGeometry>(desc_.display.geometry); }

bool TestMachine::has(const PeripheralId& id) const {
    return std::any_of(plugs_.begin(), plugs_.end(), [&](const auto& p) { return p->entry.peripheral == id; });
}

bool TestMachine::hasPrinter() const { return has(kPrinter); }

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
    for (auto& p : plugs_)
        if (p->uart) p->uart->advance(ran);
    if (clock_) clock_->advance(ran);
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

void TestMachine::tellOperation() {
    for (auto& p : plugs_) ++p->operations;
}

void TestMachine::reset() {
    tellOperation();
    startUp();
}

void TestMachine::powerCycle() {
    tellOperation();
    displayOn_ = true;
    startUp();
}

void TestMachine::coldBoot() {
    tellOperation();
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
    for (const auto& st : plugs_) p.byPlace.push_back(st->entry);
    auto onConnector = [&](const PlacePath& owner, const std::vector<PlaceDescriptor>& places, const ConnectorId& c) {
        ConnectorPlugs cp{owner, c, {}};
        for (const auto& st : plugs_) {
            const auto& steps = st->entry.place.steps;
            if (steps.size() != owner.steps.size() + 1 ||
                !std::equal(owner.steps.begin(), owner.steps.end(), steps.begin()))
                continue;
            for (const auto& pl : places)
                if (pl.name == steps.back() && pl.connector == c) cp.plugged.push_back(st->entry);
        }
        p.byConnector.push_back(std::move(cp));
    };
    for (const auto& c : desc_.connectors) onConnector(PlacePath{}, desc_.places, c.id);
    for (const auto& st : plugs_)
        for (const auto& c : st->desc.connectors) onConnector(st->entry.place, st->desc.places, c.id);
    return p;
}

std::vector<TestMachine::CapRef> TestMachine::capRefs() const {
    auto* self = const_cast<TestMachine*>(this);   // the capabilities are the machine's own members
    const ComponentId cpu{isDot() ? "main" : "cpu"};
    std::vector<CapRef> out{
        CapRef{CapabilityEntry{TestGeneric::kKind, CapabilityId{cpu.value + "/" + std::string(TestGeneric::kKind.value)},
                               ComponentOwner{cpu}},
               &self->generic_}};
    if (offersState_)
        out.push_back(CapRef{CapabilityEntry{harness::MachineState::kKind,
                                             CapabilityId{cpu.value + "/" + std::string(harness::MachineState::kKind.value)},
                                             ComponentOwner{cpu}},
                             &self->state_});
    if (clock_)
        out.push_back(CapRef{CapabilityEntry{peripherals::RealTimeClock::kKind, CapabilityId{"rtc/real-time-clock"},
                                             ComponentOwner{ComponentId{"rtc"}}},
                             clock_.get()});
    for (const auto& st : plugs_) {
        const std::string at = placeText(st->entry.place);
        const PeripheralOwner owner{st->entry.peripheral, st->entry.place};
        if (st->entry.peripheral == kPrinter)
            out.push_back(CapRef{
                CapabilityEntry{TestCounter::kKind, CapabilityId{at + "/" + std::string(TestCounter::kKind.value)}, owner},
                &self->printerCounter_});
        if (st->controls)
            out.push_back(CapRef{CapabilityEntry{peripherals::Controls::kKind,
                                                 CapabilityId{at + "/" + std::string(peripherals::Controls::kKind.value)},
                                                 owner},
                                 st->controls.get()});
        if (st->uart)
            out.push_back(
                CapRef{CapabilityEntry{peripherals::SerialPort::kKind,
                                       CapabilityId{at + "/" + std::string(peripherals::SerialPort::kKind.value)}, owner},
                       &st->uart->port});
    }
    return out;
}

std::vector<CapabilityEntry> TestMachine::capabilities() const {
    std::vector<CapabilityEntry> out;
    for (auto& r : capRefs()) out.push_back(std::move(r.entry));
    return out;
}

Capability* TestMachine::capability(const CapabilityId& id) {
    for (const auto& r : capRefs())
        if (r.entry.id == id) return r.cap;
    return nullptr;
}

// --- LIB-2: plugging in -------------------------------------------------------

void TestMachine::plugAtBuild(const PeripheralRequest& req, const Catalogue& cat) {
    if (const PeripheralDescriptor* d = cat.peripheral(req.peripheral)) plug(req, *d, desc_.tickRate);
}

void TestMachine::plug(PeripheralRequest req, const PeripheralDescriptor& d, TickRate hostRate) {
    ++plugCalls;
    auto st = std::make_unique<PlugState>();
    st->entry = PlugEntry{req.place, req.peripheral, req.variant};
    st->desc = d;
    st->roms = std::move(req.roms);
    if (!d.controls.empty()) {
        const PlacePath at = req.place;
        st->controls = std::make_unique<peripherals::ControlTable>(d.controls, [this, at](const ControlId& id, bool on) {
            controlEvents_.push_back(ControlEvent{at, id, on, ticks_});
        });
        st->controls->restore(req.switches);
    }
    if (d.id == kUart) st->uart = std::make_unique<TestUart>(hostRate);
    plugs_.push_back(std::move(st));
}

void TestMachine::unplug(const PlacePath& place, const PeripheralId& id) {
    ++unplugCalls;
    const bool there = std::any_of(plugs_.begin(), plugs_.end(),
                                   [&](const auto& s) { return s->entry.place == place && s->entry.peripheral == id; });
    if (!there) return;
    // The peripheral, and every one on a place it offers (EC-3).
    auto below = [&](const PlacePath& p) {
        return p.steps.size() > place.steps.size() && std::equal(place.steps.begin(), place.steps.end(), p.steps.begin());
    };
    plugs_.erase(std::remove_if(plugs_.begin(), plugs_.end(),
                                [&](const auto& s) {
                                    return (s->entry.place == place && s->entry.peripheral == id) || below(s->entry.place);
                                }),
                 plugs_.end());
}

std::optional<std::string> TestMachine::productRefusal(const PeripheralRequest& req,
                                                       const std::optional<PeripheralId>& replacing) const {
    auto other = [&](const PeripheralId& id) {
        return std::any_of(plugs_.begin(), plugs_.end(), [&](const auto& s) {
            return s->entry.peripheral == id && !(replacing && *replacing == id && s->entry.place == req.place);
        });
    };
    if ((req.peripheral == kPrinter && other(kUart)) || (req.peripheral == kUart && other(kPrinter)))
        return std::string(kTogether);
    return std::nullopt;
}

AfterChange TestMachine::followsNow(const PinOutDescriptor& p) const {
    if (isDot() && !displayOn_) return AfterChange::Nothing;   // switched off: its ROM finds the change at power-on
    return p.after;
}

TestMachine::PlugState* TestMachine::stateAt(const PlacePath& place) {
    for (auto& s : plugs_)
        if (s->entry.place == place) return s.get();
    return nullptr;
}

const TestMachine::PlugState* TestMachine::stateAt(const PlacePath& place) const {
    for (const auto& s : plugs_)
        if (s->entry.place == place) return s.get();
    return nullptr;
}

TestUart* TestMachine::uartAt(const PlacePath& place) {
    PlugState* s = stateAt(place);
    return s ? s->uart.get() : nullptr;
}

const std::vector<RomImage>* TestMachine::romsAt(const PlacePath& place) const {
    const PlugState* s = stateAt(place);
    return s ? &s->roms : nullptr;
}

std::uint64_t TestMachine::operationsTold(const PlacePath& place) const {
    const PlugState* s = stateAt(place);
    return s ? s->operations : 0;
}

// --- LIB-11: the machine state ------------------------------------------------

namespace {
void put(std::vector<std::uint8_t>& out, std::uint64_t v) {
    for (int i = 0; i < 8; ++i) out.push_back(static_cast<std::uint8_t>(v >> (8 * i)));
}
bool get(const std::vector<std::uint8_t>& in, std::size_t& at, std::uint64_t& v) {
    if (at + 8 > in.size()) return false;
    v = 0;
    for (int i = 0; i < 8; ++i) v |= static_cast<std::uint64_t>(in[at + std::size_t(i)]) << (8 * i);
    at += 8;
    return true;
}
} // namespace

// Everything that decides what follows: the counters, the keys down, the
// sound not yet taken, the printer's counter. No host file is part of it.
std::vector<std::uint8_t> TestMachine::State::save() const {
    std::vector<std::uint8_t> out;
    const TestMachine& m = m_;
    for (std::uint64_t v : {std::uint64_t(m.ticks_), std::uint64_t(m.sinceStart_), m.presses_, std::uint64_t(m.displayOn_),
                            m.innerCycles_, m.innerRest_, m.samplesMade_, m.generic_.instructions, m.printerCounter_.count})
        put(out, v);
    put(out, m.keysDown_.size());
    for (const auto& k : m.keysDown_) {
        put(out, k.value.size());
        out.insert(out.end(), k.value.begin(), k.value.end());
    }
    put(out, m.sound_.size());
    for (std::int16_t s : m.sound_) put(out, static_cast<std::uint16_t>(s));
    return out;
}

std::optional<std::string> TestMachine::State::restore(const std::vector<std::uint8_t>& in) {
    std::size_t at = 0;
    std::uint64_t v[9], n = 0;
    for (auto& x : v)
        if (!get(in, at, x)) return "The state is cut short.";
    std::vector<KeyId> keys;
    if (!get(in, at, n)) return "The state is cut short.";
    for (std::uint64_t i = 0; i < n; ++i) {
        std::uint64_t len = 0;
        if (!get(in, at, len) || at + len > in.size()) return "The state is cut short.";
        keys.push_back(KeyId{std::string(in.begin() + std::ptrdiff_t(at), in.begin() + std::ptrdiff_t(at + len))});
        at += len;
    }
    std::vector<std::int16_t> sound;
    if (!get(in, at, n)) return "The state is cut short.";
    for (std::uint64_t i = 0; i < n; ++i) {
        std::uint64_t s = 0;
        if (!get(in, at, s)) return "The state is cut short.";
        sound.push_back(static_cast<std::int16_t>(static_cast<std::uint16_t>(s)));
    }
    TestMachine& m = m_;
    m.ticks_ = v[0];
    m.sinceStart_ = v[1];
    m.presses_ = v[2];
    m.displayOn_ = v[3] != 0;
    m.innerCycles_ = v[4];
    m.innerRest_ = v[5];
    m.samplesMade_ = v[6];
    m.generic_.instructions = v[7];
    m.printerCounter_.count = v[8];
    m.keysDown_ = std::move(keys);
    m.sound_ = std::move(sound);
    return std::nullopt;
}

// --- TestUart, TestClock ----------------------------------------------------

void TestUart::advance(Ticks ran) {
    since += ran;
    const Ticks every = rate_ / 100;   // 10 ms
    while (every > 0 && sent < since / every) {
        port.machineSends(static_cast<std::uint8_t>(sent & 0xFF));
        ++sent;
    }
    for (std::uint8_t b; port.machineReceives(b);) received.push_back(b);
}

peripherals::ClockFields TestClock::read() const {
    peripherals::ClockFields f;   // no year: the chip keeps none
    f.month = month_;
    f.day = day_;
    f.weekday = weekday_;
    f.hour = hour_;
    f.minute = minute_;
    f.second = second_;
    return f;
}

void TestClock::set(const peripherals::ClockFields& f, int millisecond) {
    ++setCalls;
    if (f.month) month_ = *f.month;
    if (f.day) day_ = *f.day;
    if (f.weekday) weekday_ = *f.weekday;
    if (f.hour) hour_ = *f.hour;
    if (f.minute) minute_ = *f.minute;
    if (f.second) second_ = *f.second;
    rest_ = static_cast<Ticks>(millisecond) * rate_ / 1000;
}

void TestClock::advance(Ticks ran) {
    rest_ += ran;
    while (rest_ >= rate_) {
        rest_ -= rate_;
        if (++second_ < 60) continue;
        second_ = 0;
        if (++minute_ < 60) continue;
        minute_ = 0;
        if (++hour_ < 24) continue;
        hour_ = 0;
        weekday_ = (weekday_ + 1) % 7;
        if (++day_ <= 30) continue;
        day_ = 1;
        month_ = month_ % 12 + 1;
    }
}

} // namespace calcu::machine::test
