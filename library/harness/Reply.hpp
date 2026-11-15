#pragma once
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

// Replies of the harness (LIB-11 design §3.1; spec AC-4, AC-5, AC-6).
//
// Plain (the default): the first line is `done`, `done <n>` followed by n
// result lines, or `refused: <reason>`. Because of the count a reader never
// mistakes a display row for the end of a reply.
// JSON (`--json`): one object on one line with the three keys `done`,
// `result` and `reason`, the same for every command; `result` is null when
// the command reads nothing, `reason` null when it was done.
// No double reaches the output: seconds and brightness are Decimals, so two
// runs agree byte for byte (AC-6).

namespace calcu::harness {

// An exact decimal: units × 10^-places.
struct Decimal {
    std::int64_t units = 0;
    int places = 0;
    bool operator==(const Decimal&) const = default;
};
std::string toText(const Decimal&);
// ticks / rate in seconds with nine decimals, truncated (integers only).
Decimal secondsOf(std::uint64_t ticks, std::uint32_t rate);
// A value of 0 to 1 (a duty, a brightness) with three decimals, rounded.
Decimal fraction3(double v);

// A JSON value whose objects keep their keys in the order given.
class Value {
public:
    struct Member;
    using Array = std::vector<Value>;
    using Object = std::vector<Member>;

    Value();   // null
    Value(std::nullptr_t);
    Value(bool);
    Value(int);
    Value(long);
    Value(long long);
    Value(unsigned);
    Value(unsigned long);
    Value(unsigned long long);
    Value(Decimal);
    Value(std::string);
    Value(const char*);
    Value(Array);
    Value(Object);
    static Value object();
    static Value array();

    Value(const Value&);
    Value(Value&&) noexcept;
    Value& operator=(const Value&);
    Value& operator=(Value&&) noexcept;
    ~Value();

    Value& set(std::string key, Value v);   // on an object
    Value& push(Value v);                   // on an array

    bool isNull() const;
    const std::string* string() const;
    const std::int64_t* integer() const;
    const bool* boolean() const;
    const Array* elements() const;
    const Object* members() const;
    const Value* find(const std::string& key) const;

    friend bool operator==(const Value&, const Value&);
    friend std::string writeJson(const Value&);

private:
    std::variant<std::nullptr_t, bool, std::int64_t, Decimal, std::string, Array, Object> v_;
};

struct Value::Member {
    std::string key;
    Value value;
    bool operator==(const Member&) const = default;
};

struct Reply {
    bool done = true;
    Value result;                      // null: the command reads nothing
    std::string reason;                // one line; empty when done
    std::vector<std::string> lines;    // the plain form of `result`, written by the command
};

Reply doneReply(Value result = {}, std::vector<std::string> lines = {});
Reply refusedReply(std::string reason);

// One JSON text on one line, no spaces; strings escaped (control characters
// as \uXXXX, the rest of UTF-8 as is).
std::string writeJson(const Value&);
// The reply with its final newline.
std::string writePlain(const Reply&);
std::string writeJson(const Reply&);

} // namespace calcu::harness
