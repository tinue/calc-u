#pragma once
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>

// A fresh, empty folder of the tests (library/testing/):
// a unique name under the system's temporary folder, removed with all it
// holds when the object goes, also when a test made parts of it read-only.
// For the library's tests and both products'.

namespace calcu::testing {

class TempFolder {
public:
    explicit TempFolder(std::string_view tag = "calcu-test") {
        static std::atomic<int> n{0};
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        path_ = std::filesystem::temp_directory_path() /
                (std::string(tag) + "-" + std::to_string(stamp) + "-" + std::to_string(n++));
        std::filesystem::create_directories(path_);
    }
    ~TempFolder() {
        namespace fs = std::filesystem;
        std::error_code ec;
        // A test may have taken the write right away (a read-only drive).
        fs::permissions(path_, fs::perms::owner_all, fs::perm_options::add, ec);
        for (auto it = fs::recursive_directory_iterator(path_, ec); !ec && it != fs::recursive_directory_iterator();
             it.increment(ec))
            fs::permissions(it->path(), fs::perms::owner_all, fs::perm_options::add, ec);
        fs::remove_all(path_, ec);
    }
    TempFolder(const TempFolder&) = delete;
    TempFolder& operator=(const TempFolder&) = delete;

    const std::filesystem::path& path() const { return path_; }
    std::string str() const { return path_.generic_string(); }
    // The path of `name` in the folder, as a generic string.
    std::string operator/(std::string_view name) const { return (path_ / std::string(name)).generic_string(); }

    // Writes `bytes` to `name` (folders on the way made).
    void write(std::string_view name, std::string_view bytes) const {
        const std::filesystem::path p = path_ / std::string(name);
        std::filesystem::create_directories(p.parent_path());
        std::ofstream f(p, std::ios::binary | std::ios::trunc);
        f.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    }
    void remove(std::string_view name) const {
        std::error_code ec;
        std::filesystem::remove(path_ / std::string(name), ec);
    }
    // Copies every file of `dir` (not its subfolders) into the folder.
    void copyFrom(const std::filesystem::path& dir) const {
        std::error_code ec;
        for (const auto& e : std::filesystem::directory_iterator(dir, ec))
            if (e.is_regular_file(ec))
                std::filesystem::copy_file(e.path(), path_ / e.path().filename(),
                                           std::filesystem::copy_options::overwrite_existing, ec);
    }

private:
    std::filesystem::path path_;
};

} // namespace calcu::testing
