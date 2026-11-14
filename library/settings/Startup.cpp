#include "settings/Startup.hpp"

namespace calcu::settings {

std::string startupModel(const SettingsFile& settings) {
    const ProductDecl& product = settings.product();
    const std::string model = settings.text("startup", "model");
    if (product.isModel(model)) return model;
    const std::string last = settings.text("startup", "last-model");
    if (product.isModel(last)) return last;
    return product.fallbackModel;
}

std::string startupPreset(const SettingsFile& settings, std::string_view model) {
    return settings.text("preset", model);
}

} // namespace calcu::settings
