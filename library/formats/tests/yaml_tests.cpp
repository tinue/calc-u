// The YAML-subset reader of the own file formats (library/formats/Yaml.hpp).
#include "TestHarness.hpp"
#include "formats/Number.hpp"
#include "formats/Yaml.hpp"
#include <optional>
#include <string>

using namespace calcu::formats;

TEST(test_yaml_block_map_and_scalars) {
    YamlNode n;
    std::string err;
    CHECK(parseYaml("a: 1\nb: hello\nc: true\n", &n, &err));
    CHECK(n.isMap() && n.map.size() == 3);
    long v = 0;
    CHECK(n.find("a") && n.find("a")->asInt(&v, &err) && v == 1);
    std::string s;
    CHECK(n.find("b") && n.find("b")->asString(&s, &err) && s == "hello");
    bool b = false;
    CHECK(n.find("c") && n.find("c")->asBool(&b, &err) && b);
}

TEST(test_yaml_hex_ints) {
    YamlNode n;
    std::string err;
    CHECK(parseYaml("x: 0x2000\ny: 0xFF\nz: 255\n", &n, &err));
    long v = 0;
    CHECK(n.find("x")->asInt(&v, &err) && v == 0x2000);
    CHECK(n.find("y")->asInt(&v, &err) && v == 255);
    CHECK(n.find("z")->asInt(&v, &err) && v == 255);
}

TEST(test_yaml_block_and_flow_sequences) {
    YamlNode n;
    std::string err;
    CHECK(parseYaml("flow: [a, b, c]\nblock:\n  - one\n  - two\n", &n, &err));
    const YamlNode* f = n.find("flow");
    CHECK(f && f->isSeq() && f->seq.size() == 3 && f->seq[2].scalar == "c");
    const YamlNode* bl = n.find("block");
    CHECK(bl && bl->isSeq() && bl->seq.size() == 2 && bl->seq[0].scalar == "one");
}

TEST(test_yaml_flow_map_nested) {
    YamlNode n;
    std::string err;
    CHECK(parseYaml("bits: { A13: 1, A12: 0 }\n", &n, &err));
    const YamlNode* bits = n.find("bits");
    CHECK(bits && bits->isMap() && bits->map.size() == 2);
    long v = 0;
    CHECK(bits->find("A13")->asInt(&v, &err) && v == 1);
    CHECK(bits->find("A12")->asInt(&v, &err) && v == 0);
}

TEST(test_yaml_seq_of_maps_with_continuation_keys) {
    YamlNode n;
    std::string err;
    CHECK(parseYaml("groups:\n"
                    "  - all-of:\n"
                    "      - chip-select: Y0\n"
                    "    span: 0x800\n"
                    "    maps-to: 0\n",
                    &n, &err));
    const YamlNode* g = n.find("groups");
    CHECK(g && g->isSeq() && g->seq.size() == 1);
    const YamlNode& item = g->seq[0];
    CHECK(item.isMap() && item.has("all-of") && item.has("span") && item.has("maps-to"));
    CHECK(item.find("all-of")->isSeq() && item.find("all-of")->seq[0].find("chip-select"));
}

TEST(test_yaml_comments_and_blank_lines) {
    YamlNode n;
    std::string err;
    CHECK(parseYaml("# a comment\n\na: 1   # trailing\nb: \"x # y\"\n", &n, &err));
    CHECK(n.isMap() && n.map.size() == 2);
    std::string s;
    CHECK(n.find("b")->asString(&s, &err) && s == "x # y");  // '#' inside quotes kept
}

TEST(test_yaml_block_scalar) {
    YamlNode n;
    std::string err;
    CHECK(parseYaml("bytes: |\n  43 16\n  00 00\nnext: 1\n", &n, &err));
    CHECK(n.find("bytes")->scalar == "43 16\n00 00");
    long v = 0;
    CHECK(n.find("next")->asInt(&v, &err) && v == 1);
}

TEST(test_yaml_rejects_tabs_and_bad_indent) {
    YamlNode n;
    std::string err;
    CHECK(!parseYaml("a:\n\tb: 1\n", &n, &err) && !err.empty());
    err.clear();
    CHECK(!parseYaml("   a: 1\n", &n, &err) && !err.empty());
}

TEST(test_yaml_format_version_required_and_checked) {
    YamlNode n;
    std::string err;
    CHECK(parseYaml("format: x\nformat-version: 1\n", &n, &err));
    CHECK(requireFormatVersion(n, 1, "module", &err));
    CHECK(!requireFormatVersion(n, 2, "module", &err));
    CHECK(err.find("unsupported module format-version 1") != std::string::npos);
    YamlNode bare;
    CHECK(parseYaml("format: x\n", &bare, &err));
    err.clear();
    CHECK(!requireFormatVersion(bare, 1, "module", &err) && err == "missing 'format-version'");
}

// The options of a preset; the default reader is unchanged.
TEST(test_yaml_options_raw_step_verbs_and_repeated_top_level_keys) {
    YamlNode n;
    std::string err;
    YamlOptions o;
    o.rawStepVerbs = {"type", "expect"};
    o.repeatTopLevelKeys = true;
    CHECK(parseYaml("keys:\n  - type: \"a\" # b\n  - key: x # c\nkeys:\n  - expect: {1}\n", &n, &err, o));
    CHECK(n.map.size() == 2);
    const YamlNode& t = n.map[0].second.seq[0].map[0].second;
    CHECK(t.rawLine && t.scalar == "\"a\" # b");
    const YamlNode& k = n.map[0].second.seq[1].map[0].second;
    CHECK(!k.rawLine && k.scalar == "x");
    const YamlNode& e = n.map[1].second.seq[0].map[0].second;
    CHECK(e.isScalar() && e.scalar == "{1}");
    CHECK(!parseYaml("keys:\n  - type: a\nkeys:\n  - type: b\n", &n, &err));   // the default: a key once
}

TEST(test_number_forms) {
    CHECK(calcu::formats::parseNumber("&1F") == std::optional<std::int64_t>(31));
    CHECK(calcu::formats::parseNumber("0x1f") == std::optional<std::int64_t>(31));
    CHECK(calcu::formats::parseNumber("$10") == std::optional<std::int64_t>(16));
    CHECK(calcu::formats::parseNumber("-12") == std::optional<std::int64_t>(-12));
    CHECK(calcu::formats::parseNumber("12") == std::optional<std::int64_t>(12));
    CHECK(!calcu::formats::parseNumber("1F"));    // hex needs its mark
    CHECK(!calcu::formats::parseNumber("12 x"));  // the whole value
    CHECK(!calcu::formats::parseNumber(""));
    CHECK(!calcu::formats::parseNumber("&"));
    CHECK(!calcu::formats::parseNumber("99999999999999999999"));
}
