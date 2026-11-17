#pragma once
#include <memory>
#include <optional>
#include <string>
#include <vector>

// How the media part reads and writes the save folder (LIB-7 design §11.2,
// D-30; C59-5 D-6): the shelf lists and reads through it, the writer writes
// through it. A host may hand its own (the Swift frontend of Calc-U-59: the
// coordinated access to its iCloud folder); the default is the plain one:
// std::filesystem and settings::writeFileAtomically.

namespace calcu::media {

class FileAccess {
public:
    virtual ~FileAccess() = default;
    // The names of the regular files in `folder` (no paths), in any order.
    virtual std::vector<std::string> list(const std::string& folder) const;
    struct Read {
        std::optional<std::string> bytes;   // nullopt: no such file, or it cannot be read
        bool unreadable = false;            // the file is there but cannot be read now
    };
    virtual Read read(const std::string& path) const;
    struct Written {
        bool ok = false;
        std::string problem;
    };
    // The whole file, atomically: it holds either the old or the new bytes.
    virtual Written write(const std::string& path, const std::string& bytes) const;
};

// The plain file access (one for the process).
std::shared_ptr<const FileAccess> plainFileAccess();

} // namespace calcu::media
