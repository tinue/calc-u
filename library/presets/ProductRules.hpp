#pragma once
#include "machine/Display.hpp"
#include "machine/EmulatedTime.hpp"
#include "machine/Ids.hpp"
#include "machine/Machine.hpp"
#include "sessions/Operation.hpp"
#include "startup/MachinePlan.hpp"
#include <cstdint>
#include <functional>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <vector>

// What a product tells the loader: per model, when its machine is idle, what
// is done after the build, the picture `screenshot` writes and the comparison
// of `expect`; the markers of preset.md §7; and the product's own keywords.
// The library names no model and no product.

namespace calcu::presets {

class RunContext;
class ReadContext;
class Vocabulary;
struct LoadPlan;

// The outcome of an item or of the after-build step: !ok stops the preset
// there. `result`: what a harness command shows (`elapsed-time
// report`).
struct ItemResult {
    bool ok = true;
    std::string reason;
    std::string result;
};

enum class Applies { Yes, NotForThisModel, Undefined };

// When a model's machine takes keys, waits, has settled and has finished
// (the product's tables: Calc-U-59 reference/Idle.md, Calc-U-1600
// docs/developer/Idle.md):
// - takes keys: the ROM or a program reads the keyboard as input now
//   (unset: Machine::isKeyReady());
// - waits: it takes keys or awaits a medium, and nothing is busy;
// - settled: it has waited without a break for `stretch`, asked every
//   `sample` ticks (100 ms for the TI, 20 frames for the Sharp models);
// - finished: it waits and the program has ended.
struct IdleRule {
    std::function<bool(const machine::Machine&)> takesKeys;
    std::function<bool(const machine::Machine&)> awaitsMedium;                // unset: never
    std::function<std::vector<std::string>(const machine::Machine&)> busy;    // what keeps it busy; unset: nothing
    std::function<bool(const machine::Machine&)> programEnded;                // unset: always
    machine::Seconds stretch{0.0};
    machine::Ticks sample = 1;
};

// The rule's answers for the machine now.
struct IdleState {
    bool takesKeys = false;
    bool waits = false;
    bool ended = false;              // waits, and the program has ended
    std::vector<std::string> busy;   // what is busy
};
IdleState idleState(const IdleRule&, const machine::Machine&);

// The text of `expect:` / `not-expect:` (Expect.hpp): the literal text, or
// the regular expression between slashes, parsed and compiled when the preset is read.
struct ExpectText {
    std::string text;                  // the literal text (`\/` read as `/`), or the expression's source
    std::optional<std::regex> regex;   // set for `/…/`
    std::string written;               // as written, for messages
};
// What the product's comparison found.
struct ExpectReading {
    bool displayOff = false;
    bool found = false;
    std::string shows;   // what the display shows, for the reason
};

// The forms a place key takes: `<ID>[:<variant>]`, `{ name: <ID> }` (always
// taken), `{ file: … }` (read by the media area's resolver; refused without
// one).
struct PlaceKeyRule {
    bool bare = true;
    bool byFile = true;
    // The sentence for a `{ file: }` the place does not take (Calc-U-59's
    // slot-1); empty: the reader's own.
    std::string noFile;
};

class ModelRules {
public:
    virtual ~ModelRules() = default;
    virtual IdleRule idle() const = 0;
    // After the build and before the first section.
    virtual sessions::Task<ItemResult> afterBuild(RunContext&) { co_return ItemResult{}; }
    // The picture of the display as PNG bytes (the product's), or nullopt.
    virtual std::optional<std::vector<std::uint8_t>> screenshot(machine::Machine&, const machine::DisplayOutput&) = 0;
    // Whether the display shows the text.
    virtual ExpectReading compare(machine::Machine&, const machine::DisplayOutput&, const ExpectText&) = 0;
    // The refusal of an `expect:` / `not-expect:` text (keyword
    // "expect:" or "not-expect:") the comparison cannot take, when the preset
    // is read; nullopt: taken. A TI model takes a number only.
    virtual std::optional<std::string> checkExpect(const ExpectText&, std::string_view keyword) const {
        (void)keyword;
        return std::nullopt;
    }
    // The markers of preset.md §7 for this model.
    virtual Applies applies(std::string_view keyword) const {
        (void)keyword;
        return Applies::Yes;
    }
    virtual PlaceKeyRule placeKey(const machine::PlaceName&) const { return {}; }
    // One check once every header key is read (the place keys in
    // `plan.places`, the product's in ReadContext::kept), for pairs of keys
    // that do not go together; errors through ReadContext::error at the line
    // of the later key. (A header key puts its requests into the plan through
    // ReadContext::addPeripheral and setContentAt.)
    virtual void checkHeader(const LoadPlan& plan, ReadContext&) const { (void)plan; }
};

class ProductRules {
public:
    virtual ~ProductRules() = default;
    virtual std::string_view productName() const = 0;       // for the notices
    virtual std::string_view formatReference() const = 0;   // where the preset format is described (the outdated notice)
    virtual ModelRules& forModel(const machine::ModelId&) = 0;
    virtual void addKeywords(Vocabulary&) {}                // header keys, sections, steps
};

} // namespace calcu::presets
