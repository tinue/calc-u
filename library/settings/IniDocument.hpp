#pragma once
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// The lossless INI document of a settings file. The grammar: UTF-8 without a
// byte order mark; LF or CRLF line endings; a line starting with `#` or `;`
// is a comment; `[section]` headers; `key = value` lines, the value being the
// rest of the line after the first `=`, trimmed, with no quoting and no
// escapes. Section and key names are matched without regard to ASCII case; of
// a key given twice the last one counts. Keys before the first section are
// the top level (section ""). Every line is kept as it was written, so an
// unchanged document gives back the same bytes and a change touches one line
// only.

namespace calcu::settings {

struct ParseResult;

class IniDocument {
public:
    // A BOM, bad UTF-8, or a non-blank, non-comment line that is neither
    // `[section]` nor holds `=` with a key before it, make the text
    // unreadable.
    static ParseResult parse(std::string_view bytes);

    // The value of the last `key` in `section` (in any of the section's
    // blocks), or none.
    std::optional<std::string> get(std::string_view section, std::string_view key) const;
    // Changes the line of the key that counts, in place; a new key goes
    // after the last key line of its section, a new section at the end.
    void set(std::string_view section, std::string_view key, std::string_view value);
    // The document as text: the same bytes as parsed when nothing changed.
    std::string text() const;

private:
    enum class Kind { Blank, Comment, Section, Key };
    struct Line {
        Kind kind = Kind::Blank;
        std::string raw;      // the line without its ending
        std::string ending;   // "\n", "\r\n" or "" (the last line)
        std::string section;  // lower case: the line's section ("" = top level)
        std::string key;      // lower case, Key lines only
        std::string value;    // trimmed, Key lines only
    };
    std::vector<Line> lines_;
    std::string newline_ = "\n";  // the ending added lines get: the file's first one

    void insert(std::size_t at, Line line);
};

struct ParseResult {
    std::optional<IniDocument> doc;  // empty when the text cannot be read as INI
    std::string problem;             // why not
    int line = 0;                    // the line of the problem (1-based), 0 for the whole text
};

} // namespace calcu::settings
