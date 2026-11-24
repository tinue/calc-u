#pragma once
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// The product's bundled files. A Bundle
// knows no ROM name: only lookUp() asks it for files, by exact name. A
// frontend makes the Bundle of its packaging (a folder, Qt resources, the
// Emscripten file system) and hands it to its product's lookUpRoms().

namespace calcu::roms {

class Bundle {
public:
    virtual ~Bundle() = default;
    // The bytes of the file of exactly that name; std::nullopt when there is none.
    virtual std::optional<std::vector<std::uint8_t>> read(std::string_view fileName) const = 0;
    // Why read() gave none for a file of that name that is there (no
    // access, a folder, a read error), in the system's words; empty when
    // there is no such file, and for a bundle that cannot tell.
    virtual std::string problem(std::string_view) const { return {}; }
    // Where the files are looked for, for the message (a folder, "the app bundle").
    virtual std::string where() const = 0;
};

// One folder, no subfolders. read() lists the folder and compares the names
// byte for byte, so on a case-insensitive file system "ce-150.rom" is not
// found for "CE-150.ROM".
class DirectoryBundle final : public Bundle {
public:
    explicit DirectoryBundle(std::filesystem::path folder);
    std::optional<std::vector<std::uint8_t>> read(std::string_view fileName) const override;
    std::string problem(std::string_view fileName) const override;
    std::string where() const override;

private:
    std::filesystem::path folder_;
};

} // namespace calcu::roms
