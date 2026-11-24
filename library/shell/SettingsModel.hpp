#pragma once
#include "sessions/Value.hpp"
#include "settings/Declaration.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// The settings dialog's sections and rows: what the frontend shows, in order;
// the values come from the `settings` topic (SettingsPart), the startup
// presets from the `preset-list` reply. The family rows' labels are fixed
// here; the product labels its own.

namespace calcu::shell {

struct DialogRow {
    enum class Widget { StartupModel, StartupPreset, Folder, Integer, Choice, Text, Note };
    Widget widget = Widget::Text;
    std::string section, key, label;
    // Choice: the values offered, with their labels, in order; empty:
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

// General, Startup presets, Storage, Folders, Tracing, App log for the family
// keys the declaration has and this frontend shows ("section.key"; empty:
// every family key it declares). The product appends its own sections.
DialogSpec familySections(const settings::ProductDecl&, const std::vector<std::string>& keysShown = {});

// The App log section moved last, behind the sections the product appended:
// every dialog ends with it.
void appLogSectionLast(DialogSpec&);

struct ShownRow {   // what a widget shows
    DialogRow row;
    std::string value;   // the setting's value
    std::string shown;   // a folder in use with `~` for home; else the value
    std::vector<std::pair<std::string, std::string>> choices;   // value, label
    // StartupPreset: the label of the last entry, which is no value but
    // opens the platform's file chooser (then choose-startup-preset).
    std::string chooseLabel;
    bool resettable = false;   // differs from its default
    std::optional<std::int64_t> min, max;
};
struct ShownSection {
    std::string title;
    std::vector<ShownRow> rows;
};

inline constexpr const char* kSettingsTitle = "Settings";
inline constexpr const char* kChooseStartupPreset = "Choose…";

// The sections with the rows the `settings` topic has; a section without a
// row is left out.
std::vector<ShownSection> dialog(const sessions::Value& settings, const sessions::Value& presetList, const DialogSpec&);

} // namespace calcu::shell
