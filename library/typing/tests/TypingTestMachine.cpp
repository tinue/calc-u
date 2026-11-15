#include "TypingTestMachine.hpp"

namespace calcu::typing::test {

using namespace calcu::machine;

namespace {

MachineDescriptor typeDescriptor() {
    MachineDescriptor d = *machine::test::testCatalogue().model(machine::test::kSeg);
    d.model = kTypeModel;
    d.keys.clear();
    for (const char* k : {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "a", "b", "c", "shift", "kbii", "enter",
                          "clr", "on"})
        d.keys.push_back(KeyDescriptor{KeyId{k}, {}});
    d.keys[16].otherSpellings = {"CLR", "clear"};
    return d;
}

const MachineDescriptor& descriptorOnce() {
    static const MachineDescriptor d = typeDescriptor();
    return d;
}

const CapabilityId kSupportId{"main/typing-support"};

} // namespace

TypingTestMachine::TypingTestMachine() : TestMachine(descriptorOnce(), std::nullopt, 0) {}

Ticks TypingTestMachine::advance(Ticks atLeast) {
    const Ticks ran = TestMachine::advance(atLeast);
    if (samplePeriod > 0) {
        sampleRest_ += ran;
        while (sampleRest_ >= samplePeriod) {
            sampleRest_ -= samplePeriod;
            for (const KeyId& k : keysDown()) ++samples_[k.value];
        }
    }
    return ran;
}

std::vector<CapabilityEntry> TypingTestMachine::capabilities() const {
    std::vector<CapabilityEntry> out = TestMachine::capabilities();
    if (offersSupport) out.push_back(CapabilityEntry{TypingSupport::kKind, kSupportId, ComponentOwner{ComponentId{"main"}}});
    return out;
}

Capability* TypingTestMachine::capability(const CapabilityId& id) {
    if (offersSupport && id == kSupportId) return &support_;
    return TestMachine::capability(id);
}

std::vector<std::string> TypingTestMachine::log(std::size_t from) const {
    std::vector<std::string> out;
    const auto& e = keyEvents();
    for (std::size_t i = from; i < e.size(); ++i)
        out.push_back(std::to_string(e[i].tick) + " " + e[i].key.value + (e[i].down ? " down" : " up"));
    return out;
}

std::unique_ptr<TypingTestMachine> makeTypingMachine() { return std::make_unique<TypingTestMachine>(); }

} // namespace calcu::typing::test
