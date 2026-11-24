// The models a host draws elsewhere (the Web page): ModelHost over the
// session's messages, in the Value forms the page reads.
#include "TestHarness.hpp"
#include "TestShellPart.hpp"
#include "shell/ModelHost.hpp"
#include "shell/SettingsModel.hpp"
#include <string>

using namespace calcu;
using namespace calcu::shell;
namespace st = calcu::shell::test;
using sessions::Value;

namespace {

sessions::Message state(const char* topic, Value v) {
    return sessions::StateMessage{sessions::TopicName{topic}, sessions::valuePayload(topic, std::move(v))};
}

ModelSetup setup() {
    ModelSetup s;
    s.product = st::product();
    s.traits.hasQuit = false;
    s.traits.shortcuts = false;
    s.traits.savedMedia = true;
    return s;
}

std::string ids(const Value& list) {
    std::string out;
    if (const auto* a = list.elements())
        for (const auto& e : *a) out += (out.empty() ? "" : " ") + read::text(e, "id");
    return out;
}

} // namespace

TEST(model_host_null_until_set) {
    ModelHost h;
    CHECK(h.take(state("control-state", st::controlValue())));
    CHECK(h.value().isNull());
    h.set(setup(), {});
    CHECK(!h.value().isNull());
}

TEST(model_host_serves_the_menus_bar_panels_and_dialog) {
    ModelHost h;
    h.set(setup(), {});
    CHECK(h.take(state("control-state", st::controlValue())));
    const Value v = h.value();
    CHECK_EQ(ids(*v.find("menus")), std::string("file machine help"));   // no Edit item in the product
    CHECK_EQ(v.find("bar")->elements()->size(), std::size_t{5});
    CHECK(v.find("panels")->elements() != nullptr);
    CHECK(v.find("dialog")->elements() != nullptr);
    // A control in its Value form, as the page draws it.
    const Value& groups = *v.find("bar");
    const Value& model = read::elements(groups.elements()->front(), "controls").front();
    CHECK_EQ(read::text(model, "kind"), std::string("model"));
    CHECK_EQ(read::text(groups.elements()->back(), "group"), std::string("End"));
    CHECK(read::flag(groups.elements()->back(), "pushed"));
}

TEST(model_host_serves_the_name_and_save_prompt_of_each_media_place) {
    ModelSetup s = setup();
    s.product.nameAndSave.prompt = "Instance name:";
    s.product.nameAndSave.prompts = {{"floppy", "Disk name:"}};
    ModelHost h;
    h.set(s, {});
    CHECK_EQ(sessions::toText(*h.value().find("nameAndSave")), sessions::toText(Value(Value::Object{})));   // no media topic yet
    auto place = [](const char* name, bool offers) { return Value::object({{"place", name}, {"nameAndSave", offers}}); };
    CHECK(h.take(state("media", Value::object({{"kinds", Value(Value::Array{})},
                                                {"places", Value(Value::Array{place("slot-1", true), place("floppy", false)})}}))));
    CHECK_EQ(sessions::toText(*h.value().find("nameAndSave")),
             sessions::toText(Value::object({{"slot-1", "Instance name:"}, {"floppy", "Disk name:"}})));
}

TEST(model_host_takes_only_changed_inputs) {
    ModelHost h;
    CHECK(h.take(state("control-state", st::controlValue())));
    CHECK(!h.take(state("control-state", st::controlValue())));   // the same again
    CHECK(h.take(state("busy", Value::object({{"operationRuns", true}}))));
    CHECK(!h.take(state("display", Value())));   // no input of the shell's
}

namespace {

// A product with one item that follows the topic `trace`.
ModelSetup followingSetup() {
    ModelSetup s = setup();
    ProductItem trace{MenuGroup::MachineEnd, "trace", "Trace", "", {}};
    trace.command = sessions::CommandName{"trace"};
    trace.checkedBy = ProductItem::Check{sessions::TopicName{"trace"}, "running", Value(true)};
    s.product.items.push_back(trace);
    return s;
}

// The item `trace`, in the menus or a submenu of theirs: checked.
bool checkedIn(const Value::Array& items) {
    for (const auto& i : items)
        if (read::text(i, "id") == "trace" ? read::flag(i, "checked") : checkedIn(read::elements(i, "children"))) return true;
    return false;
}

bool traceChecked(const ModelHost& h) {
    const Value v = h.value();
    for (const auto& m : read::elements(v, "menus"))
        if (checkedIn(read::elements(m, "items"))) return true;
    return false;
}

} // namespace

TEST(model_host_follows_the_product_topics_from_set_on) {
    ModelHost h;
    CHECK(!h.take(state("trace", Value::object({{"running", true}}))));   // not followed before set()
    h.set(followingSetup(), {});
    CHECK(h.take(state("control-state", st::controlValue())));
    CHECK(!traceChecked(h));
    CHECK(h.take(state("trace", Value::object({{"running", true}}))));
    CHECK(traceChecked(h));
    CHECK(!h.take(state("trace", Value::object({{"running", true}}))));   // the same again
}

TEST(model_host_set_takes_the_followed_topics_that_came_before) {
    sessions::TopicMirror states;
    states.deliver(state("trace", Value::object({{"running", true}})));
    ModelHost h;
    h.set(followingSetup(), states);
    CHECK(h.take(state("control-state", st::controlValue())));
    CHECK(traceChecked(h));
}

TEST(take_state_keeps_the_shell_topics_and_the_followed_ones) {
    ShellInputs in;
    const InputNames names;
    auto take = [&](const char* topic, Value v) {
        return takeState(in, names, std::get<sessions::StateMessage>(state(topic, std::move(v))));
    };
    CHECK(take("media", Value::object({{"kinds", Value(Value::Array{})}})));
    CHECK(!in.media.isNull());
    CHECK(!take("trace", Value(true)));   // not followed
    CHECK(in.productTopics.empty());
    in.productTopics["trace"];
    CHECK(take("trace", Value(true)));
    CHECK(!take("trace", Value(true)));
    CHECK(in.productTopics["trace"] == Value(true));
}

TEST(model_host_asks_the_preset_list_at_start_and_after_settings) {
    ModelHost h;
    CHECK(h.presetListWanted());
    CHECK(!h.presetListWanted());
    CHECK(h.take(state("settings", Value::object({{"rows", Value(Value::Array{})}}))));
    CHECK(h.presetListWanted());
    sessions::ReplyMessage r;
    r.done = true;
    r.result = sessions::valuePayload("preset-list", st::presetList());
    h.takePresetList(r);
    h.set(setup(), {});
    CHECK(h.take(state("control-state", st::controlValue())));
    // The examples of the preset list are File ▸ Examples.
    const Value menus = *h.value().find("menus");
    const Value& file = menus.elements()->front();
    bool examples = false;
    for (const auto& i : read::elements(file, "items")) examples = examples || read::text(i, "id") == "examples";
    CHECK(examples);
}

// About is served with the page's build and its rows of where it runs: the
// page's `shell` args, read by takePageArgs; rows given again replace them.
TEST(model_host_serves_about_with_the_page_args) {
    ModelSetup s = setup();
    s.product.version = "0.0";
    auto row = [](const char* label, const char* value) { return Value(Value::Array{Value(label), Value(value)}); };
    takePageArgs(s, Value::object({{"version", "1.2.3"}, {"build", "abc123"}, {"buildTime", "2026-11-24T11:49:08Z"},
                                   {"host", Value(Value::Array{row("Browser", "Safari 27.2"), row("Hardware", "")})}}));
    ModelHost h;
    h.set(s, {});
    const Value v = h.value();
    const Value* a = v.find("about");
    CHECK(a != nullptr);
    CHECK_EQ(read::text(*a, "version"), std::string("Version 1.2.3 (abc123)"));
    CHECK_EQ(read::text(*a, "text"), aboutText(about(s.product, s.host)));
    const auto& sections = read::elements(*a, "sections");
    CHECK_EQ(sections.size(), std::size_t{1});
    CHECK_EQ(read::elements(sections[0], "rows").size(), std::size_t{2});   // Built, Browser; the empty Hardware left out
    auto size = [](int w, int h) { return Value::object({{"width", w}, {"height", h}}); };
    takePageArgs(s, Value::object({{"host", Value(Value::Array{row("OS", "macOS")})},
                                   {"display", Value::object({{"reported", size(1180, 820)}, {"scale", 2},
                                                              {"window", size(1180, 735)}})}}));
    CHECK_EQ(s.product.version, std::string("1.2.3"));   // not given: kept
    CHECK(s.host.size() == 1 && s.host[0].first == "OS");
    CHECK(s.display.reported.width == 1180 && s.display.scale == 2.0 && s.display.window.height == 735);
    takePageArgs(s, Value::object({{"display", Value::object({{"scale", 1.5}})}}));
    CHECK(s.display.scale == 1.5 && s.display.reported.width == 0);   // given again: replaced
    takePageArgs(s, Value::object({}));
    CHECK(s.display.scale == 0.0);
}
