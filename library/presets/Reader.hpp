#pragma once
#include "machine/Attach.hpp"
#include "machine/Catalogue.hpp"
#include "machine/Machine.hpp"
#include "machine/Names.hpp"
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

// Reading a preset: all or nothing. The reader turns a file into a LoadPlan
// of checked items; the engine carries out only a plan. It builds nothing and
// touches no session.

namespace calcu::presets {

struct ReadError {
    int line = 0;          // 0: the file as a whole
    std::string message;   // `<name>, line <n>: <what was expected>`, or `<name>: …` for line 0
};

struct HeaderInfo {
    std::string presetName;   // `preset-name`, else the file stem
    bool nameWritten = false;
    std::string description;
    std::int64_t timeoutSeconds = 3600;   // `timeout:`
};

struct PlaceRequest {
    machine::PeripheralRequest request;   // ROMs filled at the build (startup)
    int line = 0;
};

struct PlannedItem {
    std::unique_ptr<Item> item;
    bool skipWaitAfter = false;   // a `skip-wait:` follows
};

struct LoadPlan {
    std::string name;   // the source's, for messages
    machine::ModelId model;
    std::optional<machine::VariantId> variant;
    int modelLine = 0;
    std::vector<PlaceRequest> places;   // the place keys
    HeaderInfo header;
    std::map<std::string, std::shared_ptr<const ProductValue>> productHeader;   // a product's header keys
    // What a product's header keys plug in (Calc-U-1600's host-drive,
    // bus-rom; ReadContext::addPeripheral), after the places in the machine plan.
    std::vector<machine::PeripheralRequest> headerPeripherals;
    std::vector<PlannedItem> items;      // sections and steps in file order
    bool letItFinish = false;            // `let-it-finish:`
    ModelRules* rules = nullptr;         // the model's
};

// What the reader reads with.
struct ReadSetup {
    const Vocabulary& vocabulary;
    ProductRules& rules;
    const machine::Catalogue& catalogue;
    const typing::TypingProduct* typing = nullptr;   // typing's profile per model (`key`, `type`)
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
    // A file the preset names; the error is reported at `line`.
    std::optional<std::vector<std::uint8_t>> readFile(std::string_view path, int line);
    const PresetSource& source() const { return source_; }
    void keep(std::string key, std::shared_ptr<const ProductValue>);
    // What was kept under `key` in this read so far, or nullptr (media: the
    // in-file media a header key and the steps of one preset share).
    std::shared_ptr<const ProductValue> kept(const std::string& key) const {
        const auto it = kept_.find(key);
        return it == kept_.end() ? nullptr : it->second;
    }
    void error(int line, std::string message);   // collected; the lowest line wins
    // A header key's own peripheral (Calc-U-1600's host-drive, bus-rom): a
    // request of the machine plan after the place keys', in the order of the
    // model's places, those of one place in the order added.
    void addPeripheral(machine::PeripheralRequest);
    // The content of what a place key plugs into `place` (Calc-U-1600's
    // slot-N-rom), unless its reference gave one; set once the header is read.
    void setContentAt(const machine::PlaceName&, std::shared_ptr<const machine::ProductData>);

    // The reader's and run-step's own.
    void setPlugged(std::function<std::optional<machine::PeripheralId>(const machine::PlaceName&)> f) {
        plugged_ = std::move(f);
    }
    std::optional<ReadError> firstError() const;
    std::map<std::string, std::shared_ptr<const ProductValue>> takeKept() { return std::move(kept_); }
    // addPeripheral's and setContentAt's into the plan.
    void completePlan(LoadPlan&);

private:
    const ReadSetup& setup_;
    const PresetSource& source_;
    const machine::MachineDescriptor& model_;
    ModelRules* rules_;
    std::function<std::optional<machine::PeripheralId>(const machine::PlaceName&)> plugged_;
    std::vector<ReadError> errors_;
    std::map<std::string, std::shared_ptr<const ProductValue>> kept_;
    std::vector<machine::PeripheralRequest> peripherals_;
    std::vector<std::pair<machine::PlaceName, std::shared_ptr<const machine::ProductData>>> contents_;
};

std::variant<LoadPlan, ReadError> readPreset(const PresetSource&, const ReadSetup&);

// Only the header keys `format-version`, `model`, `preset-name` and
// `description`; no file the preset names is read.
struct PresetHeader {
    machine::ModelId model;
    std::string name, description;
};
std::variant<PresetHeader, ReadError> readHeader(const PresetSource&, const ProductRules&, const machine::Catalogue&);

// One step read against a running machine (run-step, the harness commands
// of the steps): nullptr with the error.
std::unique_ptr<Item> readStep(std::string_view verb, const formats::YamlNode& value, const ReadSetup&,
                               machine::Machine&, std::string* error);

// The message of an error at `line` of the preset `name`.
std::string errorText(const std::string& name, int line, const std::string& what);
// The library's one ASCII lower case (machine/Names.hpp), under the name
// library/debugger still uses.
using machine::lowerAscii;

} // namespace calcu::presets
