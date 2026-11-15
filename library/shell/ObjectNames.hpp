#pragma once
#include <string>

// The stable names of the shell's widgets and elements (LIB-14 design
// §3.10, D-21): Qt `objectName`, Web `data-calcu`. The scenarios of AC-25
// and Calc-U-1600's shot scenarios (M-49, AC-44) find widgets by them.

namespace calcu::shell::names {

inline const char* const kMenuBar = "menu-bar";
inline std::string menu(const std::string& id) { return "menu-" + id; }   // menu-file, menu-edit, menu-machine, menu-help
// item-load-preset, item-settings, item-reset, item-cold-boot, item-resume,
// item-help, item-about, item-model-<model>, item-peripheral-<place>-<peripheral>
inline std::string item(const std::string& id) { return "item-" + id; }
inline const char* const kControlBar = "control-bar";
inline const char* const kBarModel = "bar-model";
inline std::string barSlot(const std::string& place) { return "bar-slot-" + place; }
inline std::string barPlace(const std::string& place) { return "bar-place-" + place; }
inline std::string barMedia(const std::string& mediaPlace) { return "bar-media-" + mediaPlace; }
inline const char* const kSettingsDialog = "settings-dialog";
inline std::string setting(const std::string& section, const std::string& key) { return "setting-" + section + "-" + key; }
inline const char* const kSettingsClose = "settings-close";
inline const char* const kLoading = "loading";
inline const char* const kNotice = "notice";
inline const char* const kNoticeOk = "notice-ok";
inline const char* const kNoticeYes = "notice-yes";
inline const char* const kNoticeNo = "notice-no";
inline const char* const kAboutDialog = "about-dialog";
inline const char* const kPanelArea = "panel-area";
inline const char* const kRomsMissing = "roms-missing";

} // namespace calcu::shell::names
