#pragma once
#include "TestMachines.hpp"
#include "control/Compute.hpp"
#include "control/ControlRules.hpp"
#include "control/ControlState.hpp"
#include "sessions/Payload.hpp"
#include <initializer_list>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// A product's rules for the control tests: one product entry
// (`test.entry`, a value payload holding the running model) and refusals of
// named entries with the product's own reason. Plus the helpers the control
// tests share.

namespace calcu::control::test {

namespace mt = calcu::machine::test;
using machine::MediaPlaceId;
using machine::ModelId;
using machine::PeripheralId;
using machine::PlacePath;
using machine::VariantId;

inline const char* const kTestEntry = "test.entry";
inline const char* const kProductReason = "The test product says no.";

class TestControlRules final : public ControlRules {
public:
    std::optional<std::string> unavailable(const EntryKey& key, const machine::Machine&) const override {
        ++asked;
        for (const auto& [k, why] : refusals)
            if (k == key) return why;
        return std::nullopt;
    }
    void productEntries(const machine::Machine& m, std::vector<ProductEntry>& out) const override {
        if (!entry) return;
        last = sessions::valuePayload(kTestEntry, sessions::Value::object({{"model", m.model().value}}));
        out.push_back(ProductEntry{kTestEntry, last});
    }
    void refuse(EntryKey k, std::string why = kProductReason) { refusals.emplace_back(std::move(k), std::move(why)); }

    std::vector<std::pair<EntryKey, std::string>> refusals;
    bool entry = true;
    mutable int asked = 0;
    mutable sessions::PayloadPtr last;   // the payload the last computation was given
};

inline PlacePath at(std::initializer_list<const char*> steps) {
    PlacePath p;
    for (const char* s : steps) p.steps.push_back(machine::PlaceName{s});
    return p;
}

// The setup of the tests: the test catalogue, the test printer as the only
// printer, the rules given.
inline ControlSetup testSetup(std::vector<const ControlRules*> rules = {},
                              const machine::Catalogue* cat = &mt::testCatalogue()) {
    return ControlSetup{cat, {mt::kPrinter}, std::move(rules)};
}

inline std::vector<std::string> placeTexts(const ControlState& s) {
    std::vector<std::string> out;
    for (const auto& p : s.places) out.push_back(machine::placeText(p.path));
    return out;
}

inline std::vector<std::string> fittingIds(const PlaceEntry& p) {
    std::vector<std::string> out;
    for (const auto& c : p.fitting) out.push_back(c.id.value);
    return out;
}

inline const VariantChoice* variantOf(const PeripheralChoice& c, const char* id) {
    for (const auto& v : c.variants)
        if (v.id.value == id) return &v;
    return nullptr;
}

inline Availability no(const std::string& why) { return Availability{false, why}; }
inline const Availability kYes{};

} // namespace calcu::control::test
