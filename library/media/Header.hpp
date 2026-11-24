#pragma once
#include "formats/Yaml.hpp"
#include <string>
#include <vector>

// The header every medium's file starts with (docs/formats/media.md): one
// writer for the lines in their documented order, and one reader. Standard
// library and the header-only YAML reader only, so a product's core may use
// it too.

namespace calcu::media {

struct Header {
    std::string comment;       // the leading comment line, without "# "
    std::string format;        // `format:`
    int formatVersion = 1;     // `format-version:`
    std::string nameKey;       // e.g. "card-name"; empty: the file has no name key
    std::string name;
    std::string description;   // written when not empty
    std::string insertNotice;  // `insert-notice:`, written when not empty
    bool isTemplate = false;   // `template: true` when set
    std::string savedAt;       // `saved:` when not empty (ISO-8601 UTC, isoTime)
};

// A YAML scalar as the writers quote it (docs/formats/README.md: no escape
// processing): double-quoted, or single-quoted when it holds a '"'.
std::string quotedScalar(const std::string&);

// The header's lines, each ending in '\n': the comment, `format`,
// `format-version`, the name key, `description`, `insert-notice`, `template`,
// `saved`.
std::string headerText(const Header&);

// What a reader expects of a header.
struct HeaderRules {
    std::string format;        // the `format:` the file must carry
    std::string what;          // the format in a problem ("floppy disk")
    long formatVersion = 1;    // the one version read
    std::string nameKey;       // required, not empty; empty: the file has no name key
    // The kind's own top-level keys; every key that is neither the header's
    // nor one of these is refused. Empty: the caller checks the keys itself
    // (the constant memory file, whose header has no `insert-notice` and no
    // `template`).
    std::vector<std::string> keys;
};

// Checks `format` and `format-version` of a parsed file and reads the name
// key, `description`, `insert-notice`, `template` and `saved` into `out` (no
// `comment`).
// False with `problem` set (no full stop) when the file is no mapping, has
// another format, another or no version, or no name; `out->formatVersion`
// is then the version the file carries (0: none), so a caller can tell a
// newer file. With `keys` given, the first other key is then the problem
// ("line N: unknown key K").
bool readHeader(const formats::YamlNode& root, const HeaderRules&, Header* out, std::string* problem);

} // namespace calcu::media
