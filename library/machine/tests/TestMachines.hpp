#pragma once
#include "TestCpu.hpp"
#include "machine/Build.hpp"
#include "machine/Capability.hpp"
#include "machine/Catalogue.hpp"
#include "harness/MachineState.hpp"
#include "machine/Machine.hpp"
#include "media/MediaPlace.hpp"
#include "paper/PaperOutput.hpp"
#include "peripherals/Controls.hpp"
#include "peripherals/RealTimeClock.hpp"
#include "peripherals/SerialPort.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

// The machines built for the tests (LIB-1 design §6, T4; AC-24): a test
// catalogue with two models at different tick rates and two peripherals, its
// builder, and a product kind. Library tests use these, never product code.
//
//   test-seg  segment display of 3 positions (A-G and DP with a brightness
//             of its own), indicator C with brightness, no sound, no
//             variant; instructions of 3 and 5 ticks in turn; places
//             slot-1 (connector slot, one module) and printer, aux (a chain
//             on connector bus).
//   test-dot  dot matrix 12 x 8 in cells of 6 x 8, indicators RUN and BUSY,
//             a square-wave sound derived from the tick count, variants V1
//             (default) and V2; the key `off` switches the display off and
//             `on` on; instructions of 4 ticks; an inner block on a clock of
//             3/8 of the tick, converted inside the machine (EC-7).
//   Both      a start-up of 50 ticks before key readiness after a build,
//             reset, power cycle and cold boot.
//   test-seg  build option TestSegTiming (LIB-17, M-21; LIB-1 D-8): every
//             instruction of a fixed length, and a loop of a settable number
//             of instructions whose position loopPosition() reports.
//   test-printer  fits test.bus, test.chain and test.jack; the switches
//             `online` and `trace` and the key `feed` (a controls
//             capability); while plugged a capability of the product kind
//             "test.counter".
//   test-module   fits test.slot, test.bus and test.card; variants 4k
//             (default) and 8k, each with a ROM entry.
// For LIB-2 (design §6, T2):
//   test-dot  also a chain connector `chain` (pin-out test.chain, contact
//             level, Takes::Chain, PowerCycle; places a, b, one each), a
//             module connector `card` (test.card, functional, Takes::One,
//             PowerCycle; places card-1, card-2, one each) and a component
//             `rtc` with a real-time clock that keeps every field but the
//             year. Switched off by the key `off`, it answers Nothing.
//   test-hub  fits test.chain; offers the place `jack` (connector jack,
//             pin-out test.jack, functional, Takes::Chain, Nothing; two at
//             once); from LIB-4 (M-37) the holding media place `hub-disk`
//             (kind test-disk, side 0), listed with the hub as owner (EC-4).
//   test-uart fits test.bus and test.chain; a serial port that sends one
//             byte (0, 1, 2, ...) every 10 ms of emulated time and keeps what
//             it receives.
//   Every plugged peripheral counts the operations it was told of (AC-10).
//   A product rule: the test printer and the test UART are not used
//   together (the library form of LIB-2 AC-29).
// For LIB-11 (design T18): with offerState(true) a machine offers the
//   generic kind machine state (`cpu/machine-state` or `main/machine-state`),
//   off by default so that the other areas' lists stay as they are.
// For LIB-6 (design D-25): test-seg with the build option TestSegProgram
//   (or setProgram()): the key `1` starts a "program" of a settable number
//   of ticks during which isKeyReady() is false, the key `0` during it stops
//   it; and a product kind "test.memory" (64 bytes, zero after a cold boot)
//   that a preset's test section writes. Off by default.
// For LIB-8 (design §6, "Test printers"), in a catalogue of their own,
//   paperTestCatalogue() (the test catalogue plus these two), so that the
//   lists other areas' tests check stay as they are:
//   test-dotter  name TD-20; fits test.bus and test.chain; dots and a text
//             layer (unit 1/2 mm, width 40, printable 2..38, dots of 0.8,
//             greys {160, 0}, 300 dpi); key `feed`, switch `online`;
//             variants r1 (default) and r2. While `online` is on, each key
//             pressed on the machine prints a line: the text `KEY <id>`
//             padded to 8 characters at (2, top), then 5 dot rows, one per
//             20 ticks, a fixed pattern per character; a line takes 7
//             units. `feed`: 4 units and an empty text line. A detach in a
//             line ends it there, its last row with darkness 0 (EC-7).
//   test-plotter  name TP-4; fits test.bus; pen strokes in four colours, no
//             text layer (unit 1/10 mm, width 600, printable 50..550, stroke
//             width 3, 254 dpi); keys `feed` (20 units) and `back` (feeds
//             back 30). Each key k (its index among the machine's keys)
//             pressed on the machine draws one stroke through four
//             positions, one per 10 ticks, from (100 + 50k, standing), in
//             colour k mod 4 (black, blue, green, red).
// For LIB-10 (design §6): with the build option TestProgram (or
//   enableCpu()) the test processor of TestCpu.hpp runs the instructions,
//   one per instruction of the model's timing: test-seg's target
//   `test-cpu`, test-dot's `main` and `inner` by turns. debugging() is
//   nullptr without it, so the other areas' tests stay as they are.

// For LIB-7 (design §2, §6), switched on by TestBuilder::media (off by
//   default, so that the other areas' capability lists stay as they are):
//   each media place of a descriptor offers a MediaPlace (test-seg `reader`:
//   pass-through, sides 0 and 1; test-dot `drive` and test-hub `hub-disk`:
//   holding, side 0) over TestMediumContent, read and written through test
//   hooks; a card pushed through the reader passes in kPassTicks ticks. In a
//   catalogue of its own, mediaTestCatalogue() (the test catalogue plus it):
//   test-memory-module  name "Test memory module"; fits test.slot and
//             test.card; the switch `write-protect` (controls kind); a
//             ModuleMedium over the content of its request.
//   test-rom-module  name "Test ROM module"; fits test.slot; not a medium.
namespace calcu::machine::test {

inline constexpr TickRate kSegRate = 100'000;
inline constexpr TickRate kDotRate = 1'000'000;
inline constexpr Ticks kStartUpTicks = 50;

inline const ModelId kSeg{"test-seg"};
inline const ModelId kDot{"test-dot"};
inline const PeripheralId kPrinter{"test-printer"};
inline const PeripheralId kModule{"test-module"};
inline const PeripheralId kHub{"test-hub"};
inline const PeripheralId kUart{"test-uart"};
inline const PeripheralId kDotter{"test-dotter"};     // LIB-8: TD-20
inline const PeripheralId kPlotter{"test-plotter"};   // LIB-8: TP-4
inline const PeripheralId kMemoryModule{"test-memory-module"};   // LIB-7
inline const PeripheralId kRomModule{"test-rom-module"};         // LIB-7: not a medium
inline constexpr Ticks kPassTicks = 200;                          // LIB-7: a card passes the reader
inline const char* const kTogether = "The test printer and the test UART cannot be used together.";

// The product kind of the tests: the instructions run while the printer is
// plugged in. Named with a product's prefix, as product kinds are.
class TestCounter : public Capability {
public:
    static constexpr KindId kKind{"test.counter"};
    KindId kind() const override { return kKind; }
    std::uint64_t count = 0;
};

// Stands in for a generic kind (the library defines none in M-18): the
// instructions a component has run.
class TestGeneric : public Capability {
public:
    static constexpr KindId kKind{"test-generic"};
    KindId kind() const override { return kKind; }
    std::uint64_t instructions = 0;
};

// A module definition as a product would read it (ProductData, §3.9).
struct TestModuleDefinition : ProductData {
    explicit TestModuleDefinition(std::string t) : text(std::move(t)) {}
    std::string text;
};

// LIB-7: the content of a medium of the test kinds (media/tests/TestKinds.hpp).
struct TestMediumContent : ProductData {
    explicit TestMediumContent(std::string b) : bytes(std::move(b)) {}
    std::string bytes;
};

// LIB-7: a media place of a test machine.
class TestMediaPlace final : public media::MediaPlace {
public:
    explicit TestMediaPlace(MediaPlaceDescriptor d) : d_(std::move(d)) {}
    MediaPlaceId id() const override { return d_.id; }
    Inserted insert(media::Content, int side) override;
    media::Content eject() override;
    std::optional<int> side() const override { return side_; }
    media::Content content() const override { return content_; }
    std::uint64_t revision() const override { return revision_; }
    bool waitsForMedium() const override { return waits; }
    std::optional<GivenBack> takeGivenBack() override {
        std::optional<GivenBack> back = std::move(given_);
        given_.reset();
        return back;
    }
    bool writesWhenAsked() const override { return onlyWhenAsked; }
    bool takeWriteNow() override {
        const bool w = writeNow_;
        writeNow_ = false;
        return w;
    }

    // Test hooks: the "ROM" writes (a holding place: at once; a pass-through
    // place: `writeOnPass` as the medium passes); a write moment (D-11).
    bool write(std::string bytes);
    void requestWriteNow() { writeNow_ = true; }
    void advance(Ticks ran);
    std::string bytes() const;   // "" when empty
    bool passThrough() const { return d_.passage == MediaPassage::PassThrough; }
    bool waits = false;
    std::optional<std::string> writeOnPass;
    std::optional<std::string> refuse;   // insert refused with this reason
    bool holdsBack = false;              // a pass-through medium never passes
    bool onlyWhenAsked = false;          // writesWhenAsked() (M-47: a tape)
    int inserts = 0;

private:
    MediaPlaceDescriptor d_;
    std::optional<int> side_;
    media::Content content_;
    std::uint64_t revision_ = 0;
    Ticks left_ = 0;
    std::optional<GivenBack> given_;
    bool writeNow_ = false;
};

// LIB-7: the module medium of test-memory-module.
class TestModuleMedium final : public media::ModuleMedium {
public:
    TestModuleMedium(media::Content c, const peripherals::ControlTable* controls);
    media::Content content() const override { return content_; }
    std::uint64_t revision() const override { return revision_; }
    bool write(std::string bytes);   // false while the write-protect switch is on
    std::string bytes() const;

private:
    media::Content content_;
    const peripherals::ControlTable* controls_;
    std::uint64_t revision_ = 0;
};

// The build option of test-seg for the pacing tests (LIB-1 D-8), handed in
// as BuildRequest::content.
// The build option of both models for LIB-18's tests (design §2): the
// start-up after a build, reset, power cycle and cold boot lasts `ticks`
// instead of kStartUpTicks; kNever: the machine never reports key readiness
// (EC-7). Handed in as BuildRequest::content.
struct TestStartUp : ProductData {
    static constexpr Ticks kNever = ~Ticks{0};
    explicit TestStartUp(Ticks t) : ticks(t) {}
    Ticks ticks;
};

// LIB-6's build option of test-seg (design D-25): a program of `length` ticks.
struct TestSegProgram : ProductData {
    explicit TestSegProgram(Ticks l) : length(l) {}
    Ticks length;
};

// The memory a preset's test section writes (LIB-6), a product kind.
class TestMemory : public Capability {
public:
    static constexpr KindId kKind{"test.memory"};
    KindId kind() const override { return kKind; }
    std::vector<std::uint8_t> bytes = std::vector<std::uint8_t>(64, 0);
};

struct TestSegTiming : ProductData {
    TestSegTiming(Ticks length, std::uint64_t loop) : instructionLength(length), loopLength(loop) {}
    Ticks instructionLength;   // ticks of every instruction, at least 1
    std::uint64_t loopLength;  // instructions per loop, at least 1
};

Catalogue makeTestCatalogue();
const Catalogue& testCatalogue();   // one shared copy; a test that adds to it makes its own
// LIB-8: the test catalogue plus test-dotter and test-plotter.
Catalogue makePaperTestCatalogue();
const Catalogue& paperTestCatalogue();
// LIB-7: the test catalogue plus test-memory-module.
Catalogue makeMediaTestCatalogue();
const Catalogue& mediaTestCatalogue();
paper::PaperDescription td20Description();
paper::PaperDescription tp4Description();

// LIB-8: the test printers' paper output (the behaviour above).
class TestPaperPrinter final : public paper::PaperOutputBase {
public:
    TestPaperPrinter(bool plotter, const peripherals::ControlTable* controls);
    const paper::PaperDescription& description() const override { return description_; }
    paper::Coord standing() const override { return standing_; }
    void key(const KeyId&, std::size_t index);       // a key pressed on the machine
    void control(const ControlId&, bool on);         // a key or switch of its own
    void advance(Ticks ran);
    void detaching();                                // EC-7
    bool busy() const { return inLine_ || !queue_.empty(); }

private:
    void start();
    void step();
    void dotRow(std::uint8_t darkness);
    bool plotter_;
    const peripherals::ControlTable* controls_;
    paper::PaperDescription description_;
    paper::Coord standing_ = 0;
    std::vector<std::pair<std::string, std::size_t>> queue_;   // dotter: line texts; plotter: key indexes
    bool inLine_ = false;
    std::string text_;
    std::size_t index_ = 0;
    int step_ = 0;           // dotter: the next row; plotter: the next position
    Ticks untilNext_ = 0;
};

// A request with a ROM image for every entry the model's variant and each
// peripheral need.
BuildRequest requestFor(const Catalogue&, const ModelId&, std::optional<VariantId> variant = std::nullopt,
                        std::vector<PeripheralRequest> peripherals = {}, int soundRate = 8000);
PeripheralRequest peripheralAt(const Catalogue&, const std::string& place, const PeripheralId&,
                               std::optional<VariantId> variant = std::nullopt);

class TestBuilder : public MachineBuilder {
public:
    BuildResult build(const BuildRequest&, const Catalogue&) override;
    int builds = 0;   // the machines built (LIB-18 AC-5)
    bool media = false;   // LIB-7: the machines offer their media places
};

// The test UART: one byte every 10 ms of the emulated time since it was
// plugged in, at the host's tick rate it learnt then (P-2).
class TestUart {
public:
    explicit TestUart(TickRate hostRate) : rate_(hostRate) {
        port.machineSetsLine(peripherals::LineSettings{1200, 8, std::nullopt, std::nullopt});   // 1200 baud, 8 bits
    }
    void advance(Ticks ran);
    peripherals::SerialPortQueues port;
    std::vector<std::uint8_t> received;
    std::uint64_t sent = 0;
    Ticks since = 0;

private:
    TickRate rate_;
};

// The clock of test-dot: month, day, weekday, hour, minute and second, moved
// by ticks only (AC-22); 30 days a month.
class TestClock final : public peripherals::RealTimeClock {
public:
    explicit TestClock(TickRate rate) : rate_(rate) {}
    peripherals::ClockFields read() const override;
    void set(const peripherals::ClockFields&, int millisecond = 0) override;
    void advance(Ticks ran);
    int setCalls = 0;

private:
    TickRate rate_;
    Ticks rest_ = 0;
    int month_ = 1, day_ = 1, weekday_ = 0, hour_ = 0, minute_ = 0, second_ = 0;
};

struct ControlEvent {
    PlacePath place;
    ControlId id;
    bool on;
    Ticks tick;
    bool operator==(const ControlEvent&) const = default;
};

// What a test sees of a key call inside the machine.
struct KeyEvent {
    KeyId key;
    bool down;
    Ticks tick;
    bool operator==(const KeyEvent&) const = default;
};

class TestMachine : public Machine {
public:
    TestMachine(const MachineDescriptor&, std::optional<VariantId>, int soundRate);

    const MachineDescriptor& descriptor() const override { return desc_; }
    std::optional<VariantId> variant() const override { return variant_; }
    Ticks advance(Ticks atLeast) override;
    Ticks tickCount() const override { return ticks_; }
    void reset() override;
    void powerCycle() override;
    void coldBoot() override;
    bool isKeyReady() const override { return sinceStart_ >= startUpTicks_ && programLeft_ == 0; }
    KeyCall keyDown(const KeyId&) override;
    KeyCall keyUp(const KeyId&) override;
    DisplayOutput readDisplay() override;
    std::size_t takeSound(std::span<std::int16_t> out) override;
    Plugged plugged() const override;
    using Machine::capabilities;
    std::vector<CapabilityEntry> capabilities() const override;
    Capability* capability(const CapabilityId&) override;
    std::vector<DebugTargetId> debugTargets() const override { return desc_.debugTargets; }
    Debugging* debugging() override { return cpu_.get(); }   // LIB-10: with TestProgram only
    void enableCpu(const TestProgram& p) { cpu_ = std::make_unique<TestCpu>(desc_.debugTargets, p); }
    TestCpu* cpu() { return cpu_.get(); }

    // At build: the builder has checked the request.
    void plugAtBuild(const PeripheralRequest&, const Catalogue&);

    // Inside views for the tests.
    const std::vector<ControlEvent>& controlEvents() const { return controlEvents_; }
    TestUart* uartAt(const PlacePath&);
    const std::vector<RomImage>* romsAt(const PlacePath&) const;
    TestPaperPrinter* printerAt(const PlacePath&);   // LIB-8
    std::uint64_t operationsTold(const PlacePath&) const;   // AC-10
    TestClock* clock() { return clock_.get(); }
    int plugCalls = 0, unplugCalls = 0;
    const std::vector<KeyEvent>& keyEvents() const { return events_; }
    const std::vector<KeyId>& keysDown() const { return keysDown_; }
    std::uint64_t instructionsRun() const { return generic_.instructions; }
    std::uint64_t innerCycles() const { return innerCycles_; }
    void setTiming(std::shared_ptr<const TestSegTiming> t) { timing_ = std::move(t); }
    void setStartUp(Ticks t) { startUpTicks_ = t; }   // TestStartUp
    // LIB-6 (TestSegProgram): the program of `length` ticks and the memory.
    void setProgram(Ticks length) { programLength_ = length; }
    bool programRuns() const { return programLeft_ > 0; }
    TestMemory* memory() { return programLength_ ? &memory_ : nullptr; }
    std::uint64_t presses() const { return presses_; }   // the machine's memory
    // The instruction of the loop the machine is at (0 without a timing).
    std::uint64_t loopPosition() const { return timing_ ? generic_.instructions % timing_->loopLength : 0; }
    // LIB-11 AC-24: the machine-state capability (not by default).
    void offerState(bool on) { offersState_ = on; }
    // LIB-7: the media places as MediaPlace capabilities (not by default).
    void enableMedia();
    TestMediaPlace* mediaPlace(const std::string& id, const PlacePath& owner = {});
    TestModuleMedium* moduleAt(const PlacePath&);

protected:
    std::optional<std::string> productRefusal(const PeripheralRequest&,
                                              const std::optional<PeripheralId>& replacing) const override;
    void plug(PeripheralRequest, const PeripheralDescriptor&, TickRate hostRate) override;
    void unplug(const PlacePath&, const PeripheralId&) override;
    AfterChange followsNow(const PinOutDescriptor&) const override;

private:
    struct PlugState {
        PlugEntry entry;
        PeripheralDescriptor desc;
        std::vector<RomImage> roms;
        std::unique_ptr<peripherals::ControlTable> controls;
        std::unique_ptr<TestUart> uart;
        std::unique_ptr<TestPaperPrinter> printer;   // LIB-8
        std::unique_ptr<TestMediaPlace> place;       // LIB-7
        std::unique_ptr<TestModuleMedium> module;    // LIB-7
        std::uint64_t operations = 0;
    };
    struct CapRef {
        CapabilityEntry entry;
        Capability* cap;
    };
    std::vector<CapRef> capRefs() const;
    PlugState* stateAt(const PlacePath&);
    const PlugState* stateAt(const PlacePath&) const;
    bool has(const PeripheralId&) const;
    void tellOperation();
    bool isDot() const;
    bool hasPrinter() const;
    void startUp() { sinceStart_ = 0; }

    MachineDescriptor desc_;
    std::optional<VariantId> variant_;
    int soundRate_;
    Ticks ticks_ = 0;
    Ticks sinceStart_ = 0;
    Ticks startUpTicks_ = kStartUpTicks;
    std::uint64_t presses_ = 0;   // the machine's memory: a cold boot clears it
    Ticks programLength_ = 0, programLeft_ = 0;   // LIB-6: 0 length: no program
    TestMemory memory_;
    bool displayOn_ = true;
    std::uint64_t innerCycles_ = 0, innerRest_ = 0;
    std::vector<KeyId> keysDown_;
    std::vector<KeyEvent> events_;
    std::vector<std::unique_ptr<PlugState>> plugs_;
    std::vector<ControlEvent> controlEvents_;
    std::unique_ptr<TestClock> clock_;
    std::uint64_t samplesMade_ = 0;
    std::vector<std::int16_t> sound_;
    TestGeneric generic_;
    TestCounter printerCounter_;
    class State final : public harness::MachineState {
    public:
        explicit State(TestMachine& m) : m_(m) {}
        std::vector<std::uint8_t> save() const override;
        std::optional<std::string> restore(const std::vector<std::uint8_t>&) override;

    private:
        TestMachine& m_;
    };
    State state_{*this};
    bool offersState_ = false;
    bool media_ = false;
    std::vector<std::unique_ptr<TestMediaPlace>> places_;   // LIB-7
    std::shared_ptr<const TestSegTiming> timing_;
    std::unique_ptr<TestCpu> cpu_;   // LIB-10
};

// Builds through calcu::machine::build() on testCatalogue(); null if refused.
std::unique_ptr<TestMachine> buildTestMachine(const ModelId&, std::optional<VariantId> variant = std::nullopt,
                                              std::vector<PeripheralRequest> peripherals = {}, int soundRate = 8000);

// test-seg with the build option TestSegTiming (LIB-17's tests).
std::unique_ptr<TestMachine> buildTimedTestSeg(Ticks instructionLength, std::uint64_t loopLength = 1);

// Display outputs compared field by field (indicator names by content).
bool sameDisplay(const DisplayOutput&, const DisplayOutput&);

// Advances until key readiness; returns the ticks run.
Ticks runUntilKeyReady(Machine&);

} // namespace calcu::machine::test
