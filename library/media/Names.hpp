#pragma once
#include "media/Kind.hpp"
#include <functional>
#include <string>
#include <string_view>

// Names of media (docs/formats/media.md). Names are case preserving; two
// names that differ only in the case of ASCII letters are one name (other
// letters are compared as written: no Unicode table in the library), and file
// names are compared with the same fold, so that "the file the name gives" is
// found alike on every file system.

namespace calcu::media {

struct NameCheck {
    std::string name;      // trimmed, runs of spaces made one
    std::string refusal;   // empty: accepted
};
// Spaces around dropped and runs of spaces one; refused when empty or
// holding / \ : * ? " < > | or a control character (U+0000-U+001F, U+007F).
NameCheck checkName(std::string_view raw);
std::string foldName(std::string_view);                 // ASCII letters lower case
bool sameName(std::string_view, std::string_view);
std::string fileNameFor(const std::string& name, const KindDecl&);   // name + suffix
// The first of foo-1, foo-2, … (+ suffix) for which `taken` is false.
std::string numberedFileName(const std::string& name, const KindDecl&,
                             const std::function<bool(const std::string& fileName)>& taken);
// Whether the file name ends in the kind's suffix (any case), and the file
// name without it.
bool hasSuffix(std::string_view fileName, const KindDecl&);
std::string stemOf(std::string_view fileName, const KindDecl&);
// The inner name, else the file name without the suffix.
std::string nameOf(const MediumInfo&, std::string_view fileName, const KindDecl&);
// The last component of a path ("/" or "\").
std::string baseName(std::string_view path);

} // namespace calcu::media
