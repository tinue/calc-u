#pragma once
#include <optional>
#include <string>

// Whole-file reads and writes for the settings file: a write goes to a file
// beside the target and is renamed into place, so the file holds either the
// old or the new content, never part of one.

namespace calcu::settings {

struct FileWrite { bool ok = false; std::string problem; };

// The file's bytes, or none when it does not exist or cannot be read.
std::optional<std::string> readWholeFile(const std::string& path);
bool fileExists(const std::string& path);
// Creates the parent folders, writes beside, renames into place.
FileWrite writeFileAtomically(const std::string& path, const std::string& bytes);

namespace testing {
// A test hook: while on, writeFileAtomically fails after writing the file
// beside and before the rename (an interrupted write).
void failBeforeRename(bool on);
} // namespace testing

} // namespace calcu::settings
