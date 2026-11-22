#pragma once
#include "sessions/Messages.hpp"
#include "sessions/Value.hpp"
#include "shell/Inputs.hpp"
#include "shell/MenuModel.hpp"
#include "shell/SettingsModel.hpp"
#include "shell/ShellProduct.hpp"
#include <optional>

// The shell's models kept over a session's messages, for a host that draws
// them somewhere else: the Web's worker feeds it every message it passes to
// the page and posts value() whenever it changed, so the page draws the
// menus, the bar, the panels and the settings dialog the C++ models make.

namespace calcu::shell {

struct ModelSetup {
    ShellProduct product;
    DialogSpec spec;
    Traits traits;
};

class ModelHost {
public:
    explicit ModelHost(InputNames = {});

    // The product, its dialog and the traits of the page: until they come,
    // value() is null.
    void set(ModelSetup);

    // A message the session sent; true when it changed an input of the models.
    bool take(const sessions::Message&);
    // The reply to the preset-list request.
    void takePresetList(const sessions::ReplyMessage&);
    // Whether the preset list is to be asked for: at the start and again
    // after each change of the settings (the startup presets they name).
    // Asking clears it.
    bool presetListWanted();
    const sessions::RequestName& presetListRequest() const { return names_.presetList; }

    // {menus, bar, panels, dialog} (ValueForms); null before set().
    sessions::Value value() const;

private:
    std::optional<ModelSetup> setup_;
    InputNames names_;
    ShellInputs inputs_;
    bool presetListWanted_ = true;
};

} // namespace calcu::shell
