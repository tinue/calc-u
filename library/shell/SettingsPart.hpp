#pragma once
#include "sessions/SessionPart.hpp"
#include "settings/HostDirs.hpp"
#include "settings/Paths.hpp"
#include "settings/SettingsFile.hpp"
#include <functional>
#include <optional>
#include <string>
#include <string_view>

// The settings in the session (LIB-14 design §3.5, D-3, D-19; spec AC-11,
// AC-13, AC-14, EC-3): the state topic `settings` and the commands
// `set-setting` and `remember-folder`, so that the dialog of every frontend
// reads and writes the product's settings file through the session (the
// Web's file lives in the worker).
//
//   settings        {home, rows: [{section, key, kind, value, default,
//                   choices, min?, max?, inUse, isDefault}]} for every
//                   declared key but [window] and [startup] last-model;
//                   `kind`: text, path, boolean, integer, choice,
//                   model-or-last, model-name, folder ([folders] keys)
//   set-setting     {section, key, value | null}: null is the default; a
//                   value the key does not accept is refused with the
//                   reason; a file that cannot be written gives a notice
//                   with LIB-9's problem and keeps the value for this run
//                   (D-19). Never held back while an operation runs (AC-14).
//                   A [folders] key sets its fixed folder (LIB-9 D-2).
//   remember-folder {kind, folder}: the folder a file dialog of that kind
//                   was last used in (LIB-9 AC-29)

namespace calcu::shell {

inline const sessions::TopicName kSettingsTopic{"settings"};

struct SettingsPartSetup {
    settings::SettingsFile* file = nullptr;     // the product's, on the session's thread (LIB-9: one owner)
    const settings::Paths* paths = nullptr;
    settings::FrontendKind frontend = settings::FrontendKind::Qt6;
    // A product folder in use (Calc-U-1600's host drive folder): section,
    // key → the folder in use; nullopt: the library's rule.
    std::function<std::optional<std::string>(std::string_view, std::string_view)> inUse;
    // After a change: the product applies it.
    std::function<void(std::string_view section, std::string_view key)> changed;
};

class SettingsPart final : public sessions::SessionPart {
public:
    explicit SettingsPart(SettingsPartSetup);
    void attach(sessions::SessionSetup&) override;
    // S6: a change from elsewhere (the product writes `last-model`) shows.
    void beforeFlush(sessions::SessionContext&) override;

    sessions::Value state() const;   // the topic's Value now

private:
    sessions::CommandOutcome set(const sessions::Value& args, sessions::SessionContext&);
    std::string inUse(const settings::KeyDecl&) const;
    SettingsPartSetup setup_;
    sessions::StateTopic* topic_ = nullptr;
};

} // namespace calcu::shell
