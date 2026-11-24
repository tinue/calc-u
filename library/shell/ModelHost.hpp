#pragma once
#include "sessions/Messages.hpp"
#include "sessions/TopicMirror.hpp"
#include "sessions/Value.hpp"
#include "shell/Inputs.hpp"
#include "shell/MenuModel.hpp"
#include "shell/SettingsModel.hpp"
#include "shell/ShellProduct.hpp"
#include <optional>

// The shell's models kept over a session's messages, for a host that draws
// them somewhere else: the Web's worker feeds it every message it passes to
// the page and posts value() whenever it changed, so the page draws the
// menus, the bar, the panels, the settings dialog and About the C++ models make.

namespace calcu::shell {

struct ModelSetup {
    ShellProduct product;
    DialogSpec spec;
    Traits traits;
    SupportRows host;       // About's rows of where the page runs
    DisplayFacts display;   // About's Display section, as the page measured it
};

// The page's part of the setup, from its `shell` args: the build the page
// was stamped with (`version`, `build`, `buildTime`: the worker has none of
// its own; an empty one keeps the product's) and About's rows of where it
// runs (`host`: [[label, value]]) and of its screen (`display`: {windowing,
// screen, reported, panel, window: {width, height}, scale}; DisplayFacts).
void takePageArgs(ModelSetup&, const sessions::Value& args);

class ModelHost {
public:
    explicit ModelHost(InputNames = {});

    // The product, its dialog and the traits of the page: until they come,
    // value() is null. The topics the product's items follow are taken from
    // `states` as they are now: they may have come before the page's traits.
    void set(ModelSetup, const sessions::TopicMirror& states);

    // A message the session sent; true when it changed an input of the models
    // (the shell's topics and, once set(), those the product's items follow).
    bool take(const sessions::Message&);
    // The reply to the preset-list request.
    void takePresetList(const sessions::ReplyMessage&);
    // Whether the preset list is to be asked for: at the start and again
    // after each change of the settings (the startup presets they name).
    // Asking clears it.
    bool presetListWanted();
    const sessions::RequestName& presetListRequest() const { return names_.presetList; }

    // {menus, bar, panels, dialog, about} (ValueForms; about with the host
    // rows) and nameAndSave: {place: prompt}, the Name & Save dialog's
    // prompt (nameAndSavePrompt) for each place of the topic `media`; null
    // before set().
    sessions::Value value() const;

private:
    sessions::Value nameAndSavePrompts(const ShellProduct&) const;

    std::optional<ModelSetup> setup_;
    InputNames names_;
    ShellInputs inputs_;
    bool presetListWanted_ = true;
};

} // namespace calcu::shell
