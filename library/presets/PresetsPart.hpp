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

// The presets' session part: the loader of preset format 2 in the app
// session, and the startup area's startup::PresetLoader. It goes before
// StartupPart in the session's parts; typing's TypingPart may come after it.
//
// Commands: `load-preset {ref | content, name; buildOnly?}` (an operation;
// refused at once with the first error), `run-step {verb, value}` (one
// step on the running machine: a step that needs no running machine and has
// finished when it took effect is done at once, with its result; the others
// are an operation), `wait-idle` (an operation: until idle, at most an hour
// of emulated time). Requests: `check-preset {ref | content, name}`,
// `preset-list`.

namespace calcu::presets {

inline const sessions::CommandName kLoadPresetCommand{"load-preset"};
inline const sessions::CommandName kRunStepCommand{"run-step"};
inline const sessions::CommandName kWaitIdleCommand{"wait-idle"};            // until settled
inline const sessions::CommandName kWaitFinishedCommand{"wait-finished"};    // until finished
inline const sessions::RequestName kIdleStateRequest{"idle-state"};          // the rule's answers now
inline constexpr std::string_view kIdleStateKind = "idle-state";
inline const sessions::RequestName kCheckPresetRequest{"check-preset"};
inline const sessions::RequestName kPresetListRequest{"preset-list"};

// Every pointer but `typing` and `bundle` is set (parts/LibraryParts builds it).
struct PresetsSetup {
    const machine::Catalogue* catalogue = nullptr;
    ProductRules* rules = nullptr;
    startup::MachineControl* control = nullptr;   // startup's machine control
    typing::TypingPart* typing = nullptr;         // typing's part: its engine and TypingProduct
    const PresetBundle* bundle = nullptr;         // the product's bundled presets
    std::string home;                             // `~/` (README §3)
    std::function<std::optional<std::string>()> screenshotFolder;   // settings' Paths::screenshotFolder(); the harness: the run's
};

// The reply of `preset-list`.
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
    // Library keywords, then the product's; another area adds its own
    // before the session is built.
    Vocabulary& vocabulary() { return vocabulary_; }
    ReadSetup readSetup() const;

    void attach(sessions::SessionSetup&) override;
    // The command `load-preset` without its notice: a read error is the
    // refusal only (opening's opener sends the notice).
    sessions::CommandOutcome loadPreset(const sessions::Value& args, sessions::SessionContext&);
    // The load of `ref` inside another part's operation (Calc-U-1600's DAP:
    // its set-up and clean start), read and carried out as `load-preset {ref,
    // buildOnly}` does; with `forModel`, a preset for another model is not
    // loaded. Its failure is the result, no notice.
    sessions::Task<sessions::OperationResult> loadWithin(sessions::OperationContext&, std::string ref, bool buildOnly,
                                                         std::optional<machine::ModelId> forModel);
    void operationEnded(sessions::SessionContext&) override;   // regular speed at the end

    // startup::PresetLoader: a read error, an outdated file, another model
    // than forModel or a refused build: CannotLoad, nothing built; a failure
    // while it is carried out: FailedWhileRunning.
    sessions::Task<startup::PresetLoad> load(std::string ref, std::optional<machine::ModelId> forModel,
                                             startup::MachineControl&, sessions::OperationContext&) override;

private:
    std::unique_ptr<PresetSource> sourceOf(const sessions::Value& args) const;
    // load's and loadWithin's: the plan of `ref`, or why it is not loaded (a
    // read error, or with `forModel` a preset for another model).
    std::variant<std::shared_ptr<const LoadPlan>, std::string> readFor(const std::string& ref,
                                                                      const std::optional<machine::ModelId>& forModel) const;
    sessions::Task<sessions::OperationResult> runLoad(sessions::OperationContext&, std::shared_ptr<const LoadPlan>,
                                                      LoadOptions);
    EngineDeps& deps();   // refreshed for each operation (one runs at a time)

    PresetsSetup setup_;
    Vocabulary vocabulary_;
    ElapsedBase elapsed_;
    EngineDeps deps_;
    bool regularDue_ = false;
    bool readRefusal_ = false;   // the last loadPreset refused for a read error (noticed by the command)
};

} // namespace calcu::presets
