#pragma once
#include "sessions/Value.hpp"
#include <compare>
#include <memory>
#include <string_view>
#include <utility>

// Payloads: what a topic carries and what a reply gives. The library carries a payload
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
    virtual bool equals(const Payload&) const = 0;
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

// A payload that is its value (a Web page's own forms, a product's reply):
// equal when the kind and the value are. `kind` is a literal, as every
// PayloadKind is.
class ValuePayload final : public Payload {
public:
    ValuePayload(PayloadKind k, Value v) : kind_(k), value_(std::move(v)) {}
    PayloadKind kind() const override { return kind_; }
    bool equals(const Payload& o) const override { return o.kind() == kind_ && o.toValue() == value_; }
    Value toValue() const override { return value_; }

private:
    PayloadKind kind_;
    Value value_;
};

inline PayloadPtr valuePayload(std::string_view kind, Value v) {
    return std::make_shared<ValuePayload>(PayloadKind{kind}, std::move(v));
}

} // namespace calcu::sessions
