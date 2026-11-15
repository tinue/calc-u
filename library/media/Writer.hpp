#pragma once
#include "media/Kind.hpp"
#include "media/Shelf.hpp"
#include <chrono>
#include <string>
#include <vector>

// Writing a medium (LIB-7 design §2, rule A7; spec EC-1, TR "a file that is
// written holds either the old or the new content"): the codec's files, each
// through settings::writeFileAtomically, the kind's folder made first.

namespace calcu::media {

struct WriteResult {
    bool ok = false;
    std::string problem;
    std::string bytes;   // the medium's file as written
};

// The codec's files for `w`: the first at `path`, the companions beside it.
WriteResult writeMedium(const KindDecl&, const MediumCodec::Write& w, const std::string& path, const Shelf&);
// Bytes already in the kind's format (an import: byte for byte), with their
// companions by relative path.
WriteResult writeFiles(const KindDecl&, const std::vector<FileOut>& files, const std::string& path, const Shelf&);
// ISO-8601 UTC, "2026-11-15T10:00:00Z".
std::string isoTime(std::chrono::system_clock::time_point);

} // namespace calcu::media
