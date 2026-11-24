#pragma once
#include "settings/Declaration.hpp"
#include "settings/SettingsFile.hpp"
#include <optional>
#include <string_view>
#include <vector>

// A desktop window's place in [window] as plain numbers: its left and top
// edge (empty until the app writes them: the window system places the
// window) and its size. One prefix per window: "" the main window, "paper-"
// the paper's own window. A value that is not a number counts as not set.

namespace calcu::settings {

struct WindowPlace {
    std::optional<long> x, y;
    long width = 0, height = 0;
    bool operator==(const WindowPlace&) const = default;
};

namespace family {
// [window] <prefix>x, <prefix>y (no default), <prefix>width, <prefix>height (app).
std::vector<KeyDecl> windowPlace(std::string_view prefix, long width, long height);
// [window] <key> = false (app): a window's flag, such as paper-own-window.
KeyDecl windowFlag(std::string_view key, std::string_view help);
// [window] <key> (app): a number without a default, empty until the app writes it.
KeyDecl windowOptional(std::string_view key, std::string_view help);
// [window] paper-own-window and the paper window's place (prefix "paper-"),
// written by every desktop window that gives the paper a window of its own.
std::vector<KeyDecl> paperWindow();
} // namespace family

// The place as declared: a size that is not set has its default.
WindowPlace windowPlace(const SettingsFile&, std::string_view prefix);
SettingsFile::WriteResult setWindowPlace(SettingsFile&, std::string_view prefix, const WindowPlace&);

// One [window] number: nullopt when it is empty or not a number.
std::optional<long> windowNumber(const SettingsFile&, std::string_view key);
SettingsFile::WriteResult setWindowNumber(SettingsFile&, std::string_view key, std::optional<long>);

// [window] paper-own-window: the paper panel in a window of its own
// (desktop), its place under the prefix "paper-".
inline constexpr std::string_view kPaperOwnWindowKey = "paper-own-window";
inline constexpr std::string_view kPaperWindowPrefix = "paper-";
inline constexpr long kPaperWindowWidth = 400, kPaperWindowHeight = 640;

} // namespace calcu::settings
