#pragma once
#include "control/ControlState.hpp"
#include "machine/Attach.hpp"
#include "machine/Ids.hpp"
#include "presets/PresetsPart.hpp"
#include "sessions/LibraryTopics.hpp"
#include "sessions/Operation.hpp"
#include "sessions/SessionPart.hpp"
#include "shell/HitMap.hpp"
#include "shell/Inputs.hpp"
#include "shell/ShellProduct.hpp"

#include <string>
#include <string_view>
#include <map>
#include <memory>
#include <string>
#include <vector>

// What the shell's tests are drawn from:
// a control state, a preset list and a product built for the tests, without
// code of a product, and TestShellPart, a session part that publishes
// scripted Values on the topics the shell reads (control-state, run-state,
// roms-missing, media), answers `preset-list` and records the commands the
// shell sends. Header only: the library's tests, calcu_qt6_tests and the Web
// test product use it alike. The inputs are built from the owners' payload
// types where they exist (the control state's ControlState, presets'
// PresetListReply), so that a change of their toValue() fails a shell test.

namespace calcu::shell::test {

namespace c = calcu::control;
namespace m = calcu::machine;
using sessions::Value;

inline c::PeripheralChoice fits(const char* id, const char* name, bool plugged = false) {
    c::PeripheralChoice p;
    p.id = m::PeripheralId{id};
    p.name = name;
    p.plugged = plugged;
    return p;
}

inline c::PlaceEntry place(const char* path, const char* pinOut, std::vector<c::PeripheralChoice> fitting) {
    c::PlaceEntry e;
    e.path = m::PlacePath{{m::PlaceName{path}}};
    e.connector = m::ConnectorId{std::string(pinOut) + "-connector"};
    e.pinOut = m::PinOutId{pinOut};
    e.fitting = std::move(fitting);
    return e;
}

inline c::Availability unavailable(const char* reason) { return c::Availability{false, reason}; }

// Two models (Model-A with the variants v1 and v2, Model-B); on Model-A the
// module slots `slot-1` and `slot-2` (pin-out `module`), the place `printer`
// where one peripheral fits (TD-20), `aux` where two fit (TP-4, with the
// variants pen-1 and pen-2, and TD-21), the host drive `host-drive` (needs
// a folder); one media place `card` of kind `test-card`; the paper panel;
// the actions Reset, Cold boot, Load Preset, Open; the product entry
// `tp.disk`. Model-B has only `printer`.
inline c::ControlState controlState(const std::string& model = "Model-A") {
    c::ControlState s;
    s.models = {{m::ModelId{"Model-A"}, {}}, {m::ModelId{"Model-B"}, {}}};
    s.model = m::ModelId{model};
    if (model == "Model-A") {
        s.variants = {{m::VariantId{"v1"}, {}}, {m::VariantId{"v2"}, {}}};
        s.variant = m::VariantId{"v1"};
        s.places.push_back(place("slot-1", "module", {fits("mod-x", "Module X"), fits("mod-y", "Module Y")}));
        s.places.push_back(place("slot-2", "module", {fits("mod-x", "Module X")}));
    }
    s.places.push_back(place("printer", "printer-port", {fits("test-dotter", "TD-20")}));
    if (model == "Model-A") {
        auto tp4 = fits("test-plotter", "TP-4");
        tp4.variants = {{m::VariantId{"pen-1"}, {}}, {m::VariantId{"pen-2"}, {}}};
        tp4.variant = m::VariantId{"pen-1"};
        s.places.push_back(place("aux", "aux-port", {tp4, fits("test-dotter-2", "TD-21")}));
        s.places.push_back(place("host-drive", "host", {fits("drive", "Host drive")}));
        c::MediaPlaceEntry card;
        card.id = m::MediaPlaceId{"card"};
        card.mediumKind = "test-card";
        card.sides = {0, 1};
        s.mediaPlaces.push_back(card);
    }
    s.panels.push_back(c::PanelEntry{c::kPaperPanel, std::nullopt, std::nullopt});
    for (const auto& a : {c::kResetAction, c::kColdBootAction, c::kLoadPresetAction, c::kOpenAction})
        s.actions.push_back(c::ActionEntry{a, {}});
    s.productEntries.push_back(c::ProductEntry{
        "tp.disk", sessions::valuePayload("tp.disk", Value::object({{"available", false}, {"reason", "No disk drive."}}))});
    return s;
}

inline Value controlValue(const std::string& model = "Model-A") { return controlState(model).toValue(); }

// Plugs `peripheral` in at `where` (the place's choice marked).
inline void plug(c::ControlState& s, const char* where, const char* peripheral) {
    for (auto& p : s.places)
        if (m::placeText(p.path) == where)
            for (auto& f : p.fitting) f.plugged = f.id.value == peripheral;
    for (auto& panel : s.panels)
        if (panel.kind == c::kPaperPanel && std::string(where) == "printer") {
            panel.ownerPlace = m::PlacePath{{m::PlaceName{where}}};
            panel.owner = m::PeripheralId{peripheral};
        }
}

inline Value presetList() {
    presets::PresetListReply r;
    using K = presets::PresetListEntry::Kind;
    r.entries.push_back({"examples:startup/a.test", m::ModelId{"Model-A"}, "Start A", "The A start", K::Startup, "", {}});
    r.entries.push_back({"examples:startup/a2.test", m::ModelId{"Model-A"}, "Start A2", "", K::Startup, "", {}});
    r.entries.push_back({"examples:startup/b.test", m::ModelId{"Model-B"}, "Start B", "The B start", K::Startup, "", {}});
    r.entries.push_back({"examples:games/one.test", m::ModelId{"Model-A"}, "Game One", "", K::Example, "Games", {}});
    r.entries.push_back({"examples:games/two.test", m::ModelId{"Model-A"}, "Game Two", "", K::Example, "Games", {}});
    r.entries.push_back({"examples:math/sum.test", m::ModelId{"Model-B"}, "Sum", "", K::Example, "Math", {}});
    return r.toValue();
}

inline Value mediaWithKind() {
    return Value::object({{"kinds", Value(Value::Array{Value::object({{"kind", "test-card"}, {"label", "Card"}})})},
                          {"places", Value(Value::Array{})}});
}

// The media topic with the place `card`: its picker holds the empty entry
// (chosen), a template and an instance that cannot be chosen now.
inline Value mediaWithPlace() {
    auto entry = [](const char* group, const char* id, const char* label, bool canChoose, bool chosen, const char* hint = "") {
        Value::Object o{{"group", group}, {"id", id}, {"label", label}, {"canChoose", canChoose}, {"chosen", chosen}};
        if (*hint) o.push_back({"hint", hint});
        return Value(std::move(o));
    };
    const Value picker(Value::Array{entry("empty", "", "Empty", true, true),
                                    entry("bundled-template", "bundled:Blank.test-card.yaml", "Blank", true, false),
                                    entry("instance", "saved:Busy.test-card.yaml", "Busy", false, false, "In the other reader.")});
    const Value place = Value::object({{"place", "card"}, {"kind", "test-card"}, {"nameAndSave", false}, {"picker", picker}});
    return Value::object({{"kinds", Value(Value::Array{Value::object({{"kind", "test-card"}, {"label", "test card"}})})},
                          {"places", Value(Value::Array{place})}});
}

inline ShellProduct product() {
    ShellProduct p;
    p.name = "Test-Product";
    p.version = "1.2.3";
    p.build = "abc123";
    p.helpUrl = "https://calc-u.org/test/";
    p.aboutLines = {"A product made for the tests.", "No code of a product."};
    p.aboutLinks = {{"Source", "https://example.org/source"}, {"Privacy", "https://example.org/privacy"}};
    p.variantLabels = {{"Model-A", "Revision"}, {"test-plotter", "Pen"}};
    p.presetFilter = "Presets (*.test)";
    p.presetUploads = ".test";
    p.moduleSlotPinOuts = {"module"};
    p.placesWithoutControl = {"host-drive"};
    p.items.push_back(ProductItem{MenuGroup::FileKinds, "load-program", "Load Program…", "", {}});
    p.items.push_back(ProductItem{MenuGroup::FileMedia, "mount-drive", "Host Drive…", "", {}});
    p.items.push_back(ProductItem{MenuGroup::MachineEnd, "disk-thing", "Disk Thing", "tp.disk", {}});
    return p;
}

// A faceplate: the keys 1, 2 and 3 in a row of 10 × 10 units, 10 apart, and
// the display above them; 4 logical pixels of distance.
inline HitMap hitMap() {
    HitMap h;
    h.keys = {{m::KeyId{"1"}, {0, 20, 10, 10}}, {m::KeyId{"2"}, {20, 20, 10, 10}}, {m::KeyId{"3"}, {40, 20, 10, 10}}};
    h.display = Rect{0, 0, 50, 15};
    h.distance = 4;
    return h;
}

inline ShellInputs inputs(Value cs = controlValue()) {
    ShellInputs in;
    in.controlState = std::move(cs);
    in.presetList = presetList();
    return in;
}

// The session part for the window tests and the Web test product.
class TestShellPart final : public sessions::SessionPart {
public:
    struct Sent {
        std::string command;
        Value args;
    };
    // withRunControl = false: no scripted `run-state` topic and no `resume`
    // command, for a session that hosts the debugger's DebuggerPart (the Web test
    // product).
    // withMedia = false: no scripted `media` topic and no `media.import`,
    // for a session that hosts the media area's MediaPart (the Web test
    // product Test-M, media.spec.js).
    explicit TestShellPart(bool withControlState = true, bool withRunControl = true, bool withMedia = true)
        : runControl_(withRunControl), media_(withMedia) {
        if (withControlState) values_[std::string("control-state")] = controlValue();
        if (runControl_) values_[std::string("run-state")] = Value::object({{"paused", false}});
    }

    void attach(sessions::SessionSetup& s) override {
        for (const char* t : {"control-state", "run-state", "roms-missing", "media"}) {
            if (!runControl_ && std::string_view(t) == "run-state") continue;
            if (!media_ && std::string_view(t) == "media") continue;
            topics_[t] = &s.stateTopic(sessions::TopicName{t});
        }
        for (auto& [name, v] : values_) topics_[name]->set(sessions::valuePayload(kindOf(name), v));
        s.request(sessions::RequestName{"preset-list"}, [this](const Value&, sessions::SessionContext&) {
            return sessions::replyGiven(sessions::valuePayload("preset-list", presets));
        });
        // What the shell sent, for the Web tests: {sent: [{command, args}]}.
        s.request(sessions::RequestName{"tsp.sent"}, [this](const Value&, sessions::SessionContext&) {
            Value::Array list;
            for (const auto& x : sent) list.push_back(Value::object({{"command", x.command}, {"args", x.args}}));
            return sessions::replyGiven(sessions::valuePayload("tsp.sent", Value::object({{"sent", Value(std::move(list))}})));
        });
        for (const char* n : {"model", "variant", "attach", "detach", "load-preset", "resume", "paste", "media.import",
                              "screen-key-down", "screen-key-up", "tsp.confirm"}) {
            const std::string name = n;
            if (!runControl_ && name == "resume") continue;
            if (!media_ && name == "media.import") continue;
            s.command(sessions::CommandName{name}, [this, name](const Value& a, sessions::SessionContext&) -> sessions::CommandOutcome {
                sent.push_back({name, a});
                if (name == "tsp.confirm" && !a.find("confirmed")) return sessions::Refused{"asks: Replace the card?"};
                if (name == "model" && a.find("name") && a.find("name")->string()) {
                    publish("control-state", controlValue(*a.find("name")->string()));
                }
                return sessions::Done{};
            });
        }
        // Reset and Cold boot end a running operation.
        for (const char* n : {"reset", "cold-boot"}) {
            const std::string name = n;
            s.command(
                sessions::CommandName{name},
                [this, name](const Value& a, sessions::SessionContext&) -> sessions::CommandOutcome {
                    sent.push_back({name, a});
                    return sessions::StartOperation{[](sessions::OperationContext& c) { return runFor(c, 0); }};
                },
                true);
        }
        // An operation at full speed until a Reset.
        s.command(sessions::CommandName{"tsp.op"}, [this](const Value& a, sessions::SessionContext&) -> sessions::CommandOutcome {
            sent.push_back({"tsp.op", a});
            const auto ticks = static_cast<m::Ticks>(read::integer(a, "ticks", 1'000'000'000'000));
            return sessions::StartOperation{[ticks](sessions::OperationContext& c) { return runFor(c, ticks); }};
        });
        s.command(sessions::CommandName{"tsp.notice"}, [](const Value& a, sessions::SessionContext& c) -> sessions::CommandOutcome {
            c.notice(read::text(a, "text"));
            return sessions::Done{};
        });
        s.command(sessions::CommandName{"tsp.log"}, [](const Value& a, sessions::SessionContext& c) -> sessions::CommandOutcome {
            c.log(read::text(a, "text"));
            return sessions::Done{};
        });
        s.command(sessions::CommandName{"tsp.refuse"}, [this](const Value& a, sessions::SessionContext&) -> sessions::CommandOutcome {
            sent.push_back({"tsp.refuse", a});
            return sessions::Refused{read::text(a, "reason")};
        });
        // The Web tests script the topics through commands.
        s.command(sessions::CommandName{"tsp.publish"}, [this](const Value& a, sessions::SessionContext&) -> sessions::CommandOutcome {
            const Value* v = a.find("value");
            publish(read::text(a, "topic"), v ? *v : Value());
            return sessions::Done{};
        });
        s.command(sessions::CommandName{"tsp.plug"}, [this](const Value& a, sessions::SessionContext&) -> sessions::CommandOutcome {
            auto cs = controlState();
            plug(cs, read::text(a, "place").c_str(), read::text(a, "peripheral").c_str());
            publish("control-state", cs.toValue());
            return sessions::Done{};
        });
    }

    // Sets a topic's Value (Null: withdrawn as an empty object is not; a
    // Null Value is kept as Null). Flushed at the next host call or command.
    void publish(const std::string& topic, Value v) {
        values_[topic] = v;
        if (auto it = topics_.find(topic); it != topics_.end() && it->second)
            it->second->set(sessions::valuePayload(kindOf(topic), std::move(v)));
    }
    int count(const std::string& command) const {
        int n = 0;
        for (const auto& s : sent) n += s.command == command ? 1 : 0;
        return n;
    }
    const Sent* last(const std::string& command) const {
        for (auto it = sent.rbegin(); it != sent.rend(); ++it)
            if (it->command == command) return &*it;
        return nullptr;
    }

    std::vector<Sent> sent;
    Value presets = presetList();

private:
    static sessions::Task<sessions::OperationResult> runFor(sessions::OperationContext& c, m::Ticks n) {
        if (n > 0) co_await c.run(n);
        co_return sessions::OperationResult{};
    }
    static std::string_view kindOf(const std::string& topic) {
        if (topic == "control-state") return "control-state";
        if (topic == "run-state") return "run-state";
        if (topic == "roms-missing") return "roms-missing";
        return "media";
    }
    bool runControl_ = true;
    bool media_ = true;
    std::map<std::string, Value> values_;
    std::map<std::string, sessions::StateTopic*> topics_;
};

} // namespace calcu::shell::test
