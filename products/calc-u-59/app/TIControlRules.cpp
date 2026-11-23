#include "TIControlRules.hpp"
#include "TIShellProduct.hpp"
#include "machine/Capability.hpp"
#include "media/MediaPlace.hpp"
#include "sessions/Payload.hpp"
#include <variant>

namespace cu59 {

using calcu::sessions::Value;

std::vector<CardControl> cardControls(bool passing, const calcu::control::Availability& insert) {
    if (passing) return {{kCardControlEject, "⏏", "Eject Card", "Eject Card", true, {}}};
    const std::string reason = insert.now ? std::string() : insert.reason;
    // Both read "Crd"; the icon and the name tell them apart.
    return {{kItemArmCard, "⤓", "Crd", "Arm Card", insert.now, reason},
            {kItemArmFreshCard, "＋", "Crd", "Arm Fresh Card", insert.now, reason}};
}

Value toValue(const std::vector<CardControl>& controls) {
    Value::Array out;
    for (const auto& c : controls) {
        Value::Object o{{"id", Value(c.id)},       {"icon", Value(c.icon)},
                        {"label", Value(c.label)}, {"name", Value(c.name)},
                        {"available", Value(c.available)}};
        if (!c.available) o.push_back({"reason", Value(c.reason)});
        out.emplace_back(Value(std::move(o)));
    }
    return Value::object({{"controls", Value(std::move(out))}});
}

std::vector<CardControl> cardControlsOf(const calcu::control::ControlState& cs) {
    std::vector<CardControl> out;
    const auto* e = cs.productEntry(kCardControlsEntry);
    if (!e || !e->value) return out;
    const Value v = e->value->toValue();
    const Value* list = v.find("controls");
    if (!list || !list->elements()) return out;
    auto text = [](const Value& o, const char* key) {
        const Value* t = o.find(key);
        return t && t->string() ? *t->string() : std::string();
    };
    for (const auto& c : *list->elements()) {
        const Value* a = c.find("available");
        out.push_back({text(c, "id"), text(c, "icon"), text(c, "label"), text(c, "name"),
                       !(a && a->boolean() && !*a->boolean()), text(c, "reason")});
    }
    return out;
}

namespace {

// A card passes the machine's own reader now.
bool cardPasses(const calcu::machine::Machine& m) {
    for (const auto& e : m.capabilities(calcu::media::MediaPlace::kKind)) {
        if (std::holds_alternative<calcu::machine::PeripheralOwner>(e.owner)) continue;
        const auto* p = calcu::machine::capabilityAs<calcu::media::MediaPlace>(m, e.id);
        if (p && p->id().value == kCardReaderPlace) return p->side().has_value();
    }
    return false;
}

} // namespace

void TIControlRules::productEntries(const calcu::machine::Machine& m, const calcu::control::ControlState& listed,
                                    std::vector<calcu::control::ProductEntry>& out) const {
    m_debug.productEntries(m, out);
    if (const auto* reader = listed.mediaPlace(calcu::machine::MediaPlaceId{kCardReaderPlace}))
        out.push_back(calcu::control::ProductEntry{
            kCardControlsEntry,
            calcu::sessions::valuePayload(kCardControlsEntry, toValue(cardControls(cardPasses(m), reader->insert)))});
}

} // namespace cu59
