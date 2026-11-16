#pragma once
#include "formats/Yaml.hpp"
#include "machine/Attach.hpp"
#include "presets/ProductRules.hpp"
#include "sessions/Operation.hpp"
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

// The vocabulary of the preset format (LIB-6 design §3.3; spec AC-29,
// AC-30): the header keys, sections and steps the running program knows.
// This feature registers its own (addLibraryKeywords, LibraryKeywords.cpp);
// a product adds its own through ProductRules::addKeywords; a later feature
// in its step (LIB-7 media steps, LIB-8 `cut-paper`, LIB-10 the debugger
// steps). Every keyword is read by the same YAML, checked with the rest
// before anything is built, carried out in file order, and gives its errors
// with the line (AC-29). No block is handed over as raw lines (AC-1).

namespace calcu::presets {

class ReadContext;
class RunContext;

// AC-56: what "finished" means for an item. Idle: `key`, `type` (the
// implicit wait follows). Own: its carryOut includes its finish (a section,
// `card:`). TakenEffect: every other step.
enum class After { Idle, TakenEffect, Own };

// A checked section or step, ready to be carried out.
class Item {
public:
    virtual ~Item() = default;
    int line = 0;
    // Until the item has been carried out (AC-58: its keys pressed, ENTER typed).
    virtual sessions::Task<ItemResult> carryOut(RunContext&) = 0;
    virtual After after() const = 0;
    virtual bool needsRunningMachine() const = 0;   // AC-19: key, type, card, a section that writes memory
    virtual std::string logText() const = 0;        // AC-18
};

// A product's header key value kept for its owner (`debug:`, `bus-rom:`).
struct ProductValue {
    virtual ~ProductValue() = default;
};

struct HeaderKeyDef {
    std::string key;
    std::function<void(const formats::YamlNode&, int line, ReadContext&)> read;
};

struct SectionDef {
    std::string key;   // a block, or a value where the owner says so (`partition: 479`)
    std::function<std::unique_ptr<Item>(const formats::YamlNode&, ReadContext&)> read;
};

struct StepDef {
    std::string verb;
    enum class Value { None, Scalar, Raw, Any } value = Value::None;   // `- full-speed:`, `- wait: 2s`, `- expect: …`, `- floppy: {…}`
    // nullptr with an error given through ReadContext::error.
    std::function<std::unique_ptr<Item>(const formats::YamlNode&, ReadContext&)> read;
    bool command = true;     // also a harness command of the same name (§3.9)
    bool frontend = false;   // AC-63: never fails; passed over where nobody has the panel
};

// The library's step-list section (AC-4).
inline constexpr const char* kKeysSection = "keys";
inline constexpr const char* kSkipWait = "skip-wait";
inline constexpr const char* kLetItFinish = "let-it-finish";

// LIB-7 (M-41; LIB-6 AC-3, D-22): the owner of media resolves a place key's
// reference `{ name: }` or `{ file: }` (a module that is a medium, by its
// name tag or its file). NotMine: the reader goes on as without a resolver
// (a `name:` is then a peripheral ID); Error: it gave the error through
// ReadContext::error.
struct PlaceResolution {
    enum class How { NotMine, Done, Error } how = How::NotMine;
    machine::PeripheralRequest request;   // Done: the place is set by the reader
};
using PlaceResolver =
    std::function<PlaceResolution(const machine::PlaceName&, const formats::YamlNode& reference, int line, ReadContext&)>;

class Vocabulary {
public:
    // A name twice is a programming error: the second is not taken, and
    // duplicates() names it (checked by the part at attach).
    void add(HeaderKeyDef);
    void add(SectionDef);
    void add(StepDef);
    const HeaderKeyDef* header(std::string_view) const;
    const SectionDef* section(std::string_view) const;
    const StepDef* step(std::string_view) const;
    const std::vector<StepDef>& steps() const { return steps_; }
    const std::vector<std::string>& duplicates() const { return duplicates_; }
    // The verbs whose value is raw (YamlOptions::rawStepVerbs).
    std::vector<std::string> rawVerbs() const;
    // LIB-7 (M-41): the resolver of place references, and whether a file a
    // preset names is one of the product's bundled media (checkBundle,
    // bundle.md §3). Unset: none.
    void setPlaceResolver(PlaceResolver r) { placeResolver_ = std::move(r); }
    const PlaceResolver& placeResolver() const { return placeResolver_; }
    void setBundledMedium(std::function<bool(std::string_view named)> f) { bundledMedium_ = std::move(f); }
    bool isBundledMedium(std::string_view named) const { return bundledMedium_ && bundledMedium_(named); }

private:
    bool known(std::string_view) const;
    std::vector<HeaderKeyDef> headers_;
    std::vector<SectionDef> sections_;
    std::vector<StepDef> steps_;
    std::vector<std::string> duplicates_;
    PlaceResolver placeResolver_;
    std::function<bool(std::string_view)> bundledMedium_;
};

// This feature's keywords (§3.5): the header keys format-version, model,
// preset-name, description and timeout are the reader's own; here `keys`
// and the steps key, type, wait, regular-speed, full-speed, elapsed-time,
// expect, not-expect, screenshot and syncclock.
void addLibraryKeywords(Vocabulary&);

} // namespace calcu::presets
