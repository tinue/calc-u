#include "presets/Source.hpp"
#include "files/Files.hpp"
#include <fstream>
#include <iterator>
#include <system_error>

namespace calcu::presets {

namespace fs = std::filesystem;

namespace {


// The bytes, or none with the system's reason in `why`.
std::optional<std::vector<std::uint8_t>> readFile(const fs::path& p, std::string& why) {
    std::error_code ec;
    const fs::file_status st = fs::status(p, ec);
    if (ec || !fs::exists(st)) {
        why = (ec ? ec : std::make_error_code(std::errc::no_such_file_or_directory)).message();
        return std::nullopt;
    }
    if (!fs::is_regular_file(st)) {
        why = "it is not a file";
        return std::nullopt;
    }
    return files::readBytes(p, &why);
}

// `~/x` against the home folder; nullopt when the path does not start so.
std::optional<fs::path> homePath(std::string_view path, const std::string& home) {
    if (path.size() < 2 || path[0] != '~' || (path[1] != '/' && path[1] != '\\')) return std::nullopt;
    return files::toPath(home) / files::toPath(path.substr(2));
}

// The text of the bytes: BOM dropped, UTF-8 checked.
std::optional<std::string> decoded(const std::vector<std::uint8_t>& bytes, const std::string& name, std::string* why) {
    std::string t(bytes.begin(), bytes.end());
    if (t.size() >= 3 && static_cast<unsigned char>(t[0]) == 0xEF && static_cast<unsigned char>(t[1]) == 0xBB &&
        static_cast<unsigned char>(t[2]) == 0xBF)
        t.erase(0, 3);
    if (!isUtf8(t)) {
        *why = "The file " + name + " is not UTF-8 text.";
        return std::nullopt;
    }
    return t;
}

std::string notRead(std::string_view path, const std::string& why) {
    return "The file " + std::string(path) + " cannot be read: " + why + ".";
}

// A preset in a folder on disk. An upload (the Web) is a preset chosen with
// its files, all of them put into one folder by their names alone: a file it
// names is looked up there by its name, and one not chosen with it is
// refused with the advice to choose it too.
class FolderSource final : public PresetSource {
public:
    FolderSource(fs::path file, std::string home, bool upload)
        : file_(std::move(file)), home_(std::move(home)), upload_(upload) {}
    std::string name() const override { return upload_ ? files::fromPath(file_.filename()) : files::fromPath(file_); }
    std::string stem() const override { return stemOf(files::fromPath(file_.filename())); }
    std::optional<std::string> text() const override {
        std::string why;
        const auto bytes = readFile(file_, why);
        if (!bytes) {
            why_ = notRead(name(), why);
            return std::nullopt;
        }
        return decoded(*bytes, name(), &why_);
    }
    std::string readError() const override { return why_; }
    std::variant<std::vector<std::uint8_t>, std::string> readNamed(std::string_view path) const override {
        const fs::path p = resolve(path);
        std::string why;
        if (auto b = readFile(p, why)) return *b;
        if (upload_ && chosenWith(path))
            return "The preset " + name() + " names the file " + std::string(path) +
                   ", which was not chosen with it: choose the preset together with every file it names.";
        return notRead(files::fromPath(p), why);
    }
    std::string resolvedName(std::string_view path) const override { return files::fromPath(resolve(path)); }

private:
    bool chosenWith(std::string_view path) const {
        return !homePath(path, home_) && !files::toPath(path).is_absolute();
    }
    fs::path resolve(std::string_view path) const {
        if (auto h = homePath(path, home_)) return *h;
        const fs::path p = files::toPath(path);
        if (p.is_absolute()) return p;
        return file_.parent_path() / (upload_ ? p.filename() : p);
    }
    fs::path file_;
    std::string home_;
    bool upload_;
    mutable std::string why_;
};

class ContentSource final : public PresetSource {
public:
    ContentSource(std::string text, std::string name, std::string home)
        : text_(std::move(text)), name_(std::move(name)), home_(std::move(home)) {}
    std::string name() const override { return name_; }
    std::string stem() const override { return stemOf(name_); }
    std::optional<std::string> text() const override {
        return decoded(std::vector<std::uint8_t>(text_.begin(), text_.end()), name_, &why_);
    }
    std::string readError() const override { return why_; }
    std::variant<std::vector<std::uint8_t>, std::string> readNamed(std::string_view path) const override {
        std::optional<fs::path> p = homePath(path, home_);
        if (!p && files::toPath(path).is_absolute()) p = files::toPath(path);
        if (!p)   // content without a folder
            return "The preset " + name_ + " was handed over without its folder, so the file " + std::string(path) +
                   " it names cannot be read.";
        std::string why;
        if (auto b = readFile(*p, why)) return *b;
        return notRead(files::fromPath(*p), why);
    }
    std::string resolvedName(std::string_view path) const override {
        if (auto h = homePath(path, home_)) return files::fromPath(*h);
        return std::string(path);
    }

private:
    std::string text_, name_, home_;
    mutable std::string why_;
};

class BundleSource final : public PresetSource {
public:
    BundleSource(const PresetBundle& b, std::string path) : bundle_(b), path_(std::move(path)) {}
    std::string name() const override { return "examples:" + path_; }
    std::string stem() const override { return stemOf(path_); }
    std::optional<std::string> text() const override {
        const auto bytes = bundle_.read(path_);
        if (!bytes) {
            why_ = "There is no bundled preset " + name() + ".";
            return std::nullopt;
        }
        return decoded(*bytes, name(), &why_);
    }
    std::string readError() const override { return why_; }
    std::variant<std::vector<std::uint8_t>, std::string> readNamed(std::string_view path) const override {
        const auto inBundle = inside(path);
        if (!inBundle) return "The file " + std::string(path) + " leaves the bundled presets.";
        if (auto b = bundle_.read(*inBundle)) return *b;
        return "There is no bundled file examples:" + *inBundle + ".";
    }
    std::string resolvedName(std::string_view path) const override {
        const auto inBundle = inside(path);
        return inBundle ? "examples:" + *inBundle : std::string(path);
    }

private:
    // A file a bundled preset names, relative to the preset.
    std::optional<std::string> inside(std::string_view path) const {
        const std::size_t slash = path_.rfind('/');
        const std::string folder = slash == std::string::npos ? std::string() : path_.substr(0, slash + 1);
        return normalBundlePath(folder + std::string(path));
    }
    const PresetBundle& bundle_;
    std::string path_;
    mutable std::string why_;
};

} // namespace

std::optional<std::vector<std::uint8_t>> DirectoryPresetBundle::read(std::string_view pathInBundle) const {
    const auto p = normalBundlePath(pathInBundle);
    if (!p) return std::nullopt;
    std::string unused;   // a bundled file that is not there is missing, whatever the reason
    return readFile(root_ / files::toPath(*p), unused);
}

std::string DirectoryPresetBundle::where() const { return files::fromPath(root_); }

std::unique_ptr<PresetSource> folderSource(fs::path file, std::string home) {
    return std::make_unique<FolderSource>(std::move(file), std::move(home), false);
}

std::unique_ptr<PresetSource> uploadSource(fs::path file, std::string home) {
    return std::make_unique<FolderSource>(std::move(file), std::move(home), true);
}

std::unique_ptr<PresetSource> contentSource(std::string text, std::string name, std::string home) {
    return std::make_unique<ContentSource>(std::move(text), std::move(name), std::move(home));
}

std::unique_ptr<PresetSource> bundleSource(const PresetBundle& b, std::string pathInBundle) {
    return std::make_unique<BundleSource>(b, std::move(pathInBundle));
}

namespace {

std::optional<std::string> parseBundlePath(std::string_view ref) {
    constexpr std::string_view kPrefix = "examples:";
    if (ref.substr(0, kPrefix.size()) != kPrefix) return std::nullopt;
    return normalBundlePath(ref.substr(kPrefix.size()));
}

} // namespace

std::optional<std::string> normalBundlePath(std::string_view path) {
    if (path.empty() || path.front() == '/' || path.front() == '\\' || path.find(':') != std::string_view::npos)
        return std::nullopt;
    std::vector<std::string> parts;
    std::string part;
    for (char c : std::string(path) + "/") {
        if (c != '/' && c != '\\') {
            part += c;
            continue;
        }
        if (part == "..") {
            if (parts.empty()) return std::nullopt;   // leaves the bundle
            parts.pop_back();
        } else if (!part.empty() && part != ".") {
            parts.push_back(part);
        }
        part.clear();
    }
    if (parts.empty()) return std::nullopt;
    std::string out;
    for (const auto& p : parts) out += (out.empty() ? "" : "/") + p;
    return out;
}

std::unique_ptr<PresetSource> sourceFor(std::string_view ref, const PresetBundle* bundle, std::string home) {
    if (ref.substr(0, 9) == "examples:") {
        const auto p = parseBundlePath(ref);
        if (bundle && p) return bundleSource(*bundle, *p);
        // No bundle, or a path that leaves it: a source that cannot be read.
        static const DirectoryPresetBundle none{fs::path()};
        return bundleSource(bundle ? *bundle : none, p ? *p : std::string(ref.substr(9)));
    }
    if (auto h = homePath(ref, home)) return folderSource(*h, std::move(home));
    return folderSource(files::toPath(ref), std::move(home));
}

bool isUtf8(std::string_view s) {
    std::size_t i = 0;
    while (i < s.size()) {
        const auto c = static_cast<unsigned char>(s[i]);
        int n = 0;
        if (c < 0x80) n = 0;
        else if ((c & 0xE0) == 0xC0 && c >= 0xC2) n = 1;
        else if ((c & 0xF0) == 0xE0) n = 2;
        else if ((c & 0xF8) == 0xF0 && c <= 0xF4) n = 3;
        else return false;
        for (int k = 1; k <= n; ++k) {
            if (i + static_cast<std::size_t>(k) >= s.size()) return false;
            if ((static_cast<unsigned char>(s[i + static_cast<std::size_t>(k)]) & 0xC0) != 0x80) return false;
        }
        i += static_cast<std::size_t>(n) + 1;
    }
    return true;
}

std::string stemOf(std::string_view path) {
    const std::size_t slash = path.find_last_of("/\\");
    std::string_view file = slash == std::string_view::npos ? path : path.substr(slash + 1);
    const std::size_t dot = file.rfind('.');
    if (dot != std::string_view::npos && dot > 0) file = file.substr(0, dot);
    return std::string(file);
}

} // namespace calcu::presets
