#include "media/FileAccess.hpp"
#include "settings/AtomicFile.hpp"
#include <filesystem>

namespace calcu::media {

namespace fs = std::filesystem;

namespace {
fs::path pathOf(const std::string& s) { return fs::path(std::u8string(s.begin(), s.end())); }
} // namespace

std::vector<std::string> FileAccess::list(const std::string& folder) const {
    std::vector<std::string> out;
    if (folder.empty()) return out;
    std::error_code ec;
    for (auto it = fs::directory_iterator(pathOf(folder), ec); !ec && it != fs::directory_iterator(); it.increment(ec)) {
        if (!it->is_regular_file(ec)) continue;
        const std::u8string u = it->path().filename().generic_u8string();
        out.emplace_back(u.begin(), u.end());
    }
    return out;
}

FileAccess::Read FileAccess::read(const std::string& path) const {
    if (!settings::fileExists(path)) return {};
    auto bytes = settings::readWholeFile(path);
    if (!bytes) return {std::nullopt, true};
    return {std::move(bytes), false};
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
