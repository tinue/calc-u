#pragma once
#include "sessions/Value.hpp"
#include <compare>
#include <memory>
#include <string_view>

// Payloads (LIB-3 design §3.1; frontend.md I-3, I-4; spec AC-12): what a
// topic carries and what a reply gives. The library carries a payload
// without reading it: it calls only equals() (to decide whether to send) and,
// in a translating host, toValue(). An in-process frontend uses payloadAs<P>.
// A payload is immutable once sent and shared, never copied; it holds no
// pointer into a machine.

namespace calcu::sessions {

struct PayloadKind {
    std::string_view value;   // "display", "busy"; a product's with its prefix: "c59.cue-card"
    auto operator<=>(const PayloadKind&) const = default;
};

class Payload {
public:
    virtual ~Payload() = default;
    virtual PayloadKind kind() const = 0;
    virtual bool equals(const Payload&) const = 0;   // AC-5, EC-2
    virtual Value toValue() const = 0;               // for translators only
};

using PayloadPtr = std::shared_ptr<const Payload>;

// The payload as P (P::kKind compared, then static_cast; no RTTI), or nullptr.
template <class P> const P* payloadAs(const Payload& p) {
    return p.kind() == P::kKind ? static_cast<const P*>(&p) : nullptr;
}
template <class P> const P* payloadAs(const PayloadPtr& p) { return p ? payloadAs<P>(*p) : nullptr; }

// The common part of a payload class D: D has `static constexpr PayloadKind
// kKind`, `bool same(const D&) const` and `Value toValue() const override`.
template <class D> class PayloadOf : public Payload {
public:
    PayloadKind kind() const override { return D::kKind; }
    bool equals(const Payload& o) const override {
        const D* p = payloadAs<D>(o);
        return p && static_cast<const D&>(*this).same(*p);
    }
};

} // namespace calcu::sessions
