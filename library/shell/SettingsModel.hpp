#pragma once
#include "sessions/Value.hpp"
#include "settings/Declaration.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// The settings dialog's sections and rows (LIB-14 design §3.5, rules S1 to
// S7, D-20; spec AC-11 to AC-13, EC-3): what the frontend shows, in order;
// the values come from the `settings` topic (SettingsPart), the startup
// presets from the `preset-list` reply. The family rows' labels are fixed
// here (and in shellModel.js); the product labels its own.

namespace calcu::shell {

struct DialogRow {
    enum class Widget { StartupModel, StartupPreset, Folder, Integer, Choice, Text, Note };
    Widget widget = Widget::Text;
    std::string section, key, label;
    // Choice: the values offered, with their labels, in order (S8); empty:
    // every declared value as written.
    std::vector<std::pair<std::string, std::string>> choices = {};
};
struct DialogSection {
    std::string title;
    std::vector<DialogRow> rows;
};
struct DialogSpec {   // the frontend's, in order
    std::vector<DialogSection> sections;
};

// S1: General, Startup presets, Storage, Folders, Tracing for the family
// keys the declaration has and this frontend shows ("section.key"; empty:
// every family key it declares). The product appends its own sections.
DialogSpec familySections(const settings::ProductDecl&, const std::vector<std::string>& keysShown = {});

struct ShownRow {   // what a widget shows
    DialogRow row;
    std::string value;   // the setting's value
    std::string shown;   // a folder in use with `~` for home (AC-13); else the value
    std::vector<std::pair<std::string, std::string>> choices;   // value, label
    bool resettable = false;   // AC-13: differs from its default
    std::optional<std::int64_t> min, max;
};
struct ShownSection {
    std::string title;
    std::vector<ShownRow> rows;
};

inline constexpr const char* kSettingsTitle = "Settings";   // S1

// The sections with the rows the `settings` topic has; a section without a
// row is left out (S1).
std::vector<ShownSection> dialog(const sessions::Value& settings, const sessions::Value& presetList, const DialogSpec&);

// `path` with the home folder written as `~` (AC-13).
std::string withTilde(const std::string& path, const std::string& home);

} // namespace calcu::shell
