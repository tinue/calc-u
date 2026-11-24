#include "shell/ModelHost.hpp"
#include "shell/BarModel.hpp"
#include "shell/ValueForms.hpp"
#include <string>
#include <utility>

namespace calcu::shell {

void takePageArgs(ModelSetup& setup, const sessions::Value& args) {
    auto stamp = [&](std::string& field, const char* key) {
        if (std::string v = read::text(args, key); !v.empty()) field = std::move(v);
    };
    stamp(setup.product.version, "version");
    stamp(setup.product.build, "build");
    stamp(setup.product.buildTime, "buildTime");
    setup.host.clear();
    for (const auto& row : read::elements(args, "host")) {
        const auto* pair = row.elements();
        if (!pair || pair->size() != 2 || !(*pair)[0].string() || !(*pair)[1].string()) continue;
        setup.host.emplace_back(*(*pair)[0].string(), *(*pair)[1].string());
    }
    setup.display = {};
    if (const sessions::Value* d = args.find("display")) {
        auto size = [&](const char* key) -> DisplaySize {
            const sessions::Value* s = d->find(key);
            if (!s) return {};
            return {static_cast<int>(read::integer(*s, "width")), static_cast<int>(read::integer(*s, "height"))};
        };
        setup.display.windowing = read::text(*d, "windowing");
        setup.display.screen = read::text(*d, "screen");
        setup.display.reported = size("reported");
        setup.display.panel = size("panel");
        setup.display.window = size("window");
        if (const sessions::Value* scale = d->find("scale")) {
            if (scale->real()) setup.display.scale = *scale->real();
            else if (scale->integer()) setup.display.scale = static_cast<double>(*scale->integer());
        }
    }
}

ModelHost::ModelHost(InputNames names) : names_(std::move(names)) {}

void ModelHost::set(ModelSetup setup, const sessions::TopicMirror& states) {
    followProductTopics(inputs_, setup.product);
    setup_ = std::move(setup);
    for (auto& [name, value] : inputs_.productTopics)
        if (const sessions::PayloadPtr* p = states.state(sessions::TopicName{name}))
            value = *p ? (*p)->toValue() : sessions::Value();
}

bool ModelHost::take(const sessions::Message& m) {
    const auto* s = std::get_if<sessions::StateMessage>(&m);
    if (!s || !takeState(inputs_, names_, *s)) return false;
    if (s->topic == names_.settings) presetListWanted_ = true;
    return true;
}

void ModelHost::takePresetList(const sessions::ReplyMessage& r) {
    inputs_.presetList = r.done && r.result ? r.result->toValue() : sessions::Value();
}

bool ModelHost::presetListWanted() { return std::exchange(presetListWanted_, false); }

sessions::Value ModelHost::value() const {
    if (!setup_) return {};
    const ShellProduct& p = setup_->product;
    const Traits t = setup_->traits;
    return sessions::Value::object({{"menus", toValue(menus(inputs_, p, t))},
                                    {"bar", toValue(bar(inputs_, p, t))},
                                    {"panels", toValue(panels(inputs_, p))},
                                    {"dialog", toValue(dialog(inputs_.settings, inputs_.presetList, setup_->spec))},
                                    {"about", toValue(about(p, setup_->host, setup_->display))},
                                    {"nameAndSave", nameAndSavePrompts(p)}});
}

// Every place of the topic `media`, not only those that offer Name & Save
// now: an open dialog keeps its prompt when the place's offer changes.
sessions::Value ModelHost::nameAndSavePrompts(const ShellProduct& p) const {
    sessions::Value::Object out;
    for (const auto& place : read::elements(inputs_.media, "places")) {
        const std::string name = read::text(place, "place");
        out.push_back({name, sessions::Value(nameAndSavePrompt(p, name))});
    }
    return sessions::Value(std::move(out));
}

} // namespace calcu::shell
