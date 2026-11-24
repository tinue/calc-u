#include "TestHarness.hpp"
#include "media/Header.hpp"

// The header every medium's file starts with
// (docs/formats/media.md), in its documented order.

using namespace calcu::media;

TEST(header_all_lines_in_order) {
    const Header h{"Calc-U test card", "test-card", 3, "card-name", "Mine", "Two \"words\"", "Run SETUP first.", true,
                   "2026-11-15T10:00:00Z"};
    CHECK_EQ(headerText(h), std::string("# Calc-U test card\n"
                                        "format: test-card\n"
                                        "format-version: 3\n"
                                        "card-name: \"Mine\"\n"
                                        "description: 'Two \"words\"'\n"
                                        "insert-notice: \"Run SETUP first.\"\n"
                                        "template: true\n"
                                        "saved: 2026-11-15T10:00:00Z\n"));
}

TEST(header_optional_lines_left_out) {
    Header h;
    h.comment = "Calc-U test memory";
    h.format = "test-memory";
    CHECK_EQ(headerText(h), std::string("# Calc-U test memory\nformat: test-memory\nformat-version: 1\n"));
    h.nameKey = "disk-name";
    h.name = "";
    CHECK_EQ(headerText(h),
             std::string("# Calc-U test memory\nformat: test-memory\nformat-version: 1\ndisk-name: \"\"\n"));
}

TEST(header_quoted_scalar) {
    CHECK_EQ(quotedScalar("a b"), std::string("\"a b\""));
    CHECK_EQ(quotedScalar("say \"hi\""), std::string("'say \"hi\"'"));
}

TEST(header_refuses_a_key_of_no_kind) {
    const HeaderRules rules{"test-card", "test card", 1, "card-name", {"sides"}};
    const std::string good =
        "format: test-card\nformat-version: 1\ncard-name: \"A\"\ninsert-notice: \"N\"\ntemplate: true\nsides: {}\n";
    calcu::formats::YamlNode root;
    std::string problem;
    Header h;
    CHECK(calcu::formats::parseYaml(good, &root, &problem));
    CHECK(readHeader(root, rules, &h, &problem));
    CHECK_EQ(h.insertNotice, std::string("N"));
    calcu::formats::YamlNode other;
    CHECK(calcu::formats::parseYaml(good + "colour: red\n", &other, &problem));
    CHECK(!readHeader(other, rules, &h, &problem));
    CHECK_EQ(problem, std::string("line 7: unknown key colour"));
}
