#pragma once
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

// A JSON-like value (LIB-3 design §3.1): the arguments of commands and
// requests, and what a translating host (the Swift bridge, the Web bindings,
// the harness) makes of a payload with Payload::toValue(). Objects keep their
// members in the order they were given.

namespace calcu::sessions {

class Value {
public:
    struct Member;
    using Bytes  = std::vector<std::uint8_t>;
    using Array  = std::vector<Value>;
    using Object = std::vector<Member>;

    Value();                       // null
    Value(std::nullptr_t);
    Value(bool);
    template <class I>
        requires(std::is_integral_v<I> && !std::is_same_v<I, bool>)
    Value(I i) : Value(Int{static_cast<std::int64_t>(i)}) {}
    Value(double);
    Value(std::string);
    Value(const char*);
    Value(Bytes);
    Value(Array);
    Value(Object);
    static Value object(std::initializer_list<Member>);

    Value(const Value&);
    Value(Value&&) noexcept;
    Value& operator=(const Value&);
    Value& operator=(Value&&) noexcept;
    ~Value();

    bool isNull() const;
    const bool* boolean() const;          // nullptr if another alternative
    const std::int64_t* integer() const;
    const double* real() const;
    const std::string* string() const;
    const Bytes* bytes() const;
    const Array* elements() const;
    const Object* members() const;
    const Value* find(std::string_view key) const;   // an object's member, or nullptr

    friend bool operator==(const Value&, const Value&);

private:
    struct Int { std::int64_t v; };
    explicit Value(Int);
    std::variant<std::nullptr_t, bool, std::int64_t, double, std::string, Bytes, Array, Object> v_;
};

struct Value::Member {
    std::string key;
    Value value;
};

// One line of JSON-like text (bytes as "0x..." strings); for logs and tests.
std::string toText(const Value&);

} // namespace calcu::sessions
