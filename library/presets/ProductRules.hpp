#pragma once
#include "machine/Display.hpp"
#include "machine/EmulatedTime.hpp"
#include "machine/Ids.hpp"
#include "machine/Machine.hpp"
#include "sessions/Operation.hpp"
#include <cstdint>
#include <functional>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <vector>

// What a product tells the loader (LIB-6 design §3.4; spec AC-24, AC-25,
// AC-53, AC-62): per model, when its machine is idle, what is done after the
// build, the picture `screenshot` writes and the comparison of `expect`; the
// markers of preset.md §7; and the product's own keywords. The library names
// no model and no product (TR; presets.md PR-11).

namespace calcu::presets {

class RunContext;
class Vocabulary;

// The outcome of an item or of the after-build step: !ok stops the preset
// there (AC-17). `result`: what a harness command shows (`elapsed-time
// report`).
struct ItemResult {
    bool ok = true;
    std::string reason;
    std::string result;
};

enum class Applies { Yes, NotForThisModel, Undefined };

// AC-53: idle from what the machine reports: `readyNow` without a break for
// `stretch`, asked every `sample` ticks (D-8).
struct IdleRule {
    std::function<bool(const machine::Machine&)> readyNow;   // unset: Machine::isKeyReady()
    machine::Seconds stretch{0.0};
    machine::Ticks sample = 1;
};

// The text of `expect:` / `not-expect:` (Expect.hpp): the literal text, or
// the regular expression between slashes (AC-67's parse, D-18).
struct ExpectText {
    std::string text;                  // the literal text (`\/` read as `/`), or the expression's source
    std::optional<std::regex> regex;   // set for `/…/`
    std::string written;               // as written, for messages
};
// What the product's comparison found (AC-24, AC-66, EC-7).
struct ExpectReading {
    bool displayOff = false;
    bool found = false;
    std::string shows;   // what the display shows, for the reason
};

// The forms a place key takes (D-22): `<ID>[:<variant>]`, `{ name: <ID> }`,
// `{ file: … }` (refused until LIB-7, M-41).
struct PlaceKeyRule {
    bool bare = true;
    bool byName = true;
    bool byFile = false;
};

class ModelRules {
public:
    virtual ~ModelRules() = default;
    virtual IdleRule idle() const = 0;
    // AC-46, AC-53: after the build and before the first section.
    virtual sessions::Task<ItemResult> afterBuild(RunContext&) { co_return ItemResult{}; }
    // AC-25: the picture of the display as PNG bytes (the product's), or nullopt.
    virtual std::optional<std::vector<std::uint8_t>> screenshot(machine::Machine&, const machine::DisplayOutput&) = 0;
    // AC-24, AC-66: whether the display shows the text.
    virtual ExpectReading compare(machine::Machine&, const machine::DisplayOutput&, const ExpectText&) = 0;
    virtual bool takesRegex() const { return true; }   // AC-68: the TI models false
    // AC-62: the markers of preset.md §7 for this model.
    virtual Applies applies(std::string_view keyword) const {
        (void)keyword;
        return Applies::Yes;
    }
    virtual PlaceKeyRule placeKey(const machine::PlaceName&) const { return {}; }
};

class ProductRules {
public:
    virtual ~ProductRules() = default;
    virtual std::string_view productName() const = 0;       // for the notices
    virtual std::string_view formatReference() const = 0;   // where the preset format is described (AC-8)
    virtual ModelRules& forModel(const machine::ModelId&) = 0;
    virtual void addKeywords(Vocabulary&) {}                // AC-29: header keys, sections, steps
};

} // namespace calcu::presets
