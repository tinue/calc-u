#pragma once
#include "settings/SettingsFile.hpp"
#include <string>
#include <string_view>

// The startup keys.

namespace calcu::settings {

// [startup] model when it names a model of the product; with `last` (also
// for a value that is neither) [startup] last-model; when that is
// not a model of the product either, the product's fallback model.
std::string startupModel(const SettingsFile& settings);
// [preset] <model>: the value that names the model's startup preset; empty
// means that no startup preset is set.
std::string startupPreset(const SettingsFile& settings, std::string_view model);

} // namespace calcu::settings
