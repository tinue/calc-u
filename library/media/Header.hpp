#pragma once
#include <string>

// The header every medium's file starts with (docs/formats/media.md; release
// simplification L-6): one writer for the lines in their documented order.
// Standard library only, so a product's core may write it too.

namespace calcu::media {

struct Header {
    std::string comment;       // the leading comment line, without "# "
    std::string format;        // `format:`
    int formatVersion = 1;     // `format-version:`
    std::string nameKey;       // e.g. "card-name"; empty: the file has no name key
    std::string name;
    std::string description;   // written when not empty
    bool isTemplate = false;   // `template: true` when set
    std::string savedAt;       // `saved:` when not empty (ISO-8601 UTC, isoTime)
};

// A YAML scalar as the writers quote it (docs/formats/README.md: no escape
// processing): double-quoted, or single-quoted when it holds a '"'.
std::string quotedScalar(const std::string&);

// The header's lines, each ending in '\n': the comment, `format`,
// `format-version`, the name key, `description`, `template`, `saved`.
std::string headerText(const Header&);

} // namespace calcu::media
