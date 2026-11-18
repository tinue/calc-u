// The step values and durations of calcu::formats, which the harness
// commands and the preset steps share.
#include "TestHarness.hpp"
#include "formats/Duration.hpp"
#include "formats/Flow.hpp"
#include <string>

using namespace calcu::formats;

TEST(wait_takes_the_steps_duration_forms) {
    std::string err;
    CHECK(parseDuration("2s", &err) == std::optional<std::int64_t>(2'000'000));
    CHECK(parseDuration("1.5s", &err) == std::optional<std::int64_t>(1'500'000));
    CHECK(parseDuration("500ms", &err) == std::optional<std::int64_t>(500'000));
    CHECK(parseDuration("0.25ms", &err) == std::optional<std::int64_t>(250));
    CHECK(parseDuration(" 3s ", &err) == std::optional<std::int64_t>(3'000'000));
    CHECK(!parseDuration("-1s", &err));
    CHECK(!parseDuration("1.0000001s", &err));
    CHECK(!parseDuration("s", &err));
}

TEST(duration_without_unit_is_refused) {
    std::string err;
    CHECK(!parseDuration("2", &err));
    CHECK(err.find("unit") != std::string::npos);
    err.clear();
    CHECK(!parseDuration("2min", &err));
    CHECK(!parseDuration("", &err));
}

TEST(flow_mapping_as_in_a_preset) {
    YamlNode n;
    std::string err;
    CHECK(parseValue("{ name: Disk2, side: 1 }", &n, &err));
    CHECK(n.isMap() && n.map.size() == 2);
    CHECK(n.find("name") && scalarText(*n.find("name")) == "Disk2");
    long side = -1;
    CHECK(n.find("side") && n.find("side")->asInt(&side, &err) && side == 1);

    CHECK(parseValue("{ mem: X, side: 0 }", &n, &err));
    CHECK(n.isMap() && n.find("mem") && scalarText(*n.find("mem")) == "X");

    CHECK(parseValue("{ place: slot-1, as: live, name: My card }", &n, &err));
    CHECK(n.isMap() && n.map.size() == 3);
    CHECK(n.find("name") && scalarText(*n.find("name")) == "My card");
    CHECK(n.find("place") && scalarText(*n.find("place")) == "slot-1");

    // A scalar and a flow sequence by the same reader; an empty text is null.
    CHECK(parseValue("Disk2", &n, &err) && n.isScalar() && scalarText(n) == "Disk2");
    CHECK(parseValue("'quoted text'", &n, &err) && scalarText(n) == "quoted text");
    CHECK(parseValue("[a, b, c]", &n, &err) && n.isSeq() && n.seq.size() == 3);
    CHECK(parseValue("", &n, &err) && n.isNull());
    // What is no value.
    CHECK(!parseValue("{ name: Disk2", &n, &err));
    CHECK(!parseValue("{ name }", &n, &err));
    CHECK(!parseValue("\t{ a: 1 }", &n, &err));
}
