#pragma once
#include "sessions/Messages.hpp"
#include "sessions/Value.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

// What the shell draws from (LIB-14 design §3.1, D-2): the topics and
// replies of other features in their Value form, so that the shell depends
// on no C++ type of LIB-4, LIB-6, LIB-7, LIB-10 or LIB-12, and the C++ and
// the JS models take the same input. Each is the last Value that came, Null
// while none did.

namespace calcu::shell {

struct ShellInputs {
    sessions::Value controlState;   // LIB-4 `control-state`
    sessions::Value busy;           // LIB-3 `busy`: {operationRuns, speed}
    sessions::Value presetList;     // LIB-6 `preset-list` reply: {entries: [...]}
    sessions::Value runState;       // LIB-10 `run-state`: {paused, ...}
    sessions::Value romsMissing;    // LIB-12 `roms-missing`: {text}
    sessions::Value settings;       // this feature's `settings` (SettingsPart)
    sessions::Value media;          // LIB-7 `media`: {kinds, places}
    // A control state came: AC-19 is the opposite.
    bool started() const { return !controlState.isNull(); }
};

// The owners' names (LIB-4, LIB-10, LIB-12, LIB-14, LIB-7, LIB-6; D-24).
struct InputNames {
    sessions::TopicName controlState{"control-state"}, busy{"busy"}, runState{"run-state"},
        romsMissing{"roms-missing"}, settings{"settings"}, media{"media"};
    sessions::RequestName presetList{"preset-list"};
};

// The command names the shell sends (D-24): LIB-18, LIB-6, LIB-10, LIB-5,
// LIB-7, LIB-3, this feature's SettingsPart.
namespace commands {
inline const sessions::CommandName kModel{"model"};
inline const sessions::CommandName kVariant{"variant"};
inline const sessions::CommandName kAttach{"attach"};
inline const sessions::CommandName kDetach{"detach"};
inline const sessions::CommandName kReset{"reset"};
inline const sessions::CommandName kColdBoot{"cold-boot"};
inline const sessions::CommandName kLoadPreset{"load-preset"};
inline const sessions::CommandName kResume{"resume"};
inline const sessions::CommandName kPaste{"paste"};
inline const sessions::CommandName kMediaImport{"media.import"};
inline const sessions::CommandName kScreenKeyDown{"screen-key-down"};
inline const sessions::CommandName kScreenKeyUp{"screen-key-up"};
inline const sessions::CommandName kSpeed{"speed"};
inline const sessions::CommandName kSetSetting{"set-setting"};
inline const sessions::CommandName kRememberFolder{"remember-folder"};
} // namespace commands

// Reading a Value without throwing: a member by key, or what a missing or
// mistyped one gives.
namespace read {
std::string text(const sessions::Value& v, std::string_view key);
bool flag(const sessions::Value& v, std::string_view key, bool fallback = false);
std::int64_t integer(const sessions::Value& v, std::string_view key, std::int64_t fallback = 0);
const sessions::Value::Array& elements(const sessions::Value& v, std::string_view key);
} // namespace read

// An entry's availability as LIB-4 writes it: `available` (true when
// missing) and `reason`.
struct Availability {
    bool now = true;
    std::string reason;
};
Availability availabilityOf(const sessions::Value& entry, std::string_view key = "available");

// The busy topic's two fields.
struct Busy {
    bool operationRuns = false;
    bool full = false;
};
Busy busyOf(const sessions::Value& busy);

} // namespace calcu::shell
