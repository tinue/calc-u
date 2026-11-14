#include "TestHarness.hpp"
#include "settings/IniDocument.hpp"
#include <string>

// LIB-9 AC-33 (the grammar), AC-4 (lossless write-back); design §6, T-1, T-2.

using namespace calcu::settings;

namespace {
IniDocument parsed(std::string_view text) {
    ParseResult r = IniDocument::parse(text);
    CHECK(r.doc.has_value());
    return r.doc ? *r.doc : IniDocument{};
}
} // namespace

TEST(ac33_grammar_comments_both_line_endings) {
    const IniDocument doc = parsed(
        "# a comment\n"
        "; another comment\r\n"
        "top = 1\n"
        "\n"
        "[one]\r\n"
        "a = x\r\n"
        "  # an indented comment = not a key\n"
        "b = y\n"
        "[two]\n"
        "a = z");  // no ending on the last line
    CHECK(doc.get("", "top") == std::optional<std::string>("1"));
    CHECK(doc.get("one", "a") == std::optional<std::string>("x"));
    CHECK(doc.get("one", "b") == std::optional<std::string>("y"));
    CHECK(doc.get("two", "a") == std::optional<std::string>("z"));
    CHECK(!doc.get("one", "# an indented comment"));
    CHECK(!doc.get("", "a"));
    CHECK(!doc.get("three", "a"));
}

TEST(ac33_value_is_rest_of_line_trimmed) {
    const IniDocument doc = parsed(
        "[s]\n"
        "a =   spaced value \t \n"
        "b=c=d\n"
        "q = \"quoted\"\n"
        "e = back\\slash\\n\n"
        "empty =\n"
        "  indented   =  v  \n");
    CHECK(doc.get("s", "a") == std::optional<std::string>("spaced value"));
    CHECK(doc.get("s", "b") == std::optional<std::string>("c=d"));
    CHECK(doc.get("s", "q") == std::optional<std::string>("\"quoted\""));  // no quoting
    CHECK(doc.get("s", "e") == std::optional<std::string>("back\\slash\\n"));  // no escapes
    CHECK(doc.get("s", "empty") == std::optional<std::string>(""));
    CHECK(doc.get("s", "indented") == std::optional<std::string>("v"));
}

TEST(ac33_names_case_insensitive) {
    IniDocument doc = parsed("[Startup]\nModel = Model-A\n");
    CHECK(doc.get("startup", "model") == std::optional<std::string>("Model-A"));
    CHECK(doc.get("STARTUP", "MODEL") == std::optional<std::string>("Model-A"));
    // Values keep their case.
    CHECK(doc.get("startup", "model") != std::optional<std::string>("model-a"));
    // A change in another case changes the same key, and keeps its spelling.
    doc.set("startup", "model", "Model-B");
    CHECK_EQ(doc.text(), std::string("[Startup]\nModel = Model-B\n"));
}

TEST(ac33_last_duplicate_wins) {
    IniDocument doc = parsed("[s]\nk = 1\n[t]\nx = 0\n[S]\nK = 2\n");
    CHECK(doc.get("s", "k") == std::optional<std::string>("2"));
    // A change goes to the line that counts.
    doc.set("s", "k", "3");
    CHECK_EQ(doc.text(), std::string("[s]\nk = 1\n[t]\nx = 0\n[S]\nK = 3\n"));
    CHECK(doc.get("s", "k") == std::optional<std::string>("3"));
}

TEST(ac33_bom_or_bad_utf8_is_unreadable) {
    CHECK(!IniDocument::parse("\xEF\xBB\xBF[s]\nk = v\n").doc);
    const ParseResult bad = IniDocument::parse("[s]\nk = \xC3\x28\n");
    CHECK(!bad.doc);
    CHECK(!bad.problem.empty());
    CHECK(!IniDocument::parse("[s]\nk = \xED\xA0\x80\n").doc);  // a surrogate
    CHECK(!IniDocument::parse("[s]\nk = \xC0\xAF\n").doc);      // overlong
    // Good UTF-8 is read as it is.
    const ParseResult good = IniDocument::parse("[s]\nk = Zürich ✓\n");
    CHECK(good.doc && good.doc->get("s", "k") == std::optional<std::string>("Zürich ✓"));
    // A line that is neither a comment, a section nor a key (D-10).
    const ParseResult noKey = IniDocument::parse("[s]\nk = v\njust words\n");
    CHECK(!noKey.doc);
    CHECK_EQ(noKey.line, 3);
    CHECK(!IniDocument::parse("[s]\n= no key\n").doc);
    CHECK(!IniDocument::parse("[s\nk = v\n").doc);
}

TEST(ac4_unknown_keys_sections_comments_order_survive_set) {
    const std::string before =
        "# my notes\n"
        "format-version = 1\n"
        "\n"
        "[startup]\n"
        "# the model I use\n"
        "model = Model-A\n"
        "mine = kept\n"
        "\n"
        "[my-section]\n"
        "anything = goes here\n"
        "\n"
        "[test]\n"
        "flag = false\n";
    IniDocument doc = parsed(before);
    doc.set("startup", "model", "Model-B");     // a known key, changed in place
    doc.set("test", "style", "two");            // a new key in an existing section
    doc.set("window", "width", "640");          // a new section
    const std::string after =
        "# my notes\n"
        "format-version = 1\n"
        "\n"
        "[startup]\n"
        "# the model I use\n"
        "model = Model-B\n"
        "mine = kept\n"
        "\n"
        "[my-section]\n"
        "anything = goes here\n"
        "\n"
        "[test]\n"
        "flag = false\n"
        "style = two\n"
        "\n"
        "[window]\n"
        "width = 640\n";
    CHECK_EQ(doc.text(), after);
    // A new key goes after the section's last key, before its trailing blank lines.
    IniDocument doc2 = parsed("[a]\nx = 1\n\n[b]\ny = 2\n");
    doc2.set("a", "z", "3");
    CHECK_EQ(doc2.text(), std::string("[a]\nx = 1\nz = 3\n\n[b]\ny = 2\n"));
    // A new top-level key goes before the first section.
    IniDocument doc3 = parsed("# c\n[a]\nx = 1\n");
    doc3.set("", "format-version", "1");
    CHECK_EQ(doc3.text(), std::string("# c\nformat-version = 1\n[a]\nx = 1\n"));
}

TEST(ac4_unchanged_document_round_trips_bytes) {
    const std::string texts[] = {
        "",
        "[s]\nk = v\n",
        "[s]\r\nk = v\r\n",
        "  [ s ]  \n\tk\t=\tv\t\n; c\n\n\n",
        "[s]\nk = v",
        "# only a comment",
        "[s]\r\nk = v\nmixed = endings\r\n",
    };
    for (const std::string& t : texts) CHECK_EQ(parsed(t).text(), t);
    // An added line takes the file's line ending.
    IniDocument crlf = parsed("[s]\r\nk = v\r\n");
    crlf.set("s", "n", "w");
    CHECK_EQ(crlf.text(), std::string("[s]\r\nk = v\r\nn = w\r\n"));
    // A last line without an ending gets one before a line is added after it.
    IniDocument open = parsed("[s]\nk = v");
    open.set("s", "n", "w");
    CHECK_EQ(open.text(), std::string("[s]\nk = v\nn = w\n"));
}
