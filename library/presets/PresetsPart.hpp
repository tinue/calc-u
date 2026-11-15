#pragma once
#include "machine/Catalogue.hpp"
#include "presets/BundleFile.hpp"
#include "presets/Engine.hpp"
#include "presets/ProductRules.hpp"
#include "presets/Reader.hpp"
#include "presets/Source.hpp"
#include "presets/Vocabulary.hpp"
#include "sessions/Messages.hpp"
#include "sessions/Payload.hpp"
#include "sessions/SessionPart.hpp"
#include "startup/MachineControl.hpp"
#include "startup/PresetLoader.hpp"
#include "typing/TypingPart.hpp"
#include <functional>
#include <optional>
#include <string>
#include <vector>

// LIB-6's session part (design §3.8; spec AC-6, AC-12, AC-32, AC-34, AC-55):
// the loader of preset format 2 in the app session, and LIB-18's
// startup::PresetLoader. It goes before StartupPart in the session's parts;
// LIB-5's TypingPart may come after it.
//
// Commands: `load-preset {ref | content, name; buildOnly?}` (an operation;
// refused at once with the first error, AC-6), `run-step {verb, value}` (one
// step on the running machine: a step that needs no running machine and has
// finished when it took effect is done at once, with its result; the others
// are an operation), `wait-idle` (an operation: until idle, at most an hour
// of emulated time). Requests: `check-preset {ref | content, name}`,
// `preset-list`.

namespace calcu::presets {

inline const sessions::CommandName kLoadPresetCommand{"load-preset"};
inline const sessions::CommandName kRunStepCommand{"run-step"};
inline const sessions::CommandName kWaitIdleCommand{"wait-idle"};
inline const sessions::RequestName kCheckPresetRequest{"check-preset"};
inline const sessions::RequestName kPresetListRequest{"preset-list"};

struct PresetsSetup {
    const machine::Catalogue* catalogue = nullptr;
    ProductRules* rules = nullptr;
    startup::MachineControl* control = nullptr;   // LIB-18's StartupPart::control(); setControl() when made later
    typing::TypingPart* typing = nullptr;         // LIB-5's part: its engine and TypingProduct
    const PresetBundle* bundle = nullptr;         // the product's bundled presets (AC-33)
    std::string home;                             // `~/` (README §3)
    std::function<std::optional<std::string>()> screenshotFolder;   // LIB-9's Paths::screenshotFolder(); the harness: the run's
};

// The reply of `preset-list` (AC-32).
struct PresetListReply final : sessions::PayloadOf<PresetListReply> {
    static constexpr sessions::PayloadKind kKind{"preset-list"};
    std::vector<PresetListEntry> entries;
    bool same(const PresetListReply& o) const { return toValue() == o.toValue(); }
    sessions::Value toValue() const override;
};

// The result of a step done at once (`elapsed-time report`: its line).
inline constexpr std::string_view kStepResultKind = "preset-step";

class PresetsPart final : public sessions::SessionPart, public startup::PresetLoader {
public:
    explicit PresetsPart(PresetsSetup);
    // Library keywords, then the product's; a later feature adds its own
    // before the session is built.
    Vocabulary& vocabulary() { return vocabulary_; }
    void setControl(startup::MachineControl* c) { setup_.control = c; }
    void setTyping(typing::TypingPart* t) { setup_.typing = t; }
    ReadSetup readSetup() const;

    void attach(sessions::SessionSetup&) override;
    void operationEnded(sessions::SessionContext&) override;   // AC-22, AC-60: regular speed at the end

    // startup::PresetLoader (LIB-18 §3.3): a read error, an outdated file,
    // another model than forModel or a refused build: CannotLoad, nothing
    // built; a failure while it is carried out: FailedWhileRunning.
    sessions::Task<startup::PresetLoad> load(std::string ref, std::optional<machine::ModelId> forModel,
                                             startup::MachineControl&, sessions::OperationContext&) override;

private:
    std::unique_ptr<PresetSource> sourceOf(const sessions::Value& args) const;
    sessions::Task<sessions::OperationResult> runLoad(sessions::OperationContext&, std::shared_ptr<const LoadPlan>,
                                                      LoadOptions);
    EngineDeps& deps();   // refreshed for each operation (one runs at a time)

    PresetsSetup setup_;
    Vocabulary vocabulary_;
    ElapsedBase elapsed_;
    EngineDeps deps_;
    bool regularDue_ = false;
};

} // namespace calcu::presets
