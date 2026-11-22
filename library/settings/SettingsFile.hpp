#pragma once
#include "settings/Declaration.hpp"
#include "settings/IniDocument.hpp"
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// A product's settings file. Read once, when the product starts; never read
// again while it runs. Each change is written through at once, atomically,
// and merged into the file on disk so that another instance's keys survive. A
// file that cannot be read, has no version or another version is left
// untouched for the whole run, and the product starts with the defaults. Not
// thread-safe: one thread owns it. The ProductDecl must outlive the
// SettingsFile.

namespace calcu::settings {

enum class Outcome {
    Read,          // the file was read
    Created,       // there was none; it was written with every key at its default (or its first value)
    Unreadable,    // not INI: defaults, nothing written
    NoVersion,     // no `format-version`: defaults, nothing written
    OtherVersion,  // another format version: defaults, nothing written
    DefaultsOnly,  // no file at all: nothing read or written
};

class SettingsFile {
public:
    // `firstValues` false: a new file takes every key's default, its first
    // value too (a test's or a demo page's fresh start, bare).
    static SettingsFile open(const ProductDecl& product, std::string path, bool firstValues = true);
    static SettingsFile defaultsOnly(const ProductDecl& product);

    Outcome outcome() const { return outcome_; }
    // Why the file was not read, or why creating it failed: for the
    // product's log or a notice ("says so"). Empty otherwise.
    const std::string& problem() const { return problem_; }
    const std::string& path() const { return path_; }
    const ProductDecl& product() const { return *product_; }
    // Whether changes reach the file in this run.
    bool writable() const { return writable_; }

    // The value as the product uses it: the declared default when the key
    // is missing or its value is not accepted. An undeclared
    // key gives its text, or empty.
    std::string text(std::string_view section, std::string_view key) const;
    bool boolean(std::string_view section, std::string_view key) const;
    long integer(std::string_view section, std::string_view key) const;
    // The text as it is in the file, accepted or not.
    std::optional<std::string> raw(std::string_view section, std::string_view key) const;

    struct WriteResult { bool ok = true; std::string problem; };
    // Changes the value and writes the file at once; nothing is written
    // when the value is unchanged. When the file is not writable in this
    // run, the change lasts until the product quits and the result says
    // that it was not written (ok = false).
    WriteResult set(std::string_view section, std::string_view key, std::string_view value);

    // The text of a new file: `format-version`, then every declared key at
    // its default in declaration order, a `# help` line above each.
    static std::string defaultText(const ProductDecl& product);

private:
    const ProductDecl* product_ = nullptr;
    std::string path_;
    Outcome outcome_ = Outcome::DefaultsOnly;
    std::string problem_;
    bool writable_ = false;
    IniDocument doc_;
    // The keys changed in this run that have not reached the file yet.
    std::vector<std::pair<std::string, std::string>> pending_;

    WriteResult writePending();
};

} // namespace calcu::settings
