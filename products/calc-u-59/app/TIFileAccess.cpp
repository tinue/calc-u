#include "TIFileAccess.hpp"

namespace cu59 {

TIFileAccess::TIFileAccess(std::function<std::string()> saveFolder, std::shared_ptr<const calcu::media::FileAccess> files)
    : m_saveFolder(std::move(saveFolder)), m_files(files ? std::move(files) : calcu::media::plainFileAccess()) {}

std::string TIFileAccess::savePath(const std::string& fileName) const {
    const std::string folder = m_saveFolder();
    if (folder.empty()) return fileName;
    const char last = folder.back();
    return folder + (last == '/' || last == '\\' ? "" : "/") + fileName;
}

std::optional<std::string> TIFileAccess::readConstantMemory(bool* unreadable) {
    calcu::media::FileAccess::Read r = m_files->read(savePath(kConstantMemoryFile));
    if (unreadable) *unreadable = r.unreadable;
    return std::move(r.bytes);
}

bool TIFileAccess::writeConstantMemory(const std::string& text) {
    return m_files->write(savePath(kConstantMemoryFile), text).ok;
}

} // namespace cu59
