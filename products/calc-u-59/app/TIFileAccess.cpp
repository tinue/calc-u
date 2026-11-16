#include "TIFileAccess.hpp"
#include "settings/AtomicFile.hpp"

namespace cu59 {

namespace {

class PlainFileIO final : public FileIO {
public:
    Read read(const std::string& path) override {
        if (!calcu::settings::fileExists(path)) return {};
        auto text = calcu::settings::readWholeFile(path);
        if (!text) return {std::nullopt, true};
        return {std::vector<uint8_t>(text->begin(), text->end())};
    }
    bool write(const std::string& path, const std::string& text) override {
        return calcu::settings::writeFileAtomically(path, text).ok;
    }
};

} // namespace

std::shared_ptr<FileIO> plainFileIO() { return std::make_shared<PlainFileIO>(); }

TIFileAccess::TIFileAccess(std::string saveFolder, std::shared_ptr<FileIO> io)
    : m_saveFolder(std::move(saveFolder)), m_io(io ? std::move(io) : plainFileIO()) {}

std::string TIFileAccess::savePath(const std::string& fileName) const {
    if (m_saveFolder.empty()) return fileName;
    const char last = m_saveFolder.back();
    return m_saveFolder + (last == '/' || last == '\\' ? "" : "/") + fileName;
}

std::optional<std::string> TIFileAccess::readConstantMemory(bool* unreadable) {
    auto r = m_io->read(savePath(kConstantMemoryFile));
    if (unreadable) *unreadable = r.unreadable;
    if (!r.raw) return std::nullopt;
    return std::string(r.raw->begin(), r.raw->end());
}

bool TIFileAccess::writeConstantMemory(const std::string& text) { return m_io->write(savePath(kConstantMemoryFile), text); }


} // namespace cu59
