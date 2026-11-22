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
    h.set(setup());
    CHECK(!h.value().isNull());
}

TEST(model_host_serves_the_menus_bar_panels_and_dialog) {
    ModelHost h;
    h.set(setup());
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

TEST(model_host_takes_only_changed_inputs) {
    ModelHost h;
    CHECK(h.take(state("control-state", st::controlValue())));
    CHECK(!h.take(state("control-state", st::controlValue())));   // the same again
    CHECK(!h.take(state("busy", Value::object({{"operationRuns", true}}))));   // no model reads it
    CHECK(!h.take(state("display", Value())));
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
    h.set(setup());
    CHECK(h.take(state("control-state", st::controlValue())));
    // The examples of the preset list are File ▸ Examples.
    const Value menus = *h.value().find("menus");
    const Value& file = menus.elements()->front();
    bool examples = false;
    for (const auto& i : read::elements(file, "items")) examples = examples || read::text(i, "id") == "examples";
    CHECK(examples);
}
