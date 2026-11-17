#pragma once
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

// ── Directory catalogues of named files ────────────────────────────────
//
// The mechanism behind the memory-card catalogue (`*.module.yaml`, keyed by
// `module-name` -- MemoryCardCatalog.hpp): scan a directory for files
// with one suffix and index them by the name each declares. Each format
// only supplies its suffix and a function reading the name.

namespace named_file_detail {

inline bool hasSuffix(const std::string& fileName, const std::string& suffix) {
    return fileName.size() > suffix.size() &&
           fileName.compare(fileName.size() - suffix.size(), suffix.size(), suffix) == 0;
}

inline bool readTextFile(const std::string& path, std::string* out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    std::stringstream ss;
    ss << in.rdbuf();
    *out = ss.str();
    return true;
}

}  // namespace named_file_detail

// Parses every "*<suffix>" file in `dir` (non-recursive) with
// `parse(text, path, Entry* out, std::string* err) -> bool`. A file that
// fails to parse is skipped, with "<filename>: <error>" appended to `*error`
// (newline-separated) -- one bad file never hides the rest. A missing /
// unreadable directory yields an empty list and sets `*error`. Entries come
// back sorted by `nameOf(entry)`. `kind` names the file type in errors.
// `skip`, if given, passes over a file by its name without reading it.
template <typename Entry, typename ParseFn, typename NameOf>
std::vector<Entry> scanNamedFiles(const std::string& dir, const std::string& suffix, const std::string& kind,
                                  ParseFn parse, NameOf nameOf, std::string* error,
                                  bool (*skip)(std::string_view) = nullptr) {
    std::vector<Entry> out;
    auto appendErr = [&](const std::string& msg) {
        if (!error) return;
        if (!error->empty()) *error += "\n";
        *error += msg;
    };

    std::error_code ec;
    std::filesystem::directory_iterator it(dir, ec);
    const std::filesystem::directory_iterator end;
    if (ec) {
        if (error) *error = "cannot read " + kind + " directory '" + dir + "': " + ec.message();
        return out;
    }
    for (; it != end; it.increment(ec)) {
        if (ec) {
            appendErr("directory walk stopped: " + ec.message());
            break;
        }
        const std::filesystem::path& p = it->path();
        // Double extensions (".module.yaml") -- path::extension() only sees
        // ".yaml", so match the filename tail directly.
        const std::string name = p.filename().string();
        if (!named_file_detail::hasSuffix(name, suffix)) continue;
        if (skip && skip(name)) continue;

        std::string text, parseErr;
        if (!named_file_detail::readTextFile(p.string(), &text)) {
            appendErr(name + ": cannot open");
            continue;
        }
        Entry entry;
        if (!parse(text, p.string(), &entry, &parseErr)) {
            appendErr(name + ": " + parseErr);
            continue;
        }
        out.push_back(std::move(entry));
    }
    std::sort(out.begin(), out.end(), [&](const Entry& a, const Entry& b) { return nameOf(a) < nameOf(b); });
    return out;
}
