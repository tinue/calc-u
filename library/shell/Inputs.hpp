#pragma once
#include "sessions/Messages.hpp"
#include "sessions/Value.hpp"
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

// What the shell draws from: the topics and replies of other library
// areas in their Value form, so that the shell depends on no C++ type of
// control, presets, media, debugger or roms, and the C++ and
// the JS models take the same input. Each is the last Value that came, Null
// while none did.

namespace calcu::shell {

struct ShellInputs {
    sessions::Value controlState;   // control's `control-state`
    sessions::Value busy;           // sessions' `busy`: {operationRuns, speed}
    sessions::Value presetList;     // presets' `preset-list` reply: {entries: [...]}
    sessions::Value runState;       // debugger's `run-state`: {paused, ...}
    sessions::Value romsMissing;    // roms' `roms-missing`: {text}
    sessions::Value settings;       // the shell's `settings` (SettingsPart)
    sessions::Value media;          // media's `media`: {kinds, places}
    // The topics the product's items follow (shell::followProductTopics), by
    // name: an entry for each, so takeState knows which to keep.
    std::map<std::string, sessions::Value> productTopics;
    // A control state came. None comes when the product could not start:
    // then no control bar and no Machine menu, only the ROM message.
    bool started() const { return !controlState.isNull(); }
};

// The owners' names (control, debugger, roms, shell, media, presets).
struct InputNames {
    sessions::TopicName controlState{"control-state"}, busy{"busy"}, runState{"run-state"},
        romsMissing{"roms-missing"}, settings{"settings"}, media{"media"};
    sessions::RequestName presetList{"preset-list"};
};

// A state the session sent, into the inputs: a topic of `names` or one of
// `in.productTopics`. True when it changed an input; any other topic changes
// nothing.
bool takeState(ShellInputs& in, const InputNames& names, const sessions::StateMessage&);

// The command names the shell sends: machine control and startup, presets,
// debugger, typing, media, sessions, the shell's SettingsPart.
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
inline const sessions::CommandName kMediaChoose{"media.choose"};
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

// An entry's availability as the control state writes it: `available` (true when
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
