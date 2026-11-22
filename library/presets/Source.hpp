#pragma once
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

// Where a preset comes from: a file in a folder on disk (or one uploaded
// with its files), a preset of the
// product's bundle (a bundle path `examples:<path>`), or content handed over
// without a folder (iOS, the Web). The source reads the preset's text and the
// files it names.

namespace calcu::presets {

// The product's bundled presets: a folder in the app, the
// Qt resources unpacked, Emscripten's preloaded tree.
class PresetBundle {
public:
    virtual ~PresetBundle() = default;
    // The bytes of the file at `pathInBundle` (relative, `/`), or nullopt.
    virtual std::optional<std::vector<std::uint8_t>> read(std::string_view pathInBundle) const = 0;
    virtual std::string where() const = 0;   // for messages
};

class DirectoryPresetBundle final : public PresetBundle {
public:
    explicit DirectoryPresetBundle(std::filesystem::path root) : root_(std::move(root)) {}
    std::optional<std::vector<std::uint8_t>> read(std::string_view pathInBundle) const override;
    std::string where() const override;

private:
    std::filesystem::path root_;
};

class PresetSource {
public:
    virtual ~PresetSource() = default;
    virtual std::string name() const = 0;   // for messages: the path, the bundle path or the given name
    virtual std::string stem() const = 0;   // the preset-name default
    // The text, a leading byte-order mark dropped (editors on Windows write
    // one); nullopt: cannot be read, or not UTF-8 (readError() says which).
    virtual std::optional<std::string> text() const = 0;
    virtual std::string readError() const = 0;
    // A file the preset names: relative to the preset's folder, `~/` the
    // home folder (README §3). The bytes, or the sentence why not (it cannot
    // be read; a relative path without a folder).
    virtual std::variant<std::vector<std::uint8_t>, std::string> readNamed(std::string_view path) const = 0;
    virtual std::string resolvedName(std::string_view path) const = 0;   // what a section's owner shows or keeps
};

std::unique_ptr<PresetSource> folderSource(std::filesystem::path file, std::string home);
// A preset chosen with its files where there is no folder to look in (the
// Web): they lie together in one upload folder, a file it names is looked up
// there by its name alone, and one not chosen with it is refused so.
std::unique_ptr<PresetSource> uploadSource(std::filesystem::path file, std::string home);
std::unique_ptr<PresetSource> contentSource(std::string text, std::string name, std::string home);   // without a folder: no relative path
std::unique_ptr<PresetSource> bundleSource(const PresetBundle&, std::string pathInBundle);

// A path inside the bundle, `..` resolved; nullopt when it leaves the bundle
// (bundle.md §2) or is absolute.
std::optional<std::string> normalBundlePath(std::string_view path);
// A bundle path from the bundle (nullptr: none, refused when read), or
// a path on disk.
std::unique_ptr<PresetSource> sourceFor(std::string_view ref, const PresetBundle*, std::string home);

// Whether the bytes are UTF-8.
bool isUtf8(std::string_view);
// The stem of a file name: without its folder and its last extension.
std::string stemOf(std::string_view path);

} // namespace calcu::presets
