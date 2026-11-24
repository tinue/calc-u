#include "media/FileAccess.hpp"
#include "files/Files.hpp"
#include "settings/AtomicFile.hpp"
#include <filesystem>

namespace calcu::media {

namespace fs = std::filesystem;

std::vector<std::string> FileAccess::list(const std::string& folder, std::string* problem) const {
    std::vector<std::string> out;
    if (folder.empty()) return out;
    std::error_code ec;
    for (auto it = fs::directory_iterator(files::toPath(folder), ec); !ec && it != fs::directory_iterator(); it.increment(ec)) {
        if (!it->is_regular_file(ec)) continue;
        out.push_back(files::fromPath(it->path().filename()));
    }
    // A folder that is not there yet holds nothing; only another failure is one.
    if (ec && ec != std::errc::no_such_file_or_directory && problem) *problem = ec.message();
    return out;
}

FileAccess::Read FileAccess::read(const std::string& path) const {
    if (!settings::fileExists(path)) return {};
    std::string why;
    auto bytes = settings::readWholeFile(path, &why);
    if (!bytes) return {std::nullopt, true, std::move(why)};
    return {std::move(bytes), false, {}};
}

FileAccess::Written FileAccess::write(const std::string& path, const std::string& bytes) const {
    const auto w = settings::writeFileAtomically(path, bytes);
    return {w.ok, w.problem};
}

std::shared_ptr<const FileAccess> plainFileAccess() {
    static const std::shared_ptr<const FileAccess> plain = std::make_shared<FileAccess>();
    return plain;
}

} // namespace calcu::media
