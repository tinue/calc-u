#pragma once
#include "machine/Build.hpp"
#include "machine/Capability.hpp"
#include "machine/Catalogue.hpp"
#include "machine/Machine.hpp"
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
//   test-printer  fits test.bus; a switch control `online`; while plugged a
//             capability of the product kind "test.counter".
//   test-module   fits test.slot and test.bus; variants 4k (default) and
//             8k, each with a ROM entry.

namespace calcu::machine::test {

inline constexpr TickRate kSegRate = 100'000;
inline constexpr TickRate kDotRate = 1'000'000;
inline constexpr Ticks kStartUpTicks = 50;

inline const ModelId kSeg{"test-seg"};
inline const ModelId kDot{"test-dot"};
inline const PeripheralId kPrinter{"test-printer"};
inline const PeripheralId kModule{"test-module"};

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

// The build option of test-seg for the pacing tests (LIB-1 D-8), handed in
// as BuildRequest::content.
struct TestSegTiming : ProductData {
    TestSegTiming(Ticks length, std::uint64_t loop) : instructionLength(length), loopLength(loop) {}
    Ticks instructionLength;   // ticks of every instruction, at least 1
    std::uint64_t loopLength;  // instructions per loop, at least 1
};

Catalogue makeTestCatalogue();
const Catalogue& testCatalogue();   // one shared copy; a test that adds to it makes its own

// A request with a ROM image for every entry the model's variant and each
// peripheral need.
BuildRequest requestFor(const Catalogue&, const ModelId&, std::optional<VariantId> variant = std::nullopt,
                        std::vector<PeripheralRequest> peripherals = {}, int soundRate = 8000);
PeripheralRequest peripheralAt(const Catalogue&, const std::string& place, const PeripheralId&,
                               std::optional<VariantId> variant = std::nullopt);

class TestBuilder : public MachineBuilder {
public:
    BuildResult build(const BuildRequest&, const Catalogue&) override;
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
    bool isKeyReady() const override { return sinceStart_ >= kStartUpTicks; }
    KeyCall keyDown(const KeyId&) override;
    KeyCall keyUp(const KeyId&) override;
    DisplayOutput readDisplay() override;
    std::size_t takeSound(std::span<std::int16_t> out) override;
    Plugged plugged() const override;
    using Machine::capabilities;
    std::vector<CapabilityEntry> capabilities() const override;
    Capability* capability(const CapabilityId&) override;
    std::vector<DebugTargetId> debugTargets() const override { return desc_.debugTargets; }

    // Inside views for the tests.
    void plug(PlugEntry e) { plugged_.push_back(std::move(e)); }
    const std::vector<KeyEvent>& keyEvents() const { return events_; }
    const std::vector<KeyId>& keysDown() const { return keysDown_; }
    std::uint64_t instructionsRun() const { return generic_.instructions; }
    std::uint64_t innerCycles() const { return innerCycles_; }
    void setTiming(std::shared_ptr<const TestSegTiming> t) { timing_ = std::move(t); }
    // The instruction of the loop the machine is at (0 without a timing).
    std::uint64_t loopPosition() const { return timing_ ? generic_.instructions % timing_->loopLength : 0; }

private:
    bool isDot() const;
    bool hasPrinter() const;
    void startUp() { sinceStart_ = 0; }

    MachineDescriptor desc_;
    std::optional<VariantId> variant_;
    int soundRate_;
    Ticks ticks_ = 0;
    Ticks sinceStart_ = 0;
    std::uint64_t presses_ = 0;   // the machine's memory: a cold boot clears it
    bool displayOn_ = true;
    std::uint64_t innerCycles_ = 0, innerRest_ = 0;
    std::vector<KeyId> keysDown_;
    std::vector<KeyEvent> events_;
    std::vector<PlugEntry> plugged_;
    std::uint64_t samplesMade_ = 0;
    std::vector<std::int16_t> sound_;
    TestGeneric generic_;
    TestCounter printerCounter_;
    std::shared_ptr<const TestSegTiming> timing_;
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
