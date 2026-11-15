#pragma once
#include "media/Kind.hpp"
#include <functional>
#include <string>
#include <string_view>

// Names of media (LIB-7 design §3.2; spec AC-2, AC-3, AC-4; media.md MD-2,
// MD-3, MD-5). Names are case preserving; two names that differ only in the
// case of ASCII letters are one name (D-3), and file names are compared with
// the same fold (D-4).

namespace calcu::media {

struct NameCheck {
    std::string name;      // trimmed, runs of spaces made one
    std::string refusal;   // empty: accepted
};
// AC-3: spaces around dropped and runs of spaces one; refused when empty or
// holding / \ : * ? " < > | or a control character (U+0000-U+001F, U+007F).
NameCheck checkName(std::string_view raw);
std::string foldName(std::string_view);                 // ASCII letters lower case (D-3)
bool sameName(std::string_view, std::string_view);     // AC-3
std::string fileNameFor(const std::string& name, const KindDecl&);   // name + suffix (AC-4)
// The first of foo-1, foo-2, … (+ suffix) for which `taken` is false (AC-4).
std::string numberedFileName(const std::string& name, const KindDecl&,
                             const std::function<bool(const std::string& fileName)>& taken);
// Whether the file name ends in the kind's suffix (any case), and the file
// name without it.
bool hasSuffix(std::string_view fileName, const KindDecl&);
std::string stemOf(std::string_view fileName, const KindDecl&);
// AC-2: the inner name, else the file name without the suffix.
std::string nameOf(const MediumInfo&, std::string_view fileName, const KindDecl&);
// The last component of a path ("/" or "\").
std::string baseName(std::string_view path);

} // namespace calcu::media
