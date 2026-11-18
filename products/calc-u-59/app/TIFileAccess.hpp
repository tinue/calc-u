#pragma once
#include "media/FileAccess.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <string>

// The constant-memory file on the session's thread (LIB-3 59 design §5,
// adapter TIFileAccess; LIB-3 AC-30): what each frontend's 2.3.0's
// SessionController::Platform did, now in the save folder the host names
// (LIB-9's Paths), read at each use (LIB-14 AC-11, row 59-66), through the save folder's file access the host hands in
// (LIB-7's media::FileAccess, D-30; the media part's too): the plain one for
// Qt6, the Web and the tests, the Swift bridge's coordinated access for
// iCloud (C59-5 D-6).

namespace cu59 {

class TIFileAccess {
public:
    static constexpr const char* kConstantMemoryFile = "ti58c.memory.yaml";   // C59-3 AC-3

    /// `saveFolder`: the save folder as it is now; `files` null: the plain file access.
    TIFileAccess(std::function<std::string()> saveFolder, std::shared_ptr<const calcu::media::FileAccess> files);

    /// The file's text (nullopt: no file, or it cannot be read; `unreadable` says which).
    std::optional<std::string> readConstantMemory(bool* unreadable = nullptr);
    /// False when the write failed (C59-3 EC-4).
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
