// The value tree of command arguments and translated payloads.
#include "TestHarness.hpp"
#include "sessions/Value.hpp"
#include <cstdint>
#include <string>

using calcu::sessions::Value;

TEST(value_round_trip_every_alternative) {
    const Value null;
    const Value b(true);
    const Value i(std::uint64_t{1} << 40);
    const Value small(-3);
    const Value d(0.25);
    const Value s("text");
    const Value by(Value::Bytes{0x00, 0xab});
    const Value arr(Value::Array{1, "two", false});
    const Value obj = Value::object({{"full", true}, {"key", "2nd"}, {"list", arr}});

    CHECK(null.isNull());
    CHECK(b.boolean() && *b.boolean());
    CHECK(i.integer() && *i.integer() == (std::int64_t{1} << 40));
    CHECK(small.integer() && *small.integer() == -3);
    CHECK(d.real() && *d.real() == 0.25);
    CHECK(s.string() && *s.string() == "text");
    CHECK(by.bytes() && by.bytes()->size() == 2 && (*by.bytes())[1] == 0xab);
    CHECK(arr.elements() && arr.elements()->size() == 3 && *(*arr.elements())[1].string() == "two");
    CHECK(obj.find("full") && *obj.find("full")->boolean());
    CHECK(obj.find("missing") == nullptr);
    CHECK(s.integer() == nullptr && null.members() == nullptr);

    // Copies compare equal, a change does not; the text form is stable.
    const Value copy = obj;
    CHECK(copy == obj);
    CHECK(!(copy == Value::object({{"full", false}, {"key", "2nd"}, {"list", arr}})));
    CHECK(!(Value(1) == Value(1.0)));
    CHECK_EQ(toText(obj), std::string(R"({"full":true,"key":"2nd","list":[1,"two",false]})"));
    CHECK_EQ(toText(by), std::string("\"0x00ab\""));
    CHECK_EQ(toText(Value::Array{Value(), d, Value("a\"b")}), std::string(R"([null,0.25,"a\"b"])"));
}
