#include "shell/SettingsModel.hpp"
#include "files/FileCache.hpp"
#include "settings/Paths.hpp"
#include "shell/Inputs.hpp"
#include <algorithm>

namespace calcu::shell {

namespace {

using sessions::Value;
using Widget = DialogRow::Widget;

bool wanted(const std::vector<std::string>& keysShown, const settings::KeyDecl& d) {
    return keysShown.empty() ||
           std::find(keysShown.begin(), keysShown.end(), d.section + "." + d.key) != keysShown.end();
}

std::string capitalised(std::string s) {
    if (!s.empty() && s[0] >= 'a' && s[0] <= 'z') s[0] = static_cast<char>(s[0] - 'a' + 'A');
    for (auto& c : s)
        if (c == '-') c = ' ';
    return s;
}

const Value* rowOf(const Value& settings, const std::string& section, const std::string& key) {
    for (const auto& r : read::elements(settings, "rows"))
        if (read::text(r, "section") == section && read::text(r, "key") == key) return &r;
    return nullptr;
}

} // namespace

DialogSpec familySections(const settings::ProductDecl& decl, const std::vector<std::string>& keysShown) {
    DialogSection general{"General", {}}, presets{"Startup presets", {}}, storage{"Storage", {}},
        folders{"Folders", {}}, tracing{"Tracing", {}}, appLog{kAppLogSection, {}};
    for (const auto& d : decl.keys) {
        if (!wanted(keysShown, d)) continue;
        if (d.section == "startup" && d.key == "model")
            general.rows.push_back({Widget::StartupModel, d.section, d.key, "Startup model"});
        else if (d.section == "preset")
            presets.rows.push_back({Widget::StartupPreset, d.section, d.key, d.key});
        else if (d.section == "storage" && d.key == "save-folder")
            storage.rows.push_back({Widget::Folder, d.section, d.key, "Save folder"});
        else if (d.section == "storage" && d.key == "cassette-folder")
            storage.rows.push_back({Widget::Folder, d.section, d.key, "Cassettes"});
        else if (d.section == "folders")
            folders.rows.push_back({Widget::Folder, d.section, d.key, capitalised(d.key) + " folder"});
        else if (d.section == "trace" && d.key == "folder")
            tracing.rows.push_back({Widget::Folder, d.section, d.key, "Trace folder"});
        else if (d.section == "trace" && d.key == "max-file-size-mb")
            tracing.rows.push_back({Widget::Integer, d.section, d.key, "Maximum file size (MB)"});
        else if (d.section == "app-log" && d.key == "level") {
            DialogRow level{Widget::Choice, d.section, d.key, "App log level"};
            level.choices = {{"default", "Default"}, {"error", "Error"}, {"warn", "Warning"},
                             {"info", "Info"},       {"debug", "Debug"}};
            appLog.rows.push_back(std::move(level));
        }
    }
    DialogSpec spec;
    for (auto* s : {&general, &presets, &storage, &folders, &tracing, &appLog})
        if (!s->rows.empty()) spec.sections.push_back(std::move(*s));
    return spec;
}

std::vector<ShownSection> dialog(const Value& settings, const Value& presetList, const DialogSpec& spec) {
    std::vector<ShownSection> out;
    const std::string home = read::text(settings, "home");
    for (const auto& section : spec.sections) {
        ShownSection shown{section.title, {}};
        for (const auto& row : section.rows) {
            const Value* r = row.widget == Widget::Note ? nullptr : rowOf(settings, row.section, row.key);
            if (!r && row.widget != Widget::Note) continue;
            ShownRow s;
            s.row = row;
            if (r) {
                s.value = read::text(*r, "value");
                s.shown = s.value;
                s.resettable = !read::flag(*r, "isDefault", true);
                if (const Value* m = r->find("min"); m && m->integer()) s.min = *m->integer();
                if (const Value* m = r->find("max"); m && m->integer()) s.max = *m->integer();
            }
            switch (row.widget) {
            case Widget::StartupModel:
                for (const auto& c : read::elements(*r, "choices"))
                    if (c.string()) s.choices.emplace_back(*c.string(), *c.string() == "last" ? "Last used" : *c.string());
                break;
            case Widget::StartupPreset: {
                s.choices.emplace_back("", "None");
                bool listed = s.value.empty();
                for (const auto& e : read::elements(presetList, "entries")) {
                    if (read::text(e, "kind") != "startup" || read::text(e, "model") != row.key) continue;
                    const std::string ref = read::text(e, "bundlePath");
                    const std::string description = read::text(e, "description");
                    const std::string name = read::text(e, "name");
                    s.choices.emplace_back(ref, description.empty() ? name : name + " — " + description);
                    listed = listed || ref == s.value;
                }
                if (!listed) {   // a chosen file; one in the Web's file cache by its name
                    const std::string cache = read::text(presetList, "cacheFolder");
                    s.choices.emplace_back(s.value, files::liesIn(s.value, cache) ? files::plainFileName(s.value)
                                                                                 : settings::withTilde(s.value, home));
                }
                s.chooseLabel = kChooseLabel;
                break;
            }
            case Widget::Folder:
                s.shown = settings::withTilde(read::text(*r, "inUse"), home);
                break;
            case Widget::Choice:
                if (!row.choices.empty()) {   // exactly these; a value not among them as written
                    s.choices = row.choices;
                    bool listed = false;
                    for (const auto& c : row.choices) listed = listed || c.first == s.value;
                    if (!listed) s.choices.emplace_back(s.value, s.value);
                    break;
                }
                for (const auto& c : read::elements(*r, "choices"))
                    if (c.string()) s.choices.emplace_back(*c.string(), *c.string());
                break;
            case Widget::Integer:
            case Widget::Text:
            case Widget::Note:
                break;
            }
            shown.rows.push_back(std::move(s));
        }
        if (!shown.rows.empty()) out.push_back(std::move(shown));
    }
    const auto appLog = std::find_if(out.begin(), out.end(), [](const ShownSection& s) { return s.title == kAppLogSection; });
    if (appLog != out.end()) std::rotate(appLog, appLog + 1, out.end());
    return out;
}

} // namespace calcu::shell
