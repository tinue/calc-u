#include "TestHarness.hpp"
#include "media/Header.hpp"

// The header every medium's file starts with
// (docs/formats/media.md), in its documented order.

using namespace calcu::media;

TEST(header_all_lines_in_order) {
    const Header h{"Calc-U test card", "test-card", 3, "card-name", "Mine", "Two \"words\"", true,
                   "2026-11-15T10:00:00Z"};
    CHECK_EQ(headerText(h), std::string("# Calc-U test card\n"
                                        "format: test-card\n"
                                        "format-version: 3\n"
                                        "card-name: \"Mine\"\n"
                                        "description: 'Two \"words\"'\n"
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
