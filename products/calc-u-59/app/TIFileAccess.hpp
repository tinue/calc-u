#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// The constant-memory file on the session's thread (LIB-3 59
// design §5, adapter TIFileAccess; LIB-3 AC-30): what each frontend's
// 2.3.0's SessionController::Platform did, now in the save folder the host names
// (LIB-9's Paths), through a FileIO the host hands in: plain file I/O for
// Qt6 and the tests, the Swift bridge's coordinated I/O for iCloud (C59-5
// D-6). The cards went to the library's media part in M-55 (C59-7, LIB-7
// 59; the host's FileIO is its file access, LIB-7 D-30).

namespace cu59 {

class FileIO {
public:
    virtual ~FileIO() = default;
    /// `raw` is nullopt if there is no such file; `unreadable`: the file is
    /// there but cannot be read now.
    struct Read {
        std::optional<std::vector<uint8_t>> raw;
        bool unreadable = false;
    };
    virtual Read read(const std::string& path) = 0;
    virtual bool write(const std::string& path, const std::string& text) = 0;
};

/// Plain file I/O (atomic writes, LIB-9's AtomicFile).
std::shared_ptr<FileIO> plainFileIO();

class TIFileAccess {
public:
    static constexpr const char* kConstantMemoryFile = "ti58c.memory.yaml";   // C59-3 AC-3

    TIFileAccess(std::string saveFolder, std::shared_ptr<FileIO> io);

    /// The file's text (nullopt: no file, or it cannot be read; `unreadable` says which).
    std::optional<std::string> readConstantMemory(bool* unreadable = nullptr);
    /// False when the write failed (C59-3 EC-4).
    bool writeConstantMemory(const std::string& text);
    /// The path of `fileName` in the save folder.
    std::string savePath(const std::string& fileName) const;
    const std::string& saveFolder() const { return m_saveFolder; }

private:
    std::string m_saveFolder;
    std::shared_ptr<FileIO> m_io;
};

} // namespace cu59
