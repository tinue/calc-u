#pragma once
#include "TestMachines.hpp"
#include "typing/TypingSupport.hpp"
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

// `test-type` (LIB-5 design §2, §6, L1): LIB-1's test-seg (100 kHz,
// instructions of 3 and 5 ticks) with the keys typing needs, a key readiness
// the test scripts, the key log with tick counts (TestMachine::keyEvents),
// and typing support with two latches (`shift`, `kbii`) and a sample count:
// every `samplePeriod` ticks the "ROM" reads the keys, and each key down is
// counted (0: never).

namespace calcu::typing::test {

inline const machine::ModelId kTypeModel{"test-type"};

class TypingTestMachine final : public machine::test::TestMachine {
public:
    TypingTestMachine();

    // Key readiness: by default the start-up rule of test-seg (50 ticks).
    std::function<bool(const TypingTestMachine&)> readiness;
    bool isKeyReady() const override { return readiness ? readiness(*this) : TestMachine::isKeyReady(); }

    machine::Ticks advance(machine::Ticks atLeast) override;

    std::map<std::string, bool> latches;
    machine::Ticks samplePeriod = 0;
    std::uint64_t samplesOf(const machine::KeyId& k) const {
        const auto it = samples_.find(k.value);
        return it == samples_.end() ? 0 : it->second;
    }
    bool offersSupport = true;

    using TestMachine::capabilities;
    std::vector<machine::CapabilityEntry> capabilities() const override;
    machine::Capability* capability(const machine::CapabilityId&) override;
    using TestMachine::capability;

    // The key log as "tick key down|up" lines from position `from`.
    std::vector<std::string> log(std::size_t from = 0) const;

private:
    struct Support final : TypingSupport {
        explicit Support(TypingTestMachine& m) : m_(m) {}
        std::vector<std::pair<std::string, bool>> latches() const override {
            return {m_.latches.begin(), m_.latches.end()};
        }
        std::optional<std::uint64_t> samples(const machine::KeyId& k) const override { return m_.samplesOf(k); }
        std::optional<bool> promptReady() const override { return m_.isKeyReady(); }

    private:
        TypingTestMachine& m_;
    };
    Support support_{*this};
    std::map<std::string, std::uint64_t> samples_;
    machine::Ticks sampleRest_ = 0;
};

// A session machine of `test-type`, owned by the caller until handed over.
std::unique_ptr<TypingTestMachine> makeTypingMachine();

} // namespace calcu::typing::test
