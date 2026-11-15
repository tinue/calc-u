#include "media/Writer.hpp"
#include "settings/AtomicFile.hpp"
#include <chrono>
#include <cstdio>
#include <ctime>

namespace calcu::media {

std::string isoTime(std::chrono::system_clock::time_point t) {
    const std::time_t s = std::chrono::system_clock::to_time_t(t);
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &s);
#else
    gmtime_r(&s, &tm);
#endif
    char buf[32];
    std::snprintf(buf, sizeof buf, "%04d-%02d-%02dT%02d:%02d:%02dZ", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                  tm.tm_hour, tm.tm_min, tm.tm_sec);
    return buf;
}

WriteResult writeFiles(const KindDecl& k, const std::vector<FileOut>& files, const std::string& path, const Shelf& shelf) {
    WriteResult out;
    if (files.empty()) {
        out.problem = "Nothing to write.";
        return out;
    }
    // EC-1: the kind's folder is made when it is the target's.
    const std::string folder = shelf.folderOf(k);
    if (shelf.paths() && !folder.empty() && folderOfPath(path) == folder) {
        const auto ensured = shelf.paths()->ensure(k.folder);
        if (!ensured.ok) {
            out.problem = ensured.problem;
            return out;
        }
    }
    const std::string dir = folderOfPath(path);
    // Companions first: the medium's file, written last, names them.
    for (std::size_t i = files.size(); i-- > 1;) {
        const auto w = settings::writeFileAtomically(joinFile(dir, files[i].relativePath), files[i].bytes);
        if (!w.ok) {
            out.problem = w.problem;
            return out;
        }
    }
    const auto w = settings::writeFileAtomically(path, files.front().bytes);
    if (!w.ok) {
        out.problem = w.problem;
        return out;
    }
    out.ok = true;
    out.bytes = files.front().bytes;
    return out;
}

WriteResult writeMedium(const KindDecl& k, const MediumCodec::Write& w, const std::string& path, const Shelf& shelf) {
    WriteResult out;
    if (!k.codec) {
        out.problem = "The " + k.label + " cannot be written.";
        return out;
    }
    std::string problem;
    const std::vector<FileOut> files = k.codec->write(w, problem);
    if (files.empty()) {
        out.problem = problem.empty() ? "The " + k.label + " cannot be written." : problem;
        return out;
    }
    return writeFiles(k, files, path, shelf);
}

} // namespace calcu::media
