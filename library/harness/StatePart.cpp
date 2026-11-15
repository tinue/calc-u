#include "harness/StatePart.hpp"
#include "machine/Machine.hpp"
#include <cstddef>
#include "sessions/LibraryTopics.hpp"

namespace calcu::harness {

using namespace sessions;

namespace {
constexpr const char* kMagic = "calcu-machine-state 1\n";

MachineState* stateOf(machine::Machine& m) {
    for (const auto& e : m.capabilities(MachineState::kKind))
        if (auto* s = machine::capabilityAs<MachineState>(m, e.id)) return s;
    return nullptr;
}

const char* kNoKind = "This machine does not offer its machine state.";
} // namespace

std::string MachineStatePart::header(const machine::Machine& m) const {
    std::string h = kMagic;
    h += "product: " + product_ + "\n";
    h += "version: " + version_ + "\n";
    h += "model: " + m.model().value + "\n";
    h += "variant: " + (m.variant() ? m.variant()->value : std::string("-")) + "\n";
    for (const auto& e : m.plugged().byPlace) {
        std::string place;
        for (const auto& s : e.place.steps) place += (place.empty() ? "" : "/") + s.value;
        h += "plugged: " + place + " " + e.peripheral.value + (e.variant ? ":" + e.variant->value : std::string()) + "\n";
    }
    return h + "\n";
}

void MachineStatePart::attach(SessionSetup& s) {
    display_ = &s.stateTopic(kDisplayTopic);   // the library's topic: read again after a restore
    s.command(kStateSaveCommand, [this](const Value&, SessionContext& c) -> CommandOutcome {
        machine::Machine* m = c.machine();
        if (!m) return Refused{kNoMachine};
        MachineState* st = stateOf(*m);
        if (!st) return Refused{kNoKind};
        const std::string h = header(*m);
        std::vector<std::uint8_t> bytes(h.begin(), h.end());
        const std::vector<std::uint8_t> body = st->save();
        bytes.insert(bytes.end(), body.begin(), body.end());
        return Done{std::make_shared<StateBytes>(std::move(bytes))};
    });
    s.command(kStateRestoreCommand, [this](const Value& args, SessionContext& c) -> CommandOutcome {
        machine::Machine* m = c.machine();
        if (!m) return Refused{kNoMachine};
        MachineState* st = stateOf(*m);
        if (!st) return Refused{kNoKind};
        const Value* b = args.find("bytes");
        if (!b || !b->bytes()) return Refused{"The restore needs bytes."};
        const std::vector<std::uint8_t>& all = *b->bytes();
        const std::string h = header(*m);
        if (all.size() < h.size() || std::string(all.begin(), all.begin() + std::ptrdiff_t(h.size())) != h) {
            const std::string magic = kMagic;
            if (all.size() < magic.size() || std::string(all.begin(), all.begin() + std::ptrdiff_t(magic.size())) != magic)
                return Refused{"The file holds no machine state."};
            return Refused{"The file is of another product version, model, variant or set of plugged peripherals."};
        }
        if (auto why = st->restore(std::vector<std::uint8_t>(all.begin() + std::ptrdiff_t(h.size()), all.end())))
            return Refused{*why};
        if (display_) display_->set(std::make_shared<DisplayState>(m->readDisplay()));
        return Done{};
    });
}

} // namespace calcu::harness
