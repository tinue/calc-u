#pragma once
#include "machine/Attach.hpp"
#include "machine/Catalogue.hpp"
#include "machine/Machine.hpp"
#include "presets/ProductRules.hpp"
#include "presets/Source.hpp"
#include "presets/Vocabulary.hpp"
#include "typing/TypingPart.hpp"
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

// Reading a preset: all or nothing (LIB-6 design §3.2, D-2, D-3; spec AC-1
// to AC-9, AC-30, AC-58, AC-59, AC-62, EC-1, EC-5, EC-8). The reader turns a
// file into a LoadPlan of checked items; the engine carries out only a plan.
// It builds nothing and touches no session.

namespace calcu::presets {

struct ReadError {
    int line = 0;          // 0: the file as a whole
    std::string message;   // `<name>, line <n>: <what was expected>`, or `<name>: …` for line 0
};

struct HeaderInfo {
    std::string presetName;   // `preset-name`, else the file stem (AC-2)
    bool nameWritten = false;
    std::string description;
    std::int64_t timeoutSeconds = 3600;   // AC-57
};

struct PlaceRequest {
    machine::PeripheralRequest request;   // ROMs filled at the build (LIB-18)
    int line = 0;
};

struct PlannedItem {
    std::unique_ptr<Item> item;
    bool skipWaitAfter = false;   // a `skip-wait:` follows (AC-58)
};

struct LoadPlan {
    std::string name;   // the source's, for messages
    machine::ModelId model;
    std::optional<machine::VariantId> variant;
    int modelLine = 0;
    std::vector<PlaceRequest> places;   // AC-3, AC-10
    HeaderInfo header;
    std::map<std::string, std::shared_ptr<const ProductValue>> productHeader;   // a product's header keys
    std::vector<PlannedItem> items;      // sections and steps in file order (AC-14)
    bool letItFinish = false;            // AC-59
    ModelRules* rules = nullptr;         // the model's (AC-53)
};

// What the reader reads with.
struct ReadSetup {
    const Vocabulary& vocabulary;
    ProductRules& rules;
    const machine::Catalogue& catalogue;
    const typing::TypingProduct* typing = nullptr;   // LIB-5: the profile per model (`key`, `type`)
};

// What a keyword's reader sees.
class ReadContext {
public:
    ReadContext(const ReadSetup&, const PresetSource&, const machine::MachineDescriptor&);
    const machine::Catalogue& catalogue() const { return setup_.catalogue; }
    const Vocabulary& vocabulary() const { return setup_.vocabulary; }
    const machine::MachineDescriptor& model() const { return model_; }   // known before every keyword but format-version and model
    ModelRules& rules() { return *rules_; }
    const typing::TypingProfile* typingProfile() const;
    bool typesText() const { return setup_.typing && setup_.typing->typesText(); }
    // What the header names (run-step: what the running machine has).
    bool plugged(const machine::PlaceName&, const machine::PeripheralId&) const;
    bool pluggedAny(const machine::PlaceName&) const;
    // A file the preset names; the error is reported at `line` (AC-7, OQ-2, EC-5).
    std::optional<std::vector<std::uint8_t>> readFile(std::string_view path, int line);
    const PresetSource& source() const { return source_; }
    void keep(std::string key, std::shared_ptr<const ProductValue>);
    // What was kept under `key` in this read so far, or nullptr (LIB-7: the
    // in-file media a header key and the steps of one preset share).
    std::shared_ptr<const ProductValue> kept(const std::string& key) const {
        const auto it = kept_.find(key);
        return it == kept_.end() ? nullptr : it->second;
    }
    void error(int line, std::string message);   // collected; the lowest line wins

    // The reader's and run-step's own.
    void setPlugged(std::function<std::optional<machine::PeripheralId>(const machine::PlaceName&)> f) {
        plugged_ = std::move(f);
    }
    std::optional<ReadError> firstError() const;
    std::map<std::string, std::shared_ptr<const ProductValue>> takeKept() { return std::move(kept_); }

private:
    const ReadSetup& setup_;
    const PresetSource& source_;
    const machine::MachineDescriptor& model_;
    ModelRules* rules_;
    std::function<std::optional<machine::PeripheralId>(const machine::PlaceName&)> plugged_;
    std::vector<ReadError> errors_;
    std::map<std::string, std::shared_ptr<const ProductValue>> kept_;
};

std::variant<LoadPlan, ReadError> readPreset(const PresetSource&, const ReadSetup&);

// AC-32: only the header keys `format-version`, `model`, `preset-name` and
// `description`; no file the preset names is read.
struct PresetHeader {
    machine::ModelId model;
    std::string name, description;
};
std::variant<PresetHeader, ReadError> readHeader(const PresetSource&, const ProductRules&, const machine::Catalogue&);

// One step read against a running machine (run-step, the harness commands
// of the steps; §3.8): nullptr with the error.
std::unique_ptr<Item> readStep(std::string_view verb, const formats::YamlNode& value, const ReadSetup&,
                               machine::Machine&, std::string* error);

// The message of an error at `line` of the preset `name`.
std::string errorText(const std::string& name, int line, const std::string& what);
// Names of models, variants and peripherals are matched without regard to
// case (AC-1); the catalogue's spelling back.
std::string lowerAscii(std::string_view);

} // namespace calcu::presets
