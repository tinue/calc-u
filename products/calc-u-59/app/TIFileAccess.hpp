#pragma once
#include "media/FileAccess.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <string>

// The constant-memory file on the session's thread, in the save folder the
// host names (the library's Paths), read at each use (a save folder changed
// in Settings is used by the next save, with no restart), through the save
// folder's file access the host hands in (media::FileAccess; the media
// part's too): the plain one for Qt6, the Web and the tests, the Swift
// bridge's coordinated access for iCloud.

namespace cu59 {

class TIFileAccess {
public:
    static constexpr const char* kConstantMemoryFile = "ti58c.memory.yaml";

    /// `saveFolder`: the save folder as it is now; `files` null: the plain file access.
    TIFileAccess(std::function<std::string()> saveFolder, std::shared_ptr<const calcu::media::FileAccess> files);

    /// The file's text (nullopt: no file, or it cannot be read; `unreadable` says which).
    std::optional<std::string> readConstantMemory(bool* unreadable = nullptr);
    /// False when the write failed.
    bool writeConstantMemory(const std::string& text);
    /// The path of `fileName` in the save folder.
    std::string savePath(const std::string& fileName) const;
    std::string saveFolder() const { return m_saveFolder(); }
    /// The save folder's file access (never null).
    const std::shared_ptr<const calcu::media::FileAccess>& files() const { return m_files; }

private:
    std::function<std::string()> m_saveFolder;
    std::shared_ptr<const calcu::media::FileAccess> m_files;
};

} // namespace cu59
