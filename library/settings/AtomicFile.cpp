#include "settings/AtomicFile.hpp"
#include <atomic>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <random>

namespace calcu::settings {

namespace fs = std::filesystem;

namespace {
std::atomic<bool> gFailBeforeRename{false};

// The UTF-8 text of a path as a path on every host.
fs::path toPath(const std::string& s) { return fs::path(std::u8string(s.begin(), s.end())); }
} // namespace

void testing::failBeforeRename(bool on) { gFailBeforeRename = on; }

std::optional<std::string> readWholeFile(const std::string& path) {
    std::ifstream in(toPath(path), std::ios::binary);
    if (!in) return std::nullopt;
    std::string bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (in.bad()) return std::nullopt;
    return bytes;
}

bool fileExists(const std::string& path) {
    std::error_code ec;
    return fs::exists(toPath(path), ec);
}

FileWrite writeFileAtomically(const std::string& path, const std::string& bytes) {
    const fs::path target = toPath(path);
    std::error_code ec;
    if (target.has_parent_path()) {
        fs::create_directories(target.parent_path(), ec);
        if (ec) return {false, "cannot create the folder of " + path + ": " + ec.message()};
    }
    // Beside the target, under a name of its own, so that two writers do not meet.
    std::random_device rd;
    const fs::path beside = toPath(path + ".new-" + std::to_string(rd()));
    {
        std::ofstream out(beside, std::ios::binary | std::ios::trunc);
        if (!out) return {false, "cannot write beside " + path};
        out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        out.flush();
        if (!out) {
            out.close();
            fs::remove(beside, ec);
            return {false, "cannot write beside " + path};
        }
    }
    if (gFailBeforeRename) {
        fs::remove(beside, ec);
        return {false, "the write was interrupted before the rename (test hook)"};
    }
    fs::rename(beside, target, ec);
    if (ec) {
        std::error_code ignored;
        fs::remove(beside, ignored);
        return {false, "cannot replace " + path + ": " + ec.message()};
    }
    return {true, {}};
}

} // namespace calcu::settings
